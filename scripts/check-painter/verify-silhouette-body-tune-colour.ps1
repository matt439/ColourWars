# Dot-sourced only after -SilhouetteBodyTuneColourOnly native execution.
# Retained PPMs independently qualify RGB at nine literal 3x3 overlap patches.
# Native C++ owns RGBA/raw/uniformity/repeat/reset proof; no full-mask claim.
$bodySummaryPath=Join-Path $captures 'silhouette-body-colour-consumer-summary.json'
$bodyRowsPath=Join-Path $captures 'silhouette-body-colour-consumers.csv'
$bodySummary=Get-Content -LiteralPath $bodySummaryPath -Raw|ConvertFrom-Json
$bodyRows=@(Import-Csv -LiteralPath $bodyRowsPath)
$bodyContexts=@('adult-rest-mug','adult-half-stand','child-rest')
$bodyStates=@('default','rgb','rgba128','alpha0')
$bodyUpdates=@('local_updates','player_updates','level_updates','state_updates','input_updates','time_updates')
function Assert-BodyInteger($Object,[string]$Key,[int]$Expected){
    $value=$Object.$Key
    if(($value -isnot [int] -and $value -isnot [long]) -or $value -ne $Expected){throw "Body exact integer differs: $Key"}
}
function Assert-BodyKeys($Object,$Keys,[string]$Label){
    if((@($Object.PSObject.Properties.Name|Sort-Object)-join '|') -cne (@($Keys|Sort-Object)-join '|')){throw "Body exact JSON schema differs: $Label"}
}
function Assert-BodyUpdates($Summary){foreach($key in $bodyUpdates){Assert-BodyInteger $Summary $key 0}}
function Assert-BodyPairs($Rows,$States){
    $wanted=@(foreach($state in $States){foreach($context in $bodyContexts){"$state/$context/canvas32-64-96"}})
    $actual=@($Rows|ForEach-Object{"$($_.state)/$($_.context)/$($_.background)"})
    if($Rows.Count -ne $wanted.Count -or (($actual|Sort-Object)-join '|') -cne (($wanted|Sort-Object)-join '|')){throw 'Body exact state/context/Canvas pairs differ'}
}
function Assert-BodyRowGeometry($Row){
    $count=if($Row.context -ceq 'adult-rest-mug'){'6'}elseif($Row.context -ceq 'adult-half-stand'){'1'}else{'2'}
    $writes=if($Row.context -ceq 'adult-rest-mug'){'1;2;2;1;2;3'}elseif($Row.context -ceq 'adult-half-stand'){'2'}else{'1;2'}
    if($Row.patch_count -cne $count -or $Row.write_counts -cne $writes){throw 'Body literal patch/write counts differ'}
    foreach($key in @('raw_patch_channels','uniform_rgba','adoption_match','exterior_and_delta','frozen_inputs_resources_rng')){
        if($Row.$key -cne 'true'){throw "Body native proof flag differs: $key"}
    }
}
$bodyCounts=[ordered]@{readbacks=61;csv_rows=12;crops=12;crop_pixels=759808;ppm_bytes=2279604;contexts=3;
    patches=9;patch_width=3;patch_height=3;raw_effective_underlays=1;blank_reads=0;controls=13;
    partial_straight_reads=3;partial_squared_reads=3;collapsed_one_reads=3;collapsed_two_reads=1;
    alpha0_straight_reads=3;affected_rejections=34;unaffected_guards=8}
$bodySummaryKeys=@($bodyCounts.Keys)+$bodyUpdates+@('scope','viewport','event_fade','whole_frame_reference','raw_reference','adult_crop','child_crop')
Assert-BodyKeys $bodySummary $bodySummaryKeys 'shared'
foreach($key in $bodyCounts.Keys){Assert-BodyInteger $bodySummary $key $bodyCounts[$key]}
Assert-BodyUpdates $bodySummary
if($bodySummary.scope -cne 'consumer-only' -or $bodySummary.viewport -cne '1280x720' -or
    $bodySummary.event_fade -isnot [bool] -or $bodySummary.event_fade -ne $false -or
    $bodySummary.whole_frame_reference -cne 'copied-production-consumer-adoption-only' -or
    $bodySummary.raw_reference -cne 'nine-local-ordered-rgba-patches-only' -or
    ($bodySummary.adult_crop-join ',') -cne '320,224' -or ($bodySummary.child_crop-join ',') -cne '208,224'){
    throw 'Body shared oracle/crop/scope differs'
}
foreach($key in @('adult_crop','child_crop')){
    if($bodySummary.$key -isnot [array] -or $bodySummary.$key.Count -ne 2 -or
        @($bodySummary.$key|Where-Object{$_ -isnot [int] -and $_ -isnot [long]}).Count){throw 'Body crop dimensions must be exact integer pairs'}
}
Assert-BodyPairs $bodyRows $bodyStates
$bodyColumns=@('state','context','background','patch_count','write_counts','raw_patch_channels','uniform_rgba','adoption_match',
    'exterior_and_delta','frozen_inputs_resources_rng','controls','affected_rejections','unaffected_guards','scope')
if(($bodyRows[0].PSObject.Properties.Name-join '|') -cne ($bodyColumns-join '|')){throw 'Body shared CSV columns differ'}
$bodyControlTotal=0;$bodyAffectedTotal=0;$bodyUnaffectedTotal=0
foreach($row in $bodyRows){
    Assert-BodyRowGeometry $row
    if($row.scope -cne 'consumer-only'){throw 'Body shared row scope differs'}
    $contextIndex=[array]::IndexOf($bodyContexts,$row.context)
    $controls=0;$affected=0;$unaffected=0
    if($row.state -ceq 'rgba128'){$controls=@(4,3,3)[$contextIndex];$affected=@(17,3,5)[$contextIndex];$unaffected=@(7,0,1)[$contextIndex]}
    elseif($row.state -ceq 'alpha0'){$controls=1;$affected=@(6,1,2)[$contextIndex]}
    if($row.controls -cne [string]$controls -or $row.affected_rejections -cne [string]$affected -or $row.unaffected_guards -cne [string]$unaffected){
        throw 'Body per-case independent control/rejection/unchanged-guard counts differ'
    }
    $bodyControlTotal+=$controls;$bodyAffectedTotal+=$affected;$bodyUnaffectedTotal+=$unaffected
}
if($bodyControlTotal -ne 13 -or $bodyAffectedTotal -ne 34 -or $bodyUnaffectedTotal -ne 8){throw 'Body control totals differ'}
if(@(Get-ChildItem -LiteralPath $captures -Filter 'silhouette-body-failure*' -File).Count){throw 'Body failure artifacts remain'}
$bodyCrops=@(Get-ChildItem -LiteralPath $captures -Filter 'silhouette-body-colour-*.ppm' -File|Sort-Object Name)
$bodyNames=@(foreach($context in $bodyContexts){foreach($state in $bodyStates){"silhouette-body-colour-$context-$state.ppm"}})
if($bodyCrops.Count -ne 12 -or (($bodyCrops.Name|Sort-Object)-join '|') -cne (($bodyNames|Sort-Object)-join '|')){throw 'Body exact native crop cases differ'}
$bodyBytes=0;$bodyPixels=0;$bodyCheckedPixels=0
$bodyCropEvidence=@(foreach($crop in $bodyCrops){
    $context=@($bodyContexts|Where-Object{$crop.Name.StartsWith("silhouette-body-colour-$_-",[StringComparison]::Ordinal)})
    if($context.Count -ne 1){throw 'Body crop context differs'}
    $state=$crop.BaseName.Substring(("silhouette-body-colour-$($context[0])-").Length)
    $width=if($context[0] -ceq 'child-rest'){208}else{320};$height=224
    $header=[Text.Encoding]::ASCII.GetBytes("P6`n$width $height`n255`n")
    $bytes=[IO.File]::ReadAllBytes($crop.FullName)
    if($bytes.Length -ne $header.Length+$width*$height*3){throw "Body native crop byte length differs: $($crop.Name)"}
    for($i=0;$i -lt $header.Length;++$i){if($bytes[$i] -ne $header[$i]){throw 'Body canonical P6 header differs'}}
    # Literal crop-local centres/counts, independent of native CSV or drawing helpers.
    $patches=@(if($context[0] -ceq 'adult-rest-mug'){@(@(80,96,1),@(80,174,2),@(80,216,2),@(236,144,1),@(249,163,2),@(249,156,3))}
        elseif($context[0] -ceq 'adult-half-stand'){,@(240,92,2)}else{@(@(104,61,1),@(77,56,2))})
    foreach($patch in $patches){
        $rgb=if($state -ceq 'default'){@(14,14,14)}elseif($state -ceq 'rgb'){@(53,188,225)}elseif($state -ceq 'alpha0'){@(0,0,0)}
            elseif($state -ceq 'rgba128'){@(@(16,48,96),@(24,72,144),@(28,84,168))[$patch[2]-1]}else{throw 'Body crop state differs'}
        for($y=$patch[1]-1;$y -le $patch[1]+1;++$y){for($x=$patch[0]-1;$x -le $patch[0]+1;++$x){
            $offset=$header.Length+($y*$width+$x)*3
            for($channel=0;$channel -lt 3;++$channel){if($bytes[$offset+$channel] -ne $rgb[$channel]){
                throw "Body literal RGB patch differs: $($crop.Name) centre=$($patch[0]),$($patch[1]) writes=$($patch[2]) pixel=$x,$y channel=$channel"
            }}
            ++$bodyCheckedPixels
        }}
    }
    # The outer runner converts PPMs after this verifier. If PNGs already exist,
    # validate their IHDR dimensions only; PPM bytes remain the pixel evidence.
    $pngPath=[IO.Path]::ChangeExtension($crop.FullName,'.png')
    if(Test-Path -LiteralPath $pngPath -PathType Leaf){
        $png=[IO.File]::ReadAllBytes($pngPath);$signature=@(137,80,78,71,13,10,26,10)
        if($png.Length -lt 33){throw 'Body PNG header is truncated'}
        for($i=0;$i -lt 8;++$i){if($png[$i] -ne $signature[$i]){throw 'Body PNG signature differs'}}
        if([Text.Encoding]::ASCII.GetString($png,12,4) -cne 'IHDR'){throw 'Body PNG lacks initial IHDR'}
        $pngWidth=0L;$pngHeight=0L
        for($i=0;$i -lt 4;++$i){$pngWidth=$pngWidth*256+$png[16+$i];$pngHeight=$pngHeight*256+$png[20+$i]}
        if($pngWidth -ne $width -or $pngHeight -ne $height){throw 'Body PNG dimensions differ'}
    }
    $bodyBytes+=$bytes.Length;$bodyPixels+=$width*$height
    [ordered]@{name=$crop.Name;width=$width;height=$height;bytes=$bytes.Length;sha256=(Get-FileHash -LiteralPath $crop.FullName).Hash}
})
if($bodyBytes -ne 2279604 -or $bodyPixels -ne 759808 -or $bodyCheckedPixels -ne 324){throw 'Body exact crop byte/pixel/patch totals differ'}
$bodyLog=@(Get-Content -LiteralPath (Join-Path $run 'run.log'))
$bodySaves=@($bodyLog|Where-Object{$_ -match 'Saving\.\.\.$'}).Count
$bodySuccess=@($bodyLog|Where-Object{$_ -ceq 'Save was successful.'}).Count
if(@(Get-Content -LiteralPath (Join-Path $run 'run.err')|Where-Object{$_ -match '^save failure:'}).Count){throw 'Unexpected Body private Save failure'}
$bodyArtifacts=[ordered]@{}
foreach($path in @($bodySummaryPath,$bodyRowsPath)){$bodyArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}}
$bodyArtifacts.crops=$bodyCropEvidence
$bodyObserved=[ordered]@{consumer=$bodySummary;readbacks=61;csv_rows=12;crops=12;controls=13;affected_rejections=34;unaffected_guards=8;
    crop_pixels=759808;ppm_bytes=$bodyBytes;retained_rgb_patch_pixels=$bodyCheckedPixels;retained_rgb_patch_count=36;
    retained_rgb_scope='nine-literal-3x3-sites-four-palettes; native-owns-RGBA';
    local_updates=0;player_updates=0;level_updates=0;state_updates=0;input_updates=0;time_updates=0;
    private_save_calls=$bodySaves;successful_private_saves=$bodySuccess;failed_private_saves=0;artifacts=$bodyArtifacts}
if($debug){
    $bodyTuneSummaryPath=Join-Path $captures 'silhouette-body-colour-tune-summary.json'
    $bodyTuneRowsPath=Join-Path $captures 'silhouette-body-colour-tune.csv'
    $bodyTranscript=Join-Path $captures 'silhouette-body-colour-tune-commands.log'
    $bodyPreferencePath=Join-Path $captures 'silhouette-body-colour-tune-scratch/profile/ColourWars/save_data.json'
    $bodyTune=Get-Content -LiteralPath $bodyTuneSummaryPath -Raw|ConvertFrom-Json
    $bodyTuneRows=@(Import-Csv -LiteralPath $bodyTuneRowsPath)
    $bodyTuneCounts=[ordered]@{commands=9;states=9;contexts=3;readbacks=108;csv_rows=27;crops=0;saved_tokens=1}
    Assert-BodyKeys $bodyTune (@($bodyTuneCounts.Keys)+$bodyUpdates+@('scope','event_fade')) 'Debug'
    foreach($key in $bodyTuneCounts.Keys){Assert-BodyInteger $bodyTune $key $bodyTuneCounts[$key]}
    Assert-BodyUpdates $bodyTune
    Assert-BodyPairs $bodyTuneRows @('baseline','named','reset-original','rgb','rgba128','alpha0','reset-saved','rgb-after-save','reset-all-saved')
    $bodyTuneColumns=@('state','context','background','patch_count','write_counts','raw_patch_channels','uniform_rgba','adoption_match',
        'exterior_and_delta','frozen_inputs_resources_rng','private_bytes','adoption')
    if($bodyTune.scope -cne 'debug-validated-commands' -or $bodyTune.event_fade -isnot [bool] -or $bodyTune.event_fade -ne $false -or
        ($bodyTuneRows[0].PSObject.Properties.Name-join '|') -cne ($bodyTuneColumns-join '|')){throw 'Body Debug oracle/schema/scope differs'}
    foreach($row in $bodyTuneRows){
        Assert-BodyRowGeometry $row
        if($row.private_bytes -cne 'true' -or $row.adoption -cne 'live'){throw 'Body Debug private-content/preference or live/reset proof flag differs'}
    }
    if((Get-Item -LiteralPath $bodyTranscript).Length -le 0 -or $bodySaves -ne 2 -or $bodySuccess -ne 2 -or
        !(Test-Path -LiteralPath $bodyPreferencePath -PathType Leaf) -or (Get-Item -LiteralPath $bodyPreferencePath).Length -le 0){
        throw 'Body Debug transcript/private preference seed Save evidence differs'
    }
    # C++ brackets exact source/staged/actual-content bytes, all public seeded
    # preferences, JumpJ/default display, empty work queues and raster resets.
    # These retained bytes supplement that native proof; this script does not
    # infer private-state retention or alpha channels from an RGB-only PPM.
    foreach($path in @($bodyTuneSummaryPath,$bodyTuneRowsPath,$bodyTranscript,$bodyPreferencePath)){
        $bodyArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}
    }
    $bodyObserved.developer_commands=$bodyTune;$bodyObserved.readbacks=169;$bodyObserved.csv_rows=39
}elseif($bodySaves -ne 0 -or $bodySuccess -ne 0 -or @(Get-ChildItem -LiteralPath $captures -Filter 'silhouette-body-colour-tune-*').Count){
    throw 'Release Body proof must stay literal-consumer-only without developer artifacts or private Saves'
}
$result.observed_counts=$bodyObserved
