# Dot-sourced only for -CursorMaterialTuneColoursOnly after the native process.
$cmSummaryPath=Join-Path $captures 'cursor-material-consumer-summary.json';$cmRowsPath=Join-Path $captures 'cursor-material-consumers.csv';$cmSamplesPath=Join-Path $captures 'cursor-material-samples.csv'
$cmSummary=Get-Content -LiteralPath $cmSummaryPath -Raw|ConvertFrom-Json;$cmRows=@(Import-Csv -LiteralPath $cmRowsPath);$cmSamples=@(Import-Csv -LiteralPath $cmSamplesPath)
$cmStates=@('default','rgb','rgba128','alpha0')
$cmSampleColumns=@('state','background','sample','actual_rgb','expected_rgb','matched','shade_straight_rgb','shade_squared_rgb','shadow_straight_rgb','shadow_squared_rgb','pair_straight_rgb','scope')
$cmControlColumns=@('shade_straight_rgb','shade_squared_rgb','shadow_straight_rgb','shadow_squared_rgb','pair_straight_rgb')
function Assert-CursorMaterialScope($Summary){
    if($Summary.state_updates -ne 0 -or $Summary.level_updates -ne 0 -or $Summary.input_updates -ne 0 -or $Summary.local_updates -ne 0 -or
        $Summary.draw_screen_height -ne 720 -or $Summary.viewport -cne '1280x720' -or ($Summary.tip-join ',') -cne '640,240' -or $Summary.click_age -ne 0.09 -or
        $Summary.event_fade -ne $false -or $Summary.full_shape_reference_scope -cne 'production-consumer-adoption-only' -or
        $Summary.alpha_reference_scope -cne 'independent-ordered-raw-patches'){throw 'Cursor material draw/reference/update scope mismatch.'}
}
function Assert-CursorMaterialPairs($Rows,$States){
    $expected=@(foreach($state in $States){foreach($bg in @('black','nonblack')){"$state/$bg"}});$actual=@($Rows|ForEach-Object {"$($_.state)/$($_.background)"})
    if($Rows.Count -ne $States.Count*2 -or (($actual|Sort-Object)-join '|') -cne (($expected|Sort-Object)-join '|')){throw 'Cursor material exact case set mismatch.'}
}
function Assert-CursorMaterialSamples($Rows,$States,$Scope,$Controls){
    $expected=@(foreach($state in $States){foreach($bg in @('black','nonblack')){foreach($sample in @('shade-over-shadow','shadow-only')){"$state/$bg/$sample"}}})
    $actual=@($Rows|ForEach-Object {"$($_.state)/$($_.background)/$($_.sample)"})
    if($Rows.Count -ne $States.Count*4 -or (($Rows[0].PSObject.Properties.Name)-join '|') -cne ($cmSampleColumns-join '|') -or
        (($actual|Sort-Object)-join '|') -cne (($expected|Sort-Object)-join '|')){throw 'Cursor material sample schema/exact sites mismatch.'}
    $negative=0
    foreach($row in $Rows){
        if($row.scope -cne $Scope -or $row.matched -cne 'true' -or $row.actual_rgb -cnotmatch '^\d+$' -or $row.expected_rgb -cnotmatch '^\d+$' -or
            [uint32]$row.actual_rgb -gt 16777215 -or [uint32]$row.actual_rgb -ne [uint32]$row.expected_rgb){throw 'Cursor material independent ordered reference channels failed.'}
        for($index=0;$index -lt 5;$index++){
            $wanted=$Controls -and (($row.state -ceq 'rgba128' -and $index -lt 4 -and ($index -ge 2 -or $row.sample -ceq 'shade-over-shadow')) -or ($row.state -ceq 'alpha0' -and $index -eq 4))
            $value=$row.($cmControlColumns[$index])
            if($wanted){if($value -cnotmatch '^\d+$' -or [uint32]$value -gt 16777215 -or [uint32]$value -eq [uint32]$row.actual_rgb){throw 'Cursor material wrong-alpha sample failed rejection.'};$negative++}
            elseif($value -cne 'not-sampled'){throw 'Cursor material claims an unexecuted negative sample.'}
        }
    }
    if($negative -ne $(if($Controls){16}else{0})){throw 'Cursor material negative comparison count mismatch.'}
}
Assert-CursorMaterialScope $cmSummary;Assert-CursorMaterialPairs $cmRows $cmStates;Assert-CursorMaterialSamples $cmSamples $cmStates 'consumer-only' $true
$cmColumns=@('state','background','rest_match','click_match','reduced_rest','ordered_samples','ink_fill_ticks_rng','control','scope')
if($cmSummary.scope -cne 'consumer-only' -or $cmSummary.readbacks -ne 68 -or $cmSummary.csv_rows -ne 24 -or $cmSummary.case_rows -ne 8 -or $cmSummary.sample_rows -ne 16 -or
    $cmSummary.crops -ne 8 -or $cmSummary.crop_pixels -ne 46080 -or $cmSummary.negative_reference_frames -ne 10 -or $cmSummary.negative_sample_comparisons -ne 16 -or $cmSummary.blank_readbacks -ne 2 -or
    (($cmRows[0].PSObject.Properties.Name)-join '|') -cne ($cmColumns-join '|') -or @($cmRows|Where-Object {$_.rest_match -cne 'true' -or $_.click_match -cne 'true' -or $_.reduced_rest -cne 'true' -or
        $_.ordered_samples -cne 'true' -or $_.ink_fill_ticks_rng -cne 'true' -or $_.scope -cne 'consumer-only'}).Count){throw 'Cursor material shared counters/assertions/schema mismatch.'}
$cmControls=@{default='unchanged-default-adoption';rgb='distinct-opaque-materials';rgba128='per-field-straight-squared-controls';alpha0='selected-zero-retains-ink-fill-ticks'}
foreach($row in $cmRows){if($row.control -cne $cmControls[$row.state]){throw 'Cursor material control label mismatch.'}}
$cmCrops=@(Get-ChildItem -LiteralPath $captures -Filter 'cursor-material-*.ppm' -File|Sort-Object Name)
$cmNames=@(foreach($state in $cmStates){foreach($bg in @('black','nonblack')){"cursor-material-$state-$bg.ppm"}})
if($cmCrops.Count -ne 8 -or (($cmCrops.Name|Sort-Object)-join '|') -cne (($cmNames|Sort-Object)-join '|')){throw 'Cursor material exact crop set mismatch.'}
$cmHeader=[Text.Encoding]::ASCII.GetBytes("P6`n72 80`n255`n");$cmBytes=0
$cmCropEvidence=@(foreach($crop in $cmCrops){$bytes=[IO.File]::ReadAllBytes($crop.FullName)
    if($bytes.Length -ne $cmHeader.Length+72*80*3){throw 'Cursor material compact PPM payload mismatch.'}
    for($i=0;$i -lt $cmHeader.Length;$i++){if($bytes[$i] -ne $cmHeader[$i]){throw 'Cursor material canonical PPM header mismatch.'}}
    $cmBytes+=$bytes.Length;[ordered]@{name=$crop.Name;bytes=$bytes.Length;sha256=(Get-FileHash -LiteralPath $crop.FullName).Hash}
})
$cmLog=@(Get-Content -LiteralPath (Join-Path $run 'run.log'));$cmSaveCalls=@($cmLog|Where-Object {$_ -match 'Saving\.\.\.$'}).Count;$cmSaveSuccesses=@($cmLog|Where-Object {$_ -ceq 'Save was successful.'}).Count
if(@(Get-Content -LiteralPath (Join-Path $run 'run.err')|Where-Object {$_ -match '^save failure:'}).Count){throw 'Unexpected cursor material private Save failure.'}
$cmArtifacts=[ordered]@{}
foreach($path in @($cmSummaryPath,$cmRowsPath,$cmSamplesPath)){$cmArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}}
$cmArtifacts.crops=$cmCropEvidence
$cmObserved=[ordered]@{consumer=$cmSummary;state_updates=0;level_updates=0;input_updates=0;local_updates=0;readbacks=68;csv_rows=24;case_rows=8;sample_rows=16;crops=8;crop_pixels=46080;ppm_bytes=$cmBytes;
    negative_reference_frames=10;negative_sample_comparisons=16;private_save_calls=$cmSaveCalls;successful_private_saves=$cmSaveSuccesses;failed_private_saves=0;artifacts=$cmArtifacts}
if($debug){
    $cmTuneSummaryPath=Join-Path $captures 'cursor-material-tune-summary.json';$cmTuneRowsPath=Join-Path $captures 'cursor-material-tune-colours.csv';$cmTuneSamplesPath=Join-Path $captures 'cursor-material-tune-samples.csv';$cmTranscriptPath=Join-Path $captures 'cursor-material-tune-commands.log'
    $cmTuneSummary=Get-Content -LiteralPath $cmTuneSummaryPath -Raw|ConvertFrom-Json;$cmTuneRows=@(Import-Csv -LiteralPath $cmTuneRowsPath);$cmTuneSamples=@(Import-Csv -LiteralPath $cmTuneSamplesPath)
    $cmTuneStates=@('baseline','shade-named','named-pair','reset-shade','reset-original','rgb-pair','rgba-pair','saved-rgba','edited-after-save','reset-saved-rgba','alpha0-pair','reset-alpha0-to-saved')
    Assert-CursorMaterialScope $cmTuneSummary;Assert-CursorMaterialPairs $cmTuneRows $cmTuneStates;Assert-CursorMaterialSamples $cmTuneSamples $cmTuneStates 'debug-validated-commands' $false
    $cmTuneColumns=@('state','background','rest_match','click_match','ordered_samples','ink_fill_ticks_rng','private_bytes','adoption')
    if($cmTuneSummary.scope -cne 'debug-validated-commands' -or $cmTuneSummary.commands -ne 15 -or $cmTuneSummary.readbacks -ne 144 -or $cmTuneSummary.csv_rows -ne 72 -or
        $cmTuneSummary.case_rows -ne 24 -or $cmTuneSummary.sample_rows -ne 48 -or $cmTuneSummary.crops -ne 0 -or $cmTuneSummary.saved_tokens -ne 2 -or
        (($cmTuneRows[0].PSObject.Properties.Name)-join '|') -cne ($cmTuneColumns-join '|') -or @($cmTuneRows|Where-Object {$_.rest_match -cne 'true' -or $_.click_match -cne 'true' -or
            $_.ordered_samples -cne 'true' -or $_.ink_fill_ticks_rng -cne 'true' -or $_.private_bytes -cne 'true' -or $_.adoption -cne 'live'}).Count -or
        $cmSaveCalls -ne 2 -or $cmSaveSuccesses -ne 2 -or (Get-Item -LiteralPath $cmTranscriptPath).Length -le 0 -or
        !(Test-Path -LiteralPath (Join-Path $captures 'cursor-material-tune-scratch/profile/ColourWars/save_data.json') -PathType Leaf)){throw 'Cursor material Debug/private Save counters/schema mismatch.'}
    foreach($path in @($cmTuneSummaryPath,$cmTuneRowsPath,$cmTuneSamplesPath,$cmTranscriptPath)){$cmArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}}
    $cmObserved.developer_commands=$cmTuneSummary;$cmObserved.readbacks=212;$cmObserved.csv_rows=96;$cmObserved.case_rows=32;$cmObserved.sample_rows=64
}elseif($cmSaveCalls -ne 0 -or $cmSaveSuccesses -ne 0 -or @(Get-ChildItem -LiteralPath $captures -Filter 'cursor-material-tune-*').Count){throw 'Release cursor material proof must remain consumer-only without developer artifacts/Saves.'}
$result.observed_counts=$cmObserved
