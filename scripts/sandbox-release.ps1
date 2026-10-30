# ROAD#37: prepare, and optionally run, a Windows Sandbox run of the Release ZIP.
#
# Writes one run folder under -OutputDirectory:
#   package/   the ZIP and its extraction, mapped into the sandbox read-only
#   results/   logon.ps1 (scripts/sandbox/logon.ps1), mapped writable; the
#              sandbox leaves receipt.json, game.err/.out, screen.png and the
#              isolated profile here
#   clean-machine.wsb  networking off, the two mappings, the logon command
#   host.json  the host's ZIP SHA-256 and, after -Run or -HostTrial, the verdict
#
# Without -Run it only writes; open clean-machine.wsb to run it by hand. -Run
# opens it with WindowsSandbox.exe and waits for the receipt. -HostTrial runs
# the same logon script on this machine with -NoShutdown instead, to exercise
# it without a sandbox. Either way the receipt's ZIP hash must equal the host's.
#
# A sandbox is a clean Windows without Visual Studio or the Visual C++ runtime,
# but its GPU is the host's behind a paravirtual driver: it proves the loader
# and runtime requirements, not a player's GPU. See docs/feature-roadmap.md,
# "Windows Sandbox run of the Release ZIP".
[CmdletBinding()]
param(
    [string]$Zip,
    [string]$OutputDirectory,
    [ValidateSet('Default', 'Enable', 'Disable')]
    [string]$VGpu = 'Default',
    [ValidateRange(1, 300)]
    [int]$StartupSeconds = 20,
    [switch]$Run,
    [switch]$HostTrial,
    [ValidateRange(30, 3600)]
    [int]$TimeoutSeconds = 600
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'check-path.ps1')

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if ($Run -and $HostTrial) { throw 'Choose either -Run (Windows Sandbox) or -HostTrial (this machine).' }
if (-not $Zip) { $Zip = Join-Path $repo 'out/build/x64-release/packages/ColourWars-windows-x64.zip' }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $repo 'out/validation/sandbox' }

$zipPath = Resolve-CheckFileSystemPath $Zip
if (-not (Test-Path -LiteralPath $zipPath -PathType Leaf)) {
    throw "No Release ZIP at $zipPath; build it with: cmake --build --preset x64-release --target package"
}
if ([IO.Path]::GetExtension($zipPath) -ne '.zip') { throw "Not a .zip: $zipPath" }
$zipName = [IO.Path]::GetFileName($zipPath)
$releaseName = [IO.Path]::GetFileNameWithoutExtension($zipPath)

$runDirectory = Join-Path (Resolve-CheckFileSystemPath $OutputDirectory) ([Guid]::NewGuid().ToString('N'))
$package = Join-Path $runDirectory 'package'
$results = Join-Path $runDirectory 'results'
$null = New-Item -ItemType Directory -Path $package, $results -Force

$hostRecord = [ordered]@{
    zip = $zipPath
    zip_sha256 = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash
    vgpu = $VGpu
    mode = $(if ($Run) { 'sandbox' } elseif ($HostTrial) { 'host-trial' } else { 'prepared' })
    created_utc = [DateTime]::UtcNow.ToString('o')
    checkout = ''
    outcome = 'prepared'
    diagnostic = ''
}
try { $hostRecord.checkout = (& git -C $repo rev-parse HEAD 2>$null | Out-String).Trim() } catch { }
function Save-HostRecord {
    $hostRecord | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $runDirectory 'host.json') -Encoding utf8
}

Copy-Item -LiteralPath $zipPath -Destination (Join-Path $package $zipName)
if ((Get-FileHash -LiteralPath (Join-Path $package $zipName) -Algorithm SHA256).Hash -ne $hostRecord.zip_sha256) {
    throw 'The ZIP changed while it was copied.'
}
Expand-Archive -LiteralPath $zipPath -DestinationPath $package
if (-not (Test-Path -LiteralPath (Join-Path $package "$releaseName/ColourWars.exe") -PathType Leaf)) {
    throw "$zipName has no $releaseName/ColourWars.exe."
}
# The game opens its content through MAX_PATH-limited paths; a host trial in a
# deep folder would fail on that rather than on anything the sandbox tests.
if ($HostTrial) {
    $longest = @(Get-ChildItem -LiteralPath $package -File -Recurse | ForEach-Object { $_.FullName.Length } | Measure-Object -Maximum)[0].Maximum
    if ($longest -ge 240) { throw "The extracted package has $longest-character paths; use a shorter -OutputDirectory for -HostTrial." }
}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'sandbox/logon.ps1') -Destination (Join-Path $results 'logon.ps1')

$desktop = 'C:\Users\WDAGUtilityAccount\Desktop'
$escape = { param([string]$Text) [Security.SecurityElement]::Escape($Text) }
$wsb = @"
<Configuration>
  <VGpu>$VGpu</VGpu>
  <Networking>Disable</Networking>
  <MappedFolders>
    <MappedFolder>
      <HostFolder>$(& $escape $package)</HostFolder>
      <SandboxFolder>$desktop\package</SandboxFolder>
      <ReadOnly>true</ReadOnly>
    </MappedFolder>
    <MappedFolder>
      <HostFolder>$(& $escape $results)</HostFolder>
      <SandboxFolder>$desktop\results</SandboxFolder>
      <ReadOnly>false</ReadOnly>
    </MappedFolder>
  </MappedFolders>
  <LogonCommand>
    <Command>powershell.exe -NoProfile -ExecutionPolicy Bypass -File $desktop\results\logon.ps1 -StartupSeconds $StartupSeconds</Command>
  </LogonCommand>
</Configuration>
"@
$wsbPath = Join-Path $runDirectory 'clean-machine.wsb'
[IO.File]::WriteAllText($wsbPath, $wsb, [Text.UTF8Encoding]::new($false))
Save-HostRecord
Write-Output "Sandbox run folder: $runDirectory"

if (-not ($Run -or $HostTrial)) {
    Write-Output "Prepared. Open $wsbPath to run it; the receipt lands in $results."
    return
}

$receiptPath = Join-Path $results 'receipt.json'
$failure = $null
try {
    if ($Run) {
        $sandbox = Join-Path $env:SystemRoot 'System32/WindowsSandbox.exe'
        if (-not (Test-Path -LiteralPath $sandbox -PathType Leaf)) {
            throw 'WindowsSandbox.exe is absent; enable the Windows Sandbox optional feature.'
        }
        # Windows runs one sandbox at a time; a second would not start this one.
        # WindowsSandboxServer can outlive a closed sandbox, so it is not counted.
        if (@(Get-Process -Name 'WindowsSandbox', 'WindowsSandboxClient', 'WindowsSandboxRemoteSession' -ErrorAction SilentlyContinue).Count -ne 0) {
            throw 'A Windows Sandbox is already running; close it first.'
        }
        $null = Start-Process -FilePath $sandbox -ArgumentList "`"$wsbPath`"" -PassThru
        $timer = [Diagnostics.Stopwatch]::StartNew()
        while (-not (Test-Path -LiteralPath $receiptPath -PathType Leaf)) {
            if ($timer.Elapsed.TotalSeconds -ge $TimeoutSeconds) {
                throw "No receipt after $TimeoutSeconds seconds; the sandbox window may be waiting, or its logon command failed."
            }
            Start-Sleep -Seconds 2
        }
        # The guest shuts itself down after the receipt, but the client window
        # can stay open on a disconnected session; close it once it has had time.
        $timer.Restart()
        while ($timer.Elapsed.TotalSeconds -lt 60 -and
            @(Get-Process -Name 'WindowsSandboxRemoteSession' -ErrorAction SilentlyContinue).Count -ne 0) {
            Start-Sleep -Seconds 2
            if ($timer.Elapsed.TotalSeconds -ge 30) {
                Get-Process -Name 'WindowsSandboxRemoteSession' -ErrorAction SilentlyContinue |
                    ForEach-Object { $null = $_.CloseMainWindow() }
            }
        }
    }
    else {
        # The sandbox runs Windows PowerShell 5.1, so the trial does too, and,
        # like the sandbox, leaves -Results to default to the script's folder.
        $shell = Join-Path $env:SystemRoot 'System32/WindowsPowerShell/v1.0/powershell.exe'
        & $shell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $results 'logon.ps1') `
            -Package $package -StartupSeconds $StartupSeconds -NoShutdown
        if ($LASTEXITCODE -ne 0) { throw "The logon script exited $LASTEXITCODE on the host." }
    }

    $receipt = Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json
    if ($receipt.schema -cne 'colourwars-sandbox-receipt/1') { throw "Unknown receipt schema '$($receipt.schema)'." }
    # A guest that failed before hashing the ZIP leaves zip null; report why
    # rather than calling it a hash mismatch.
    if ($null -eq $receipt.zip) { throw "The receipt records no ZIP hash ($($receipt.outcome)): $($receipt.diagnostic)" }
    if ($receipt.zip.sha256 -cne $hostRecord.zip_sha256) {
        throw "The receipt's ZIP hash $($receipt.zip.sha256) does not match the host's $($hostRecord.zip_sha256)."
    }
    $hostRecord.outcome = $receipt.outcome
    $hostRecord.diagnostic = $receipt.diagnostic
    Write-Output "Receipt: $receiptPath"
    Write-Output "ZIP SHA-256 $($receipt.zip.sha256) matches the host."
    Write-Output "Outcome: $($receipt.outcome) - $($receipt.diagnostic) (exit code $($receipt.game.exit_code))"
    if ($receipt.outcome -notin @('passed', 'refused-no-hardware-adapter')) { throw "The run failed: $($receipt.diagnostic)" }
}
catch {
    $failure = $_
    $hostRecord.outcome = 'failed'
    $hostRecord.diagnostic = $_.Exception.Message
}
finally { Save-HostRecord }
if ($null -ne $failure) { throw $failure }
