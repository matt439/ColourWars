# Shared build contract for probes linking this checkout's game library.
# Callers choose platform adapters explicitly when they provide recording fakes.
function ConvertTo-ProbeResponsePath([string] $Path) {
    if ($Path.Contains('"') -or $Path.Contains("`r") -or $Path.Contains("`n")) {
        throw 'A probe path contains an unsupported quote or newline.'
    }
    return '"' + [IO.Path]::GetFullPath($Path) + '"'
}

function Get-ProbeCaptureLimit([string] $Repo) {
    $header = Join-Path $Repo 'game/debug/capture/debug_perf.h'
    $text = [IO.File]::ReadAllText($header)
    $declarations = [regex]::Matches($text,
        'static\s+constexpr\s+std::size_t\s+max_capture_frames\s*=\s*([1-9][0-9]*)\s*;')
    if ($declarations.Count -ne 1) { throw "Cannot read DebugPerf::max_capture_frames from $header" }
    return [int]::Parse($declarations[0].Groups[1].Value, [Globalization.CultureInfo]::InvariantCulture)
}

function Get-ProbeBuild {
    param(
        [Parameter(Mandatory = $true)][string] $Repo,
        [Parameter(Mandatory = $true)][ValidateSet('x64-debug', 'x64-release')][string] $Preset,
        [ValidateSet('rumble_gamepads', 'display_awake')][string[]] $Adapters = @('rumble_gamepads', 'display_awake'),
        [string[]] $RequiredContent = @()
    )
    $build = Join-Path $Repo "out/build/$Preset"
    $cachePath = Join-Path $build 'CMakeCache.txt'
    if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf)) {
        throw "No configured build at $build. Run without -SkipBuild first."
    }
    $cache = @{}
    foreach ($line in Get-Content -LiteralPath $cachePath) {
        if ($line -match '^([^/#][^:]*):[^=]*=(.*)$') { $cache[$Matches[1]] = $Matches[2] }
    }
    foreach ($key in 'CMAKE_BUILD_TYPE', 'CMAKE_CXX_COMPILER', 'CMAKE_HOME_DIRECTORY',
        'LABRADOR_RENDER_BACKEND', 'LABRADOR_AUDIO_BACKEND', 'LABRADOR_INPUT_BACKEND',
        'VCPKG_INSTALLED_DIR', 'VCPKG_TARGET_TRIPLET') {
        if (-not $cache.ContainsKey($key)) { throw "Configured build is missing $key." }
    }
    $debug = $Preset -eq 'x64-debug'
    $configuration = if ($debug) { 'Debug' } else { 'Release' }
    if (-not [StringComparer]::OrdinalIgnoreCase.Equals([IO.Path]::GetFullPath($cache.CMAKE_HOME_DIRECTORY), $Repo) -or
        $cache.CMAKE_BUILD_TYPE -ne $configuration -or $cache.LABRADOR_RENDER_BACKEND -ne 'd3d11' -or
        $cache.LABRADOR_AUDIO_BACKEND -ne 'xaudio2' -or $cache.LABRADOR_INPUT_BACKEND -ne 'xinput' -or
        $cache.VCPKG_TARGET_TRIPLET -ne 'x64-windows') {
        throw 'The probe requires this checkout and the selected d3d11/xaudio2/xinput x64-windows preset.'
    }
    $game = Join-Path $build 'game'
    $installed = Join-Path $cache.VCPKG_INSTALLED_DIR $cache.VCPKG_TARGET_TRIPLET
    $dependencyLibraries = if ($debug) { Join-Path $installed 'debug/lib' } else { Join-Path $installed 'lib' }
    $libraries = @((Join-Path $game 'colourwars_game.lib'))
    $libraries += @($Adapters | ForEach-Object { Join-Path $game "colourwars_$_.lib" })
    $libraries += @(
        (Join-Path $build 'external/labrador/engine/LabradorEngine.lib'),
        (Join-Path $build 'external/labrador/engine/math/LabradorMath.lib'),
        (Join-Path $dependencyLibraries 'DirectXTK.lib'),
        (Join-Path $dependencyLibraries 'xaudio2_9redist.lib')
    )
    $outputs = @($cache.CMAKE_CXX_COMPILER) + $libraries + @((Join-Path $game 'ColourWars.exe'))
    $outputs += @($RequiredContent | ForEach-Object { Join-Path $game $_ })
    foreach ($path in $outputs) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing build output: $path" }
    }
    $arguments = @('/nologo', '/EHsc', '/std:c++20', '/W4', '/WX', '/permissive-', '/utf-8',
        '/DDIRECTX_TOOLKIT_IMPORT', '/DUSING_XAUDIO2_REDIST', '/DUSING_XINPUT',
        '/DUNICODE', '/D_UNICODE', '/external:W0')
    if ($debug) { $arguments += '/MDd' } else { $arguments += @('/MD', '/DNDEBUG') }
    foreach ($include in @($Repo, (Join-Path $Repo 'external/labrador'), (Join-Path $build 'generated'))) {
        $arguments += '/I' + (ConvertTo-ProbeResponsePath $include)
    }
    # RapidJSON belongs to this client, separately from Labrador's own copy.
    foreach ($include in @((Join-Path $Repo 'external/rapidjson/include'),
        (Join-Path $installed 'include/directxtk'), (Join-Path $installed 'include/xaudio2redist'),
        (Join-Path $installed 'include'))) {
        $arguments += '/external:I' + (ConvertTo-ProbeResponsePath $include)
    }
    [pscustomobject]@{
        Cache = $cache; Debug = $debug; Configuration = $configuration
        GameDirectory = $game; Installed = $installed; DependencyLibraries = $dependencyLibraries
        Libraries = $libraries; CompileArguments = $arguments
        WindowsLibraries = @('d3d11.lib', 'dxgi.lib', 'dxguid.lib', 'kernel32.lib', 'user32.lib',
            'gdi32.lib', 'winspool.lib', 'shell32.lib', 'ole32.lib', 'oleaut32.lib',
            'uuid.lib', 'comdlg32.lib', 'advapi32.lib', 'comctl32.lib', 'Xinput.lib')
    }
}
