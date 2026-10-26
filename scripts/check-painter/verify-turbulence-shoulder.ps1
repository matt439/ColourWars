# Explicit isolated shoulder trial: controlled native stops, no human-route claim.
$shoulderSummary=Get-Content -LiteralPath (Join-Path $captures 'turbulence-shoulder-summary.json') -Raw|ConvertFrom-Json
$shoulderRows=@(Import-Csv -LiteralPath (Join-Path $captures 'turbulence-shoulder.csv'))
$shoulderCounts=[ordered]@{full_frames=25;native_crops=61;csv_rows=61;readbacks=51;level_builds=8;initial_placements=20}
$shoulderKeys=@($shoulderCounts.Keys)+@('outcome','scope','level_updates','controlled_heavy_contacts','heavy_initial_fraction_min','trial_bytes_unchanged','human_acceptance')
if ((($shoulderSummary.PSObject.Properties.Name|Sort-Object)-join '|') -cne (($shoulderKeys|Sort-Object)-join '|') -or
    $shoulderSummary.outcome -cne 'passed' -or $shoulderSummary.scope -cne 'turbulence-shoulder-only' -or
    $shoulderSummary.trial_bytes_unchanged -isnot [bool] -or -not $shoulderSummary.trial_bytes_unchanged -or
    $shoulderSummary.human_acceptance -isnot [bool] -or $shoulderSummary.human_acceptance -or -not $result.trial_source.unchanged) {
    throw 'Shoulder native summary/source scope differs.'
}
foreach($key in $shoulderCounts.Keys){if(($shoulderSummary.$key -isnot [int] -and $shoulderSummary.$key -isnot [long]) -or
    $shoulderSummary.$key -ne $shoulderCounts[$key]){throw "Shoulder native bounded count differs: $key"}}
function Shoulder-Number([object]$Value,[string]$Label){
    $number=0.0
    if(-not [double]::TryParse([string]$Value,[Globalization.NumberStyles]::Float,[Globalization.CultureInfo]::InvariantCulture,[ref]$number) -or
        [double]::IsNaN($number) -or [double]::IsInfinity($number)){throw "Nonfinite/invalid shoulder metric: $Label"}
    return $number
}
$shoulderStage=Get-Content -LiteralPath $trialResolved -Raw|ConvertFrom-Json
$shoulderPresentation=Get-Content -LiteralPath (Join-Path $gameDirectory 'presentation.json') -Raw|ConvertFrom-Json
$shoulderTuning=Get-Content -LiteralPath (Join-Path $gameDirectory 'tuning.json') -Raw|ConvertFrom-Json
$shoulderBody=$shoulderTuning.player.size;$shoulderWorld=$shoulderStage.level_bounds.zoom_out_finish_bounds
$shoulderBounds=$shoulderStage.level_bounds.camera_bounds
if($shoulderStage.collision_objects.Count -ne 26 -or $shoulderStage.display_name -cne 'Turbulence Yard - Shoulder Trial' -or
    $shoulderStage.include_in_random -isnot [bool] -or $shoulderStage.include_in_random -or $shoulderTuning.player.paint_underfoot.enabled -or
    $shoulderBody.x -ne 52 -or $shoulderBody.y -ne 120){throw 'Shoulder source/body/underfoot fixture differs.'}
$shoulderObjects=@{}
foreach($object in $shoulderStage.collision_objects){$shoulderObjects[$object.name]=$object}
$shoulderTriangles=@();$shoulderShoes=@()
for($side=0;$side -lt 2;++$side){
    $suffix=if($side -eq 0){'a'}else{'b'};$ramp=$shoulderObjects['shoulder_ramp_'+$suffix];$shoe=$shoulderObjects['shoulder_shoe_'+$suffix]
    $triangle=$ramp.triangle;$left=if($side -eq 0){1240}else{2600};$tag=if($side -eq 0){'STRUCTURE_RAMP_RIGHT'}else{'STRUCTURE_RAMP_LEFT'}
    $expected=if($side -eq 0){@(1240,880,1400,880,1400,800)}else{@(2760,880,2600,880,2600,800)}
    $actual=@($triangle.x1,$triangle.y1,$triangle.x2,$triangle.y2,$triangle.x3,$triangle.y3)
    if($ramp.type -cne 'StructureRamp' -or $ramp.collision_type -cne $tag -or $ramp.material -cne 'set_square' -or
        ($actual -join ',') -cne ($expected -join ',') -or $shoe.type -cne 'StructurePaintable' -or $shoe.collision_type -cne 'STRUCTURE_PAINTABLE' -or
        $shoe.rectangle.x -ne $left -or $shoe.rectangle.y -ne 880 -or $shoe.rectangle.width -ne 160 -or $shoe.rectangle.height -ne 24){throw 'Shoulder source triangle/shoe geometry differs.'}
    foreach($face in @('left','top','right','bottom')){if($shoe.paintable_faces.$face -isnot [bool] -or -not $shoe.paintable_faces.$face){throw 'Shoulder shoe lost a credited flat face.'}}
    $upper=$shoulderObjects['upper_approach_'+$suffix].rectangle;$cross=$shoulderObjects['crossover_'+$suffix].rectangle
    if($upper.y -ne 880 -or $cross.y -ne 800 -or
        ($side -eq 0 -and ($upper.x+$upper.width -ne $left -or $cross.x -ne $left+160)) -or
        ($side -eq 1 -and ($cross.x+$cross.width -ne $left -or $upper.x -ne $left+160))){throw 'Shoulder triangle does not join its original flat supports.'}
    $shoulderTriangles+=,$triangle;$shoulderShoes+=,$shoe.rectangle
}
$shoulderUpdates=Shoulder-Number $shoulderSummary.level_updates 'updates';$shoulderHeavy=Shoulder-Number $shoulderSummary.heavy_initial_fraction_min 'heavy fraction'
$shoulderPaintables=@($shoulderStage.collision_objects|Where-Object{$_.type -ceq 'StructurePaintable'}).Count
if($shoulderUpdates -le 0 -or $shoulderUpdates -gt 6000 -or $shoulderUpdates -ne [Math]::Truncate($shoulderUpdates) -or
    $shoulderSummary.controlled_heavy_contacts -ne 4*$shoulderPaintables -or $shoulderHeavy -le 0.99 -or $shoulderHeavy -gt 1){throw 'Shoulder update/controlled-paint budget differs.'}
$shoulderColumns='capture,kind,phase,paint,players,view,input_slot,actor_role,scene_views,scene_objects,moving_owners,moving_shadows,camera_source,player_x,player_y,player_width,player_height,centre_x,feet_y,velocity_x,velocity_y,move_state,supported,carrier_id,expected_feet,triangle_x1,triangle_y1,triangle_x2,triangle_y2,triangle_x3,triangle_y3,camera_x,camera_y,camera_scale,pane_x,pane_y,pane_width,pane_height,crop_x,crop_y,crop_width,crop_height,owned_fraction,ramp_x,ramp_y,ramp_width,ramp_height,ramp_visible,flat_x,flat_y,flat_width,flat_height,flat_visible,shoe_x,shoe_y,shoe_width,shoe_height,shoe_visible,frozen_pixels,public_state,full_paint_owner_shadow_state,debug_fingerprint,rng,crop_rgba,evidence_scope'
if($shoulderRows.Count -ne 61 -or ($shoulderRows[0].PSObject.Properties.Name -join ',') -cne $shoulderColumns){throw 'Shoulder CSV schema/count differs.'}
$shoulderExpected=@{};$shoulderSlots=@(3,1,2,0)
foreach($count in 1..4){foreach($paint in @('clean','heavy')){foreach($phase in @('before-descent','mid-ramp-stop','upper-flat-stop')){
    $stem="hidden-inspection-turbulence-shoulder-$($count)p-$paint-$phase-720p"
    foreach($view in 1..$count){$shoulderExpected["$stem|$view"]=@('actual-Level-fixture',$phase,$paint,[string]$count,[string]$shoulderSlots[$view-1])}
}}}
$shoulderExpected['hidden-inspection-turbulence-shoulder-stage-preview-clean-720p|1']=@('actual-StagePreview-isolated','miniature','clean','0','-1')
$shoulderLook=$shoulderPresentation.main_menu.stage_select;$shoulderMenuScale=[single](720.0/1080.0)
$shoulderPreviewX=[single]([single]$shoulderLook.preview_position.x*$shoulderMenuScale);$shoulderPreviewY=[single]([single]$shoulderLook.preview_position.y*$shoulderMenuScale)
$shoulderPreviewWidth=[single]([single]$shoulderLook.preview_size.x*$shoulderMenuScale);$shoulderPreviewHeight=[single]([single]$shoulderLook.preview_size.y*$shoulderMenuScale)
$shoulderPreviewPane=@([int][Math]::Truncate($shoulderPreviewX),[int][Math]::Truncate($shoulderPreviewY),
    ([int][Math]::Truncate([single]($shoulderPreviewX+$shoulderPreviewWidth))-[int][Math]::Truncate($shoulderPreviewX)),
    ([int][Math]::Truncate([single]($shoulderPreviewY+$shoulderPreviewHeight))-[int][Math]::Truncate($shoulderPreviewY)))
$shoulderSeen=@{};$shoulderDimensions=@{};$shoulderMetrics=@{};$shoulderCropPixels=0L
foreach($row in $shoulderRows){
    $key=$row.capture+'|'+$row.view
    if(-not $shoulderExpected.ContainsKey($key) -or $shoulderSeen.ContainsKey($key)){throw "Unexpected/duplicate shoulder row: $key"}
    $shoulderSeen[$key]=$true;$expected=$shoulderExpected[$key];$widget=$row.kind -ceq 'actual-StagePreview-isolated';$viewIndex=[int]$row.view-1;$side=$viewIndex%2
    $role=if($widget){'static-widget'}elseif($viewIndex -lt 2){'shoulder-walker'}else{'static-upper-wait'}
    $public=if($widget){'bounds-only'}else{'checked'};$fullState=if($widget){'not-applicable-widget'}else{'checked'}
    $fingerprint=if($Preset -cne 'x64-debug'){'not-built'}elseif($widget){'not-applicable-widget'}else{'checked'}
    $cameraSource=if($widget){'StagePreview-fit'}else{'actual-Scene'};$objects=if($widget){'0'}else{'26'};$owners=if($widget){'0'}else{'2'}
    if($row.kind -cne $expected[0] -or $row.phase -cne $expected[1] -or $row.paint -cne $expected[2] -or $row.players -cne $expected[3] -or $row.input_slot -cne $expected[4] -or
        $row.actor_role -cne $role -or $row.scene_views -cne $row.players -or $row.scene_objects -cne $objects -or $row.moving_owners -cne $owners -or $row.moving_shadows -cne $owners -or
        $row.camera_source -cne $cameraSource -or $row.frozen_pixels -cne '1' -or $row.public_state -cne $public -or $row.full_paint_owner_shadow_state -cne $fullState -or
        $row.debug_fingerprint -cne $fingerprint -or $row.rng -cne 'checked' -or $row.crop_rgba -cne 'checked' -or $row.ramp_visible -cne '1' -or $row.flat_visible -cne '1' -or $row.shoe_visible -cne '1' -or
        $row.carrier_id -cne '0' -or $row.evidence_scope -cne 'controlled-native-inspection-not-human-acceptance'){throw "Shoulder fixture/state scope differs: $key"}
    $n=@{}
    foreach($metric in @('player_x','player_y','player_width','player_height','centre_x','feet_y','velocity_x','velocity_y','expected_feet',
        'triangle_x1','triangle_y1','triangle_x2','triangle_y2','triangle_x3','triangle_y3','camera_x','camera_y','camera_scale','owned_fraction',
        'ramp_x','ramp_y','ramp_width','ramp_height','flat_x','flat_y','flat_width','flat_height','shoe_x','shoe_y','shoe_width','shoe_height')){$n[$metric]=Shoulder-Number $row.$metric "$key/$metric"}
    foreach($metric in @('move_state','supported','pane_x','pane_y','pane_width','pane_height','crop_x','crop_y','crop_width','crop_height')){
        $parsed=0;if(-not [int]::TryParse($row.$metric,[ref]$parsed) -or ($metric -cne 'move_state' -and $parsed -lt 0)){throw "Invalid shoulder integer $key/$metric"};$n[$metric]=$parsed
    }
    if($n.camera_scale -le 0 -or $n.pane_width -le 0 -or $n.pane_height -le 0 -or $n.crop_width -le 0 -or $n.crop_height -le 0 -or
        ($row.paint -ceq 'clean' -and $n.owned_fraction -ne 0) -or ($row.paint -ceq 'heavy' -and ($n.owned_fraction -le 0.99 -or $n.owned_fraction -gt 1))){throw "Shoulder camera/held-paint bounds differ: $key"}
    $pane=@($n.pane_x,$n.pane_y,$n.pane_width,$n.pane_height);$crop=@($n.crop_x,$n.crop_y,$n.crop_width,$n.crop_height)
    $expectedPane=if($widget){$shoulderPreviewPane}elseif($row.players -ceq '1'){@(0,0,1280,720)}elseif($row.players -ceq '2'){@(0,($viewIndex*360),1280,360)}
        elseif($row.players -ceq '3'){if($viewIndex -eq 0){@(0,0,640,360)}elseif($viewIndex -eq 1){@(640,360,640,360)}else{@(0,360,640,360)}}
        else{@((($viewIndex%2)*640),([int][Math]::Floor($viewIndex/2.0)*360),640,360)}
    if(($pane -join ',') -cne ($expectedPane -join ',') -or ($crop -join ',') -cne ($expectedPane -join ',')){throw "Shoulder actual pane/crop layout differs: $key"}
    $physicalWidth=if($widget){$shoulderPreviewWidth}else{$n.pane_width};$physicalHeight=if($widget){$shoulderPreviewHeight}else{$n.pane_height}
    if($widget){
        $scale=[Math]::Min($physicalWidth/[double]$shoulderWorld.width,$physicalHeight/[double]$shoulderWorld.height)
        $cameraX=[double]$shoulderWorld.x-($physicalWidth/$scale-[double]$shoulderWorld.width)/2.0;$cameraY=[double]$shoulderWorld.y-($physicalHeight/$scale-[double]$shoulderWorld.height)/2.0
        if([Math]::Abs($n.camera_scale-$scale) -gt 0.000001 -or [Math]::Abs($n.camera_x-$cameraX) -gt 0.002 -or [Math]::Abs($n.camera_y-$cameraY) -gt 0.002){throw 'Shoulder StagePreview world-fit camera differs.'}
        foreach($metric in @('player_x','player_y','player_width','player_height','centre_x','feet_y','velocity_x','velocity_y','supported','expected_feet','triangle_x1','triangle_y1','triangle_x2','triangle_y2','triangle_x3','triangle_y3')){if($n[$metric] -ne 0){throw 'Shoulder widget receipt claims live player/triangle state.'}}
        if($n.move_state -ne -1){throw 'Shoulder widget claims a player move state.'}
    }else{
        $triangle=$shoulderTriangles[$side];$shoe=$shoulderShoes[$side];$suffix=if($side -eq 0){'a'}else{'b'}
        foreach($part in @('x1','y1','x2','y2','x3','y3')){if($n['triangle_'+$part] -ne $triangle.$part){throw 'Shoulder triangle receipt differs from actual source vertices.'}}
        $middle=$viewIndex -lt 2 -and $row.phase -ceq 'mid-ramp-stop';$before=$viewIndex -lt 2 -and $row.phase -ceq 'before-descent'
        $goal=if($viewIndex -ge 2){if($side -eq 0){1160.0}else{2840.0}}elseif($middle){if($side -eq 0){1320.0}else{2680.0}}elseif($before){if($side -eq 0){1430.0}else{2570.0}}else{if($side -eq 0){1060.0}else{2940.0}}
        $supportX=if($side -eq 0){$n.player_x+$n.player_width}else{$n.player_x}
        $plane=if($middle){[double]$triangle.y1+($supportX-[double]$triangle.x1)*([double]$triangle.y3-[double]$triangle.y1)/([double]$triangle.x3-[double]$triangle.x1)}elseif($before){800.0}else{880.0}
        if($n.player_width -ne $shoulderBody.x -or $n.player_height -ne $shoulderBody.y -or [Math]::Abs($n.centre_x-($n.player_x+$n.player_width/2.0)) -gt 0.0002 -or
            [Math]::Abs($n.feet_y-($n.player_y+$n.player_height)) -gt 0.0002 -or [Math]::Abs($n.centre_x-$goal) -gt 0.04 -or
            [Math]::Abs($n.feet_y-$plane) -gt 0.06 -or [Math]::Abs($n.expected_feet-$plane) -gt 0.0002 -or $n.velocity_x -ne 0 -or $n.supported -ne 1){throw "Shoulder actual supported stop/triangle body-edge foot-plane differs: $key"}
        if($middle){
            $expectedState=if($side -eq 0){3}else{2};$left=[Math]::Min($triangle.x1,$triangle.x3);$right=[Math]::Max($triangle.x1,$triangle.x3)
            if($n.move_state -ne $expectedState -or $n.player_x -lt $left -or $n.player_x+$n.player_width -gt $right -or [Math]::Abs($n.feet_y-827) -gt 0.08){throw 'Shoulder midpoint is not a whole-body, resting ramp contact.'}
        }else{
            $surface=if($before){$shoulderObjects['crossover_'+$suffix].rectangle}else{$shoulderObjects['upper_approach_'+$suffix].rectangle}
            if($n.move_state -notin @(0,1) -or $n.player_x -lt $surface.x -or $n.player_x+$n.player_width -gt $surface.x+$surface.width){throw 'Shoulder flat stop clips its actual fixed support.'}
        }
        $scale=720.0/[double]$shoulderPresentation.camera.reference_height
        if($shoulderPresentation.camera.minimum_world_width -gt 0){$scale=[Math]::Min($scale,$physicalWidth/[double]$shoulderPresentation.camera.minimum_world_width)}
        $worldWidth=$physicalWidth/$n.camera_scale;$worldHeight=$physicalHeight/$n.camera_scale
        if([Math]::Abs($n.camera_scale-$scale) -gt 0.000001 -or $n.player_x -lt $n.camera_x-0.02 -or $n.player_y -lt $n.camera_y-0.02 -or
            $n.player_x+$n.player_width -gt $n.camera_x+$worldWidth+0.02 -or $n.feet_y -gt $n.camera_y+$worldHeight+0.02 -or
            $n.camera_x -lt $shoulderBounds.x-0.02 -or $n.camera_x+$worldWidth -gt $shoulderBounds.x+$shoulderBounds.width+0.02){throw 'Shoulder whole-body/actual camera bounds differ.'}
        if($worldHeight -ge $shoulderBounds.height){if([Math]::Abs($n.camera_y-([double]$shoulderBounds.y+([double]$shoulderBounds.height-$worldHeight)/2.0)) -gt 0.02){throw 'Shoulder oversized camera is not centered.'}}
        elseif($n.camera_y -lt $shoulderBounds.y-0.02 -or $n.camera_y+$worldHeight -gt $shoulderBounds.y+$shoulderBounds.height+0.02){throw 'Shoulder camera leaves authored vertical bounds.'}
    }
    foreach($roleName in @('ramp','flat','shoe')){
        $goalFlat=if($side -eq 0){1060.0}else{2940.0}
        $patch=if($widget){@([double]$shoulderWorld.x,[double]$shoulderWorld.y,[double]$shoulderWorld.width,[double]$shoulderWorld.height)}
            elseif($roleName -ceq 'ramp'){@([Math]::Min($triangle.x1,$triangle.x3),([Math]::Min($triangle.y1,$triangle.y3)-$shoulderBody.y),160,(80+$shoulderBody.y))}
            elseif($roleName -ceq 'shoe'){@($shoe.x,$shoe.y,$shoe.width,$shoe.height)}
            else{@(($goalFlat-$shoulderBody.x/2.0),(880-$shoulderBody.y),$shoulderBody.x,($shoulderBody.y+8))}
        $parts=@('x','y','width','height');for($part=0;$part -lt 4;++$part){if($n[$roleName+'_'+$parts[$part]] -ne $patch[$part]){throw "Shoulder warning/shoe patch differs from source: $key/$roleName"}}
        if($patch[0] -lt $n.camera_x-0.002 -or $patch[1] -lt $n.camera_y-0.002 -or $patch[0]+$patch[2] -gt $n.camera_x+$physicalWidth/$n.camera_scale+0.002 -or
            $patch[1]+$patch[3] -gt $n.camera_y+$physicalHeight/$n.camera_scale+0.002){throw "Shoulder required ramp/flat/shoe patch is outside actual camera: $key/$roleName"}
    }
    $shoulderDimensions[$row.capture+'.ppm']=@(1280,720);$shoulderDimensions[$row.capture+'-view'+$row.view+'-native.ppm']=@($n.crop_width,$n.crop_height)
    $shoulderMetrics[$key]=$n;$shoulderCropPixels += [long]$n.crop_width*$n.crop_height
}
foreach($row in $shoulderRows|Where-Object{$_.kind -ceq 'actual-Level-fixture' -and $_.paint -ceq 'clean'}){
    $key=$row.capture+'|'+$row.view;$heavyKey=$row.capture.Replace('-clean-','-heavy-')+'|'+$row.view
    foreach($metric in @('player_x','player_y','player_width','player_height','centre_x','feet_y','velocity_x','velocity_y','move_state','supported','expected_feet')){
        if($shoulderMetrics[$key][$metric] -ne $shoulderMetrics[$heavyKey][$metric]){throw 'Shoulder clean/heavy actual public supported motion differs.'}
    }
}

$shoulderFiles=@(Get-ChildItem -LiteralPath $captures -Filter '*.ppm' -File)
if ($shoulderDimensions.Count -ne 86 -or $shoulderFiles.Count -ne 86 -or (($shoulderFiles.Name|Sort-Object)-join '|') -cne (($shoulderDimensions.Keys|Sort-Object)-join '|')) {
    throw 'Shoulder leaf requires exactly25 full frames and61 native crops without another painter matrix.'
}
$shoulderHeaderLengths=@{}; $shoulderPpmBytes=0L
foreach ($file in $shoulderFiles) {
    $size=$shoulderDimensions[$file.Name]; $header="P6`n$($size[0]) $($size[1])`n255`n"; $headerBytes=[Text.Encoding]::ASCII.GetByteCount($header)
    $actual=@(Get-Content -LiteralPath $file.FullName -TotalCount 3)
    if ($file.Length -ne $headerBytes+[long]$size[0]*$size[1]*3 -or $actual.Count -ne 3 -or $actual[0] -cne 'P6' -or $actual[1] -cne "$($size[0]) $($size[1])" -or $actual[2] -cne '255') {
        throw "Shoulder PPM dimensions/header/RGB payload differs: $($file.Name)"
    }
    $shoulderHeaderLengths[$file.Name]=$headerBytes; $shoulderPpmBytes += $file.Length
}
foreach ($group in $shoulderRows | Group-Object capture) {
    $fullName=$group.Name+'.ppm'; $full=[IO.File]::ReadAllBytes((Join-Path $captures $fullName))
    foreach ($row in $group.Group) {
        $cropName=$row.capture+'-view'+$row.view+'-native.ppm'; $crop=[IO.File]::ReadAllBytes((Join-Path $captures $cropName))
        $width=[int]$row.crop_width; $height=[int]$row.crop_height
        for ($line=0; $line -lt $height; ++$line) {
            $source=$shoulderHeaderLengths[$fullName]+(([int]$row.crop_y+$line)*1280+[int]$row.crop_x)*3
            $dest=$shoulderHeaderLengths[$cropName]+$line*$width*3
            if ([Convert]::ToBase64String($full,$source,$width*3) -cne [Convert]::ToBase64String($crop,$dest,$width*3)) { throw 'Shoulder crop RGB differs from its retained full-frame pane.' }
        }
    }
}
$result['observed_counts']=[ordered]@{ fixture=$shoulderSummary; full_frames=25; native_crops=61; csv_rows=61; readbacks=51
    native_crop_pixels=$shoulderCropPixels; ppm_bytes=$shoulderPpmBytes; crop_rgb_parity='checked'; scene_metadata='actual-Scene'; trial_source_hash='unchanged' }
