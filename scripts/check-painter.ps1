[CmdletBinding()]
param(
    [ValidateSet('x64-debug', 'x64-release')]
    [string] $Preset = 'x64-debug',
    [switch] $SkipBuild,
    [switch] $IdentityOnly,
    [switch] $QuietOnly,
    [switch] $MonitorOnly,
    [string] $OutputDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (([int]$IdentityOnly.IsPresent + [int]$QuietOnly.IsPresent + [int]$MonitorOnly.IsPresent) -gt 1) {
    throw 'Choose only one focused painter matrix.'
}

# Windows PowerShell 5.1 does not set PSScriptRoot during parameter defaults.
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $PSScriptRoot '../out/validation/painter'
}

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

# Bound every page, including the complete matrix, before allocating a bitmap.
# GDI+ cannot save an arbitrarily tall aggregate even when every crop is valid.
function Write-ContactSheetPages {
    param([object[]] $Images, [int] $CellWidth, [int] $CellHeight,
        [string] $CaptureDirectory, [string] $Stem)
    if ($Images.Count -eq 0) { return }
    $pageWidth = $CellWidth * 4
    $rows = [int][Math]::Min(16, [Math]::Floor(8192.0 / $CellHeight))
    $rows = [int][Math]::Min($rows, [Math]::Floor(16777216.0 / ($pageWidth * $CellHeight)))
    if ($pageWidth -gt 16384 -or $rows -lt 1) { throw 'A native-size painter crop exceeds the review page bounds.' }
    $perPage = $rows * 4
    $font = [Drawing.Font]::new('Arial', 9)
    try {
        for ($start = 0; $start -lt $Images.Count; $start += $perPage) {
            $count = [Math]::Min($perPage, $Images.Count - $start)
            $sheet = $null
            $graphics = $null
            try {
                $sheet = [Drawing.Bitmap]::new($pageWidth, $CellHeight * [int][Math]::Ceiling($count / 4.0))
                $graphics = [Drawing.Graphics]::FromImage($sheet)
                $graphics.Clear([Drawing.Color]::FromArgb(238, 238, 238))
                for ($index = 0; $index -lt $count; ++$index) {
                    $x = ($index % 4) * $CellWidth + 8
                    $y = [int][Math]::Floor($index / 4.0) * $CellHeight + 8
                    $item = $Images[$start + $index]
                    $graphics.DrawString($item.name, $font, [Drawing.Brushes]::Black,
                        [Drawing.RectangleF]::new($x, $y, $CellWidth - 16, 34))
                    $graphics.DrawImageUnscaled($item.bitmap, $x, $y + 36)
                }
                $page = [int]($start / $perPage) + 1
                $name = if ($page -eq 1) { "$Stem.png" } else { '{0}-{1:D3}.png' -f $Stem, $page }
                $sheet.Save((Join-Path $CaptureDirectory $name), [Drawing.Imaging.ImageFormat]::Png)
                Write-Output $name
            }
            finally {
                if ($null -ne $graphics) { $graphics.Dispose() }
                if ($null -ne $sheet) { $sheet.Dispose() }
            }
        }
    }
    finally { $font.Dispose() }
}

# Keep the probes' lossless PPMs, and provide labelled playing-size PNG pages.
# Crops are never enlarged for the readability review.
function Write-ContactSheet {
    param([string] $CaptureDirectory)
    Add-Type -AssemblyName System.Drawing
    $images = [Collections.Generic.List[object]]::new()
    try {
        $cellWidth = 320
        $cellHeight = 160
        $monitorCellWidth = 320
        $monitorCellHeight = 160
        foreach ($file in Get-ChildItem -LiteralPath $CaptureDirectory -Filter '*.ppm' -File |
            Where-Object { $_.Name -like '*-player*.ppm' -or $_.Name -like 'monitor-*.ppm' } | Sort-Object Name) {
            $bytes = [IO.File]::ReadAllBytes($file.FullName)
            $header = [Text.Encoding]::ASCII.GetString($bytes, 0, [Math]::Min(64, $bytes.Length))
            if ($header -notmatch '^P6\n(\d+) (\d+)\n255\n') { throw "Unsupported capture header: $($file.FullName)" }
            $width = [int] $Matches[1]
            $height = [int] $Matches[2]
            $source = $Matches[0].Length
            $bitmap = [Drawing.Bitmap]::new($width, $height, [Drawing.Imaging.PixelFormat]::Format24bppRgb)
            $images.Add([pscustomobject]@{ name = $file.BaseName; bitmap = $bitmap })
            $data = $bitmap.LockBits([Drawing.Rectangle]::new(0, 0, $width, $height),
                [Drawing.Imaging.ImageLockMode]::WriteOnly, [Drawing.Imaging.PixelFormat]::Format24bppRgb)
            try {
                $pixels = [byte[]]::new($data.Stride * $height)
                for ($y = 0; $y -lt $height; ++$y) {
                    for ($x = 0; $x -lt $width; ++$x) {
                        $destination = $y * $data.Stride + $x * 3
                        $pixels[$destination] = $bytes[$source + 2]
                        $pixels[$destination + 1] = $bytes[$source + 1]
                        $pixels[$destination + 2] = $bytes[$source]
                        $source += 3
                    }
                }
                [Runtime.InteropServices.Marshal]::Copy($pixels, 0, $data.Scan0, $pixels.Length)
            }
            finally { $bitmap.UnlockBits($data) }
            $bitmap.Save([IO.Path]::ChangeExtension($file.FullName, '.png'), [Drawing.Imaging.ImageFormat]::Png)
            if ($file.BaseName -like 'monitor-*') {
                $monitorCellWidth = [Math]::Max($monitorCellWidth, $width + 16)
                $monitorCellHeight = [Math]::Max($monitorCellHeight, $height + 48)
            }
            else {
                $cellWidth = [Math]::Max($cellWidth, $width + 16)
                $cellHeight = [Math]::Max($cellHeight, $height + 48)
            }
        }
        if ($images.Count -eq 0) { throw 'No painter or monitor captures were produced.' }
        $painters = @($images | Where-Object { $_.name -notlike 'monitor-*' })
        Write-ContactSheetPages -Images $painters -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-contact'
        # Keep the complete matrix above, and compact action review pages
        # with one native-size figure per action/frame/facing or 4P palette.
        # Selecting view 1 avoids repeating the four panes on this summary;
        # every player crop remains available beside it for teammate review.
        $actions = @($images | Where-Object { $_.name -match '^[14]p-action-.*-player1$' })
        Write-ContactSheetPages -Images $actions -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-actions-contact'
        $tools = @($images | Where-Object {
            $_.name -match '^1p-tool-.*-(idle|paid-shot|held-fire|release|recovery|travel|lift)-player1$' -or
            $_.name -match '^4p-tool-mixed-.*-(idle|held-fire|release)-player[1-4]$' -or
            $_.name -match '^4p-tool-roller-.*-(travel|lift)-player1$'
        })
        Write-ContactSheetPages -Images $tools -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-tools-contact'
        $weapons = @($images | Where-Object {
            $_.name -match '^1p-weapon-.*-player1$' -or
            $_.name -match '^4p-weapon-(mixed|controlled)-.*-player1$'
        })
        Write-ContactSheetPages -Images $weapons -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-weapons-contact'
        $quiet = @($images | Where-Object {
            $_.name -match '^[14]p-quiet-.*-detail-player1$' -or
            $_.name -match '^4p-quiet-blue_orange-(breath|blink)-1280-detail-player[2-4]$'
        })
        Write-ContactSheetPages -Images $quiet -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-quiet-contact'
        $markers = @($images | Where-Object { $_.name -match '^4p-aim-marker-.*-player[1-4]$' })
        Write-ContactSheetPages -Images $markers -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-aim-markers-contact'
        $blocked = @($images | Where-Object { $_.name -match '^blocked-.*-context-player[1-4]$' })
        Write-ContactSheetPages -Images $blocked -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-blocked-shots-contact'
        $identity = @($images | Where-Object {
            $_.name -match '^[24]p-identity-.*-context-player[1-4]$' -or
            $_.name -match '^4p-identity-glyph-.*-player[1-4]$'
        })
        Write-ContactSheetPages -Images $identity -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-player-identity-contact'
        $monitors = @($images | Where-Object { $_.name -like 'monitor-*' })
        Write-ContactSheetPages -Images $monitors -CellWidth $monitorCellWidth -CellHeight $monitorCellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-monitor-contact'
    }
    finally {
        foreach ($item in $images) { $item.bitmap.Dispose() }
    }
}

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$build = Join-Path $repo "out/build/$Preset"
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
$run = Join-Path (Join-Path $outputRoot $Preset) ([Guid]::NewGuid().ToString('N'))
$captures = Join-Path $run 'captures'
$working = Join-Path $run 'working-directory'
$profile = Join-Path $run 'profile'
$null = New-Item -ItemType Directory -Path $run, $captures, $working, $profile -Force
$saveDirectory = Join-Path $profile 'ColourWars'
$null = New-Item -ItemType Directory -Path $saveDirectory -Force
$saveFixture = Join-Path $saveDirectory 'save_data.json'
[IO.File]::WriteAllText($saveFixture, '{"aim_marker":false}', [Text.UTF8Encoding]::new($false))
$saveFixtureHash = (Get-FileHash -LiteralPath $saveFixture).Hash

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
    scope = if ($IdentityOnly) { 'player-identity-only' } elseif ($QuietOnly) { 'quiet-only' } elseif ($MonitorOnly) { 'monitor-only' } else { 'full-painter-matrix' }
    preset = $Preset
    game_revision = (& git rev-parse HEAD)
    engine_revision = (& git -C external/labrador rev-parse HEAD)
    worktree_status = @(& git status --short)
    stage = 'movement_course'
    player_counts = @(1, 4)
    palettes = @('blue_orange', 'pink_green', 'blue_yellow')
    weapons = @('sprayer', 'sniper', 'mister', 'bucket', 'roller')
    actions = @('takeoff', 'land', 'fire', 'hurt', 'elimination')
    input_slots = @(3, 1, 2, 0)
    inputs = 'Scripted pad-slot inputs at the real 60 Hz simulation step; no physical-device claim.'
    audio = 'Muted; this check establishes no listening acceptance.'
    contact_checks = 'Real resolved 4P dry steps/reversals; controlled step/skid initial/middle/expired rasters in all palettes at 720p/1080p/QHD. Dry/painted and left/right raster differences, enabled/disabled comparisons, frozen direction/tint/timers and culling bounds.'
    weapon_checks = 'Actual Weapon firing workflow with locally retained projectiles: 1P all five tools and 4P mixed tools in all three palettes, four aim quadrants, then live aim/flip changes while each paid-shot pulse remains active. Roller has no muzzle pulse. Controlled 4P muzzle/empty/recovered event triggers at initial/middle/expiry, enabled/disabled at the same simulation state, at 720p/1080p/QHD. Frozen redraw and observed feedback timers, current-nozzle attachment and raster culling bounds. Empty/recovery gameplay producers are covered by separate CPU acceptance checks. Mechanism raster comparisons suppress the new pulses.'
    quiet_checks = 'Actual builder/Scene 1P and 4P sprayers in all three palettes at 720p: settled breath crest, bead growth/stretch/release/fall, blink, movement interruption and left lean. Sniper residual beads and bucket rim runoff add growth/stretch/fall, same-bead continuous four-quadrant aim with authored lip/stripe anchors, both body facings, per-tool enabled suppression, and same-state reduced-effects hiding/restoration for attached and falling liquid in every 1P/4P720 palette. Representative attached-bead extents at 1080p/QHD. Mature bead raster touches the actual rendered tool silhouette within one pixel. Same-state enabled/disabled breathing/shift rasters and controlled blink-only comparison at playing size, distinct roster phases, frozen cosmetic timers/drops, separate Scene drip raster bounds and combined nozzle/rim-context crops. All runs use muted play settings; no listening acceptance.'
    aim_marker_checks = 'Isolated save loads aim_marker=false and remains byte-identical. Existing matrices disable markers. Actual four aimed weapons enter ready/recovering/empty through paid shots and strict cooldown, in all three palettes and four aim quadrants at 4P720; representative 4P1080/QHD. Marker fixture uses a silent bank while paying many shots without wall-clock audio updates. Same-state Level on/off raster changes lie only inside each owning pane marker bounds; all three shapes have distinct isolated native rasters. A mixed-angle/readiness roster checks independent owner panes. Repeated frozen draws, suspension, simulation state and replay fingerprint are checked. CPU acceptance separately verifies affordability boundaries, all lifecycle gates and gameplay equivalence.'
    blocked_shot_checks = 'Detached contacts in actual four-pane Level drawing; controlled opaque-cover/nozzle overlap and four outward normals at native 4P720 in every palette, plus representative 1080p/QHD. Initial/middle/expired, enabled/disabled pixels, projected cue and aggregate bounds, rebound/fade raster changes, repeated frozen redraws, cosmetic timers, simulation state and Debug replay fingerprint. CPU acceptance separately checks actual launch/flight contact classification and lifecycle.'
    player_identity_checks = 'Real same-team builder rosters with identical tools in all three 4P720 palettes; overlapping figures, actual scripted teammate crossing and sparse/reordered input slots. Quiet, requested emphasis and reduce-effects captures, representative 1080p/QHD extents, pause, dead/respawn and story suppression. Same-state zero-world-size comparison preserves pane HUD badges and constrains every changed pixel to the owning world badge bounds. Distinct shared P1-P4 native glyph rasters, native context and badge crops, frozen cosmetic state, repeated redraw and Debug replay fingerprint. CPU acceptance separately verifies lifecycle and gameplay equivalence; human reading acceptance remains separate.'
    monitor_checks = 'Bounded 75-capture matrix: six authored body shades, adult with mug/ring/crack/notes and child with stickers; manual picture-edge note overlap at 720p/1080p/QHD and one bounded glass push. Foreground notes survive picture and preview, picture/palette interiors stay unchanged, saving fills the empty slot and closing replaces the picture with the desktop. Isolated adult/child/mug seated, sigh, stand and partial-leave foreground rasters at all three native sizes; full departure equals blank, rim/outline/shading differ from plain fill, offset-window translation is exact. Native cursor hotspot stays fixed, detached initial/mid-click changes stay local and leave every resting arrow pixel unchanged, negative/expired ages and reduced effects equal rest. Native foreground and pointer detail crops supplement actual sparse-slot 2P/4P story frames, including before/at/after winning save and no-paint close clicks with same-state reduced-effects comparisons; solo practice has no monitor. Frozen rasters, persona/pose/public simulation state, draw RNG and Debug replay fingerprint remain unchanged. Captures establish no listening or physical-device acceptance.'
    checks = 'Real builder/Scene rosters, both movement facings and four aim quadrants at 720p; representative facings also at 1080p/QHD; three 4P palettes; all four same-team accents in both facings. Five body action clips on actual events, both frames and distinct pinned body rasters. All five tools at 1P720 in both facings: paid fire/hold/release/recovery, hose/bucket travel and roller travel/lift. Three 4P palettes: right-facing mixed aimed tools and left-facing roller, plus 1080p/QHD extent captures. Sampled non-idle pinned tools differ from idle at gameplay zoom, including released roller travel/lift. Frozen draws byte-equal, public simulation/action/tool motion state and Debug replay fingerprint unchanged. Isolated player/tool raster output stays inside projected cull bounds. Death, outline, fill, respawn, tool-free squash and whole-dead story poses; early elimination whole-dead raster equals later idle whole-dead raster. Combat on/off comparisons at identical simulation state: hurt and elimination across all three 4P palettes at 720p; controlled 4P hurt initial/recovery/expiry and elimination initial/middle/late/expiry at 720p/1080p/QHD. Combat timers/death snapshot stay unchanged during drawing; whole-dead story figures suppress the burst.'
    profile = $profile
    diagnostic = ''
}
if ($IdentityOnly) {
    $result.player_counts = @(2, 4)
    $result.weapons = @('sprayer', 'sniper')
    $result.actions = @()
    foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'monitor_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.player_identity_checks
}
elseif ($QuietOnly) {
    $result.weapons = @('sprayer', 'sniper', 'bucket')
    $result.actions = @()
    foreach ($key in 'contact_checks', 'weapon_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.quiet_checks
}
elseif ($MonitorOnly) {
    $result.player_counts = @(1, 2, 4)
    $result.palettes = @('blue_orange')
    $result.weapons = @('roller')
    $result.actions = @()
    foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.monitor_checks
}
try {
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
    $objectDirectory = Join-Path $gameDirectory 'CMakeFiles/ColourWars.dir'
    $objects = @(Get-ChildItem -LiteralPath $objectDirectory `
        -Filter '*.obj' -File -Recurse | Where-Object {
            if ($_.Name -eq 'main.cpp.obj') { return $false }
            if ($_.Name -eq 'cmake_pch.cxx.obj') { return $true }
            # CMake may leave an old object behind after its source is retired.
            $sourceRelative = $_.FullName.Substring($objectDirectory.Length + 1)
            $sourceRelative = $sourceRelative.Substring(0, $sourceRelative.Length - '.obj'.Length)
            Test-Path -LiteralPath (Join-Path (Join-Path $repo 'game') $sourceRelative) -PathType Leaf
        } | Sort-Object FullName)
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
    $source = Join-Path $PSScriptRoot 'acceptance/painter_visual.cpp'
    $executable = Join-Path $run 'painter_visual.exe'
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
        ('/Fo:' + (Quote-Path (Join-Path $run 'painter_visual.obj'))))
    $arguments += @($objects | ForEach-Object { Quote-Path $_.FullName })
    $arguments += @($libraries | ForEach-Object { Quote-Path $_ })
    $arguments += @('d3d11.lib', 'dxgi.lib', 'dxguid.lib', 'kernel32.lib', 'user32.lib',
        'gdi32.lib', 'winspool.lib', 'shell32.lib', 'ole32.lib', 'oleaut32.lib',
        'uuid.lib', 'comdlg32.lib', 'advapi32.lib')
    $arguments | Out-File -LiteralPath $responseFile -Encoding unicode
    Write-Host "Compile painter visual probe ($Preset)"
    Invoke-Logged -Command $cache.CMAKE_CXX_COMPILER -Arguments @("@$responseFile") `
        -LogPath (Join-Path $run 'compile.log')

    $result.staged_content_hashes = @(
        'manifest.json', 'tuning.json', 'presentation.json', 'levels/movement_course.json',
        'textures/painter.dds', 'textures/painter.json',
        'textures/weapon_paint.dds', 'textures/weapon_paint.json' |
        ForEach-Object {
            [ordered]@{ path = $_; sha256 = (Get-FileHash -LiteralPath (Join-Path $gameDirectory $_)).Hash }
        }
    )
    $env:PATH = "$gameDirectory;$previousPath"
    $env:LOCALAPPDATA = $profile
    $probeArguments = @((Quote-Path $gameDirectory), (Quote-Path $captures))
    if ($IdentityOnly) { $probeArguments += 'identity' }
    elseif ($QuietOnly) { $probeArguments += 'quiet' }
    elseif ($MonitorOnly) { $probeArguments += 'monitor' }
    $process = Start-Process -FilePath $executable -WindowStyle Hidden -PassThru `
        -WorkingDirectory $working -ArgumentList $probeArguments `
        -RedirectStandardOutput (Join-Path $run 'run.log') `
        -RedirectStandardError (Join-Path $run 'run.err')
    $null = $process.Handle
    $env:PATH = $previousPath
    $env:LOCALAPPDATA = $previousLocalAppData
    $timer = [Diagnostics.Stopwatch]::StartNew()
    while (-not $process.WaitForExit(1000)) {
        if ($timer.Elapsed.TotalSeconds -gt 600) { throw 'Visual probe exceeded its 600-second runtime limit.' }
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
    if (-not (Test-Path -LiteralPath $saveFixture -PathType Leaf) -or
        (Get-FileHash -LiteralPath $saveFixture).Hash -ne $saveFixtureHash) {
        throw 'The visual probe changed its isolated aim_marker=false save fixture.'
    }
    Write-ContactSheet -CaptureDirectory $captures
    $result.outcome = 'passed'
    $identityDiagnostic = 'Player identity passed all-palette same-team overlap, scripted crossing, sparse/reordered slots, native P1-P4 glyph distinction, quiet/emphasis/reduced effects, own-pane world-only bounds, dead/respawn/story suppression and frozen state. Human playing-size review remains separate.'
    if ($IdentityOnly) {
        $result.diagnostic = "Identity-only visual check passed. $identityDiagnostic Other painter matrices were not run."
    }
    elseif ($QuietOnly) {
        $result.diagnostic = 'Quiet-only visual check passed: sprayer quiet character matrix, sniper residual bead and bucket rim runoff; all 1P/4P720 palettes, continuous aim/flip attachment, representative 1080p/QHD, frozen redraw/state, raster bounds, per-tool enabled and reduced-effects suppression/restoration. Other painter matrices were not run.'
    }
    elseif ($MonitorOnly) {
        $result.diagnostic = 'Monitor-only visual check passed: six body shades, adult/mug and child/stickers; native 720p/1080p/QHD and pushed framing, foreground notes survive scene/preview, untinted picture/palette interiors, saved empty slot and closed desktop. Adult/child/mug seated/sigh/stand/leave art, full departure and offset-window translation passed; native pointer hotspot, local detached click ticks, expiry and reduced-effects suppression passed. Actual 2P/4P story beats and before/at/after winning-save and no-paint-close clicks, plus solo practice, passed frozen raster/persona/pose/state/fingerprint and draw RNG checks. Other painter matrices were not run.'
    }
    else {
        $result.diagnostic = "Painter base/action, five-tool mechanism, weapon paid-shot/aim-follow and controlled muzzle/empty/recovered on/off, step/skid contact, and actual idle drip/quiet character matrices passed. Aim readiness markers passed actual ready/recovering/empty, all palette/quadrant own-pane on/off bounds, isolated marker raster, frozen redraw and pause checks; the disabled isolated-save fixture remained unchanged. Local view shake preserved other panes, opaque HUD ink, base camera/aim, frozen redraw and simulation fingerprint, including suspension and resume. Distinct pinned body/tool and quiet on/off/blink rasters, frozen motion/feedback/quiet timers/redraw, separate player/drip raster culling bounds, elimination/redraw/respawn and idle whole-story poses passed. $identityDiagnostic"
    }
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
    Write-Host "Painter evidence: $run"
    Pop-Location
}
if ($null -ne $failure) { throw $failure }
