# Dot-sourced only after -ActingTuneColoursOnly native execution.
$actingSummaryPath=Join-Path $captures 'acting-colour-consumer-summary.json'
$actingRowsPath=Join-Path $captures 'acting-colour-consumers.csv'
$actingSummary=Get-Content -LiteralPath $actingSummaryPath -Raw|ConvertFrom-Json
$actingRows=@(Import-Csv -LiteralPath $actingRowsPath)
function Assert-ActingUpdates($Summary){foreach($key in @('local_updates','player_updates','level_updates','state_updates','input_updates','time_updates')){
    if($Summary.$key -ne 0){throw "Acting unexpectedly claims updates: $key"}
}}
function Assert-ActingCases($Rows,$States,$Facings,$Backgrounds){
    $wanted=@(foreach($state in $States){foreach($facing in $Facings){foreach($background in $Backgrounds){"$state/$facing/$background"}}})
    $actual=@($Rows|ForEach-Object {"$($_.state)/$($_.facing)/$($_.background)"})
    if($Rows.Count -ne $States.Count*$Facings.Count*$Backgrounds.Count -or (($actual|Sort-Object)-join '|') -cne (($wanted|Sort-Object)-join '|')){
        throw 'Acting state/facing/Canvas cases differ'
    }
}
Assert-ActingUpdates $actingSummary
Assert-ActingCases $actingRows @('default','rgb','rgba','alpha0') @('left','right') @('black','nonblack')
$actingColumns=@('state','facing','background','full_raw_match','adoption_match','mask_guards','pupils_retained','frozen_records_strokes_rng','controls','scope')
if($actingSummary.scope -cne 'consumer-only' -or $actingSummary.readbacks -ne 90 -or $actingSummary.csv_rows -ne 16 -or $actingSummary.crops -ne 16 -or
    $actingSummary.controls -ne 24 -or $actingSummary.blank_reads -ne 2 -or $actingSummary.straight_face_controls -ne 8 -or $actingSummary.straight_sweat_controls -ne 8 -or
    $actingSummary.squared_face_controls -ne 4 -or $actingSummary.squared_sweat_controls -ne 4 -or
    ($actingSummary.figure-join ',') -cne '100,100,340,600' -or (@($actingSummary.eye_masks|ForEach-Object {$_-join ','})-join '|') -cne '220,220,20,40|300,220,20,40' -or
    $actingSummary.sweat_strips_per_facing -ne 8 -or $actingSummary.facings -ne 2 -or $actingSummary.canvas_backgrounds -ne 2 -or
    $actingSummary.viewport -cne '1280x720' -or $actingSummary.event_fade -ne $false -or $actingSummary.adoption_reference -cne 'production-consumer-only' -or
    $actingSummary.alpha_reference -cne 'independent-ordered-full-raw' -or
    (($actingRows[0].PSObject.Properties.Name)-join '|') -cne ($actingColumns-join '|') -or
    @($actingRows|Where-Object {$_.full_raw_match -cne 'true' -or $_.adoption_match -cne 'true' -or $_.mask_guards -cne 'true' -or $_.pupils_retained -cne 'true' -or
        $_.frozen_records_strokes_rng -cne 'true' -or $_.scope -cne 'consumer-only'}).Count){throw 'Acting shared exact budget/oracle/schema mismatch'}
$actingControlLabels=@{default='opaque-parity';rgb='opaque-parity';rgba='each-field-straight-and-squared-rejected';alpha0='each-field-straight-rejected-pupils-retained'}
foreach($row in $actingRows){if($row.controls -cne $actingControlLabels[$row.state]){throw 'Acting per-field control label mismatch'}}
$actingCrops=@(Get-ChildItem -LiteralPath $captures -Filter 'acting-colour-*.ppm' -File|Sort-Object Name)
$actingNames=@(foreach($state in @('default','rgb','rgba','alpha0')){foreach($facing in @('left','right')){foreach($background in @('black','nonblack')){"acting-colour-$state-$facing-$background.ppm"}}})
if($actingCrops.Count -ne 16 -or (($actingCrops.Name|Sort-Object)-join '|') -cne (($actingNames|Sort-Object)-join '|') -or
    @(Get-ChildItem -LiteralPath $captures -Filter 'acting-failure-*' -File).Count){throw 'Acting crop cases or failure artifacts differ'}
$actingHeader=[Text.Encoding]::ASCII.GetBytes("P6`n320 128`n255`n");$actingBytes=0
$actingCropEvidence=@(foreach($crop in $actingCrops){
    $bytes=[IO.File]::ReadAllBytes($crop.FullName)
    if($bytes.Length -ne $actingHeader.Length+320*128*3){throw 'Acting compact eye+sweat crop size mismatch'}
    for($i=0;$i -lt $actingHeader.Length;$i++){if($bytes[$i] -ne $actingHeader[$i]){throw 'Acting PPM header mismatch'}}
    $actingBytes+=$bytes.Length
    [ordered]@{name=$crop.Name;bytes=$bytes.Length;sha256=(Get-FileHash -LiteralPath $crop.FullName).Hash}
})
$actingLog=@(Get-Content -LiteralPath (Join-Path $run 'run.log'))
$actingSaves=@($actingLog|Where-Object {$_ -match 'Saving\.\.\.$'}).Count
$actingSuccess=@($actingLog|Where-Object {$_ -ceq 'Save was successful.'}).Count
if(@(Get-Content -LiteralPath (Join-Path $run 'run.err')|Where-Object {$_ -match '^save failure:'}).Count){throw 'Unexpected acting private Save failure'}
$actingArtifacts=[ordered]@{}
foreach($path in @($actingSummaryPath,$actingRowsPath)){$actingArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}}
$actingArtifacts.crops=$actingCropEvidence
$actingObserved=[ordered]@{consumer=$actingSummary;readbacks=90;csv_rows=16;crops=16;controls=24;crop_pixels=655360;ppm_bytes=$actingBytes;
    local_updates=0;player_updates=0;level_updates=0;state_updates=0;input_updates=0;time_updates=0;
    private_save_calls=$actingSaves;successful_private_saves=$actingSuccess;failed_private_saves=0;artifacts=$actingArtifacts}
if($debug){
    $actingTuneSummaryPath=Join-Path $captures 'acting-colour-tune-summary.json';$actingTuneRowsPath=Join-Path $captures 'acting-colour-tune.csv'
    $actingTranscript=Join-Path $captures 'acting-colour-tune-commands.log'
    $actingTune=Get-Content -LiteralPath $actingTuneSummaryPath -Raw|ConvertFrom-Json;$actingTuneRows=@(Import-Csv -LiteralPath $actingTuneRowsPath)
    Assert-ActingUpdates $actingTune
    Assert-ActingCases $actingTuneRows @('baseline','named-face','named-both','rgb-face','rgb-both','rgba-face','rgba-both','face-reset-original','face-restored-rgba','sweat-reset-original','face-alpha0','alpha0-both','face-reset-saved','reset-all-saved') @('right') @('black')
    $columns=@('state','facing','background','full_raw_match','adoption_match','frozen_records_strokes_rng','private_bytes','adoption')
    if($actingTune.scope -cne 'debug-validated-commands' -or $actingTune.commands -ne 14 -or $actingTune.states -ne 14 -or $actingTune.readbacks -ne 56 -or
        $actingTune.csv_rows -ne 14 -or $actingTune.crops -ne 0 -or $actingTune.saved_tokens -ne 1 -or $actingTune.facings -ne 1 -or $actingTune.canvas_backgrounds -ne 1 -or
        (($actingTuneRows[0].PSObject.Properties.Name)-join '|') -cne ($columns-join '|') -or
        @($actingTuneRows|Where-Object {$_.full_raw_match -cne 'true' -or $_.adoption_match -cne 'true' -or $_.frozen_records_strokes_rng -cne 'true' -or
            $_.private_bytes -cne 'true' -or $_.adoption -cne 'live'}).Count -or (Get-Item -LiteralPath $actingTranscript).Length -le 0 -or
        $actingSaves -ne 2 -or $actingSuccess -ne 2 -or
        !(Test-Path -LiteralPath (Join-Path $captures 'acting-colour-tune-scratch/profile/ColourWars/save_data.json') -PathType Leaf)){
        throw 'Acting Debug exact budget/schema/private-save evidence mismatch'
    }
    foreach($path in @($actingTuneSummaryPath,$actingTuneRowsPath,$actingTranscript)){$actingArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}}
    $actingObserved.developer_commands=$actingTune;$actingObserved.readbacks=146;$actingObserved.csv_rows=30
}elseif($actingSaves -ne 0 -or $actingSuccess -ne 0 -or @(Get-ChildItem -LiteralPath $captures -Filter 'acting-colour-tune-*').Count){
    throw 'Release acting proof must remain literal-consumer-only without developer artifacts or Save'
}
$result.observed_counts=$actingObserved
