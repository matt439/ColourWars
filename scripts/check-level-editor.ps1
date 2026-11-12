[CmdletBinding()]
param(
    [ValidateSet('x64-debug', 'x64-release', 'x64-asan')]
    [string] $Preset = 'x64-debug',
    [switch] $SkipBuild
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'check-windows.ps1')

# Load standard helpers from this host before nested native-probe setup.
Import-Module (Join-Path $PSHOME 'Modules/Microsoft.PowerShell.Utility/Microsoft.PowerShell.Utility.psd1') -ErrorAction Stop

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

function Invoke-EditorPendingProbe {
    param(
        [string] $Command,
        [string[]] $NativeArguments,
        [string] $LogPath
    )

    $pendingControl = $null
    $pendingCommand = $null
    $pendingResult = $null
    $pendingFailure = $null
    $pendingInterrupted = $false
    $pendingStartAttempted = $false
    $pendingDisposalErrors = [Collections.Generic.List[string]]::new()
    try {
        # Only this new modal probe needs a runtime deadline and private job.
        . (Join-Path $PSScriptRoot 'acceptance-process.ps1')
        Initialize-AcceptanceProcess
        $pendingControl = [ColourWars.Acceptance.OwnedRunControl]::new()
        $pendingStartAttempted = $true
        $pendingCommand = [ColourWars.Acceptance.OwnedCommand]::Start(
            $Command, [string[]]$NativeArguments, $repo, $LogPath,
            [Console]::OutputEncoding.CodePage, 120, $pendingControl)
        while (-not $pendingCommand.Poll(100)) { }
    } catch [System.Management.Automation.PipelineStoppedException] {
        $pendingInterrupted = $true
        throw
    } catch {
        $pendingFailure = $_
    } finally {
        try {
            if ($null -ne $pendingControl) { $pendingControl.Cancel() }
        } catch {
            $pendingDisposalErrors.Add("Cancel: $($_.Exception.Message)")
        }
        try {
            if ($null -ne $pendingCommand) { $pendingCommand.Dispose() }
        } catch {
            $pendingDisposalErrors.Add("Command disposal: $($_.Exception.Message)")
        }
        try {
            if ($null -ne $pendingControl) { $pendingControl.Dispose() }
        } catch {
            $pendingDisposalErrors.Add("Control disposal: $($_.Exception.Message)")
        }
        if ($null -ne $pendingCommand) { $pendingResult = $pendingCommand.Result }
        # Dispose first: cancellation also needs the terminal outcome and its
        # cleanup proof. The raw sidecar remains owned by the existing helper.
        $pendingReceipt = [ordered]@{
            schema = 1
            scope = 'editor-pending-property-modal-probe'
            command = $Command
            arguments = @($NativeArguments)
            working_directory = $repo
            deadline_seconds = 120
            outcome = if ($null -ne $pendingResult) { $pendingResult.Outcome } elseif ($null -ne $pendingCommand) {
                'cleanup_uncertain'
            } elseif ($pendingStartAttempted) {
                'start_outcome_unknown'
            } else { 'not_started' }
            start_attempted = $pendingStartAttempted
            exit_code = if ($null -ne $pendingResult) { $pendingResult.ExitCode } else { $null }
            process_id = if ($null -ne $pendingResult) { $pendingResult.ProcessId } else { $null }
            cleanup_completed = if ($null -ne $pendingResult) { $pendingResult.CleanupCompleted } else { $false }
            elapsed_ms = if ($null -ne $pendingResult) { $pendingResult.ElapsedMilliseconds } else { $null }
            log_path = $LogPath
            raw_log_path = $LogPath + '.raw'
            message = if ($null -ne $pendingResult) { $pendingResult.Message } else { '' }
            runner_error = if ($null -ne $pendingFailure) { $pendingFailure.Exception.Message } else { '' }
            runner_interrupted = $pendingInterrupted
            disposal_errors = @($pendingDisposalErrors.ToArray())
        }
        $pendingJson = ConvertTo-Json -InputObject $pendingReceipt -Depth 4
        [IO.File]::WriteAllText($LogPath + '.process.json', $pendingJson, [Text.UTF8Encoding]::new($false))
    }
    if ([IO.File]::Exists($LogPath)) {
        Get-Content -LiteralPath $LogPath -Encoding utf8 | ForEach-Object { Write-Host $_ }
    }
    if ($null -ne $pendingFailure -or $pendingDisposalErrors.Count -ne 0 -or
        $null -eq $pendingResult -or $pendingResult.Outcome -cne 'exited' -or
        $null -eq $pendingResult.ExitCode -or $pendingResult.ExitCode -ne 0 -or -not $pendingResult.CleanupCompleted) {
        # CI retains its console even when private acceptance files are not uploaded.
        foreach ($name in @('', 'pending-properties.csv', 'pending-dialogs.csv',
            'pending-driver-cleanup.csv', 'pending-environment.csv')) {
            try {
                $pendingPath = if ($name) { Join-Path $NativeArguments[0] $name } else { $LogPath + '.process.json' }
                Write-Host "Pending-property failure evidence: $pendingPath"
                if ([IO.File]::Exists($pendingPath)) {
                    Get-Content -LiteralPath $pendingPath -Encoding utf8 | ForEach-Object { Write-Host $_ }
                } else { Write-Host '(not written)' }
            } catch { Write-Host "Could not read pending-property failure evidence: $($_.Exception.Message)" }
        }
    }
    if ($null -ne $pendingFailure) { throw $pendingFailure }
    if ($pendingDisposalErrors.Count -ne 0) {
        throw "Pending-property probe disposal failed. See $LogPath.process.json"
    }
    if ($null -eq $pendingResult -or $pendingResult.Outcome -cne 'exited' -or
        $null -eq $pendingResult.ExitCode -or $pendingResult.ExitCode -ne 0 -or -not $pendingResult.CleanupCompleted) {
        throw "Pending-property probe did not exit successfully with confirmed cleanup. See $LogPath.process.json"
    }
}

function Test-EditorWindow {
    param([string] $Executable, [string] $Level, [string] $RunDirectory, [string] $ExpectStatus = '',
        [Parameter(Mandatory)] [hashtable] $Commands)

    if (-not ('ColourWars.EditorWindowProbe' -as [type])) {
        Add-Type -TypeDefinition (@'
using System;
using System.Runtime.InteropServices;
using System.Text;

namespace ColourWars
{
    public static class EditorWindowProbe
    {
        private delegate bool EnumWindowProc(IntPtr window, IntPtr parameter);
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
        private static extern bool EnumChildWindows(IntPtr parent, EnumWindowProc callback, IntPtr parameter);
        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern IntPtr SendMessageTimeout(IntPtr window, uint message,
            IntPtr wParam, StringBuilder lParam, uint flags, uint timeout, out IntPtr result);
        [DllImport("user32.dll")]
        private static extern IntPtr GetMenu(IntPtr window);
        [DllImport("user32.dll")]
        private static extern uint GetMenuState(IntPtr menu, uint item, uint flags);
        private static void Send(IntPtr window, uint message, IntPtr wparam, IntPtr lparam)
        {
            IntPtr result;
            if (SendMessageTimeout(window, message, wparam, lparam, 2, 2000, out result) == IntPtr.Zero)
                throw new InvalidOperationException("Editor UI command timed out");
        }

        // A menu item by its command id; GetMenuState searches the submenus.
        private static void MenuCommand(IntPtr window, uint command)
        {
            if (GetMenuState(GetMenu(window), command, 0) == 0xffffffff)
                throw new InvalidOperationException("Editor menu has no command " + command);
            Send(window, 0x0111, new IntPtr(command), IntPtr.Zero);
        }

        public static void CheckBlockoutControls(IntPtr window, uint blockout, uint frameSelection)
        {
            IntPtr list = FindWindowEx(window, IntPtr.Zero, "ListBox", null);
            IntPtr canvas = FindWindowEx(window, IntPtr.Zero, "ColourWarsEditorCanvas", null);
            if (list == IntPtr.Zero || canvas == IntPtr.Zero)
                throw new InvalidOperationException("Editor controls missing");
            // Entry order is level, four bounds, then team A spawns. Select
            // the first spawn through the same notification as a list click.
            Send(list, 0x0186, new IntPtr(5), IntPtr.Zero); // LB_SETCURSEL
            Send(window, 0x0111, new IntPtr(GetDlgCtrlID(list) | (1 << 16)), list);
            // Toggle Blockout aids off/on (the latter reloads source tuning), then frame it.
            MenuCommand(window, blockout);
            MenuCommand(window, blockout);
            MenuCommand(window, frameSelection);
        }

        // The status line is the Static control that starts with Ready or Needs
        // attention. WM_GETTEXT is marshalled across processes; GetWindowText is not.
        public static string Status(IntPtr window)
        {
            string result = "";
            EnumChildWindows(window, (child, parameter) => {
                var name = new StringBuilder(256);
                GetClassName(child, name, name.Capacity);
                if (name.ToString() != "Static") return true;
                var text = new StringBuilder(4096);
                IntPtr copied;
                if (SendMessageTimeout(child, 0x000D, new IntPtr(text.Capacity), text, 2, 2000, out copied) == IntPtr.Zero)
                    return true; // WM_GETTEXT
                string value = text.ToString();
                if (value.StartsWith("Ready") || value.StartsWith("Needs attention")) result = value;
                return true;
            }, IntPtr.Zero);
            return result;
        }

        public static IntPtr Find(int processId)
        {
            return EditorOwnedWindows.Find(processId, "ColourWarsLevelEditor");
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
            EditorOwnedWindows.Close(processId);
        }
    }
}
'@ + (Get-CheckWindowSource 'EditorOwnedWindows'))
    }

    $working = Join-Path $RunDirectory 'unrelated-working-directory'
    [System.IO.Directory]::CreateDirectory($working) | Out-Null
    $process = $null
    $before = (Get-FileHash -LiteralPath $Level -Algorithm SHA256).Hash
    $result = [ordered]@{ outcome = 'failed'; level = $Level; window_title = ''; status = ''; exit_code = $null }
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
        [ColourWars.EditorWindowProbe]::CheckBlockoutControls($window, $Commands.view_blockout, $Commands.view_selection)
        $result.status = [ColourWars.EditorWindowProbe]::Status($window)
        if (-not $result.status.StartsWith('Ready')) {
            throw "The editor status line is not ready: $($result.status)"
        }
        if ($ExpectStatus -and -not $result.status.Contains($ExpectStatus)) {
            throw "The editor status line lacks '$ExpectStatus': $($result.status)"
        }
        if (-not $ExpectStatus -and $result.status.Contains('the game will refuse')) {
            throw "The editor reports an unresolved name for a shipped level: $($result.status)"
        }
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
        Write-Host 'PASS hidden editor launch, explicit level, blockout toggle/selection, status line and graceful close'
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

# The editor's menu command ids, read from its Command enum (editor/editor.h),
# so the window checks address a menu item by what it does, not its position.
function Get-EditorCommandIds([string] $Header) {
    $declaration = [regex]::Match([IO.File]::ReadAllText($Header), 'enum\s+Command\s*:\s*int\s*\{([^}]*)\}')
    if (-not $declaration.Success) { throw "No Command enum in $Header" }
    $ids = @{}
    $next = 0
    foreach ($entry in $declaration.Groups[1].Value -split ',') {
        $name = $entry.Trim()
        if ($name -eq '') { continue }
        if ($name -match '^(\w+)\s*=\s*(\d+)$') { $name = $Matches[1]; $next = [int]$Matches[2] }
        elseif ($name -notmatch '^\w+$') { throw "Unreadable editor command '$name' in $Header" }
        $ids[$name] = $next
        ++$next
    }
    foreach ($needed in 'file_save', 'file_save_as', 'file_save_user_stage', 'view_selection', 'view_blockout') {
        if (-not $ids.ContainsKey($needed)) { throw "Editor Command enum lacks $needed" }
    }
    return $ids
}
$editorCommands = Get-EditorCommandIds (Join-Path $repo 'editor/editor.h')
$build = Join-Path $repo "out/build/$Preset"
$evidence = Join-Path $repo "out/acceptance/$Preset/level-editor"
$run = Join-Path $evidence ([Guid]::NewGuid().ToString('N'))
[System.IO.Directory]::CreateDirectory($run) | Out-Null
# Save to My Stages writes under LOCALAPPDATA: point it, for the acceptance
# executable and every editor launch, at a profile inside this run.
$previousLocalAppData = $env:LOCALAPPDATA
$previousPath = $env:PATH
$isolatedProfile = Join-Path $run 'profile'
[System.IO.Directory]::CreateDirectory($isolatedProfile) | Out-Null

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
            -Arguments @('--build', '--preset', $Preset, '--target', 'colourwars_editor_acceptance', 'ColourWarsEditor', 'colourwars_editor_dpi_acceptance', '--parallel', '1') `
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
    # x64-asan is RelWithDebInfo with every target of this tree instrumented (HYG#99).
    $asan = $Preset -eq 'x64-asan'
    $configuration = if ($Preset -eq 'x64-debug') { 'Debug' } elseif ($asan) { 'RelWithDebInfo' } else { 'Release' }
    if ($cache.CMAKE_BUILD_TYPE -ne $configuration) {
        throw "$Preset must contain a $configuration build."
    }
    if ($asan) {
        if (-not $cache.ContainsKey('CMAKE_CXX_FLAGS') -or -not $cache.ContainsKey('CMAKE_CXX_COMPILER') -or
            @($cache.CMAKE_CXX_FLAGS -split '\s+') -cnotcontains '/fsanitize=address') {
            throw 'The x64-asan build was not configured with /fsanitize=address.'
        }
        # The runtime DLL ships beside the compiler that built the editor and
        # must be that toolset's, whichever one the caller's PATH finds first.
        $sanitizerRuntimeDirectory = Split-Path -Parent $cache.CMAKE_CXX_COMPILER
        if (-not (Test-Path -LiteralPath (Join-Path $sanitizerRuntimeDirectory 'clang_rt.asan_dynamic-x86_64.dll') -PathType Leaf)) {
            throw "Missing the sanitizer runtime beside $($cache.CMAKE_CXX_COMPILER). Install Visual Studio's 'C++ AddressSanitizer' component."
        }
        $env:PATH = "$sanitizerRuntimeDirectory;$previousPath"
    }
    $executable = Join-Path $build 'editor/colourwars_editor_acceptance.exe'
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
        throw "Missing editor acceptance executable: $executable. Run without -SkipBuild first."
    }

    # Build tools retain the caller's dependency cache; every runtime check is isolated.
    $env:LOCALAPPDATA = $isolatedProfile
    Invoke-Logged -Command $executable -Arguments @($repo, $run) `
        -LogPath (Join-Path $run 'run.log')
    $dpiProbe = Join-Path $build 'editor/colourwars_editor_dpi_acceptance.exe'
    if (-not (Test-Path -LiteralPath $dpiProbe -PathType Leaf)) {
        throw "Missing editor DPI executable: $dpiProbe. Run without -SkipBuild first."
    }
    Invoke-Logged -Command $dpiProbe -Arguments @((Join-Path $run 'dpi-window')) `
        -LogPath (Join-Path $run 'dpi-window.log')
    Invoke-Logged -Command $dpiProbe -Arguments @((Join-Path $run 'canvas-gestures'), '--gestures') `
        -LogPath (Join-Path $run 'canvas-gestures.log')
    Invoke-EditorPendingProbe -Command $dpiProbe `
        -NativeArguments @((Join-Path $run 'pending-properties'), '--pending') `
        -LogPath (Join-Path $run 'pending-properties.log')
    $editor = Join-Path $build 'editor/ColourWarsEditor.exe'
    if (-not (Test-Path -LiteralPath $editor -PathType Leaf)) {
        throw "Missing editor executable: $editor. Run without -SkipBuild first."
    }
    Test-EditorWindow -Executable $editor -Level (Join-Path $run 'king_of_the_hill.json') -RunDirectory $run -Commands $editorCommands
    $courseRun = Join-Path $run 'movement-course'
    [System.IO.Directory]::CreateDirectory($courseRun) | Out-Null
    Test-EditorWindow -Executable $editor -Level (Join-Path $run 'movement_course.json') -RunDirectory $courseRun -Commands $editorCommands
    # A stage the game would refuse opens normally and names the bad field in the status line.
    $unresolvedRun = Join-Path $run 'unresolved-icon'
    [System.IO.Directory]::CreateDirectory($unresolvedRun) | Out-Null
    Test-EditorWindow -Executable $editor -Level (Join-Path $run 'unresolved_icon.json') -RunDirectory $unresolvedRun `
        -ExpectStatus "1 name the game will refuse: icon_frame 'not_an_icon'" -Commands $editorCommands
    if (Test-Path -LiteralPath (Join-Path $isolatedProfile 'ColourWars/save_data.json')) {
        throw 'The editor wrote a player save into the isolated profile.'
    }
    if (Test-Path -LiteralPath (Join-Path $isolatedProfile 'ColourWars/stages')) {
        throw 'Opening the editor created the user stage folder; only Save to My Stages may.'
    }
    . (Join-Path $PSScriptRoot 'check-level-editor-save-guidance.ps1')
    # The model check owns save-guidance/; dialog cancellation needs fresh targets.
    Test-EditorSaveGuidance -Executable $editor -Level (Join-Path $run 'king_of_the_hill.json') `
        -RunDirectory (Join-Path $run 'save-guidance-window') -SourceLevel (Join-Path $repo 'game/content/levels/king_of_the_hill.json') -Commands $editorCommands
    Write-Host "PASS level editor acceptance ($Preset). Evidence: $run"
}
finally {
    $env:LOCALAPPDATA = $previousLocalAppData
    $env:PATH = $previousPath
    Pop-Location
}
