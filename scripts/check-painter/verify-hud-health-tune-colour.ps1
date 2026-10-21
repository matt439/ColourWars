# Dot-sourced only after -HudHealthTuneColourOnly native execution.
# Independent literal192x16 health-track RGB proof; native owns RGBA and actual
# wrong-reference/full-frame adoption/empty-health/ammunition invariants.
# Four approved representative arithmetic models are equal for these12 cases;
# this is no universal D3D11, full-HUD, Player/Level or human acceptance claim.
$hudHealthContexts=@('empty','half','full')
$hudHealthStates=@('default','rgb','rgba128','alpha0')
$hudHealthWidths=@{empty=0;half=96;full=192}
$hudHealthAmounts=@{empty=0.0;half=0.5;full=1.0}
$hudHealthSelectedRGB=@{default=@(118,232,181);rgb=@(53,188,225);rgba128=@(108,140,188);alpha0=@(184,184,184)}
$hudHealthModels=@('F_AF','Q_AF','F_AQ','Q_AQ')
function New-HudHealthExpectedRGB([string]$Context,[string]$State){
    if($Context -cnotin $hudHealthContexts -or $State -cnotin $hudHealthStates){throw 'HUD Health literal source case differs'}
    $expected=[byte[]]::new(9216);$width=$hudHealthWidths[$Context];$selected=$hudHealthSelectedRGB[$State]
    # Crop-local half-open literal prefix: [0,width)x[0,16), after opaqueB8.
    # No production draw_meter geometry/helper or sampled channels are used.
    for($y=0;$y -lt 16;++$y){for($x=0;$x -lt 192;++$x){
        $offset=($y*192+$x)*3
        for($channel=0;$channel -lt 3;++$channel){
            $expected[$offset+$channel]=if($x -lt $width){$selected[$channel]}else{184}
        }
    }}
    return ,$expected
}
function Assert-HudHealthInteger($Object,[string]$Key,[long]$Expected){
    $value=$Object.$Key
    if(($value -isnot [int] -and $value -isnot [long]) -or $value -ne $Expected){throw "HUD Health exact integer differs: $Key"}
}
function Assert-HudHealthKeys($Object,[string[]]$Expected,[string]$Scope){
    if((($Object.PSObject.Properties.Name|Sort-Object)-join '|') -cne (($Expected|Sort-Object)-join '|')){throw "HUD Health exact $Scope JSON schema differs"}
}
function Assert-HudHealthPairs($Rows,[string[]]$States,[string[]]$Contexts){
    $expectedPairs=@(foreach($state in $States){foreach($context in $Contexts){"$state/$context"}})
    $pairs=@($Rows|ForEach-Object{"$($_.state)/$($_.context)"})
    if($Rows.Count -ne $expectedPairs.Count -or (($pairs|Sort-Object)-join '|') -cne (($expectedPairs|Sort-Object)-join '|')){throw 'HUD Health exact state/context pairs differ'}
}
$hudHealthUpdates=@('local_updates','player_updates','level_updates','scene_updates','state_updates','input_updates','time_updates','input_polls')
$hudHealthTrueFlags=@('roi_rgba_match','adoption_match','repeat_match','literal_channels','delta_prefix','ammo_unchanged','frozen_inputs_resources_rng')
function Assert-HudHealthUpdates($Object){foreach($key in $hudHealthUpdates){Assert-HudHealthInteger $Object $key 0}}
function Assert-HudHealthRowFlags($Row){
    if($Row.background -cne 'canvas32-64-96'){throw 'HUD Health Canvas differs'}
    foreach($key in $hudHealthTrueFlags){if($Row.$key -cne 'true'){throw "HUD Health native proof flag differs: $key"}}
}
$hudHealthSummaryPath=Join-Path $captures 'hud-health-colour-consumer-summary.json'
$hudHealthCSVPath=Join-Path $captures 'hud-health-colour-consumers.csv'
foreach($path in @($hudHealthSummaryPath,$hudHealthCSVPath,(Join-Path $run 'run.log'),(Join-Path $run 'run.err'))){
    if(!(Test-Path -LiteralPath $path -PathType Leaf)){throw 'HUD Health required evidence file is missing'}
}
if(@(Get-ChildItem -LiteralPath $captures -Filter 'hud-health-failure-*' -File).Count){throw 'HUD Health failure artifacts remain'}
$hudHealthSummary=Get-Content -LiteralPath $hudHealthSummaryPath -Raw|ConvertFrom-Json
$hudHealthRows=@(Import-Csv -LiteralPath $hudHealthCSVPath)
$hudHealthCounts=[ordered]@{readbacks=54;csv_rows=12;crops=12;crop_width=192;crop_height=16;crop_pixels=36864;ppm_bytes=110760;
    contexts=3;palettes=4;roi_pixels=3072;base_case_reads=48;blank_reads=0;maximum_selected_writes=1;controls=6;
    partial_straight_reads=2;partial_squared_reads=2;alpha0_straight_reads=2;changed_control_pixels=13824;unchanged_control_pixels=4608;
    zero_health_invariant_cases=4;ammo_guard_cases=12}
Assert-HudHealthKeys $hudHealthSummary (@($hudHealthCounts.Keys)+$hudHealthUpdates+@('scope','viewport','background','event_fade','raw_reference','whole_frame_reference','roi_half_open')) 'shared'
foreach($key in $hudHealthCounts.Keys){Assert-HudHealthInteger $hudHealthSummary $key $hudHealthCounts[$key]}
Assert-HudHealthUpdates $hudHealthSummary
$hudHealthROI=@($hudHealthSummary.roi_half_open)
$hudHealthIntegerROI=@($hudHealthROI|Where-Object{$_ -is [int] -or $_ -is [long]})
if($hudHealthSummary.scope -cne 'consumer-only' -or $hudHealthSummary.viewport -cne '1280x720' -or $hudHealthSummary.background -cne 'canvas32-64-96' -or
    $hudHealthSummary.event_fade -isnot [bool] -or $hudHealthSummary.event_fade -ne $false -or
    $hudHealthSummary.raw_reference -cne '192x16-opaqueB8-track-then-literal-prefix-only' -or
    $hudHealthSummary.whole_frame_reference -cne 'copied-complete-HUD-consumer-adoption-only' -or
    $hudHealthIntegerROI.Count -ne 4 -or ($hudHealthROI-join ',') -cne '1036,64,1228,80'){throw 'HUD Health shared oracle/ROI/scope differs'}
$hudHealthColumns=@('state','context','background','fill_width','roi_rgba_match','adoption_match','repeat_match','literal_channels','delta_prefix',
    'zero_health_invariant','ammo_unchanged','frozen_inputs_resources_rng','controls','changed_control_pixels','unchanged_control_pixels','scope')
Assert-HudHealthPairs $hudHealthRows $hudHealthStates $hudHealthContexts
if(($hudHealthRows[0].PSObject.Properties.Name-join '|') -cne ($hudHealthColumns-join '|')){throw 'HUD Health shared CSV columns differ'}
$hudHealthControls=0;$hudHealthChanged=0;$hudHealthUnchanged=0;$hudHealthEmptyInvariants=0
foreach($row in $hudHealthRows){
    Assert-HudHealthRowFlags $row
    $width=$hudHealthWidths[$row.context]
    $invariant=if($width -eq 0){'true'}else{'not-applicable'}
    if($row.scope -cne 'consumer-only' -or $row.fill_width -cne [string]$width -or $row.zero_health_invariant -cne $invariant){
        throw 'HUD Health literal prefix/zero-health invariant/scope differs'
    }
    if($width -eq 0){++$hudHealthEmptyInvariants}
    $controls=if($width -gt 0 -and $row.state -ceq 'rgba128'){2}elseif($width -gt 0 -and $row.state -ceq 'alpha0'){1}else{0}
    $changed=$controls*$width*16;$unchanged=$controls*(3072-$width*16)
    if($row.controls -cne [string]$controls -or $row.changed_control_pixels -cne [string]$changed -or $row.unchanged_control_pixels -cne [string]$unchanged){
        throw 'HUD Health selected-prefix/control-complement counts differ'
    }
    $hudHealthControls+=$controls;$hudHealthChanged+=$changed;$hudHealthUnchanged+=$unchanged
}
if($hudHealthControls -ne 6 -or $hudHealthChanged -ne 13824 -or $hudHealthUnchanged -ne 4608 -or $hudHealthEmptyInvariants -ne 4){
    throw 'HUD Health control/empty-health totals differ'
}
$hudHealthCrops=@(Get-ChildItem -LiteralPath $captures -Filter 'hud-health-colour-*.ppm' -File|Sort-Object Name)
$hudHealthNames=@(foreach($context in $hudHealthContexts){foreach($state in $hudHealthStates){"hud-health-colour-$context-$state.ppm"}})
if($hudHealthCrops.Count -ne 12 -or (($hudHealthCrops.Name|Sort-Object)-join '|') -cne (($hudHealthNames|Sort-Object)-join '|')){throw 'HUD Health exact retained crop cases differ'}
$hudHealthHeader=[Text.Encoding]::ASCII.GetBytes("P6`n192 16`n255`n")
$hudHealthBytes=0;$hudHealthCheckedPixels=0
$hudHealthCropEvidence=@(foreach($crop in $hudHealthCrops){
    $context=@($hudHealthContexts|Where-Object{$crop.Name.StartsWith("hud-health-colour-$_-",[StringComparison]::Ordinal)})
    if($context.Count -ne 1){throw 'HUD Health crop context differs'}
    $state=$crop.BaseName.Substring(("hud-health-colour-$($context[0])-").Length)
    $bytes=[IO.File]::ReadAllBytes($crop.FullName)
    if($bytes.Length -ne 9230){throw 'HUD Health crop byte length differs'}
    for($i=0;$i -lt $hudHealthHeader.Length;++$i){if($bytes[$i] -ne $hudHealthHeader[$i]){throw 'HUD Health canonical P6 header differs'}}
    $expected=New-HudHealthExpectedRGB $context[0] $state
    for($pixel=0;$pixel -lt 3072;++$pixel){for($channel=0;$channel -lt 3;++$channel){
        $offset=$pixel*3+$channel
        if($bytes[$hudHealthHeader.Length+$offset] -ne $expected[$offset]){
            $worldX=1036+$pixel%192;$worldY=64+[int][Math]::Floor($pixel/192)
            throw "HUD Health retained RGB ROI differs: $($crop.Name) pixel=$worldX,$worldY channel=$channel expected=$($expected[$offset]) actual=$($bytes[$hudHealthHeader.Length+$offset])"
        }
    }}
    # The outer runner uses Depth5: crop metadata stays primitive/flat.
    # All four source representatives are identical for these twelve cases;
    # full expected payload equality therefore admits their single equivalence.
    $payloadHash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($expected))
    $pngPath=[IO.Path]::ChangeExtension($crop.FullName,'.png')
    if(Test-Path -LiteralPath $pngPath -PathType Leaf){
        $png=[IO.File]::ReadAllBytes($pngPath);$signature=@(137,80,78,71,13,10,26,10)
        if($png.Length -lt 33){throw 'HUD Health PNG header is truncated'}
        for($i=0;$i -lt 8;++$i){if($png[$i] -ne $signature[$i]){throw 'HUD Health PNG signature differs'}}
        if([Text.Encoding]::ASCII.GetString($png,12,4) -cne 'IHDR'){throw 'HUD Health PNG lacks initial IHDR'}
        $pngWidth=0L;$pngHeight=0L
        for($i=0;$i -lt 4;++$i){$pngWidth=$pngWidth*256+$png[16+$i];$pngHeight=$pngHeight*256+$png[20+$i]}
        if($pngWidth -ne 192 -or $pngHeight -ne 16){throw 'HUD Health PNG dimensions differ'}
    }
    $hudHealthBytes+=$bytes.Length;$hudHealthCheckedPixels+=3072
    [ordered]@{name=$crop.Name;width=192;height=16;bytes=$bytes.Length;sha256=(Get-FileHash -LiteralPath $crop.FullName).Hash;
        context=$context[0];state=$state;selected_width=$hudHealthWidths[$context[0]];source_payload_sha256=$payloadHash;
        matching_source_models=@($hudHealthModels);source_model_equivalence=($payloadHash+':F_AF,F_AQ,Q_AF,Q_AQ');
        admission='one exact literal prefix payload; all four scoped source representatives are equivalent'}
})
if($hudHealthBytes -ne 110760 -or $hudHealthCheckedPixels -ne 36864){throw 'HUD Health retained RGB pixel/PPM totals differ'}
$hudHealthLog=@(Get-Content -LiteralPath (Join-Path $run 'run.log'))
$hudHealthSaves=@($hudHealthLog|Where-Object{$_ -match 'Saving\.\.\.$'}).Count
$hudHealthSuccess=@($hudHealthLog|Where-Object{$_ -ceq 'Save was successful.'}).Count
if(@(Get-Content -LiteralPath (Join-Path $run 'run.err')|Where-Object{$_ -match '^save failure:'}).Count){throw 'Unexpected HUD Health private Save failure'}
$hudHealthArtifacts=[ordered]@{}
foreach($path in @($hudHealthSummaryPath,$hudHealthCSVPath)){$hudHealthArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}}
$hudHealthArtifacts.crops=$hudHealthCropEvidence
$hudHealthObserved=[ordered]@{consumer=$hudHealthSummary;readbacks=54;csv_rows=12;crops=12;controls=6;changed_control_pixels=13824;unchanged_control_pixels=4608;
    crop_pixels=36864;ppm_bytes=$hudHealthBytes;retained_rgb_pixels=$hudHealthCheckedPixels;retained_rgb_payload_bytes=110592;
    retained_rgb_scope='independent-complete-literal192x16-opaqueB8-and-prefix-only; native ownsRGBA';source_precision_models=@($hudHealthModels);
    source_precision_scope='four scoped representative models equal for these12track-cases only; no universal D3D11/fullHUD/device claim';
    zero_health_invariant_cases=4;zero_health_scope='Native whole-frame invariant; independent retained ROI is unchanged opaqueB8';
    ammo_guard_cases=12;ammo_scope='Native retained unchanged ammunition bytes; no independent ammo alpha/art reconstruction';
    control_scope='Native actual wrong-reference RGBA discriminates each full selected prefix and preserves its exact ROI complement';
    local_updates=0;player_updates=0;level_updates=0;scene_updates=0;state_updates=0;input_updates=0;time_updates=0;input_polls=0;
    private_save_calls=$hudHealthSaves;successful_private_saves=$hudHealthSuccess;failed_private_saves=0;artifacts=$hudHealthArtifacts}
if($debug){
    $hudHealthTunePath=Join-Path $captures 'hud-health-colour-tune-summary.json'
    $hudHealthTuneCSV=Join-Path $captures 'hud-health-colour-tune.csv'
    $hudHealthTranscript=Join-Path $captures 'hud-health-colour-tune-commands.log'
    $hudHealthPreferencePath=Join-Path $captures 'hud-health-colour-tune-scratch/profile/ColourWars/save_data.json'
    $hudHealthTune=Get-Content -LiteralPath $hudHealthTunePath -Raw|ConvertFrom-Json
    $hudHealthTuneRows=@(Import-Csv -LiteralPath $hudHealthTuneCSV)
    $hudHealthTuneCounts=[ordered]@{commands=9;states=9;contexts=1;readbacks=36;csv_rows=9;crops=0;saved_tokens=1;private_seed_saves=2;ammo_guard_cases=9}
    Assert-HudHealthKeys $hudHealthTune (@($hudHealthTuneCounts.Keys)+$hudHealthUpdates+@('scope','event_fade','health_context')) 'Debug'
    foreach($key in $hudHealthTuneCounts.Keys){Assert-HudHealthInteger $hudHealthTune $key $hudHealthTuneCounts[$key]}
    Assert-HudHealthUpdates $hudHealthTune
    Assert-HudHealthPairs $hudHealthTuneRows @('baseline','named','reset-original','rgb','rgba128','alpha0','reset-saved','rgb-after-save','reset-all-saved') @('full')
    $hudHealthTuneColumns=@('state','context','background','fill_width','roi_rgba_match','adoption_match','repeat_match','literal_channels','delta_prefix',
        'zero_health_invariant','ammo_unchanged','frozen_inputs_resources_rng','private_bytes','adoption')
    if($hudHealthTune.scope -cne 'debug-validated-commands' -or $hudHealthTune.event_fade -isnot [bool] -or $hudHealthTune.event_fade -ne $false -or
        $hudHealthTune.health_context -cne 'full' -or ($hudHealthTuneRows[0].PSObject.Properties.Name-join '|') -cne ($hudHealthTuneColumns-join '|')){
        throw 'HUD Health Debug oracle/schema/context differs'
    }
    foreach($row in $hudHealthTuneRows){
        Assert-HudHealthRowFlags $row
        if($row.fill_width -cne '192' -or $row.zero_health_invariant -cne 'not-applicable' -or $row.private_bytes -cne 'true' -or $row.adoption -cne 'live'){
            throw 'HUD Health Debug full-prefix/private-byte/live proof flag differs'
        }
    }
    if(!(Test-Path -LiteralPath $hudHealthTranscript -PathType Leaf) -or (Get-Item -LiteralPath $hudHealthTranscript).Length -le 0 -or
        $hudHealthSaves -ne 2 -or $hudHealthSuccess -ne 2 -or !(Test-Path -LiteralPath $hudHealthPreferencePath -PathType Leaf) -or
        (Get-Item -LiteralPath $hudHealthPreferencePath).Length -le 0){throw 'HUD Health Debug transcript/private preference seed Save evidence differs'}
    # Native owns exact one-token source/staged/content Save/rebase, whole-HUD
    # original/saved reversals, seeded preferences/default display/JumpJ and
    # empty queues; retained files/logs corroborate rather than replace that.
    foreach($path in @($hudHealthTunePath,$hudHealthTuneCSV,$hudHealthTranscript,$hudHealthPreferencePath)){
        $hudHealthArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}
    }
    $hudHealthObserved.developer_commands=$hudHealthTune;$hudHealthObserved.readbacks=90;$hudHealthObserved.csv_rows=21
}elseif($hudHealthSaves -ne 0 -or $hudHealthSuccess -ne 0 -or @(Get-ChildItem -LiteralPath $captures -Filter 'hud-health-colour-tune-*').Count){
    throw 'Release HUD Health proof must stay consumer-only without developer artifacts or private Saves'
}
$result.observed_counts=$hudHealthObserved
