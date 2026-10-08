[CmdletBinding()]
param(
    [ValidateSet('x64-debug', 'x64-release')]
    [string] $Preset = 'x64-debug',
    [switch] $SkipBuild,
    # Optional consecutive 720p art-study frames at 60 fps; not gameplay footage.
    [ValidateRange(0, 90)]
    [int] $SequenceFrames = 0,
    [string] $KitDirectory = '',
    [string] $OutputDirectory = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Invoke-Logged {
    param([string] $Command, [string[]] $Arguments, [string] $LogPath)
    $ErrorActionPreference = 'Continue'
    $output = & $Command @Arguments 2>&1
    $code = $LASTEXITCODE
    $output | Out-File -LiteralPath $LogPath -Encoding utf8
    if ($code -ne 0) {
        $output | ForEach-Object { Write-Host $_ }
        throw "$Command exited with $code. See $LogPath"
    }
}

function Quote-Path {
    param([string] $Path)
    if ($Path.Contains('"') -or $Path.Contains("`r") -or $Path.Contains("`n")) {
        throw 'A path contains an unsupported quote or newline.'
    }
    return '"' + [IO.Path]::GetFullPath($Path) + '"'
}

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if ($KitDirectory -eq '') { $KitDirectory = Join-Path $repo 'game/art/polish/kit' }
if ($OutputDirectory -eq '') { $OutputDirectory = Join-Path $repo 'out/validation/workshop-style' }
$build = Join-Path $repo "out/build/$Preset"
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
$kit = [IO.Path]::GetFullPath($KitDirectory)
$run = Join-Path (Join-Path $outputRoot $Preset) ([Guid]::NewGuid().ToString('N'))
$captures = Join-Path $run 'captures'
$working = Join-Path $run 'working-directory'
$profile = Join-Path $run 'profile'
$null = New-Item -ItemType Directory -Path $run, $captures, $working, $profile -Force

foreach ($command in 'cmake', 'cl', 'git') {
    if (-not (Get-Command $command -ErrorAction SilentlyContinue)) {
        throw "Run this script from an x64 Visual Studio Developer shell ($command is missing)."
    }
}
if (-not $env:INCLUDE -or -not $env:LIB -or
    ($env:VSCMD_ARG_TGT_ARCH -and $env:VSCMD_ARG_TGT_ARCH -ne 'x64')) {
    throw 'This probe requires the x64 Visual Studio Developer environment.'
}

Push-Location -LiteralPath $repo
$previousPath = $env:PATH
$previousLocalAppData = $env:LOCALAPPDATA
$process = $null
$failure = $null
$result = [ordered]@{
    outcome = 'failed'
    preset = $Preset
    game_revision = (& git rev-parse HEAD)
    engine_revision = (& git -C external/labrador rev-parse HEAD)
    worktree_status = @(& git status --short)
    scope = 'Renderer/camera art study only; static mannequins and synthetic paint. No gameplay, collision, score, audio or controller acceptance.'
    player_counts = @(1, 4)
    palettes = @('blue_orange', 'pink_green', 'blue_yellow')
    terrain = @('clean', 'painted')
    resolutions = @('1280x720', '1920x1080', '2560x1440')
    weapons = @('sprayer', 'roller', 'sprayer', 'roller')
    inputs = 'Scripted aim and camera targets; kit body poses are static. Optional 60 fps sequence is an art-study animation.'
    audio = 'Muted; this check establishes no listening acceptance.'
    sequence_frames = $SequenceFrames
    profile = $profile
    diagnostic = ''
}
try {
    $kitFiles = @('source.json', 'workshop_kit.png', 'workshop_kit.json',
        'workshop_kit.dds', 'workshop_kit.layout.json')
    foreach ($name in $kitFiles) {
        if (-not (Test-Path -LiteralPath (Join-Path $kit $name) -PathType Leaf)) {
            throw "Missing kit file: $name. Run scripts/build-workshop-kit.ps1 first."
        }
    }
    $layout = Get-Content -LiteralPath (Join-Path $kit 'workshop_kit.layout.json') -Raw | ConvertFrom-Json
    $sourceHash = (Get-FileHash -LiteralPath (Join-Path $kit 'source.json')).Hash
    if ($layout.source_sha256 -ine $sourceHash) { throw 'Kit layout was generated from a different source.json.' }
    $result.kit_directory = $kit
    $result.kit_hashes = @($kitFiles | ForEach-Object {
        [ordered]@{ path = $_; sha256 = (Get-FileHash -LiteralPath (Join-Path $kit $_)).Hash }
    })
    if (-not $SkipBuild) {
        Write-Host "Configure and build $Preset; logs: $run"
        Invoke-Logged -Command 'cmake' -Arguments @('--preset', $Preset) `
            -LogPath (Join-Path $run 'configure.log')
        Invoke-Logged -Command 'cmake' -Arguments @('--build', '--preset', $Preset) `
            -LogPath (Join-Path $run 'build.log')
    }
    $cachePath = Join-Path $build 'CMakeCache.txt'
    if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf)) {
        throw "No configured build at $build. Run without -SkipBuild first."
    }
    $cache = @{}
    foreach ($line in Get-Content -LiteralPath $cachePath) {
        if ($line -match '^([^/#][^:]*):[^=]*=(.*)$') { $cache[$Matches[1]] = $Matches[2] }
    }
    foreach ($key in 'CMAKE_BUILD_TYPE', 'CMAKE_CXX_COMPILER', 'CMAKE_HOME_DIRECTORY',
        'LABRADOR_RENDER_BACKEND', 'LABRADOR_AUDIO_BACKEND', 'VCPKG_INSTALLED_DIR',
        'VCPKG_TARGET_TRIPLET') {
        if (-not $cache.ContainsKey($key)) { throw "Configured build is missing $key." }
    }
    $debug = $Preset -eq 'x64-debug'
    $configuration = if ($debug) { 'Debug' } else { 'Release' }
    if ([IO.Path]::GetFullPath($cache.CMAKE_HOME_DIRECTORY) -ne $repo -or
        $cache.CMAKE_BUILD_TYPE -ne $configuration -or
        $cache.LABRADOR_RENDER_BACKEND -ne 'd3d11' -or
        $cache.LABRADOR_AUDIO_BACKEND -ne 'xaudio2' -or
        $cache.VCPKG_TARGET_TRIPLET -ne 'x64-windows') {
        throw 'The probe requires this checkout and the selected d3d11/xaudio2 x64-windows preset.'
    }
    $gameDirectory = Join-Path $build 'game'
    $objects = @(Get-ChildItem -LiteralPath (Join-Path $gameDirectory 'CMakeFiles/ColourWars.dir') `
        -Filter '*.obj' -File -Recurse | Where-Object { $_.Name -ne 'main.cpp.obj' } | Sort-Object FullName)
    if ($objects.Count -eq 0) { throw 'Game objects are missing. Run without -SkipBuild first.' }
    $installed = Join-Path $cache.VCPKG_INSTALLED_DIR $cache.VCPKG_TARGET_TRIPLET
    $dependencyLibraries = if ($debug) { Join-Path $installed 'debug/lib' } else { Join-Path $installed 'lib' }
    $libraries = @(
        (Join-Path $build 'external/labrador/engine/LabradorEngine.lib'),
        (Join-Path $build 'external/labrador/engine/math/MattMath.lib'),
        (Join-Path $dependencyLibraries 'DirectXTK.lib'),
        (Join-Path $dependencyLibraries 'xaudio2_9redist.lib')
    )
    foreach ($path in @($cache.CMAKE_CXX_COMPILER) + $libraries +
        @((Join-Path $gameDirectory 'ColourWars.exe'), (Join-Path $gameDirectory 'presentation.json'))) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing build output: $path" }
    }
    $source = Join-Path $PSScriptRoot 'acceptance/workshop_style_visual.cpp'
    $result.probe_sha256 = (Get-FileHash -LiteralPath $source).Hash
    $executable = Join-Path $run 'workshop_style_visual.exe'
    $responseFile = Join-Path $run 'compile.rsp'
    $arguments = @('/nologo', '/EHsc', '/std:c++20', '/W4', '/WX', '/permissive-',
        '/DDIRECTX_TOOLKIT_IMPORT', '/DUSING_XAUDIO2_REDIST', '/DUSING_XINPUT',
        '/DUNICODE', '/D_UNICODE', '/external:W0')
    if ($debug) { $arguments += '/MDd' } else { $arguments += @('/MD', '/DNDEBUG') }
    foreach ($include in @($repo, (Join-Path $repo 'external/labrador'), (Join-Path $build 'generated'))) {
        $arguments += '/I' + (Quote-Path $include)
    }
    foreach ($include in @((Join-Path $installed 'include/directxtk'),
        (Join-Path $installed 'include/xaudio2redist'), (Join-Path $installed 'include'))) {
        $arguments += '/external:I' + (Quote-Path $include)
    }
    $arguments += @((Quote-Path $source), ('/Fe:' + (Quote-Path $executable)),
        ('/Fo:' + (Quote-Path (Join-Path $run 'workshop_style_visual.obj'))))
    $arguments += @($objects | ForEach-Object { Quote-Path $_.FullName })
    $arguments += @($libraries | ForEach-Object { Quote-Path $_ })
    $arguments += @('d3d11.lib', 'dxgi.lib', 'dxguid.lib', 'kernel32.lib', 'user32.lib',
        'gdi32.lib', 'winspool.lib', 'shell32.lib', 'ole32.lib', 'oleaut32.lib',
        'uuid.lib', 'comdlg32.lib', 'advapi32.lib')
    $arguments | Out-File -LiteralPath $responseFile -Encoding unicode
    Write-Host "Compile workshop style visual probe ($Preset)"
    Invoke-Logged -Command $cache.CMAKE_CXX_COMPILER -Arguments @("@$responseFile") `
        -LogPath (Join-Path $run 'compile.log')

    $result.staged_content_hashes = @(
        'manifest.json', 'tuning.json', 'presentation.json' |
        ForEach-Object {
            [ordered]@{ path = $_; sha256 = (Get-FileHash -LiteralPath (Join-Path $gameDirectory $_)).Hash }
        }
    )
    $env:PATH = "$gameDirectory;$previousPath"
    $env:LOCALAPPDATA = $profile
    $process = Start-Process -FilePath $executable -WindowStyle Hidden -PassThru `
        -WorkingDirectory $working -ArgumentList @((Quote-Path $gameDirectory),
            (Quote-Path $kit), (Quote-Path $captures), $SequenceFrames.ToString()) `
        -RedirectStandardOutput (Join-Path $run 'run.log') `
        -RedirectStandardError (Join-Path $run 'run.err')
    $null = $process.Handle
    $env:PATH = $previousPath
    $env:LOCALAPPDATA = $previousLocalAppData
    $timer = [Diagnostics.Stopwatch]::StartNew()
    while (-not $process.WaitForExit(1000)) {
        if ($timer.Elapsed.TotalSeconds -gt 180) { throw 'Visual probe exceeded its 180-second runtime limit.' }
    }
    $process.WaitForExit()
    Get-Content -LiteralPath (Join-Path $run 'run.log') | ForEach-Object { Write-Host $_ }
    if ($process.ExitCode -ne 0) {
        Get-Content -LiteralPath (Join-Path $run 'run.err') | ForEach-Object { Write-Host $_ }
        throw "Visual probe exited with $($process.ExitCode)."
    }
    foreach ($line in Get-Content -LiteralPath (Join-Path $run 'run.err')) {
        if ($line.Trim() -ne '' -and
            $line -notmatch '^no audio: .*The manifest marks this bank optional, so it plays nothing\.') {
            throw "Unexpected probe diagnostic: $line"
        }
    }
    if (Test-Path -LiteralPath (Join-Path $profile 'ColourWars/save_data.json')) {
        throw 'The visual probe unexpectedly created a save.'
    }
    foreach ($entry in $result.kit_hashes) {
        if ((Get-FileHash -LiteralPath (Join-Path $kit $entry.path)).Hash -ne $entry.sha256) {
            throw "Kit changed during capture: $($entry.path)"
        }
    }
    $geometry = @(Import-Csv -LiteralPath (Join-Path $captures 'geometry.csv'))
    $result.matrix_capture_count = @(Get-ChildItem -LiteralPath $captures -Filter '*.ppm' -File).Count
    if ($result.matrix_capture_count -ne 36) { throw 'Art matrix did not retain all 36 captures.' }
    $result.geometry_rows = $geometry.Count
    $result.max_world_anchor_error = ($geometry | ForEach-Object {
        [double]::Parse($_.world_anchor_error, [Globalization.CultureInfo]::InvariantCulture)
    } | Measure-Object -Maximum).Maximum
    $result.max_screen_anchor_error_pixels = ($geometry | ForEach-Object {
        [double]::Parse($_.screen_anchor_error_pixels, [Globalization.CultureInfo]::InvariantCulture)
    } | Measure-Object -Maximum).Maximum
    $result.outcome = 'passed'
    $result.diagnostic = 'Art composition matrix and identical redraws passed. Anchor diagnostics retained. Human style/motion review remains separate.'
}
catch {
    $failure = $_
    $result.diagnostic = $_.Exception.Message
}
finally {
    $env:PATH = $previousPath
    $env:LOCALAPPDATA = $previousLocalAppData
    if ($null -ne $process) {
        if (-not $process.HasExited) {
            # Only this isolated probe; it owns no player save or session.
            $process.Kill()
            $null = $process.WaitForExit(3000)
        }
        $process.Dispose()
    }
    $result | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $run 'result.json') -Encoding utf8
    Write-Host "Workshop style evidence: $run"
    Pop-Location
}
if ($null -ne $failure) { throw $failure }
