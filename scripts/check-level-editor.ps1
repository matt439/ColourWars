[CmdletBinding()]
param(
    [ValidateSet('x64-debug', 'x64-release')]
    [string] $Preset = 'x64-debug',
    [switch] $SkipBuild
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Invoke-Logged {
    param(
        [string] $Command,
        [string[]] $Arguments,
        [string] $LogPath
    )

    # Windows PowerShell wraps native stderr in ErrorRecord. The process exit
    # code decides success; retain both streams in the evidence directory.
    $ErrorActionPreference = 'Continue'
    $output = & $Command @Arguments 2>&1
    $code = $LASTEXITCODE
    $output | Out-File -LiteralPath $LogPath -Encoding utf8
    $output | ForEach-Object { Write-Host $_ }
    if ($code -ne 0) {
        throw "$Command exited with $code. See $LogPath"
    }
}

function Test-EditorWindow {
    param([string] $Executable, [string] $Level, [string] $RunDirectory)

    if (-not ('ColourWars.EditorWindowProbe' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Text;

namespace ColourWars
{
    public static class EditorWindowProbe
    {
        private delegate bool EnumWindowProc(IntPtr window, IntPtr parameter);
        [DllImport("user32.dll")]
        private static extern bool EnumWindows(EnumWindowProc callback, IntPtr parameter);
        [DllImport("user32.dll")]
        private static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);
        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetClassName(IntPtr window, StringBuilder name, int maximum);
        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetWindowText(IntPtr window, StringBuilder title, int maximum);
        [DllImport("user32.dll")]
        private static extern bool PostMessage(IntPtr window, uint message, IntPtr wParam, IntPtr lParam);
        [DllImport("user32.dll")]
        private static extern IntPtr SendMessageTimeout(IntPtr window, uint message,
            IntPtr wParam, IntPtr lParam, uint flags, uint timeout, out IntPtr result);
        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern IntPtr FindWindowEx(IntPtr parent, IntPtr after, string name, string title);
        [DllImport("user32.dll")]
        private static extern int GetDlgCtrlID(IntPtr window);
        [DllImport("user32.dll")]
        private static extern IntPtr GetMenu(IntPtr window);
        [DllImport("user32.dll")]
        private static extern IntPtr GetSubMenu(IntPtr menu, int position);
        [DllImport("user32.dll")]
        private static extern uint GetMenuItemID(IntPtr menu, int position);
        private static void Send(IntPtr window, uint message, IntPtr wparam, IntPtr lparam)
        {
            IntPtr result;
            if (SendMessageTimeout(window, message, wparam, lparam, 2, 2000, out result) == IntPtr.Zero)
                throw new InvalidOperationException("Editor UI command timed out");
        }

        public static void CheckBlockoutControls(IntPtr window)
        {
            IntPtr list = FindWindowEx(window, IntPtr.Zero, "ListBox", null);
            IntPtr canvas = FindWindowEx(window, IntPtr.Zero, "ColourWarsEditorCanvas", null);
            if (list == IntPtr.Zero || canvas == IntPtr.Zero)
                throw new InvalidOperationException("Editor controls missing");
            // Entry order is level, four bounds, then team A spawns. Select
            // the first spawn through the same notification as a list click.
            Send(list, 0x0186, new IntPtr(5), IntPtr.Zero); // LB_SETCURSEL
            Send(window, 0x0111, new IntPtr(GetDlgCtrlID(list) | (1 << 16)), list);
            IntPtr view = GetSubMenu(GetMenu(window), 2);
            // Toggle off/on (the latter reloads source tuning), then frame it.
            uint aids = GetMenuItemID(view, 4);
            Send(window, 0x0111, new IntPtr(aids), IntPtr.Zero);
            Send(window, 0x0111, new IntPtr(aids), IntPtr.Zero);
            Send(window, 0x0111, new IntPtr(GetMenuItemID(view, 1)), IntPtr.Zero);
        }

        public static IntPtr Find(int processId)
        {
            IntPtr result = IntPtr.Zero;
            EnumWindows((window, parameter) => {
                uint owner;
                GetWindowThreadProcessId(window, out owner);
                if (owner != processId) return true;
                var name = new StringBuilder(256);
                GetClassName(window, name, name.Capacity);
                if (name.ToString() == "ColourWarsLevelEditor") result = window;
                return true;
            }, IntPtr.Zero);
            return result;
        }

        public static string Title(IntPtr window)
        {
            var title = new StringBuilder(4096);
            GetWindowText(window, title, title.Capacity);
            return title.ToString();
        }

        public static bool Responds(IntPtr window)
        {
            IntPtr result;
            return SendMessageTimeout(window, 0, IntPtr.Zero, IntPtr.Zero,
                2, 2000, out result) != IntPtr.Zero;
        }

        public static void Close(int processId)
        {
            EnumWindows((window, parameter) => {
                uint owner;
                GetWindowThreadProcessId(window, out owner);
                if (owner == processId) PostMessage(window, 0x0010, IntPtr.Zero, IntPtr.Zero);
                return true;
            }, IntPtr.Zero);
        }
    }
}
'@
    }

    $working = Join-Path $RunDirectory 'unrelated-working-directory'
    [System.IO.Directory]::CreateDirectory($working) | Out-Null
    $process = $null
    $before = (Get-FileHash -LiteralPath $Level -Algorithm SHA256).Hash
    $result = [ordered]@{ outcome = 'failed'; level = $Level; window_title = ''; exit_code = $null }
    try {
        if ($Level.Contains('"')) { throw 'The level path contains an unsupported quote.' }
        $process = Start-Process -FilePath $Executable -ArgumentList ('"' + $Level + '"') `
            -WorkingDirectory $working -WindowStyle Hidden -PassThru `
            -RedirectStandardError (Join-Path $RunDirectory 'gui.err') `
            -RedirectStandardOutput (Join-Path $RunDirectory 'gui.out')
        $null = $process.Handle
        $timer = [Diagnostics.Stopwatch]::StartNew()
        $window = [IntPtr]::Zero
        while ($timer.Elapsed.TotalSeconds -lt 15) {
            $process.Refresh()
            if ($process.HasExited) { throw "Editor exited during startup (code $($process.ExitCode))." }
            $window = [ColourWars.EditorWindowProbe]::Find($process.Id)
            if ($window -ne [IntPtr]::Zero -and [ColourWars.EditorWindowProbe]::Responds($window) -and
                [ColourWars.EditorWindowProbe]::Title($window).Contains([System.IO.Path]::GetFileName($Level))) {
                break
            }
            Start-Sleep -Milliseconds 100
        }
        if ($window -eq [IntPtr]::Zero -or -not [ColourWars.EditorWindowProbe]::Responds($window)) {
            throw 'The editor did not create a responsive ColourWarsLevelEditor window.'
        }
        $result.window_title = [ColourWars.EditorWindowProbe]::Title($window)
        if (-not $result.window_title.Contains([System.IO.Path]::GetFileName($Level))) {
            throw "The editor title does not identify the requested file: $($result.window_title)"
        }
        [ColourWars.EditorWindowProbe]::CheckBlockoutControls($window)
        [ColourWars.EditorWindowProbe]::Close($process.Id)
        if (-not $process.WaitForExit(10000)) { throw 'The editor did not close gracefully after WM_CLOSE.' }
        $process.WaitForExit()
        $result.exit_code = $process.ExitCode
        if ($process.ExitCode -ne 0) { throw "Editor failed during shutdown (code $($process.ExitCode))." }
        if ((Get-FileHash -LiteralPath $Level -Algorithm SHA256).Hash -ne $before) {
            throw 'Opening and closing the editor changed the supplied file.'
        }
        $diagnostics = [string](Get-Content -LiteralPath (Join-Path $RunDirectory 'gui.err') -Raw)
        if (-not [string]::IsNullOrWhiteSpace($diagnostics)) { throw "Unexpected editor diagnostic: $diagnostics" }
        $result.outcome = 'passed'
        Write-Host 'PASS hidden editor launch, explicit level, blockout toggle/selection and graceful close'
    }
    finally {
        if ($null -ne $process -and -not $process.HasExited) {
            [ColourWars.EditorWindowProbe]::Close($process.Id)
            if (-not $process.WaitForExit(3000)) {
                # This process was created solely by the probe and holds no edits.
                $process.Kill()
                $process.WaitForExit()
            }
        }
        $result | ConvertTo-Json | Out-File -LiteralPath (Join-Path $RunDirectory 'gui-result.json') -Encoding utf8
        if ($null -ne $process) { $process.Dispose() }
    }
}

$repo = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$build = Join-Path $repo "out/build/$Preset"
$evidence = Join-Path $repo "out/acceptance/$Preset/level-editor"
$run = Join-Path $evidence ([Guid]::NewGuid().ToString('N'))
[System.IO.Directory]::CreateDirectory($run) | Out-Null

Push-Location -LiteralPath $repo
try {
    if (-not $SkipBuild) {
        foreach ($command in 'cmake', 'cl') {
            if (-not (Get-Command $command -ErrorAction SilentlyContinue)) {
                throw "Run from an x64 Visual Studio Developer shell ($command is missing)."
            }
        }
        if (-not $env:INCLUDE -or -not $env:LIB -or
            ($env:VSCMD_ARG_TGT_ARCH -and $env:VSCMD_ARG_TGT_ARCH -ne 'x64')) {
            throw 'Run from an x64 Visual Studio Developer shell.'
        }
        Invoke-Logged -Command 'cmake' -Arguments @('--preset', $Preset) `
            -LogPath (Join-Path $run 'configure.log')
        Invoke-Logged -Command 'cmake' `
            -Arguments @('--build', '--preset', $Preset, '--target', 'colourwars_editor_acceptance', 'ColourWarsEditor') `
            -LogPath (Join-Path $run 'build.log')
    }

    $cachePath = Join-Path $build 'CMakeCache.txt'
    if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf)) {
        throw "No configured build at $build. Run without -SkipBuild first."
    }
    $cache = @{}
    foreach ($line in Get-Content -LiteralPath $cachePath) {
        if ($line -match '^([^/#][^:]*):[^=]*=(.*)$') {
            $cache[$Matches[1]] = $Matches[2]
        }
    }
    foreach ($key in 'CMAKE_HOME_DIRECTORY', 'CMAKE_BUILD_TYPE') {
        if (-not $cache.ContainsKey($key)) {
            throw "The configured build is missing $key. Configure it again."
        }
    }
    if ([System.IO.Path]::GetFullPath($cache.CMAKE_HOME_DIRECTORY) -ne $repo) {
        throw 'The configured build belongs to another checkout.'
    }
    $configuration = if ($Preset -eq 'x64-debug') { 'Debug' } else { 'Release' }
    if ($cache.CMAKE_BUILD_TYPE -ne $configuration) {
        throw "$Preset must contain a $configuration build."
    }
    $executable = Join-Path $build 'editor/colourwars_editor_acceptance.exe'
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
        throw "Missing editor acceptance executable: $executable. Run without -SkipBuild first."
    }

    Invoke-Logged -Command $executable -Arguments @($repo, $run) `
        -LogPath (Join-Path $run 'run.log')
    $editor = Join-Path $build 'editor/ColourWarsEditor.exe'
    if (-not (Test-Path -LiteralPath $editor -PathType Leaf)) {
        throw "Missing editor executable: $editor. Run without -SkipBuild first."
    }
    Test-EditorWindow -Executable $editor -Level (Join-Path $run 'king_of_the_hill.json') -RunDirectory $run
    $courseRun = Join-Path $run 'movement-course'
    [System.IO.Directory]::CreateDirectory($courseRun) | Out-Null
    Test-EditorWindow -Executable $editor -Level (Join-Path $run 'movement_course.json') -RunDirectory $courseRun
    Write-Host "PASS level editor acceptance ($Preset). Evidence: $run"
}
finally {
    Pop-Location
}
