# ROAD#37: check scripts/sandbox-release.ps1's output on the host without
# starting a sandbox. A synthetic ZIP stands in for the release, so no build,
# compiler or GPU is needed: the check reads the .wsb as XML (networking off,
# the package mapped read-only, results writable, the logon command), the
# copied ZIP's hash and extraction, host.json, and parses the copied logon
# script. -Zip with a real Release ZIP adds -HostTrial: the logon script runs
# the game on this machine and its receipt must carry the ZIP's hash.
[CmdletBinding()]
param(
    [string]$OutputDirectory,
    [string]$Zip
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'check-path.ps1')

if (-not $OutputDirectory) { $OutputDirectory = Join-Path $PSScriptRoot '../out/validation/sandbox-check' }
$root = Join-Path (Resolve-CheckFileSystemPath $OutputDirectory) ([Guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Path $root -Force
$driver = Join-Path $PSScriptRoot 'sandbox-release.ps1'
$desktop = 'C:\Users\WDAGUtilityAccount\Desktop'
$checks = 0

function Assert-True([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw "check-sandbox-release: $Message" }
    $script:checks++
}

function New-FakeZip([string]$Name, [bool]$WithExecutable) {
    $stage = Join-Path $root "stage-$Name"
    $folder = Join-Path $stage 'ColourWars-windows-x64'
    $null = New-Item -ItemType Directory -Path $folder -Force
    if ($WithExecutable) { [IO.File]::WriteAllText((Join-Path $folder 'ColourWars.exe'), 'not a real executable') }
    [IO.File]::WriteAllText((Join-Path $folder 'build-info.txt'), "check-sandbox-release $Name")
    $zipFolder = Join-Path $root "zip-$Name"
    $null = New-Item -ItemType Directory -Path $zipFolder -Force
    $path = Join-Path $zipFolder 'ColourWars-windows-x64.zip'
    Compress-Archive -LiteralPath $folder -DestinationPath $path
    return $path
}

function Invoke-Driver([hashtable]$Arguments) {
    $output = & $driver @Arguments
    $line = @($output | Where-Object { "$_" -like 'Sandbox run folder: *' })
    Assert-True ($line.Count -eq 1) 'the driver did not name exactly one run folder'
    return ([string]$line[0]).Substring('Sandbox run folder: '.Length)
}

function Assert-Prepared([string]$Run, [string]$ZipPath, [string]$VGpu, [int]$Seconds) {
    $hash = (Get-FileHash -LiteralPath $ZipPath -Algorithm SHA256).Hash
    $package = Join-Path $Run 'package'
    $results = Join-Path $Run 'results'
    Assert-True ((Get-FileHash -LiteralPath (Join-Path $package 'ColourWars-windows-x64.zip') -Algorithm SHA256).Hash -eq $hash) 'the copied ZIP differs'
    Assert-True (Test-Path -LiteralPath (Join-Path $package 'ColourWars-windows-x64/ColourWars.exe') -PathType Leaf) 'the ZIP was not extracted beside itself'
    Assert-True (@(Get-ChildItem -LiteralPath $package -Filter '*.zip' -File).Count -eq 1) 'the package folder must hold one ZIP'
    $copied = Get-Content -LiteralPath (Join-Path $results 'logon.ps1') -Raw
    Assert-True ($copied -ceq (Get-Content -LiteralPath (Join-Path $PSScriptRoot 'sandbox/logon.ps1') -Raw)) 'the logon script copy differs'
    $tokens = $null; $errors = $null
    $null = [Management.Automation.Language.Parser]::ParseInput($copied, [ref]$tokens, [ref]$errors)
    Assert-True ($errors.Count -eq 0) "the logon script does not parse: $($errors | Out-String)"

    [xml]$wsb = Get-Content -LiteralPath (Join-Path $Run 'clean-machine.wsb') -Raw
    $configuration = $wsb.Configuration
    $names = @($configuration.ChildNodes | ForEach-Object { $_.Name }) -join ','
    Assert-True ($names -ceq 'VGpu,Networking,MappedFolders,LogonCommand') "unexpected .wsb elements: $names"
    Assert-True ($configuration.VGpu -ceq $VGpu) "VGpu is '$($configuration.VGpu)', not $VGpu"
    Assert-True ($configuration.Networking -ceq 'Disable') 'networking is not disabled'
    $folders = @($configuration.MappedFolders.MappedFolder)
    Assert-True ($folders.Count -eq 2) 'the .wsb must map exactly two folders'
    Assert-True ($folders[0].HostFolder -ceq $package -and $folders[0].SandboxFolder -ceq "$desktop\package" -and $folders[0].ReadOnly -ceq 'true') 'the package is not mapped read-only'
    Assert-True ($folders[1].HostFolder -ceq $results -and $folders[1].SandboxFolder -ceq "$desktop\results" -and $folders[1].ReadOnly -ceq 'false') 'the results folder is not mapped writable'
    Assert-True ($configuration.LogonCommand.Command -ceq "powershell.exe -NoProfile -ExecutionPolicy Bypass -File $desktop\results\logon.ps1 -StartupSeconds $Seconds") "unexpected logon command: $($configuration.LogonCommand.Command)"

    $record = Get-Content -LiteralPath (Join-Path $Run 'host.json') -Raw | ConvertFrom-Json
    Assert-True ($record.zip_sha256 -ceq $hash -and $record.vgpu -ceq $VGpu -and $record.outcome -ceq 'prepared') 'host.json does not record the ZIP hash, VGpu and prepared outcome'
    Assert-True (-not (Test-Path -LiteralPath (Join-Path $results 'receipt.json'))) 'a prepared run already has a receipt'
}

function Assert-Refused([hashtable]$Arguments, [string]$Pattern, [string]$Label) {
    $refused = $false
    try { $null = & $driver @Arguments } catch { $refused = $_.Exception.Message -match $Pattern }
    Assert-True $refused "the driver did not refuse $Label"
}

$good = New-FakeZip 'good' $true
Assert-Prepared (Invoke-Driver @{ Zip = $good; OutputDirectory = (Join-Path $root 'runs') }) $good 'Default' 20
Assert-Prepared (Invoke-Driver @{ Zip = $good; OutputDirectory = (Join-Path $root 'runs'); VGpu = 'Disable'; StartupSeconds = 45 }) $good 'Disable' 45

# A path that needs XML escaping stays the same path once parsed.
$awkward = Join-Path $root "runs & 'odd'"
Assert-Prepared (Invoke-Driver @{ Zip = $good; OutputDirectory = $awkward }) $good 'Default' 20

# Relative paths follow the caller's PowerShell location, as the other checkers' do.
Push-Location -LiteralPath $root
try { Assert-Prepared (Invoke-Driver @{ Zip = (Resolve-Path -Relative $good); OutputDirectory = 'relative-runs' }) $good 'Default' 20 }
finally { Pop-Location }
Assert-True (Test-Path -LiteralPath (Join-Path $root 'relative-runs') -PathType Container) 'a relative OutputDirectory did not resolve against the caller location'

Assert-Refused @{ Zip = (Join-Path $root 'missing.zip'); OutputDirectory = (Join-Path $root 'runs') } 'No Release ZIP' 'a missing ZIP'
Assert-Refused @{ Zip = (New-FakeZip 'empty' $false); OutputDirectory = (Join-Path $root 'runs') } 'has no ColourWars-windows-x64/ColourWars\.exe' 'a ZIP without the game'
Assert-Refused @{ Zip = $good; OutputDirectory = (Join-Path $root 'runs'); Run = $true; HostTrial = $true } 'Choose either' '-Run with -HostTrial'

$trial = ''
if ($Zip) {
    $real = Resolve-CheckFileSystemPath $Zip
    $output = & $driver -Zip $real -OutputDirectory (Join-Path $root 'host-trial') -HostTrial
    $output | ForEach-Object { Write-Output "  $_" }
    $run = ([string]@($output | Where-Object { "$_" -like 'Sandbox run folder: *' })[0]).Substring('Sandbox run folder: '.Length)
    $receipt = Get-Content -LiteralPath (Join-Path $run 'results/receipt.json') -Raw | ConvertFrom-Json
    Assert-True ($receipt.zip.sha256 -ceq (Get-FileHash -LiteralPath $real -Algorithm SHA256).Hash) 'the host trial receipt hash differs'
    Assert-True ($receipt.outcome -ceq 'passed' -and $receipt.game.exit_code -eq 0 -and $receipt.game.window_found -and $receipt.game.save_created) "the host trial did not pass: $($receipt.diagnostic)"
    Assert-True ($receipt.game.screenshot -ceq 'not captured: host trial' -and -not (Test-Path -LiteralPath (Join-Path $run 'results/screen.png'))) 'the host trial captured the host screen'
    Assert-True ($receipt.host_trial -and @($receipt.display_adapters).Count -ge 1 -and $null -ne $receipt.windows.build) 'the host trial receipt lacks its machine record'
    $trial = " and a host trial of $real"
}

Write-Output "check-sandbox-release passed: $checks checks$trial ($($PSVersionTable.PSEdition) $($PSVersionTable.PSVersion)). Evidence: $root"
