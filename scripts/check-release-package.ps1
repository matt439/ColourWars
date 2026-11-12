# CI#6: the Release ZIP's packaging rules, runnable locally as well as in CI.
# Extracts the ZIP into -OutputDirectory (replacing only the
# ColourWars-windows-x64 folder this script extracts there) and refuses it
# when development files or unreviewed audio leaked in, a required file is
# missing, the bank differs from the accepted publication, the Debug-only
# developer console was linked, or the executable does not name its symbols
# as plain ColourWars.pdb (REL#7: the PDB is a CI artifact, never in the ZIP).
# It starts nothing; startup checks of the extracted game are check-startup.ps1's.
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
. (Join-Path $PSScriptRoot 'release-package-path.ps1')

$repository = Split-Path -Parent $PSScriptRoot
if (-not $Zip) { $Zip = Get-ColourWarsReleasePackagePath $repository }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $repository 'out/validation/release-package' }
$zipPath = Resolve-CheckFileSystemPath $Zip
$destination = Resolve-CheckFileSystemPath $OutputDirectory
if (-not (Test-Path -LiteralPath $zipPath -PathType Leaf)) {
    throw "check-release-package: no Release ZIP at $zipPath; build the 'package' target first."
}

$package = Join-Path $destination 'ColourWars-windows-x64'
# Only this fixed child belongs to the extractor; prove that before cleanup.
$package = [IO.Path]::GetFullPath($package)
if ([IO.Path]::GetDirectoryName($package) -ine $destination.TrimEnd('\', '/')) {
    throw "check-release-package: extraction path escaped its output directory: $package"
}
if ((Test-Path -LiteralPath $package) -and
    ((Get-Item -LiteralPath $package -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
    throw "check-release-package: refusing a linked extraction directory: $package"
}
if (Test-Path -LiteralPath $package) { Remove-Item -LiteralPath $package -Recurse -Force }
$null = New-Item -ItemType Directory -Path $package -Force
Expand-Archive -LiteralPath $zipPath -DestinationPath $package
if (-not (Test-Path -LiteralPath (Join-Path $package 'ColourWars.exe') -PathType Leaf)) {
    throw 'check-release-package: the ZIP does not hold ColourWars.exe at its root.'
}
$checks = 0

$files = @(Get-ChildItem -LiteralPath $package -File -Recurse)
$leaked = @($files | Where-Object {
    $_.Extension -in '.wav', '.mp3', '.ogg', '.pdb' -or $_.FullName -eq (Join-Path $package 'README.md') -or
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

# REL#7: the executable's CodeView record names ColourWars.pdb and no build
# path, so a debugger matches the CI artifact by name and signature alone.
function Get-PdbNames([byte[]]$Image) {
    $pe = [BitConverter]::ToInt32($Image, 0x3C)
    if ([BitConverter]::ToUInt32($Image, $pe) -ne 0x4550) { throw 'ColourWars.exe is not a PE image.' }
    $sections = [BitConverter]::ToUInt16($Image, $pe + 6)
    $optional = $pe + 24
    $optionalSize = [BitConverter]::ToUInt16($Image, $pe + 20)
    if ([BitConverter]::ToUInt16($Image, $optional) -ne 0x20B) { throw 'ColourWars.exe is not a PE32+ image.' }
    if ([BitConverter]::ToUInt32($Image, $optional + 108) -le 6) { return @() }
    $debugRva = [BitConverter]::ToUInt32($Image, $optional + 112 + 6 * 8)
    $debugSize = [BitConverter]::ToUInt32($Image, $optional + 112 + 6 * 8 + 4)
    if ($debugRva -eq 0) { return @() }
    $debugOffset = $null
    for ($index = 0; $index -lt $sections; $index++) {
        $section = $optional + $optionalSize + 40 * $index
        $address = [BitConverter]::ToUInt32($Image, $section + 12)
        $size = [Math]::Max([BitConverter]::ToUInt32($Image, $section + 8), [BitConverter]::ToUInt32($Image, $section + 16))
        if ($debugRva -ge $address -and $debugRva -lt $address + $size) {
            $debugOffset = [BitConverter]::ToUInt32($Image, $section + 20) + ($debugRva - $address)
        }
    }
    if ($null -eq $debugOffset) { throw 'ColourWars.exe has a debug directory outside its sections.' }
    $names = @()
    for ($entry = $debugOffset; $entry -lt $debugOffset + $debugSize; $entry += 28) {
        if ([BitConverter]::ToUInt32($Image, $entry + 12) -ne 2) { continue }
        $record = [BitConverter]::ToUInt32($Image, $entry + 24)
        if ([Text.Encoding]::ASCII.GetString($Image, $record, 4) -ne 'RSDS') { throw 'ColourWars.exe has a CodeView record that is not RSDS.' }
        $end = [Array]::IndexOf($Image, [byte]0, [int]($record + 24))
        $names += [Text.Encoding]::UTF8.GetString($Image, $record + 24, $end - $record - 24)
    }
    return $names
}
$pdbNames = @(Get-PdbNames ([IO.File]::ReadAllBytes((Join-Path $package 'ColourWars.exe'))))
if ($pdbNames.Count -ne 1 -or $pdbNames[0] -cne 'ColourWars.pdb') {
    throw "ColourWars.exe must name its symbols as ColourWars.pdb with no path; it names: $($pdbNames -join ', ')"
}
$checks++

Write-Host "check-release-package: $checks checks passed on $($files.Count) files; extracted to $package"
