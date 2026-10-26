# Focused private-path mover inspection; no shipped level or manifest is edited.
$moverSummary = Get-Content -LiteralPath (Join-Path $captures 'turbulence-mover-summary.json') -Raw | ConvertFrom-Json
$moverRows = @(Import-Csv -LiteralPath (Join-Path $captures 'turbulence-mover.csv'))
$moverCounts = [ordered]@{ full_frames=33; native_crops=81; csv_rows=81; readbacks=67; level_builds=8 }
$moverKeys = @($moverCounts.Keys)+@('outcome','scope','level_updates','controlled_heavy_contacts','heavy_initial_fraction_min','trial_bytes_unchanged','human_acceptance')
if ((($moverSummary.PSObject.Properties.Name | Sort-Object) -join '|') -cne (($moverKeys | Sort-Object) -join '|') -or
    $moverSummary.outcome -cne 'passed' -or $moverSummary.scope -cne 'turbulence-mover-only' -or
    $moverSummary.trial_bytes_unchanged -isnot [bool] -or -not $moverSummary.trial_bytes_unchanged -or
    $moverSummary.human_acceptance -isnot [bool] -or $moverSummary.human_acceptance -or -not $result.trial_source.unchanged) {
    throw 'Private mover native summary scope/schema/source identity differs.'
}
foreach ($key in $moverCounts.Keys) {
    if (($moverSummary.$key -isnot [int] -and $moverSummary.$key -isnot [long]) -or $moverSummary.$key -ne $moverCounts[$key]) {
        throw "Private mover bounded inventory differs: $key"
    }
}
function Mover-Number([object]$Value,[string]$Label) {
    $number=0.0
    if (-not [double]::TryParse([string]$Value,[Globalization.NumberStyles]::Float,[Globalization.CultureInfo]::InvariantCulture,[ref]$number) -or
        [double]::IsNaN($number) -or [double]::IsInfinity($number)) { throw "Nonfinite/invalid private mover metric: $Label" }
    return $number
}
$moverStage = Get-Content -LiteralPath $trialResolved -Raw | ConvertFrom-Json
$moverPresentation = Get-Content -LiteralPath (Join-Path $gameDirectory 'presentation.json') -Raw | ConvertFrom-Json
$moverTuning = Get-Content -LiteralPath (Join-Path $gameDirectory 'tuning.json') -Raw | ConvertFrom-Json
$moverAtlas = Get-Content -LiteralPath (Join-Path $gameDirectory 'textures/terrain.json') -Raw | ConvertFrom-Json
$moverShadowTile = @($moverAtlas.sprite_frames | Where-Object { $_.name -ceq 'shadow_0' })
$moverObjects = @($moverStage.collision_objects | Where-Object { $_.PSObject.Properties.Name -contains 'motion' })
$moverBody=$moverTuning.player.size; $moverWorld=$moverStage.level_bounds.zoom_out_finish_bounds
$moverBounds=$moverStage.level_bounds.camera_bounds
if ($moverStage.collision_objects.Count -ne 22 -or $moverObjects.Count -ne 2 -or $moverStage.include_in_random -isnot [bool] -or
    $moverStage.include_in_random -or $moverStage.display_name -cne 'Turbulence Yard - Mover Trial' -or
    $moverTuning.player.paint_underfoot.enabled -or $moverShadowTile.Count -ne 1 -or $moverShadowTile[0].size.h -le 0) {
    throw 'Private mover trial/underfoot/shadow resource fixture differs.'
}
for ($side=0; $side -lt 2; ++$side) {
    $owner=$moverObjects[$side]; $anchorX=if($side -eq 0){600}else{2760}; $phase=if($side -eq 0){0.0}else{0.5}
    $ownerName=if($side -eq 0){'mover_a'}else{'mover_b'}
    if ($owner.name -cne $ownerName -or $owner.type -cne 'StructurePaintable' -or $owner.collision_type -cne 'STRUCTURE_JUMP_THROUGH' -or
        $owner.rectangle.x -ne $anchorX -or $owner.rectangle.y -ne 720 -or $owner.rectangle.width -ne 320 -or $owner.rectangle.height -ne 20 -or
        $owner.motion.type -cne 'horizontal' -or $owner.motion.travel_x -ne 320 -or $owner.motion.period -ne 4 -or $owner.motion.phase -ne $phase) {
        throw 'Private mover source does not match the actual-editor-authored pair.'
    }
    $upperName=if($side -eq 0){'upper_approach_a'}else{'upper_approach_b'}
    $crossName=if($side -eq 0){'crossover_a'}else{'crossover_b'}
    $upper=@($moverStage.collision_objects | Where-Object { $_.name -ceq $upperName })
    $cross=@($moverStage.collision_objects | Where-Object { $_.name -ceq $crossName })
    $landingX=if($side -eq 0){1400}else{2360}; $escapeX=if($side -eq 0){1000}else{2840}
    if ($upper.Count -ne 1 -or $cross.Count -ne 1 -or $moverBody.x -gt 160 -or $moverBody.y -ne 120 -or
        680 -ne $cross[0].rectangle.y-$moverBody.y -or $landingX -lt $cross[0].rectangle.x -or
        $landingX+240 -gt $cross[0].rectangle.x+$cross[0].rectangle.width -or $cross[0].rectangle.height -lt 80 -or
        760 -ne $upper[0].rectangle.y-$moverBody.y -or $escapeX -lt $upper[0].rectangle.x -or
        $escapeX+160 -gt $upper[0].rectangle.x+$upper[0].rectangle.width -or $upper[0].rectangle.height -lt 8) {
        throw 'Private mover body/landing/first-safe escape patches differ from their actual authored supports.'
    }
}
$moverUpdates=Mover-Number $moverSummary.level_updates 'updates'
$moverHeavy=Mover-Number $moverSummary.heavy_initial_fraction_min 'initial heavy fraction'
$moverPaintables=@($moverStage.collision_objects | Where-Object { $_.type -ceq 'StructurePaintable' }).Count
if ($moverUpdates -le 0 -or $moverUpdates -gt 6000 -or $moverUpdates -ne [Math]::Truncate($moverUpdates) -or
    $moverSummary.controlled_heavy_contacts -ne 4*$moverPaintables -or $moverHeavy -le 0.99 -or $moverHeavy -gt 1) {
    throw 'Private mover native update/controlled paint budget differs.'
}
$moverColumns='capture,kind,phase,condition,paint,players,view,input_slot,actor_role,scene_views,camera_source,player_x,player_y,velocity_x,velocity_y,carrier_id,mover_id,mover_phase,mover_direction,previous_offset,current_offset,mover_x,mover_y,mover_width,mover_height,shadow_x,shadow_y,shadow_width,shadow_height,local_owned_area,local_credited_area,camera_x,camera_y,camera_scale,pane_x,pane_y,pane_width,pane_height,crop_x,crop_y,crop_width,crop_height,owned_fraction,landing_required,landing_x,landing_y,landing_width,landing_height,landing_visible,escape_x,escape_y,escape_width,escape_height,escape_visible,frozen_pixels,public_state,motion_paint_shadow_state,debug_fingerprint,rng,crop_rgba,evidence_scope'
if ($moverRows.Count -ne 81 -or ($moverRows[0].PSObject.Properties.Name -join ',') -cne $moverColumns) { throw 'Private mover CSV schema/count differs.' }
$moverConditions=@{ 'outer-wait'='wait-not-crossover-commit'; 'inner-commit'='qualified-inward-window'; 'inner-reversal'='carried-direction-reversal'; 'outward-ride'='continuous-carrier-ride' }
$moverExpected=@{}; $moverSlots=@(3,1,2,0)
foreach ($count in @(1,2,3,4)) {
    foreach ($paint in @('clean','heavy')) {
        foreach ($phase in @('outer-wait','inner-commit','inner-reversal','outward-ride')) {
            $stem="hidden-inspection-turbulence-mover-$($count)p-$paint-$phase-720p"
            foreach ($view in 1..$count) { $moverExpected["$stem|$view"]=@('actual-Level-fixture',$phase,$moverConditions[$phase],$paint,[string]$count,[string]$moverSlots[$view-1]) }
        }
    }
}
$moverExpected['hidden-inspection-turbulence-mover-stage-preview-clean-720p|1']=@('actual-StagePreview-isolated','miniature','static-authored-initial-pose','clean','0','-1')
$moverPreview=$moverPresentation.main_menu.stage_select
$moverMenuScale=[single][Math]::Min(1280.0/1920.0,720.0/1080.0)
$moverPreviewX=[single]([single]$moverPreview.preview_position.x*$moverMenuScale)
$moverPreviewY=[single]([single]$moverPreview.preview_position.y*$moverMenuScale)
$moverPreviewWidth=[single]([single]$moverPreview.preview_size.x*$moverMenuScale)
$moverPreviewHeight=[single]([single]$moverPreview.preview_size.y*$moverMenuScale)
$moverPreviewPane=@([int][Math]::Truncate($moverPreviewX),[int][Math]::Truncate($moverPreviewY),
    ([int][Math]::Truncate([single]($moverPreviewX+$moverPreviewWidth))-[int][Math]::Truncate($moverPreviewX)),
    ([int][Math]::Truncate([single]($moverPreviewY+$moverPreviewHeight))-[int][Math]::Truncate($moverPreviewY)))
$moverSeen=@{}; $moverDimensions=@{}; $moverMetrics=@{}; $moverCropPixels=0L
foreach ($row in $moverRows) {
    $key=$row.capture+'|'+$row.view
    if (-not $moverExpected.ContainsKey($key) -or $moverSeen.ContainsKey($key)) { throw "Unexpected/duplicate private mover row: $key" }
    $moverSeen[$key]=$true; $expected=$moverExpected[$key]; $widget=$row.kind -ceq 'actual-StagePreview-isolated'
    $viewIndex=[int]$row.view-1; $side=$viewIndex%2; $rider=-not $widget -and $viewIndex -lt 2
    $role=if($widget){'static-widget'}elseif($rider){'mover-rider'}else{'static-upper-wait'}
    $cameraSource=if($widget){'StagePreview-fit'}else{'actual-Scene'}
    $publicState=if($widget){'bounds-only'}else{'checked'}; $ownerState=if($widget){'not-applicable-widget'}else{'checked'}
    $fingerprint=if($Preset -cne 'x64-debug'){'not-built'}elseif($widget){'not-applicable-widget'}else{'checked'}
    $landingRequired=if($widget -or $row.phase -ceq 'inner-commit'){'1'}else{'0'}
    if ($row.kind -cne $expected[0] -or $row.phase -cne $expected[1] -or $row.condition -cne $expected[2] -or $row.paint -cne $expected[3] -or
        $row.players -cne $expected[4] -or $row.input_slot -cne $expected[5] -or $row.actor_role -cne $role -or $row.scene_views -cne $row.players -or
        $row.camera_source -cne $cameraSource -or $row.frozen_pixels -cne '1' -or $row.public_state -cne $publicState -or
        $row.motion_paint_shadow_state -cne $ownerState -or $row.debug_fingerprint -cne $fingerprint -or $row.rng -cne 'checked' -or
        $row.crop_rgba -cne 'checked' -or $row.landing_required -cne $landingRequired -or $row.escape_visible -cne '1' -or
        $row.landing_visible -cnotin @('0','1') -or ($landingRequired -ceq '1' -and $row.landing_visible -cne '1') -or
        $row.evidence_scope -cne 'controlled-native-inspection-not-human-acceptance') { throw "Private mover fixture/state/visibility scope differs: $key" }
    $n=@{}
    foreach ($metric in @('player_x','player_y','velocity_x','velocity_y','mover_phase','previous_offset','current_offset',
        'mover_x','mover_y','mover_width','mover_height','shadow_x','shadow_y','shadow_width','shadow_height','local_owned_area','local_credited_area',
        'camera_x','camera_y','camera_scale','owned_fraction','landing_x','landing_y','landing_width','landing_height','escape_x','escape_y','escape_width','escape_height')) {
        $n[$metric]=Mover-Number $row.$metric "$key/$metric"
    }
    foreach ($metric in @('carrier_id','mover_id','mover_direction','pane_x','pane_y','pane_width','pane_height','crop_x','crop_y','crop_width','crop_height')) {
        $parsed=0
        if (-not [int]::TryParse($row.$metric,[ref]$parsed) -or ($metric -cne 'mover_direction' -and $parsed -lt 0)) { throw "Invalid private mover integer $key/$metric" }
        $n[$metric]=$parsed
    }
    if ($n.camera_scale -le 0 -or $n.pane_width -le 0 -or $n.pane_height -le 0 -or $n.crop_width -le 0 -or $n.crop_height -le 0 -or
        $n.pane_x+$n.pane_width -gt 1280 -or $n.pane_y+$n.pane_height -gt 720 -or
        ($row.paint -ceq 'clean' -and ($n.owned_fraction -ne 0 -or $n.local_owned_area -ne 0 -or $n.local_credited_area -ne 0)) -or
        ($row.paint -ceq 'heavy' -and ($n.owned_fraction -le 0.99 -or $n.owned_fraction -gt 1 -or $n.local_owned_area -le 0 -or $n.local_credited_area -ne $n.local_owned_area))) {
        throw "Private mover pane/camera/ownership bounds differ: $key"
    }
    $pane=@($n.pane_x,$n.pane_y,$n.pane_width,$n.pane_height); $crop=@($n.crop_x,$n.crop_y,$n.crop_width,$n.crop_height)
    $expectedPane=if($widget){$moverPreviewPane}
        elseif($row.players -ceq '1'){@(0,0,1280,720)}
        elseif($row.players -ceq '2'){@(0,($viewIndex*360),1280,360)}
        # Three-player order is top-left, bottom-right, bottom-left; the
        # top-right quadrant is intentionally unused by player views.
        elseif($row.players -ceq '3'){
            if($viewIndex -eq 0){@(0,0,640,360)}
            elseif($viewIndex -eq 1){@(640,360,640,360)}
            else{@(0,360,640,360)}
        }
        else{@((($viewIndex%2)*640),([int][Math]::Floor($viewIndex/2.0)*360),640,360)}
    if (($pane -join ',') -cne ($expectedPane -join ',') -or ($crop -join ',') -cne ($expectedPane -join ',')) { throw "Private mover crop/pane differs from native layout: $key" }
    $physicalWidth=if($widget){$moverPreviewWidth}else{$n.pane_width}; $physicalHeight=if($widget){$moverPreviewHeight}else{$n.pane_height}
    if ($widget) {
        $scale=[Math]::Min($physicalWidth/[double]$moverWorld.width,$physicalHeight/[double]$moverWorld.height)
        $cameraX=[double]$moverWorld.x-($physicalWidth/$scale-[double]$moverWorld.width)/2.0
        $cameraY=[double]$moverWorld.y-($physicalHeight/$scale-[double]$moverWorld.height)/2.0
        if ([Math]::Abs($n.camera_scale-$scale) -gt 0.000001 -or [Math]::Abs($n.camera_x-$cameraX) -gt 0.002 -or [Math]::Abs($n.camera_y-$cameraY) -gt 0.002) { throw 'Private mover StagePreview camera differs from authored fit.' }
        foreach ($metric in @('player_x','player_y','velocity_x','velocity_y','carrier_id','mover_id','mover_phase','mover_direction','previous_offset','current_offset',
            'mover_x','mover_y','mover_width','mover_height','shadow_x','shadow_y','shadow_width','shadow_height','local_owned_area','local_credited_area')) {
            if ($n[$metric] -ne 0) { throw 'Static StagePreview receipt claims a live mover or player.' }
        }
    } else {
        $owner=$moverObjects[$side]; $anchor=$owner.rectangle
        $expectedDirection=if($n.mover_phase -lt 0.5){1}else{-1}
        $ratio=if($n.mover_phase -le 0.5){$n.mover_phase*2.0}else{(1.0-$n.mover_phase)*2.0}
        $expectedOffset=[single]([double]$owner.motion.travel_x*$ratio)
        $expectedX=[single]([single]$anchor.x+$expectedOffset)
        if ($n.mover_id -ne $side+1 -or $n.mover_phase -lt 0 -or $n.mover_phase -ge 1 -or $n.mover_direction -ne $expectedDirection -or
            [Math]::Abs($n.current_offset-$expectedOffset) -gt 0.0001 -or $n.previous_offset -lt 0 -or $n.previous_offset -gt 320 -or
            [Math]::Abs($n.previous_offset-$n.current_offset) -gt 320.0*2.0/4.0/60.0+0.0001 -or
            [Math]::Abs($n.mover_x-$expectedX) -gt 0.0001 -or $n.mover_y -ne $anchor.y -or $n.mover_width -ne $anchor.width -or $n.mover_height -ne $anchor.height -or
            $n.shadow_x -ne $n.mover_x -or $n.shadow_y -ne $anchor.y+$anchor.height -or $n.shadow_width -ne $anchor.width -or $n.shadow_height -ne $moverShadowTile[0].size.h) {
            throw "Actual private mover phase/collider/owner-bound shadow differs from source: $key"
        }
        # Normalize B's opposite starting phase to the A trajectory for each
        # named capture window. Near-outer can lie on either side of wrapping.
        $phaseA=if($side -eq 0){$n.mover_phase}elseif($n.mover_phase -ge 0.5){$n.mover_phase-0.5}else{$n.mover_phase+0.5}
        $aOffset=if($side -eq 0){$n.current_offset}else{320.0-$n.current_offset}
        if (($row.phase -ceq 'outer-wait' -and $aOffset -ge 16) -or
            ($row.phase -ceq 'inner-commit' -and ($phaseA -lt 0.49 -or $phaseA -ge 0.5)) -or
            ($row.phase -ceq 'inner-reversal' -and ($phaseA -lt 0.5 -or $phaseA -ge 0.505)) -or
            ($row.phase -ceq 'outward-ride' -and ($phaseA -lt 0.745 -or $phaseA -ge 0.755))) { throw "Private mover capture phase differs from its named window: $key" }
        if ($rider) {
            $goal=if($side -eq 0){220.0}else{100.0}
            if ($n.carrier_id -ne $side+1 -or $n.velocity_x -ne 0 -or [Math]::Abs($n.player_y+$moverBody.y-$n.mover_y) -gt 0.06 -or
                $n.player_x -lt $n.mover_x -or $n.player_x+$moverBody.x -gt $n.mover_x+$n.mover_width) { throw "Private mover rider lost actual carrier/top support: $key" }
            if ($landingRequired -ceq '1' -and ([Math]::Abs($n.player_x+$moverBody.x/2.0-$n.mover_x-$goal) -ge 8 -or
                ($side -eq 0 -and $n.mover_x+$n.mover_width -lt 1230) -or ($side -eq 1 -and $n.mover_x -gt 2770))) { throw 'Native landing visibility claims an unqualified inward rider departure.' }
        } else {
            $waitX=if($side -eq 0){1160.0}else{2840.0}
            if ($n.carrier_id -ne 0 -or [Math]::Abs($n.player_x+$moverBody.x/2.0-$waitX) -gt 0.06 -or
                [Math]::Abs($n.player_y+$moverBody.y-880.0) -gt 0.06) { throw 'Extra3P/4P static waiting inspector moved from its declared upper approach.' }
        }
        $scale=720.0/[double]$moverPresentation.camera.reference_height
        if ($moverPresentation.camera.minimum_world_width -gt 0) { $scale=[Math]::Min($scale,$physicalWidth/[double]$moverPresentation.camera.minimum_world_width) }
        $worldWidth=$physicalWidth/$n.camera_scale; $worldHeight=$physicalHeight/$n.camera_scale
        if ([Math]::Abs($n.camera_scale-$scale) -gt 0.000001 -or $n.player_x -lt $n.camera_x-0.02 -or $n.player_y -lt $n.camera_y-0.02 -or
            $n.player_x+$moverBody.x -gt $n.camera_x+$worldWidth+0.02 -or $n.player_y+$moverBody.y -gt $n.camera_y+$worldHeight+0.02 -or
            $n.camera_x -lt $moverBounds.x-0.02 -or $n.camera_x+$worldWidth -gt $moverBounds.x+$moverBounds.width+0.02) { throw "Private mover actual camera/body bounds differ: $key" }
        if ($worldHeight -ge $moverBounds.height) {
            if ([Math]::Abs($n.camera_y-([double]$moverBounds.y+([double]$moverBounds.height-$worldHeight)/2.0)) -gt 0.02) { throw 'Private mover oversized camera is not centered.' }
        } elseif ($n.camera_y -lt $moverBounds.y-0.02 -or $n.camera_y+$worldHeight -gt $moverBounds.y+$moverBounds.height+0.02) { throw 'Private mover camera left its authored vertical bounds.' }
    }
    foreach ($roleName in @('landing','escape')) {
        $patch=if($widget){@([double]$moverWorld.x,[double]$moverWorld.y,[double]$moverWorld.width,[double]$moverWorld.height)}
            elseif($roleName -ceq 'landing'){if($side -eq 0){@(1400,680,240,200)}else{@(2360,680,240,200)}}
            else{if($side -eq 0){@(1000,760,160,128)}else{@(2840,760,160,128)}}
        $parts=@('x','y','width','height')
        for ($part=0; $part -lt 4; ++$part) { if ($n[$roleName+'_'+$parts[$part]] -ne $patch[$part]) { throw "Private mover whole-body warning patch differs: $key/$roleName" } }
        $visible=$patch[0] -ge $n.camera_x-0.002 -and $patch[1] -ge $n.camera_y-0.002 -and
            $patch[0]+$patch[2] -le $n.camera_x+$physicalWidth/$n.camera_scale+0.002 -and $patch[1]+$patch[3] -le $n.camera_y+$physicalHeight/$n.camera_scale+0.002
        if (($roleName -ceq 'escape' -or $landingRequired -ceq '1') -and -not $visible) { throw "Private mover required warning patch is outside its actual camera: $key" }
        if (-not $widget -and ([int]$row.($roleName+'_visible') -ne [int]$visible)) { throw "Private mover visibility flag differs from actual camera/patch geometry: $key/$roleName" }
    }
    $moverDimensions[$row.capture+'.ppm']=@(1280,720)
    $moverDimensions[$row.capture+'-view'+$row.view+'-native.ppm']=@($n.crop_width,$n.crop_height)
    $moverMetrics[$key]=$n; $moverCropPixels += [long]$n.crop_width*$n.crop_height
}
# Independent receipt parity across paint pairs and unchanged local rider offset
# and credited area across all four actual motion phases.
foreach ($row in $moverRows | Where-Object { $_.kind -ceq 'actual-Level-fixture' }) {
    $key=$row.capture+'|'+$row.view; $n=$moverMetrics[$key]
    $outerKey=$row.capture.Replace('-'+$row.phase+'-','-outer-wait-')+'|'+$row.view; $outer=$moverMetrics[$outerKey]
    if ($n.local_owned_area -ne $outer.local_owned_area -or $n.local_credited_area -ne $outer.local_credited_area -or
        ($row.actor_role -ceq 'mover-rider' -and [Math]::Abs(($n.player_x-$n.mover_x)-($outer.player_x-$outer.mover_x)) -gt 0.06)) { throw 'Mover local credit or rider-local standing position changed across captured phases.' }
    if ($row.paint -ceq 'clean') {
        $heavyKey=$row.capture.Replace('-clean-','-heavy-')+'|'+$row.view
        foreach ($metric in @('player_x','player_y','velocity_x','velocity_y','carrier_id','mover_id','mover_phase','mover_direction','previous_offset','current_offset','mover_x','mover_y','mover_width','mover_height')) {
            if ($n[$metric] -ne $moverMetrics[$heavyKey][$metric]) { throw 'Private mover clean/heavy actor/motion pairs differ.' }
        }
    }
}
$moverFiles=@(Get-ChildItem -LiteralPath $captures -Filter '*.ppm' -File)
if ($moverDimensions.Count -ne 114 -or $moverFiles.Count -ne 114 -or (($moverFiles.Name|Sort-Object)-join '|') -cne (($moverDimensions.Keys|Sort-Object)-join '|')) {
    throw 'Private mover leaf requires exactly33 full frames and81 native crops without another painter matrix.'
}
$moverHeaderLengths=@{}; $moverPpmBytes=0L
foreach ($file in $moverFiles) {
    $size=$moverDimensions[$file.Name]; $header="P6`n$($size[0]) $($size[1])`n255`n"; $headerBytes=[Text.Encoding]::ASCII.GetByteCount($header)
    $actual=@(Get-Content -LiteralPath $file.FullName -TotalCount 3)
    if ($file.Length -ne $headerBytes+[long]$size[0]*$size[1]*3 -or $actual.Count -ne 3 -or $actual[0] -cne 'P6' -or $actual[1] -cne "$($size[0]) $($size[1])" -or $actual[2] -cne '255') {
        throw "Private mover PPM dimensions/header/RGB payload differs: $($file.Name)"
    }
    $moverHeaderLengths[$file.Name]=$headerBytes; $moverPpmBytes += $file.Length
}
foreach ($group in $moverRows | Group-Object capture) {
    $fullName=$group.Name+'.ppm'; $full=[IO.File]::ReadAllBytes((Join-Path $captures $fullName))
    foreach ($row in $group.Group) {
        $cropName=$row.capture+'-view'+$row.view+'-native.ppm'; $crop=[IO.File]::ReadAllBytes((Join-Path $captures $cropName))
        $width=[int]$row.crop_width; $height=[int]$row.crop_height
        for ($line=0; $line -lt $height; ++$line) {
            $source=$moverHeaderLengths[$fullName]+(([int]$row.crop_y+$line)*1280+[int]$row.crop_x)*3
            $dest=$moverHeaderLengths[$cropName]+$line*$width*3
            if ([Convert]::ToBase64String($full,$source,$width*3) -cne [Convert]::ToBase64String($crop,$dest,$width*3)) { throw 'Private mover crop RGB differs from its retained full-frame pane.' }
        }
    }
}
$result['observed_counts']=[ordered]@{ fixture=$moverSummary; full_frames=33; native_crops=81; csv_rows=81; readbacks=67
    native_crop_pixels=$moverCropPixels; ppm_bytes=$moverPpmBytes; crop_rgb_parity='checked'; scene_metadata='actual-Scene'; trial_source_hash='unchanged' }
