[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,
    [string]$OutputDirectory,
    [ValidateRange(1, 120)]
    [int]$StartupSeconds = 15,
    [switch]$RequireNoAudio,
    [switch]$RequireAudio,
    [switch]$SystemOnlyPath,
    # Release has no WARP fallback. Only its exact missing-adapter diagnostic
    # can be skipped; the default remains a required hardware startup check.
    [switch]$AllowNoHardwareAdapter,
    # ROAD#16: seed the isolated profile's stage folder with one valid user
    # stage and one broken file. Startup must go on, with exactly one
    # refusal line for the broken file and no other diagnostic.
    [switch]$UserStageFixture
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'check-path.ps1')

# Windows PowerShell 5.1 does not set PSScriptRoot during parameter defaults.
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $PSScriptRoot '../out/validation/startup'
}

$executablePath = (Resolve-Path -LiteralPath $Executable).Path
if (-not (Test-Path -LiteralPath $executablePath -PathType Leaf)) {
    throw "Executable is not a file: $executablePath"
}
$executableDirectory = Split-Path -Parent $executablePath
if ($RequireNoAudio -and $RequireAudio) {
    throw 'Choose either the audible bank check or the missing-bank check.'
}
if ($RequireAudio -and -not (Test-Path -LiteralPath (
    Join-Path $executableDirectory 'sounds/paint_exe.xwb'))) {
    throw 'The startup check requires the accepted wave bank.'
}
if ($RequireNoAudio -and (Test-Path -LiteralPath (
    Join-Path $executableDirectory 'sounds/paint_exe.xwb'))) {
    throw 'The startup check requires an absent optional wave bank.'
}

$outputRoot = Resolve-CheckFileSystemPath $OutputDirectory
$runDirectory = Join-Path $outputRoot ([Guid]::NewGuid().ToString('N'))
$workingDirectory = Join-Path $runDirectory 'working-directory'
$profileDirectory = Join-Path $runDirectory 'profile'
$null = New-Item -ItemType Directory -Path $workingDirectory, $profileDirectory -Force
$stderrPath = Join-Path $runDirectory 'startup.err'
$userStageRefusal = $null
if ($UserStageFixture) {
    $userStages = Join-Path $profileDirectory 'ColourWars/stages'
    $null = New-Item -ItemType Directory -Path $userStages -Force
    Copy-Item -LiteralPath (Join-Path $executableDirectory 'levels/close_quarters.json') `
        -Destination (Join-Path $userStages 'startup_user_stage.json')
    [IO.File]::WriteAllText((Join-Path $userStages 'startup_broken.json'), '{ "display_name": ')
    $userStageRefusal = '^user stage refused: startup_broken\.json: .*not valid JSON'
}
$stdoutPath = Join-Path $runDirectory 'startup.out'

if (-not ('ColourWars.StartupWindow' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Text;

namespace ColourWars
{
    public static class StartupWindow
    {
        private delegate bool EnumWindowProc(IntPtr window, IntPtr parameter);
        [DllImport("user32.dll")]
        private static extern bool EnumWindows(EnumWindowProc callback, IntPtr parameter);
        [DllImport("user32.dll")]
        private static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);
        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetClassName(IntPtr window, StringBuilder name, int maximum);
        [DllImport("user32.dll", SetLastError = true)]
        private static extern bool PostMessage(IntPtr window, uint message, IntPtr wParam, IntPtr lParam);
        [DllImport("user32.dll", SetLastError = true)]
        private static extern IntPtr SendMessageTimeout(IntPtr window, uint message,
            IntPtr wParam, IntPtr lParam, uint flags, uint timeout, out IntPtr result);
        [DllImport("shcore.dll")]
        private static extern int GetProcessDpiAwareness(IntPtr process, out int value);
        [DllImport("user32.dll")]
        private static extern IntPtr GetWindowDpiAwarenessContext(IntPtr window);
        [DllImport("user32.dll")]
        private static extern bool AreDpiAwarenessContextsEqual(IntPtr a, IntPtr b);

        // PROCESS_DPI_AWARENESS: 0 unaware, 1 system, 2 per-monitor; -1 if the
        // query itself failed.
        public static int ProcessDpiAwareness(IntPtr process)
        {
            int value;
            return GetProcessDpiAwareness(process, out value) == 0 ? value : -1;
        }

        // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 is the pseudo-handle -4;
        // per-monitor v1 reports the same process value, so only the window
        // can tell the two apart.
        public static bool PerMonitorV2(IntPtr window)
        {
            return AreDpiAwarenessContextsEqual(
                GetWindowDpiAwarenessContext(window), new IntPtr(-4));
        }

        public static IntPtr FindGame(int processId)
        {
            IntPtr result = IntPtr.Zero;
            EnumWindows((window, parameter) => {
                uint owner;
                GetWindowThreadProcessId(window, out owner);
                if (owner != processId) return true;
                var name = new StringBuilder(256);
                GetClassName(window, name, name.Capacity);
                if (name.ToString() != "ColourWarsWindowClass") return true;
                result = window;
                return false;
            }, IntPtr.Zero);
            return result;
        }

        public static bool Responds(IntPtr window)
        {
            IntPtr result;
            return SendMessageTimeout(window, 0, IntPtr.Zero, IntPtr.Zero,
                2, 2000, out result) != IntPtr.Zero;
        }

        public static void CloseOwnedWindows(int processId)
        {
            // Also dismiss an exception dialog, without touching another process.
            EnumWindows((window, parameter) => {
                uint owner;
                GetWindowThreadProcessId(window, out owner);
                if (owner == processId)
                    PostMessage(window, 0x0010, IntPtr.Zero, IntPtr.Zero);
                return true;
            }, IntPtr.Zero);
        }
    }
}
'@
}

# game/settings.manifest declares per-monitor DPI awareness; a manifest that
# stopped being embedded, or lost the element, would leave the game bitmap-
# stretched on every scaled display with nothing else failing.
function Assert-DpiAware {
    $awareness = [ColourWars.StartupWindow]::ProcessDpiAwareness($process.Handle)
    $result.dpi_awareness = $awareness
    if ($awareness -ne 2) {
        throw "The game process is not per-monitor DPI aware (PROCESS_DPI_AWARENESS $awareness); check game/settings.manifest is embedded."
    }
}

function Read-StartupError {
    if (Test-Path -LiteralPath $stderrPath) {
        return [string](Get-Content -LiteralPath $stderrPath -Raw)
    }
    return ''
}

$process = $null
$originalLocalAppData = $env:LOCALAPPDATA
$originalPath = $env:PATH
$result = [ordered]@{
    executable = $executablePath
    working_directory = $workingDirectory
    profile = $profileDirectory
    version = ''
    dpi_awareness = $null
    audio_expectation = $(if ($RequireAudio) { 'published-bank' } elseif ($RequireNoAudio) { 'missing-bank' } else { 'optional' })
    outcome = 'failed'
    diagnostic = ''
}
$failure = $null
try {
    # game/resource.rc stamps project(VERSION) from the root CMakeLists.txt.
    $project = Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot '../CMakeLists.txt')
    if ($project -notmatch 'project\(ColourWars VERSION (\d+\.\d+\.\d+)') {
        throw 'The root CMakeLists.txt does not declare project(ColourWars VERSION x.y.z).'
    }
    $expectedVersion = $Matches[1]
    $version = (Get-Item -LiteralPath $executablePath).VersionInfo
    $result.version = $version.ProductVersion
    if ($version.ProductName -cne 'ColourWars' -or $version.CompanyName -cne 'Matt Harding' -or
        $version.FileVersion -cne $expectedVersion -or $version.ProductVersion -cne $expectedVersion -or
        "$($version.FileMajorPart).$($version.FileMinorPart).$($version.FileBuildPart)" -cne $expectedVersion) {
        throw "The executable's version resource is not stamped $expectedVersion (found '$($version.ProductName)' '$($version.FileVersion)')."
    }

    # Launch hidden and enumerate by class: MainWindowHandle ignores hidden
    # windows and could otherwise mistake an error MessageBox for the game.
    $env:LOCALAPPDATA = $profileDirectory
    try {
        if ($SystemOnlyPath) {
            # A release must not find missing redistributables in developer tools.
            $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot;$env:SystemRoot\System32\Wbem"
        }
        $process = Start-Process -FilePath $executablePath -WindowStyle Hidden `
            -WorkingDirectory $workingDirectory -PassThru `
            -RedirectStandardError $stderrPath -RedirectStandardOutput $stdoutPath
        # Retain the handle before exit; Windows PowerShell otherwise sometimes
        # returns a null ExitCode for a process obtained through Start-Process.
        $null = $process.Handle
    }
    finally {
        $env:LOCALAPPDATA = $originalLocalAppData
        $env:PATH = $originalPath
    }

    $timer = [Diagnostics.Stopwatch]::StartNew()
    $missingHardware = $false
    while ($timer.Elapsed.TotalSeconds -lt $StartupSeconds) {
        $process.Refresh()
        [string]$stderr = Read-StartupError
        if ($RequireAudio -and $stderr -match '(?m)^no audio: ') {
            throw 'The audible startup check unexpectedly used a silent bank.'
        }
        if ($stderr.Contains('startup failure:')) {
            if ($AllowNoHardwareAdapter -and
                $stderr.Trim() -ceq 'startup failure: No Direct3D hardware device found') {
                $missingHardware = $true
                break
            }
            throw "The game reported a startup failure: $($stderr.Trim())"
        }
        if ($process.HasExited) {
            throw "The game exited during startup (code $($process.ExitCode)). $($stderr.Trim())"
        }
        Start-Sleep -Milliseconds 100
    }

    if ($missingHardware) {
        Assert-DpiAware
        [ColourWars.StartupWindow]::CloseOwnedWindows($process.Id)
        if (-not $process.WaitForExit(3000)) {
            throw 'The missing-adapter error dialog did not close normally.'
        }
        $process.WaitForExit()
        if ($process.ExitCode -ne 1) {
            throw "The missing-adapter failure exited unexpectedly (code $($process.ExitCode))."
        }
        [string]$stderr = Read-StartupError
        if ($stderr.Trim() -cne 'startup failure: No Direct3D hardware device found') {
            throw "Additional diagnostics accompanied the missing-adapter failure: $($stderr.Trim())"
        }
        $result.outcome = 'skipped-no-hardware-adapter'
        $result.diagnostic = 'Release could not find a Direct3D hardware adapter; hardware startup remains unverified.'
        Write-Warning $result.diagnostic
    }
    else {
        $window = [ColourWars.StartupWindow]::FindGame($process.Id)
        if ($window -eq [IntPtr]::Zero) {
            throw 'The process did not create ColourWarsWindowClass.'
        }
        if (-not [ColourWars.StartupWindow]::Responds($window)) {
            throw 'The game window did not respond after startup.'
        }
        Assert-DpiAware
        if (-not [ColourWars.StartupWindow]::PerMonitorV2($window)) {
            throw 'The game window is not per-monitor v2 DPI aware; check the dpiAwareness element in game/settings.manifest.'
        }
        if (-not (Test-Path -LiteralPath (Join-Path $profileDirectory 'ColourWars/save_data.json'))) {
            throw 'The game did not create its save in the isolated profile.'
        }
        [string]$stderr = Read-StartupError
        if ($RequireNoAudio -and $stderr -notmatch '(?m)^no audio: .*The manifest marks this bank optional, so it plays nothing\.') {
            throw 'The expected optional-wave-bank fallback was not reported.'
        }
        foreach ($line in ($stderr -split '\r?\n')) {
            if ($RequireAudio -and $line -match '^no audio: ') {
                throw 'The audible startup check unexpectedly used a silent bank.'
            }
            if ($line.Trim() -ne '' -and
                $line -notmatch '^no audio: .*The manifest marks this bank optional, so it plays nothing\.' -and
                -not ($userStageRefusal -and $line -match $userStageRefusal)) {
                throw "Unexpected startup diagnostic: $line"
            }
        }
        if ($userStageRefusal -and @($stderr -split '\r?\n' | Where-Object { $_ -match $userStageRefusal }).Count -ne 1) {
            throw 'The broken user stage was not refused with exactly one line.'
        }
        [ColourWars.StartupWindow]::CloseOwnedWindows($process.Id)
        if (-not $process.WaitForExit(10000)) {
            throw 'The game did not shut down after WM_CLOSE.'
        }
        $process.WaitForExit()
        if ($process.ExitCode -ne 0) {
            throw "The game failed during shutdown (code $($process.ExitCode))."
        }
        # Destruction can report errors too, after the last running-process read.
        [string]$stderr = Read-StartupError
        foreach ($line in ($stderr -split '\r?\n')) {
            if ($RequireAudio -and $line -match '^no audio: ') {
                throw 'The audible startup check unexpectedly used a silent bank.'
            }
            if ($line.Trim() -ne '' -and
                $line -notmatch '^no audio: .*The manifest marks this bank optional, so it plays nothing\.' -and
                -not ($userStageRefusal -and $line -match $userStageRefusal)) {
                throw "Unexpected shutdown diagnostic: $line"
            }
        }
        $result.outcome = 'passed'
        $result.diagnostic = 'Game window, per-monitor v2 DPI awareness, content startup, isolated save and graceful shutdown passed.'
        Write-Output $result.diagnostic
    }
}
catch {
    $failure = $_
    $result.diagnostic = $_.Exception.Message
}
finally {
    if ($null -ne $process) {
        $process.Refresh()
        if (-not $process.HasExited) {
            [ColourWars.StartupWindow]::CloseOwnedWindows($process.Id)
            if (-not $process.WaitForExit(3000)) {
                # Only the process this invocation created; never another game.
                $process.Kill()
                $null = $process.WaitForExit(3000)
            }
        }
        $process.Dispose()
    }
    $result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $runDirectory 'result.json') -Encoding utf8
    Write-Output "Startup evidence: $runDirectory"
    [string]$stderr = Read-StartupError
    if ($stderr.Trim() -ne '') { Write-Output $stderr.Trim() }
}
if ($null -ne $failure) { throw $failure }
