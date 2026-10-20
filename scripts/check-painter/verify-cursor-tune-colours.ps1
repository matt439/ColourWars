# Dot-sourced only for -CursorTuneColoursOnly after the native process.
$cursorSummaryPath=Join-Path $captures 'cursor-colour-consumer-summary.json'
$cursorRowsPath=Join-Path $captures 'cursor-colour-consumers.csv'
$cursorSamplesPath=Join-Path $captures 'cursor-colour-samples.csv'
$cursorSummary=Get-Content -LiteralPath $cursorSummaryPath -Raw|ConvertFrom-Json
$cursorRows=@(Import-Csv -LiteralPath $cursorRowsPath)
$cursorSamples=@(Import-Csv -LiteralPath $cursorSamplesPath)
$cursorStates=@('default','ink-rgb','fill-rgb','ink-rgba','fill-rgba','pair-rgba','pair-alpha0')
$cursorSampleNames=@('tip','contour','interior','shade','shadow','click-ink','click-fill')
$cursorCaseColumns=@('state','background','rest_match','click_match','reduced_rest','raw_samples','shade_shadow_fixed','control','scope')
$cursorSampleColumns=@('state','background','sample','pose','actual_rgb','expected_rgb','matched','straight_rgb','squared_rgb','scope')
function Assert-CursorPairs($Rows,$States,$ExpectedCount) {
    $expected=@(foreach($state in $States){foreach($background in @('black','nonblack')){"$state/$background"}})
    $actual=@($Rows|ForEach-Object {"$($_.state)/$($_.background)"})
    if($Rows.Count -ne $ExpectedCount -or (($actual|Sort-Object)-join '|') -cne (($expected|Sort-Object)-join '|')){
        throw 'Cursor case rows do not match the exact state/underlay pairs.'
    }
}
function Assert-CursorSamples($Rows,$States,$Scope,$Controls) {
    $expected=@(foreach($state in $States){foreach($background in @('black','nonblack')){foreach($sample in $cursorSampleNames){"$state/$background/$sample"}}})
    $actual=@($Rows|ForEach-Object {"$($_.state)/$($_.background)/$($_.sample)"})
    if($Rows.Count -ne $States.Count*2*7 -or (($Rows[0].PSObject.Properties.Name)-join '|') -cne ($cursorSampleColumns-join '|') -or
        (($actual|Sort-Object)-join '|') -cne (($expected|Sort-Object)-join '|')){throw 'Cursor sample rows/schema differ from the exact seven-site scope.'}
    foreach($row in $Rows){
        $pose=if($row.sample -like 'click-*'){'click'}else{'rest'}
        if($row.pose -cne $pose -or $row.scope -cne $Scope -or $row.matched -cne 'true' -or
            $row.actual_rgb -cnotmatch '^\d+$' -or $row.expected_rgb -cnotmatch '^\d+$' -or
            [uint32]$row.actual_rgb -gt 16777215 -or [uint32]$row.actual_rgb -ne [uint32]$row.expected_rgb){
            throw 'Cursor raw sample channels, pose or independent-reference equality failed.'
        }
        $ink=$row.sample -in @('tip','contour','click-ink');$fill=$row.sample -in @('interior','click-fill')
        $straight=$Controls -and (($row.state -ceq 'ink-rgba' -and $ink) -or ($row.state -ceq 'fill-rgba' -and $fill) -or
            ($row.state -ceq 'pair-alpha0' -and ($ink -or $fill)))
        $squared=$Controls -and (($row.state -ceq 'ink-rgba' -and $ink) -or ($row.state -ceq 'fill-rgba' -and $fill))
        foreach($control in @(@('straight_rgb',$straight),@('squared_rgb',$squared))){
            $value=$row.($control[0])
            if($control[1]){
                if($value -cnotmatch '^\d+$' -or [uint32]$value -gt 16777215 -or [uint32]$value -eq [uint32]$row.actual_rgb){
                    throw 'Cursor targeted wrong-alpha sample was not independently rejected.'
                }
            }elseif($value -cne 'not-sampled'){throw 'Cursor claims an unexecuted negative-control sample.'}
        }
    }
}
function Assert-CursorDrawScope($Summary){
    if($Summary.state_updates -ne 0 -or $Summary.level_updates -ne 0 -or $Summary.effect_preparation_updates -ne 0 -or
        $Summary.draw_screen_height -ne 720 -or $Summary.viewport -cne '1280x720' -or ($Summary.tip-join ',') -cne '640,240' -or
        $Summary.click_age -ne 0.09 -or $Summary.event_fade -ne $false -or
        $Summary.full_shape_reference_scope -cne 'production-consumer-adoption-only' -or
        $Summary.alpha_reference_scope -cne 'independent-raw-sample-patches'){
        throw 'Cursor draw inputs or reference/update scope differ from the approved synthetic fixture.'
    }
}
Assert-CursorDrawScope $cursorSummary
Assert-CursorPairs $cursorRows $cursorStates 14
Assert-CursorSamples $cursorSamples $cursorStates 'consumer-only' $true
if($cursorSummary.scope -cne 'consumer-only' -or $cursorSummary.readbacks -ne 124 -or $cursorSummary.csv_rows -ne 112 -or
    $cursorSummary.case_rows -ne 14 -or $cursorSummary.sample_rows -ne 98 -or $cursorSummary.crops -ne 12 -or
    $cursorSummary.negative_reference_frames -ne 10 -or $cursorSummary.blank_readbacks -ne 2 -or
    (($cursorRows[0].PSObject.Properties.Name)-join '|') -cne ($cursorCaseColumns-join '|') -or
    @($cursorRows|Where-Object {$_.rest_match -cne 'true' -or $_.click_match -cne 'true' -or $_.reduced_rest -cne 'true' -or
        $_.raw_samples -cne 'true' -or $_.shade_shadow_fixed -cne 'true' -or $_.scope -cne 'consumer-only'}).Count){
    throw 'Cursor shared counters/schema/assertions exceed the approved frozen scope.'
}
$cursorControls=@{default='opaque-baseline-parity';'ink-rgb'='changed-ink-rgb';'fill-rgb'='changed-fill-rgb';
    'ink-rgba'='ink-straight-and-squared-base-alpha-rejected';'fill-rgba'='fill-straight-and-squared-base-alpha-rejected';
    'pair-rgba'='paired-raw-alpha-composite';'pair-alpha0'='selected-alpha0-shade-shadow-retained-and-straight-rejected'}
foreach($row in $cursorRows){if($row.control -cne $cursorControls[$row.state]){throw 'Cursor control label mismatch.'}}
$cursorCrops=@(Get-ChildItem -LiteralPath $captures -Filter 'cursor-colour-*.ppm' -File|Sort-Object Name)
$cursorNames=@(foreach($state in @('default','pair-rgba','pair-alpha0')){foreach($background in @('black','nonblack')){foreach($pose in @('rest','click')){"cursor-colour-$state-$background-$pose.ppm"}}})
if($cursorCrops.Count -ne 12 -or (($cursorCrops.Name|Sort-Object)-join '|') -cne (($cursorNames|Sort-Object)-join '|')){
    throw 'Cursor crops do not match the exact twelve state/underlay/pose cases.'
}
$cursorHeader=[Text.Encoding]::ASCII.GetBytes("P6`n72 80`n255`n")
$cursorBytes=0
$cursorCropEvidence=@(foreach($crop in $cursorCrops){
    $bytes=[IO.File]::ReadAllBytes($crop.FullName)
    if($bytes.Length -ne $cursorHeader.Length+72*80*3){throw "Incorrect compact cursor crop size:$($crop.Name)"}
    for($i=0;$i -lt $cursorHeader.Length;$i++){if($bytes[$i] -ne $cursorHeader[$i]){throw 'Incorrect canonical cursor PPM header.'}}
    $cursorBytes+=$bytes.Length
    [ordered]@{name=$crop.Name;bytes=$bytes.Length;sha256=(Get-FileHash -LiteralPath $crop.FullName).Hash}
})
$cursorLog=@(Get-Content -LiteralPath (Join-Path $run 'run.log'))
$cursorSaveCalls=@($cursorLog|Where-Object {$_ -match 'Saving\.\.\.$'}).Count
$cursorSaveSuccesses=@($cursorLog|Where-Object {$_ -ceq 'Save was successful.'}).Count
if(@(Get-Content -LiteralPath (Join-Path $run 'run.err')|Where-Object {$_ -match '^save failure:'}).Count){throw 'Unexpected failed private Save in the cursor proof.'}
$cursorArtifacts=[ordered]@{}
foreach($path in @($cursorSummaryPath,$cursorRowsPath,$cursorSamplesPath)){
    $cursorArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}
}
$cursorArtifacts.crops=$cursorCropEvidence
$cursorObserved=[ordered]@{consumer=$cursorSummary;state_updates=0;level_updates=0;effect_preparation_updates=0;
    readbacks=124;csv_rows=112;case_rows=14;sample_rows=98;crops=12;crop_pixels=69120;ppm_bytes=$cursorBytes;
    negative_reference_frames=10;private_save_calls=$cursorSaveCalls;successful_private_saves=$cursorSaveSuccesses;failed_private_saves=0;artifacts=$cursorArtifacts}
if($debug){
    $cursorTuneSummaryPath=Join-Path $captures 'cursor-tune-summary.json'
    $cursorTuneRowsPath=Join-Path $captures 'cursor-tune-colours.csv'
    $cursorTuneSamplesPath=Join-Path $captures 'cursor-tune-samples.csv'
    $cursorTranscriptPath=Join-Path $captures 'cursor-tune-commands.log'
    $cursorTuneSummary=Get-Content -LiteralPath $cursorTuneSummaryPath -Raw|ConvertFrom-Json
    $cursorTuneRows=@(Import-Csv -LiteralPath $cursorTuneRowsPath)
    $cursorTuneSamples=@(Import-Csv -LiteralPath $cursorTuneSamplesPath)
    $cursorTuneStates=@('baseline','ink-named','named-pair','reset-ink','reset-original','rgb-pair','rgba-pair','saved-rgba',
        'edited-after-save','reset-saved-rgba','alpha0-pair','reset-alpha0-to-saved')
    $cursorTuneColumns=@('state','background','rest_match','click_match','raw_samples','shade_shadow_fixed','private_bytes','adoption')
    Assert-CursorDrawScope $cursorTuneSummary
    Assert-CursorPairs $cursorTuneRows $cursorTuneStates 24
    Assert-CursorSamples $cursorTuneSamples $cursorTuneStates 'debug-validated-commands' $false
    if($cursorTuneSummary.scope -cne 'debug-validated-commands' -or $cursorTuneSummary.commands -ne 15 -or
        $cursorTuneSummary.readbacks -ne 168 -or $cursorTuneSummary.csv_rows -ne 192 -or $cursorTuneSummary.case_rows -ne 24 -or
        $cursorTuneSummary.sample_rows -ne 168 -or $cursorTuneSummary.crops -ne 0 -or $cursorTuneSummary.saved_tokens -ne 2 -or
        (($cursorTuneRows[0].PSObject.Properties.Name)-join '|') -cne ($cursorTuneColumns-join '|') -or
        @($cursorTuneRows|Where-Object {$_.rest_match -cne 'true' -or $_.click_match -cne 'true' -or $_.raw_samples -cne 'true' -or
            $_.shade_shadow_fixed -cne 'true' -or $_.private_bytes -cne 'true' -or $_.adoption -cne 'live'}).Count -or
        (Get-Item -LiteralPath $cursorTranscriptPath).Length -le 0 -or $cursorSaveCalls -ne 2 -or $cursorSaveSuccesses -ne 2 -or
        !(Test-Path -LiteralPath (Join-Path $captures 'cursor-tune-scratch/profile/ColourWars/save_data.json') -PathType Leaf)){
        throw 'Cursor Debug counters/private-save/case evidence exceeds the approved scope.'
    }
    foreach($path in @($cursorTuneSummaryPath,$cursorTuneRowsPath,$cursorTuneSamplesPath,$cursorTranscriptPath)){
        $cursorArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}
    }
    $cursorObserved.developer_commands=$cursorTuneSummary
    $cursorObserved.readbacks=292;$cursorObserved.csv_rows=304;$cursorObserved.case_rows=38;$cursorObserved.sample_rows=266
}
elseif($cursorSaveCalls -ne 0 -or $cursorSaveSuccesses -ne 0 -or @(Get-ChildItem -LiteralPath $captures -Filter 'cursor-tune-*').Count){
    throw 'Release cursor proof must stay literal-consumer-only without developer artifacts or private Saves.'
}
$result.observed_counts=$cursorObserved
