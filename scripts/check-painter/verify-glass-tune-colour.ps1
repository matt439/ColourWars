# Dot-sourced only after -GlassTuneColourOnly native execution.
# Main raw proof is five ordered ROIs, never a full raw Monitor reference.
$glassSummaryPath=Join-Path $captures 'glass-colour-consumer-summary.json'
$glassRowsPath=Join-Path $captures 'glass-colour-consumers.csv'
$glassSummary=Get-Content -LiteralPath $glassSummaryPath -Raw|ConvertFrom-Json
$glassRows=@(Import-Csv -LiteralPath $glassRowsPath)
function Assert-GlassUpdates($Summary){foreach($key in @('local_updates','player_updates','level_updates','state_updates','input_updates','time_updates')){
    if($Summary.$key -is [string] -or $Summary.$key -is [bool] -or $Summary.$key -ne 0){throw "Glass unexpectedly claims updates: $key"}
}}
function Assert-GlassPairs($Rows,$States,[bool]$TwoStandalone){
    $wanted=@(foreach($method in @('main','picker','close')){foreach($state in $States){
        $backgrounds=if($method -eq 'main'){@('opaque-beige')}elseif($TwoStandalone){@('black','nonblack')}else{@('black')}
        foreach($background in $backgrounds){"$state/$method/$background"}
    }})
    $actual=@($Rows|ForEach-Object {"$($_.state)/$($_.method)/$($_.background)"})
    if($Rows.Count -ne $wanted.Count -or (($actual|Sort-Object)-join '|') -cne (($wanted|Sort-Object)-join '|')){throw 'Glass exact state/method/effective-backdrop pairs differ'}
}
Assert-GlassUpdates $glassSummary
Assert-GlassPairs $glassRows @('default','rgb','rgba128','alpha0') $true
$glassColumns=@('state','method','background','roi_raw_match','adoption_match','guards','frozen_style_persona_rng','controls','scope')
$glassCounts=[ordered]@{readbacks=100;csv_rows=20;crops=20;crop_pixels=55272;blank_reads=2;partial_controls=12;alpha0_controls=6;
    main_cases=4;picker_cases=8;close_cases=8;main_effective_backdrops=1;standalone_backdrops=2;selected_writes=4;main_raw_rois=5;close_x_dabs=24}
foreach($key in $glassCounts.Keys){if($glassSummary.$key -is [string] -or $glassSummary.$key -is [bool] -or $glassSummary.$key -ne $glassCounts[$key]){throw "Glass shared budget differs: $key"}}
if($glassSummary.scope -cne 'consumer-only' -or $glassSummary.viewport -cne '1280x720' -or $glassSummary.event_fade -isnot [bool] -or $glassSummary.event_fade -ne $false -or
    $glassSummary.adoption_reference -cne 'production-consumer-only' -or $glassSummary.alpha_reference -cne 'independent-ordered-main-five-rois-and-standalone-rois' -or
    ($glassSummary.main_crop-join ',') -cne '80,24' -or ($glassSummary.picker_crop-join ',') -cne '99,53' -or ($glassSummary.close_crop-join ',') -cne '27,26' -or
    ($glassRows[0].PSObject.Properties.Name-join '|') -cne ($glassColumns-join '|') -or
    @($glassRows|Where-Object {$_.roi_raw_match -cne 'true' -or $_.adoption_match -cne 'true' -or $_.guards -cne 'true' -or
        $_.frozen_style_persona_rng -cne 'true' -or $_.scope -cne 'consumer-only'}).Count){throw 'Glass shared exact oracle/schema/scope mismatch'}
$glassPartial=0;$glassZero=0
foreach($row in $glassRows){
    $writes=if($row.method -ceq 'main'){2}else{1}
    $wanted=if($row.state -ceq 'rgba128'){$writes*2}elseif($row.state -ceq 'alpha0'){$writes}else{0}
    if($row.controls -cne [string]$wanted){throw 'Glass independent per-write control count differs'}
    if($row.state -ceq 'rgba128'){$glassPartial+=[int]$row.controls}elseif($row.state -ceq 'alpha0'){$glassZero+=[int]$row.controls}
}
if($glassPartial -ne 12 -or $glassZero -ne 6){throw 'Glass per-write partial/alpha0 control sum differs'}
$glassCrops=@(Get-ChildItem -LiteralPath $captures -Filter 'glass-colour-*.ppm' -File|Sort-Object Name)
$glassNames=@(foreach($method in @('main','picker','close')){foreach($state in @('default','rgb','rgba128','alpha0')){
    $backgrounds=if($method -ceq 'main'){@('opaque-beige')}else{@('black','nonblack')}
    foreach($background in $backgrounds){"glass-colour-$method-$state-$background.ppm"}
}})
if($glassCrops.Count -ne 20 -or (($glassCrops.Name|Sort-Object)-join '|') -cne (($glassNames|Sort-Object)-join '|') -or
    @(Get-ChildItem -LiteralPath $captures -Filter 'glass-failure*' -File).Count){throw 'Glass exact compact-crop list or failure artifacts differ'}
$glassBytes=0;$glassPixels=0
$glassCropEvidence=@(foreach($crop in $glassCrops){
    $size=if($crop.Name -like 'glass-colour-main-*'){@(80,24)}elseif($crop.Name -like 'glass-colour-picker-*'){@(99,53)}else{@(27,26)}
    $header=[Text.Encoding]::ASCII.GetBytes("P6`n$($size[0]) $($size[1])`n255`n")
    $bytes=[IO.File]::ReadAllBytes($crop.FullName)
    if($bytes.Length -ne $header.Length+$size[0]*$size[1]*3){throw 'Glass lossless native crop size differs'}
    for($i=0;$i -lt $header.Length;++$i){if($bytes[$i] -ne $header[$i]){throw 'Glass PPM header differs'}}
    $glassBytes+=$bytes.Length;$glassPixels+=$size[0]*$size[1]
    [ordered]@{name=$crop.Name;width=$size[0];height=$size[1];bytes=$bytes.Length;sha256=(Get-FileHash -LiteralPath $crop.FullName).Hash}
})
if($glassBytes -ne 166076 -or $glassPixels -ne 55272){throw 'Glass compact crop total byte/pixel count differs'}
$glassLog=@(Get-Content -LiteralPath (Join-Path $run 'run.log'))
$glassSaves=@($glassLog|Where-Object {$_ -match 'Saving\.\.\.$'}).Count
$glassSuccess=@($glassLog|Where-Object {$_ -ceq 'Save was successful.'}).Count
if(@(Get-Content -LiteralPath (Join-Path $run 'run.err')|Where-Object {$_ -match '^save failure:'}).Count){throw 'Unexpected Glass private Save failure'}
$glassArtifacts=[ordered]@{}
foreach($path in @($glassSummaryPath,$glassRowsPath)){$glassArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}}
$glassArtifacts.crops=$glassCropEvidence
$glassObserved=[ordered]@{consumer=$glassSummary;readbacks=100;csv_rows=20;crops=20;controls=18;partial_controls=12;alpha0_controls=6;
    crop_pixels=55272;ppm_bytes=$glassBytes;local_updates=0;player_updates=0;level_updates=0;state_updates=0;input_updates=0;time_updates=0;
    private_save_calls=$glassSaves;successful_private_saves=$glassSuccess;failed_private_saves=0;artifacts=$glassArtifacts}
if($debug){
    $glassTuneSummaryPath=Join-Path $captures 'glass-colour-tune-summary.json';$glassTuneRowsPath=Join-Path $captures 'glass-colour-tune.csv'
    $glassTranscript=Join-Path $captures 'glass-colour-tune-commands.log'
    $glassTune=Get-Content -LiteralPath $glassTuneSummaryPath -Raw|ConvertFrom-Json;$glassTuneRows=@(Import-Csv -LiteralPath $glassTuneRowsPath)
    Assert-GlassUpdates $glassTune
    Assert-GlassPairs $glassTuneRows @('baseline','named','reset-original','rgb','rgba128','alpha0','reset-saved','rgb-after-save','reset-all-saved') $false
    $columns=@('state','method','background','roi_raw_match','adoption_match','frozen_style_persona_rng','private_bytes','adoption')
    if($glassTune.scope -cne 'debug-validated-commands' -or $glassTune.commands -ne 9 -or $glassTune.states -ne 9 -or $glassTune.methods -ne 3 -or
        $glassTune.readbacks -ne 108 -or $glassTune.csv_rows -ne 27 -or $glassTune.crops -ne 0 -or $glassTune.saved_tokens -ne 1 -or $glassTune.standalone_backdrops -ne 1 -or
        ($glassTuneRows[0].PSObject.Properties.Name-join '|') -cne ($columns-join '|') -or
        @($glassTuneRows|Where-Object {$_.roi_raw_match -cne 'true' -or $_.adoption_match -cne 'true' -or
            $_.frozen_style_persona_rng -cne 'true' -or $_.private_bytes -cne 'true' -or $_.adoption -cne 'live'}).Count -or
        (Get-Item -LiteralPath $glassTranscript).Length -le 0 -or $glassSaves -ne 2 -or $glassSuccess -ne 2 -or
        !(Test-Path -LiteralPath (Join-Path $captures 'glass-colour-tune-scratch/profile/ColourWars/save_data.json') -PathType Leaf)){
        throw 'Glass Debug exact budget/schema/private-save evidence mismatch'
    }
    foreach($path in @($glassTuneSummaryPath,$glassTuneRowsPath,$glassTranscript)){$glassArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}}
    $glassObserved.developer_commands=$glassTune;$glassObserved.readbacks=208;$glassObserved.csv_rows=47
}elseif($glassSaves -ne 0 -or $glassSuccess -ne 0 -or @(Get-ChildItem -LiteralPath $captures -Filter 'glass-colour-tune-*').Count){
    throw 'Release Glass proof must stay literal-consumer-only without developer artifacts or Save'
}
$result.observed_counts=$glassObserved
