# Dot-sourced only after -ProgramLevelVisibilityOnly native execution.
# Six actual Level readbacks; this verifier performs no native drawing.
$plSummaryPath = Join-Path $captures 'program-level-summary.json'
$plCasesPath = Join-Path $captures 'program-level-cases.csv'
$plSamplesPath = Join-Path $captures 'program-level-samples.csv'
$plSummary = Get-Content -LiteralPath $plSummaryPath -Raw | ConvertFrom-Json
$plCases = @(Import-Csv -LiteralPath $plCasesPath)
$plSamples = @(Import-Csv -LiteralPath $plSamplesPath)
$plScope = 'actual-level-single-opening-checkpoint'
$plFields = @('scope','stage','player_count','input_slots','opening','state','viewport','builder_seed','level_builds',
    'setup_level_updates','setup_elapsed_seconds','construction_admission','observation_level_updates','observation_player_updates',
    'observation_scene_updates','observation_state_updates','observation_input_updates','observation_time_updates',
    'readbacks','case_rows','sample_rows','csv_rows','full_ppms','crops','commands','rng_brackets','repeat_pairs','whole_frame_reset',
    'frozen_public_style_resources','private_story_clocks_observed','program_mask','program_patch','paper_patch','glass_patch','event_fade')
if ((($plSummary.PSObject.Properties.Name | Sort-Object) -join '|') -cne (($plFields | Sort-Object) -join '|')) {
    throw 'Program Level summary schema differs'
}
$plNumbers = [ordered]@{player_count=2;builder_seed=9411;level_builds=1;setup_level_updates=1;setup_elapsed_seconds=0.4;
    observation_level_updates=0;observation_player_updates=0;observation_scene_updates=0;observation_state_updates=0;
    observation_input_updates=0;observation_time_updates=0;readbacks=6;case_rows=3;sample_rows=9;csv_rows=12;
    full_ppms=2;crops=0;commands=0;rng_brackets=1;repeat_pairs=3}
foreach ($plKey in $plNumbers.Keys) {
    if ($plSummary.$plKey -is [string] -or $plSummary.$plKey -is [bool] -or $plSummary.$plKey -ne $plNumbers[$plKey]) {
        throw "Program Level budget/checkpoint differs: $plKey"
    }
}
if ($plSummary.scope -cne $plScope -or $plSummary.stage -cne 'crossfeed' -or $plSummary.opening -cne 'full' -or
    $plSummary.state -cne 'opening' -or $plSummary.viewport -cne '1280x720' -or ($plSummary.input_slots -join ',') -cne '4,3' -or
    $plSummary.construction_admission -cne 'source-derived-end-tick-not-instrumented' -or
    ($plSummary.program_mask -join ',') -cne '136,59,1007,480' -or ($plSummary.program_patch -join ',') -cne '632,92,16,16' -or
    ($plSummary.paper_patch -join ',') -cne '392,242,16,16' -or ($plSummary.glass_patch -join ',') -cne '132,100,1,1') {
    throw 'Program Level declared route/sites or source-only admission label differ'
}
foreach ($plKey in @('whole_frame_reset','frozen_public_style_resources')) {
    if ($plSummary.$plKey -isnot [bool] -or $plSummary.$plKey -ne $true) { throw "Program Level invariant failed: $plKey" }
}
foreach ($plKey in @('private_story_clocks_observed','event_fade')) {
    if ($plSummary.$plKey -isnot [bool] -or $plSummary.$plKey -ne $false) { throw "Program Level scope changed: $plKey" }
}
$plCaseColumns = @('state','readbacks','repeat_equal','baseline_equal','delta_pixels','outside_mask_delta_pixels','frozen_public_style','rng_scope','scope')
$plSampleColumns = @('state','site','x','y','width','height','expected_rgb','observed_rgb','all_pixels_equal','baseline_unchanged','scope')
if ($plCases.Count -ne 3 -or $plSamples.Count -ne 9 -or
    ($plCases[0].PSObject.Properties.Name -join '|') -cne ($plCaseColumns -join '|') -or
    ($plSamples[0].PSObject.Properties.Name -join '|') -cne ($plSampleColumns -join '|')) {
    throw 'Program Level CSV count/schema differs'
}
$plStates = @('baseline','rgb','restored')
$plSites = @('exposed-program','covered-paper','outside-program-glass')
$plAreas = @(@(632,92,16,16),@(392,242,16,16),@(132,100,1,1))
$plColours = @(0x2C2C2C,0x35BCE1,0x2C2C2C)
for ($plIndex=0; $plIndex -lt 3; ++$plIndex) {
    $plCase = $plCases[$plIndex]
    $plBaselineEqual = if ($plIndex -eq 1) {'false'} else {'true'}
    if ($plCase.state -cne $plStates[$plIndex] -or $plCase.readbacks -cne '2' -or $plCase.repeat_equal -cne 'true' -or
        $plCase.baseline_equal -cne $plBaselineEqual -or $plCase.outside_mask_delta_pixels -cne '0' -or
        $plCase.frozen_public_style -cne 'true' -or $plCase.rng_scope -cne 'single-six-draw-bracket' -or $plCase.scope -cne $plScope -or
        ($plIndex -eq 1 -and [long]$plCase.delta_pixels -lt 256) -or ($plIndex -ne 1 -and $plCase.delta_pixels -cne '0')) {
        throw 'Program Level case/whole-frame invariant differs'
    }
    for ($plSite=0; $plSite -lt 3; ++$plSite) {
        $plRow = $plSamples[$plIndex*3+$plSite]
        $plExpected = if ($plSite -eq 0) {$plColours[$plIndex]} elseif ($plSite -eq 1) {0xC9D3D9} else {0x161616}
        $plUnchanged = if ($plIndex -eq 1 -and $plSite -eq 0) {'false'} else {'true'}
        if ($plRow.state -cne $plStates[$plIndex] -or $plRow.site -cne $plSites[$plSite] -or
            "$($plRow.x),$($plRow.y),$($plRow.width),$($plRow.height)" -cne ($plAreas[$plSite] -join ',') -or
            [long]$plRow.expected_rgb -ne $plExpected -or [long]$plRow.observed_rgb -ne $plExpected -or
            $plRow.all_pixels_equal -cne 'true' -or $plRow.baseline_unchanged -cne $plUnchanged -or $plRow.scope -cne $plScope) {
            throw 'Program Level sample coordinates/literal RGB/unchanged assertion differs'
        }
    }
}
$plFiles = @(Get-ChildItem -LiteralPath $captures -Filter '*.ppm' -File | Sort-Object Name)
if ($plFiles.Count -ne 2 -or ($plFiles.Name -join '|') -cne 'program-level-baseline.ppm|program-level-rgb.ppm' -or
    @(Get-ChildItem -LiteralPath $captures -Filter 'program-level-failure*' -File).Count) {
    throw 'Program Level exact full-frame list or failure artifacts differ'
}
$plHeader = [Text.Encoding]::ASCII.GetBytes("P6`n1280 720`n255`n")
$plFrames = @(foreach ($plFile in $plFiles) {
    $plBytes = [IO.File]::ReadAllBytes($plFile.FullName)
    if ($plBytes.Length -ne $plHeader.Length+1280*720*3) { throw 'Program Level full PPM size differs' }
    for ($plAt=0; $plAt -lt $plHeader.Length; ++$plAt) {
        if ($plBytes[$plAt] -ne $plHeader[$plAt]) { throw 'Program Level PPM header differs' }
    }
    ,$plBytes
})
function Assert-ProgramLevelPatch([byte[]]$Bytes,[int[]]$Area,[int]$Rgb) {
    for ($plY=$Area[1]; $plY -lt $Area[1]+$Area[3]; ++$plY) {
        for ($plX=$Area[0]; $plX -lt $Area[0]+$Area[2]; ++$plX) {
            $plAt=$plHeader.Length+($plY*1280+$plX)*3
            $plActual=([int]$Bytes[$plAt] -shl 16) -bor ([int]$Bytes[$plAt+1] -shl 8) -bor [int]$Bytes[$plAt+2]
            if ($plActual -ne $Rgb) { throw 'Program Level retained PPM fails independent literal patch' }
        }
    }
}
for ($plIndex=0; $plIndex -lt 2; ++$plIndex) {
    Assert-ProgramLevelPatch $plFrames[$plIndex] $plAreas[0] $plColours[$plIndex]
    Assert-ProgramLevelPatch $plFrames[$plIndex] $plAreas[1] 0xC9D3D9
    Assert-ProgramLevelPatch $plFrames[$plIndex] $plAreas[2] 0x161616
}
# Compare all exterior bytes in row spans, and count actual RGB pixel changes
# inside the declared mask. No synthetic renderer or production geometry clone.
$plDelta=0
for ($plY=0; $plY -lt 720; ++$plY) {
    $plRowStart=$plHeader.Length+$plY*1280*3
    $plSpans = @()
    if ($plY -lt 59 -or $plY -ge 539) { $plSpans += ,@(0,1280) }
    else { $plSpans += ,@(0,136); $plSpans += ,@(1143,137) }
    foreach ($plSpan in $plSpans) {
        $plAt=$plRowStart+$plSpan[0]*3; $plLength=$plSpan[1]*3
        if ([Convert]::ToBase64String($plFrames[0],$plAt,$plLength) -cne [Convert]::ToBase64String($plFrames[1],$plAt,$plLength)) {
            throw 'Program Level retained PPM delta escaped program mask'
        }
    }
    if ($plY -ge 59 -and $plY -lt 539) {
        for ($plX=136; $plX -lt 1143; ++$plX) {
            $plAt=$plRowStart+$plX*3
            if ($plFrames[0][$plAt] -ne $plFrames[1][$plAt] -or $plFrames[0][$plAt+1] -ne $plFrames[1][$plAt+1] -or
                $plFrames[0][$plAt+2] -ne $plFrames[1][$plAt+2]) { ++$plDelta }
        }
    }
}
if ($plDelta -lt 256 -or $plDelta -ne [long]$plCases[1].delta_pixels) { throw 'Program Level native delta count differs from retained full frames' }
$plLog = @(Get-Content -LiteralPath (Join-Path $run 'run.log'))
if (@($plLog | Where-Object {$_ -match 'Saving\.\.\.$' -or $_ -ceq 'Save was successful.'}).Count -or
    @(Get-Content -LiteralPath (Join-Path $run 'run.err') | Where-Object {$_ -match '^save failure:'}).Count) {
    throw 'Program Level literal consumer unexpectedly wrote Save'
}
$plArtifacts=[ordered]@{}
foreach ($plPath in @($plSummaryPath,$plCasesPath,$plSamplesPath)+@($plFiles.FullName)) {
    $plArtifacts[(Split-Path $plPath -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $plPath).Length;sha256=(Get-FileHash -LiteralPath $plPath).Hash}
}
$result.observed_counts=[ordered]@{consumer=$plSummary;readbacks=6;case_rows=3;sample_rows=9;csv_rows=12;full_ppms=2;crops=0;
    setup_level_updates=1;setup_elapsed_seconds=0.4;observation_level_updates=0;observation_player_updates=0;observation_scene_updates=0;
    observation_state_updates=0;observation_input_updates=0;observation_time_updates=0;rng_brackets=1;commands=0;
    private_save_calls=0;successful_private_saves=0;failed_private_saves=0;ppm_rgb_delta_pixels=$plDelta;artifacts=$plArtifacts}
