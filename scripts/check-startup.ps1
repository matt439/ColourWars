[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '../out/validation/startup'),
    [ValidateRange(1, 120)]
    [int]$StartupSeconds = 15,
    [switch]$RequireNoAudio,
    [switch]$SystemOnlyPath,
    # Release has no WARP fallback. Only its exact missing-adapter diagnostic
    # can be skipped; the default remains a required hardware startup check.
    [switch]$AllowNoHardwareAdapter
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$executablePath = (Resolve-Path -LiteralPath $Executable).Path
if (-not (Test-Path -LiteralPath $executablePath -PathType Leaf)) {
    throw "Executable is not a file: $executablePath"
}
$executableDirectory = Split-Path -Parent $executablePath
if ($RequireNoAudio -and (Test-Path -LiteralPath (
    Join-Path $executableDirectory 'sounds/sound_bank_1.xwb'))) {
    throw 'The startup check requires an absent optional wave bank.'
}

$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
$runDirectory = Join-Path $outputRoot ([Guid]::NewGuid().ToString('N'))
$workingDirectory = Join-Path $runDirectory 'working-directory'
$profileDirectory = Join-Path $runDirectory 'profile'
$null = New-Item -ItemType Directory -Path $workingDirectory, $profileDirectory -Force
$stderrPath = Join-Path $runDirectory 'startup.err'
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
    outcome = 'failed'
    diagnostic = ''
}
$failure = $null
try {
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
        if (-not (Test-Path -LiteralPath (Join-Path $profileDirectory 'ColourWars/save_data.json'))) {
            throw 'The game did not create its save in the isolated profile.'
        }
        [string]$stderr = Read-StartupError
        if ($RequireNoAudio -and $stderr -notmatch '(?m)^no audio: .*The manifest marks this bank optional, so it plays nothing\.') {
            throw 'The expected optional-wave-bank fallback was not reported.'
        }
        foreach ($line in ($stderr -split '\r?\n')) {
            if ($line.Trim() -ne '' -and
                $line -notmatch '^no audio: .*The manifest marks this bank optional, so it plays nothing\.') {
                throw "Unexpected startup diagnostic: $line"
            }
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
            if ($line.Trim() -ne '' -and
                $line -notmatch '^no audio: .*The manifest marks this bank optional, so it plays nothing\.') {
                throw "Unexpected shutdown diagnostic: $line"
            }
        }
        $result.outcome = 'passed'
        $result.diagnostic = 'Game window, content startup, isolated save and graceful shutdown passed.'
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
