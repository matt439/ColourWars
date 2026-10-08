[CmdletBinding()]
param(
    [ValidateSet('x64-debug', 'x64-release')]
    [string] $Preset = 'x64-debug',
    [switch] $SkipBuild
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Invoke-Logged {
    param(
        [string] $Command,
        [string[]] $Arguments,
        [string] $LogPath,
        [switch] $ShowOutput
    )

    # Windows PowerShell represents native stderr as ErrorRecord instances.
    # The executable's exit code decides success; preserve both streams.
    $ErrorActionPreference = 'Continue'
    $output = & $Command @Arguments 2>&1
    $code = $LASTEXITCODE
    $output | Out-File -LiteralPath $LogPath -Encoding utf8
    if ($ShowOutput -or $code -ne 0) {
        $output | ForEach-Object { Write-Host $_ }
    }
    if ($code -ne 0) {
        throw "$Command exited with $code. See $LogPath"
    }
}

function Quote-ResponsePath {
    param([string] $Path)
    if ($Path.Contains('"') -or $Path.Contains("`r") -or $Path.Contains("`n")) {
        throw 'A compiler path contains an unsupported quote or newline.'
    }
    return '"' + [System.IO.Path]::GetFullPath($Path) + '"'
}

$repo = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$build = Join-Path $repo "out/build/$Preset"
$evidence = Join-Path $repo "out/acceptance/$Preset"
[System.IO.Directory]::CreateDirectory($evidence) | Out-Null

foreach ($command in 'cmake', 'cl') {
    if (-not (Get-Command $command -ErrorAction SilentlyContinue)) {
        throw "Run this script from an x64 Visual Studio Developer shell ($command is missing)."
    }
}
if (-not $env:INCLUDE -or -not $env:LIB) {
    throw 'Run this script from an x64 Visual Studio Developer shell (INCLUDE/LIB are missing).'
}
if ($env:VSCMD_ARG_TGT_ARCH -and $env:VSCMD_ARG_TGT_ARCH -ne 'x64') {
    throw 'The acceptance executables require the x64 Visual Studio environment.'
}

Push-Location -LiteralPath $repo
$previousPath = $env:PATH
try {
    if (-not $SkipBuild) {
        Write-Host "Configure and build $Preset; logs: $evidence"
        Invoke-Logged -Command 'cmake' -Arguments @('--preset', $Preset) `
            -LogPath (Join-Path $evidence 'configure.log')
        Invoke-Logged -Command 'cmake' -Arguments @('--build', '--preset', $Preset) `
            -LogPath (Join-Path $evidence 'build.log')
    }

    $cachePath = Join-Path $build 'CMakeCache.txt'
    if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf)) {
        throw "No configured build at $build. Run without -SkipBuild first."
    }
    $cache = @{}
    foreach ($line in Get-Content -LiteralPath $cachePath) {
        if ($line -match '^([^/#][^:]*):[^=]*=(.*)$') {
            $cache[$Matches[1]] = $Matches[2]
        }
    }
    foreach ($key in 'CMAKE_BUILD_TYPE', 'CMAKE_CXX_COMPILER', 'CMAKE_HOME_DIRECTORY',
        'LABRADOR_RENDER_BACKEND', 'LABRADOR_AUDIO_BACKEND', 'VCPKG_INSTALLED_DIR',
        'VCPKG_TARGET_TRIPLET') {
        if (-not $cache.ContainsKey($key)) {
            throw "The configured build is missing $key. Configure it again."
        }
    }
    if ([System.IO.Path]::GetFullPath($cache.CMAKE_HOME_DIRECTORY) -ne $repo) {
        throw 'The configured build belongs to another checkout.'
    }
    if ($cache.LABRADOR_RENDER_BACKEND -ne 'd3d11' -or
        $cache.LABRADOR_AUDIO_BACKEND -ne 'xaudio2' -or
        $cache.VCPKG_TARGET_TRIPLET -ne 'x64-windows') {
        throw 'These consumer acceptance cases use the d3d11/xaudio2 x64-windows game build.'
    }
    $debug = $Preset -eq 'x64-debug'
    $configuration = if ($debug) { 'Debug' } else { 'Release' }
    if ($cache.CMAKE_BUILD_TYPE -ne $configuration) {
        throw "$Preset must contain a $configuration build."
    }

    $gameDirectory = Join-Path $build 'game'
    $objectDirectory = Join-Path $gameDirectory 'CMakeFiles/ColourWars.dir'
    $objects = @(Get-ChildItem -LiteralPath $objectDirectory -Filter '*.obj' -File -Recurse |
        Where-Object { $_.Name -ne 'main.cpp.obj' } |
        Sort-Object FullName)
    if ($objects.Count -eq 0) {
        throw 'No game objects exist. Run without -SkipBuild first.'
    }

    $installed = Join-Path $cache.VCPKG_INSTALLED_DIR $cache.VCPKG_TARGET_TRIPLET
    $dependencyLibraries = if ($debug) { Join-Path $installed 'debug/lib' } else {
        Join-Path $installed 'lib'
    }
    $libraries = @(
        (Join-Path $build 'external/labrador/engine/LabradorEngine.lib'),
        (Join-Path $build 'external/labrador/engine/math/MattMath.lib'),
        (Join-Path $dependencyLibraries 'DirectXTK.lib'),
        (Join-Path $dependencyLibraries 'xaudio2_9redist.lib')
    )
    foreach ($path in @($cache.CMAKE_CXX_COMPILER) + $libraries +
        @((Join-Path $gameDirectory 'ColourWars.exe'), (Join-Path $gameDirectory 'tuning.json'))) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "A required build output is missing: $path"
        }
    }
    $windowsLibraries = @('d3d11.lib', 'dxgi.lib', 'dxguid.lib', 'kernel32.lib',
        'user32.lib', 'gdi32.lib', 'winspool.lib', 'shell32.lib', 'ole32.lib',
        'oleaut32.lib', 'uuid.lib', 'comdlg32.lib', 'advapi32.lib')
    $common = @('/nologo', '/EHsc', '/std:c++20', '/W4', '/WX', '/permissive-',
        '/DDIRECTX_TOOLKIT_IMPORT', '/DUSING_XAUDIO2_REDIST', '/DUSING_XINPUT',
        '/DUNICODE', '/D_UNICODE', '/external:W0')
    if ($debug) { $common += '/MDd' } else { $common += @('/MD', '/DNDEBUG') }
    foreach ($include in @($repo, (Join-Path $repo 'external/labrador'),
        (Join-Path $build 'generated'))) {
        $common += '/I' + (Quote-ResponsePath $include)
    }
    foreach ($include in @((Join-Path $installed 'include/directxtk'),
        (Join-Path $installed 'include/xaudio2redist'), (Join-Path $installed 'include'))) {
        $common += '/external:I' + (Quote-ResponsePath $include)
    }

    # The probes use CPU-side game data and do not create a window or device.
    # DLL imports still resolve from the same staged folder as the game.
    $env:PATH = "$gameDirectory;$previousPath"
    foreach ($case in 'ammo', 'settings', 'paint', 'camera_framing', 'movement', 'stage_selection') {
        $source = Join-Path $PSScriptRoot "acceptance/$case.cpp"
        $executable = Join-Path $evidence "$case.exe"
        $responseFile = Join-Path $evidence "$case.rsp"
        $arguments = $common + @(
            (Quote-ResponsePath $source),
            ('/Fe:' + (Quote-ResponsePath $executable)),
            ('/Fo:' + (Quote-ResponsePath (Join-Path $evidence "$case.obj")))
        )
        $arguments += @($objects | ForEach-Object { Quote-ResponsePath $_.FullName })
        $arguments += @($libraries | ForEach-Object { Quote-ResponsePath $_ })
        $arguments += $windowsLibraries
        # CL accepts UTF-16 response files, including checkout paths that are
        # not representable in the machine's active ANSI code page.
        $arguments | Out-File -LiteralPath $responseFile -Encoding unicode
        Write-Host "Compile acceptance: $case ($Preset)"
        Invoke-Logged -Command $cache.CMAKE_CXX_COMPILER -Arguments @("@$responseFile") `
            -LogPath (Join-Path $evidence "$case-build.log")
        Invoke-Logged -Command $executable -Arguments @($gameDirectory) `
            -LogPath (Join-Path $evidence "$case-run.log") -ShowOutput
    }
    Write-Host "PASS acceptance ($Preset). Evidence: $evidence"
}
finally {
    $env:PATH = $previousPath
    Pop-Location
}
