# Builds from a plain PowerShell or a double-click (build.cmd), loading the
# Visual Studio environment and its bundled vcpkg when needed. Installs nothing.
[CmdletBinding()]
param(
    [ValidateSet('x64-release', 'x64-debug')]
    [string] $Preset = 'x64-release',
    [switch] $Package,
    [switch] $Run
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$log = Join-Path $root "out/build-$Preset.log"
$help = 'https://github.com/matt439/ColourWars#if-something-goes-wrong'
$transcribing = $false

function Invoke-BuildCommand([string]$Command, [string[]]$Arguments, [string]$Failure) {
    # Re-emit each native line through the host while it arrives. Forwarding
    # the native success stream lets an outer redirect bypass the transcript
    # under Windows PowerShell 5.1, leaving the build log without compiler
    # output. Host writes are both streamed and transcribed synchronously.
    # Stderr joins those lines without becoming a terminating NativeCommandError.
    $previous = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $Command @Arguments 2>&1 | ForEach-Object { Write-Host ([string]$_) }
        $code = $LASTEXITCODE
    }
    finally { $ErrorActionPreference = $previous }
    if ($code -ne 0) { throw "$Failure (exit $code)." }
}

try {
    $null = New-Item -ItemType Directory -Path (Split-Path -Parent $log) -Force
    $null = Start-Transcript -LiteralPath $log -Force
    $transcribing = $true
    Write-Host "Build log: $log"
    if ($Package -and $Preset -ne 'x64-release') {
        throw 'Only the x64-release preset can be packaged.'
    }
    if ($root.Length -ge 80) {
        Write-Warning ("This checkout is in a long path ($($root.Length) characters). " +
            'Dependencies may exceed Windows path limits; use a short folder such as C:\ColourWars if a build fails.')
    }

    if (-not (Test-Path -LiteralPath (Join-Path $root 'external/labrador/CMakeLists.txt'))) {
        if (-not (Test-Path -LiteralPath (Join-Path $root '.git')) -or
            -not (Get-Command git -ErrorAction SilentlyContinue)) {
            throw ("The engine (external/labrador) is missing. GitHub's 'Download ZIP' does not include it. " +
                "Install Git for Windows (https://git-scm.com/downloads/win), then open PowerShell and enter:`n" +
                '  git clone --recurse-submodules https://github.com/matt439/ColourWars.git C:\ColourWars' +
                "`nOpen C:\ColourWars and double-click build.cmd.")
        }
        Write-Host 'Fetching the engine submodule...'
        Invoke-BuildCommand git @('-C', $root, 'submodule', 'update', '--init', '--recursive') 'Fetching the engine submodule failed'
    }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $vs = $null
    if (Test-Path -LiteralPath $vswhere) {
        $vs = & $vswhere -latest -products * -version '[17.0,' `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath | Select-Object -First 1
        if ($LASTEXITCODE -ne 0) { $vs = $null }
    }
    if (-not $vs) {
        throw ("No Visual Studio 2022 or newer with C++ tools was found. Install Visual Studio " +
            "Community (free) from https://visualstudio.microsoft.com/downloads/ and select " +
            "'Desktop development with C++', including a Windows 10 or 11 SDK, " +
            "'C++ CMake tools for Windows' and 'vcpkg package manager'. Then run build.cmd again.")
    }
    Write-Host "Using Visual Studio at $vs"

    # Capture vcvars output separately from `set`: environment values can be
    # private, and must never be printed into the transcript.
    $callerVcpkg = $env:VCPKG_ROOT
    $vcvars = Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
    $vcvarsLog = Join-Path $root "out/vcvars-$Preset.log"
    $previous = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $environment = & cmd.exe /d /s /c "`"$vcvars`" >`"$vcvarsLog`" 2>&1 && set"
        $vcvarsExit = $LASTEXITCODE
    }
    finally { $ErrorActionPreference = $previous }
    if ($vcvarsExit -ne 0) {
        if (Test-Path -LiteralPath $vcvarsLog) { Get-Content -LiteralPath $vcvarsLog | ForEach-Object { Write-Host $_ } }
        throw "Loading Visual Studio's C++ environment failed. Repair the C++ workload and Windows SDK in Visual Studio Installer. Details: $vcvarsLog"
    }
    foreach ($line in $environment) {
        $equals = $line.IndexOf('=')
        if ($equals -gt 0) {
            [Environment]::SetEnvironmentVariable($line.Substring(0, $equals), $line.Substring($equals + 1))
        }
    }
    if (-not (Get-Command fxc.exe -ErrorAction SilentlyContinue)) {
        throw "The Windows SDK shader compiler (fxc.exe) was not found. In Visual Studio Installer, modify the C++ workload and add a Windows 10 or 11 SDK."
    }

    $bundled = Join-Path $vs 'VC/vcpkg'
    if ($callerVcpkg -and (Test-Path -LiteralPath (Join-Path $callerVcpkg 'scripts/buildsystems/vcpkg.cmake') -PathType Leaf)) {
        $env:VCPKG_ROOT = $callerVcpkg
    }
    else {
        if ($callerVcpkg) { Write-Warning "VCPKG_ROOT does not contain a vcpkg toolchain: $callerVcpkg. Trying Visual Studio's bundled copy." }
        if (-not (Test-Path -LiteralPath (Join-Path $bundled 'scripts/buildsystems/vcpkg.cmake') -PathType Leaf)) {
            throw "No usable vcpkg was found. Add 'vcpkg package manager' in Visual Studio Installer, or set VCPKG_ROOT to a vcpkg folder containing scripts/buildsystems/vcpkg.cmake."
        }
        $env:VCPKG_ROOT = $bundled
    }
    Write-Host "Using vcpkg at $env:VCPKG_ROOT"
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        throw "CMake was not found. Add 'C++ CMake tools for Windows' in Visual Studio Installer."
    }

    Push-Location $root
    try {
        Write-Host "Configuring $Preset (the first time downloads dependencies and can take several minutes)..."
        Invoke-BuildCommand cmake @('--preset', $Preset) 'Configure failed; check the download, disk-space and path guidance at the help link below'
        Write-Host "Building $Preset (the first compile can take several minutes; compiler output follows)..."
        Invoke-BuildCommand cmake @('--build', '--preset', $Preset) 'The build failed; the compiler messages above name the failing file'
        if ($Package) {
            Invoke-BuildCommand cmake @('--build', '--preset', $Preset, '--target', 'package') 'Packaging failed'
        }
    }
    finally { Pop-Location }

    $exe = Join-Path $root "out/build/$Preset/game/ColourWars.exe"
    Write-Host ''
    Write-Host "Built $exe" -ForegroundColor Green
    Write-Host "Player guide: $(Join-Path (Split-Path -Parent $exe) 'HOW-TO-PLAY.txt')"
    Write-Host "Play again without rebuilding: double-click $exe"
    if ($Package) {
        . (Join-Path $PSScriptRoot 'release-package-path.ps1')
        Write-Host ('ZIP: ' + (Get-ColourWarsReleasePackagePath $root)) -ForegroundColor Green
    }
    # -Run explicitly requests the interactive game window.
    if ($Run) { Start-Process -FilePath $exe -WorkingDirectory (Split-Path -Parent $exe) }
}
catch {
    Write-Host ''
    Write-Host $_.Exception.Message -ForegroundColor Red
    Write-Host "Build log: $log"
    Write-Host "Help: $help"
    exit 1
}
finally {
    if ($transcribing) { $null = Stop-Transcript }
}
