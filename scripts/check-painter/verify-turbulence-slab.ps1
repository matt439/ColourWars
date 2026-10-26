# Explicit fixed scored slabs: controlled native stops, no human acceptance.
$slabSummary = Get-Content -LiteralPath (Join-Path $captures 'turbulence-slab-summary.json') -Raw | ConvertFrom-Json
$slabRows = @(Import-Csv -LiteralPath (Join-Path $captures 'turbulence-slab.csv'))
$slabCounts = [ordered]@{ full_frames=33; native_crops=75; csv_rows=75; readbacks=67; level_builds=10; initial_placements=20
    wet_frames=4; wet_slab_contacts=8; interior_pixel_candidates=7; interior_pixel_checks=6
    ending_full_frames=4; ending_csv_rows=4; gameplay_csv_rows=71; ending_levels=2 }
$slabKeys = @($slabCounts.Keys) + @('outcome','scope','level_updates','controlled_heavy_contacts','heavy_initial_fraction_min','trial_bytes_unchanged','human_acceptance','friendly_repaint_owner_state','ending_level_updates')
if ((($slabSummary.PSObject.Properties.Name | Sort-Object) -join '|') -cne (($slabKeys | Sort-Object) -join '|') -or
    $slabSummary.outcome -cne 'passed' -or $slabSummary.scope -cne 'turbulence-slab-only' -or
    $slabSummary.trial_bytes_unchanged -isnot [bool] -or -not $slabSummary.trial_bytes_unchanged -or
    $slabSummary.human_acceptance -isnot [bool] -or $slabSummary.human_acceptance -or -not $result.trial_source.unchanged -or
    $slabSummary.friendly_repaint_owner_state -cne 'checked') {
    throw 'Fixed slab native summary/source scope differs.'
}
foreach ($key in $slabCounts.Keys) {
    if (($slabSummary.$key -isnot [int] -and $slabSummary.$key -isnot [long]) -or $slabSummary.$key -ne $slabCounts[$key]) {
        throw "Fixed slab bounded count differs: $key"
    }
}
function Slab-Number([object]$Value, [string]$Label) {
    $number = 0.0
    if (-not [double]::TryParse([string]$Value, [Globalization.NumberStyles]::Float, [Globalization.CultureInfo]::InvariantCulture, [ref]$number) -or
        [double]::IsNaN($number) -or [double]::IsInfinity($number)) { throw "Invalid fixed slab metric: $Label" }
    return $number
}
$slabStage = Get-Content -LiteralPath $trialResolved -Raw | ConvertFrom-Json
$slabPresentation = Get-Content -LiteralPath (Join-Path $gameDirectory 'presentation.json') -Raw | ConvertFrom-Json
$slabTuning = Get-Content -LiteralPath (Join-Path $gameDirectory 'tuning.json') -Raw | ConvertFrom-Json
$slabBody = $slabTuning.player.size
$slabWorld = $slabStage.level_bounds.zoom_out_finish_bounds
$slabBounds = $slabStage.level_bounds.camera_bounds
if ($slabStage.collision_objects.Count -ne 24 -or $slabStage.display_name -cnotin @('Turbulence Yard - Mover Trial','Turbulence Yard - Slab Trial') -or
    $slabStage.include_in_random -isnot [bool] -or $slabStage.include_in_random -or $slabTuning.player.paint_underfoot.enabled -or
    $slabBody.x -ne 52 -or $slabBody.y -ne 120) { throw 'Fixed slab source/body/underfoot fixture differs.' }
$slabObjects = @{}
foreach ($object in $slabStage.collision_objects) {
    if ($slabObjects.ContainsKey($object.name)) { throw 'Fixed slab source has duplicate object names.' }
    $slabObjects[$object.name] = $object
}
$slabGeometry = @()
for ($side = 0; $side -lt 2; ++$side) {
    $suffix = if ($side -eq 0) { 'a' } else { 'b' }
    $object = $slabStage.collision_objects[22 + $side]
    $x = if ($side -eq 0) { 1240.0 } else { 2600.0 }
    $y = if ($side -eq 0) { 880.0 } else { 800.0 }
    $dy = if ($side -eq 0) { -80.0 } else { 80.0 }
    $rise = if ($side -eq 0) { 'right' } else { 'left' }
    if ($object.name -cne ('fixed_slab_' + $suffix) -or $object.type -cne 'FixedSlabPaintable' -or
        $object.collision_type -cne 'STRUCTURE_FIXED_SLAB' -or $object.material -cne 'eraser' -or
        $object.slab.x -ne $x -or $object.slab.y -ne $y -or $object.slab.rise -cne $rise) {
        throw 'Fixed slab source identity/position/orientation differs.'
    }
    foreach ($face in @('left','top','right','bottom')) {
        if ($object.paintable_faces.$face -isnot [bool] -or -not $object.paintable_faces.$face) { throw 'Fixed slab lost a credited face.' }
    }
    # Independent ideal normal extrusion. Actual float OBB geometry must stay
    # within0.002 world units, independently of the emitted bounds.
    $length = [Math]::Sqrt(160.0 * 160.0 + $dy * $dy)
    $nx = -$dy / $length; $ny = 160.0 / $length
    $points = @(@($x,$y), @(($x+160),($y+$dy)),
        @(($x+160+24*$nx),($y+$dy+24*$ny)), @(($x+24*$nx),($y+24*$ny)))
    $left = ($points | ForEach-Object { $_[0] } | Measure-Object -Minimum).Minimum
    $top = ($points | ForEach-Object { $_[1] } | Measure-Object -Minimum).Minimum
    $right = ($points | ForEach-Object { $_[0] } | Measure-Object -Maximum).Maximum
    $bottom = ($points | ForEach-Object { $_[1] } | Measure-Object -Maximum).Maximum
    $slabGeometry += ,@{ x=$x; y=$y; dy=$dy; points=$points; bounds=@($left,$top,($right-$left),($bottom-$top)) }
    $upper = $slabObjects['upper_approach_' + $suffix].rectangle
    $cross = $slabObjects['crossover_' + $suffix].rectangle
    if ($upper.y -ne 880 -or $cross.y -ne 800 -or
        ($side -eq 0 -and ($upper.x+$upper.width -ne $x -or $cross.x -ne $x+160)) -or
        ($side -eq 1 -and ($cross.x+$cross.width -ne $x -or $upper.x -ne $x+160))) {
        throw 'Fixed slab top does not join the original flat supports.'
    }
}
$slabUpdates = Slab-Number $slabSummary.level_updates 'updates'
$slabEndingUpdates = Slab-Number $slabSummary.ending_level_updates 'ending updates'
$slabHeavy = Slab-Number $slabSummary.heavy_initial_fraction_min 'heavy fraction'
$slabPaintables = @($slabStage.collision_objects | Where-Object { $_.type -cin @('StructurePaintable','FixedSlabPaintable') }).Count
if ($slabUpdates -le 0 -or $slabUpdates -gt 8000 -or $slabUpdates -ne [Math]::Truncate($slabUpdates) -or
    $slabEndingUpdates -le 0 -or $slabEndingUpdates -gt 2400 -or $slabEndingUpdates -ne [Math]::Truncate($slabEndingUpdates) -or
    $slabEndingUpdates -ge $slabUpdates -or $slabSummary.controlled_heavy_contacts -ne (5*$slabPaintables+8) -or $slabHeavy -le 0.99 -or $slabHeavy -gt 1) {
    throw 'Fixed slab update/controlled paint budget differs.'
}
$slabColumns = 'capture,kind,phase,paint,players,view,input_slot,actor_role,scene_views,scene_objects,moving_owners,moving_shadows,fixed_slabs,fixed_shadows,camera_source,player_x,player_y,player_width,player_height,centre_x,feet_y,velocity_x,velocity_y,move_state,supported,carrier_id,expected_feet,top_start_x,top_start_y,top_end_x,top_end_y,normal_thickness,slab_p0_x,slab_p0_y,slab_p1_x,slab_p1_y,slab_p2_x,slab_p2_y,slab_p3_x,slab_p3_y,camera_x,camera_y,camera_scale,pane_x,pane_y,pane_width,pane_height,crop_x,crop_y,crop_width,crop_height,owned_fraction,slope_x,slope_y,slope_width,slope_height,slope_visible,flat_x,flat_y,flat_width,flat_height,flat_visible,slab_x,slab_y,slab_width,slab_height,slab_visible,frozen_pixels,public_state,full_paint_owner_shadow_state,debug_fingerprint,rng,crop_rgba,evidence_scope'
if ($slabRows.Count -ne 71 -or ($slabRows[0].PSObject.Properties.Name -join ',') -cne $slabColumns) { throw 'Fixed slab CSV schema/count differs.' }
$slabExpected = @{}; $slabSlots = @(3,1,2,0)
foreach ($count in 1..4) { foreach ($paint in @('clean','heavy')) { foreach ($phase in @('before-descent','mid-slab-stop','upper-flat-stop')) {
    $stem = "hidden-inspection-turbulence-slab-$($count)p-$paint-$phase-720p"
    foreach ($view in 1..$count) { $slabExpected["$stem|$view"] = @('actual-Level-fixture',$phase,$paint,[string]$count,[string]$slabSlots[$view-1]) }
}}}
foreach ($count in 1..4) {
    $stem = "hidden-inspection-turbulence-slab-$($count)p-heavy-mid-slab-wet-720p"
    foreach ($view in 1..$count) { $slabExpected["$stem|$view"] = @('actual-Level-fixture','mid-slab-wet','heavy',[string]$count,[string]$slabSlots[$view-1]) }
}
$slabExpected['hidden-inspection-turbulence-slab-stage-preview-clean-720p|1'] = @('actual-StagePreview-isolated','miniature','clean','0','-1')
$slabLook = $slabPresentation.main_menu.stage_select
$slabMenuScale = [single](720.0/1080.0)
$slabPreviewX = [single]([single]$slabLook.preview_position.x*$slabMenuScale)
$slabPreviewY = [single]([single]$slabLook.preview_position.y*$slabMenuScale)
$slabPreviewWidth = [single]([single]$slabLook.preview_size.x*$slabMenuScale)
$slabPreviewHeight = [single]([single]$slabLook.preview_size.y*$slabMenuScale)
$slabPreviewPane = @([int][Math]::Truncate($slabPreviewX),[int][Math]::Truncate($slabPreviewY),
    ([int][Math]::Truncate([single]($slabPreviewX+$slabPreviewWidth))-[int][Math]::Truncate($slabPreviewX)),
    ([int][Math]::Truncate([single]($slabPreviewY+$slabPreviewHeight))-[int][Math]::Truncate($slabPreviewY)))
$slabSeen = @{}; $slabDimensions = @{}; $slabMetrics = @{}; $slabCropPixels = 0L
foreach ($row in $slabRows) {
    $key = $row.capture+'|'+$row.view
    if (-not $slabExpected.ContainsKey($key) -or $slabSeen.ContainsKey($key)) { throw "Unexpected/duplicate fixed slab row: $key" }
    $slabSeen[$key] = $true; $expected = $slabExpected[$key]
    $widget = $row.kind -ceq 'actual-StagePreview-isolated'; $viewIndex = [int]$row.view-1; $side = $viewIndex%2
    $role = if ($widget) { 'static-widget' } elseif ($viewIndex -lt 2) { 'slab-walker' } else { 'static-upper-wait' }
    $public = if ($widget) { 'bounds-only' } else { 'checked' }
    $fullState = if ($widget) { 'not-applicable-widget' } else { 'checked' }
    $fingerprint = if ($Preset -cne 'x64-debug') { 'not-built' } elseif ($widget) { 'not-applicable-widget' } else { 'checked' }
    $cameraSource = if ($widget) { 'StagePreview-fit' } else { 'actual-Scene' }
    $objects = if ($widget) { '0' } else { '24' }; $owners = if ($widget) { '0' } else { '2' }
    if ($row.kind -cne $expected[0] -or $row.phase -cne $expected[1] -or $row.paint -cne $expected[2] -or
        $row.players -cne $expected[3] -or $row.input_slot -cne $expected[4] -or $row.actor_role -cne $role -or
        $row.scene_views -cne $row.players -or $row.scene_objects -cne $objects -or $row.moving_owners -cne $owners -or
        $row.moving_shadows -cne $owners -or $row.fixed_slabs -cne $owners -or $row.fixed_shadows -cne $owners -or
        $row.camera_source -cne $cameraSource -or $row.frozen_pixels -cne '1' -or $row.public_state -cne $public -or
        $row.full_paint_owner_shadow_state -cne $fullState -or $row.debug_fingerprint -cne $fingerprint -or
        $row.rng -cne 'checked' -or $row.crop_rgba -cne 'checked' -or $row.slope_visible -cne '1' -or
        $row.flat_visible -cne '1' -or $row.slab_visible -cne '1' -or $row.carrier_id -cne '0' -or
        $row.evidence_scope -cne 'controlled-native-inspection-not-human-acceptance') { throw "Fixed slab fixture/state scope differs: $key" }
    $n = @{}
    foreach ($metric in @('player_x','player_y','player_width','player_height','centre_x','feet_y','velocity_x','velocity_y','expected_feet',
        'top_start_x','top_start_y','top_end_x','top_end_y','normal_thickness',
        'slab_p0_x','slab_p0_y','slab_p1_x','slab_p1_y','slab_p2_x','slab_p2_y','slab_p3_x','slab_p3_y',
        'camera_x','camera_y','camera_scale','owned_fraction','slope_x','slope_y','slope_width','slope_height',
        'flat_x','flat_y','flat_width','flat_height','slab_x','slab_y','slab_width','slab_height')) { $n[$metric] = Slab-Number $row.$metric "$key/$metric" }
    foreach ($metric in @('move_state','supported','pane_x','pane_y','pane_width','pane_height','crop_x','crop_y','crop_width','crop_height')) {
        $parsed = 0
        if (-not [int]::TryParse($row.$metric,[ref]$parsed) -or ($metric -cne 'move_state' -and $parsed -lt 0)) { throw "Invalid fixed slab integer $key/$metric" }
        $n[$metric] = $parsed
    }
    if ($n.camera_scale -le 0 -or $n.pane_width -le 0 -or $n.pane_height -le 0 -or $n.crop_width -le 0 -or $n.crop_height -le 0 -or
        ($row.paint -ceq 'clean' -and $n.owned_fraction -ne 0) -or ($row.paint -ceq 'heavy' -and ($n.owned_fraction -le 0.99 -or $n.owned_fraction -gt 1))) { throw "Fixed slab camera/paint bounds differ: $key" }
    $pane = @($n.pane_x,$n.pane_y,$n.pane_width,$n.pane_height); $crop = @($n.crop_x,$n.crop_y,$n.crop_width,$n.crop_height)
    $expectedPane = if ($widget) { $slabPreviewPane } elseif ($row.players -ceq '1') { @(0,0,1280,720) } elseif ($row.players -ceq '2') { @(0,($viewIndex*360),1280,360) }
        elseif ($row.players -ceq '3') { if ($viewIndex -eq 0) { @(0,0,640,360) } elseif ($viewIndex -eq 1) { @(640,360,640,360) } else { @(0,360,640,360) } }
        else { @((($viewIndex%2)*640),([int][Math]::Floor($viewIndex/2.0)*360),640,360) }
    if (($pane -join ',') -cne ($expectedPane -join ',') -or ($crop -join ',') -cne ($expectedPane -join ',')) { throw "Fixed slab actual pane/crop layout differs: $key" }
    $physicalWidth = if ($widget) { $slabPreviewWidth } else { $n.pane_width }
    $physicalHeight = if ($widget) { $slabPreviewHeight } else { $n.pane_height }
    if ($widget) {
        $scale = [Math]::Min($physicalWidth/[double]$slabWorld.width,$physicalHeight/[double]$slabWorld.height)
        $cameraX = [double]$slabWorld.x-($physicalWidth/$scale-[double]$slabWorld.width)/2.0
        $cameraY = [double]$slabWorld.y-($physicalHeight/$scale-[double]$slabWorld.height)/2.0
        if ([Math]::Abs($n.camera_scale-$scale) -gt 0.000001 -or [Math]::Abs($n.camera_x-$cameraX) -gt 0.002 -or [Math]::Abs($n.camera_y-$cameraY) -gt 0.002) { throw 'Fixed slab StagePreview camera differs.' }
        foreach ($metric in @('player_x','player_y','player_width','player_height','centre_x','feet_y','velocity_x','velocity_y','supported','expected_feet',
            'top_start_x','top_start_y','top_end_x','top_end_y','normal_thickness',
            'slab_p0_x','slab_p0_y','slab_p1_x','slab_p1_y','slab_p2_x','slab_p2_y','slab_p3_x','slab_p3_y')) {
            if ($n[$metric] -ne 0) { throw 'Fixed slab widget claims a live player or slab collider.' }
        }
        if ($n.move_state -ne -1) { throw 'Fixed slab widget claims a player move state.' }
    } else {
        $geometry = $slabGeometry[$side]; $suffix = if ($side -eq 0) { 'a' } else { 'b' }
        foreach ($part in @(@('top_start_x',$geometry.x),@('top_start_y',$geometry.y),
            @('top_end_x',($geometry.x+160)),@('top_end_y',($geometry.y+$geometry.dy)))) {
            if ([Math]::Abs($n[$part[0]]-$part[1]) -gt 0.002) { throw 'Fixed slab actual top segment differs from source orientation.' }
        }
        if ($n.normal_thickness -ne 24) { throw 'Fixed slab normal thickness differs.' }
        for ($point = 0; $point -lt 4; ++$point) {
            if ([Math]::Abs($n['slab_p'+$point+'_x']-$geometry.points[$point][0]) -gt 0.002 -or
                [Math]::Abs($n['slab_p'+$point+'_y']-$geometry.points[$point][1]) -gt 0.002) { throw 'Fixed slab actual collider corner differs from independent normal extrusion.' }
        }
        $middle = $viewIndex -lt 2 -and $row.phase -cin @('mid-slab-stop','mid-slab-wet'); $before = $viewIndex -lt 2 -and $row.phase -ceq 'before-descent'
        $goal = if ($viewIndex -ge 2) { if ($side -eq 0) { 1160.0 } else { 2840.0 } } elseif ($middle) { if ($side -eq 0) { 1320.0 } else { 2680.0 } }
            elseif ($before) { if ($side -eq 0) { 1430.0 } else { 2570.0 } } else { if ($side -eq 0) { 1060.0 } else { 2940.0 } }
        $supportX = if ($side -eq 0) { $n.player_x+$n.player_width } else { $n.player_x }
        $plane = if ($middle) { $geometry.y+($supportX-$geometry.x)*$geometry.dy/160.0 } elseif ($before) { 800.0 } else { 880.0 }
        if ($n.player_width -ne $slabBody.x -or $n.player_height -ne $slabBody.y -or
            [Math]::Abs($n.centre_x-($n.player_x+$n.player_width/2.0)) -gt 0.0002 -or [Math]::Abs($n.feet_y-($n.player_y+$n.player_height)) -gt 0.0002 -or
            [Math]::Abs($n.centre_x-$goal) -gt 0.04 -or [Math]::Abs($n.feet_y-$plane) -gt 0.06 -or
            [Math]::Abs($n.expected_feet-$plane) -gt 0.002 -or $n.velocity_x -ne 0 -or $n.supported -ne 1) { throw "Fixed slab supported stop/foot plane differs: $key" }
        if ($middle) {
            $expectedState = if ($side -eq 0) { 3 } else { 2 }
            if ($n.move_state -ne $expectedState -or $n.player_x -lt $geometry.x-0.002 -or
                $n.player_x+$n.player_width -gt $geometry.x+160.002 -or [Math]::Abs($n.feet_y-827) -gt 0.08) { throw 'Fixed slab midpoint lacks whole-body resting support.' }
        } else {
            $surface = if ($before) { $slabObjects['crossover_'+$suffix].rectangle } else { $slabObjects['upper_approach_'+$suffix].rectangle }
            if ($n.move_state -notin @(0,1) -or $n.player_x -lt $surface.x -or $n.player_x+$n.player_width -gt $surface.x+$surface.width) { throw 'Fixed slab flat stop clips actual support.' }
        }
        $scale = 720.0/[double]$slabPresentation.camera.reference_height
        if ($slabPresentation.camera.minimum_world_width -gt 0) { $scale = [Math]::Min($scale,$physicalWidth/[double]$slabPresentation.camera.minimum_world_width) }
        $worldWidth = $physicalWidth/$n.camera_scale; $worldHeight = $physicalHeight/$n.camera_scale
        if ([Math]::Abs($n.camera_scale-$scale) -gt 0.000001 -or $n.player_x -lt $n.camera_x-0.02 -or $n.player_y -lt $n.camera_y-0.02 -or
            $n.player_x+$n.player_width -gt $n.camera_x+$worldWidth+0.02 -or $n.feet_y -gt $n.camera_y+$worldHeight+0.02 -or
            $n.camera_x -lt $slabBounds.x-0.02 -or $n.camera_x+$worldWidth -gt $slabBounds.x+$slabBounds.width+0.02) { throw 'Fixed slab whole-body/camera bounds differ.' }
        if ($worldHeight -ge $slabBounds.height) {
            if ([Math]::Abs($n.camera_y-([double]$slabBounds.y+([double]$slabBounds.height-$worldHeight)/2.0)) -gt 0.02) { throw 'Fixed slab oversized camera is not centered.' }
        } elseif ($n.camera_y -lt $slabBounds.y-0.02 -or $n.camera_y+$worldHeight -gt $slabBounds.y+$slabBounds.height+0.02) { throw 'Fixed slab camera leaves authored vertical bounds.' }
    }
    foreach ($roleName in @('slope','flat','slab')) {
        $goalFlat = if ($side -eq 0) { 1060.0 } else { 2940.0 }
        $patch = if ($widget) { @([double]$slabWorld.x,[double]$slabWorld.y,[double]$slabWorld.width,[double]$slabWorld.height) }
            elseif ($roleName -ceq 'slope') { @($geometry.x,(800-$slabBody.y),160,(80+$slabBody.y)) }
            elseif ($roleName -ceq 'slab') { $geometry.bounds }
            else { @(($goalFlat-$slabBody.x/2.0),(880-$slabBody.y),$slabBody.x,($slabBody.y+8)) }
        $parts = @('x','y','width','height')
        for ($part = 0; $part -lt 4; ++$part) {
            if ([Math]::Abs($n[$roleName+'_'+$parts[$part]]-$patch[$part]) -gt 0.002) { throw "Fixed slab warning/shape patch differs: $key/$roleName" }
        }
        if ($patch[0] -lt $n.camera_x-0.002 -or $patch[1] -lt $n.camera_y-0.002 -or
            $patch[0]+$patch[2] -gt $n.camera_x+$physicalWidth/$n.camera_scale+0.002 -or
            $patch[1]+$patch[3] -gt $n.camera_y+$physicalHeight/$n.camera_scale+0.002) { throw "Fixed slab required patch is outside actual camera: $key/$roleName" }
    }
    $slabDimensions[$row.capture+'.ppm'] = @(1280,720)
    $slabDimensions[$row.capture+'-view'+$row.view+'-native.ppm'] = @($n.crop_width,$n.crop_height)
    $slabMetrics[$key] = $n; $slabCropPixels += [long]$n.crop_width*$n.crop_height
}
foreach ($row in $slabRows | Where-Object { $_.kind -ceq 'actual-Level-fixture' -and $_.paint -ceq 'clean' }) {
    $key = $row.capture+'|'+$row.view; $heavyKey = $row.capture.Replace('-clean-','-heavy-')+'|'+$row.view
    foreach ($metric in @('player_x','player_y','player_width','player_height','centre_x','feet_y','velocity_x','velocity_y','move_state','supported','expected_feet')) {
        if ($slabMetrics[$key][$metric] -ne $slabMetrics[$heavyKey][$metric]) { throw 'Fixed slab clean/heavy actual supported motion differs.' }
    }
}
foreach ($row in $slabRows | Where-Object { $_.phase -ceq 'mid-slab-wet' }) {
    $key = $row.capture+'|'+$row.view
    $settledKey = $row.capture.Replace('-mid-slab-wet-','-mid-slab-stop-')+'|'+$row.view
    foreach ($metric in $slabMetrics[$settledKey].Keys) {
        if ($slabMetrics[$key][$metric] -ne $slabMetrics[$settledKey][$metric]) {
            throw 'Friendly wet slab repaint changed the settled pose, geometry, camera or held area.'
        }
    }
}
$slabEndingRows = @(Import-Csv -LiteralPath (Join-Path $captures 'turbulence-slab-ending.csv'))
$slabEndingColumns = 'capture,kind,paint,players,level_state,scene_views,scene_objects,moving_owners,moving_shadows,fixed_slabs,fixed_shadows,initial_placements,level_updates,duration_seconds,menu_overlay,menu_result_source,team_a_played,team_b_played,total_area,team_a_area,team_b_area,coverage_ratio,winning_team,session_round,session_a_wins,session_b_wins,session_complete,camera_source,camera_x,camera_y,camera_scale,pane_x,pane_y,pane_width,pane_height,crop_x,crop_y,crop_width,crop_height,owned_fraction,frozen_pixels,public_state,full_paint_owner_shadow_state,debug_fingerprint,rng,crop_rgba,evidence_scope'
if ($slabEndingRows.Count -ne 4 -or ($slabEndingRows[0].PSObject.Properties.Name -join ',') -cne $slabEndingColumns) {
    throw 'Fixed slab ending CSV schema/count differs.'
}
$slabEndingExpected = @{}
foreach ($count in @(1,4)) {
    $paint = if ($count -eq 1) { 'clean' } else { 'heavy' }
    foreach ($phase in @('overview','end-menu')) {
        $stem = "hidden-inspection-turbulence-slab-ending-$($count)p-$paint-$phase-720p"
        $slabEndingExpected[$stem] = @($count,$paint,$phase)
    }
}
$slabEndingSeen = @{}; $slabEndingMetrics = @{}
foreach ($row in $slabEndingRows) {
    if (-not $slabEndingExpected.ContainsKey($row.capture) -or $slabEndingSeen.ContainsKey($row.capture)) {
        throw 'Unexpected/duplicate fixed slab ending capture.'
    }
    $slabEndingSeen[$row.capture] = $true
    $expected = $slabEndingExpected[$row.capture]
    $menu = $expected[2] -ceq 'end-menu'
    $kind = if ($menu) { 'actual-EndMenuInitial-derived-result' } else { 'actual-Level-overview' }
    $source = if ($menu) { 'actual-LevelEndInfo-Session' } else { 'not-applicable' }
    $cameraSource = if ($menu) { 'actual-Scene-finished-plus-menu' } else { 'actual-Scene-overview' }
    $fingerprint = if ($Preset -ceq 'x64-debug') { 'checked' } else { 'not-built' }
    if ($row.kind -cne $kind -or $row.paint -cne $expected[1] -or $row.players -cne [string]$expected[0] -or
        $row.level_state -cne $(if ($menu) { '4' } else { '3' }) -or $row.menu_overlay -cne [string][int]$menu -or
        $row.menu_result_source -cne $source -or $row.camera_source -cne $cameraSource -or
        $row.scene_views -cne '1' -or $row.scene_objects -cne '24' -or $row.moving_owners -cne '2' -or
        $row.moving_shadows -cne '2' -or $row.fixed_slabs -cne '2' -or $row.fixed_shadows -cne '2' -or
        $row.initial_placements -cne '0' -or $row.frozen_pixels -cne '1' -or $row.public_state -cne 'checked' -or
        $row.full_paint_owner_shadow_state -cne 'checked' -or $row.debug_fingerprint -cne $fingerprint -or
        $row.rng -cne 'checked' -or $row.crop_rgba -cne 'checked' -or
        $row.evidence_scope -cne 'controlled-native-inspection-not-human-acceptance') {
        throw 'Fixed slab ending result provenance/state scope differs.'
    }
    $n = @{}
    foreach ($metric in @('duration_seconds','total_area','team_a_area','team_b_area','coverage_ratio',
        'camera_x','camera_y','camera_scale','owned_fraction')) { $n[$metric] = Slab-Number $row.$metric ('ending/'+$metric) }
    foreach ($metric in @('level_updates','team_a_played','team_b_played','winning_team','session_round',
        'session_a_wins','session_b_wins','session_complete','pane_x','pane_y','pane_width','pane_height',
        'crop_x','crop_y','crop_width','crop_height')) {
        $value = 0
        if (-not [int]::TryParse($row.$metric,[ref]$value) -or $value -lt 0) { throw 'Invalid fixed slab ending integer.' }
        $n[$metric] = $value
    }
    if ($n.level_updates -le 0 -or $n.level_updates -gt 1200 -or $n.duration_seconds -ne 0.75 -or
        $n.team_a_played -ne 1 -or $n.team_b_played -ne $(if ($expected[0] -eq 4) { 1 } else { 0 }) -or
        $n.total_area -le 0 -or $n.team_a_area -lt 0 -or $n.team_b_area -lt 0 -or
        $n.team_a_area+$n.team_b_area -gt $n.total_area+0.05 -or $n.coverage_ratio -lt 0 -or $n.coverage_ratio -gt 1.000001 -or
        [Math]::Abs($n.coverage_ratio-($n.team_a_area+$n.team_b_area)/$n.total_area) -gt 0.000001 -or
        [Math]::Abs($n.owned_fraction-$n.coverage_ratio) -gt 0.000001 -or
        ($expected[1] -ceq 'clean' -and ($n.team_a_area -ne 0 -or $n.team_b_area -ne 0 -or $n.coverage_ratio -ne 0)) -or
        ($expected[1] -ceq 'heavy' -and $n.coverage_ratio -le 0.99)) { throw 'Fixed slab actual ending update/participation/held score differs.' }
    $winner = if ($n.team_a_area -gt $n.team_b_area) { 0 } elseif ($n.team_b_area -gt $n.team_a_area) { 1 } else { 2 }
    if ($n.winning_team -ne $winner) { throw 'Fixed slab actual ending winner differs from held areas.' }
    if ($menu) {
        $aWins = if ($expected[0] -eq 4 -and $winner -eq 0) { 1 } else { 0 }
        $bWins = if ($expected[0] -eq 4 -and $winner -eq 1) { 1 } else { 0 }
        if ($n.session_round -ne 1 -or $n.session_a_wins -ne $aWins -or $n.session_b_wins -ne $bWins -or
            $n.session_complete -ne 1) { throw 'Fixed slab actual single-round Session ledger differs from the real round result.' }
    } elseif ($n.session_round -ne 0 -or $n.session_a_wins -ne 0 -or $n.session_b_wins -ne 0 -or $n.session_complete -ne 0) {
        throw 'Fixed slab pre-finish overview claims a finished Session result.'
    }
    $pane = @($n.pane_x,$n.pane_y,$n.pane_width,$n.pane_height)
    $crop = @($n.crop_x,$n.crop_y,$n.crop_width,$n.crop_height)
    if (($pane -join ',') -cne '0,0,1280,720' -or ($crop -join ',') -cne '0,0,1280,720') {
        throw 'Fixed slab ending full-native pane/crop differs.'
    }
    $scale = [Math]::Min(1280.0/[double]$slabWorld.width,720.0/[double]$slabWorld.height)
    $cameraX = [double]$slabWorld.x-(1280.0/$scale-[double]$slabWorld.width)/2.0
    $cameraY = [double]$slabWorld.y-(720.0/$scale-[double]$slabWorld.height)/2.0
    if ([Math]::Abs($n.camera_scale-$scale) -gt 0.000001 -or [Math]::Abs($n.camera_x-$cameraX) -gt 0.002 -or
        [Math]::Abs($n.camera_y-$cameraY) -gt 0.002) { throw 'Fixed slab ending source-world fit differs.' }
    foreach ($shape in $slabGeometry) {
        $bounds = $shape.bounds
        if ($bounds[0] -lt $n.camera_x-0.002 -or $bounds[1] -lt $n.camera_y-0.002 -or
            $bounds[0]+$bounds[2] -gt $n.camera_x+1280.0/$n.camera_scale+0.002 -or
            $bounds[1]+$bounds[3] -gt $n.camera_y+720.0/$n.camera_scale+0.002) { throw 'Fixed slab ending camera clips a diagonal solid.' }
    }
    $slabEndingMetrics[$row.capture] = $n
    $slabDimensions[$row.capture+'.ppm'] = @(1280,720)
    $slabDimensions[$row.capture+'-view1-native.ppm'] = @(1280,720)
    $slabCropPixels += [long]1280*720
}
$slabActualEndingUpdates = 0
foreach ($count in @(1,4)) {
    $paint = if ($count -eq 1) { 'clean' } else { 'heavy' }
    $prefix = "hidden-inspection-turbulence-slab-ending-$($count)p-$paint-"
    $overview = $slabEndingMetrics[$prefix+'overview-720p']
    $menu = $slabEndingMetrics[$prefix+'end-menu-720p']
    if ($overview.level_updates -ge $menu.level_updates) { throw 'Fixed slab ending did not advance from overview to actual finish.' }
    foreach ($metric in @('duration_seconds','team_a_played','team_b_played','total_area','team_a_area','team_b_area',
        'coverage_ratio','winning_team','camera_x','camera_y','camera_scale','owned_fraction')) {
        if ($overview[$metric] -ne $menu[$metric]) { throw 'Fixed slab actual finish changed the recorded neutral score or ending camera.' }
    }
    $slabActualEndingUpdates += $menu.level_updates
}
if ($slabActualEndingUpdates -ne $slabEndingUpdates -or
    $slabEndingMetrics['hidden-inspection-turbulence-slab-ending-1p-clean-end-menu-720p'].total_area -ne
    $slabEndingMetrics['hidden-inspection-turbulence-slab-ending-4p-heavy-end-menu-720p'].total_area) {
    throw 'Fixed slab ending totals do not reconcile across the actual two Levels.'
}
$slabFiles = @(Get-ChildItem -LiteralPath $captures -Filter '*.ppm' -File)
if ($slabDimensions.Count -ne 108 -or $slabFiles.Count -ne 108 -or
    (($slabFiles.Name | Sort-Object) -join '|') -cne (($slabDimensions.Keys | Sort-Object) -join '|')) { throw 'Fixed slab leaf requires exactly33 full frames and75 native crops.' }
$slabHeaderLengths = @{}; $slabPpmBytes = 0L
foreach ($file in $slabFiles) {
    $size = $slabDimensions[$file.Name]
    $header = "P6" + [char]10 + "$($size[0]) $($size[1])" + [char]10 + "255" + [char]10
    $headerBytes = [Text.Encoding]::ASCII.GetByteCount($header)
    $actual = @(Get-Content -LiteralPath $file.FullName -TotalCount 3)
    if ($file.Length -ne $headerBytes+[long]$size[0]*$size[1]*3 -or $actual.Count -ne 3 -or
        $actual[0] -cne 'P6' -or $actual[1] -cne "$($size[0]) $($size[1])" -or $actual[2] -cne '255') { throw "Fixed slab PPM header/payload differs: $($file.Name)" }
    $slabHeaderLengths[$file.Name] = $headerBytes; $slabPpmBytes += $file.Length
}
# Retained v1 pinhole candidates are classified from independent world-space
# geometry before inspecting their RGB. The first is outside the diagonal;
# only six actual interior points must retain blue ownership or its black edge.
$slabInteriorStem = 'hidden-inspection-turbulence-slab-1p-heavy-mid-slab-stop-720p'
$slabInteriorName = $slabInteriorStem+'.ppm'
$slabInteriorFull = [IO.File]::ReadAllBytes((Join-Path $captures $slabInteriorName))
$slabInteriorCamera = $slabMetrics[$slabInteriorStem+'|1']
$slabInteriorRows = @(Import-Csv -LiteralPath (Join-Path $captures 'slab-interior-pixels.csv'))
if ($slabInteriorRows.Count -ne 7 -or
    ($slabInteriorRows[0].PSObject.Properties.Name -join ',') -cne 'capture,x,y,world_x,world_y,local_x,local_y,interior,red,green,blue') {
    throw 'Fixed slab interior candidate CSV schema/count differs.'
}
$slabCandidatePoints = @(@(704,334),@(712,334),@(713,336),@(722,336),@(700,340),@(710,342),@(705,344))
$slabInteriorChecks = 0
$slabInteriorLength = [Math]::Sqrt(160.0*160.0+80.0*80.0)
for ($index = 0; $index -lt $slabCandidatePoints.Count; ++$index) {
    $point = $slabCandidatePoints[$index]; $row = $slabInteriorRows[$index]
    if ($row.capture -cne $slabInteriorStem -or $row.x -cne [string]$point[0] -or $row.y -cne [string]$point[1]) {
        throw 'Fixed slab interior candidate identity/order differs.'
    }
    $worldX = $slabInteriorCamera.camera_x+($point[0]+0.5-$slabInteriorCamera.pane_x)/$slabInteriorCamera.camera_scale
    $worldY = $slabInteriorCamera.camera_y+($point[1]+0.5-$slabInteriorCamera.pane_y)/$slabInteriorCamera.camera_scale
    $dx = $worldX-1240.0; $dy = $worldY-880.0
    $localX = ($dx*160.0-$dy*80.0)/$slabInteriorLength
    $localY = ($dx*80.0+$dy*160.0)/$slabInteriorLength
    foreach ($metric in @(@('world_x',$worldX),@('world_y',$worldY),@('local_x',$localX),@('local_y',$localY))) {
        if ([Math]::Abs((Slab-Number $row.($metric[0]) ('interior/'+$metric[0]))-$metric[1]) -gt 0.002) {
            throw 'Fixed slab interior candidate world/local reconstruction differs.'
        }
    }
    $interior = $localX -gt 24.0 -and $localX -lt $slabInteriorLength-24.0 -and $localY -gt 0.0 -and $localY -lt 24.0
    if ($row.interior -cne [string][int]$interior -or $interior -ne ($index -gt 0)) {
        throw 'Fixed slab known boundary/interior candidate classification differs.'
    }
    $offset = $slabHeaderLengths[$slabInteriorName]+($point[1]*1280+$point[0])*3
    $rgb = @([int]$slabInteriorFull[$offset],[int]$slabInteriorFull[$offset+1],[int]$slabInteriorFull[$offset+2])
    $rowRgb = @()
    foreach ($channel in @('red','green','blue')) {
        $value = 0
        if (-not [int]::TryParse($row.$channel,[ref]$value) -or $value -lt 0 -or $value -gt 255) { throw 'Invalid fixed slab candidate RGB channel.' }
        $rowRgb += $value
    }
    if (($rgb -join ',') -cne ($rowRgb -join ',')) { throw 'Fixed slab candidate RGB differs from retained PPM.' }
    if ($interior) {
        if (($rgb -join ',') -cnotin @('6,52,204','0,0,0')) {
            throw "Fixed slab owned interior retains a primer pinhole at $($point[0]),$($point[1]): $($rgb -join ',')"
        }
        ++$slabInteriorChecks
    }
}
if ($slabInteriorChecks -ne 6) { throw 'Fixed slab interior pixel check count differs.' }
$slabEndingCropRows = @($slabEndingRows | ForEach-Object {
    # Ending captures have one explicitly validated fullscreen crop.
    [pscustomobject]@{ capture=$_.capture; view=1; crop_x=[int]$_.crop_x; crop_y=[int]$_.crop_y
        crop_width=[int]$_.crop_width; crop_height=[int]$_.crop_height }
})
$slabAllCropRows = @($slabRows) + $slabEndingCropRows
foreach ($group in $slabAllCropRows | Group-Object capture) {
    $fullName = $group.Name+'.ppm'; $full = [IO.File]::ReadAllBytes((Join-Path $captures $fullName))
    foreach ($row in $group.Group) {
        $cropName = $row.capture+'-view'+$row.view+'-native.ppm'; $crop = [IO.File]::ReadAllBytes((Join-Path $captures $cropName))
        $width = [int]$row.crop_width; $height = [int]$row.crop_height
        for ($line = 0; $line -lt $height; ++$line) {
            $source = $slabHeaderLengths[$fullName]+(([int]$row.crop_y+$line)*1280+[int]$row.crop_x)*3
            $dest = $slabHeaderLengths[$cropName]+$line*$width*3
            if ([Convert]::ToBase64String($full,$source,$width*3) -cne [Convert]::ToBase64String($crop,$dest,$width*3)) { throw 'Fixed slab crop RGB differs from retained full-frame pane.' }
        }
    }
}
$result['observed_counts'] = [ordered]@{ fixture=$slabSummary; full_frames=33; native_crops=75; csv_rows=75; readbacks=67
    native_crop_pixels=$slabCropPixels; ppm_bytes=$slabPpmBytes; crop_rgb_parity='checked'; scene_metadata='actual-Scene'; trial_source_hash='unchanged'
    wet_frames=4; wet_slab_contacts=8; friendly_repaint_owner_state='checked'; interior_pixel_candidates=7; interior_pixel_checks=$slabInteriorChecks
    gameplay_csv_rows=71; ending_csv_rows=4; ending_full_frames=4; ending_levels=2; ending_level_updates=$slabEndingUpdates }
