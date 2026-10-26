# Dot-sourced only for TurbulenceBlockoutOnly, before native contact sheets.
# These are controlled composition fixtures, not completed routes or human play.
$turbulenceSummary = Get-Content -LiteralPath (Join-Path $captures 'turbulence-blockout-summary.json') -Raw | ConvertFrom-Json
$turbulenceRows = @(Import-Csv -LiteralPath (Join-Path $captures 'turbulence-blockout.csv'))
$turbulenceCounts = [ordered]@{ full_frames=21; native_crops=51; csv_rows=51; readbacks=43; level_builds=10; scripted_input_steps=300 }
$turbulenceKeys = @($turbulenceCounts.Keys) + @('outcome','scope','level_updates','controlled_heavy_contacts','heavy_initial_fraction_min','human_acceptance')
if ((($turbulenceSummary.PSObject.Properties.Name | Sort-Object) -join '|') -cne (($turbulenceKeys | Sort-Object) -join '|') -or
    $turbulenceSummary.outcome -cne 'passed' -or $turbulenceSummary.scope -cne 'turbulence-blockout-only' -or
    $turbulenceSummary.human_acceptance -isnot [bool] -or $turbulenceSummary.human_acceptance) { throw 'Turbulence summary scope/schema differs.' }
foreach ($key in $turbulenceCounts.Keys) {
    if (($turbulenceSummary.$key -isnot [int] -and $turbulenceSummary.$key -isnot [long]) -or
        $turbulenceSummary.$key -ne $turbulenceCounts[$key]) { throw "Turbulence bounded count differs: $key" }
}
function Turbulence-Number([object]$Value, [string]$Label) {
    $parsed = 0.0
    if (-not [double]::TryParse([string]$Value, [Globalization.NumberStyles]::Float,
        [Globalization.CultureInfo]::InvariantCulture, [ref]$parsed) -or [double]::IsNaN($parsed) -or [double]::IsInfinity($parsed)) {
        throw "Turbulence metric is not finite: $Label"
    }
    return $parsed
}
$turbulencePresentation = Get-Content -LiteralPath (Join-Path $gameDirectory 'presentation.json') -Raw | ConvertFrom-Json
$turbulenceTuning = Get-Content -LiteralPath (Join-Path $gameDirectory 'tuning.json') -Raw | ConvertFrom-Json
$turbulenceStage = Get-Content -LiteralPath (Join-Path $gameDirectory 'levels/turbulence_blockout.json') -Raw | ConvertFrom-Json
$turbulenceWorld = $turbulenceStage.level_bounds.zoom_out_finish_bounds
$turbulenceBody = $turbulenceTuning.player.size
$turbulenceRectangles = @{}
foreach ($object in $turbulenceStage.collision_objects) {
    if ($object.PSObject.Properties.Name -contains 'rectangle') { $turbulenceRectangles[$object.name] = $object.rectangle }
}
$turbulencePaintables = @($turbulenceStage.collision_objects | Where-Object { $_.type -ceq 'StructurePaintable' }).Count
$turbulenceHeavy = Turbulence-Number $turbulenceSummary.heavy_initial_fraction_min 'heavy initial fraction'
$turbulenceUpdates = Turbulence-Number $turbulenceSummary.level_updates 'updates'
if ($turbulenceSummary.controlled_heavy_contacts -ne 5*$turbulencePaintables -or $turbulenceHeavy -le 0.99 -or $turbulenceHeavy -gt 1.0 -or
    $turbulenceUpdates -le 0 -or $turbulenceUpdates -gt 8000 -or $turbulenceUpdates -ne [Math]::Truncate($turbulenceUpdates) -or
    $turbulenceTuning.player.paint_underfoot.enabled) { throw 'Turbulence ownership/contact/update/underfoot fixture differs.' }
$turbulenceColumns = 'capture,kind,phase,paint,players,view,input_slot,scene_views,camera_source,player_x,player_y,velocity_x,velocity_y,camera_x,camera_y,camera_scale,pane_x,pane_y,pane_width,pane_height,crop_x,crop_y,crop_width,crop_height,owned_fraction,landing,landing_x,landing_y,landing_width,landing_height,landing_visible,escape,escape_x,escape_y,escape_width,escape_height,escape_visible,frozen_pixels,public_state,debug_fingerprint,rng,evidence_scope'
if ($turbulenceRows.Count -ne 51 -or ($turbulenceRows[0].PSObject.Properties.Name -join ',') -cne $turbulenceColumns) { throw 'Turbulence CSV schema/count differs.' }

# A patch names an authored support, left edge, width, cap depth and receipt name.
# Reconstruct whole standing-body space plus the declared visible support cap.
$turbulenceLandings = @{
    'lower-landing-escape' = @(@('lower_bed_a',1260,320,80,'lower_bed_a'), @('lower_bed_b',2420,320,80,'lower_bed_b'),
        @('ground',1740,260,80,'centre_ground_a'), @('ground',2000,260,80,'centre_ground_b'))
    'upper-landing-escape' = @(@('crossover_a',1400,260,80,'crossover_a_near'), @('crossover_b',2340,260,80,'crossover_b_near'),
        @('upper_approach_a',1080,160,80,'upper_approach_a_near'), @('upper_approach_b',2760,160,80,'upper_approach_b_near'))
    'home-exit' = @(@('side_step_a',600,280,80,'side_step_a'), @('side_step_b',3120,280,80,'side_step_b'),
        @('side_step_a',600,280,80,'side_step_a'), @('side_step_b',3120,280,80,'side_step_b'))
}
$turbulenceEscapes = @{
    'lower-landing-escape' = @(@('ground',840,160,80,'yard_a'), @('ground',3000,160,80,'yard_b'),
        @('lower_bed_a',1260,260,80,'lower_bed_a'), @('lower_bed_b',2480,260,80,'lower_bed_b'))
    'upper-landing-escape' = @(@('upper_approach_a',920,160,8,'upper_approach_a_return_cap8'), @('upper_approach_b',2920,160,8,'upper_approach_b_return_cap8'),
        @('crossover_a',1680,200,8,'crossover_a_reversal_cap8'), @('crossover_b',2120,200,8,'crossover_b_reversal_cap8'))
    'home-exit' = @(@('ground',300,260,8,'home_ground_a_cap8'), @('ground',3440,260,8,'home_ground_b_cap8'),
        @('home_floor_a',220,260,80,'home_floor_a'), @('home_floor_b',3520,260,80,'home_floor_b'))
}
function Turbulence-Patch([object[]]$Spec) {
    $support = $turbulenceRectangles[$Spec[0]]
    $left = [double]$Spec[1]; $width = [double]$Spec[2]; $cap = [double]$Spec[3]
    if ($null -eq $support -or $left -lt $support.x -or $left+$width -gt $support.x+$support.width -or
        $width -lt $turbulenceBody.x -or $cap -le 0) { throw 'Turbulence expected warning patch escaped its authored support.' }
    return @{ x=$left; y=([double]$support.y-[double]$turbulenceBody.y); width=$width
        height=([double]$turbulenceBody.y+[Math]::Min([double]$support.height,$cap)) }
}
$turbulenceExpected = @{}; $turbulenceSlots = @(3,1,2,0)
foreach ($count in 1..4) {
    foreach ($paint in @('clean','heavy')) {
        foreach ($phase in @('lower-landing-escape','upper-landing-escape')) {
            $stem = "hidden-inspection-turbulence-blockout-$($count)p-$paint-$phase-720p"
            foreach ($view in 1..$count) {
                $turbulenceExpected["$stem|$view"] = @('actual-Level-fixture',$phase,$paint,[string]$count,[string]$turbulenceSlots[$view-1])
            }
        }
    }
}
foreach ($paint in @('clean','heavy')) {
    $stem = "hidden-inspection-turbulence-blockout-4p-$paint-home-exit-720p"
    foreach ($view in 1..4) { $turbulenceExpected["$stem|$view"] = @('actual-Level-fixture','home-exit',$paint,'4',[string]$turbulenceSlots[$view-1]) }
    $turbulenceExpected["hidden-inspection-turbulence-blockout-overview-$paint-720p|1"] = @('actual-Level-overview','overview',$paint,'1','3')
}
$turbulenceExpected['hidden-inspection-turbulence-blockout-stage-preview-clean-720p|1'] = @('actual-StagePreview-isolated','miniature','clean','0','-1')
$turbulenceLook = $turbulencePresentation.main_menu.stage_select
$turbulenceMenuScale = [single][Math]::Min(1280.0/1920.0,720.0/1080.0)
$turbulencePreviewX = [single]([single]$turbulenceLook.preview_position.x*$turbulenceMenuScale)
$turbulencePreviewY = [single]([single]$turbulenceLook.preview_position.y*$turbulenceMenuScale)
$turbulencePreviewWidth = [single]([single]$turbulenceLook.preview_size.x*$turbulenceMenuScale)
$turbulencePreviewHeight = [single]([single]$turbulenceLook.preview_size.y*$turbulenceMenuScale)
$turbulencePreviewPixels = @([int][Math]::Truncate($turbulencePreviewX),[int][Math]::Truncate($turbulencePreviewY),
    ([int][Math]::Truncate([single]($turbulencePreviewX+$turbulencePreviewWidth))-[int][Math]::Truncate($turbulencePreviewX)),
    ([int][Math]::Truncate([single]($turbulencePreviewY+$turbulencePreviewHeight))-[int][Math]::Truncate($turbulencePreviewY)))
$turbulenceSeen = @{}; $turbulenceDimensions = @{}; $turbulenceMetrics = @{}; $turbulenceCropPixels = 0L
foreach ($row in $turbulenceRows) {
    $key = $row.capture+'|'+$row.view
    if (-not $turbulenceExpected.ContainsKey($key) -or $turbulenceSeen.ContainsKey($key)) { throw "Unexpected/duplicate Turbulence row: $key" }
    $turbulenceSeen[$key] = $true; $expected = $turbulenceExpected[$key]
    $widget = $row.kind -ceq 'actual-StagePreview-isolated'; $overview = $row.kind -ceq 'actual-Level-overview'
    $sceneViews = if ($widget) { '0' } elseif ($overview) { '1' } else { $row.players }
    $cameraSource = if ($widget) { 'StagePreview-fit' } else { 'actual-Scene' }
    $publicState = if ($widget) { 'static-widget-bounds' } else { 'checked' }
    $fingerprint = if ($Preset -cne 'x64-debug') { 'not-built' } elseif ($widget) { 'not-applicable-widget' } else { 'checked' }
    if ($row.kind -cne $expected[0] -or $row.phase -cne $expected[1] -or $row.paint -cne $expected[2] -or
        $row.players -cne $expected[3] -or $row.input_slot -cne $expected[4] -or $row.scene_views -cne $sceneViews -or
        $row.camera_source -cne $cameraSource -or $row.frozen_pixels -cne '1' -or $row.public_state -cne $publicState -or
        $row.debug_fingerprint -cne $fingerprint -or $row.rng -cne 'checked' -or $row.landing_visible -cne '1' -or $row.escape_visible -cne '1' -or
        $row.evidence_scope -cne 'controlled-native-inspection-not-human-acceptance') { throw "Turbulence fixture/state/visibility scope differs: $key" }
    $n = @{}
    foreach ($metric in @('player_x','player_y','velocity_x','velocity_y','camera_x','camera_y','camera_scale','owned_fraction',
        'landing_x','landing_y','landing_width','landing_height','escape_x','escape_y','escape_width','escape_height')) {
        $n[$metric] = Turbulence-Number $row.$metric "$key/$metric"
    }
    foreach ($metric in @('pane_x','pane_y','pane_width','pane_height','crop_x','crop_y','crop_width','crop_height')) {
        $parsed = 0
        if (-not [int]::TryParse($row.$metric,[ref]$parsed) -or $parsed -lt 0) { throw "Invalid Turbulence integer $key/$metric" }
        $n[$metric] = $parsed
    }
    if ($n.camera_scale -le 0 -or $n.pane_width -le 0 -or $n.pane_height -le 0 -or $n.crop_width -le 0 -or $n.crop_height -le 0 -or
        $n.pane_x+$n.pane_width -gt 1280 -or $n.pane_y+$n.pane_height -gt 720 -or $n.crop_x -lt $n.pane_x -or $n.crop_y -lt $n.pane_y -or
        $n.crop_x+$n.crop_width -gt $n.pane_x+$n.pane_width -or $n.crop_y+$n.crop_height -gt $n.pane_y+$n.pane_height -or
        ($row.paint -ceq 'clean' -and $n.owned_fraction -ne 0) -or ($row.paint -ceq 'heavy' -and ($n.owned_fraction -le 0.99 -or $n.owned_fraction -gt 1))) {
        throw "Turbulence crop/camera/ownership bounds differ: $key"
    }
    $viewIndex = [int]$row.view-1
    $pane = @($n.pane_x,$n.pane_y,$n.pane_width,$n.pane_height)
    $expectedPane = if ($widget) { $turbulencePreviewPixels } elseif ($overview -or $row.players -ceq '1') { @(0,0,1280,720) }
        elseif ($row.players -ceq '2') { @(0,($viewIndex*360),1280,360) }
        elseif ($row.players -ceq '3') {
            if ($viewIndex -eq 0) { @(0,0,640,360) } elseif ($viewIndex -eq 1) { @(640,360,640,360) } else { @(0,360,640,360) }
        } else { @((($viewIndex%2)*640),([int][Math]::Floor($viewIndex/2.0)*360),640,360) }
    if (($pane -join ',') -cne ($expectedPane -join ',')) { throw "Turbulence pane differs from staged native layout: $key" }
    $physicalWidth = if ($widget) { $turbulencePreviewWidth } else { $n.pane_width }
    $physicalHeight = if ($widget) { $turbulencePreviewHeight } else { $n.pane_height }
    if ($widget -or $overview) {
        $scale = [Math]::Min($physicalWidth/[double]$turbulenceWorld.width,$physicalHeight/[double]$turbulenceWorld.height)
        $cameraX = [double]$turbulenceWorld.x-($physicalWidth/$scale-[double]$turbulenceWorld.width)/2.0
        $cameraY = [double]$turbulenceWorld.y-($physicalHeight/$scale-[double]$turbulenceWorld.height)/2.0
        if ([Math]::Abs($n.camera_scale-$scale) -gt 0.000001 -or [Math]::Abs($n.camera_x-$cameraX) -gt 0.002 -or [Math]::Abs($n.camera_y-$cameraY) -gt 0.002) {
            throw "Turbulence actual overview/widget camera differs from authored world fit: $key"
        }
    } else {
        $scale = 720.0/[double]$turbulencePresentation.camera.reference_height
        $minimumWidth = [double]$turbulencePresentation.camera.minimum_world_width
        if ($minimumWidth -gt 0) { $scale = [Math]::Min($scale,$physicalWidth/$minimumWidth) }
        if ([Math]::Abs($n.camera_scale-$scale) -gt 0.000001 -or $n.player_x -lt $n.camera_x-0.02 -or $n.player_y -lt $n.camera_y-0.02 -or
            $n.player_x+$turbulenceBody.x -gt $n.camera_x+$physicalWidth/$n.camera_scale+0.02 -or
            $n.player_y+$turbulenceBody.y -gt $n.camera_y+$physicalHeight/$n.camera_scale+0.02) { throw "Turbulence body/gameplay camera fit differs: $key" }
    }
    foreach ($role in @('landing','escape')) {
        $spec = if ($widget -or $overview) { @() } elseif ($role -ceq 'landing') {
            $turbulenceLandings[$row.phase][$viewIndex]
        } else { $turbulenceEscapes[$row.phase][$viewIndex] }
        $patch = if ($widget -or $overview) { $turbulenceWorld } else { Turbulence-Patch $spec }
        $name = if ($widget -or $overview) { 'overview' } else { [string]$spec[4] }
        if ($row.$role -cne $name) { throw "Turbulence patch label differs: $key/$role" }
        foreach ($part in @('x','y','width','height')) {
            if ($n[$role+'_'+$part] -ne $patch.$part) { throw "Turbulence whole-body/support patch differs from staged geometry: $key/$role/$part" }
        }
        if ($n[$role+'_x'] -lt $n.camera_x-0.02 -or $n[$role+'_y'] -lt $n.camera_y-0.02 -or
            $n[$role+'_x']+$n[$role+'_width'] -gt $n.camera_x+$physicalWidth/$n.camera_scale+0.02 -or
            $n[$role+'_y']+$n[$role+'_height'] -gt $n.camera_y+$physicalHeight/$n.camera_scale+0.02) { throw "Turbulence patch is outside its reported camera: $key/$role" }
    }
    $crop = @($n.crop_x,$n.crop_y,$n.crop_width,$n.crop_height)
    $expectedCrop = $expectedPane
    if ($overview) {
        $left = [Math]::Max($n.pane_x,[int][Math]::Floor(($turbulenceWorld.x-$n.camera_x)*$n.camera_scale)+$n.pane_x)
        $top = [Math]::Max($n.pane_y,[int][Math]::Floor(($turbulenceWorld.y-$n.camera_y)*$n.camera_scale)+$n.pane_y)
        $right = [Math]::Min($n.pane_x+$n.pane_width,[int][Math]::Ceiling(($turbulenceWorld.x+$turbulenceWorld.width-$n.camera_x)*$n.camera_scale)+$n.pane_x)
        $bottom = [Math]::Min($n.pane_y+$n.pane_height,[int][Math]::Ceiling(($turbulenceWorld.y+$turbulenceWorld.height-$n.camera_y)*$n.camera_scale)+$n.pane_y)
        $expectedCrop = @($left,$top,($right-$left),($bottom-$top))
    }
    if (($crop -join ',') -cne ($expectedCrop -join ',')) { throw "Turbulence crop does not correspond to its actual camera/pane: $key" }
    $turbulenceDimensions[$row.capture+'.ppm'] = @(1280,720)
    $turbulenceDimensions[$row.capture+'-view'+$row.view+'-native.ppm'] = @($n.crop_width,$n.crop_height)
    $turbulenceMetrics[$key] = $n
    $turbulenceCropPixels += [long]$n.crop_width*$n.crop_height
}
foreach ($row in $turbulenceRows | Where-Object { $_.paint -ceq 'clean' -and $_.kind -ceq 'actual-Level-fixture' }) {
    $cleanKey = $row.capture+'|'+$row.view; $heavyKey = $row.capture.Replace('-clean-','-heavy-')+'|'+$row.view
    foreach ($metric in @('player_x','player_y','velocity_x','velocity_y')) {
        if ($turbulenceMetrics[$cleanKey][$metric] -ne $turbulenceMetrics[$heavyKey][$metric]) { throw 'Turbulence paired clean/heavy actor motion differs.' }
    }
}
$turbulenceFiles = @(Get-ChildItem -LiteralPath $captures -Filter '*.ppm' -File)
if ($turbulenceDimensions.Count -ne 72 -or $turbulenceFiles.Count -ne 72 -or
    (($turbulenceFiles.Name | Sort-Object) -join '|') -cne (($turbulenceDimensions.Keys | Sort-Object) -join '|')) {
    throw 'Turbulence inspection needs exactly21 full frames and51 native crops, without another painter matrix.'
}
$turbulenceHeaderLengths = @{}; $turbulencePpmBytes = 0L
foreach ($file in $turbulenceFiles) {
    $size = $turbulenceDimensions[$file.Name]; $header = "P6`n$($size[0]) $($size[1])`n255`n"
    $headerBytes = [Text.Encoding]::ASCII.GetByteCount($header)
    if ($file.Length -ne $headerBytes+[long]$size[0]*$size[1]*3) { throw "Turbulence RGB file length differs: $($file.Name)" }
    $actualHeader = @(Get-Content -LiteralPath $file.FullName -TotalCount 3)
    if ($actualHeader.Count -ne 3 -or $actualHeader[0] -cne 'P6' -or $actualHeader[1] -cne "$($size[0]) $($size[1])" -or $actualHeader[2] -cne '255') {
        throw "Turbulence native file dimensions/header differ: $($file.Name)"
    }
    $turbulenceHeaderLengths[$file.Name] = $headerBytes; $turbulencePpmBytes += $file.Length
}
# Load one full frame at a time; compare every crop RGB byte without scaling.
foreach ($group in $turbulenceRows | Group-Object capture) {
    $fullName = $group.Name+'.ppm'; $full = [IO.File]::ReadAllBytes((Join-Path $captures $fullName))
    foreach ($row in $group.Group) {
        $cropName = $row.capture+'-view'+$row.view+'-native.ppm'; $crop = [IO.File]::ReadAllBytes((Join-Path $captures $cropName))
        $width = [int]$row.crop_width; $height = [int]$row.crop_height
        for ($line=0; $line -lt $height; ++$line) {
            $source = $turbulenceHeaderLengths[$fullName]+(([int]$row.crop_y+$line)*1280+[int]$row.crop_x)*3
            $dest = $turbulenceHeaderLengths[$cropName]+$line*$width*3
            if ([Convert]::ToBase64String($full,$source,$width*3) -cne [Convert]::ToBase64String($crop,$dest,$width*3)) {
                throw 'Turbulence native crop RGB differs from its full-frame source.'
            }
        }
    }
}
$result['observed_counts'] = [ordered]@{
    fixture=$turbulenceSummary; full_frames=21; native_crops=51; csv_rows=51; readbacks=43
    native_crop_pixels=$turbulenceCropPixels; ppm_bytes=$turbulencePpmBytes; crop_rgb_parity='checked'; scene_metadata='actual-Scene'
}
