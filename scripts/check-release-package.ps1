# CI#6: the Release ZIP's packaging rules, runnable locally as well as in CI.
# Extracts the ZIP into -OutputDirectory (replacing only the
# ColourWars-windows-x64 folder this script extracts there) and refuses it
# when development files or unreviewed audio leaked in, a required file is
# missing, the bank differs from the accepted publication, or the
# Debug-only developer console was linked. It starts nothing; startup checks
# of the extracted game are check-startup.ps1's.
#
#   pwsh -File scripts/check-release-package.ps1            # after `--target package`
#   pwsh -File scripts/check-release-package.ps1 -Zip <zip> -OutputDirectory <dir>
[CmdletBinding()]
param(
    [string]$Zip,
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'check-path.ps1')

$repository = Split-Path -Parent $PSScriptRoot
if (-not $Zip) { $Zip = Join-Path $repository 'out/build/x64-release/packages/ColourWars-windows-x64.zip' }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $repository 'out/validation/release-package' }
$zipPath = Resolve-CheckFileSystemPath $Zip
$destination = Resolve-CheckFileSystemPath $OutputDirectory
if (-not (Test-Path -LiteralPath $zipPath -PathType Leaf)) {
    throw "check-release-package: no Release ZIP at $zipPath; build the 'package' target first."
}

$package = Join-Path $destination 'ColourWars-windows-x64'
if (Test-Path -LiteralPath $package) { Remove-Item -LiteralPath $package -Recurse -Force }
$null = New-Item -ItemType Directory -Path $destination -Force
Expand-Archive -LiteralPath $zipPath -DestinationPath $destination
if (-not (Test-Path -LiteralPath $package -PathType Container)) {
    throw 'check-release-package: the ZIP does not hold a ColourWars-windows-x64 folder.'
}
$checks = 0

$files = @(Get-ChildItem -LiteralPath $package -File -Recurse)
$leaked = @($files | Where-Object {
    $_.Extension -in '.wav', '.mp3', '.ogg', '.pdb' -or
    ($_.Extension -eq '.xwb' -and $_.FullName -ne (Join-Path $package 'sounds\paint_exe.xwb')) -or
    ($_.Extension -eq '.exe' -and $_.Name -ne 'ColourWars.exe')
})
if ($leaked.Count -gt 0) {
    throw "Development files or unreviewed audio leaked into the package: $(($leaked | ForEach-Object Name) -join ', ')"
}
$checks++

foreach ($required in 'ColourWars.exe', 'DirectXTK.dll', 'xaudio2_9redist.dll',
    'msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll', 'build-info.txt',
    'HOW-TO-PLAY.txt',
    'sounds/paint_exe.json', 'sounds/paint_exe.xwb',
    'licenses/paint_exe/PROVENANCE.md', 'licenses/paint_exe/CC0-1.0.txt',
    'licenses/paint_exe/runtime-bank.json', 'licenses/fonts/Lato-OFL.txt',
    'licenses/fonts/BowlbyOne-OFL.txt', 'licenses/fonts/CourierPrime-OFL.txt') {
    if (-not (Test-Path -LiteralPath (Join-Path $package $required))) {
        throw "Package is missing $required"
    }
    $checks++
}

$receipt = Get-Content -Raw (Join-Path $repository 'scripts/audio/runtime-bank.json') | ConvertFrom-Json
if ((Get-FileHash -LiteralPath (Join-Path $package 'sounds/paint_exe.xwb')).Hash -ine $receipt.sha256) {
    throw 'Packaged bank differs from the accepted publication'
}
$checks++

# The developer console is Debug-only. Its polymorphic classes would leave
# MSVC RTTI names in the binary if any of it were linked. Keep this list
# aligned with scripts/acceptance/debug_console.cpp.
$exe = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes((Join-Path $package 'ColourWars.exe')))
foreach ($name in '.?AVDebugConsoleState@@', '.?AVDebugRegistry@@', '.?AVLambdaCommand@@',
    '.?AVDebugMenuPage@@', '.?AVNumberVariable@@',
    '.?AVBot@@', '.?AVPaintBot@@', '.?AVBotDriver@@', '.?AVReplayRecorder@@',
    '.?AVReplayPlayer@@', '.?AVTuneTarget@@', '.?AVTuningTarget@@',
    '.?AVDebugBackgroundInput@@', '.?AVDebugWait@@', '.?AVUpdatesWait@@', '.?AVMatchWait@@') {
    if ($exe.Contains($name)) { throw "Developer console code leaked into the package: $name" }
    $checks++
}

Write-Host "check-release-package: $checks checks passed on $($files.Count) files; extracted to $package"
