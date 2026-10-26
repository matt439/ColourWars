# Focused Press Tower evidence. Controlled positions/paint are inspection fixtures.
$pressSummary = Get-Content -LiteralPath (Join-Path $captures 'press-tower-summary.json') -Raw | ConvertFrom-Json
$pressRows = @(Import-Csv -LiteralPath (Join-Path $captures 'press-tower.csv'))
$pressCounts = [ordered]@{ players=4; phases=2; paint_states=2; full_frames=4; native_crops=16; csv_rows=16; readbacks=8; level_builds=2 }
$pressKeys = @($pressCounts.Keys) + @('outcome','scope','level_updates','controlled_heavy_contacts','heavy_initial_fraction_min','human_acceptance')
if ((($pressSummary.PSObject.Properties.Name | Sort-Object) -join '|') -cne (($pressKeys | Sort-Object) -join '|') -or
    $pressSummary.outcome -cne 'passed' -or $pressSummary.scope -cne 'press-tower-only' -or
    $pressSummary.human_acceptance -isnot [bool] -or $pressSummary.human_acceptance) { throw 'Press Tower summary scope/schema differs.' }
foreach ($key in $pressCounts.Keys) {
    if (($pressSummary.$key -isnot [int] -and $pressSummary.$key -isnot [long]) -or $pressSummary.$key -ne $pressCounts[$key]) {
        throw "Press Tower bounded count differs: $key"
    }
}
function PressTower-Number([object]$Value, [string]$Label) {
    $parsed = 0.0
    if (-not [double]::TryParse([string]$Value, [Globalization.NumberStyles]::Float,
        [Globalization.CultureInfo]::InvariantCulture, [ref]$parsed) -or [double]::IsNaN($parsed) -or [double]::IsInfinity($parsed)) {
        throw "Press Tower metric is not finite: $Label"
    }
    return $parsed
}
$pressPresentation = Get-Content -LiteralPath (Join-Path $gameDirectory 'presentation.json') -Raw | ConvertFrom-Json
$pressTuning = Get-Content -LiteralPath (Join-Path $gameDirectory 'tuning.json') -Raw | ConvertFrom-Json
$pressStage = Get-Content -LiteralPath (Join-Path $gameDirectory 'levels/press_tower.json') -Raw | ConvertFrom-Json
$pressRectangles = @{}
foreach ($object in $pressStage.collision_objects) {
    if ($object.PSObject.Properties.Name -contains 'rectangle') { $pressRectangles[$object.name] = $object.rectangle }
}
$pressPaintables = @($pressStage.collision_objects | Where-Object { $_.type -ceq 'StructurePaintable' }).Count
$pressHeavy = PressTower-Number $pressSummary.heavy_initial_fraction_min 'heavy initial fraction'
$pressUpdates = PressTower-Number $pressSummary.level_updates 'updates'
if ($pressSummary.controlled_heavy_contacts -ne $pressPaintables -or $pressHeavy -le 0.99 -or $pressHeavy -gt 1.0 -or
    $pressUpdates -le 0 -or $pressUpdates -gt 1404 -or $pressUpdates -ne [Math]::Truncate($pressUpdates)) {
    throw 'Press Tower ownership/contact/update budget differs.'
}
$pressColumns = 'capture,phase,paint,view,input_slot,station,player_x,player_y,velocity_x,velocity_y,camera_x,camera_y,camera_scale,pane_x,pane_y,pane_width,pane_height,crop_x,crop_y,crop_width,crop_height,owned_fraction,target,target_x,target_y,target_width,target_height,target_visible,frozen_pixels,public_state,debug_fingerprint,rng,evidence_scope'
if ($pressRows.Count -ne 16 -or ($pressRows[0].PSObject.Properties.Name -join ',') -cne $pressColumns) { throw 'Press Tower CSV schema/count differs.' }
$pressExpected = @{}; $pressFiles = @{}; $pressSeen = @{}
$pressSlots = @(3,1,2,0)
$pressStationNames = @(@('stair_1_a','stair_1_b','stair_3_a','stair_3_b'), @('shoulder_a','shoulder_b','press_head_a','press_head_b'))
$pressTargetNames = @(@('stair_2_a','stair_2_b','shoulder_a','shoulder_b'), @('press_head','press_head','shoulder_a','shoulder_b'))
$pressPhases = @('stairs','hoist-exit')
foreach ($paint in @('clean','heavy')) {
    for ($phase=0; $phase -lt 2; $phase++) {
        $stem = 'hidden-inspection-press-tower-4p-' + $paint + '-' + $pressPhases[$phase] + '-720p'
        $pressFiles[$stem + '.ppm'] = @(1280,720)
        for ($view=0; $view -lt 4; $view++) {
            $pressExpected[$stem + '|' + ($view+1)] = @($stem,$pressPhases[$phase],$paint,$view,$pressStationNames[$phase][$view],$pressTargetNames[$phase][$view])
            $pressFiles[$stem + '-view' + ($view+1) + '-native.ppm'] = @(640,360)
        }
    }
}
$pressFingerprint = if ($Preset -ceq 'x64-debug') { 'checked' } else { 'not-built' }
$pressScale = 720.0 / [double]$pressPresentation.camera.reference_height
$pressMinimumWidth = [double]$pressPresentation.camera.minimum_world_width
if ($pressMinimumWidth -gt 0) { $pressScale = [Math]::Min($pressScale,640.0/$pressMinimumWidth) }
foreach ($row in $pressRows) {
    $key = $row.capture + '|' + $row.view
    if (-not $pressExpected.ContainsKey($key) -or $pressSeen.ContainsKey($key)) { throw 'Unexpected/duplicate Press Tower row.' }
    $pressSeen[$key] = $true; $expected = $pressExpected[$key]; $view = [int]$expected[3]
    if ($row.capture -cne $expected[0] -or $row.phase -cne $expected[1] -or $row.paint -cne $expected[2] -or
        $row.station -cne $expected[4] -or $row.target -cne $expected[5] -or [int]$row.input_slot -ne $pressSlots[$view] -or
        $row.target_visible -cne '1' -or $row.frozen_pixels -cne '1' -or $row.public_state -cne '1' -or
        $row.rng -cne '1' -or $row.debug_fingerprint -cne $pressFingerprint -or
        $row.evidence_scope -cne 'controlled-native-inspection-not-human-acceptance') { throw 'Press Tower fixture/state scope differs.' }
    $n = @{}
    foreach ($metric in @('player_x','player_y','velocity_x','velocity_y','camera_x','camera_y','camera_scale',
        'pane_x','pane_y','pane_width','pane_height','crop_x','crop_y','crop_width','crop_height','owned_fraction',
        'target_x','target_y','target_width','target_height')) { $n[$metric] = PressTower-Number $row.$metric $metric }
    $left = if ($view % 2 -eq 1) { 640 } else { 0 }; $top = if ($view -ge 2) { 360 } else { 0 }
    if ($n.pane_x -ne $left -or $n.pane_y -ne $top -or $n.pane_width -ne 640 -or $n.pane_height -ne 360 -or
        $n.crop_x -ne $left -or $n.crop_y -ne $top -or $n.crop_width -ne 640 -or $n.crop_height -ne 360 -or
        [Math]::Abs($n.camera_scale - $pressScale) -gt 0.000001 -or
        ($row.paint -ceq 'clean' -and $n.owned_fraction -ne 0) -or
        ($row.paint -ceq 'heavy' -and ($n.owned_fraction -le 0.99 -or $n.owned_fraction -gt 1))) {
        throw 'Press Tower pane/crop/camera/ownership differs.'
    }
    $target = $pressRectangles[$row.target]
    if ($null -eq $target -or $n.target_x -ne $target.x -or $n.target_y -ne $target.y -or
        $n.target_width -ne $target.width -or $n.target_height -ne $target.height) { throw 'Press Tower target differs from authored geometry.' }
    if ($n.target_x -lt $n.camera_x - 0.02 -or $n.target_y -lt $n.camera_y - 0.02 -or
        $n.target_x + $n.target_width -gt $n.camera_x + 640.0/$n.camera_scale + 0.02 -or
        $n.target_y + $n.target_height -gt $n.camera_y + 360.0/$n.camera_scale + 0.02) { throw 'Press Tower target is outside the reported real camera.' }
    $support = $pressRectangles[$row.station]
    if ($row.station -ceq 'press_head_a' -or $row.station -ceq 'press_head_b') {
        $head = $pressRectangles.press_head; $crown = $pressRectangles.press_crown
        $center = if ($row.station -ceq 'press_head_a') { ([double]$head.x + [double]$crown.x)/2.0 } else { ([double]$crown.x + [double]$crown.width + [double]$head.x + [double]$head.width)/2.0 }
        $expectedX = $center - [double]$pressTuning.player.size.x/2.0; $expectedY = [double]$head.y - [double]$pressTuning.player.size.y
    } else {
        if ($null -eq $support) { throw 'Press Tower controlled support is missing.' }
        $expectedX = [double]$support.x + [double]$support.width/2.0 - [double]$pressTuning.player.size.x/2.0
        $expectedY = [double]$support.y - [double]$pressTuning.player.size.y
    }
    if ([Math]::Abs($n.player_x-$expectedX) -gt 0.02 -or [Math]::Abs($n.player_y-$expectedY) -gt 0.02 -or
        [Math]::Abs($n.velocity_x) -gt 0.02 -or [Math]::Abs($n.velocity_y) -gt 0.02) { throw 'Press Tower player left its controlled standing station.' }
}
$pressNative = @(Get-ChildItem -LiteralPath $captures -Filter '*.ppm' -File)
if ($pressNative.Count -ne 20 -or (($pressNative.Name | Sort-Object) -join '|') -cne (($pressFiles.Keys | Sort-Object) -join '|')) {
    throw 'Press Tower native capture inventory differs.'
}
$pressPayloads = @{}; $pressHeaderLengths = @{}
foreach ($file in $pressNative) {
    $size = $pressFiles[$file.Name]; $bytes = [IO.File]::ReadAllBytes($file.FullName)
    $header = 'P6' + [char]10 + $size[0] + ' ' + $size[1] + [char]10 + '255' + [char]10
    if ($bytes.Length -ne $header.Length + 3*$size[0]*$size[1] -or [Text.Encoding]::ASCII.GetString($bytes,0,$header.Length) -cne $header) {
        throw 'Press Tower native PPM dimensions/payload differ.'
    }
    $pressPayloads[$file.Name] = $bytes; $pressHeaderLengths[$file.Name] = $header.Length
}
foreach ($row in $pressRows) {
    $fullName = $row.capture + '.ppm'; $cropName = $row.capture + '-view' + $row.view + '-native.ppm'
    $full = $pressPayloads[$fullName]; $crop = $pressPayloads[$cropName]
    for ($line=0; $line -lt 360; $line++) {
        $source = $pressHeaderLengths[$fullName] + (([int]$row.crop_y+$line)*1280 + [int]$row.crop_x)*3
        $dest = $pressHeaderLengths[$cropName] + $line*640*3
        if ([Convert]::ToBase64String($full,$source,640*3) -cne [Convert]::ToBase64String($crop,$dest,640*3)) {
            throw 'Press Tower native crop RGB differs from its actual full-frame pane.'
        }
    }
}
$result.press_tower = $pressSummary
