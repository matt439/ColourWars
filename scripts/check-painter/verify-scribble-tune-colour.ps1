# Dot-sourced only after -ScribbleTuneColourOnly native execution.
# Independent literal whole-crop source precision models check retained PPM RGB
# pixel. Native C++ owns RGBA, actual wrong references, repeat and saved resets.
# This is not an independent full Monitor, actual-Level or human art proof.
function Convert-ScribbleLiteralRow([string]$Literal){
    foreach($word in ($Literal.Trim() -split '\s+')){
        $values=@($word.Split(',')|ForEach-Object{[int]$_})
        if($values.Count -ne 4){throw 'Scribble literal rectangle is malformed'}
        ,([int[]]$values)
    }
}
# Approved source-derived half-open edges, not production line calculations.
# Each context keeps row0's28, row1's28 and row2's18 dabs in exact order.
$scribbleDabs=@{
    'yellow-flat'=@(Convert-ScribbleLiteralRow @'
25,356,28,359 27,356,30,359 29,356,31,359 30,356,33,359 32,356,35,359 33,356,36,359 35,356,38,359 37,356,39,359 38,356,41,359 40,356,43,359 42,356,44,359 43,356,46,359 45,356,48,359 46,356,49,359 48,356,51,359 50,356,53,359 51,356,54,359 53,356,56,359 55,356,57,359 56,356,59,359 58,356,61,359 60,356,62,359 61,356,64,359 63,356,66,359 64,356,67,359 66,356,69,359 68,356,70,359 69,356,72,359
25,368,28,371 27,368,30,371 29,368,31,371 30,368,33,371 32,368,35,371 33,368,36,371 35,368,38,371 37,368,39,371 38,368,41,371 40,368,43,371 42,368,44,371 43,368,46,371 45,368,48,371 46,368,49,371 48,368,51,371 50,368,53,371 51,368,54,371 53,368,56,371 55,368,57,371 56,368,59,371 58,368,61,371 60,368,62,371 61,368,64,371 63,368,66,371 64,368,67,371 66,368,69,371 68,368,70,371 69,368,72,371
25,381,28,383 27,381,30,383 29,381,31,383 30,381,33,383 32,381,35,383 33,381,36,383 35,381,38,383 37,381,39,383 38,381,41,383 40,381,43,383 41,381,44,383 43,381,46,383 45,381,47,383 46,381,49,383 48,381,51,383 50,381,52,383 51,381,54,383 53,381,56,383
'@)
    'blue-tilted'=@(Convert-ScribbleLiteralRow @'
25,356,28,359 27,356,30,359 29,356,31,359 30,357,33,359 32,357,35,360 33,357,36,360 35,357,38,360 37,357,39,360 38,357,41,360 40,358,43,360 42,358,44,360 43,358,46,361 45,358,48,361 46,358,49,361 48,358,51,361 50,359,53,361 51,359,54,361 53,359,56,362 55,359,57,362 56,359,59,362 58,359,61,362 60,360,62,362 61,360,64,362 63,360,66,363 64,360,67,363 66,360,69,363 68,360,70,363 69,361,72,363
25,368,28,371 27,369,30,371 29,369,31,371 30,369,33,372 32,369,35,372 33,369,36,372 35,369,38,372 37,369,39,372 38,370,41,372 40,370,43,373 42,370,44,373 43,370,46,373 45,370,48,373 46,370,49,373 48,371,51,373 50,371,53,374 51,371,54,374 53,371,56,374 55,371,57,374 56,371,59,374 58,372,61,374 60,372,62,375 61,372,64,375 63,372,66,375 64,372,67,375 66,372,69,375 68,373,70,375 69,373,72,376
25,381,28,383 27,381,30,384 29,381,31,384 30,381,33,384 32,381,35,384 33,381,36,384 35,382,38,384 37,382,39,384 38,382,41,385 40,382,43,385 41,382,44,385 43,382,46,385 45,383,47,385 46,383,49,385 48,383,51,386 50,383,52,386 51,383,54,386 53,383,56,386
'@)
}
$scribbleContexts=@('yellow-flat','blue-tilted')
$scribbleStates=@('default','rgb','rgba128','alpha0')
$scribblePaper=@{'yellow-flat'=@(255,242,122);'blue-tilted'=@(174,235,255)}
$scribbleTokens=@{'yellow-flat'='FFF27A';'blue-tilted'='AEEBFF'}
$scribblePalette=@{default=@(48,80,192,255);rgb=@(53,188,225,255);rgba128=@(32,96,192,128);alpha0=@(165,50,214,0)}
$scribblePartialOnce=@{'yellow-flat'=@(143,169,157);'blue-tilted'=@(103,165,223)}
$scribblePartialTwice=@{'yellow-flat'=@(87,132,175);'blue-tilted'=@(67,130,207)}
$scribbleSingleCounts=@{'yellow-flat'=@(48,48,24);'blue-tilted'=@(52,54,37)}
$scribbleDoubleCounts=@{'yellow-flat'=@(93,93,38);'blue-tilted'=@(80,82,50)}
$scribbleRowMasks=@{};$scribbleMasks=@{}
foreach($context in $scribbleContexts){
    if($scribbleDabs[$context].Count -ne 74){throw 'Scribble literal dab count differs'}
    $rows=@([int[]]::new(4489),[int[]]::new(4489),[int[]]::new(4489));$mask=[int[]]::new(4489)
    for($dab=0;$dab -lt 74;++$dab){
        $rowIndex=if($dab -lt 28){0}elseif($dab -lt 56){1}else{2};$box=$scribbleDabs[$context][$dab]
        if($box[0] -lt 22 -or $box[1] -lt 349 -or $box[2] -gt 77 -or $box[3] -gt 393 -or
            $box[2] -le $box[0] -or $box[3] -le $box[1]){throw 'Scribble selected rectangle escaped its opaque paper interior'}
        for($y=$box[1];$y -lt $box[3];++$y){for($x=$box[0];$x -lt $box[2];++$x){
            $offset=($y-337)*67+$x-18;$rows[$rowIndex][$offset]++;$mask[$offset]++
        }}
    }
    for($rowIndex=0;$rowIndex -lt 3;++$rowIndex){
        if(@($rows[$rowIndex]|Where-Object{$_ -eq 1}).Count -ne $scribbleSingleCounts[$context][$rowIndex] -or
            @($rows[$rowIndex]|Where-Object{$_ -eq 2}).Count -ne $scribbleDoubleCounts[$context][$rowIndex] -or
            @($rows[$rowIndex]|Where-Object{$_ -gt 2}).Count){throw 'Scribble literal per-row multiplicity differs'}
    }
    if(@($mask|Where-Object{$_ -gt 2}).Count -or @($mask|Where-Object{$_ -gt 0}).Count -ne
        $(if($context -ceq 'yellow-flat'){344}else{355})){throw 'Scribble literal union differs'}
    # Every selected pixel belongs to exactly one row; no row collapse or
    # full-note union can stand in for the correct repeated alpha submissions.
    for($offset=0;$offset -lt 4489;++$offset){
        if(@($rows|Where-Object{$_[$offset] -gt 0}).Count -gt 1){throw 'Scribble rows unexpectedly overlap'}
    }
    $scribbleRowMasks[$context]=$rows;$scribbleMasks[$context]=$mask
}
# Four approved representative blend-input models for this scoped proof.
# They neither identify this device's implementation nor enumerate every
# permitted D3D11 precision/conversion. No per-pixel tolerance is admitted.
$scribblePrecisionModels=@('F_AF','Q_AF','F_AQ','Q_AQ')
function Blend-ScribbleRectangle([byte[]]$Buffer,$Box,$RGB,[double]$ShaderAlpha,[string]$Model){
    $source=[double[]]::new(3)
    for($channel=0;$channel -lt 3;++$channel){
        # Shader premultiplication ALWAYS precedes representative RGB8/alpha8
        # choices. Rounded blend alpha never replaces the shader alpha here.
        $source[$channel]=[double]$RGB[$channel]*$ShaderAlpha
        if($Model.StartsWith('Q_',[StringComparison]::Ordinal)){
            $source[$channel]=[Math]::Floor([Math]::Max(0.0,[Math]::Min(255.0,($source[$channel])))+0.5)
        }
    }
    $blendAlpha=$ShaderAlpha
    if($Model.EndsWith('_AQ',[StringComparison]::Ordinal)){
        $blendAlpha=[Math]::Floor([Math]::Max(0.0,[Math]::Min(255.0,($ShaderAlpha*255.0)))+0.5)/255.0
    }
    for($y=$Box[1];$y -lt $Box[3];++$y){for($x=$Box[0];$x -lt $Box[2];++$x){
        if($x -lt 18 -or $x -ge 85 -or $y -lt 337 -or $y -ge 404){throw 'Scribble literal layer escaped ROI'}
        $offset=(($y-337)*67+$x-18)*3
        for($channel=0;$channel -lt 3;++$channel){
            $value=[Math]::Max(0.0,[Math]::Min(255.0,($source[$channel]+[double]$Buffer[$offset+$channel]*(1-$blendAlpha))))
            $Buffer[$offset+$channel]=[byte][Math]::Floor($value+0.5)
        }
    }}
}
function New-ScribbleExpectedRGB([string]$Context,[string]$State,[string]$Model){
    if($Model -cnotin $scribblePrecisionModels){throw 'Unapproved Scribble precision model'}
    $buffer=[byte[]]::new(13467)
    for($offset=0;$offset -lt 13467;$offset+=3){$buffer[$offset]=32;$buffer[$offset+1]=64;$buffer[$offset+2]=96}
    Blend-ScribbleRectangle $buffer @(22,342,84,403) @(0,0,0) (89.0/255) $Model
    Blend-ScribbleRectangle $buffer @(19,338,80,399) @(0,0,0) 1 $Model
    Blend-ScribbleRectangle $buffer @(22,341,77,396) $scribblePaper[$Context] 1 $Model
    Blend-ScribbleRectangle $buffer @(22,341,77,349) @(0,0,0) .12 $Model
    Blend-ScribbleRectangle $buffer @(24,393,74,396) @(238,238,238) 1 $Model
    $rgba=$scribblePalette[$State];$shaderAlpha=$rgba[3]/255.0
    foreach($box in $scribbleDabs[$Context]){Blend-ScribbleRectangle $buffer $box $rgba $shaderAlpha $Model}
    return ,$buffer
}
function Assert-ScribbleInteger($Object,[string]$Key,[int]$Expected){
    $value=$Object.$Key
    if(($value -isnot [int] -and $value -isnot [long]) -or $value -ne $Expected){throw "Scribble exact integer differs: $Key"}
}
function Assert-ScribbleKeys($Object,$Keys,[string]$Label){
    if((@($Object.PSObject.Properties.Name|Sort-Object)-join '|') -cne (@($Keys|Sort-Object)-join '|')){throw "Scribble exact JSON schema differs: $Label"}
}
$scribbleUpdates=@('local_updates','player_updates','level_updates','state_updates','input_updates','time_updates')
function Assert-ScribbleUpdates($Summary){foreach($key in $scribbleUpdates){Assert-ScribbleInteger $Summary $key 0}}
function Assert-ScribblePairs($Rows,$States,$Contexts){
    $wanted=@(foreach($state in $States){foreach($context in $Contexts){"$state/$context/canvas32-64-96"}})
    $actual=@($Rows|ForEach-Object{"$($_.state)/$($_.context)/$($_.background)"})
    if($Rows.Count -ne $wanted.Count -or (($actual|Sort-Object)-join '|') -cne (($wanted|Sort-Object)-join '|')){throw 'Scribble exact state/context/Canvas pairs differ'}
}
function Assert-ScribbleRowFlags($Row){
    if($Row.paper -cne $scribbleTokens[$Row.context]){throw 'Scribble paired paper differs'}
    foreach($key in @('roi_rgba_match','adoption_match','repeat_match','guard_channels','delta_union','frozen_inputs_resources_rng')){
        if($Row.$key -cne 'true'){throw "Scribble native proof flag differs: $key"}
    }
}
function Assert-ScribbleExpectedChannels([string]$Context,[string]$State,[byte[]]$Expected,[string]$Model){
    for($pixel=0;$pixel -lt 4489;++$pixel){
        $writes=$scribbleMasks[$Context][$pixel]
        if($writes -eq 0){continue}
        $rgb=if($State -ceq 'rgba128'){
            if($writes -eq 1){$scribblePartialOnce[$Context]}elseif($Context -ceq 'yellow-flat' -and $Model.StartsWith('Q_',[StringComparison]::Ordinal)){@(87,132,174)}else{$scribblePartialTwice[$Context]}
        }elseif($State -ceq 'alpha0'){$scribblePaper[$Context]}else{$scribblePalette[$State][0..2]}
        for($channel=0;$channel -lt 3;++$channel){if($Expected[$pixel*3+$channel] -ne $rgb[$channel]){
            throw 'Scribble per-write model differs from explicit selected channel expectations'
        }}
    }
    $header=if($Context -ceq 'yellow-flat'){@(224,213,107)}elseif($Model.EndsWith('_AQ',[StringComparison]::Ordinal)){@(153,206,224)}else{@(153,207,224)}
    $guards=@(
        @{x=18;y=337;rgb=@(32,64,96)},@{x=84;y=403;rgb=@(32,64,96)},
        @{x=30;y=350;rgb=$scribblePaper[$Context]},@{x=30;y=394;rgb=@(238,238,238)},
        @{x=83;y=402;rgb=@(21,42,62)},@{x=20;y=340;rgb=@(0,0,0)},@{x=30;y=345;rgb=$header})
    foreach($guard in $guards){
        $offset=(($guard.y-337)*67+$guard.x-18)*3
        for($channel=0;$channel -lt 3;++$channel){if($Expected[$offset+$channel] -ne $guard.rgb[$channel]){
            throw 'Scribble ordered underlayer model differs from explicit guard channels'
        }}
    }
}
$scribbleSummaryPath=Join-Path $captures 'scribble-colour-consumer-summary.json'
$scribbleRowsPath=Join-Path $captures 'scribble-colour-consumers.csv'
foreach($path in @($scribbleSummaryPath,$scribbleRowsPath,(Join-Path $run 'run.log'),(Join-Path $run 'run.err'))){
    if(!(Test-Path -LiteralPath $path -PathType Leaf)){throw 'Scribble required evidence file is missing'}
}
$scribbleSummary=Get-Content -LiteralPath $scribbleSummaryPath -Raw|ConvertFrom-Json
$scribbleRows=@(Import-Csv -LiteralPath $scribbleRowsPath)
$scribbleCounts=[ordered]@{readbacks=40;csv_rows=8;crops=8;crop_width=67;crop_height=67;crop_pixels=35912;ppm_bytes=107840;
    contexts=2;palettes=4;roi_pixels=4489;ordered_fixed_rectangles=5;selected_dabs_per_context=74;rows_per_note=3;blank_reads=0;
    controls=8;partial_straight_reads=2;partial_squared_reads=2;partial_collapsed_union_reads=2;alpha0_straight_reads=2;
    control_row_discriminations=24;changed_control_pixels=2533;unchanged_control_pixels=33379}
Assert-ScribbleKeys $scribbleSummary (@($scribbleCounts.Keys)+$scribbleUpdates+@('scope','viewport','background','event_fade','raw_reference','whole_frame_reference','roi_half_open')) 'shared'
foreach($key in $scribbleCounts.Keys){Assert-ScribbleInteger $scribbleSummary $key $scribbleCounts[$key]}
Assert-ScribbleUpdates $scribbleSummary
if($scribbleSummary.scope -cne 'consumer-only' -or $scribbleSummary.viewport -cne '1280x720' -or
    $scribbleSummary.background -cne 'canvas32-64-96' -or $scribbleSummary.event_fade -isnot [bool] -or $scribbleSummary.event_fade -ne $false -or
    $scribbleSummary.raw_reference -cne 'complete67x67-ordered-five-plus74-literal-rectangles-only' -or
    $scribbleSummary.whole_frame_reference -cne 'copied-production-front-consumer-adoption-only' -or
    $scribbleSummary.roi_half_open -isnot [array] -or $scribbleSummary.roi_half_open.Count -ne 4 -or
    @($scribbleSummary.roi_half_open|Where-Object{$_ -isnot [int] -and $_ -isnot [long]}).Count -or
    ($scribbleSummary.roi_half_open-join ',') -cne '18,337,85,404'){throw 'Scribble shared oracle/ROI/scope differs'}
Assert-ScribblePairs $scribbleRows $scribbleStates $scribbleContexts
$scribbleColumns=@('state','context','background','paper','roi_rgba_match','adoption_match','repeat_match','guard_channels','delta_union',
    'frozen_inputs_resources_rng','controls','control_row_discriminations','changed_control_pixels','unchanged_control_pixels','scope')
if(($scribbleRows[0].PSObject.Properties.Name-join '|') -cne ($scribbleColumns-join '|')){throw 'Scribble shared CSV columns differ'}
$scribbleControlTotal=0;$scribbleRowTotal=0;$scribbleChangedTotal=0;$scribbleUnchangedTotal=0
$scribbleControlMembership=[ordered]@{}
foreach($context in $scribbleContexts){
    $scribbleControlMembership[$context]=@(for($rowIndex=0;$rowIndex -lt 3;++$rowIndex){
        $single=$scribbleSingleCounts[$context][$rowIndex];$double=$scribbleDoubleCounts[$context][$rowIndex]
        [ordered]@{row=$rowIndex;selected=$single+$double;single=$single;double=$double;
            partial_straight_changed=$single+$double;partial_squared_changed=$single+$double;
            collapsed_changed=$double;collapsed_selected_unchanged=$single;alpha0_straight_changed=$single+$double}
    })
}
foreach($row in $scribbleRows){
    Assert-ScribbleRowFlags $row
    if($row.scope -cne 'consumer-only'){throw 'Scribble shared row scope differs'}
    $selected=@($scribbleMasks[$row.context]|Where-Object{$_ -gt 0}).Count
    $double=@($scribbleMasks[$row.context]|Where-Object{$_ -eq 2}).Count
    $controls=0;$discriminations=0;$changed=0;$unchanged=0
    if($row.state -ceq 'rgba128'){$controls=3;$discriminations=9;$changed=2*$selected+$double;$unchanged=3*4489-$changed}
    elseif($row.state -ceq 'alpha0'){$controls=1;$discriminations=3;$changed=$selected;$unchanged=4489-$selected}
    if($row.controls -cne [string]$controls -or $row.control_row_discriminations -cne [string]$discriminations -or
        $row.changed_control_pixels -cne [string]$changed -or $row.unchanged_control_pixels -cne [string]$unchanged){
        throw 'Scribble per-case row/multiplicity/union control counts differ'
    }
    $scribbleControlTotal+=$controls;$scribbleRowTotal+=$discriminations;$scribbleChangedTotal+=$changed;$scribbleUnchangedTotal+=$unchanged
}
if($scribbleControlTotal -ne 8 -or $scribbleRowTotal -ne 24 -or $scribbleChangedTotal -ne 2533 -or $scribbleUnchangedTotal -ne 33379){
    throw 'Scribble control totals differ'
}
if(@(Get-ChildItem -LiteralPath $captures -Filter 'scribble-failure-*' -File).Count){throw 'Scribble failure artifacts remain'}
$scribbleCrops=@(Get-ChildItem -LiteralPath $captures -Filter 'scribble-colour-*.ppm' -File|Sort-Object Name)
$scribbleNames=@(foreach($context in $scribbleContexts){foreach($state in $scribbleStates){"scribble-colour-$context-$state.ppm"}})
if($scribbleCrops.Count -ne 8 -or (($scribbleCrops.Name|Sort-Object)-join '|') -cne (($scribbleNames|Sort-Object)-join '|')){throw 'Scribble exact retained crop cases differ'}
$scribblePPMHeader=[Text.Encoding]::ASCII.GetBytes("P6`n67 67`n255`n")
$scribbleBytes=0;$scribbleCheckedRGBPixels=0
$scribbleCropEvidence=@(foreach($crop in $scribbleCrops){
    $context=@($scribbleContexts|Where-Object{$crop.Name.StartsWith("scribble-colour-$_-",[StringComparison]::Ordinal)})
    if($context.Count -ne 1){throw 'Scribble crop context differs'}
    $state=$crop.BaseName.Substring(("scribble-colour-$($context[0])-").Length)
    $bytes=[IO.File]::ReadAllBytes($crop.FullName)
    if($bytes.Length -ne 13480){throw 'Scribble native crop byte length differs'}
    for($i=0;$i -lt $scribblePPMHeader.Length;++$i){if($bytes[$i] -ne $scribblePPMHeader[$i]){throw 'Scribble canonical P6 header differs'}}
    $matchingModels=[Collections.Generic.List[string]]::new()
    $modelEvidence=@();$firstDifferences=@()
    foreach($modelName in $scribblePrecisionModels){
        $expected=New-ScribbleExpectedRGB $context[0] $state $modelName
        Assert-ScribbleExpectedChannels $context[0] $state $expected $modelName
        $equal=$true;$differentOffset=-1
        for($offset=0;$offset -lt 13467;++$offset){
            if($bytes[$scribblePPMHeader.Length+$offset] -ne $expected[$offset]){
                $equal=$false;$differentOffset=$offset;break
            }
        }
        if($equal){$matchingModels.Add($modelName)}
        else{$firstDifferences+=[ordered]@{model=$modelName;byte_offset=$differentOffset}}
        $digest=Get-PainterSha256 $expected
        $modelEvidence+=[pscustomobject]@{model=$modelName;source_payload_sha256=$digest;whole_crop_match=$equal}
    }
    if($matchingModels.Count -eq 0){
        $details=($firstDifferences|ForEach-Object{"$($_.model):$($_.byte_offset)"})-join ';'
        throw "Scribble retained RGB ROI differs: $($crop.Name) matches no whole frozen source model ($details); per-pixel model mixing is forbidden"
    }
    $equivalence=@(foreach($group in ($modelEvidence|Group-Object source_payload_sha256)){
        [ordered]@{models=@($group.Group.model);source_payload_sha256=$group.Name;whole_crop_match=($group.Group[0].whole_crop_match)}
    })
    $scribbleCheckedRGBPixels+=4489
    # PNG conversion normally follows this verifier in the outer runner.
    # Existing PNGs get header/dimension validation only, never new pixel proof.
    $pngPath=[IO.Path]::ChangeExtension($crop.FullName,'.png')
    if(Test-Path -LiteralPath $pngPath -PathType Leaf){
        $png=[IO.File]::ReadAllBytes($pngPath);$signature=@(137,80,78,71,13,10,26,10)
        if($png.Length -lt 33){throw 'Scribble PNG header is truncated'}
        for($i=0;$i -lt 8;++$i){if($png[$i] -ne $signature[$i]){throw 'Scribble PNG signature differs'}}
        if([Text.Encoding]::ASCII.GetString($png,12,4) -cne 'IHDR'){throw 'Scribble PNG lacks initial IHDR'}
        $pngWidth=0L;$pngHeight=0L
        for($i=0;$i -lt 4;++$i){$pngWidth=$pngWidth*256+$png[16+$i];$pngHeight=$pngHeight*256+$png[20+$i]}
        if($pngWidth -ne 67 -or $pngHeight -ne 67){throw 'Scribble PNG dimensions differ'}
    }
    $scribbleBytes+=$bytes.Length
    [ordered]@{name=$crop.Name;width=67;height=67;bytes=$bytes.Length;sha256=(Get-FileHash -LiteralPath $crop.FullName).Hash;
        matching_source_models=@($matchingModels);source_model_equivalence_classes=$equivalence;
        admission='one exact whole-crop representative model; never per-pixel mixing'}
})
if($scribbleBytes -ne 107840 -or $scribbleCheckedRGBPixels -ne 35912){throw 'Scribble retained RGB pixel/PPM byte totals differ'}
$scribbleLog=@(Get-Content -LiteralPath (Join-Path $run 'run.log'))
$scribbleSaves=@($scribbleLog|Where-Object{$_ -match 'Saving\.\.\.$'}).Count
$scribbleSuccess=@($scribbleLog|Where-Object{$_ -ceq 'Save was successful.'}).Count
if(@(Get-Content -LiteralPath (Join-Path $run 'run.err')|Where-Object{$_ -match '^save failure:'}).Count){throw 'Unexpected Scribble private Save failure'}
$scribbleArtifacts=[ordered]@{}
foreach($path in @($scribbleSummaryPath,$scribbleRowsPath)){$scribbleArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}}
$scribbleArtifacts.crops=$scribbleCropEvidence
$scribbleObserved=[ordered]@{consumer=$scribbleSummary;readbacks=40;csv_rows=8;crops=8;controls=8;control_row_discriminations=24;
    changed_control_pixels=2533;unchanged_control_pixels=33379;crop_pixels=35912;ppm_bytes=$scribbleBytes;
    retained_rgb_pixels=$scribbleCheckedRGBPixels;retained_rgb_payload_bytes=107736;
    retained_rgb_scope='independent-literal-complete67x67-whole-source-model; native owns RGBA';
    source_precision_models=@($scribblePrecisionModels);
    source_precision_scope='four representative models only, not universal D3D11 conformance or device identification';
    precision_admission='one complete exact model per crop; all matching names/equivalence recorded; no per-pixel mixing or blanket tolerance';
    expected_control_row_membership=$scribbleControlMembership;
    control_scope='literal per-row source membership pins counters; native compares actual wrong-reference RGBA frames';
    local_updates=0;player_updates=0;level_updates=0;state_updates=0;input_updates=0;time_updates=0;
    private_save_calls=$scribbleSaves;successful_private_saves=$scribbleSuccess;failed_private_saves=0;artifacts=$scribbleArtifacts}
if($debug){
    $scribbleTuneSummaryPath=Join-Path $captures 'scribble-colour-tune-summary.json'
    $scribbleTuneRowsPath=Join-Path $captures 'scribble-colour-tune.csv'
    $scribbleTranscript=Join-Path $captures 'scribble-colour-tune-commands.log'
    $scribblePreferencePath=Join-Path $captures 'scribble-colour-tune-scratch/profile/ColourWars/save_data.json'
    $scribbleTune=Get-Content -LiteralPath $scribbleTuneSummaryPath -Raw|ConvertFrom-Json
    $scribbleTuneRows=@(Import-Csv -LiteralPath $scribbleTuneRowsPath)
    $scribbleTuneCounts=[ordered]@{commands=9;states=9;contexts=1;readbacks=36;csv_rows=9;crops=0;saved_tokens=1;private_seed_saves=2}
    Assert-ScribbleKeys $scribbleTune (@($scribbleTuneCounts.Keys)+$scribbleUpdates+@('scope','event_fade')) 'Debug'
    foreach($key in $scribbleTuneCounts.Keys){Assert-ScribbleInteger $scribbleTune $key $scribbleTuneCounts[$key]}
    Assert-ScribbleUpdates $scribbleTune
    Assert-ScribblePairs $scribbleTuneRows @('baseline','named','reset-original','rgb','rgba128','alpha0','reset-saved','rgb-after-save','reset-all-saved') @('yellow-flat')
    $scribbleTuneColumns=@('state','context','background','paper','roi_rgba_match','adoption_match','repeat_match','guard_channels','delta_union',
        'frozen_inputs_resources_rng','private_bytes','adoption')
    if($scribbleTune.scope -cne 'debug-validated-commands' -or $scribbleTune.event_fade -isnot [bool] -or $scribbleTune.event_fade -ne $false -or
        ($scribbleTuneRows[0].PSObject.Properties.Name-join '|') -cne ($scribbleTuneColumns-join '|')){throw 'Scribble Debug oracle/schema/scope differs'}
    foreach($row in $scribbleTuneRows){
        Assert-ScribbleRowFlags $row
        if($row.private_bytes -cne 'true' -or $row.adoption -cne 'live'){throw 'Scribble Debug private-content/preference or live/reset proof flag differs'}
    }
    if((Get-Item -LiteralPath $scribbleTranscript).Length -le 0 -or $scribbleSaves -ne 2 -or $scribbleSuccess -ne 2 -or
        !(Test-Path -LiteralPath $scribblePreferencePath -PathType Leaf) -or (Get-Item -LiteralPath $scribblePreferencePath).Length -le 0){
        throw 'Scribble Debug transcript/private preference seed Save evidence differs'
    }
    # C++ owns the exact one-token Save, source/staged/actual bytes, seeded
    # preferences/default display/JumpJ, empty queues and whole-RGBA resets.
    foreach($path in @($scribbleTuneSummaryPath,$scribbleTuneRowsPath,$scribbleTranscript,$scribblePreferencePath)){
        $scribbleArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}
    }
    $scribbleObserved.developer_commands=$scribbleTune;$scribbleObserved.readbacks=76;$scribbleObserved.csv_rows=17
}elseif($scribbleSaves -ne 0 -or $scribbleSuccess -ne 0 -or @(Get-ChildItem -LiteralPath $captures -Filter 'scribble-colour-tune-*').Count){
    throw 'Release Scribble proof must stay literal-consumer-only without developer artifacts or private Saves'
}
# BEGIN Scribble precision metadata v3
# The outer runner serializes at Depth5. Keep its per-crop equivalence values
# primitive; preserve the complete shared records in a deterministic sidecar.
# No preset, run path or timestamp enters this shared metadata artifact.
$scribblePrecisionSidecarPath=Join-Path $captures 'scribble-colour-precision-admissions.json'
$scribblePrecisionSidecar=[ordered]@{
    kind='scribble-shared-whole-crop-precision-admissions-v1'
    representative_models=@($scribblePrecisionModels)
    precision_scope='four representative models only, not universal D3D11 conformance or device identification'
    admission='one complete exact model per crop; all matching names/equivalence recorded; no per-pixel mixing or blanket tolerance'
    cases=$scribbleCropEvidence
}
[IO.File]::WriteAllText($scribblePrecisionSidecarPath,($scribblePrecisionSidecar|ConvertTo-Json -Depth 12),[Text.UTF8Encoding]::new($false))
$scribbleArtifacts.crops=@(foreach($cropEvidence in $scribbleCropEvidence){
    $flatEquivalence=@(foreach($equivalenceGroup in $cropEvidence.source_model_equivalence_classes){
        [string[]]$canonicalNames=@($equivalenceGroup.models)
        [Array]::Sort($canonicalNames,[StringComparer]::Ordinal)
        $equivalenceGroup.source_payload_sha256+':'+($canonicalNames-join ',')
    })
    [ordered]@{name=$cropEvidence.name;width=$cropEvidence.width;height=$cropEvidence.height;bytes=$cropEvidence.bytes;sha256=$cropEvidence.sha256
        matching_source_models=@($cropEvidence.matching_source_models);source_model_equivalence_classes=$flatEquivalence;admission=$cropEvidence.admission}
})
$scribbleArtifacts['scribble-colour-precision-admissions.json']=[ordered]@{
    bytes=(Get-Item -LiteralPath $scribblePrecisionSidecarPath).Length
    sha256=(Get-FileHash -LiteralPath $scribblePrecisionSidecarPath).Hash
}
# END Scribble precision metadata v3
$result.observed_counts=$scribbleObserved
