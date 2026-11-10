# Keep script defaults tied to the same project version as CPack.
function Get-ColourWarsReleasePackagePath([string]$Repository) {
    $project = Get-Content -LiteralPath (Join-Path $Repository 'CMakeLists.txt') -Raw
    $version = [regex]::Match($project, '(?m)^project\(ColourWars VERSION ([0-9]+\.[0-9]+\.[0-9]+)\s')
    if (-not $version.Success) { throw 'Cannot read ColourWars project(VERSION) from CMakeLists.txt.' }
    return Join-Path $Repository ("out/build/x64-release/packages/ColourWars-{0}-windows-x64.zip" -f $version.Groups[1].Value)
}
