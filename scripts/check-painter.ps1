[CmdletBinding()]
param(
    [ValidateSet('x64-debug', 'x64-release')]
    [string] $Preset = 'x64-debug',
    [switch] $SkipBuild,
    [switch] $IdentityOnly,
    [switch] $QuietOnly,
    [switch] $MonitorOnly,
    [switch] $CountdownOnly,
    [switch] $ClosingOnly,
    [switch] $SupplyOnly,
    [switch] $HudOnly,
    [switch] $ResumeOnly,
    [switch] $EndMenuOnly,
    [switch] $PauseMenuOnly,
    [switch] $ResultsMenuOnly,
    [switch] $MainMenuOnly,
    [switch] $WeaponCatalogOnly,
    [switch] $MenuTextOnly,
    [switch] $MenuMotionOnly,
    [switch] $SetupMotionOnly,
    [switch] $JoinStageMotionOnly,
    [switch] $SetupPaperOnly,
    [switch] $SettingsMenuOnly,
    [switch] $ControlsMenuOnly,
    [switch] $StickSettingsOnly,
    [switch] $TeamSetupOnly,
    [string] $OutputDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (([int]$IdentityOnly.IsPresent + [int]$QuietOnly.IsPresent + [int]$MonitorOnly.IsPresent + [int]$CountdownOnly.IsPresent + [int]$ClosingOnly.IsPresent + [int]$SupplyOnly.IsPresent + [int]$HudOnly.IsPresent + [int]$ResumeOnly.IsPresent + [int]$EndMenuOnly.IsPresent + [int]$PauseMenuOnly.IsPresent + [int]$ResultsMenuOnly.IsPresent + [int]$MainMenuOnly.IsPresent + [int]$WeaponCatalogOnly.IsPresent + [int]$MenuTextOnly.IsPresent + [int]$MenuMotionOnly.IsPresent + [int]$SetupMotionOnly.IsPresent + [int]$JoinStageMotionOnly.IsPresent + [int]$SetupPaperOnly.IsPresent + [int]$SettingsMenuOnly.IsPresent + [int]$ControlsMenuOnly.IsPresent + [int]$StickSettingsOnly.IsPresent + [int]$TeamSetupOnly.IsPresent) -gt 1) {
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
            Where-Object { $_.Name -like '*-player*.ppm' -or $_.Name -like 'monitor-*.ppm' -or
                $_.Name -like 'countdown-*-detail.ppm' -or $_.Name -like 'closing-*.ppm' -or
                $_.Name -like 'supply-*-detail.ppm' -or $_.Name -like 'hud-*-detail.ppm' -or
                $_.Name -like 'resume-*-detail.ppm' -or $_.Name -like 'end-menu-*-detail.ppm' -or
                $_.Name -like 'pause-menu-*-detail.ppm' -or $_.Name -like 'results-menu-*-detail.ppm' -or
                $_.Name -like 'main-menu-*-detail.ppm' -or $_.Name -like 'settings-menu-*-detail.ppm' -or
                $_.Name -like 'controls-menu-*-detail.ppm' -or $_.Name -like 'stick-settings-*-detail.ppm' -or
                $_.Name -like 'team-setup-*-detail.ppm' -or $_.Name -like 'weapon-catalog-*-detail.ppm' -or
                $_.Name -like 'menu-text-*-detail.ppm' -or
                $_.Name -like 'menu-motion-*-detail.ppm' -or
                $_.Name -like 'setup-motion-*-detail.ppm' -or
                $_.Name -like 'join-stage-motion-*-detail.ppm' -or
                $_.Name -like 'setup-paper-*-detail.ppm' -or
                $_.Name -eq 'controls-menu-pad-sticks-1280x720.ppm' } | Sort-Object Name) {
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
            if ($file.BaseName -like 'monitor-*' -or $file.BaseName -like 'closing-story-*') {
                $monitorCellWidth = [Math]::Max($monitorCellWidth, $width + 16)
                $monitorCellHeight = [Math]::Max($monitorCellHeight, $height + 48)
            }
            else {
                $cellWidth = [Math]::Max($cellWidth, $width + 16)
                $cellHeight = [Math]::Max($cellHeight, $height + 48)
            }
        }
        if ($images.Count -eq 0) { throw 'No painter or monitor captures were produced.' }
        $painters = @($images | Where-Object { $_.name -notlike 'monitor-*' -and $_.name -notlike 'closing-story-*' })
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
        $countdowns = @($images | Where-Object { $_.name -like 'countdown-*' })
        Write-ContactSheetPages -Images $countdowns -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-countdown-contact'
        $closing = @($images | Where-Object { $_.name -like 'closing-*' -and $_.name -notlike 'closing-story-*' })
        Write-ContactSheetPages -Images $closing -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-closing-contact'
        $closingStories = @($images | Where-Object { $_.name -like 'closing-story-*' })
        Write-ContactSheetPages -Images $closingStories -CellWidth $monitorCellWidth -CellHeight $monitorCellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-closing-story-contact'
        $supply = @($images | Where-Object { $_.name -like 'supply-*' })
        Write-ContactSheetPages -Images $supply -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-supply-contact'
        $mainMenu = @($images | Where-Object { $_.name -like 'main-menu-*-1280x720-detail' })
        Write-ContactSheetPages -Images $mainMenu -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-main-menu-contact'
        $weaponCatalog = @($images | Where-Object { $_.name -like 'weapon-catalog-*-1280x720-detail' })
        Write-ContactSheetPages -Images $weaponCatalog -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-weapon-catalog-contact'
        $menuText = @($images | Where-Object { $_.name -like 'menu-text-*-1280x720-detail' })
        Write-ContactSheetPages -Images $menuText -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-menu-text-contact'
        $menuMotion = @($images | Where-Object { $_.name -like 'menu-motion-*-1280x720-detail' })
        Write-ContactSheetPages -Images $menuMotion -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-menu-motion-contact'
        $setupMotion = @($images | Where-Object { $_.name -like 'setup-motion-*-1280x720-detail' })
        Write-ContactSheetPages -Images $setupMotion -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-setup-motion-contact'
        $joinStageMotion = @($images | Where-Object { $_.name -like 'join-stage-motion-*-1280x720-detail' })
        Write-ContactSheetPages -Images $joinStageMotion -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-join-stage-motion-contact'
        $setupPaper = @($images | Where-Object { $_.name -like 'setup-paper-*-1280x720-detail' })
        Write-ContactSheetPages -Images $setupPaper -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-setup-paper-contact'
        $controlsMenu = @($images | Where-Object { $_.name -like 'controls-menu-*-1280x720-detail' })
        Write-ContactSheetPages -Images $controlsMenu -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-controls-menu-contact'
        $stickSettings = @($images | Where-Object { $_.name -like 'stick-settings-*-1280x720-detail' })
        Write-ContactSheetPages -Images $stickSettings -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-stick-settings-contact'
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
    scope = if ($IdentityOnly) { 'player-identity-only' } elseif ($QuietOnly) { 'quiet-only' } elseif ($MonitorOnly) { 'monitor-only' } elseif ($CountdownOnly) { 'countdown-only' } elseif ($ClosingOnly) { 'closing-only' } elseif ($SupplyOnly) { 'supply-only' } elseif ($HudOnly) { 'gameplay-hud-only' } elseif ($ResumeOnly) { 'resume-only' } elseif ($EndMenuOnly) { 'end-menu-only' } elseif ($PauseMenuOnly) { 'pause-menu-only' } elseif ($ResultsMenuOnly) { 'results-menu-only' } elseif ($MainMenuOnly) { 'main-menu-only' } elseif ($WeaponCatalogOnly) { 'weapon-catalog-only' } elseif ($MenuTextOnly) { 'menu-text-only' } elseif ($MenuMotionOnly) { 'menu-motion-only' } elseif ($SettingsMenuOnly) { 'settings-menu-only' } elseif ($ControlsMenuOnly) { 'controls-menu-only' } elseif ($StickSettingsOnly) { 'stick-settings-only' } elseif ($TeamSetupOnly) { 'team-setup-only' } else { 'full-painter-matrix' }
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
    hud_checks = 'Real InterfaceGameplay native full/half/empty health and paint, independently supplied quantities and negative/oversized clamps. Meter differences stay inside exact tracks; the half raster agrees with full/empty on either side of its midpoint and dark keylines remain outside the track. Dark text/pale paper raster contrast, sparse keyboard/pad-slot labels, 1P/2P/3P/4P720 and all three 1P/4P palettes; representative 4P1080/QHD, 4:3, portrait and ultrawide. Isolated owner-pane artwork remains clear of pane edges, including the unused three-player quadrant. Ordinary clock and static information match under Reduce effects, repeated drawing preserves pixels/draw RNG. Real 4P match frozen pixels, public state, Debug replay fingerprint and exact 720p/1080p resize round-trip. Existing final-ten clock boundary matrix rerun; reveal cards/story timing are outside the focused HUD run. Native empty/half/full detail PNGs and full-context PPMs retained. Raster checks establish no human readability, physical-device or listening acceptance.'
    contact_checks = 'Real resolved 4P dry steps/reversals; controlled step/skid initial/middle/expired rasters in all palettes at 720p/1080p/QHD. Dry/painted and left/right raster differences, enabled/disabled comparisons, frozen direction/tint/timers and culling bounds.'
    weapon_checks = 'Actual Weapon firing workflow with locally retained projectiles: 1P all five tools and 4P mixed tools in all three palettes, four aim quadrants, then live aim/flip changes while each paid-shot pulse remains active. Roller has no muzzle pulse. Controlled 4P muzzle/empty/recovered event triggers at initial/middle/expiry, enabled/disabled at the same simulation state, at 720p/1080p/QHD. Frozen redraw and observed feedback timers, current-nozzle attachment and raster culling bounds. Empty/recovery gameplay producers are covered by separate CPU acceptance checks. Mechanism raster comparisons suppress the new pulses.'
    quiet_checks = 'Actual builder/Scene 1P and 4P sprayers in all three palettes at 720p: settled breath crest, bead growth/stretch/release/fall, blink, movement interruption and left lean. Sniper residual beads and bucket rim runoff add growth/stretch/fall, same-bead continuous four-quadrant aim with authored lip/stripe anchors, both body facings, per-tool enabled suppression, and same-state reduced-effects hiding/restoration for attached and falling liquid in every 1P/4P720 palette. Representative attached-bead extents at 1080p/QHD. Mature bead raster touches the actual rendered tool silhouette within one pixel. Same-state enabled/disabled breathing/shift rasters and controlled blink-only comparison at playing size, distinct roster phases, frozen cosmetic timers/drops, separate Scene drip raster bounds and combined nozzle/rim-context crops. All runs use muted play settings; no listening acceptance.'
    aim_marker_checks = 'Isolated save loads aim_marker=false and remains byte-identical. Existing matrices disable markers. Actual four aimed weapons enter ready/recovering/empty through paid shots and strict cooldown, in all three palettes and four aim quadrants at 4P720; representative 4P1080/QHD. Marker fixture uses a silent bank while paying many shots without wall-clock audio updates. Same-state Level on/off raster changes lie only inside each owning pane marker bounds; all three shapes have distinct isolated native rasters. A mixed-angle/readiness roster checks independent owner panes. Repeated frozen draws, suspension, simulation state and replay fingerprint are checked. CPU acceptance separately verifies affordability boundaries, all lifecycle gates and gameplay equivalence.'
    blocked_shot_checks = 'Detached contacts in actual four-pane Level drawing; controlled opaque-cover/nozzle overlap and four outward normals at native 4P720 in every palette, plus representative 1080p/QHD. Initial/middle/expired, enabled/disabled pixels, projected cue and aggregate bounds, rebound/fade raster changes, repeated frozen redraws, cosmetic timers, simulation state and Debug replay fingerprint. CPU acceptance separately checks actual launch/flight contact classification and lifecycle.'
    player_identity_checks = 'Real same-team builder rosters with identical tools in all three 4P720 palettes; overlapping figures, actual scripted teammate crossing and sparse/reordered input slots. Quiet, requested emphasis and reduce-effects captures, representative 1080p/QHD extents, pause, dead/respawn and story suppression. Same-state zero-world-size comparison preserves pane HUD badges and constrains every changed pixel to the owning world badge bounds. Distinct shared P1-P4 native glyph rasters, native context and badge crops, frozen cosmetic state, repeated redraw and Debug replay fingerprint. CPU acceptance separately verifies lifecycle and gameplay equivalence; human reading acceptance remains separate.'
    monitor_checks = 'Bounded 75-capture matrix: six authored body shades, adult with mug/ring/crack/notes and child with stickers; manual picture-edge note overlap at 720p/1080p/QHD and one bounded glass push. Foreground notes survive picture and preview, picture/palette interiors stay unchanged, saving fills the empty slot and closing replaces the picture with the desktop. Isolated adult/child/mug seated, sigh, stand and partial-leave foreground rasters at all three native sizes; full departure equals blank, rim/outline/shading differ from plain fill, offset-window translation is exact. Native cursor hotspot stays fixed, detached initial/mid-click changes stay local and leave every resting arrow pixel unchanged, negative/expired ages and reduced effects equal rest. Native foreground and pointer detail crops supplement actual sparse-slot 2P/4P story frames, including before/at/after winning save and no-paint close clicks with same-state reduced-effects comparisons; solo practice has no monitor. Frozen rasters, persona/pose/public simulation state, draw RNG and Debug replay fingerprint remain unchanged. Captures establish no listening or physical-device acceptance.'
    countdown_checks = 'Isolated 0-9/GO glyphs differ at native 720p, with representative 1080p/QHD. Animated and reduced/negative/expired ages stay inside the supplied number box; reduced effects retain the settled readable signal. Actual keyboard solo and mixed keyboard/sparse-pad 2P/4P full/quick/off openings cover native 720p/1080p/QHD, reminder-card bounds, frozen initial/beat/settled/pre-GO/GO/tail/expiry captures, paused and nonpositive-time draws, exact GO control-release/card-clear boundary and resize round-trip. All four authored stage names appear on fitted shared paper headers; a three-player roster and 4:3, portrait and ultrawide resize round-trips cover stage/title bounds and separation from countdown/device cards. Header information remains static with Reduce effects, hides during early story opening and at GO, and survives skips from both opening and monitor countdown. Same-state GO art on/off differences remain inside the former card, persist through its tail and disappear after one second. Frozen rasters, public simulation/cosmetic state, draw RNG and Debug replay fingerprint remain unchanged. Native stage-header, card and glyph crops accompany full-context PPMs. This is muted hardware rendering, not human readability acceptance.'
    closing_checks = 'Actual HUD last-ten-second clock at 1P/2P/4P720 and 4P1080/QHD: initial/early beat, settled, pre-closing and zero states, local normal/reduced-effects raster comparisons and frozen redraw. Same-state paper suppression checks the entire settled card gate in every pane; short ten/one-second clocks show neither card nor ticks until live play. Native settled/click A/B coverage cards in all three palettes at 720p, representative 1080p/QHD, negative/expired/reduced-effects equivalence, supplied bounds and draw RNG. Actual sparse-pad 2P A/B winners, 4P no-paint draw and solo finish, before/at/after click card gating, repeated frozen pixels, public state and Debug replay fingerprint. Native card/clock detail PNGs and full story contexts are preserved. Muted rendering establishes no listening, physical-device or human readability acceptance.'
    supply_checks = 'Sprayer and mister full/half/empty reservoirs at actual builder-owned camera scales and layouts: all three 1P/4P720 palettes, four aim quadrants, both facing inputs, all four poses, representative 4P1080/QHD and motion-disabled tools. Independent authored full/half-layer composition must match actual Weapon pixels exactly; half pixels stay within full supply and filling introduces no new silhouette pixels or changes to permanent hardware/nozzle paint. Negative/oversized ammo clamps, unchanged reduced-effects information, frozen pixels/weapon state/draw RNG, paid full-shot depletion, natural regeneration/reset and unchanged match observation/Debug fingerprint. Menu previews retain full supply and switching tools clears it. Native detail PNGs and full-context PPMs are preserved; human readability and physical-device acceptance remain separate.'
    checks = 'Real builder/Scene rosters, both movement facings and four aim quadrants at 720p; representative facings also at 1080p/QHD; three 4P palettes; all four same-team accents in both facings. Five body action clips on actual events, both frames and distinct pinned body rasters. All five tools at 1P720 in both facings: paid fire/hold/release/recovery, hose/bucket travel and roller travel/lift. Three 4P palettes: right-facing mixed aimed tools and left-facing roller, plus 1080p/QHD extent captures. Sampled non-idle pinned tools differ from idle at gameplay zoom, including released roller travel/lift. Frozen draws byte-equal, public simulation/action/tool motion state and Debug replay fingerprint unchanged. Isolated player/tool raster output stays inside projected cull bounds. Death, outline, fill, respawn, tool-free squash and whole-dead story poses; early elimination whole-dead raster equals later idle whole-dead raster. Combat on/off comparisons at identical simulation state: hurt and elimination across all three 4P palettes at 720p; controlled 4P hurt initial/recovery/expiry and elimination initial/middle/late/expiry at 720p/1080p/QHD. Combat timers/death snapshot stay unchanged during drawing; whole-dead story figures suppress the burst.'
    profile = $profile
    diagnostic = ''
}
if ($IdentityOnly) {
    $result.player_counts = @(2, 4)
    $result.weapons = @('sprayer', 'sniper')
    $result.actions = @()
    foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'monitor_checks', 'countdown_checks', 'closing_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.player_identity_checks
}
elseif ($QuietOnly) {
    $result.weapons = @('sprayer', 'sniper', 'bucket')
    $result.actions = @()
    foreach ($key in 'contact_checks', 'weapon_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks', 'countdown_checks', 'closing_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.quiet_checks
}
elseif ($MonitorOnly) {
    $result.player_counts = @(1, 2, 4)
    $result.palettes = @('blue_orange')
    $result.weapons = @('roller')
    $result.actions = @()
    foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'countdown_checks', 'closing_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.monitor_checks
}
elseif ($CountdownOnly) {
    $result.player_counts = @(1, 2, 4)
    $result.palettes = @('blue_orange')
    $result.weapons = @('roller')
    $result.actions = @()
    $result.input_slots = @(4, 3, 1, 0)
    $result.inputs = 'Scripted keyboard/mouse seat and sparse pad-slot inputs; no physical-device claim.'
    foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks', 'closing_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.countdown_checks
}
elseif ($ClosingOnly) {
    $result.player_counts = @(1, 2, 4)
    $result.weapons = @('roller')
    $result.actions = @()
    foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks', 'countdown_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.closing_checks
}
elseif ($SupplyOnly) {
    $result.weapons = @('sprayer', 'mister')
    $result.actions = @()
    foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks', 'countdown_checks', 'closing_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.supply_checks
}
elseif ($TeamSetupOnly) {
    $result.player_counts = @(2, 3, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('sprayer', 'sniper', 'roller', 'bucket')
    $result.actions = @('shuffle', 'concurrent-confirm', 'reconfirm', 'weapon-return', 'manual-team-edit', 'held-entry', 'conflicting-actions', 'disconnect', 'focus-return', 'solo-no-op', 'warning-clear', 'pause-restart', 'series-next-round')
    $result.input_slots = @(4, 3, 0, 2)
    $result.inputs = 'Real MainMenuTeamSelect/MainMenuWeaponSelect, StateContext and MenuInput; test-only concrete GamepadReader backend supplies connected states and armed button releases. Public keyboard messages supply R/Enter/Escape. No physical-controller or native-foreground acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Actual 2/3/4-player seeded balanced shuffles, both three-player majority sides, requests from every joined seat, concurrent confirm discarded, all readiness cleared and every player required to reconfirm. Provisional teams match a fresh actual page raster and independently specified complete X/R legend; final confirmation commits just teams. Weapon return preserves teams, tools, sparse slots, stage, duration and best-of; manual edits, held entry, conflicting actions, missing pad, reconnect-held X and focus-return-held R are gated. Shuffle clears the empty-team warning; solo return hides Random teams and ignores R. Native 720p/1080p rows/footer captures and measured four-row/legend bounds, frozen page raster. One committed shuffled 2P roster retains actual Player teams, input slots and weapons through Pause Restart and two best-of-three rounds using ordinary Results/End continuation, with the ongoing-series primary card checked against its expected ledger. Physical pads, human readability and listening acceptance remain separate.'
}
elseif ($StickSettingsOnly) {
    $result.player_counts = @()
    $result.palettes = @('blue_yellow')
    $result.weapons = @()
    $result.actions = @('slot-selection', 'deadzone-clamps', 'apply', 'failed-save', 'back-discard', 'parent-apply')
    $result.input_slots = @(0, 1, 2, 3, 4)
    $result.inputs = 'Public keyboard through production Pad sticks, Controls/audio and StateContext, with a separate isolated save profile. Preview reads the current pad snapshot; no physical controller is required.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Actual Pad sticks child at 720p/1080p/QHD: five bounded focus cards/arrows, full labels/values/status and response/association copy, native linear ink and contrast, initial and pausing slot selection, slot/deadzone clamps and pending cutoff previews, Apply/reload/failure and Back/Escape discard. Suspended parent Apply preserves child profiles. Frozen redraw, draw RNG, static Reduce effects and resize. No physical-controller comfort, human readability or listening acceptance.'
}
elseif ($ControlsMenuOnly) {
    $result.player_counts = @(1)
    $result.palettes = @('blue_yellow')
    $result.weapons = @('sprayer')
    $result.actions = @('row-change', 'numeric-clamps', 'apply', 'failed-save', 'back-discard', 'options-return', 'pause-return', 'custom-labels', 'long-label-fit', 'controls-font-snapshot', 'focus-return', 'custom-help', 'multiline-help-fit', 'help-font-snapshot', 'preview-refresh', 'custom-status', 'status-font-snapshot', 'save-recovery', 'interrupted-apply')
    $result.actions += @('custom-setting-values', 'value-width-height-fit', 'value-font-snapshot', 'value-child-return')
    $result.input_slots = @(4)
    $result.inputs = 'Public keyboard through production Controls/audio, Options, Pause and actual GameLevel/StateContext, with a separate isolated save profile. Preview gain is zero for this raster check; saved music/SFX remain muted.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Controls/audio paper, all eight focus cards and detached arrows; measured full labels/values, bounded complete controller/keyboard/readiness/effects help and save status, native linear text and ink/paper contrast across 720p/1080p/QHD, 4:3, 16:10 and ultrawide. Actual clamps and pending edits, Apply with persisted reload, controlled save failure, Back discard, pending Options values/focus and actual paused GameLevel return. Actual Pad sticks child at 720p/1080p/QHD: five focus cards, full labels/values/status and response/association copy, initial and pausing slot selection, slot/deadzone clamps and pending cutoff previews, Apply/reload/failure and Back/Escape discard. Thirteen independent historical label substitutions across all six sizes/aspects; long OptionRow fitting, full label/value focus-card bounds, a separate Controls item font missing an old glyph, retained/reopened/reversed text/font snapshots, repeated child return and Apply, interrupted Enter requiring a fresh selection, save reload and pending-edit discard. Three independent static-help defaults; authored Controls and association blocks exercise both width and multiline height fitting. Unscaled aim caption uses the Move height probe and the full help column beyond its diagram width. A replacement help font missing an old glyph, retained/reopened/reversed help snapshots, value/preview refresh, old-parent/new-child return, repeated/interrupted Apply and discard pass at 720p. Independent pending/saved/failure status rasters pass across six sizes/aspects on Controls and Pad Sticks. Authored width fitting with compact help, scale reset to short saved copy, retained/reopened/reversed status callbacks using a font missing an old glyph, interrupted/repeated Apply, controlled failure/recovery, isolated reload, both discard routes and old-parent/new-child profile merge pass at 720p. Suspended parent Apply preserves child profiles. Frozen redraw/state, draw RNG, static Reduce effects and resize. No physical-controller, human readability, preview audition or listening acceptance.'
    $result.checks += ' Independent Controls On/Off defaults pass across six sizes/aspects. A Controls item-font fixture exercises width and tall-glyph height fitting, complete value/focus-card unions, refitting between width/height limits, reopened short copy and replacement without an old glyph. Retained/reopened/reversed callbacks, Left/Right boolean assignments, Pad sticks child return/profile merge, interrupted/repeated Apply, controlled failure/recovery, isolated reload and discard pass at 720p.'
}
elseif ($SettingsMenuOnly) {
    $result.player_counts = @()
    $result.palettes = @('blue_orange', 'pink_green', 'blue_yellow', 'random')
    $result.weapons = @()
    $result.actions = @('row-change', 'apply', 'failed-save', 'controls-return', 'discard', 'disabled-resolution', 'credits-back', 'custom-status', 'status-font-snapshot', 'save-recovery', 'interrupted-apply')
    $result.actions += @('custom-setting-values', 'value-width-height-fit', 'value-font-snapshot', 'value-child-return')
    $result.input_slots = @(4)
    $result.inputs = 'Public keyboard through production Options/Credits and StateContext, with a second isolated save profile; fullscreen metadata exercises disabled rows. Only false/Off is applied using an isolated offscreen, windowed fixture, which is immediately hidden again.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Options/Credits paper, all enabled focus cards/arrows, native ink/paper contrast, measured labels, all palette values, long failure status and complete authored credits at 720p/1080p/QHD, 4:3, 16:10 and ultrawide. Actual row edits, staged Apply, persisted reload and failed-write warning, Controls/audio return preserving pending edits/focus, Back discard, disabled fullscreen resolution and retained windowed preset, Credits Select/Back, frozen redraw, draw RNG, static Reduce effects and resize. Independent Options pending/saved/failure rasters pass across six sizes/aspects; authored width fitting and scale reset, missing-old-glyph status font, retained/reopened/reversed callbacks, interrupted/repeated Apply, controlled failure/recovery, isolated reload and both discard routes pass at 720p. No physical-controller, human readability or listening acceptance.'
    $result.checks += ' Independent Options On/Off/Random defaults pass across six sizes/aspects. An Options item-font fixture exercises width and tall-glyph height fitting, complete value/focus-card unions, Random-to-short-fixed-palette scale reset and replacement without an old glyph. Retained/reopened/reversed callbacks, both fullscreen toggle directions and Random palette cycles, one Controls child On/Off pair and return, interrupted/repeated windowed-Off Apply, controlled failure/recovery, isolated reload and discard pass at 720p.'
}
elseif ($SetupPaperOnly) {
    $result.scope = 'setup-paper-only'
    $result.player_counts = @(1, 2, 3, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @()
    $result.actions = @('all-mode-focus', 'all-count-focus', 'disabled-solo', 'duration-cycle', 'mode-choice', 'count-join', 'back-routes', 'custom-copy-font-snapshots', 'captured-effective-art', 'invalid-duration-union', 'focus-motion', 'duration-press', 'rapid-navigation', 'reduce-effects', 'deactivation', 'covering-page-return', 'developer-reset', 'resize')
    $result.input_slots = @(4)
    $result.inputs = 'Local keyboard messages through actual Mode Select, Player Count, Join and Home transitions in StateContext; normal GamepadReader backend, hidden no-activate window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Independent complete paper/card/arrow and transparent-shadow text rasters for every Mode/Count focus and disabled-solo series at 720p/1080p/QHD, 4:3, 16:10 and ultrawide, with measured card/text containment and native ink/paper contrast. All duration cycles, three modes, one-to-four-player count/layout/keyboard claims and ordinary Back/Join/Home routes retain behavior; authored text/font snapshots survive replacement, reopen, reversal, duration and child return. Effective paper art, compact focus padding and shadow remain captured through source replacement, focus callbacks, reopening and reversal. An actual Mode page rejects an overflowing compound duration value with its specific diagnostic. Native Mode/Count arrow initial/middle/rest and duration paper press/release, local envelopes, fixed opaque label/value pixels, rapid navigation, nonpositive time, immediate Reduce effects, activation/cover/developer reset, frozen redraw/RNG and resize round-trip. No physical-pad, human readability, listening or performance acceptance.'
}
elseif ($JoinStageMotionOnly) {
    $result.scope = 'join-stage-motion-only'
    $result.stage = @('movement_course', 'close_quarters', 'random')
    $result.player_counts = @(4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('bucket', 'sprayer')
    $result.actions = @('join', 'release', 'stable-hole', 'simultaneous-claims', 'same-update-replacement', 'rapid-row-restart', 'duplicate-full-refusal', 'final-claim-barrier', 'missing-p1-rescue', 'browse', 'real-random-wrap', 'lock', 'unlock', 'locked-arrow-refusal', 'final-random-start', 'rapid-final-start', 'pending-fade', 'single-excluded-stage', 'reduce-effects', 'interruption-resets', 'held-reconnect', 'resize')
    $result.input_slots = @(4, 3, 0, 2, 1)
    $result.inputs = 'Real Join Players and Arena Stage Select pages in StateContext. Local keyboard messages and the existing scripted GamepadReader link seam provide sparse seats, claims, disconnects and held reconnects. Hidden no-activate window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Native initial/middle/rest Join gutter acknowledgements and Stage briefing stripe at 720p/1080p/QHD; each changed pixel stays inside independent row-gutter or briefing-padding envelopes, preserving all text, previews, camera, edges and other rows. Stable release holes, simultaneous claims, same-update release/replacement, rapid row restart, duplicate/full refusal, final-claim snapshot barrier and independent missing-P1 Back destination; real/Random wrapping, lock/unlock, refused locked arrows, no eligible Random and deliberate settled/rapid final eligible Random commits with exactly one RNG choice, settled outgoing Random raster and gated pending fade through focus interruption and repeated input. Immediate applied Reduce effects, nonpositive time, activation/covering-page/developer-input resets, missing/reconnect rearming, frozen redraw/plan/RNG and resize round-trip. Scripted reader only; no physical-controller, human readability, listening or performance acceptance.'
}
elseif ($SetupMotionOnly) {
    $result.scope = 'setup-motion-only'
    $result.player_counts = @(4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('sprayer', 'sniper', 'roller', 'mister', 'bucket', 'random')
    $result.actions = @('changed-focus', 'duration-press', 'independent-team-choice', 'independent-weapon-choice', 'ready', 'unready', 'shuffle', 'rejected-input', 'rapid-navigation', 'reduce-effects', 'deactivation', 'missing-reconnect', 'covering-page-return', 'resize')
    $result.input_slots = @(4, 3, 0, 2)
    $result.inputs = 'Real Mode Select, Player Count, Team Select, Weapon Select and StateContext. Public local keyboard messages and the existing scripted GamepadReader link seam supply sparse connected seats and armed button releases. Hidden no-activate window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Moving/middle/rest native setup ornaments at 720p/1080p/QHD with independent per-row pixel envelopes and fixed weapon artwork, masks, names, descriptions and player badges. Four-seat setup pages, initial settled pages, simultaneous and rapid choices, readiness/unreadiness, shuffle, refused confirmation, endpoint and confirmed-row input, immediate Count-to-2P Join and final Team/Weapon transitions, applied Reduce effects suppression, activation, missing/reconnect and test-pushed covering-page return, frozen redraw/RNG and resize. No physical-controller, human readability, listening or performance acceptance.'
}
elseif ($MenuMotionOnly) {
    $result.player_counts = @()
    $result.palettes = @()
    $result.weapons = @()
    $result.actions = @('changed-focus', 'adjust-value', 'apply', 'rapid-navigation', 'reduce-effects', 'deactivation', 'child-return', 'resize')
    $result.input_slots = @(4)
    $result.inputs = 'Public local keyboard messages through actual MainMenuHome, ControlsSettingsMenu, StickSettingsMenu and StateContext; normal GamepadReader backend. Hidden no-activate window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Initial focus stays still. Actual changed-focus arrows and accepted OptionRow paper presses have distinct initial/middle/rest native rasters at 720p/1080p/QHD; successful Apply has distinct pressed/rest rasters; all changed pixels stay inside independent ornament envelopes and label/value lettering remains fixed. Nonpositive time, rapid focus, immediate applied reduced-effects drawing, real Controls Apply/reload/reversal, deactivation, nested child return, frozen redraw/RNG and resize round-trip. No physical-pad, human readability, listening or performance acceptance.'
}
elseif ($MenuTextOnly) {
    $result.player_counts = @(1, 2, 3, 4)
    $result.palettes = @()
    $result.weapons = @()
    $result.actions = @('named-prompt-rebuild', 'arena-browse', 'arena-lock', 'arena-unlock', 'random-stage-copy', 'random-stage-width-fit', 'random-stage-eligible-start', 'home-options-credits-items', 'fitted-options-labels', 'mode-count-items', 'duration-cycle', 'mode-choice', 'count-join-back', 'solo-disabled', 'focus-round-trip', 'credits-select-home', 'reopen', 'presentation-replacement', 'title-home-fixed-copy', 'title-start-card', 'interrupted-title-home-enter')
    $result.input_slots = @(4, 0)
    $result.inputs = 'Public keyboard messages through actual Title, Home, Stage Select, Options, Credits, Mode Select, Player Count and Join Players plus a concrete MenuPage fixture. This focused binary uses the existing scripted GamepadReader link seam; pads remain neutral until the briefing fixture supplies one connected second seat. No physical-controller or native-foreground acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Independent shipped brand, all 11 static headings and all 17 formatted legends compared through real MenuPage construction/prompt rebuild against native text/chrome fixtures at 720p/1080p/QHD. Ten legacy static item roles compared on actual Home/Options/Credits at those sizes; long Options labels fit before their value columns, focus round trips and Credits Select retain routing. Eight Mode/Player Count labels and the solo hint match independent complete page rasters across six sizes/aspects, including duration-column placement and the existing item-shadow hint. Actual duration cycles/wrap/confirm, all three mode choices, disabled solo series, all 1-4-player count/layout/keyboard claims and Join/Mode/Home Back routes. Reachable Random-stage display uses an eligible real arena alongside excluded Movement Course; independent full text/shadow crops at six sizes, real/Random cycling without RNG or plan commits, long width-only fit/short reset, retained/reopened/reversed copy/item font and live width/negative-shadow fitting on refresh. Lock/unlock, interrupted Enter and deliberate two-release Random start preserve eligibility and the pending music-exit plan. Title wordmark/subtitle/Start and Home subtitle retain independent linear rasters and complete Start-card bounds across source assignments, reopen and reversal with separate title/item/detail fonts missing an old glyph. Repeated Title/Home/Back and Mode return, Home focus movement and interrupted Enter rearming are checked at 720p; the connection hint remains hidden under the existing always-connected keyboard policy, with its full independently measured absence crop. Custom brand/heading/prompt/item/hint/font/layout, old-page snapshots, repeated prompt rebuilds, duration edits, focus and child return, reopen adoption, frozen redraw and draw RNG. Native crops and CSV retained. No visible controller-loss, physical-controller, human readability, performance or listening acceptance.'
    $result.actions += @('arena-status-copy', 'arena-status-captured-geometry')
    $result.checks += ' Both fixed arena status strings have independent complete text/shadow crops on real/Random browse/locked pages across six sizes. Unscaled detail-font status stays inside the full briefing before its hint row. Forced cycling/lock/unlock after source replacement, reopen and reversal retain captured text/font/position/shadow offset/shadow colour while refresh uses live ink; short custom copy was measured in the shipped font first. Provisional plan and RNG remain unchanged.'
    $result.actions += @('fixed-stage-briefing', 'connected-scripted-versus-briefing', 'briefing-captured-ink')
    $result.checks += ' Four fixed stage-briefing strings and independently specified adjacent mode/roster have complete text/shadow crops at six sizes for solo and connected scripted 2P single/best-of-three/best-of-five. Unscaled brand-font copy fits the design and briefing vertical span before authored following rows; the full solo objective overhang is retained. Captured copy/brand font/position/shadow offset/shadow colour/ink survive source replacement/reopen/reversal and cycling/lock/unlock, with unchanged provisional plan and RNG. The normal-reader full matrix runs the solo briefing subset and explicitly reports the scripted 2P contexts skipped; ResumeOnly retains the real backend.'
    $result.actions += @('fixed-random-navigation', 'fixed-stage-hints', 'hint-width-offset-refresh')
    $result.checks += ' Fixed Random navigation and browse/locked hints have independent full glyph/shadow crops across real/Random browse/locked states at six sizes, alongside unchanged numbered arenas and the eligible Random-count branch. Navigation remains unscaled with constructor-owned detail font/position/ink/shadow. Hints retain captured copy/brand font/position/ink/shadow colour while width budget and signed detail-shadow offset refresh on slot/state changes; long hints, short reset, separate old-glyph-missing fonts, cached fit, reopen/reversal and locked-arrow refusal are covered without changing the plan or RNG.'
    $result.actions += @('fixed-stage-mode-labels', 'stage-mode-detail-font-snapshot')
    $result.checks += ' The two fixed mode labels use independent fixture words for solo/single-match cases, while BEST OF 3/5 remain independent composed-text literals. Native mode crops remain unscaled in the actual detail font before roster row two across six sizes. Custom old-glyph copy, a separate detail font missing that glyph, constructor-owned position/shadow/ink and source replacement/reopen/reversal through cycling/lock/unlock preserve the connected roster, plan and RNG.'
}
elseif ($WeaponCatalogOnly) {
    $result.player_counts = @(1)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('sprayer', 'sniper', 'roller', 'mister', 'bucket', 'random')
    $result.actions = @('browse', 'wrap-left', 'wrap-right', 'confirm-release', 'stage-back', 'mode-back', 'reopen')
    $result.input_slots = @(4)
    $result.inputs = 'Public keyboard input through actual Weapon/Stage/Mode Select pages on the normal GamepadReader backend.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Six historical default names and icon/body/paint/full-supply bindings compared with independent fixtures at 720p/1080p/QHD, both wraps, provisional settings, fresh release confirmation, seeded concrete Random primary, actual Stage/Mode Back preserving all roster and match settings. Edited name/body/paint/supply and a sparse alternate atlas/fill become visible on reopen; existing pages keep their snapshots across edits, reversals and cycling. Frozen raster and draw RNG, native detail/context images and CSV retained. No physical-controller, human readability, performance or listening acceptance.'
}
elseif ($MainMenuOnly) {
    $result.player_counts = @()
    $result.palettes = @()
    $result.weapons = @()
    $result.actions = @('title-start', 'home-play', 'home-options', 'home-credits', 'home-exit', 'back')
    $result.input_slots = @(4)
    $result.inputs = 'Public keyboard input through actual MainMenuTitle/MainMenuHome and StateContext; Exit closes only this probe hidden window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Title/home paper, measured centered title text, heading/subtitle/build bounds, native ink/paper contrast and all four Home focus cards/arrows at 720p/1080p/QHD, 4:3, 16:10 and ultrawide. Independent historical multiline wordmark, Title subtitle/Start and Home subtitle rasters plus complete Start-card bounds use native linear sampling at all six sizes. The connection hint retains the existing always-connected keyboard hidden policy; its complete independently measured extent matches paper, without a visible controller-loss claim. Actual keyboard Start/Back/Play/Options/Credits/Exit, frozen redraw, draw RNG, static Reduce effects and resize round-trip. No physical-controller, human readability or listening acceptance.'
}
elseif ($ResultsMenuOnly) {
    $result.player_counts = @(1, 2, 4)
    $result.palettes = @('blue_orange', 'pink_green', 'blue_yellow')
    $result.weapons = @()
    $result.actions = @('fill', 'revealed', 'solo', 'draw', 'series', 'contributions', 'continue')
    $result.input_slots = @(4, 3, 0, 2)
    $result.inputs = 'Public keyboard input through the actual ResultsMenuInitial and StateContext; sparse pad slots are roster fixtures.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Results paper, centered lettering and score cards at native 720p/1080p/QHD and differing aspects; competitive/solo score bars and percentage rendering, absent teams, sparse-slot contributions, revealed and fill timing gates, draws and series. Frozen redraw, static Reduce effects, draw RNG, resize and deliberate continuation. No human readability or listening acceptance.'
}
elseif ($PauseMenuOnly) {
    $result.player_counts = @(1, 2, 4)
    $result.palettes = @()
    $result.weapons = @()
    $result.actions = @('resume', 'restart', 'controls-audio', 'quit', 'confirm-yes', 'confirm-no', 'missing-controllers', 'custom-labels', 'custom-questions', 'font-snapshot', 'focus-return', 'missing-seat-gate-clear', 'restart-missing-seat-cancel')
    $result.input_slots = @(4, 0, 1, 2, 3)
    $result.inputs = 'Public keyboard input through actual pause/confirmation pages and StateContext; absent pad slots are roster fixtures, with no physical pad claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Pause and confirmation paper plus six independent legacy label and two independent legacy question rasters at native 720p/1080p/QHD, 4:3, 16:10 and ultrawide; actual initial Resume, safe No, deliberate Restart/Quit and Controls/audio return. Questions use the unscaled actual detail font and full-paper centering; their complete measured bounds are compared over the stepped paper with production point filtering. Custom labels/questions and replacement item/detail fonts missing an old glyph; retained/reopened/reversed snapshots through focus movement, Controls return, No/Back and repeated typed actions. Focus interruption and synthetic missing-seat Restart cancellation/gate clearing require fresh input, alongside missing-participant refusal, enabled navigation, all-absent keyboard rescue, frozen redraw/RNG, Reduce effects and resize. No physical reconnection, human readability or listening acceptance.'
}
elseif ($EndMenuOnly) {
    $result.player_counts = @()
    $result.palettes = @()
    $result.weapons = @()
    $result.actions = @('rematch', 'change-teams', 'change-weapons', 'change-stage', 'exit', 'next-round', 'replay-draw', 'play-series-again', 'custom-labels', 'custom-headings', 'font-snapshot', 'pending-music-exit', 'focus-return')
    $result.input_slots = @(4)
    $result.inputs = 'Public keyboard input through the actual EndMenuInitial and StateContext; no physical pad claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'End-menu paper, primary continuation, four independent legacy action labels and three independent legacy headings at native 720p/1080p/QHD, 4:3, 16:10 and ultrawide; single match, tied/continuing/completed series, both Exit rows and hidden setup choices. Complete measured heading bounds use the unscaled actual heading font, full-paper centering and a stepped-paper reference with production point filtering. Custom labels/headings and replacement item/heading fonts missing an old glyph; retained/reopened/reversed snapshots through focus movement and repeated typed actions preserve the Session ledger. Owner-controlled pending music exits reject further input and preserve their original action through focus interruption; interrupted Enter releases require fresh selection. Frozen redraw/RNG, Reduce effects and resize remain covered. No physical controllers, human readability or listening acceptance.'
}
elseif ($ResumeOnly) {
    $result.player_counts = @(1, 2, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('sprayer')
    $result.actions = @('pause', 'resume', 'jump', 'fire', 'drop', 'death', 'focus', 'restart', 'quit')
    $result.input_slots = @(4, -1)
    $result.inputs = 'Public keyboard/mouse message input through actual GameLevel, StateContext and pause pages; other seats are idle dummies, with idle bots in Debug. No physical pad claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Actual 1P/2P/4P pause-page Resume freezes the world through count-in, nonpositive time, re-pause, focus loss/return and a long GO-crossing update. Public player health/ammo/cooldown/dead respawn, live projectile count, detailed cosmetic observation and underlying world/HUD raster remain fixed. Held jump/fire/drop remain suppressed until neutral and a deliberate new request; fresh pause interrupts the count. Actual keyboard-driven Restart clears the gate and Quit leaves immediately. Shared RESUMING paper is bounded and frozen at native 720p/1080p/QHD with exact resize round-trip. Muted hardware rendering; no human readability or listening acceptance.'
}
elseif ($HudOnly) {
    $result.player_counts = @(1, 2, 3, 4)
    $result.weapons = @('sprayer', 'sniper', 'mister', 'bucket')
    $result.actions = @()
    $result.input_slots = @(4, 3, 1, 0)
    $result.inputs = 'Direct HUD quantities and sparse keyboard/pad-slot labels; one real four-player builder-owned frozen match. No physical-device claim.'
    foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks', 'countdown_checks', 'closing_checks', 'supply_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.hud_checks
}
if (-not $HudOnly -and ($IdentityOnly -or $QuietOnly -or $MonitorOnly -or $CountdownOnly -or $ClosingOnly -or $SupplyOnly)) {
    $result.Remove('hud_checks')
}
if (-not $SupplyOnly -and ($IdentityOnly -or $QuietOnly -or $MonitorOnly -or $CountdownOnly -or $ClosingOnly -or $HudOnly)) {
    $result.Remove('supply_checks')
}
if ($result.Contains('countdown_checks')) {
    $result.Remove('stage')
    $result.stages = @('movement_course', 'close_quarters', 'king_of_the_hill', 'turbulence')
    $result.player_counts = @(1, 2, 3, 4)
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
    if ($TeamSetupOnly -or $MenuTextOnly -or $SetupMotionOnly -or $JoinStageMotionOnly) { $arguments += '/DCOLOURWARS_PAINTER_SCRIPTED_PADS' }
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
        'levels/close_quarters.json', 'levels/king_of_the_hill.json', 'levels/turbulence.json',
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
    elseif ($CountdownOnly) { $probeArguments += 'countdown' }
    elseif ($ClosingOnly) { $probeArguments += 'closing' }
    elseif ($SupplyOnly) { $probeArguments += 'supply' }
    elseif ($HudOnly) { $probeArguments += 'hud' }
    elseif ($ResumeOnly) { $probeArguments += 'resume' }
    elseif ($EndMenuOnly) { $probeArguments += 'end-menu' }
    elseif ($PauseMenuOnly) { $probeArguments += 'pause-menu' }
    elseif ($ResultsMenuOnly) { $probeArguments += 'results-menu' }
    elseif ($MainMenuOnly) { $probeArguments += 'main-menu' }
    elseif ($WeaponCatalogOnly) { $probeArguments += 'weapon-catalog' }
    elseif ($MenuTextOnly) { $probeArguments += 'menu-text' }
    elseif ($MenuMotionOnly) { $probeArguments += 'menu-motion' }
    elseif ($SetupMotionOnly) { $probeArguments += 'setup-motion' }
    elseif ($JoinStageMotionOnly) { $probeArguments += 'join-stage-motion' }
    elseif ($SetupPaperOnly) { $probeArguments += 'setup-paper' }
    elseif ($SettingsMenuOnly) { $probeArguments += 'settings-menu' }
    elseif ($ControlsMenuOnly) { $probeArguments += 'controls-menu' }
    elseif ($StickSettingsOnly) { $probeArguments += 'stick-settings' }
    elseif ($TeamSetupOnly) { $probeArguments += 'team-setup' }
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
    $expectedSaveFailures = @{}
    if ($SettingsMenuOnly -or $result.scope -eq 'full-painter-matrix') {
        $line = 'save failure: Could not write ' + (Join-Path $captures 'settings-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 1
        $line = 'save failure: Could not write ' + (Join-Path $captures 'status-options-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 8
        $line = 'save failure: Could not write ' + (Join-Path $captures 'values-options-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 2
    }
    if ($ControlsMenuOnly -or $result.scope -eq 'full-painter-matrix') {
        $line = 'save failure: Could not write ' + (Join-Path $captures 'controls-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 1
        $line = 'save failure: Could not write ' + (Join-Path $captures 'status-controls-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 16
        $line = 'save failure: Could not write ' + (Join-Path $captures 'values-controls-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 2
    }
    if ($ControlsMenuOnly -or $StickSettingsOnly -or $result.scope -eq 'full-painter-matrix') {
        $line = 'save failure: Could not write ' + (Join-Path $captures 'stick-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 1
    }
    $expectedSaveTail = 'Your settings will not be remembered.'
    $expectedTailCount = [int]($expectedSaveFailures.Values | Measure-Object -Sum).Sum
    $saveTailCount = 0
    foreach ($line in Get-Content -LiteralPath (Join-Path $run 'run.err')) {
        if ($expectedSaveFailures.ContainsKey($line) -and $expectedSaveFailures[$line] -gt 0) {
            --$expectedSaveFailures[$line]
            continue
        }
        if ($line -eq $expectedSaveTail) { ++$saveTailCount; continue }
        if ($line.Trim() -ne '' -and
            $line -notmatch '^no audio: .*The manifest marks this bank optional, so it plays nothing\.') {
            throw "Unexpected probe diagnostic: $line"
        }
    }
    if (@($expectedSaveFailures.Values | Where-Object { $_ -ne 0 }).Count -ne 0 -or
        $saveTailCount -ne $expectedTailCount) {
        throw 'The controlled isolated settings-save failures did not produce exactly their expected diagnostics.'
    }
    if (-not (Test-Path -LiteralPath $saveFixture -PathType Leaf) -or
        (Get-FileHash -LiteralPath $saveFixture).Hash -ne $saveFixtureHash) {
        throw 'The visual probe changed its isolated aim_marker=false save fixture.'
    }
    Write-ContactSheet -CaptureDirectory $captures
    $result.outcome = 'passed'
    $identityDiagnostic = 'Player identity passed all-palette same-team overlap, scripted crossing, sparse/reordered slots, native P1-P4 glyph distinction, quiet/emphasis/reduced effects, own-pane world-only bounds, dead/respawn/story suppression and frozen state. Human playing-size review remains separate.'
    $stageEntryDiagnostic = 'Stage entry passed all four authored names, a three-player roster, bounded title fitting at narrow/wide aspects, opening/monitor skips, static reduced-effects information and hiding before countdown and at GO.'
    if ($IdentityOnly) {
        $result.diagnostic = "Identity-only visual check passed. $identityDiagnostic Other painter matrices were not run."
    }
    elseif ($QuietOnly) {
        $result.diagnostic = 'Quiet-only visual check passed: sprayer quiet character matrix, sniper residual bead and bucket rim runoff; all 1P/4P720 palettes, continuous aim/flip attachment, representative 1080p/QHD, frozen redraw/state, raster bounds, per-tool enabled and reduced-effects suppression/restoration. Other painter matrices were not run.'
    }
    elseif ($MonitorOnly) {
        $result.diagnostic = 'Monitor-only visual check passed: six body shades, adult/mug and child/stickers; native 720p/1080p/QHD and pushed framing, foreground notes survive scene/preview, untinted picture/palette interiors, saved empty slot and closed desktop. Adult/child/mug seated/sigh/stand/leave art, full departure and offset-window translation passed; native pointer hotspot, local detached click ticks, expiry and reduced-effects suppression passed. Actual 2P/4P story beats and before/at/after winning-save and no-paint-close clicks, plus solo practice, passed frozen raster/persona/pose/state/fingerprint and draw RNG checks. Other painter matrices were not run.'
    }
    elseif ($CountdownOnly) {
        $result.diagnostic = "Countdown-only visual check passed: distinct 0-9/GO glyphs, bounded accents and reduced/negative/expired ages at native 720p/1080p/QHD. Real keyboard solo and mixed-device 2P/4P full/quick/off openings, reminder geometry, pause, nonpositive-time updates, GO boundary, resize round-trip and frozen pixels/state/draw RNG/Debug fingerprint passed. $stageEntryDiagnostic Other painter matrices were not run."
    }
    elseif ($ClosingOnly) {
        $result.diagnostic = 'Closing-only visual check passed: actual last-ten clock beats/reduced/settled/zero at 1P/2P/4P720 and 4P1080/QHD, entire-card gating and short-clock suppression before live play; bounded native A/B result cards in every palette and reduced/negative/expired settled states; real A/B winner, no-paint draw and solo card gating. Frozen pixels/state/Debug fingerprint and draw RNG checks passed. Other painter matrices were not run.'
    }
    elseif ($SupplyOnly) {
        $result.diagnostic = 'Supply-only visual check passed: sprayer/mister full, half and empty native reservoirs; exact authored-layer registration, unchanged silhouette and permanent hardware, all 1P/4P720 palettes/aim quadrants/facing inputs/poses and representative 4P1080/QHD. Reduced effects, motion-disabled rendering, ammo clamps, paid depletion, natural regeneration/reset, menu previews and tool switching, frozen pixels/weapon state/draw RNG and unchanged match observation/Debug fingerprint passed. Other painter matrices were not run.'
    }
    elseif ($TeamSetupOnly) {
        $result.diagnostic = 'Team-setup-only hardware check passed: actual 2/3/4-player pages, seeded balanced teams and both three-player majority sides, all joined requesters, readiness reset/reconfirmation, independent X/R footer raster, settings-preserving Weapon return, manual edits, held/conflict/disconnect/focus gates, warning clear, solo no-op and native 720p/1080p four-row/footer bounds. One committed shuffled 2P roster preserved actual teams, input slots and weapons through Pause Restart and two best-of-three rounds via ordinary Results/End continuation; ongoing-series primary card matched its expected ledger. This binary used only the scripted GamepadReader link seam; normal painter binaries retain the real backend. No physical-controller, human readability or listening acceptance.'
    }
    elseif ($StickSettingsOnly) {
        $result.diagnostic = 'Stick-settings-only hardware check passed: actual child navigation, initial/pausing slot selection, clamps, pending cutoff previews, Apply/reload/failure, Back/Escape discard and suspended parent Apply preserving profiles. Full measured response/association copy, five bounded focus cards/arrows, native linear ink/paper contrast, frozen redraw, draw RNG, static Reduce effects and resize passed at 720p/1080p/QHD. Other painter matrices, physical controllers, human readability and listening acceptance were not run.'
    }
    elseif ($ControlsMenuOnly) {
        $result.diagnostic = 'Controls-menu-only hardware check passed: actual Controls/audio edits/clamps, delayed Apply, persisted reload and failed-save warning, Back discard, pending Options values/focus and actual GameLevel Pause return. Complete fitted help, measured labels/values/status, native linear ink, paper/cards/arrows/contrast, frozen redraw/state, draw RNG, static Reduce effects and resize passed across six sizes/aspects. Actual Pad sticks child passed at 720p/1080p/QHD: full response/association copy, focus, initial/pausing slot selection, clamps, pending cutoff previews, Apply/reload/failure, Back/Escape discard and suspended parent Apply preserving profiles. Thirteen independent historical labels across six sizes/aspects, long fitted options, full focus-card label/value unions, separate Controls item-font replacement, retained/reopened/reversed snapshots, repeated child return/Apply, interrupted Enter rearming and custom-label save/discard flows passed. Three independent static-help defaults; Controls/association width and multiline-height fits, unscaled Move-positioned aim caption across the full help column, a help font missing an old glyph, retained/reopened/reversed snapshots through value/preview refresh, old-parent/new-child return, repeated/interrupted Apply, reload and discard passed at 720p. Independent Controls/Sticks pending/saved/failure rasters at six sizes/aspects, authored width fitting with compact help, short-copy scale reset, missing-old-glyph fonts, retained/reopened/reversed callbacks, interrupted/repeated Apply, controlled failure/recovery, isolated reload, both discard routes and old-parent/new-child merge passed. Other painter matrices, physical controllers, human readability, preview audition and listening acceptance were not run.'
        $result.diagnostic += ' Controls On/Off defaults at six sizes/aspects, width/height fitting and refitting, complete focus unions, reopened short copy, a Controls font missing an old glyph, retained/reopened/reversed callbacks, Left/Right boolean assignments, Pad sticks child return/profile merge, interrupted/repeated Apply, failure/recovery, isolated reload and discard passed.'
    }
    elseif ($SettingsMenuOnly) {
        $result.diagnostic = 'Settings-menu-only hardware check passed: actual Options/Credits navigation, row edits, delayed Apply, persisted reload and failed-save warning, Controls/audio return, Back discard, disabled fullscreen resolution and retained windowed preset. Measured full copy/values/status, native paper/cards/arrows/contrast, frozen redraw, draw RNG, static Reduce effects and resize passed across six sizes/aspects. Independent Options pending/saved/failure rasters at six sizes/aspects, authored width fitting and short-copy scale reset, missing-old-glyph font, retained/reopened/reversed callbacks, interrupted/repeated Apply, controlled failure/recovery, isolated reload and both discard routes passed. Other painter matrices, physical controllers, human readability and listening acceptance were not run.'
        $result.diagnostic += ' Options On/Off/Random defaults at six sizes/aspects, width/height fitting, complete focus unions, Random-to-short-fixed-palette scale reset, an Options font missing an old glyph, retained/reopened/reversed callbacks, both fullscreen toggle directions/Random cycles, one Controls child On/Off pair and return, interrupted/repeated windowed-Off Apply, failure/recovery, isolated reload and discard passed.'
    }
    elseif ($SetupPaperOnly) {
        $result.diagnostic = 'Setup-paper-only native check passed: every Mode/Count focus and disabled-solo series at six sizes/aspects matched independent complete paper, card, arrow and fixed-text fixtures, measured containment and ink/paper contrast. Duration cycles, all mode/count/layout/keyboard claim and ordinary routes, copy/font snapshots, native arrow and duration press motion, fixed lettering, reduction/lifecycle reset, frozen redraw/RNG and resize passed. Normal reader, keyboard-driven only; no physical-pad, human readability, listening or performance acceptance.'
    }
    elseif ($JoinStageMotionOnly) {
        $result.diagnostic = 'Join/Stage-motion-only native check passed: actual Join row-gutter initial/middle/rest responses, stable holes, simultaneous and refused claims, final snapshot barrier and missing-P1 rescue; Stage briefing accent with fixed text/preview/edge/camera, real/Random wraps, lock/unlock and locked refusal, deliberate one-choice eligible Random start with its outgoing Random raster latched through pending fade, and single excluded-stage no-op. Native 720p/1080p/QHD, applied Reduce effects, nonpositive time, lifecycle/device/cover/developer resets, frozen pixels/plan/RNG and resize passed. Scripted reader seam; no physical-pad, human readability, listening or performance acceptance.'
    }
    elseif ($SetupMotionOnly) {
        $result.diagnostic = 'Setup-motion-only native check passed: real Mode/Count focus accent and duration press, independent Team selected-icon and Weapon tile motion at 720p/1080p/QHD; fixed weapon art/paint/supply, names/descriptions and badges, local envelopes, simultaneous/rapid input, readiness, shuffle, rejected actions, lifecycle resets, immediate applied Reduce effects, frozen redraw/RNG and resize. Scripted reader seam; no physical-pad, human readability, listening or performance acceptance.'
    }
    elseif ($MenuMotionOnly) {
        $result.diagnostic = 'Menu-motion-only hardware check passed: actual Home changed-focus arrows and Controls OptionRow paper initial/middle/rest, plus Apply pressed/rest native rasters at 720p/1080p/QHD, unchanged label/value lettering and local ornament bounds; first focus stays still, nonpositive time, rapid navigation, immediate applied Reduce effects, actual Controls Apply/reload/reversal, deactivation, nested Sticks return, frozen redraw/RNG and resize. Normal GamepadReader backend; other painter matrices, physical pads, human readability, listening and performance acceptance were not run.'
    }
    elseif ($MenuTextOnly) {
        $result.diagnostic = 'Menu-text-only hardware check passed: independent brand, 11 headings, 17 legends and ten static item roles at 720p/1080p/QHD; eight Mode/Count labels and solo hint at six sizes/aspects; reachable Random-stage full text/shadow rasters, width-only long/short fitting, real/Random cycles without RNG or plan commits, captured copy/item font across replacement/reopen/reversal, live fit geometry and deliberate eligible Random start with pending music-exit plan. Actual arena lock/unlock, fitted Options labels, duration cycle/wrap/confirm, all modes/counts/layouts/keyboard claims, solo-disabled series, Join/Mode/Home Back and Credits return passed. Title/Home wordmark and fixed copy, complete Start card, separate title/item/detail fonts, retained/reopened/reversed snapshots, repeated Title/Home/Back, Mode return and interrupted Enter rearming passed at 720p. The no-controller hint remained hidden under the keyboard policy and matched its full independently measured paper crop. Custom text/font snapshots through prompt rebuilds, focus, duration and child return, frozen redraw and draw RNG passed. Other painter matrices, visible controller-loss, physical controllers, human readability, performance and listening acceptance were not run.'
        $result.diagnostic += ' Both arena status messages passed independent full-shadow raster checks across real/Random browse/locked states at six sizes, with captured detail font/position/shadow retained through forced refresh/reopen/reversal and live ink updates; provisional plan and RNG remained unchanged.'
        $result.diagnostic += ' Four fixed briefing strings plus adjacent mode/roster passed independent complete-shadow crops for solo and connected scripted 2P single/best-three/best-five at six sizes. Captured brand font/text/position/shadow/ink survived replacement/reopen/reversal and selection refresh, including the full solo overhang. This focused binary used the scripted reader seam, with neutral pads outside the briefing fixture; no physical pad acceptance.'
        $result.diagnostic += ' Fixed Random navigation and both fixed hints passed full-shadow rasters across real/Random browse/locked states at six sizes, including unchanged numbered arenas/Random count. Unscaled navigation, width-only hint fit/short reset, separate detail/brand font snapshots, cached versus live fit refresh/reopen/reversal and ignored locked arrows preserved the provisional plan and RNG.'
        $result.diagnostic += ' Solo/single-match mode labels passed independent fixed-copy and separate detail-font snapshot checks at six sizes, with captured unscaled mode position/shadow/ink retained through refresh/reopen/reversal; independently specified BEST OF 3/5, connected roster, plan and RNG remained unchanged.'
    }
    elseif ($WeaponCatalogOnly) {
        $result.diagnostic = 'Weapon-catalog-only hardware check passed: six historical labels and body/paint/full-supply rasters at 720p/1080p/QHD, both wraps, provisional browsing, fresh release confirmation, seeded Random primary, Stage/Mode Back preserving settings, custom layers and sparse alternate atlas/fill, reopen adoption and retained page snapshots, frozen redraw and draw RNG. Normal GamepadReader backend; other painter matrices, physical controllers, human readability, performance and listening acceptance were not run.'
    }
    elseif ($MainMenuOnly) {
        $result.diagnostic = 'Main-menu-only hardware check passed: actual keyboard Start/Back/Play/Options/Credits/Exit; measured text/paper bounds, independent historical Title wordmark/subtitle/Start and Home subtitle rasters, complete Start-card bounds, native linear text, ink/paper contrast, all four Home focus cards and detached arrows. The connection hint stayed hidden under the keyboard policy across its full independently measured crop. Frozen redraw, draw RNG, static Reduce effects and resize passed across six display sizes/aspects. Other painter matrices, visible controller-loss, physical controllers, human readability and listening acceptance were not run.'
    }
    elseif ($ResultsMenuOnly) {
        $result.diagnostic = 'Results-menu-only hardware check passed: actual results page timing and keyboard continuation, paper and score rendering, solo/absent-team and revealed/draw/series states, sparse-slot contributions, native display/aspect matrix, frozen redraw, Reduce effects and resize. Other painter matrices, human readability and listening acceptance were not run.'
    }
    elseif ($PauseMenuOnly) {
        $result.diagnostic = 'Pause-menu-only hardware check passed: six independent legacy labels and both independent legacy confirmation questions on actual Pause/Restart/Quit pages across six display sizes/aspects; unscaled detail-font question bounds/full-paper centering and production point filtering, custom labels/questions and item/detail fonts, retained/reopened/reversed snapshots, Controls return, safe No/Back, deliberate Resume/Restart/Quit, interrupted Enter focus and synthetic missing-seat Restart cancellation/gate clearing without automatic Resume. Existing missing-controller recovery routes, native paper/enabled focus cards, static Reduce effects, frozen redraw/RNG and resize passed. Other painter matrices, physical controllers/reconnection, human readability and listening acceptance were not run.'
    }
    elseif ($EndMenuOnly) {
        $result.diagnostic = 'End-menu-only hardware check passed: four independent historical action labels, continuation and three independent fixed headings on actual single/ongoing/draw/completed-series pages across six native display sizes/aspects; both Exit placements and hidden setup rows. Complete unscaled heading bounds/full-paper centering and production point filtering, custom labels/headings and item/heading fonts, retained/reopened/reversed snapshots, unchanged Session ledger, repeated typed actions, pending music-exit gating, focus interruption and held Enter rearming passed alongside native paper/focus rendering, frozen redraw/RNG, Reduce effects and resize. Other painter matrices, physical controllers, actual audio fade/listening and human readability acceptance were not run.'
    }
    elseif ($ResumeOnly) {
        $result.diagnostic = 'Resume-only actual GameLevel hardware check passed: 1P/2P/4P keyboard-driven pause/resume/re-pause, focus loss/return, full world/HUD raster and detailed player/projectile/cosmetic freeze, dead respawn, held-action rearming, long final count-in step, Restart and Quit. Native bounded RESUMING paper, repeated frozen drawing and exact 720p/1080p/QHD resize round-trip passed. Other painter matrices, physical controllers, human readability and listening acceptance were not run.'
    }
    elseif ($HudOnly) {
        $result.diagnostic = 'HUD-only visual check passed: exact full/half/empty and clamped health/paint tracks, exterior keylines, dark-text/pale-paper raster contrast, 1P/2P/3P/4P720, all three 1P/4P palettes and representative 1080p/QHD/4:3/portrait/ultrawide. Own-pane bounds, static Reduce effects information, frozen pixels/draw RNG, real-match observation/Debug fingerprint and resize round-trip passed. Existing final-ten clock matrix passed. Other painter matrices and human readability acceptance were not run.'
    }
    else {
        $result.diagnostic = "Painter base/action, five-tool mechanism, weapon paid-shot/aim-follow and controlled muzzle/empty/recovered on/off, step/skid contact, and actual idle drip/quiet character matrices passed. Gameplay HUD full/half/empty and clamped tracks, independent health/paint, native text contrast, pane bounds, palettes/aspects, static Reduce effects, frozen real-match state and exact resize round-trip passed. Aim readiness markers passed actual ready/recovering/empty, all palette/quadrant own-pane on/off bounds, isolated marker raster, frozen redraw and pause checks; the disabled isolated-save fixture remained unchanged. Local view shake preserved other panes, opaque HUD ink, base camera/aim, frozen redraw and simulation fingerprint, including suspension and resume. Distinct pinned body/tool and quiet on/off/blink rasters, frozen motion/feedback/quiet timers/redraw, separate player/drip raster culling bounds, elimination/redraw/respawn and idle whole-story poses passed. $identityDiagnostic $stageEntryDiagnostic"
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
