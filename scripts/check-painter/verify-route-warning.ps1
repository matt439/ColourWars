# Dot-sourced only for RouteWarningOnly. These are observation integrity gates:
# off-screen landings and censored/no-hit projectile trials are retained outcomes.
function Get-RouteWarningNumber {
    param([object] $Value, [string] $Name)
    $number = 0.0
    if (-not [double]::TryParse([string]$Value, [Globalization.NumberStyles]::Float,
        [Globalization.CultureInfo]::InvariantCulture, [ref]$number) -or
        [double]::IsNaN($number) -or [double]::IsInfinity($number)) {
        throw "Route/warning evidence has invalid finite number $Name."
    }
    return $number
}

function Get-RouteWarningInteger {
    param([object] $Value, [string] $Name, [long] $Minimum = 0, [long] $Maximum = 25000)
    $number = 0L
    if (-not [long]::TryParse([string]$Value, [Globalization.NumberStyles]::Integer,
        [Globalization.CultureInfo]::InvariantCulture, [ref]$number) -or
        $number -lt $Minimum -or $number -gt $Maximum) {
        throw "Route/warning evidence has invalid integer $Name."
    }
    return $number
}

function Assert-RouteWarningColumns {
    param([object[]] $Rows, [string[]] $Columns, [string] $Name)
    if ($Rows.Count -eq 0 -or
        (($Rows[0].PSObject.Properties.Name) -join '|') -cne ($Columns -join '|')) {
        throw "Route/warning $Name changed its nonempty CSV schema."
    }
}

function Get-RouteWarningPane {
    param([int] $Count, [int] $View)
    if ($Count -lt 1 -or $Count -gt 4 -or $View -lt 1 -or $View -gt $Count) {
        throw 'Route/warning evidence names an invalid player/view pair.'
    }
    if ($Count -eq 1) { return @(0, 0, 1280, 720) }
    if ($Count -eq 2) { return @(0, (($View - 1) * 360), 1280, 360) }
    # Three-player order is TL, BR, BL: the top-right quadrant stays unused.
    if ($Count -eq 3) {
        if ($View -eq 1) { return @(0, 0, 640, 360) }
        if ($View -eq 2) { return @(640, 360, 640, 360) }
        return @(0, 360, 640, 360)
    }
    return @((($View - 1) % 2 * 640), ([int][Math]::Floor(($View - 1) / 2.0) * 360), 640, 360)
}

function Get-RouteWarningGeometry {
    param([double[]] $World, [double[]] $Camera, [double[]] $Pane,
        [object[]] $Panels = @(), [bool] $NearRight = $false)
    # Round at the same float-operation boundaries as Camera, rather than
    # allowing a decimal-text tolerance to change edge inclusion decisions.
    $x = [single]([single]([single]$World[0] - [single]$Camera[0]) * [single]$Camera[2])
    $y = [single]([single]([single]$World[1] - [single]$Camera[1]) * [single]$Camera[2])
    $width = [single]([single]$World[2] * [single]$Camera[2])
    $height = [single]([single]$World[3] * [single]$Camera[2])
    $right = [single]($x + $width); $bottom = [single]($y + $height)
    $left = [Math]::Max(0.0, $x); $top = [Math]::Max(0.0, $y)
    $clipWidth = [single][Math]::Max(0.0, [single]([Math]::Min($Pane[2], $right) - $left))
    $clipHeight = [single][Math]::Max(0.0, [single]([Math]::Min($Pane[3], $bottom) - $top))
    $edge = if ($NearRight) { $right } else { $x }
    $hudOverlap = $false
    foreach ($panel in $Panels) {
        $overlapWidth = [Math]::Min([single]($left + $clipWidth), [single]($panel[0] + $panel[2])) -
            [Math]::Max($left, $panel[0])
        $overlapHeight = [Math]::Min([single]($top + $clipHeight), [single]($panel[1] + $panel[3])) -
            [Math]::Max($top, $panel[1])
        if ($overlapWidth -gt 0.0 -and $overlapHeight -gt 0.0) { $hudOverlap = $true }
    }
    return @{
        projected_x = $x; projected_y = $y; projected_width = $width; projected_height = $height
        clip_width = $clipWidth; clip_height = $clipHeight; edge_x = $edge
        intersects = [int]($clipWidth -gt 0.0 -and $clipHeight -gt 0.0)
        contained = [int]($x -ge 0.0 -and $y -ge 0.0 -and $right -le $Pane[2] -and $bottom -le $Pane[3])
        edge_inside = [int]($edge -ge 0.0 -and $edge -lt $Pane[2] -and $y -ge 0.0 -and $bottom -le $Pane[3])
        hud_overlap = [int]$hudOverlap
    }
}

# Literal controls distinguish a positive sliver, exact outside edge, vertical
# clipping, oversized support and nonzero pane origin from complete visibility.
$routeControlPanels = [object[]]::new(3)
$routeControlPanels[0] = @(10.0, 10.0, 50.0, 50.0)
$routeControlPanels[1] = @(0.0, 0.0, 0.0, 0.0)
$routeControlPanels[2] = @(0.0, 0.0, 0.0, 0.0)
$routeControlCamera = @(100.0, 200.0, [single](2.0 / 3.0))
$routeControlPane = @(640.0, 360.0, 640.0, 360.0)
$routeControlFull = Get-RouteWarningGeometry @(130.0, 230.0, 52.0, 8.0) $routeControlCamera $routeControlPane $routeControlPanels
$routeControlSliver = Get-RouteWarningGeometry @(1059.5, 230.0, 52.0, 8.0) $routeControlCamera $routeControlPane $routeControlPanels
$routeControlOutside = Get-RouteWarningGeometry @(1060.0, 230.0, 52.0, 8.0) $routeControlCamera $routeControlPane $routeControlPanels
$routeControlBottom = Get-RouteWarningGeometry @(130.0, 739.0, 52.0, 8.0) $routeControlCamera $routeControlPane $routeControlPanels
$routeControlWide = Get-RouteWarningGeometry @(100.0, 230.0, 980.0, 40.0) $routeControlCamera $routeControlPane $routeControlPanels
if ($routeControlFull.contained -ne 1 -or $routeControlFull.edge_inside -ne 1 -or $routeControlFull.hud_overlap -ne 1 -or
    [Math]::Abs($routeControlFull.projected_x - 20.0) -gt 0.00001 -or
    $routeControlSliver.intersects -ne 1 -or $routeControlSliver.contained -ne 0 -or
    $routeControlSliver.clip_width -ge 1.0 -or $routeControlSliver.hud_overlap -ne 0 -or
    $routeControlOutside.intersects -ne 0 -or $routeControlOutside.edge_inside -ne 0 -or
    $routeControlBottom.intersects -ne 1 -or $routeControlBottom.contained -ne 0 -or $routeControlBottom.edge_inside -ne 0 -or
    $routeControlWide.intersects -ne 1 -or $routeControlWide.contained -ne 0 -or
    ((Get-RouteWarningPane 3 2) -join ',') -cne '640,360,640,360' -or
    ((Get-RouteWarningPane 3 3) -join ',') -cne '0,360,640,360') {
    throw 'Route/warning independent visibility/layout negative controls failed.'
}

$routeSummary = Get-Content -LiteralPath (Join-Path $captures 'route-warning-summary.json') -Raw | ConvertFrom-Json
$routeTicks = @(Import-Csv -LiteralPath (Join-Path $captures 'route-warning-ticks.csv'))
$routeTrials = @(Import-Csv -LiteralPath (Join-Path $captures 'route-warning-trials.csv'))
$routeCaptures = @(Import-Csv -LiteralPath (Join-Path $captures 'route-warning-captures.csv'))
$routePanels = @(Import-Csv -LiteralPath (Join-Path $captures 'route-warning-hud.csv'))
$routeTickColumns = @('trial', 'kind', 'tick', 'players', 'view', 'input_slot', 'object_id', 'player_x', 'player_y',
    'velocity_x', 'velocity_y', 'health', 'ammo', 'camera_x', 'camera_y', 'camera_scale', 'pane_x', 'pane_y',
    'pane_width', 'pane_height', 'world_x', 'world_y', 'world_width', 'world_height', 'projected_x', 'projected_y',
    'projected_width', 'projected_height', 'clip_width', 'clip_height', 'intersects', 'contained', 'edge_x',
    'edge_inside', 'hud_overlap', 'commit', 'lip_crossing', 'contact', 'paid', 'evidence_scope')
$routeTrialColumns = @('trial', 'stage', 'case', 'paint', 'players', 'view', 'input_slot', 'direction', 'tool', 'ticks',
    'commit_tick', 'lip_crossing_tick', 'landing_tick', 'first_edge_tick', 'first_strip_tick', 'strip_full_before_commit', 'strip_full_at_commit',
    'strip_losses', 'paid_tick', 'first_actor_tick', 'first_collision_tick', 'first_draw_candidate_tick', 'first_damage_tick',
    'health_loss', 'live_projectiles_end', 'landing_support', 'whole_target_before_commit', 'target_x', 'target_y',
    'target_width', 'target_height', 'strip_x', 'strip_y', 'strip_width', 'strip_height', 'initial_owned_fraction',
    'aim_x', 'aim_y', 'projectile_attribution', 'evidence_scope')
$routeCaptureColumns = @('capture', 'trial', 'event', 'tick', 'players', 'view', 'input_slot', 'pane_x', 'pane_y',
    'pane_width', 'pane_height', 'full_width', 'full_height', 'frozen_pixels', 'public_state', 'projectile_state',
    'debug_fingerprint', 'rng', 'evidence_scope')
Assert-RouteWarningColumns $routeTicks $routeTickColumns 'ticks'
Assert-RouteWarningColumns $routeTrials $routeTrialColumns 'trials'
Assert-RouteWarningColumns $routeCaptures $routeCaptureColumns 'captures'
Assert-RouteWarningColumns $routePanels @('trial', 'panel', 'x', 'y', 'width', 'height', 'envelope_scope') 'HUD panels'
if ($routeSummary.outcome -cne 'passed' -or $routeSummary.scope -cne 'route-warning-only') {
    throw 'Route/warning native summary did not pass its focused scope.'
}
$routeCounts = @{}
foreach ($entry in @(@('full_frames', 1, 40), @('native_crops', 1, 100), @('readbacks', 2, 80),
    @('level_updates', 1, 15000), @('level_builds', 38, 38), @('tick_rows', 1, 25000),
    @('trials', 66, 66), @('capture_rows', 1, 100), @('negative_controls', 5, 5),
    @('route_trials', 44, 44), @('warning_trials', 22, 22),
    @('controlled_heavy_contacts', 1, 2000), @('paired_ticks', 1, 15000))) {
    $routeCounts[$entry[0]] = Get-RouteWarningInteger $routeSummary.($entry[0]) $entry[0] $entry[1] $entry[2]
}
if ($routeCounts.tick_rows -ne $routeTicks.Count -or $routeCounts.trials -ne $routeTrials.Count -or
    $routeCounts.capture_rows -ne $routeCaptures.Count -or $routeCounts.native_crops -ne $routeCaptures.Count -or
    $routeCounts.readbacks -ne 2 * $routeCounts.full_frames -or $routePanels.Count -ne 3 * $routeTrials.Count) {
    throw 'Route/warning summary differs from its bounded CSV counts or repeated-draw budget.'
}
$routeDt = Get-RouteWarningNumber $routeSummary.simulated_dt 'simulated_dt'
if ([Math]::Abs($routeDt - [single](166666.0 / 10000000.0)) -gt 0.000000001) {
    throw 'Route/warning evidence no longer uses the real quantized 60 Hz simulation step.'
}

$routeTrialById = @{}; $routePanelByTrial = @{}; $routeRowsByTrial = @{}; $routePrimaryByTrial = @{}
$routeSlots = @(3, 1, 2, 0)
$routeExpected = @{}
foreach ($count in 1..4) {
    foreach ($paint in @('clean', 'heavy')) {
        foreach ($view in 1..$count) {
            $direction = if ($view % 2 -eq 1) { 1 } else { -1 }
            $upper = $view -ge 3
            $case = if ($upper) { 'optional-ruler-tap' } else { 'compulsory-ramp-walk' }
            $target = if (-not $upper) { 'centre_plinth' } elseif ($direction -eq 1) { 'upper_step_a' } else { 'upper_step_b' }
            $routeExpected["crossfeed-$($count)p-$paint-v$view"] = @('crossfeed', $case, $paint, $count, $view, $direction, 'none', $target)
        }
        foreach ($gap in @(@('tap120', 'short_landing_long_takeoff'), @('held280', 'long_landing_running_takeoff'),
            @('running440-early120', 'running_landing'))) {
            $routeExpected["course-$($count)p-$paint-$($gap[0])"] = @('movement_course', $gap[0], $paint, $count, 1, 1, 'none', $gap[1])
        }
    }
}
$routeIncomingExpected = [Collections.Generic.List[object]]::new()
foreach ($tool in @('sprayer', 'sniper', 'mister', 'bucket')) {
    foreach ($motion in @('stationary', 'retreat')) { $routeIncomingExpected.Add(@(2, $tool, $motion, 'left', 'clean')) }
}
foreach ($count in @(3, 4)) {
    foreach ($tool in @('sniper', 'bucket')) {
        foreach ($motion in @('stationary', 'retreat')) { $routeIncomingExpected.Add(@($count, $tool, $motion, 'left', 'clean')) }
    }
}
foreach ($tool in @('sniper', 'bucket')) {
    foreach ($motion in @('stationary', 'retreat')) { $routeIncomingExpected.Add(@(4, $tool, $motion, 'mirror', 'clean')) }
    $routeIncomingExpected.Add(@(4, $tool, 'retreat', 'left', 'heavy'))
}
foreach ($entry in $routeIncomingExpected) {
    $id = "incoming-$($entry[0])p-$($entry[1])-$($entry[2])-$($entry[3])-$($entry[4])"
    $direction = if ($entry[3] -ceq 'mirror') { 1 } else { -1 }
    $routeExpected[$id] = @('crossfeed', ('fixed-single-burst-' + $entry[2]), $entry[4], $entry[0], 1, $direction, $entry[1], '')
}
if ($routeExpected.Count -ne 66) { throw 'Route/warning verifier scenario contract is malformed.' }
$routeStages = @{}
foreach ($stage in @('crossfeed', 'movement_course')) {
    $routeStages[$stage] = Get-Content -LiteralPath (Join-Path $gameDirectory "levels/$stage.json") -Raw | ConvertFrom-Json
}
$routeTuning = Get-Content -LiteralPath (Join-Path $gameDirectory 'tuning.json') -Raw | ConvertFrom-Json
foreach ($trial in $routeTrials) {
    if ($trial.trial -cnotmatch '^[a-z0-9][a-z0-9-]*$' -or $routeTrialById.ContainsKey($trial.trial) -or
        $trial.stage -cnotin @('crossfeed', 'movement_course') -or $trial.paint -cnotin @('clean', 'heavy') -or
        $trial.tool -cnotin @('none', 'sprayer', 'sniper', 'mister', 'bucket') -or
        $trial.direction -cnotin @('-1', '1') -or $trial.projectile_attribution -cne 'single-paid-burst-or-no-projectile' -or
        $trial.evidence_scope -cne 'simulated-60Hz-observation') {
        throw "Route/warning trial changed its identity or observation scope: $($trial.trial)"
    }
    $count = Get-RouteWarningInteger $trial.players 'trial.players' 1 4
    $view = Get-RouteWarningInteger $trial.view 'trial.view' 1 $count
    if (-not $routeExpected.ContainsKey($trial.trial)) { throw "Route/warning trial is outside its representative matrix: $($trial.trial)" }
    $expected = $routeExpected[$trial.trial]
    $actual = @($trial.stage, $trial.case, $trial.paint, $trial.players, $trial.view, $trial.direction, $trial.tool)
    if (($actual -join '|') -cne (($expected[0..6]) -join '|')) { throw 'Route/warning trial changed its declared scenario.' }
    if ($trial.input_slot -cne [string]$routeSlots[$view - 1]) { throw 'Route/warning trial changed its sparse input/view mapping.' }
    $last = Get-RouteWarningInteger $trial.ticks 'trial.ticks' 1 1200
    foreach ($field in @('commit_tick', 'lip_crossing_tick', 'landing_tick', 'first_edge_tick', 'first_strip_tick', 'paid_tick',
        'first_actor_tick', 'first_collision_tick', 'first_draw_candidate_tick', 'first_damage_tick')) {
        $null = Get-RouteWarningInteger $trial.$field $field -1 $last
    }
    foreach ($field in @('strip_full_before_commit', 'strip_full_at_commit')) {
        $null = Get-RouteWarningInteger $trial.$field $field -1 1
    }
    foreach ($field in @('whole_target_before_commit')) { $null = Get-RouteWarningInteger $trial.$field $field 0 1 }
    $null = Get-RouteWarningInteger $trial.strip_losses 'strip_losses' 0 $last
    $null = Get-RouteWarningInteger $trial.live_projectiles_end 'live_projectiles_end' 0 20
    foreach ($field in @('target_x', 'target_y', 'target_width', 'target_height', 'strip_x', 'strip_y', 'strip_width',
        'strip_height', 'initial_owned_fraction', 'aim_x', 'aim_y', 'health_loss')) {
        $null = Get-RouteWarningNumber $trial.$field $field
    }
    $owned = [double]$trial.initial_owned_fraction
    if ($owned -lt 0.0 -or $owned -gt 1.0 -or ($trial.paint -ceq 'heavy' -and $owned -le 0.99) -or
        ($trial.paint -ceq 'clean' -and $owned -ne 0.0)) { throw 'Route/warning initial controlled paint state is invalid.' }
    if ([double]$trial.health_loss -lt 0.0 -or [double]$trial.health_loss -gt 1.0) { throw 'Route/warning health loss is invalid.' }
    if ($trial.tool -ceq 'none') {
        $authored = @($routeStages[$trial.stage].collision_objects | Where-Object { $_.name -ceq $expected[7] })
        if ($authored.Count -ne 1) { throw 'Route/warning required support is absent from staged authoring.' }
        $target = $authored[0].rectangle
        foreach ($field in @('x', 'y', 'width', 'height')) {
            if ([double]$trial.('target_' + $field) -ne [double]$target.$field) { throw 'Route/warning landing target differs from staged authoring.' }
        }
        $stripWidth = [double]$routeTuning.player.size.x
        $stripX = if ($trial.direction -ceq '1') { [double]$target.x } else { [double]$target.x + [double]$target.width - $stripWidth }
        if ([double]$trial.strip_x -ne $stripX -or [double]$trial.strip_y -ne [double]$target.y -or
            [double]$trial.strip_width -ne $stripWidth -or [double]$trial.strip_height -ne 8.0 -or
            $trial.landing_support -cnotin @('target', 'other', 'not-observed')) {
            throw 'Route/warning body-width landing strip changed its authored near-lip interval.'
        }
    }
    elseif ($trial.landing_support -cne 'not-observed' -or $trial.commit_tick -cne '-1' -or $trial.lip_crossing_tick -cne '-1' -or
        $trial.landing_tick -cne '-1' -or $trial.first_edge_tick -cne '-1' -or $trial.first_strip_tick -cne '-1') {
        throw 'Route/warning incoming-shot trial claims route events.'
    }
    $routeTrialById[$trial.trial] = $trial
    $routePanelByTrial[$trial.trial] = @{}
    $routeRowsByTrial[$trial.trial] = [Collections.Generic.List[object]]::new()
    $routePrimaryByTrial[$trial.trial] = @{}
}
foreach ($panel in $routePanels) {
    if (-not $routeTrialById.ContainsKey($panel.trial) -or
        $panel.envelope_scope -cne 'conservative-paper-face-shadow-clock-union') { throw 'Route/warning HUD envelope has unknown scope/trial.' }
    $index = Get-RouteWarningInteger $panel.panel 'panel' 0 2
    $byIndex = $routePanelByTrial[$panel.trial]
    if ($byIndex.ContainsKey([string]$index)) { throw 'Route/warning HUD envelope index is duplicated.' }
    $box = @()
    foreach ($field in @('x', 'y', 'width', 'height')) { $box += Get-RouteWarningNumber $panel.$field "panel.$field" }
    if ($box[2] -le 0.0 -or $box[3] -le 0.0) { throw 'Route/warning HUD envelope is empty.' }
    $byIndex[[string]$index] = $box
}
$routePresentation = Get-Content -LiteralPath (Join-Path $gameDirectory 'presentation.json') -Raw | ConvertFrom-Json
$routeScale = [single](720.0 / [double]$routePresentation.camera.reference_height)
$routeSeenRows = @{}
foreach ($row in $routeTicks) {
    if (-not $routeTrialById.ContainsKey($row.trial) -or
        $row.kind -cnotin @('landing-strip', 'target', 'attacker', 'collision', 'draw-candidate') -or
        $row.evidence_scope -cne 'post-tick-geometric-observation') { throw 'Route/warning tick has an unknown trial/kind/scope.' }
    $trial = $routeTrialById[$row.trial]
    $tick = Get-RouteWarningInteger $row.tick 'tick' 0 ([int]$trial.ticks)
    $object = Get-RouteWarningInteger $row.object_id 'object_id' 0 20
    $key = "$($row.trial)|$tick|$($row.kind)|$object"
    if ($routeSeenRows.ContainsKey($key)) { throw "Route/warning tick row is duplicated: $key" }
    $routeSeenRows[$key] = $true
    if ($row.players -cne $trial.players -or $row.view -cne $trial.view -or $row.input_slot -cne $trial.input_slot -or
        (($row.kind -cin @('collision', 'draw-candidate')) -ne ($object -gt 0))) {
        throw "Route/warning tick changed its trial roster or projectile identity: $key"
    }
    foreach ($field in @('player_x', 'player_y', 'velocity_x', 'velocity_y', 'health', 'ammo', 'camera_x', 'camera_y',
        'camera_scale', 'world_x', 'world_y', 'world_width', 'world_height', 'projected_x', 'projected_y',
        'projected_width', 'projected_height', 'clip_width', 'clip_height', 'edge_x')) {
        $null = Get-RouteWarningNumber $row.$field "$field/$key"
    }
    if ([double]$row.world_width -le 0.0 -or [double]$row.world_height -le 0.0 -or
        [double]$row.health -lt 0.0 -or [double]$row.health -gt 1.0 -or
        [double]$row.ammo -lt 0.0 -or [double]$row.ammo -gt 1.0 -or
        [Math]::Abs([double]$row.camera_scale - $routeScale) -gt 0.0000001) {
        throw "Route/warning tick has invalid simulation bounds or gameplay camera scale: $key"
    }
    $pane = @(Get-RouteWarningPane ([int]$row.players) ([int]$row.view))
    if ((@($row.pane_x, $row.pane_y, $row.pane_width, $row.pane_height) -join ',') -cne ($pane -join ',')) {
        throw "Route/warning tick changed the sparse native pane order: $key"
    }
    $panels = @($routePanelByTrial[$row.trial].Values)
    if ($panels.Count -ne 3) { throw 'Route/warning trial needs exactly three conservative HUD envelopes.' }
    $geometry = Get-RouteWarningGeometry @([double]$row.world_x, [double]$row.world_y, [double]$row.world_width,
        [double]$row.world_height) @([double]$row.camera_x, [double]$row.camera_y, [double]$row.camera_scale) $pane $panels `
        ($row.kind -ceq 'landing-strip' -and $trial.direction -ceq '-1')
    foreach ($field in @('projected_x', 'projected_y', 'projected_width', 'projected_height', 'clip_width', 'clip_height', 'edge_x')) {
        if ([Math]::Abs([double]$row.$field - [double]$geometry[$field]) -gt 0.00001) {
            throw "Route/warning independent projection differs at $field/$key."
        }
    }
    foreach ($field in @('intersects', 'contained', 'edge_inside', 'hud_overlap', 'commit', 'lip_crossing', 'contact', 'paid')) {
        $null = Get-RouteWarningInteger $row.$field "$field/$key" 0 1
        if ($geometry.ContainsKey($field) -and [int]$row.$field -ne [int]$geometry[$field]) {
            throw "Route/warning independent geometry flag differs at $field/$key."
        }
    }
    if ($row.kind -ceq 'landing-strip') {
        if ($trial.tool -cne 'none' -or
            (@($row.world_x, $row.world_y, $row.world_width, $row.world_height) -join ',') -cne
                (@($trial.strip_x, $trial.strip_y, $trial.strip_width, $trial.strip_height) -join ',')) {
            throw 'Route/warning route changed its declared landing strip between ticks.'
        }
    }
    elseif ($trial.tool -ceq 'none') { throw 'Route/warning route includes undeclared projectile observations.' }
    if ($row.kind -ceq 'target' -and
        ([double]$row.world_x -ne [double]$row.player_x -or [double]$row.world_y -ne [double]$row.player_y -or
         [double]$row.world_width -ne [double]$routeTuning.player.size.x -or
         [double]$row.world_height -ne [double]$routeTuning.player.size.y)) {
        throw 'Route/warning target geometry differs from its observed actual player body.'
    }
    $primaryKind = if ($trial.tool -ceq 'none') { 'landing-strip' } else { 'target' }
    if ($row.kind -ceq $primaryKind) { $routePrimaryByTrial[$row.trial][[string]$tick] = $row }
    $routeRowsByTrial[$row.trial].Add($row)
}

foreach ($trial in $routeTrials) {
    $primary = $routePrimaryByTrial[$trial.trial]; $rows = $routeRowsByTrial[$trial.trial]
    if ($primary.Count -ne [int]$trial.ticks + 1) { throw 'Route/warning primary observations are not continuous from initial tick zero.' }
    $events = @{ commit_tick = -1; lip_crossing_tick = -1; landing_tick = -1; first_edge_tick = -1; first_strip_tick = -1; paid_tick = -1;
        first_actor_tick = -1; first_collision_tick = -1; first_draw_candidate_tick = -1; first_damage_tick = -1 }
    $previousFull = $false; $losses = 0; $before = -1; $at = -1; $wholeBefore = 0
    $initialHealth = [double]$primary['0'].health
    if (($trial.tool -ceq 'none' -and $trial.stage -ceq 'crossfeed' -and $trial.ticks -cne '120') -or
        ($trial.stage -ceq 'movement_course' -and [int]$trial.ticks -gt 150) -or
        ($trial.tool -cne 'none' -and [int]$trial.ticks -gt 96)) { throw 'Route/warning trial exceeded its declared continuous observation horizon.' }
    for ($tick = 0; $tick -le [int]$trial.ticks; ++$tick) {
        if (-not $primary.ContainsKey([string]$tick)) { throw 'Route/warning primary observation skipped a tick.' }
        $row = $primary[[string]$tick]
        if ($row.commit -ceq '1') {
            if ($events.commit_tick -ge 0) { throw 'Route/warning trial committed more than once.' }
            $events.commit_tick = $tick; $before = [int]$previousFull; $at = [int]$row.contained
            if ($tick -gt 0) {
                $prior = $primary[[string]($tick - 1)]
                $pane = @(Get-RouteWarningPane ([int]$trial.players) ([int]$trial.view))
                $worldRight = [single]([single]$prior.camera_x + [single]([single]$pane[2] / [single]$prior.camera_scale))
                $worldBottom = [single]([single]$prior.camera_y + [single]([single]$pane[3] / [single]$prior.camera_scale))
                $wholeBefore = [int]([single]$trial.target_x -ge [single]$prior.camera_x -and
                    [single]$trial.target_y -ge [single]$prior.camera_y -and
                    [single]([single]$trial.target_x + [single]$trial.target_width) -le $worldRight -and
                    [single]([single]$trial.target_y + [single]$trial.target_height) -le $worldBottom)
            }
        }
        if ($row.paid -ceq '1') {
            if ($events.paid_tick -ge 0) { throw 'Route/warning trial has more than one paid burst.' }
            $events.paid_tick = $tick
        }
        if ($row.lip_crossing -ceq '1') {
            if ($events.lip_crossing_tick -ge 0) { throw 'Route/warning route crossed its takeoff lip more than once.' }
            $events.lip_crossing_tick = $tick
        }
        if ($trial.tool -ceq 'none') {
            if ($row.edge_inside -ceq '1' -and $events.first_edge_tick -lt 0) { $events.first_edge_tick = $tick }
            if ($row.contained -ceq '1' -and $events.first_strip_tick -lt 0) { $events.first_strip_tick = $tick }
            if ($row.contact -ceq '1') {
                if ($events.landing_tick -ge 0) { throw 'Route/warning route has more than one first landing.' }
                $events.landing_tick = $tick
            }
            if (([int]$trial.landing_tick -lt 0 -or $tick -le [int]$trial.landing_tick) -and
                $previousFull -and $row.contained -ceq '0') { ++$losses }
        }
        elseif ([double]$row.health -lt $initialHealth -and $events.first_damage_tick -lt 0) {
            $events.first_damage_tick = $tick
        }
        $previousFull = $row.contained -ceq '1'
    }
    foreach ($row in $rows) {
        $sameTick = $primary[$row.tick]
        foreach ($field in @('player_x', 'player_y', 'velocity_x', 'velocity_y', 'health', 'ammo', 'camera_x', 'camera_y',
            'camera_scale', 'commit', 'lip_crossing', 'contact', 'paid')) {
            if ($row.$field -cne $sameTick.$field) { throw 'Route/warning same-tick observations disagree on the actual world state.' }
        }
        if ($row.kind -cin @('collision', 'draw-candidate')) {
            $otherKind = if ($row.kind -ceq 'collision') { 'draw-candidate' } else { 'collision' }
            if (-not $routeSeenRows.ContainsKey("$($row.trial)|$($row.tick)|$otherKind|$($row.object_id)")) {
                throw 'Route/warning surviving projectile lacks its paired collision/cull observation.'
            }
        }
        if ($row.intersects -cne '1') { continue }
        $field = switch -CaseSensitive ($row.kind) {
            'attacker' { 'first_actor_tick' }; 'collision' { 'first_collision_tick' }; 'draw-candidate' { 'first_draw_candidate_tick' }
        }
        if ($field -and ($events[$field] -lt 0 -or [int]$row.tick -lt $events[$field])) { $events[$field] = [int]$row.tick }
    }
    foreach ($field in $events.Keys) {
        if ([int]$trial.$field -ne $events[$field]) { throw "Route/warning trial event disagrees with continuous observations: $field/$($trial.trial)" }
    }
    if ($trial.tool -ceq 'none') {
        if ([int]$trial.strip_losses -ne $losses -or [int]$trial.strip_full_before_commit -ne $before -or
            [int]$trial.strip_full_at_commit -ne $at -or [int]$trial.whole_target_before_commit -ne $wholeBefore) {
            throw 'Route/warning route strip/whole-target commitment observations disagree with the tick evidence.'
        }
    }
    elseif ([int]$trial.paid_tick -lt 0 -or ([int]$trial.first_damage_tick -ge 0 -and
        [int]$trial.first_damage_tick -lt [int]$trial.paid_tick)) { throw 'Route/warning damage/launch chronology is invalid.' }
    if ($trial.tool -cne 'none') {
        $endTick = if ([int]$trial.first_damage_tick -lt 0) { 96 } else { [Math]::Min(96, [int]$trial.first_damage_tick + 6) }
        if ([int]$trial.ticks -ne $endTick -or
            ($events.first_collision_tick -ge 0 -and $events.first_collision_tick -lt $events.paid_tick) -or
            ($events.first_draw_candidate_tick -ge 0 -and $events.first_draw_candidate_tick -lt $events.paid_tick)) {
            throw 'Route/warning single-burst candidate/censoring chronology differs from its recorded inputs.'
        }
        $actorRows = @($rows | Where-Object { $_.kind -ceq 'attacker' })
        $lastCollisionRows = @($rows | Where-Object { $_.kind -ceq 'collision' -and $_.tick -ceq $trial.ticks })
        if ($actorRows.Count -ne $primary.Count -or $lastCollisionRows.Count -ne [int]$trial.live_projectiles_end) {
            throw 'Route/warning actor continuity or final surviving projectile count is incomplete.'
        }
    }
    $loss = [single]([single]$initialHealth - [single]$primary[[string]$trial.ticks].health)
    if ([Math]::Abs([double]$trial.health_loss - $loss) -gt 0.000001) { throw 'Route/warning final health loss differs from its tick evidence.' }
}

$routeDimensions = @{}; $routeFullStems = @{}; $routeCaptureKeys = @{}; $routeCropPixels = 0L
foreach ($row in $routeCaptures) {
    if (-not $routeTrialById.ContainsKey($row.trial) -or $row.capture -cnotmatch '^route-warning-[a-z0-9-]+$' -or
        $row.event -cnotmatch '^[a-z0-9-]+$' -or $row.evidence_scope -cne 'hidden-native-inspection' -or
        $row.frozen_pixels -cne '1' -or $row.public_state -cne '1' -or $row.projectile_state -cne '1' -or $row.rng -cne '1' -or
        $row.debug_fingerprint -cne $(if ($debug) { '1' } else { 'not-available-in-Release' })) {
        throw 'Route/warning capture changed its identity or frozen-draw guards.'
    }
    $trial = $routeTrialById[$row.trial]
    $tick = Get-RouteWarningInteger $row.tick 'capture.tick' 0 ([int]$trial.ticks)
    $view = Get-RouteWarningInteger $row.view 'capture.view' 1 ([int]$trial.players)
    $eventTick = switch -CaseSensitive ($row.event) {
        'walk-entry' { [int]$trial.commit_tick }; 'takeoff' { [int]$trial.commit_tick }
        'first-contact' { [int]$trial.landing_tick }; 'first-geometric-candidate' { [int]$trial.first_draw_candidate_tick }
        'first-health-loss' { [int]$trial.first_damage_tick }
        'censored-end' { if ($trial.first_damage_tick -ceq '-1') { [int]$trial.ticks } else { -1 } }
        default { -1 }
    }
    if ($tick -ne $eventTick -or ($row.event -cne 'walk-entry' -and $row.view -cne $trial.view)) {
        throw 'Route/warning capture is unrelated to its actual recorded event/owner pane.'
    }
    $key = "$($row.capture)|$view"
    if ($routeCaptureKeys.ContainsKey($key) -or $row.players -cne $trial.players -or
        $row.input_slot -cne [string]$routeSlots[$view - 1] -or $row.full_width -cne '1280' -or $row.full_height -cne '720' -or
        $row.capture -cne "route-warning-$($row.trial)-$($row.event)-$tick") { throw 'Route/warning capture roster/stem is invalid or duplicated.' }
    $routeCaptureKeys[$key] = $true
    $pane = @(Get-RouteWarningPane ([int]$row.players) $view)
    if ((@($row.pane_x, $row.pane_y, $row.pane_width, $row.pane_height) -join ',') -cne ($pane -join ',')) {
        throw 'Route/warning capture pane changed its native sparse layout.'
    }
    $routeFullStems[$row.capture] = $true
    $routeDimensions[$row.capture + '-full.ppm'] = @(1280, 720)
    $routeDimensions[$row.capture + '-player' + $view + '.ppm'] = @($pane[2], $pane[3])
    $routeCropPixels += [long]$pane[2] * $pane[3]
}
$routePpms = @(Get-ChildItem -LiteralPath $captures -Filter '*.ppm' -File)
if ($routeFullStems.Count -ne $routeCounts.full_frames -or $routePpms.Count -ne $routeDimensions.Count -or
    $routePpms.Count -ne $routeCounts.full_frames + $routeCounts.native_crops -or
    (($routePpms.Name | Sort-Object) -join '|') -cne (($routeDimensions.Keys | Sort-Object) -join '|')) {
    throw 'Route/warning PPM file set differs from its bounded full-frame/native-crop evidence.'
}
$routePpmBytes = 0L; $routeHeaderBytes = @{}
foreach ($file in $routePpms) {
    $size = $routeDimensions[$file.Name]
    $header = @(Get-Content -LiteralPath $file.FullName -TotalCount 3)
    $expectedHeader = "P6`n$($size[0]) $($size[1])`n255`n"
    $bytes = [Text.Encoding]::ASCII.GetByteCount($expectedHeader) + [long]$size[0] * $size[1] * 3
    if ($header.Count -ne 3 -or $header[0] -cne 'P6' -or $header[1] -cne "$($size[0]) $($size[1])" -or
        $header[2] -cne '255' -or $file.Length -ne $bytes) { throw "Route/warning PPM has invalid dimensions/RGB payload: $($file.Name)" }
    $routePpmBytes += $file.Length
    $routeHeaderBytes[$file.Name] = [Text.Encoding]::ASCII.GetByteCount($expectedHeader)
}
# PPMs contain RGB bytes without alpha or a colour conversion. Reconstruct each
# expected pane row directly from its full-frame bytes, then hash both exact
# byte sequences. Headers and total payload lengths were checked above.
$routeVerifiedCropBytes = 0L
$routeCropHash = [Security.Cryptography.SHA256]::Create()
try {
    foreach ($stem in $routeFullStems.Keys) {
        $fullName = $stem + '-full.ppm'
        $fullBytes = [IO.File]::ReadAllBytes((Join-Path $captures $fullName))
        foreach ($row in @($routeCaptures | Where-Object { $_.capture -ceq $stem })) {
            $cropName = $stem + '-player' + $row.view + '.ppm'
            $cropBytes = [IO.File]::ReadAllBytes((Join-Path $captures $cropName))
            $width = [int]$row.pane_width; $height = [int]$row.pane_height
            $expectedRgb = [byte[]]::new($width * $height * 3)
            for ($y = 0; $y -lt $height; ++$y) {
                $source = $routeHeaderBytes[$fullName] + (([int]$row.pane_y + $y) * 1280 + [int]$row.pane_x) * 3
                [Buffer]::BlockCopy($fullBytes, $source, $expectedRgb, $y * $width * 3, $width * 3)
            }
            $expectedHash = [BitConverter]::ToString($routeCropHash.ComputeHash($expectedRgb))
            $actualHash = [BitConverter]::ToString($routeCropHash.ComputeHash($cropBytes, $routeHeaderBytes[$cropName], $expectedRgb.Length))
            if ($actualHash -cne $expectedHash) { throw "Route/warning native crop RGB bytes differ from its full-frame pane: $cropName" }
            $routeVerifiedCropBytes += $expectedRgb.Length
        }
    }
}
finally { $routeCropHash.Dispose() }
if (-not (Test-Path -LiteralPath (Join-Path $captures 'route-warning-scope.txt') -PathType Leaf)) {
    throw 'Route/warning native scope statement is missing.'
}
$result['observed_counts'] = [ordered]@{
    fixture = $routeSummary; full_frames = $routeCounts.full_frames; native_crops = $routeCounts.native_crops
    tick_rows = $routeTicks.Count; trial_rows = $routeTrials.Count; capture_rows = $routeCaptures.Count
    hud_envelopes = $routePanels.Count; native_crop_pixels = $routeCropPixels; ppm_bytes = $routePpmBytes
    verified_crop_rgb_bytes = $routeVerifiedCropBytes; crop_comparison = 'Exact source-row RGB reconstruction and SHA256 parity; no resampling/colour conversion.'
    independent_geometry_controls = 5; independent_sparse_layout_controls = 2
    interpretation = 'Geometric candidates and conservative HUD envelopes; no human readability or reaction-time acceptance.'
}
