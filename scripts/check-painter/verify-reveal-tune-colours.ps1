# Dot-sourced only for -RevealTuneColoursOnly after the native process.
$revealSummaryPath=Join-Path $captures 'reveal-colour-consumer-summary.json'
$revealRowsPath=Join-Path $captures 'reveal-colour-consumers.csv'
$revealSamplesPath=Join-Path $captures 'reveal-colour-samples.csv'
$revealSummary=Get-Content -LiteralPath $revealSummaryPath -Raw|ConvertFrom-Json
$revealRows=@(Import-Csv -LiteralPath $revealRowsPath);$revealSamples=@(Import-Csv -LiteralPath $revealSamplesPath)
$revealStates=@('default','ink-rgb','paper-rgb','shade-rgb','ink-rgba','paper-rgba','shade-rgba','ink-alpha0','paper-alpha0','shade-alpha0','all-rgba')
$revealSites=@('ink-edge','ink-overlap','paper-edge','paper-overlap','shadow-lip','airbrush-one','airbrush-three','tick-ink-a','tick-core-a','tick-ink-b','tick-core-b')
$revealSampleColumns=@('state','background','sample','pose','actual_rgb','expected_rgb','matched','straight_rgb','squared_rgb','full_airbrush_rgb','single_airbrush_rgb','no_beat_rgb','linear_beat_rgb','double_beat_rgb','scope')
$revealControlColumns=@('straight_rgb','squared_rgb','full_airbrush_rgb','single_airbrush_rgb','no_beat_rgb','linear_beat_rgb','double_beat_rgb')
function Assert-RevealScope($Summary){
    if($Summary.state_updates -ne 0 -or $Summary.level_updates -ne 0 -or $Summary.input_updates -ne 0 -or $Summary.local_updates -ne 0 -or
        $Summary.viewport -cne '1280x720' -or ($Summary.bounds-join ',') -cne '80,80,360,100' -or $Summary.half_age -ne 0.09 -or
        $Summary.quadratic_half_beat -ne 0.25 -or $Summary.airbrush_scale -ne 0.12 -or $Summary.space_measurement -cne 'actual-font-zero' -or
        $Summary.material_samples -ne 7 -or $Summary.tick_samples -ne 4 -or
        $Summary.full_shape_reference_scope -cne 'production-consumer-adoption-only' -or
        $Summary.alpha_reference_scope -cne 'independent-ordered-raw-patches'){throw 'Reveal draw/reference/SPACE/update scope mismatch.'}
}
function Assert-RevealPairs($Rows,$States){
    $expected=@(foreach($state in $States){foreach($background in @('black','nonblack')){"$state/$background"}})
    $actual=@($Rows|ForEach-Object {"$($_.state)/$($_.background)"})
    if($Rows.Count -ne $States.Count*2 -or (($actual|Sort-Object)-join '|') -cne (($expected|Sort-Object)-join '|')){throw 'Reveal state/underlay rows differ from exact approved cases.'}
}
function Assert-RevealSamples($Rows,$States,$Scope,$Controls){
    $expected=@(foreach($state in $States){foreach($background in @('black','nonblack')){foreach($site in $revealSites){"$state/$background/$site"}}})
    $actual=@($Rows|ForEach-Object {"$($_.state)/$($_.background)/$($_.sample)"})
    if($Rows.Count -ne $States.Count*22 -or (($Rows[0].PSObject.Properties.Name)-join '|') -cne ($revealSampleColumns-join '|') -or
        (($actual|Sort-Object)-join '|') -cne (($expected|Sort-Object)-join '|')){throw 'Reveal eleven-site row/schema scope mismatch.'}
    $rejections=0
    foreach($row in $Rows){
        $tick=$row.sample -like 'tick-*';$pose=if($tick){'half'}else{'rest'}
        if($row.pose -cne $pose -or $row.scope -cne $Scope -or $row.matched -cne 'true' -or
            $row.actual_rgb -cnotmatch '^\d+$' -or $row.expected_rgb -cnotmatch '^\d+$' -or
            [uint32]$row.actual_rgb -gt 16777215 -or [uint32]$row.actual_rgb -ne [uint32]$row.expected_rgb){throw 'Reveal raw channels/pose/reference equality failed.'}
        $ink=$tick -or $row.sample -like 'ink-*';$paper=$row.sample -like 'paper-*';$shade=$row.sample -in @('shadow-lip','airbrush-one','airbrush-three')
        $base=$Controls -and (($row.state -in @('ink-rgba','ink-alpha0') -and $ink) -or
            ($row.state -in @('paper-rgba','paper-alpha0') -and $paper) -or ($row.state -in @('shade-rgba','shade-alpha0') -and $shade))
        $squared=$base -and $row.state -like '*-rgba'
        $air=$Controls -and $row.state -ceq 'shade-rgba';$beat=$Controls -and $row.state -ceq 'ink-rgba' -and $tick
        $wanted=@($base,$squared,($air -and $row.sample -like 'airbrush-*'),($air -and $row.sample -ceq 'airbrush-three'),$beat,$beat,$beat)
        for($index=0;$index -lt $revealControlColumns.Count;$index++){
            $value=$row.($revealControlColumns[$index])
            if($wanted[$index]){
                if($value -cnotmatch '^\d+$' -or [uint32]$value -gt 16777215 -or [uint32]$value -eq [uint32]$row.actual_rgb){throw 'Reveal wrong-alpha/overlap/beat control was not rejected.'};$rejections++
            }elseif($value -cne 'not-sampled'){throw 'Reveal claims an unexecuted negative sample.'}
        }
    }
    if($rejections -ne $(if($Controls){96}else{0})){throw 'Reveal negative sample comparison count mismatch.'}
}
Assert-RevealScope $revealSummary;Assert-RevealPairs $revealRows $revealStates;Assert-RevealSamples $revealSamples $revealStates 'consumer-only' $true
$revealCaseColumns=@('state','background','rest_match','half_match','reduced_rest','expired_rest','raw_samples','dependencies_rng','control','scope')
if($revealSummary.scope -cne 'consumer-only' -or $revealSummary.readbacks -ne 228 -or $revealSummary.csv_rows -ne 264 -or
    $revealSummary.case_rows -ne 22 -or $revealSummary.sample_rows -ne 242 -or $revealSummary.crops -ne 10 -or $revealSummary.crop_pixels -ne 672000 -or
    $revealSummary.negative_reference_frames -ne 28 -or $revealSummary.blank_readbacks -ne 2 -or
    (($revealRows[0].PSObject.Properties.Name)-join '|') -cne ($revealCaseColumns-join '|') -or
    @($revealRows|Where-Object {$_.rest_match -cne 'true' -or $_.half_match -cne 'true' -or $_.reduced_rest -cne 'true' -or
        $_.expired_rest -cne 'true' -or $_.raw_samples -cne 'true' -or $_.dependencies_rng -cne 'true' -or $_.scope -cne 'consumer-only'}).Count){throw 'Reveal shared counters/assertions/schema mismatch.'}
$revealControls=@{default='default-adoption-parity';'ink-rgb'='independent-ink-rgb';'paper-rgb'='independent-paper-rgb';'shade-rgb'='independent-shade-rgb';
    'ink-rgba'='ink-base-and-quadratic-controls';'paper-rgba'='paper-base-and-stepped-overlap';'shade-rgba'='shade-base-airbrush-scale-and-overlap';
    'ink-alpha0'='ink-zero-restores-underlayers-and-straight-rejected';'paper-alpha0'='paper-zero-restores-ink-and-straight-rejected';
    'shade-alpha0'='shade-zero-removes-shadow-airbrush-and-straight-rejected';'all-rgba'='combined-ordered-materials-and-caller-teams'}
foreach($row in $revealRows){if($row.control -cne $revealControls[$row.state]){throw 'Reveal control label mismatch.'}}
$revealCrops=@(Get-ChildItem -LiteralPath $captures -Filter 'reveal-colour-*.ppm' -File|Sort-Object Name)
$revealNames=@(foreach($state in @('ink-alpha0','paper-alpha0','shade-alpha0')){foreach($background in @('black','nonblack')){"reveal-colour-$state-$background-materials.ppm"}};foreach($state in @('default','all-rgba')){foreach($background in @('black','nonblack')){"reveal-colour-$state-$background-labels.ppm"}})
if($revealCrops.Count -ne 10 -or (($revealCrops.Name|Sort-Object)-join '|') -cne (($revealNames|Sort-Object)-join '|')){throw 'Reveal compact crop names/count mismatch.'}
$revealBytes=0;$revealPixels=0
$revealCropEvidence=@(foreach($crop in $revealCrops){
    $width=if($crop.Name -like '*-materials.ppm'){400}else{800};$height=120
    $header=[Text.Encoding]::ASCII.GetBytes("P6`n$width $height`n255`n");$bytes=[IO.File]::ReadAllBytes($crop.FullName)
    if($bytes.Length -ne $header.Length+$width*$height*3){throw 'Reveal compact crop payload size mismatch.'}
    for($index=0;$index -lt $header.Length;$index++){if($bytes[$index] -ne $header[$index]){throw 'Reveal canonical PPM header mismatch.'}}
    $revealBytes+=$bytes.Length;$revealPixels+=$width*$height
    [ordered]@{name=$crop.Name;width=$width;height=$height;bytes=$bytes.Length;sha256=(Get-FileHash -LiteralPath $crop.FullName).Hash}
})
if($revealPixels -ne 672000){throw 'Reveal retained pixel budget mismatch.'}
$revealLog=@(Get-Content -LiteralPath (Join-Path $run 'run.log'))
$revealSaveCalls=@($revealLog|Where-Object {$_ -match 'Saving\.\.\.$'}).Count;$revealSaveSuccesses=@($revealLog|Where-Object {$_ -ceq 'Save was successful.'}).Count
if(@(Get-Content -LiteralPath (Join-Path $run 'run.err')|Where-Object {$_ -match '^save failure:'}).Count){throw 'Unexpected reveal private Save failure.'}
$revealArtifacts=[ordered]@{}
foreach($path in @($revealSummaryPath,$revealRowsPath,$revealSamplesPath)){$revealArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}}
$revealArtifacts.crops=$revealCropEvidence
$revealObserved=[ordered]@{consumer=$revealSummary;state_updates=0;level_updates=0;input_updates=0;local_updates=0;readbacks=228;csv_rows=264;case_rows=22;sample_rows=242;
    crops=10;crop_pixels=672000;ppm_bytes=$revealBytes;negative_reference_frames=28;negative_sample_comparisons=96;
    private_save_calls=$revealSaveCalls;successful_private_saves=$revealSaveSuccesses;failed_private_saves=0;artifacts=$revealArtifacts}
if($debug){
    $revealTuneSummaryPath=Join-Path $captures 'reveal-tune-summary.json';$revealTuneRowsPath=Join-Path $captures 'reveal-tune-colours.csv'
    $revealTuneSamplesPath=Join-Path $captures 'reveal-tune-samples.csv';$revealTranscriptPath=Join-Path $captures 'reveal-tune-commands.log'
    $revealTuneSummary=Get-Content -LiteralPath $revealTuneSummaryPath -Raw|ConvertFrom-Json
    $revealTuneRows=@(Import-Csv -LiteralPath $revealTuneRowsPath);$revealTuneSamples=@(Import-Csv -LiteralPath $revealTuneSamplesPath)
    $revealTuneStates=@('baseline','ink-named','paper-named','named-triple','reset-ink','reset-paper','reset-original','rgb-triple','rgba-triple','saved-rgba','edited-after-save','reset-saved-rgba','alpha0-triple','reset-alpha0-to-saved')
    Assert-RevealScope $revealTuneSummary;Assert-RevealPairs $revealTuneRows $revealTuneStates;Assert-RevealSamples $revealTuneSamples $revealTuneStates 'debug-validated-commands' $false
    $revealTuneColumns=@('state','background','rest_match','half_match','raw_samples','dependencies_rng','private_bytes','adoption')
    if($revealTuneSummary.scope -cne 'debug-validated-commands' -or $revealTuneSummary.commands -ne 21 -or $revealTuneSummary.states -ne 14 -or
        $revealTuneSummary.readbacks -ne 196 -or $revealTuneSummary.csv_rows -ne 336 -or $revealTuneSummary.case_rows -ne 28 -or $revealTuneSummary.sample_rows -ne 308 -or
        $revealTuneSummary.crops -ne 0 -or $revealTuneSummary.saved_tokens -ne 3 -or (($revealTuneRows[0].PSObject.Properties.Name)-join '|') -cne ($revealTuneColumns-join '|') -or
        @($revealTuneRows|Where-Object {$_.rest_match -cne 'true' -or $_.half_match -cne 'true' -or $_.raw_samples -cne 'true' -or $_.dependencies_rng -cne 'true' -or $_.private_bytes -cne 'true' -or $_.adoption -cne 'live'}).Count -or
        $revealSaveCalls -ne 2 -or $revealSaveSuccesses -ne 2 -or (Get-Item -LiteralPath $revealTranscriptPath).Length -le 0 -or
        !(Test-Path -LiteralPath (Join-Path $captures 'reveal-tune-scratch/profile/ColourWars/save_data.json') -PathType Leaf)){throw 'Reveal Debug counters/three-token private Save/case evidence mismatch.'}
    foreach($path in @($revealTuneSummaryPath,$revealTuneRowsPath,$revealTuneSamplesPath,$revealTranscriptPath)){$revealArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}}
    $revealObserved.developer_commands=$revealTuneSummary;$revealObserved.readbacks=424;$revealObserved.csv_rows=600;$revealObserved.case_rows=50;$revealObserved.sample_rows=550
}elseif($revealSaveCalls -ne 0 -or $revealSaveSuccesses -ne 0 -or @(Get-ChildItem -LiteralPath $captures -Filter 'reveal-tune-*').Count){throw 'Release reveal proof must remain consumer-only without developer artifacts or Saves.'}
$result.observed_counts=$revealObserved
