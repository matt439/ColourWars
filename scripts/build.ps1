# Builds the game from a plain PowerShell or a double-click (build.cmd at the
# repository root), without a Developer shell or VCPKG_ROOT set by hand. It
# fetches the engine submodule if it is missing, finds Visual Studio's C++
# tools with vswhere, loads vcvars64 into this process, falls back to the vcpkg
# Visual Studio ships, then configures and builds the chosen preset. It
# installs nothing: when Visual Studio is missing it says what to install.
[CmdletBinding()]
param(
    [ValidateSet('x64-release', 'x64-debug')]
    [string] $Preset = 'x64-release',
    # Also build the shareable ZIP (Release only).
    [switch] $Package,
    # Start the game once it is built.
    [switch] $Run
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

function Stop-Build([string] $Message) {
    Write-Host ''
    Write-Host $Message -ForegroundColor Red
    exit 1
}

if ($Package -and $Preset -ne 'x64-release') {
    Stop-Build 'Only the x64-release preset can be packaged.'
}

# A plain clone, or GitHub's Download ZIP, leaves the engine folder empty.
if (-not (Test-Path -LiteralPath (Join-Path $root 'external/labrador/CMakeLists.txt'))) {
    if (-not (Test-Path -LiteralPath (Join-Path $root '.git')) -or
        -not (Get-Command git -ErrorAction SilentlyContinue)) {
        Stop-Build ("The engine (external/labrador) is missing. GitHub's 'Download ZIP' " +
            "does not include it. Install Git and clone instead:`n" +
            '  git clone --recurse-submodules https://github.com/matt439/ColourWars.git')
    }
    Write-Host 'Fetching the engine submodule...'
    git -C $root submodule update --init --recursive
    if ($LASTEXITCODE -ne 0) { Stop-Build 'Fetching the engine submodule failed.' }
}

# The same query CI uses to find a Visual Studio with the x64 C++ tools.
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vs = $null
if (Test-Path -LiteralPath $vswhere) {
    # 2022 (17) is the oldest whose CMake reads these presets.
    $vs = & $vswhere -latest -products * -version '[17.0,' `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath | Select-Object -First 1
}
if (-not $vs) {
    Stop-Build ("No Visual Studio with C++ tools was found. Install Visual Studio 2022 or " +
        "newer (Community is free) with the 'Desktop development with C++' workload:`n" +
        "  https://visualstudio.microsoft.com/downloads/`n" +
        'then run this again. Only want to play? Download the ZIP from ' +
        'https://github.com/matt439/ColourWars/releases instead.')
}
Write-Host "Using Visual Studio at $vs"

# Load vcvars64 into this process: Ninja and cl need the MSVC environment.
# vcvars sets VCPKG_ROOT to the bundled copy unconditionally; keep the caller's.
$callerVcpkg = $env:VCPKG_ROOT
$vcvars = Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
$environment = & cmd.exe /s /c "`"$vcvars`" >nul 2>&1 && set"
if ($LASTEXITCODE -ne 0) { Stop-Build "Loading $vcvars failed." }
foreach ($line in $environment) {
    $equals = $line.IndexOf('=')
    if ($equals -gt 0) {
        [Environment]::SetEnvironmentVariable($line.Substring(0, $equals), $line.Substring($equals + 1))
    }
}
if ($callerVcpkg) { $env:VCPKG_ROOT = $callerVcpkg }

# CMakePresets.json reads VCPKG_ROOT for the toolchain file.
if (-not $env:VCPKG_ROOT) {
    $bundled = Join-Path $vs 'VC/vcpkg'
    if (-not (Test-Path -LiteralPath (Join-Path $bundled 'scripts/buildsystems/vcpkg.cmake'))) {
        Stop-Build ("VCPKG_ROOT is not set and this Visual Studio has no bundled vcpkg. " +
            "Add the 'vcpkg package manager' component in the Visual Studio Installer, " +
            'or set VCPKG_ROOT to your own vcpkg.')
    }
    $env:VCPKG_ROOT = $bundled
}
Write-Host "Using vcpkg at $env:VCPKG_ROOT"

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Stop-Build "CMake was not found. Add 'C++ CMake tools for Windows' in the Visual Studio Installer."
}

Push-Location $root
try {
    # The first configure downloads and builds the vcpkg dependencies.
    Write-Host "Configuring $Preset (the first time takes several minutes)..."
    cmake --preset $Preset
    if ($LASTEXITCODE -ne 0) { Stop-Build 'Configure failed; the messages above say why.' }
    Write-Host "Building $Preset..."
    cmake --build --preset $Preset
    if ($LASTEXITCODE -ne 0) { Stop-Build 'The build failed; the messages above say why.' }
    if ($Package) {
        cmake --build --preset $Preset --target package
        if ($LASTEXITCODE -ne 0) { Stop-Build 'Packaging failed; the messages above say why.' }
    }
}
finally {
    Pop-Location
}

$exe = Join-Path $root "out/build/$Preset/game/ColourWars.exe"
Write-Host ''
Write-Host "Built $exe" -ForegroundColor Green
if ($Package) {
    Write-Host ('ZIP: ' + (Join-Path $root 'out/build/x64-release/packages/ColourWars-windows-x64.zip')) `
        -ForegroundColor Green
}
if ($Run) { Start-Process -FilePath $exe -WorkingDirectory (Split-Path -Parent $exe) }
