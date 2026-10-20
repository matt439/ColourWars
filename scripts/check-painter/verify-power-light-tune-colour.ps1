# Dot-sourced only after -PowerLightTuneColourOnly native execution.
$powerLightSummaryPath=Join-Path $captures 'power-light-colour-consumer-summary.json'
$powerLightRowsPath=Join-Path $captures 'power-light-colour-consumers.csv'
$powerLightSummary=Get-Content -LiteralPath $powerLightSummaryPath -Raw|ConvertFrom-Json
$powerLightRows=@(Import-Csv -LiteralPath $powerLightRowsPath)
function Assert-PowerLightUpdates($Summary){foreach($key in @('local_updates','player_updates','level_updates','state_updates','input_updates','time_updates')){
    if($Summary.$key -ne 0){throw "PowerLight unexpectedly claims updates: $key"}
}}
function Assert-PowerLightPairs($Rows,$States){
    $wanted=@(foreach($state in $States){foreach($background in @('black','nonblack')){"$state/$background"}})
    $actual=@($Rows|ForEach-Object {"$($_.state)/$($_.background)"})
    if($Rows.Count -ne $States.Count*2 -or (($actual|Sort-Object)-join '|') -cne (($wanted|Sort-Object)-join '|')){throw 'PowerLight state/underlay pairs differ'}
}
Assert-PowerLightUpdates $powerLightSummary
Assert-PowerLightPairs $powerLightRows @('default','rgb','rgba128','alpha0')
$powerLightColumns=@('state','background','roi_raw_match','adoption_match','ink_border_exterior','frozen_style_persona_rng','control','scope')
if($powerLightSummary.scope -cne 'consumer-only' -or $powerLightSummary.readbacks -ne 40 -or $powerLightSummary.csv_rows -ne 8 -or
    $powerLightSummary.crops -ne 8 -or $powerLightSummary.controls -ne 6 -or $powerLightSummary.blank_reads -ne 2 -or
    ($powerLightSummary.roi-join ',') -cne '1237,642,32,32' -or ($powerLightSummary.outer-join ',') -cne '1246,652,10,10' -or ($powerLightSummary.inner-join ',') -cne '1249,655,5,4' -or $powerLightSummary.viewport -cne '1280x720' -or
    $powerLightSummary.event_fade -ne $false -or $powerLightSummary.adoption_reference -cne 'production-consumer-only' -or
    $powerLightSummary.alpha_reference -cne 'independent-ordered-raw-roi' -or
    (($powerLightRows[0].PSObject.Properties.Name)-join '|') -cne ($powerLightColumns-join '|') -or
    @($powerLightRows|Where-Object {$_.roi_raw_match -cne 'true' -or $_.adoption_match -cne 'true' -or $_.ink_border_exterior -cne 'true' -or
        $_.frozen_style_persona_rng -cne 'true' -or $_.scope -cne 'consumer-only'}).Count){throw 'PowerLight shared exact budget/oracle/schema mismatch'}
$powerLightControls=@{default='opaque-parity';rgb='opaque-parity';rgba128='straight-and-squared-rejected';alpha0='straight-rejected-ink-retained'}
foreach($row in $powerLightRows){if($row.control -cne $powerLightControls[$row.state]){throw 'PowerLight control label mismatch'}}
$powerLightCrops=@(Get-ChildItem -LiteralPath $captures -Filter 'power-light-colour-*.ppm' -File|Sort-Object Name)
$powerLightNames=@(foreach($state in @('default','rgb','rgba128','alpha0')){foreach($background in @('black','nonblack')){"power-light-colour-$state-$background.ppm"}})
if($powerLightCrops.Count -ne 8 -or (($powerLightCrops.Name|Sort-Object)-join '|') -cne (($powerLightNames|Sort-Object)-join '|') -or
    @(Get-ChildItem -LiteralPath $captures -Filter 'power-light-failure-*' -File).Count){throw 'PowerLight crop case list or failure artifacts differ'}
$powerLightHeader=[Text.Encoding]::ASCII.GetBytes("P6`n32 32`n255`n");$powerLightBytes=0
$powerLightCropEvidence=@(foreach($crop in $powerLightCrops){
    $bytes=[IO.File]::ReadAllBytes($crop.FullName)
    if($bytes.Length -ne $powerLightHeader.Length+32*32*3){throw 'PowerLight compact indicator ROI size mismatch'}
    for($i=0;$i -lt $powerLightHeader.Length;$i++){if($bytes[$i] -ne $powerLightHeader[$i]){throw 'PowerLight PPM header mismatch'}}
    $powerLightBytes+=$bytes.Length
    [ordered]@{name=$crop.Name;bytes=$bytes.Length;sha256=(Get-FileHash -LiteralPath $crop.FullName).Hash}
})
$powerLightLog=@(Get-Content -LiteralPath (Join-Path $run 'run.log'))
$powerLightSaves=@($powerLightLog|Where-Object {$_ -match 'Saving\.\.\.$'}).Count
$powerLightSuccess=@($powerLightLog|Where-Object {$_ -ceq 'Save was successful.'}).Count
if(@(Get-Content -LiteralPath (Join-Path $run 'run.err')|Where-Object {$_ -match '^save failure:'}).Count){throw 'Unexpected powerLight private Save failure'}
$powerLightArtifacts=[ordered]@{}
foreach($path in @($powerLightSummaryPath,$powerLightRowsPath)){$powerLightArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}}
$powerLightArtifacts.crops=$powerLightCropEvidence
$powerLightObserved=[ordered]@{consumer=$powerLightSummary;readbacks=40;csv_rows=8;crops=8;controls=6;crop_pixels=8192;ppm_bytes=$powerLightBytes;
    local_updates=0;player_updates=0;level_updates=0;state_updates=0;input_updates=0;time_updates=0;
    private_save_calls=$powerLightSaves;successful_private_saves=$powerLightSuccess;failed_private_saves=0;artifacts=$powerLightArtifacts}
if($debug){
    $powerLightTuneSummaryPath=Join-Path $captures 'power-light-colour-tune-summary.json';$powerLightTuneRowsPath=Join-Path $captures 'power-light-colour-tune.csv'
    $powerLightTranscript=Join-Path $captures 'power-light-colour-tune-commands.log'
    $powerLightTune=Get-Content -LiteralPath $powerLightTuneSummaryPath -Raw|ConvertFrom-Json;$powerLightTuneRows=@(Import-Csv -LiteralPath $powerLightTuneRowsPath)
    Assert-PowerLightUpdates $powerLightTune
    Assert-PowerLightPairs $powerLightTuneRows @('baseline','named','reset-original','rgb','rgba128','alpha0','reset-saved','rgb-after-save','reset-all-saved')
    $columns=@('state','background','roi_raw_match','adoption_match','frozen_style_persona_rng','private_bytes','adoption')
    if($powerLightTune.scope -cne 'debug-validated-commands' -or $powerLightTune.commands -ne 9 -or $powerLightTune.states -ne 9 -or
        $powerLightTune.readbacks -ne 72 -or $powerLightTune.csv_rows -ne 18 -or $powerLightTune.crops -ne 0 -or $powerLightTune.saved_tokens -ne 1 -or
        (($powerLightTuneRows[0].PSObject.Properties.Name)-join '|') -cne ($columns-join '|') -or
        @($powerLightTuneRows|Where-Object {$_.roi_raw_match -cne 'true' -or $_.adoption_match -cne 'true' -or
            $_.frozen_style_persona_rng -cne 'true' -or $_.private_bytes -cne 'true' -or $_.adoption -cne 'live'}).Count -or
        (Get-Item -LiteralPath $powerLightTranscript).Length -le 0 -or $powerLightSaves -ne 2 -or $powerLightSuccess -ne 2 -or
        !(Test-Path -LiteralPath (Join-Path $captures 'power-light-colour-tune-scratch/profile/ColourWars/save_data.json') -PathType Leaf)){
        throw 'PowerLight Debug budget/schema/private-save evidence mismatch'
    }
    foreach($path in @($powerLightTuneSummaryPath,$powerLightTuneRowsPath,$powerLightTranscript)){$powerLightArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}}
    $powerLightObserved.developer_commands=$powerLightTune;$powerLightObserved.readbacks=112;$powerLightObserved.csv_rows=26
}elseif($powerLightSaves -ne 0 -or $powerLightSuccess -ne 0 -or @(Get-ChildItem -LiteralPath $captures -Filter 'power-light-colour-tune-*').Count){
    throw 'Release powerLight proof must stay literal-consumer-only without developer artifacts or Save'
}
$result.observed_counts=$powerLightObserved
