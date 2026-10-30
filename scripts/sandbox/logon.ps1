# ROAD#37: the logon command of a Windows Sandbox run of the Release ZIP.
#
# scripts/sandbox-release.ps1 copies this file into a run's results folder and
# names it in the .wsb it writes. Windows Sandbox maps the package folder (the
# ZIP and its extraction) read-only and the results folder writable, runs this
# at logon, and is shut down by it once receipt.json is written. On the host,
# the driver's -HostTrial runs the same file with -NoShutdown against the same
# folders, so its logic is exercised without starting a sandbox.
#
# It is self-contained: nothing from the checkout exists inside the sandbox.
# It starts ColourWars.exe with LOCALAPPDATA in the results folder, waits for
# the window, closes it with WM_CLOSE as check-startup.ps1 does and records the
# ZIP's SHA-256, the exit code, what the game wrote to stderr (and its
# last-error.log, if any), the Windows version, whether the Visual C++ runtime
# is in System32 and the display adapters. Windows PowerShell 5.1 is what a
# sandbox has, so this must stay 5.1-compatible.
[CmdletBinding()]
param(
    [string]$Package = 'C:\Users\WDAGUtilityAccount\Desktop\package',
    [string]$Results,
    [ValidateRange(1, 300)]
    [int]$StartupSeconds = 20,
    [switch]$NoShutdown
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
# Windows PowerShell 5.1 leaves PSScriptRoot empty in parameter defaults.
if (-not $Results) { $Results = $PSScriptRoot }

$receipt = [ordered]@{
    schema = 'colourwars-sandbox-receipt/1'
    started_utc = [DateTime]::UtcNow.ToString('o')
    written_utc = ''
    host_trial = [bool]$NoShutdown
    computer = $env:COMPUTERNAME
    user = $env:USERNAME
    zip = $null
    build_info = ''
    windows = $null
    runtime_in_system32 = [ordered]@{}
    display_adapters = @()
    game = [ordered]@{
        window_found = $false
        save_created = $false
        screenshot = ''
        exit_code = $null
        stderr = ''
        stdout = ''
        last_error_log = ''
    }
    outcome = 'failed'
    diagnostic = ''
}

function Write-Receipt {
    $receipt.written_utc = [DateTime]::UtcNow.ToString('o')
    $final = Join-Path $Results 'receipt.json'
    $temporary = "$final.tmp"
    [IO.File]::WriteAllText($temporary, ($receipt | ConvertTo-Json -Depth 6), [Text.UTF8Encoding]::new($false))
    # The host polls for receipt.json, so it must appear complete or not at all.
    if (Test-Path -LiteralPath $final) { Remove-Item -LiteralPath $final -Force }
    Move-Item -LiteralPath $temporary -Destination $final
}

function Read-Text([string]$Path) {
    # Get-Content -Raw gives nothing for an empty file, and a function that
    # returns [string] of nothing still returns $null.
    $text = $null
    if (Test-Path -LiteralPath $Path -PathType Leaf) { $text = Get-Content -LiteralPath $Path -Raw }
    if ($null -eq $text) { return '' }
    return [string]$text
}

# The machine half is recorded first and separately, so a game that never
# starts still leaves a receipt saying what it was refused on.
try {
    $current = Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion'
    # ProductName still says "Windows 10" on Windows 11; the CIM caption does not.
    $receipt.windows = [ordered]@{
        caption = [string](Get-CimInstance Win32_OperatingSystem).Caption
        product_name = [string]$current.ProductName
        edition_id = [string]$current.EditionID
        display_version = $(if ($current.PSObject.Properties['DisplayVersion']) { [string]$current.DisplayVersion } else { '' })
        build = "$([string]$current.CurrentBuild).$([string]$current.UBR)"
        version = [Environment]::OSVersion.VersionString
    }
}
catch { $receipt.windows = [ordered]@{ error = $_.Exception.Message } }
foreach ($name in @('vcruntime140.dll', 'vcruntime140_1.dll', 'msvcp140.dll')) {
    $receipt.runtime_in_system32[$name] = Test-Path -LiteralPath (Join-Path $env:SystemRoot "System32\$name") -PathType Leaf
}
try {
    $receipt.display_adapters = @(Get-CimInstance Win32_VideoController | ForEach-Object {
        [ordered]@{
            name = [string]$_.Name
            driver_version = [string]$_.DriverVersion
            adapter_compatibility = [string]$_.AdapterCompatibility
            pnp_device_id = [string]$_.PNPDeviceID
        }
    })
}
catch { $receipt.display_adapters = @([ordered]@{ error = $_.Exception.Message }) }

$process = $null
try {
    $zips = @(Get-ChildItem -LiteralPath $Package -Filter '*.zip' -File)
    if ($zips.Count -ne 1) { throw "The package folder must hold exactly one ZIP; it holds $($zips.Count)." }
    $zip = $zips[0]
    $receipt.zip = [ordered]@{
        name = $zip.Name
        bytes = $zip.Length
        sha256 = (Get-FileHash -LiteralPath $zip.FullName -Algorithm SHA256).Hash
    }
    $release = Join-Path $Package ([IO.Path]::GetFileNameWithoutExtension($zip.Name))
    $executable = Join-Path $release 'ColourWars.exe'
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) { throw "The extracted package has no ColourWars.exe at $executable." }
    $receipt.build_info = Read-Text (Join-Path $release 'build-info.txt')

    if (-not ('ColourWarsSandbox.GameWindow' -as [type])) {
        Add-Type -ReferencedAssemblies System.Drawing, System.Windows.Forms -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

namespace ColourWarsSandbox
{
    public static class GameWindow
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

        public static bool Find(int processId)
        {
            bool found = false;
            EnumWindows((window, parameter) => {
                uint owner;
                GetWindowThreadProcessId(window, out owner);
                if (owner != processId) return true;
                var name = new StringBuilder(256);
                GetClassName(window, name, name.Capacity);
                if (name.ToString() != "ColourWarsWindowClass") return true;
                found = true;
                return false;
            }, IntPtr.Zero);
            return found;
        }

        // WM_CLOSE to every window the game owns, as check-startup.ps1 does;
        // that also dismisses the missing-adapter dialog.
        public static void CloseOwnedWindows(int processId)
        {
            EnumWindows((window, parameter) => {
                uint owner;
                GetWindowThreadProcessId(window, out owner);
                if (owner == processId)
                    PostMessage(window, 0x0010, IntPtr.Zero, IntPtr.Zero);
                return true;
            }, IntPtr.Zero);
        }

        public static void CaptureScreen(string path)
        {
            Rectangle bounds = SystemInformation.VirtualScreen;
            using (var bitmap = new Bitmap(bounds.Width, bounds.Height))
            {
                using (var graphics = Graphics.FromImage(bitmap))
                    graphics.CopyFromScreen(bounds.Left, bounds.Top, 0, 0, bounds.Size);
                bitmap.Save(path, ImageFormat.Png);
            }
        }
    }
}
'@
    }

    $profileDirectory = Join-Path $Results 'profile'
    $workingDirectory = Join-Path $Results 'working-directory'
    $stderrPath = Join-Path $Results 'game.err'
    $stdoutPath = Join-Path $Results 'game.out'
    $null = New-Item -ItemType Directory -Path $profileDirectory, $workingDirectory -Force
    $refusal = 'startup failure: No Direct3D hardware device found'

    $originalLocalAppData = $env:LOCALAPPDATA
    $env:LOCALAPPDATA = $profileDirectory
    try {
        $process = Start-Process -FilePath $executable -WorkingDirectory $workingDirectory -PassThru `
            -RedirectStandardError $stderrPath -RedirectStandardOutput $stdoutPath
        $null = $process.Handle
    }
    finally { $env:LOCALAPPDATA = $originalLocalAppData }

    $timer = [Diagnostics.Stopwatch]::StartNew()
    $refused = $false
    while ($timer.Elapsed.TotalSeconds -lt $StartupSeconds) {
        $process.Refresh()
        $stderr = Read-Text $stderrPath
        if ($stderr.Trim() -ceq $refusal) { $refused = $true; break }
        if ($stderr.Contains('startup failure:')) { throw "The game reported a startup failure: $($stderr.Trim())" }
        if ($process.HasExited) { throw "The game exited during startup (code $($process.ExitCode))." }
        Start-Sleep -Milliseconds 250
    }

    if (-not $refused) {
        $receipt.game.window_found = [ColourWarsSandbox.GameWindow]::Find($process.Id)
        if (-not $receipt.game.window_found) { throw 'The process did not create ColourWarsWindowClass.' }
        $receipt.game.save_created = Test-Path -LiteralPath (Join-Path $profileDirectory 'ColourWars\save_data.json')
        # Only inside a sandbox, whose desktop holds nothing but the game: on
        # the host the whole screen would capture the person's other windows.
        if ($NoShutdown) { $receipt.game.screenshot = 'not captured: host trial' }
        else {
            try {
                $shot = Join-Path $Results 'screen.png'
                [ColourWarsSandbox.GameWindow]::CaptureScreen($shot)
                $receipt.game.screenshot = 'screen.png'
            }
            catch { $receipt.game.screenshot = "not captured: $($_.Exception.Message)" }
        }
    }

    [ColourWarsSandbox.GameWindow]::CloseOwnedWindows($process.Id)
    if (-not $process.WaitForExit(15000)) { throw 'The game did not exit within 15 seconds of WM_CLOSE.' }
    $process.WaitForExit()
    $receipt.game.exit_code = $process.ExitCode
    $stderr = Read-Text $stderrPath

    if ($refused) {
        if ($process.ExitCode -ne 1 -or $stderr.Trim() -cne $refusal) {
            throw "The missing-adapter refusal exited $($process.ExitCode) with: $($stderr.Trim())"
        }
        $receipt.outcome = 'refused-no-hardware-adapter'
        $receipt.diagnostic = 'Release found no Direct3D hardware adapter and refused loudly, as it has no WARP fallback.'
    }
    else {
        if ($process.ExitCode -ne 0) { throw "The game exited $($process.ExitCode) after WM_CLOSE." }
        if (-not $receipt.game.save_created) { throw 'The game did not create its save under the results profile.' }
        $receipt.outcome = 'passed'
        $receipt.diagnostic = 'The game window opened, the save was created and WM_CLOSE exited 0.'
    }
}
catch {
    $receipt.outcome = 'failed'
    $receipt.diagnostic = $_.Exception.Message
}
finally {
    if ($null -ne $process) {
        $process.Refresh()
        if (-not $process.HasExited) {
            # Only the process this script started.
            $process.Kill()
            $null = $process.WaitForExit(5000)
        }
        if ($null -eq $receipt.game.exit_code -and $process.HasExited) { $receipt.game.exit_code = $process.ExitCode }
    }
    $receipt.game.stderr = Read-Text (Join-Path $Results 'game.err')
    $receipt.game.stdout = Read-Text (Join-Path $Results 'game.out')
    $receipt.game.last_error_log = Read-Text (Join-Path $Results 'profile\ColourWars\last-error.log')
    Write-Receipt
}

if (-not $NoShutdown) { & shutdown.exe /s /t 0 }
