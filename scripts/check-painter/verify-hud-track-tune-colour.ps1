# Dot-sourced after -HudTrackTuneColourOnly native execution only.
# Independent lossless HEALTH+PAINT ROI stack:192x16 native pixels each,
# joined into192x32 without the source gap. Native owns full actual/rawRGBA,
# negative controls, complete-HUD adoption/repeat/invariance and private Saves.
$hudTrackContexts=@('empty','half','full');$hudTrackStates=@('default','rgb','rgba128','alpha0')
$hudTrackWidths=@{empty=0;half=96;full=192}
$hudTrackModels=@('F_AF','Q_AF','F_AQ','Q_AQ')
$hudTrackRGBA=@{default=@(184,184,184,255);rgb=@(53,188,225,255);rgba128=@(1,97,193,128);alpha0=@(165,50,214,0)}
function Blend-HudTrackSource($Destination,$RGBA,[string]$Model){
    # Original shader alpha premultiplies sourceRGB before either independent
    # representative conversion. Rounded blend alpha never replaces it.
    $shaderAlpha=$RGBA[3]/255.0;$blendAlpha=$shaderAlpha
    if($Model.EndsWith('_AQ',[StringComparison]::Ordinal)){$blendAlpha=[Math]::Floor($shaderAlpha*255.0+0.5)/255.0}
    $result=[byte[]]::new(4)
    for($channel=0;$channel -lt 3;++$channel){
        $source=$RGBA[$channel]*$shaderAlpha
        if($Model.StartsWith('Q_',[StringComparison]::Ordinal)){$source=[Math]::Floor($source+0.5)}
        $result[$channel]=[byte][Math]::Floor([Math]::Max(0.0,[Math]::Min(255.0,($source+$Destination[$channel]*(1-$blendAlpha))))+0.5)
    }
    $result[3]=[byte][Math]::Floor([Math]::Max(0.0,[Math]::Min(255.0,($blendAlpha*255.0+$Destination[3]*(1-$blendAlpha))))+0.5)
    return ,$result
}
function New-HudTrackExpectedRGBA([string]$Context,[string]$State,[string]$Model){
    if($Context -cnotin $hudTrackContexts -or $State -cnotin $hudTrackStates -or $Model -cnotin $hudTrackModels){throw 'HUD Track literal source case/model differs'}
    # Every owned pixel receives the same ordered opaque ink, then selected
    # track once. Each literal opaque fill subsequently covers its own prefix.
    $ink=Blend-HudTrackSource @(0,0,0,0) @(20,20,20,255) $Model
    $track=Blend-HudTrackSource $ink $hudTrackRGBA[$State] $Model
    $health=Blend-HudTrackSource $track @(118,232,181,255) $Model
    $paint=Blend-HudTrackSource $track @(6,52,204,255) $Model
    $partial=if($Model.StartsWith('Q_',[StringComparison]::Ordinal)){@(11,59,107,255)}else{@(10,59,107,255)}
    $literal=switch($State){default{@(184,184,184,255)}rgb{@(53,188,225,255)}rgba128{$partial}alpha0{@(20,20,20,255)}}
    if(($track-join ',') -cne ($literal-join ',') -or ($health-join ',') -cne '118,232,181,255' -or ($paint-join ',') -cne '6,52,204,255'){throw 'HUD Track ordered source differs from literal channel guards'}
    $buffer=[byte[]]::new(24576);$width=$hudTrackWidths[$Context]
    for($row=0;$row -lt 32;++$row){for($column=0;$column -lt 192;++$column){
        $colour=if($column -ge $width){$track}elseif($row -lt 16){$health}else{$paint}
        $at=($row*192+$column)*4
        for($channel=0;$channel -lt 4;++$channel){$buffer[$at+$channel]=$colour[$channel]}
    }}
    return ,$buffer
}
function Get-HudTrackRGB([byte[]]$RGBA){
    $rgb=[byte[]]::new(18432)
    for($pixel=0;$pixel -lt 6144;++$pixel){for($channel=0;$channel -lt 3;++$channel){$rgb[$pixel*3+$channel]=$RGBA[$pixel*4+$channel]}}
    return ,$rgb
}
function Assert-HudTrackInteger($Object,[string]$Key,[long]$Expected){
    $value=$Object.$Key
    if(($value -isnot [int] -and $value -isnot [long]) -or $value -ne $Expected){throw "HUD Track exact integer differs: $Key"}
}
function Assert-HudTrackKeys($Object,[string[]]$Expected,[string]$Scope){
    if((($Object.PSObject.Properties.Name|Sort-Object)-join '|') -cne (($Expected|Sort-Object)-join '|')){throw "HUD Track exact $Scope JSON schema differs"}
}
function Assert-HudTrackPairs($Rows,[string[]]$States,[string[]]$Contexts){
    $expected=@(foreach($state in $States){foreach($context in $Contexts){"$state/$context"}})
    if($Rows.Count -ne $expected.Count -or (($Rows|ForEach-Object{"$($_.state)/$($_.context)"}|Sort-Object)-join '|') -cne (($expected|Sort-Object)-join '|')){throw 'HUD Track exact state/context pairs differ'}
}
$hudTrackUpdates=@('local_updates','player_updates','level_updates','scene_updates','state_updates','input_updates','time_updates','input_polls')
$hudTrackFlags=@('roi_rgba_match','adoption_match','repeat_match','literal_channels','delta_suffix','fill_roles_unchanged','frozen_inputs_resources_rng')
function Assert-HudTrackUpdates($Object){foreach($key in $hudTrackUpdates){Assert-HudTrackInteger $Object $key 0}}
function Assert-HudTrackFlags($Row){
    if($Row.background -cne 'canvas32-64-96'){throw 'HUD Track Canvas differs'}
    foreach($key in $hudTrackFlags){if($Row.$key -cne 'true'){throw "HUD Track native flag differs: $key"}}
}
function Assert-HudTrackSourceNames([string]$Names,[bool]$VisiblePartial){
    if($VisiblePartial){
        if($Names -cnotin @('F_AF|F_AQ','Q_AF|Q_AQ')){throw 'HUD Track coherent source model names differ'}
    }elseif($Names -cne 'F_AF|Q_AF|F_AQ|Q_AQ'){throw 'HUD Track all-model equivalence names differ'}
}
$hudTrackSummaryPath=Join-Path $captures 'hud-track-colour-consumer-summary.json'
$hudTrackCSVPath=Join-Path $captures 'hud-track-colour-consumers.csv'
foreach($path in @($hudTrackSummaryPath,$hudTrackCSVPath,(Join-Path $run 'run.log'),(Join-Path $run 'run.err'))){
    if(!(Test-Path -LiteralPath $path -PathType Leaf)){throw 'HUD Track required evidence file missing'}
}
if(@(Get-ChildItem -LiteralPath $captures -Filter 'hud-track-failure-*' -File).Count){throw 'HUD Track failure artifacts remain'}
$hudTrackSummary=Get-Content $hudTrackSummaryPath -Raw|ConvertFrom-Json;$hudTrackRows=@(Import-Csv $hudTrackCSVPath)
$hudTrackCounts=[ordered]@{readbacks=54;csv_rows=12;crops=12;crop_width=192;crop_height=32;crop_pixels=73728;ppm_bytes=221352;
    contexts=3;palettes=4;roi_pixels=6144;source_rois=2;base_case_reads=48;blank_reads=0;selected_track_writes=2;maximum_selected_writes_per_pixel=1;
    controls=6;track_discriminations=12;partial_straight_reads=2;partial_squared_reads=2;alpha0_straight_reads=2;
    changed_control_pixels=27648;unchanged_control_pixels=9216;full_cover_invariant_cases=4;fill_role_guard_cases=12}
Assert-HudTrackKeys $hudTrackSummary (@($hudTrackCounts.Keys)+$hudTrackUpdates+@('scope','viewport','background','event_fade','raw_reference','whole_frame_reference','health_roi_half_open','ammo_roi_half_open','capture_kind','paired_amounts_only')) 'shared'
foreach($key in $hudTrackCounts.Keys){Assert-HudTrackInteger $hudTrackSummary $key $hudTrackCounts[$key]}
Assert-HudTrackUpdates $hudTrackSummary
foreach($roiField in @('health_roi_half_open','ammo_roi_half_open')){
    $roi=@($hudTrackSummary.$roiField);$integers=@($roi|Where-Object{$_ -is [int] -or $_ -is [long]})
    $literal=if($roiField -ceq 'health_roi_half_open'){'1036,64,1228,80'}else{'1036,122,1228,138'}
    if($integers.Count -ne 4 -or ($roi-join ',') -cne $literal){throw 'HUD Track disjoint literal ROI differs'}
}
if($hudTrackSummary.scope -cne 'consumer-only' -or $hudTrackSummary.viewport -cne '1280x720' -or $hudTrackSummary.background -cne 'canvas32-64-96' -or
    $hudTrackSummary.event_fade -isnot [bool] -or $hudTrackSummary.event_fade -ne $false -or
    $hudTrackSummary.paired_amounts_only -isnot [bool] -or $hudTrackSummary.paired_amounts_only -ne $true -or
    $hudTrackSummary.raw_reference -cne 'two-independent192x16-ink-track-prefix-interiors-only' -or
    $hudTrackSummary.whole_frame_reference -cne 'copied-complete-HUD-consumer-adoption-only' -or
    $hudTrackSummary.capture_kind -cne 'lossless-paired-ROI-stack'){throw 'HUD Track oracle/stack/paired scope differs'}
Assert-HudTrackPairs $hudTrackRows $hudTrackStates $hudTrackContexts
$hudTrackColumns=@('state','context','background','fill_width','roi_rgba_match','adoption_match','repeat_match','literal_channels','delta_suffix',
    'full_cover_invariant','fill_roles_unchanged','frozen_inputs_resources_rng','source_models','controls','track_discriminations','changed_control_pixels','unchanged_control_pixels','scope')
if(($hudTrackRows[0].PSObject.Properties.Name-join '|') -cne ($hudTrackColumns-join '|')){throw 'HUD Track shared CSV columns differ'}
$hudTrackControlReads=0;$hudTrackDiscriminations=0;$hudTrackChanged=0;$hudTrackUnchanged=0;$hudTrackCovered=0
foreach($row in $hudTrackRows){
    Assert-HudTrackFlags $row;$width=$hudTrackWidths[$row.context]
    $covered=if($width -eq 192){'true'}else{'not-applicable'}
    if($row.fill_width -cne [string]$width -or $row.full_cover_invariant -cne $covered -or $row.scope -cne 'consumer-only'){throw 'HUD Track prefix/full-cover/scope differs'}
    if($width -eq 192){++$hudTrackCovered}
    Assert-HudTrackSourceNames $row.source_models ($row.state -ceq 'rgba128' -and $width -lt 192)
    $controls=if($width -lt 192 -and $row.state -ceq 'rgba128'){2}elseif($width -lt 192 -and $row.state -ceq 'alpha0'){1}else{0}
    $discriminations=$controls*2;$changed=$controls*(192-$width)*16*2;$unchanged=$controls*$width*16*2
    if($row.controls -cne [string]$controls -or $row.track_discriminations -cne [string]$discriminations -or
        $row.changed_control_pixels -cne [string]$changed -or $row.unchanged_control_pixels -cne [string]$unchanged){throw 'HUD Track both-track suffix/control-complement counts differ'}
    $hudTrackControlReads+=$controls;$hudTrackDiscriminations+=$discriminations;$hudTrackChanged+=$changed;$hudTrackUnchanged+=$unchanged
}
if($hudTrackControlReads -ne 6 -or $hudTrackDiscriminations -ne 12 -or $hudTrackChanged -ne 27648 -or $hudTrackUnchanged -ne 9216 -or $hudTrackCovered -ne 4){throw 'HUD Track control/full-cover totals differ'}
$hudTrackStacks=@(Get-ChildItem -LiteralPath $captures -Filter 'hud-track-colour-*.ppm' -File|Sort-Object Name)
$hudTrackExpectedNames=@(foreach($context in $hudTrackContexts){foreach($state in $hudTrackStates){"hud-track-colour-$context-$state.ppm"}})
if($hudTrackStacks.Count -ne 12 -or (($hudTrackStacks.Name|Sort-Object)-join '|') -cne (($hudTrackExpectedNames|Sort-Object)-join '|')){throw 'HUD Track exact paired-stack cases differ'}
$hudTrackHeader=[Text.Encoding]::ASCII.GetBytes("P6`n192 32`n255`n");$hudTrackP6Bytes=0;$hudTrackCheckedPixels=0;$hudTrackClassCount=0;$hudTrackSplitCases=0
$hudTrackStackEvidence=@(foreach($stack in $hudTrackStacks){
    $context=@($hudTrackContexts|Where-Object{$stack.Name.StartsWith("hud-track-colour-$_-",[StringComparison]::Ordinal)})
    if($context.Count -ne 1){throw 'HUD Track stack context differs'}
    $state=$stack.BaseName.Substring(("hud-track-colour-$($context[0])-").Length);$bytes=[IO.File]::ReadAllBytes($stack.FullName)
    if($bytes.Length -ne 18446){throw 'HUD Track paired P6 byte length differs'}
    for($i=0;$i -lt $hudTrackHeader.Length;++$i){if($bytes[$i] -ne $hudTrackHeader[$i]){throw 'HUD Track canonical paired P6 header differs'}}
    $payload=[byte[]]::new(18432);[Array]::Copy($bytes,14,$payload,0,18432)
    $actualPayloadHash=Get-PainterSha256 $payload
    $modelNames=[Collections.Generic.List[string]]::new();$classes=[ordered]@{}
    foreach($modelName in $hudTrackModels){
        $rgba=New-HudTrackExpectedRGBA $context[0] $state $modelName;$expected=Get-HudTrackRGB $rgba
        $digest=Get-PainterSha256 $expected
        if($digest -cnotmatch '^[0-9A-F]{64}$'){throw 'HUD Track empty/malformed source equivalence digest'}
        if(!$classes.Contains($digest)){$classes[$digest]=[Collections.Generic.List[string]]::new()}
        $classes[$digest].Add($modelName)
        $same=$true
        for($offset=0;$offset -lt 18432;++$offset){if($payload[$offset] -ne $expected[$offset]){$same=$false;break}}
        if($same -and $actualPayloadHash -ceq $digest){$modelNames.Add($modelName)}
    }
    if($modelNames.Count -eq 0){throw "HUD Track retained RGB stack differs: $($stack.Name) matches no whole composite source model; per-pixel/per-track mixing is forbidden"}
    $expectedClasses=if($state -ceq 'rgba128' -and $context[0] -cne 'full'){2}else{1}
    if($classes.Count -ne $expectedClasses){throw 'HUD Track corrected source equivalence class count differs'}
    $hudTrackClassCount+=$classes.Count;if($classes.Count -eq 2){++$hudTrackSplitCases}
    $nativeRow=@($hudTrackRows|Where-Object{$_.context -ceq $context[0] -and $_.state -ceq $state})[0]
    if(($modelNames.ToArray()-join '|') -cne $nativeRow.source_models){throw 'HUD Track retained composite/native source-model declaration differs'}
    # Explicit dictionary-key grouping; never Group-Object on OrderedDictionary
    # properties. Flat strings survive the outer runner's Depth5 serialization.
    $equivalenceStrings=@(foreach($classDigest in $classes.Keys){
        [string[]]$names=@($classes[$classDigest].ToArray());[Array]::Sort($names,[StringComparer]::Ordinal)
        $classDigest+':'+($names-join ',')
    })
    $pngPath=[IO.Path]::ChangeExtension($stack.FullName,'.png')
    if(Test-Path -LiteralPath $pngPath -PathType Leaf){
        $png=[IO.File]::ReadAllBytes($pngPath);$signature=@(137,80,78,71,13,10,26,10)
        if($png.Length -lt 33){throw 'HUD Track PNG header truncated'}
        for($i=0;$i -lt 8;++$i){if($png[$i] -ne $signature[$i]){throw 'HUD Track PNG signature differs'}}
        if([Text.Encoding]::ASCII.GetString($png,12,4) -cne 'IHDR'){throw 'HUD Track PNG lacks initial IHDR'}
        $pngWidth=0L;$pngHeight=0L
        for($i=0;$i -lt 4;++$i){$pngWidth=$pngWidth*256+$png[16+$i];$pngHeight=$pngHeight*256+$png[20+$i]}
        if($pngWidth -ne 192 -or $pngHeight -ne 32){throw 'HUD Track PNG paired-stack dimensions differ'}
    }
    $hudTrackP6Bytes+=$bytes.Length;$hudTrackCheckedPixels+=6144
    [ordered]@{name=$stack.Name;width=192;height=32;bytes=$bytes.Length;sha256=(Get-FileHash $stack.FullName).Hash;context=$context[0];state=$state;
        source_payload_sha256=$actualPayloadHash;matching_source_models=@($modelNames.ToArray());source_model_equivalence_classes=$equivalenceStrings;
        capture_kind='lossless-paired-ROI-stack';mapping='HEALTHrows0..15fromy64..79;PAINTrows16..31fromy122..137';
        admission='one exact complete BOTH-ROI source model; no per-pixel/per-track mixing or tolerance'}
})
if($hudTrackP6Bytes -ne 221352 -or $hudTrackCheckedPixels -ne 73728 -or $hudTrackClassCount -ne 14 -or $hudTrackSplitCases -ne 2){throw 'HUD Track composite/class/pixel/P6 totals differ'}
$hudTrackLog=@(Get-Content (Join-Path $run 'run.log'))
$hudTrackSaves=@($hudTrackLog|Where-Object{$_ -match 'Saving\.\.\.$'}).Count
$hudTrackSaveSuccess=@($hudTrackLog|Where-Object{$_ -ceq 'Save was successful.'}).Count
if(@(Get-Content (Join-Path $run 'run.err')|Where-Object{$_ -match '^save failure:'}).Count){throw 'Unexpected HUD Track private Save failure'}
$hudTrackArtifacts=[ordered]@{}
foreach($path in @($hudTrackSummaryPath,$hudTrackCSVPath)){$hudTrackArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item $path).Length;sha256=(Get-FileHash $path).Hash}}
$hudTrackArtifacts.crops=$hudTrackStackEvidence
$hudTrackObserved=[ordered]@{consumer=$hudTrackSummary;readbacks=54;csv_rows=12;crops=12;controls=6;track_discriminations=12;changed_control_pixels=27648;unchanged_control_pixels=9216;
    crop_pixels=73728;retained_rgb_pixels=$hudTrackCheckedPixels;retained_rgb_payload_bytes=221184;ppm_bytes=$hudTrackP6Bytes;
    capture_kind='lossless-paired-ROI-stack';retained_rgb_scope='two-disjoint192x16native-interiors; independent ordered ink/track/prefix';
    source_precision_models=@($hudTrackModels);source_equivalence_classes=14;source_split_cases=2;source_single_class_cases=10;
    precision_scope='four scoped source representatives only; no device/universalD3D11/fullHUD inference';
    precision_admission='one whole BOTH-ROI model per composite; no per-pixel/per-track mixing or blanket tolerance';
    full_cover_invariant_cases=4;fill_role_guard_cases=12;full_frame_scope='native copied-HUD adoption/repeat/exterior and full-cover invariance only';
    control_scope='native exact suffix/complement in each track and wrong-versus-correct raw exterior';
    local_updates=0;player_updates=0;level_updates=0;scene_updates=0;state_updates=0;input_updates=0;time_updates=0;input_polls=0;
    private_save_calls=$hudTrackSaves;successful_private_saves=$hudTrackSaveSuccess;failed_private_saves=0;artifacts=$hudTrackArtifacts}
if($debug){
    $hudTrackTunePath=Join-Path $captures 'hud-track-colour-tune-summary.json';$hudTrackTuneCSV=Join-Path $captures 'hud-track-colour-tune.csv'
    $hudTrackTranscript=Join-Path $captures 'hud-track-colour-tune-commands.log'
    $hudTrackPrefs=Join-Path $captures 'hud-track-colour-tune-scratch/profile/ColourWars/save_data.json'
    $hudTrackTune=Get-Content $hudTrackTunePath -Raw|ConvertFrom-Json;$hudTrackTuneRows=@(Import-Csv $hudTrackTuneCSV)
    $hudTrackTuneCounts=[ordered]@{commands=9;states=9;contexts=1;readbacks=36;csv_rows=9;crops=0;saved_tokens=1;private_seed_saves=2;fill_role_guard_cases=9}
    Assert-HudTrackKeys $hudTrackTune (@($hudTrackTuneCounts.Keys)+$hudTrackUpdates+@('scope','event_fade','paired_context')) 'Debug'
    foreach($key in $hudTrackTuneCounts.Keys){Assert-HudTrackInteger $hudTrackTune $key $hudTrackTuneCounts[$key]}
    Assert-HudTrackUpdates $hudTrackTune
    Assert-HudTrackPairs $hudTrackTuneRows @('baseline','named','reset-original','rgb','rgba128','alpha0','reset-saved','rgb-after-save','reset-all-saved') @('half')
    $hudTrackTuneColumns=@('state','context','background','fill_width','roi_rgba_match','adoption_match','repeat_match','literal_channels','delta_suffix',
        'full_cover_invariant','fill_roles_unchanged','frozen_inputs_resources_rng','source_models','private_bytes','adoption')
    if($hudTrackTune.scope -cne 'debug-validated-commands' -or $hudTrackTune.event_fade -isnot [bool] -or $hudTrackTune.event_fade -ne $false -or
        $hudTrackTune.paired_context -cne 'half' -or ($hudTrackTuneRows[0].PSObject.Properties.Name-join '|') -cne ($hudTrackTuneColumns-join '|')){throw 'HUD Track Debug oracle/schema/paired-context differs'}
    foreach($row in $hudTrackTuneRows){
        Assert-HudTrackFlags $row
        Assert-HudTrackSourceNames $row.source_models ($row.state -cin @('rgba128','reset-saved','reset-all-saved'))
        if($row.fill_width -cne '96' -or $row.full_cover_invariant -cne 'not-applicable' -or $row.private_bytes -cne 'true' -or $row.adoption -cne 'live'){throw 'HUD Track Debug half-prefix/private-byte/live flag differs'}
    }
    if(!(Test-Path $hudTrackTranscript -PathType Leaf) -or (Get-Item $hudTrackTranscript).Length -le 0 -or
        !(Test-Path $hudTrackPrefs -PathType Leaf) -or (Get-Item $hudTrackPrefs).Length -le 0 -or $hudTrackSaves -ne 2 -or $hudTrackSaveSuccess -ne 2){throw 'HUD Track Debug transcript/preference seed Save evidence differs'}
    # Native owns actual one-token Save/rebase/full-HUD original/saved resets,
    # private source/staged bytes, default display/JumpJ and empty queues.
    foreach($path in @($hudTrackTunePath,$hudTrackTuneCSV,$hudTrackTranscript,$hudTrackPrefs)){$hudTrackArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item $path).Length;sha256=(Get-FileHash $path).Hash}}
    $hudTrackObserved.developer_commands=$hudTrackTune;$hudTrackObserved.readbacks=90;$hudTrackObserved.csv_rows=21
}elseif($hudTrackSaves -ne 0 -or $hudTrackSaveSuccess -ne 0 -or @(Get-ChildItem $captures -Filter 'hud-track-colour-tune-*').Count){throw 'Release HUD Track proof must remain consumer-only without developer artifacts or private Saves'}
$result.observed_counts=$hudTrackObserved
