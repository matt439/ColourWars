# Dot-sourced only after -ProgramTuneColourOnly native execution.
$programSummaryPath=Join-Path $captures 'program-colour-consumer-summary.json'
$programRowsPath=Join-Path $captures 'program-colour-consumers.csv'
$programSummary=Get-Content -LiteralPath $programSummaryPath -Raw|ConvertFrom-Json
$programRows=@(Import-Csv -LiteralPath $programRowsPath)
function Assert-ProgramUpdates($Summary){foreach($key in @('local_updates','player_updates','level_updates','state_updates','input_updates','time_updates')){
    if($Summary.$key -ne 0){throw "Program unexpectedly claims updates: $key"}
}}
function Assert-ProgramPairs($Rows,$States){
    $wanted=@(foreach($state in $States){foreach($background in @('black')){"$state/$background"}})
    $actual=@($Rows|ForEach-Object {"$($_.state)/$($_.background)"})
    if($Rows.Count -ne $States.Count -or (($actual|Sort-Object)-join '|') -cne (($wanted|Sort-Object)-join '|')){throw 'Program state/underlay pairs differ'}
}
Assert-ProgramUpdates $programSummary
Assert-ProgramPairs $programRows @('default','rgb','rgba128','alpha0')
$programColumns=@('state','background','roi_raw_match','adoption_match','glass_guards','frozen_style_persona_rng','control','scope')
if($programSummary.scope -cne 'consumer-only' -or $programSummary.readbacks -ne 20 -or $programSummary.csv_rows -ne 4 -or
    $programSummary.crops -ne 4 -or $programSummary.controls -ne 3 -or $programSummary.glass_only_reads -ne 1 -or
    ($programSummary.roi-join ',') -cne '135,58,1009,482' -or ($programSummary.inner-join ',') -cne '136,59,1007,480' -or $programSummary.viewport -cne '1280x720' -or
    $programSummary.event_fade -ne $false -or $programSummary.adoption_reference -cne 'production-consumer-only' -or
    $programSummary.alpha_reference -cne 'independent-ordered-raw-roi' -or
    (($programRows[0].PSObject.Properties.Name)-join '|') -cne ($programColumns-join '|') -or
    @($programRows|Where-Object {$_.roi_raw_match -cne 'true' -or $_.adoption_match -cne 'true' -or $_.glass_guards -cne 'true' -or
        $_.frozen_style_persona_rng -cne 'true' -or $_.scope -cne 'consumer-only'}).Count){throw 'Program shared exact budget/oracle/schema mismatch'}
$programControls=@{default='opaque-parity';rgb='opaque-parity';rgba128='straight-and-squared-rejected';alpha0='straight-rejected-glass-retained'}
foreach($row in $programRows){if($row.control -cne $programControls[$row.state]){throw 'Program control label mismatch'}}
$programCrops=@(Get-ChildItem -LiteralPath $captures -Filter 'program-colour-*.ppm' -File|Sort-Object Name)
$programNames=@(foreach($state in @('default','rgb','rgba128','alpha0')){foreach($background in @('black')){"program-colour-$state-$background.ppm"}})
if($programCrops.Count -ne 4 -or (($programCrops.Name|Sort-Object)-join '|') -cne (($programNames|Sort-Object)-join '|') -or
    @(Get-ChildItem -LiteralPath $captures -Filter 'program-failure-*' -File).Count){throw 'Program crop case list or failure artifacts differ'}
$programHeader=[Text.Encoding]::ASCII.GetBytes("P6`n320 192`n255`n");$programBytes=0
$programCropEvidence=@(foreach($crop in $programCrops){
    $bytes=[IO.File]::ReadAllBytes($crop.FullName)
    if($bytes.Length -ne $programHeader.Length+320*192*3){throw 'Program compact corner montage size mismatch'}
    for($i=0;$i -lt $programHeader.Length;$i++){if($bytes[$i] -ne $programHeader[$i]){throw 'Program PPM header mismatch'}}
    $programBytes+=$bytes.Length
    [ordered]@{name=$crop.Name;bytes=$bytes.Length;sha256=(Get-FileHash -LiteralPath $crop.FullName).Hash}
})
$programLog=@(Get-Content -LiteralPath (Join-Path $run 'run.log'))
$programSaves=@($programLog|Where-Object {$_ -match 'Saving\.\.\.$'}).Count
$programSuccess=@($programLog|Where-Object {$_ -ceq 'Save was successful.'}).Count
if(@(Get-Content -LiteralPath (Join-Path $run 'run.err')|Where-Object {$_ -match '^save failure:'}).Count){throw 'Unexpected program private Save failure'}
$programArtifacts=[ordered]@{}
foreach($path in @($programSummaryPath,$programRowsPath)){$programArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}}
$programArtifacts.crops=$programCropEvidence
$programObserved=[ordered]@{consumer=$programSummary;readbacks=20;csv_rows=4;crops=4;controls=3;crop_pixels=245760;ppm_bytes=$programBytes;
    local_updates=0;player_updates=0;level_updates=0;state_updates=0;input_updates=0;time_updates=0;
    private_save_calls=$programSaves;successful_private_saves=$programSuccess;failed_private_saves=0;artifacts=$programArtifacts}
if($debug){
    $programTuneSummaryPath=Join-Path $captures 'program-colour-tune-summary.json';$programTuneRowsPath=Join-Path $captures 'program-colour-tune.csv'
    $programTranscript=Join-Path $captures 'program-colour-tune-commands.log'
    $programTune=Get-Content -LiteralPath $programTuneSummaryPath -Raw|ConvertFrom-Json;$programTuneRows=@(Import-Csv -LiteralPath $programTuneRowsPath)
    Assert-ProgramUpdates $programTune
    Assert-ProgramPairs $programTuneRows @('baseline','named','reset-original','rgb','rgba128','alpha0','reset-saved','rgb-after-save','reset-all-saved')
    $columns=@('state','background','roi_raw_match','adoption_match','frozen_style_persona_rng','private_bytes','adoption')
    if($programTune.scope -cne 'debug-validated-commands' -or $programTune.commands -ne 9 -or $programTune.states -ne 9 -or
        $programTune.readbacks -ne 36 -or $programTune.csv_rows -ne 9 -or $programTune.crops -ne 0 -or $programTune.saved_tokens -ne 1 -or
        (($programTuneRows[0].PSObject.Properties.Name)-join '|') -cne ($columns-join '|') -or
        @($programTuneRows|Where-Object {$_.roi_raw_match -cne 'true' -or $_.adoption_match -cne 'true' -or
            $_.frozen_style_persona_rng -cne 'true' -or $_.private_bytes -cne 'true' -or $_.adoption -cne 'live'}).Count -or
        (Get-Item -LiteralPath $programTranscript).Length -le 0 -or $programSaves -ne 2 -or $programSuccess -ne 2 -or
        !(Test-Path -LiteralPath (Join-Path $captures 'program-colour-tune-scratch/profile/ColourWars/save_data.json') -PathType Leaf)){
        throw 'Program Debug budget/schema/private-save evidence mismatch'
    }
    foreach($path in @($programTuneSummaryPath,$programTuneRowsPath,$programTranscript)){$programArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}}
    $programObserved.developer_commands=$programTune;$programObserved.readbacks=56;$programObserved.csv_rows=13
}elseif($programSaves -ne 0 -or $programSuccess -ne 0 -or @(Get-ChildItem -LiteralPath $captures -Filter 'program-colour-tune-*').Count){
    throw 'Release program proof must stay literal-consumer-only without developer artifacts or Save'
}
$result.observed_counts=$programObserved
