# Dot-sourced only after -DesktopTuneColourOnly native execution.
$desktopSummaryPath=Join-Path $captures 'desktop-colour-consumer-summary.json'
$desktopRowsPath=Join-Path $captures 'desktop-colour-consumers.csv'
$desktopSummary=Get-Content -LiteralPath $desktopSummaryPath -Raw|ConvertFrom-Json
$desktopRows=@(Import-Csv -LiteralPath $desktopRowsPath)
function Assert-DesktopUpdates($Summary){foreach($key in @('local_updates','player_updates','level_updates','state_updates','input_updates','time_updates')){
    if($Summary.$key -ne 0){throw "Desktop unexpectedly claims updates: $key"}
}}
function Assert-DesktopPairs($Rows,$States){
    $wanted=@(foreach($state in $States){foreach($background in @('black','nonblack')){"$state/$background"}})
    $actual=@($Rows|ForEach-Object {"$($_.state)/$($_.background)"})
    if($Rows.Count -ne $States.Count*2 -or (($actual|Sort-Object)-join '|') -cne (($wanted|Sort-Object)-join '|')){throw 'Desktop state/underlay pairs differ'}
}
Assert-DesktopUpdates $desktopSummary
Assert-DesktopPairs $desktopRows @('default','rgb','rgba128','alpha0')
$desktopColumns=@('state','background','full_raw_match','adoption_match','edges_exterior','frozen_style_persona_rng','control','scope')
if($desktopSummary.scope -cne 'consumer-only' -or $desktopSummary.readbacks -ne 40 -or $desktopSummary.csv_rows -ne 8 -or
    $desktopSummary.crops -ne 8 -or $desktopSummary.controls -ne 6 -or $desktopSummary.blank_reads -ne 2 -or
    ($desktopSummary.rectangle-join ',') -cne '136,59,1007,529' -or $desktopSummary.viewport -cne '1280x720' -or
    $desktopSummary.event_fade -ne $false -or $desktopSummary.adoption_reference -cne 'production-consumer-only' -or
    $desktopSummary.alpha_reference -cne 'independent-full-raw-rectangle' -or
    (($desktopRows[0].PSObject.Properties.Name)-join '|') -cne ($desktopColumns-join '|') -or
    @($desktopRows|Where-Object {$_.full_raw_match -cne 'true' -or $_.adoption_match -cne 'true' -or $_.edges_exterior -cne 'true' -or
        $_.frozen_style_persona_rng -cne 'true' -or $_.scope -cne 'consumer-only'}).Count){throw 'Desktop shared exact budget/oracle/schema mismatch'}
$desktopControls=@{default='opaque-default-parity';rgb='opaque-default-parity';rgba128='straight-and-squared-rejected';alpha0='straight-rejected-blank'}
foreach($row in $desktopRows){if($row.control -cne $desktopControls[$row.state]){throw 'Desktop control label mismatch'}}
$desktopCrops=@(Get-ChildItem -LiteralPath $captures -Filter 'desktop-colour-*.ppm' -File|Sort-Object Name)
$desktopNames=@(foreach($state in @('default','rgb','rgba128','alpha0')){foreach($background in @('black','nonblack')){"desktop-colour-$state-$background.ppm"}})
if($desktopCrops.Count -ne 8 -or (($desktopCrops.Name|Sort-Object)-join '|') -cne (($desktopNames|Sort-Object)-join '|') -or
    @(Get-ChildItem -LiteralPath $captures -Filter 'desktop-failure-*' -File).Count){throw 'Desktop crop case list or failure artifacts differ'}
$desktopHeader=[Text.Encoding]::ASCII.GetBytes("P6`n320 192`n255`n");$desktopBytes=0
$desktopCropEvidence=@(foreach($crop in $desktopCrops){
    $bytes=[IO.File]::ReadAllBytes($crop.FullName)
    if($bytes.Length -ne $desktopHeader.Length+320*192*3){throw 'Desktop compact corner montage size mismatch'}
    for($i=0;$i -lt $desktopHeader.Length;$i++){if($bytes[$i] -ne $desktopHeader[$i]){throw 'Desktop PPM header mismatch'}}
    $desktopBytes+=$bytes.Length
    [ordered]@{name=$crop.Name;bytes=$bytes.Length;sha256=(Get-FileHash -LiteralPath $crop.FullName).Hash}
})
$desktopLog=@(Get-Content -LiteralPath (Join-Path $run 'run.log'))
$desktopSaves=@($desktopLog|Where-Object {$_ -match 'Saving\.\.\.$'}).Count
$desktopSuccess=@($desktopLog|Where-Object {$_ -ceq 'Save was successful.'}).Count
if(@(Get-Content -LiteralPath (Join-Path $run 'run.err')|Where-Object {$_ -match '^save failure:'}).Count){throw 'Unexpected desktop private Save failure'}
$desktopArtifacts=[ordered]@{}
foreach($path in @($desktopSummaryPath,$desktopRowsPath)){$desktopArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}}
$desktopArtifacts.crops=$desktopCropEvidence
$desktopObserved=[ordered]@{consumer=$desktopSummary;readbacks=40;csv_rows=8;crops=8;controls=6;crop_pixels=491520;ppm_bytes=$desktopBytes;
    local_updates=0;player_updates=0;level_updates=0;state_updates=0;input_updates=0;time_updates=0;
    private_save_calls=$desktopSaves;successful_private_saves=$desktopSuccess;failed_private_saves=0;artifacts=$desktopArtifacts}
if($debug){
    $desktopTuneSummaryPath=Join-Path $captures 'desktop-colour-tune-summary.json';$desktopTuneRowsPath=Join-Path $captures 'desktop-colour-tune.csv'
    $desktopTranscript=Join-Path $captures 'desktop-colour-tune-commands.log'
    $desktopTune=Get-Content -LiteralPath $desktopTuneSummaryPath -Raw|ConvertFrom-Json;$desktopTuneRows=@(Import-Csv -LiteralPath $desktopTuneRowsPath)
    Assert-DesktopUpdates $desktopTune
    Assert-DesktopPairs $desktopTuneRows @('baseline','named','reset-original','rgb','rgba128','alpha0','reset-saved','rgb-after-save','reset-all-saved')
    $columns=@('state','background','full_raw_match','adoption_match','frozen_style_persona_rng','private_bytes','adoption')
    if($desktopTune.scope -cne 'debug-validated-commands' -or $desktopTune.commands -ne 9 -or $desktopTune.states -ne 9 -or
        $desktopTune.readbacks -ne 72 -or $desktopTune.csv_rows -ne 18 -or $desktopTune.crops -ne 0 -or $desktopTune.saved_tokens -ne 1 -or
        (($desktopTuneRows[0].PSObject.Properties.Name)-join '|') -cne ($columns-join '|') -or
        @($desktopTuneRows|Where-Object {$_.full_raw_match -cne 'true' -or $_.adoption_match -cne 'true' -or
            $_.frozen_style_persona_rng -cne 'true' -or $_.private_bytes -cne 'true' -or $_.adoption -cne 'live'}).Count -or
        (Get-Item -LiteralPath $desktopTranscript).Length -le 0 -or $desktopSaves -ne 2 -or $desktopSuccess -ne 2 -or
        !(Test-Path -LiteralPath (Join-Path $captures 'desktop-colour-tune-scratch/profile/ColourWars/save_data.json') -PathType Leaf)){
        throw 'Desktop Debug budget/schema/private-save evidence mismatch'
    }
    foreach($path in @($desktopTuneSummaryPath,$desktopTuneRowsPath,$desktopTranscript)){$desktopArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}}
    $desktopObserved.developer_commands=$desktopTune;$desktopObserved.readbacks=112;$desktopObserved.csv_rows=26
}elseif($desktopSaves -ne 0 -or $desktopSuccess -ne 0 -or @(Get-ChildItem -LiteralPath $captures -Filter 'desktop-colour-tune-*').Count){
    throw 'Release desktop proof must stay literal-consumer-only without developer artifacts or Save'
}
$result.observed_counts=$desktopObserved
