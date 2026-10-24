# Opaque role48 uses the timer family's PNG/schema helpers, with distinct
# applicability and accepted/refused command records. It executes no artifact.
function Read-HudClosingTextEvidence {
    $prefix='hud-closing-text'; $states=@('default','red','rgb','rgba_ff')
    $summaryPath=Join-Path $captures "$prefix-colour-consumer-summary.json"
    $mainPath=Join-Path $captures "$prefix-colour-main.csv"; $branchPath=Join-Path $captures "$prefix-colour-branches.csv"
    $inputs=[Collections.Generic.List[string]]::new()
    foreach ($path in @($summaryPath,$mainPath,$branchPath,(Join-Path $run 'run.log'),(Join-Path $run 'run.err'))) {
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Closing-text required evidence missing' }; $inputs.Add($path)
    }
    if (@(Get-ChildItem -LiteralPath $captures -Filter "$prefix-failure-*").Count) { throw 'Closing-text native failure artifacts remain' }
    $summary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
    $main=@(Import-Csv -LiteralPath $mainPath); $branches=@(Import-Csv -LiteralPath $branchPath)
    $counts=[ordered]@{readbacks=50;csv_rows=18;main_rows=4;branch_rows=14;crops=4;crop_width=122;crop_height=51;crop_pixels=24888;ppm_bytes=74720;roi_pixels=6222;independent_roi_observations=8;roi_pixel_checks=49776;repeat_checks=18;copied_adoption_checks=4;normal_colour_invariance_checks=5;reduce_effects_checks=6;controls=2;calibration_pixel_checks=1600;private_game_saves=0}
    Assert-TimerKeys $summary (@($counts.Keys)+$timerUpdates+@('scope','main_context','control_discriminating_pixels')) 'closing shared'
    foreach ($key in $counts.Keys) { Assert-TimerInteger $summary $key $counts[$key] }
    foreach ($key in $timerUpdates) { Assert-TimerInteger $summary $key 0 }
    $controlPixels=$summary.control_discriminating_pixels
    if (($controlPixels -isnot [int] -and $controlPixels -isnot [long]) -or $controlPixels -le 0 -or $controlPixels -gt 12444 -or $summary.scope -cne 'opaque-closing-text-glyph-roi-and-normal-invariance' -or $summary.main_context -cne 'running-exact-ten') { throw 'Closing-text opaque scope, actual main context or control support differs' }
    Assert-TimerCsvColumns $main @('state','raw_rgba_match','adoption_match','repeat_match','locality_match','opaque_selected','controls','discriminating_pixels','frozen_style_resources_rng','scope') 4 'closing main'
    if (($main.state-join'|') -cne ($states-join'|')) { throw 'Closing-text opaque palette order differs' }
    $sumPixels=0L
    foreach ($row in $main) {
        Assert-TimerFlags $row @('raw_rgba_match','adoption_match','repeat_match','locality_match','opaque_selected','frozen_style_resources_rng')
        $controls=if ($row.state -ceq 'default') {2} else {0}; $pixels=Get-TimerCsvInteger $row 'discriminating_pixels'
        if ((Get-TimerCsvInteger $row 'controls') -ne $controls -or $pixels -gt $controls*6222 -or ($controls -gt 0 -and $pixels -le 0) -or $row.scope -cne 'opaque-closing-text-glyph-roi') { throw 'Closing-text opaque tint/added-shadow accounting differs' }
        $sumPixels += $pixels
    }
    if ($sumPixels -ne $controlPixels) { throw 'Closing-text control pixels do not sum to summary' }
    Assert-TimerCsvColumns $branches @('state','context','closing_branch','raw_rgba_match','repeat_match','selection_guard','opaque_selected','frozen_style_resources_rng','scope') 14 'closing branch'
    $contexts=@('normal','above-ten','fractional-closing','zero','stopped-short','reduced-normal','reduced-closing')
    $applicable=@('fractional-closing','reduced-closing')
    for ($i=0;$i-lt14;++$i) {
        $row=$branches[$i]; $state=if ($i-lt7) {'default'} else {'rgba_ff'}; $context=$contexts[$i%7]
        $closing=$context-cin$applicable; $closingText=if ($closing) {'true'} else {'false'}
        $raw=if ($closing) {'true'} else {'not-run-normal-invariance-only'}
        $guard=if ($state-ceq'default') {'baseline'} elseif ($closing) {'closing-locality'} else {'normal-exact'}
        Assert-TimerFlags $row @('repeat_match','opaque_selected','frozen_style_resources_rng')
        if ($row.state-cne$state -or $row.context-cne$context -or $row.closing_branch-cne$closingText -or $row.raw_rgba_match-cne$raw -or $row.selection_guard-cne$guard -or $row.scope-cne'public-hud-closing-text-counterexample') { throw 'Closing-text applicability/boundary observation differs' }
    }
    $expectedNames=@($states|ForEach-Object{"$prefix-colour-$_.ppm"})
    $ppms=@(Get-ChildItem -LiteralPath $captures -Filter "$prefix-colour-*.ppm" -File)
    if ($ppms.Count-ne4 -or (($ppms.Name|Sort-Object)-join'|')-cne(($expectedNames|Sort-Object)-join'|')) { throw 'Closing-text exact PPM inventory differs' }
    $header=[Text.Encoding]::ASCII.GetBytes("P6`n122 51`n255`n")
    $ppmEvidence=@(foreach ($name in $expectedNames) {
        $path=Join-Path $captures $name; $bytes=[IO.File]::ReadAllBytes($path)
        if ($bytes.Length-ne18680) { throw 'Closing-text canonical native P6 length differs' }
        for ($i=0;$i-lt14;++$i) { if ($bytes[$i]-ne$header[$i]) { throw 'Closing-text canonical P6 header differs' } }
        $rgb=[byte[]]::new(18666); [Array]::Copy($bytes,14,$rgb,0,18666); $inputs.Add($path)
        [ordered]@{name=$name;bytes=18680;sha256=(Get-TimerSha $bytes);rgb_payload_sha256=(Get-TimerSha $rgb)}
    })
    $observed=[ordered]@{consumer=$summary;opacity_policy='opaque-only; partial and zero are refusals';readbacks=50;csv_rows=18;crops=4;crop_pixels=24888;ppm_bytes=74720;controls=2;control_discriminating_pixels=$controlPixels;calibration_pixel_checks=1600;private_game_saves=0;reference_scope='closing paper plus opaque glyph only; normal shadow omitted; copied HUD adoption only';png_count=0;png_crops=@();png_complete=$false}
    if ($debug) {
        $tunePath=Join-Path $captures "$prefix-tune-summary.json"; $tuneCsv=Join-Path $captures "$prefix-tune.csv"
        $refusalsPath=Join-Path $captures "$prefix-tune-refusals.csv"; $normalPath=Join-Path $captures "$prefix-tune-normal.csv"
        $transcript=Join-Path $captures "$prefix-tune-commands.log"
        foreach ($path in @($tunePath,$tuneCsv,$refusalsPath,$normalPath,$transcript)) {
            if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Closing-text required Debug evidence missing' }; $inputs.Add($path)
        }
        $tune=Get-Content -LiteralPath $tunePath -Raw|ConvertFrom-Json
        $accepted=@(Import-Csv -LiteralPath $tuneCsv); $refusals=@(Import-Csv -LiteralPath $refusalsPath); $normal=@(Import-Csv -LiteralPath $normalPath)
        $tuneCounts=[ordered]@{commands=9;accepted_commands=7;refused_commands=2;states=7;readbacks=32;csv_rows=11;accepted_rows=7;refusal_rows=2;normal_rows=2;crops=0;saved_tokens=1;presentation_save_commands=1;private_game_saves=0;roi_pixel_checks=43554;repeat_checks=7;copied_adoption_checks=7;normal_colour_invariance_checks=1;refusal_pixel_checks=2;calibration_pixel_checks=1024}
        Assert-TimerKeys $tune (@($tuneCounts.Keys)+$timerUpdates+@('scope')) 'closing Debug'
        foreach ($key in $tuneCounts.Keys) { Assert-TimerInteger $tune $key $tuneCounts[$key] }
        foreach ($key in $timerUpdates) { Assert-TimerInteger $tune $key 0 }
        if ($tune.scope-cne'debug-opaque-closing-text-and-refusal') { throw 'Closing-text Debug scope differs' }
        Assert-TimerCsvColumns $accepted @('state','raw_rgba_match','adoption_match','repeat_match','private_bytes','opaque_selected','frozen_style_resources_rng','token','scope') 7 'closing accepted'
        $acceptedNames=@('baseline','named','reset-original','rgb','rgba-ff','after-save-rgb','reset-saved')
        $tokens=@('141414','RED','141414','35BCE1','2060C0FF','E17C35','2060C0FF')
        for ($i=0;$i-lt7;++$i) {
            $row=$accepted[$i]; Assert-TimerFlags $row @('raw_rgba_match','adoption_match','repeat_match','private_bytes','opaque_selected','frozen_style_resources_rng')
            if ($row.state-cne$acceptedNames[$i] -or $row.token-cne$tokens[$i] -or $row.scope-cne'debug-opaque-closing-text') { throw 'Closing-text accepted token/opacity observation differs' }
        }
        Assert-TimerCsvColumns $refusals @('input','accepted','atomic_preserved','raster_unchanged','opaque_selected','current_token','frozen_style_resources_rng','scope') 2 'closing refusal'
        $invalid=@('2060C080','A532D600')
        for ($i=0;$i-lt2;++$i) {
            $row=$refusals[$i]; Assert-TimerFlags $row @('atomic_preserved','raster_unchanged','opaque_selected','frozen_style_resources_rng')
            if ($row.input-cne$invalid[$i] -or $row.accepted-cne'false' -or $row.current_token-cne'2060C0FF' -or $row.scope-cne'debug-opaque-refusal') { throw 'Closing-text partial/zero atomic refusal differs' }
        }
        Assert-TimerCsvColumns $normal @('state','context','normal_unchanged','frozen_style_resources_rng','scope') 2 'closing normal guard'
        for ($i=0;$i-lt2;++$i) {
            $row=$normal[$i]; $state=if ($i-eq0) {'baseline'} else {'rgba-ff'}; $same=if ($i-eq0) {'baseline'} else {'true'}
            if ($row.state-cne$state -or $row.context-cne'normal' -or $row.normal_unchanged-cne$same -or $row.frozen_style_resources_rng-cne'true' -or $row.scope-cne'debug-normal-invariance') { throw 'Closing command changed the normal branch guard' }
        }
        if ((Get-Item -LiteralPath $transcript).Length-le0 -or (Test-Path -LiteralPath (Join-Path $captures "$prefix-tune-scratch/profile/ColourWars/save_data.json"))) { throw 'Closing transcript or no-game-Save invariant differs' }
        foreach ($name in @('source/tuning.json','source/presentation.json','staged/tuning.json','staged/presentation.json')) { $inputs.Add((Join-Path $captures "$prefix-tune-scratch/$name")) }
        $observed.developer_commands=$tune; $observed.readbacks=82; $observed.csv_rows=29; $observed.calibration_pixel_checks=2624
    } elseif (@(Get-ChildItem -LiteralPath $captures -Filter "$prefix-tune-*").Count) { throw 'Release closing role must have no Debug artifacts' }
    return [ordered]@{subject='closing_text48';prefix=$prefix;png_scope='opaque-closing-text-glyph-paper-ROI';directory=[IO.Path]::GetFullPath($captures);observed=$observed;ppms=$ppmEvidence;inputs=@(foreach($path in $inputs){[ordered]@{path=$path;sha256=(Get-FileHash -LiteralPath $path).Hash}})}
}
