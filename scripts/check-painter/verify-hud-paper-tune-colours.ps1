# HUD shared-paper family: strict source-frozen producer schemas before export,
# immutable inputs and complete native PNG/PPM RGB parity after contact export.
$hpUpdates=@('local_updates','player_updates','level_updates','scene_updates','state_updates','input_updates','time_updates','input_polls','observation_weapon_updates')
function Get-HPSha([byte[]]$Bytes) { [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($Bytes)) }
function Assert-HPKeys($Object,[string[]]$Expected,[string]$Scope) {
    if ($Object -isnot [pscustomobject] -or (($Object.PSObject.Properties.Name|Sort-Object)-join'|') -cne (($Expected|Sort-Object)-join'|')) { throw "HUD paper exact $Scope schema differs" }
}
function Assert-HPInt($Object,[string]$Key,[long]$Expected) {
    $value=$Object.$Key
    if (($value -isnot [int] -and $value -isnot [long]) -or $value -ne $Expected) { throw "HUD paper exact integer differs: $Key" }
}
function Get-HPCsvInt($Row,[string]$Key) {
    $text=[string]$Row.$Key; $value=0L
    if ($text -cnotmatch '^(0|[1-9][0-9]*)$' -or ![long]::TryParse($text,[ref]$value)) { throw "HUD paper canonical CSV integer differs: $Key" }
    return $value
}
function Assert-HPColumns($Rows,[string[]]$Expected,[int]$Count,[string]$Scope) {
    if ($Rows.Count -ne $Count -or ($Rows[0].PSObject.Properties.Name-join'|') -cne ($Expected-join'|')) { throw "HUD paper exact $Scope CSV shape differs" }
}
function Assert-HPFlags($Row,[string[]]$Flags) {
    foreach ($flag in $Flags) { if ($Row.$flag -cne 'true') { throw "HUD paper native flag differs: $flag" } }
}
function Replace-HPPaperToken([string]$Text,[string]$Key,[string]$Before,[string]$After) {
    # The pinned authored source has HUD.paper first and flat. A narrow splice
    # must not edit another section's unrelated paper/ink/shade spelling.
    $blocks=[regex]::Matches($Text,'"hud"\s*:\s*\{\s*"paper"\s*:\s*\{[^{}]*\}')
    if ($blocks.Count -ne 1) { throw 'HUD paper independent Save target is not the pinned flat first HUD.paper object' }
    $block=$blocks[0]; $leaves=[regex]::Matches($block.Value,'"'+[regex]::Escape($Key)+'"\s*:\s*("[^"\\]*")')
    if ($leaves.Count -ne 1 -or $leaves[0].Groups[1].Value -cne ('"'+$Before+'"')) { throw 'HUD paper authored exact selected token differs' }
    $leaf=$leaves[0].Groups[1]; $at=$block.Index+$leaf.Index
    return $Text.Substring(0,$at)+'"'+$After+'"'+$Text.Substring($at+$leaf.Length)
}
$hpInputs=[Collections.Generic.List[string]]::new()
$hpSummaryPath=Join-Path $captures 'hud-paper-consumer-summary.json'
$hpMainPath=Join-Path $captures 'hud-paper-main.csv'
$hpBranchPath=Join-Path $captures 'hud-paper-branches.csv'
$hpControlPath=Join-Path $captures 'hud-paper-controls.csv'
foreach ($path in @($hpSummaryPath,$hpMainPath,$hpBranchPath,$hpControlPath,(Join-Path $run 'run.log'),(Join-Path $run 'run.err'))) {
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'HUD paper required evidence missing' }; $hpInputs.Add($path)
}
if (@(Get-ChildItem -LiteralPath $captures -Filter 'hud-paper-tune-failure-*').Count) { throw 'HUD paper source-hypothesis failure artifacts remain' }
$hpSummary=Get-Content -LiteralPath $hpSummaryPath -Raw|ConvertFrom-Json
$hpCounts=[ordered]@{main_observations=9;main_reads=36;raw_references=9;control_reads=12;controls=12;control_site_discriminations=202;control_pixel_upper_bound=3232;branch_observations=8;branch_reads=16;readbacks=64;main_rows=9;control_rows=12;branch_rows=8;csv_rows=29;repeat_checks=17;copied_adoption_checks=9;independent_observations=17;patch_pixel_checks=8464;normal_reduced_equalities=2;closing_region_invariance_checks=3;closing_region_pixels=60600;closing_full_hud_positive_changes=3;settled_reduced_equalities=2;tick_counterexamples=2;unselected_region_checks=35;unselected_pixel_checks=15008;band_canvas_relations=3;band_canvas_pixels=864;base_canvas_site_relations=18;base_canvas_pixels=288;crops=18;crop_pixels=898584;ppm_bytes=2696022;calibration_pixel_checks=2048;patches=35;patch_pixels=560;closing_patch_sites=24;closing_patch_pixels=384;flat_first_paper_sites=3;composed_unique_second_paper_sites=6;private_game_saves=0}
foreach ($key in $hpUpdates) { $hpCounts[$key]=0 }
Assert-HPKeys $hpSummary (@($hpCounts.Keys)+@('scope','control_discriminating_pixels')) 'shared'
foreach ($key in $hpCounts.Keys) { Assert-HPInt $hpSummary $key $hpCounts[$key] }
$hpControlPixels=$hpSummary.control_discriminating_pixels
if (($hpControlPixels -isnot [int] -and $hpControlPixels -isnot [long]) -or $hpControlPixels -lt 1 -or $hpControlPixels -gt 3232 -or $hpSummary.scope -cne 'hud-paper-material-patches-and-branch-isolation') { throw 'HUD paper shared scope or observed control support differs' }
$hpMain=@(Import-Csv -LiteralPath $hpMainPath); $hpBranches=@(Import-Csv -LiteralPath $hpBranchPath); $hpControls=@(Import-Csv -LiteralPath $hpControlPath)
Assert-HPColumns $hpMain @('state','background','ink_sites','flat_paper_sites','shade_base_sites','shade_band_sites','composed_second_paper_sites','raw_rgba_match','adoption_match','repeat_match','locality_match','unselected_interior_match','frozen_style_resources_rng','scope') 9 'main'
$hpMainOrder=@('default|black','default|nonblack','ink|nonblack','paper|nonblack','shade-opaque|black','shade-opaque|nonblack','shade-partial|black','shade-partial|nonblack','joint|nonblack')
$hpSiteKeys=@('ink_sites','flat_paper_sites','shade_base_sites','shade_band_sites','composed_second_paper_sites'); $hpSiteCounts=@(8,3,6,18,6)
for ($i=0;$i -lt 9;++$i) {
    $row=$hpMain[$i]; $expected=$hpMainOrder[$i].Split('|')
    Assert-HPFlags $row @('raw_rgba_match','adoption_match','repeat_match','locality_match','frozen_style_resources_rng')
    for ($site=0;$site -lt 5;++$site) { if ((Get-HPCsvInt $row $hpSiteKeys[$site]) -ne $hpSiteCounts[$site]) { throw 'HUD paper exact site classes differ; composed second-paper sites overlap shade bands' } }
    $interior=if($i -lt 2){'not-run-baseline'}else{'true'}
    if ($row.state -cne $expected[0] -or $row.background -cne $expected[1] -or $row.unselected_interior_match -cne $interior -or $row.scope -cne 'hud-paper-material-patches') { throw 'HUD paper main tuple/background/interior applicability differs' }
}
Assert-HPColumns $hpBranches @('state','context','applicable_sites','raw_rgba_match','repeat_match','selection_guard','clock_region_match','frozen_style_resources_rng','scope') 8 'branch'
$hpBranchOrder=@('default|reduced-normal','joint|reduced-normal','default|exact-ten','joint|exact-ten','default|fractional-closing','joint|fractional-closing','default|reduced-closing','joint|reduced-closing')
for ($i=0;$i -lt 8;++$i) {
    $row=$hpBranches[$i]; $expected=$hpBranchOrder[$i].Split('|'); $normal=$i -lt 2; $default=$expected[0] -ceq 'default'
    $sites=if($normal){35}else{24}; $guard=if($normal){'normal-reduced-equal'}elseif($default){'baseline'}else{'closing-panels-localized'}
    $clock=if($normal){'not-run-normal-whole-frame-relation'}elseif($default){'baseline'}else{'true'}
    Assert-HPFlags $row @('raw_rgba_match','repeat_match','frozen_style_resources_rng')
    if ($row.state -cne $expected[0] -or $row.context -cne $expected[1] -or (Get-HPCsvInt $row 'applicable_sites') -ne $sites -or $row.selection_guard -cne $guard -or $row.clock_region_match -cne $clock -or $row.scope -cne 'hud-paper-applicability') { throw 'HUD paper normal/closing subset or bounded clock-region isolation differs' }
}
Assert-HPColumns $hpControls @('material','control','background','affected_sites','discriminating_pixels','unselected_exterior_match','scope') 12 'control'
$hpControlOrder=@('ink|wrong-opaque|nonblack|8|128','ink|omitted|nonblack|8|128','paper|wrong-opaque|nonblack|21|336','paper|omitted|nonblack|21|336','shade|straight|black|24|384','shade|straight|nonblack|24|384','shade|double-submitted-alpha|black|24|384','shade|double-submitted-alpha|nonblack|24|384','shade|forced-black-band|nonblack|18|288','shade|omitted-band-scale|nonblack|18|288','shade|omitted-middle-bottom|nonblack|6|96','shade|omitted-middle-right|nonblack|6|96')
$hpSumSites=0L; $hpSumPixels=0L
for ($i=0;$i -lt 12;++$i) {
    $row=$hpControls[$i]; $expected=$hpControlOrder[$i].Split('|'); $sites=Get-HPCsvInt $row 'affected_sites'; $pixels=Get-HPCsvInt $row 'discriminating_pixels'
    if ($row.material -cne $expected[0] -or $row.control -cne $expected[1] -or $row.background -cne $expected[2] -or $sites -ne [long]$expected[3] -or $pixels -lt 1 -or $pixels -gt [long]$expected[4] -or $row.unselected_exterior_match -cne 'true' -or $row.scope -cne 'hud-paper-material-counterexample') { throw 'HUD paper control role, individual site discrimination or exact exterior differs' }
    $hpSumSites+=$sites; $hpSumPixels+=$pixels
}
if ($hpSumSites -ne 202 -or $hpSumPixels -ne $hpControlPixels) { throw 'HUD paper observed control totals do not reconcile' }
$hpCropStates=@('default','ink','paper','shade-opaque','shade-partial','joint')
$hpPanels=@('identity|491|144|70704','meters|327|180|58860','timer|202|100|20200')
$hpCropPlan=@(foreach ($state in $hpCropStates) { foreach ($panel in $hpPanels) {
    $parts=$panel.Split('|'); [ordered]@{name="hud-paper-colour-$state-$($parts[0]).ppm";panel=$parts[0];width=[int]$parts[1];height=[int]$parts[2];pixels=[int]$parts[3]}
} })
$script:hudPaperObserved=[ordered]@{consumer=$hpSummary;readbacks=64;csv_rows=29;crops=18;crop_pixels=898584;ppm_bytes=2696022;controls=12;control_site_discriminations=202;control_discriminating_pixels=$hpControlPixels;calibration_pixel_checks=2048;private_game_saves=0;opacity_policy='ink/paper distinct and opaque; shade positive alpha; no new contrast rule';reference_scope='35 literal material sites: three shared panels plus two direct meter ink borders; composed second-paper underlays overlap band sites; bounded closing-clock region only is invariant';png_count=0;png_crops=@();png_complete=$false}
if ($debug) {
    $tunePath=Join-Path $captures 'hud-paper-tune-summary.json'; $tuneCsv=Join-Path $captures 'hud-paper-tune.csv'
    $refusalPath=Join-Path $captures 'hud-paper-tune-refusals.json'; $transcript=Join-Path $captures 'hud-paper-tune-commands.log'
    foreach ($path in @($tunePath,$tuneCsv,$refusalPath,$transcript)) { if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'HUD paper required Debug evidence missing' }; $hpInputs.Add($path) }
    $tune=Get-Content -LiteralPath $tunePath -Raw|ConvertFrom-Json
    $tuneCounts=[ordered]@{accepted_rendered_states=9;accepted_reads=27;new_raw_reads=4;cached_reference_observations=5;refusal_reads=5;readbacks=36;accepted_rows=9;refusal_rows=5;csv_rows=14;repeat_checks=9;copied_adoption_checks=9;independent_observations=9;patch_pixel_checks=5040;saved_tokens=3;original_reset_frame_equalities=1;saved_reset_frame_equalities=1;sibling_pending_reset_checks=3;refusal_json_records=5;crops=0;calibration_pixel_checks=1152;commands=20;accepted_commands=15;query_commands=3;tune_commands=7;reset_commands=4;presentation_save_commands=1;refused_commands=5;refusal_raster_reads=5;private_game_saves=0}
    foreach ($key in $hpUpdates) { $tuneCounts[$key]=0 }
    Assert-HPKeys $tune (@($tuneCounts.Keys)+@('scope')) 'Debug'
    foreach ($key in $tuneCounts.Keys) { Assert-HPInt $tune $key $tuneCounts[$key] }
    if ($tune.scope -cne 'debug-live-hud-paper-family-and-policy-refusal') { throw 'HUD paper Debug scope differs' }
    $rows=@(Import-Csv -LiteralPath $tuneCsv)
    Assert-HPColumns $rows @('event','state','ink_token','paper_token','shade_token','raw_reference_source','raw_rgba_match','adoption_match','repeat_match','unchanged_frame','private_bytes','siblings_preserved','frozen_style_resources_rng','scope') 14 'Debug'
    $hpTuneOrder=@('accepted|baseline|141414|FFF5DF|00000055|shared-default','accepted|ink-edit|RED|FFF5DF|00000055|shared-ink','accepted|reset-original|141414|FFF5DF|00000055|shared-default','accepted|pair-pending|RED|35BCE1|00000055|new-raw','refused|equal-ink|RED|35BCE1|00000055|not-run-policy-refusal','refused|equal-paper|RED|35BCE1|00000055|not-run-policy-refusal','refused|partial-ink|RED|35BCE1|00000055|not-run-policy-refusal','refused|partial-paper|RED|35BCE1|00000055|not-run-policy-refusal','accepted|joint-pending|RED|35BCE1|2060C080|shared-joint','refused|zero-shade|RED|35BCE1|2060C080|not-run-policy-refusal','accepted|all-overwrite|2060C0|FFF5DF|FF0000|new-raw','accepted|reset-ink|RED|FFF5DF|FF0000|new-raw','accepted|reset-paper|RED|35BCE1|FF0000|new-raw','accepted|reset-shade|RED|35BCE1|2060C080|shared-joint')
    for ($i=0;$i -lt 14;++$i) {
        $row=$rows[$i]; $expected=$hpTuneOrder[$i].Split('|'); $accepted=$expected[0] -ceq 'accepted'
        $raw=if($accepted){'true'}else{'not-run-refusal-held-frame-only'}
        $unchanged=if($accepted){'not-run-accepted-state'}else{'true'}
        $scope=if($accepted){'debug-live-hud-paper-family'}else{'debug-atomic-hud-paper-policy-refusal'}
        Assert-HPFlags $row @('private_bytes','siblings_preserved','frozen_style_resources_rng')
        if ($row.event -cne $expected[0] -or $row.state -cne $expected[1] -or $row.ink_token -cne $expected[2] -or $row.paper_token -cne $expected[3] -or $row.shade_token -cne $expected[4] -or $row.raw_reference_source -cne $expected[5] -or $row.raw_rgba_match -cne $raw -or $row.adoption_match -cne $raw -or $row.repeat_match -cne $raw -or $row.unchanged_frame -cne $unchanged -or $row.scope -cne $scope) { throw 'HUD paper Debug exact tuple, accepted/raw or refused/held-frame semantics differ' }
    }
    $refusals=Get-Content -LiteralPath $refusalPath -Raw|ConvertFrom-Json
    Assert-HPKeys $refusals @('scope','records') 'refusal envelope'
    if ($refusals.scope -cne 'atomic-hud-paper-policy-refusals-with-held-frame-reads' -or $refusals.records.Count -ne 5) { throw 'HUD paper refusal envelope differs' }
    $hpRefusalOrder=@('pair-pending|ink|35BCE1|equal-opaque-siblings|2','pair-pending|paper|RED|equal-opaque-siblings|2','pair-pending|ink|2060C080|opaque-ink-required|2','pair-pending|paper|35BCE180|opaque-paper-required|2','joint-pending|shade|A532D600|positive-shade-alpha-required|3')
    for ($i=0;$i -lt 5;++$i) {
        $record=$refusals.records[$i]; $expected=$hpRefusalOrder[$i].Split('|')
        Assert-HPKeys $record @('phase','path','input','policy','pending_tokens','accepted','atomic_preserved','live_tuple_preserved','documents_preserved','bytes_preserved','resources_rng_preserved','raster_checked','raster_reads') 'refusal record'
        foreach ($flag in @('atomic_preserved','live_tuple_preserved','documents_preserved','bytes_preserved','resources_rng_preserved','raster_checked')) { if ($record.$flag -isnot [bool] -or !$record.$flag) { throw 'HUD paper typed atomic/held-frame refusal flag differs' } }
        if ($record.accepted -isnot [bool] -or $record.accepted -or $record.phase -cne $expected[0] -or $record.path -cne ('presentation.hud.paper.'+$expected[1]) -or $record.input -cne $expected[2] -or $record.policy -cne $expected[3]) { throw 'HUD paper exact refusal policy/phase/path/input differs' }
        Assert-HPInt $record 'pending_tokens' ([long]$expected[4]); Assert-HPInt $record 'raster_reads' 1
    }
    $originalPresentation=Join-Path $gameDirectory 'presentation.json'; $originalTuning=Join-Path $gameDirectory 'tuning.json'
    foreach ($path in @($originalPresentation,$originalTuning)) { if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'HUD paper original staged source missing' }; $hpInputs.Add($path) }
    $originalBytes=[IO.File]::ReadAllBytes($originalPresentation); $sourceText=[IO.File]::ReadAllText($originalPresentation)
    if ((Get-HPSha $originalBytes) -cne (Get-HPSha ([Text.Encoding]::UTF8.GetBytes($sourceText)))) { throw 'HUD paper original source is not exact UTF8 without BOM' }
    $savedText=Replace-HPPaperToken (Replace-HPPaperToken (Replace-HPPaperToken $sourceText 'ink' '141414' 'RED') 'paper' 'FFF5DF' '35BCE1') 'shade' '00000055' '2060C080'
    $savedPresentation=[Text.Encoding]::UTF8.GetBytes($savedText); $originalTuningBytes=[IO.File]::ReadAllBytes($originalTuning)
    foreach ($directory in @('source','staged')) { foreach ($name in @('presentation.json','tuning.json')) {
        $path=Join-Path $captures "hud-paper-tune-scratch/$directory/$name"
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'HUD paper private saved file missing' }
        $expected=if($name -ceq 'presentation.json'){$savedPresentation}else{$originalTuningBytes}
        if ((Get-HPSha ([IO.File]::ReadAllBytes($path))) -cne (Get-HPSha $expected)) { throw 'HUD paper private bytes differ from three exact independent token splices' }; $hpInputs.Add($path)
    } }
    if (Test-Path -LiteralPath (Join-Path $captures 'hud-paper-tune-scratch/profile/ColourWars/save_data.json')) { throw 'HUD paper fixture wrote a private game Save' }
    $log=@(Get-Content -LiteralPath $transcript)
    foreach ($query in @('presentation.hud.paper.ink = "141414"  [live]','presentation.hud.paper.paper = "FFF5DF"  [live]','presentation.hud.paper.shade = "00000055"  [live]')) {
        if (@($log|Where-Object{$_ -ceq $query}).Count -ne 1) { throw 'HUD paper actual value-less live query transcript differs' }
    }
    $expectedTunes=@('tune: presentation.hud.paper.ink "141414" -> "RED" (live)','tune: presentation.hud.paper.ink "141414" -> "RED" (live)','tune: presentation.hud.paper.paper "FFF5DF" -> "35BCE1" (live)','tune: presentation.hud.paper.shade "00000055" -> "2060C080" (live)','tune: presentation.hud.paper.ink "RED" -> "2060C0" (live)','tune: presentation.hud.paper.paper "35BCE1" -> "FFF5DF" (live)','tune: presentation.hud.paper.shade "2060C080" -> "FF0000" (live)')
    $actualTunes=@($log|Where-Object{$_ -cmatch '^tune: presentation\.hud\.paper\.(ink|paper|shade) '})
    $expectedResets=@('tune.reset: presentation.hud.paper.ink "RED" -> "141414" (live)','tune.reset: presentation.hud.paper.ink "2060C0" -> "RED" (live)','tune.reset: presentation.hud.paper.paper "FFF5DF" -> "35BCE1" (live)','tune.reset: presentation.hud.paper.shade "FF0000" -> "2060C080" (live)')
    $actualResets=@($log|Where-Object{$_ -cmatch '^tune\.reset: presentation\.hud\.paper\.(ink|paper|shade) '})
    if (($actualTunes -join '|') -cne ($expectedTunes -join '|') -or ($actualResets -join '|') -cne ($expectedResets -join '|') -or
        @($log|Where-Object{$_ -cmatch '^tune\.save: 3 value\(s\) written to '}).Count -ne 1 -or
        @($log|Where-Object{$_ -cmatch 'distinct opaque ink/paper and visible shade required\.'}).Count -ne 5) { throw 'HUD paper actual edit/reset/three-token Save/reader-refusal transcript differs' }
    $script:hudPaperObserved.developer_commands=$tune; $script:hudPaperObserved.readbacks=100
    $script:hudPaperObserved.csv_rows=43; $script:hudPaperObserved.calibration_pixel_checks=3200
} elseif (@(Get-ChildItem -LiteralPath $captures -Filter 'hud-paper-tune-*').Count) { throw 'Release HUD paper must contain no Debug artifacts' }
$actualPpms=@(Get-ChildItem -LiteralPath $captures -Filter 'hud-paper-colour-*.ppm' -File)
$expectedNames=@($hpCropPlan|ForEach-Object{$_.name})
if ($actualPpms.Count -ne 18 -or (($actualPpms.Name|Sort-Object)-join'|') -cne (($expectedNames|Sort-Object)-join'|')) { throw 'HUD paper exact native PPM inventory differs' }
$hpPpms=@(foreach ($crop in $hpCropPlan) {
    $path=Join-Path $captures $crop.name; $bytes=[IO.File]::ReadAllBytes($path)
    $header=[Text.Encoding]::ASCII.GetBytes("P6`n$($crop.width) $($crop.height)`n255`n"); $payload=$crop.pixels*3
    if ($header.Length -ne 15 -or $bytes.Length -ne $payload+15 -or $crop.pixels -ne $crop.width*$crop.height) { throw 'HUD paper canonical P6 dimensions/length differ' }
    for ($i=0;$i -lt 15;++$i) { if ($bytes[$i] -ne $header[$i]) { throw 'HUD paper canonical native P6 header differs' } }
    $rgb=[byte[]]::new($payload); [Array]::Copy($bytes,15,$rgb,0,$payload); $hpInputs.Add($path)
    [ordered]@{name=$crop.name;panel=$crop.panel;width=$crop.width;height=$crop.height;pixels=$crop.pixels;header_bytes=15;bytes=$bytes.Length;sha256=(Get-HPSha $bytes);rgb_payload_sha256=(Get-HPSha $rgb)}
})
$hpTotalPixels=0L; $hpTotalPpmBytes=0L
foreach ($ppm in $hpPpms) { $hpTotalPixels += [long]$ppm.pixels; $hpTotalPpmBytes += [long]$ppm.bytes }
if ($hpTotalPixels -ne 898584 -or $hpTotalPpmBytes -ne 2696022) { throw 'HUD paper native crop totals differ' }
if (@(Get-Content -LiteralPath (Join-Path $run 'run.log')|Where-Object{$_ -match 'Saving\.\.\.$' -or $_ -ceq 'Save was successful.'}).Count -or
    @(Get-Content -LiteralPath (Join-Path $run 'run.err')|Where-Object{$_ -match '^save failure:'}).Count) { throw 'HUD paper leaf called or failed a game Save' }
$script:hudPaperVerified=[ordered]@{directory=[IO.Path]::GetFullPath($captures);ppms=$hpPpms;inputs=@(foreach($path in $hpInputs){[ordered]@{path=$path;sha256=(Get-FileHash -LiteralPath $path).Hash}})}
$result.observed_counts=$script:hudPaperObserved
function Complete-HudPaperTuneColourEvidence {
    param([Parameter(Mandatory)][string]$Directory)
    $context=$script:hudPaperVerified
    if ($null -eq $context -or ![StringComparer]::OrdinalIgnoreCase.Equals([IO.Path]::GetFullPath($Directory),$context.directory)) { throw 'HUD paper PNG completion directory differs' }
    foreach ($inputFile in $context.inputs) { if (!(Test-Path -LiteralPath $inputFile.path -PathType Leaf) -or (Get-FileHash -LiteralPath $inputFile.path).Hash -cne $inputFile.sha256) { throw 'HUD paper verified input changed before PNG completion' } }
    $pngs=@(Get-ChildItem -LiteralPath $Directory -Filter 'hud-paper-colour-*.png' -File)
    $expected=@($context.ppms|ForEach-Object{[IO.Path]::ChangeExtension($_.name,'.png')})
    if ($pngs.Count -ne 18 -or (($pngs.Name|Sort-Object)-join'|') -cne (($expected|Sort-Object)-join'|')) { throw 'HUD paper exact native PNG inventory differs' }
    Add-Type -AssemblyName System.Drawing
    $evidence=@(foreach ($ppm in $context.ppms) {
        $ppmPath=Join-Path $Directory $ppm.name; $ppmBytes=[IO.File]::ReadAllBytes($ppmPath)
        if ((Get-HPSha $ppmBytes) -cne $ppm.sha256) { throw 'HUD paper PPM changed before PNG decode' }
        $path=[IO.Path]::ChangeExtension($ppmPath,'.png'); $bytes=[IO.File]::ReadAllBytes($path); $signature=@(137,80,78,71,13,10,26,10)
        if ($bytes.Length -lt 33) { throw 'HUD paper PNG header truncated' }
        for ($i=0;$i -lt 8;++$i) { if ($bytes[$i] -ne $signature[$i]) { throw 'HUD paper PNG signature differs' } }
        if (($bytes[8..11]-join',') -cne '0,0,0,13' -or [Text.Encoding]::ASCII.GetString($bytes,12,4) -cne 'IHDR') { throw 'HUD paper PNG initial IHDR differs' }
        $width=0L; $height=0L
        for ($i=0;$i -lt 4;++$i) { $width=$width*256+$bytes[16+$i]; $height=$height*256+$bytes[20+$i] }
        if ($width -ne $ppm.width -or $height -ne $ppm.height -or $bytes[24] -ne 8 -or $bytes[25] -notin @(2,6) -or $bytes[26] -ne 0 -or $bytes[27] -ne 0 -or $bytes[28] -notin @(0,1)) { throw 'HUD paper native RGB/RGBA PNG IHDR differs' }
        $bitmap=[Drawing.Bitmap]::new($path)
        try {
            if ($bitmap.Width -ne $ppm.width -or $bitmap.Height -ne $ppm.height) { throw 'HUD paper decoded PNG dimensions differ' }
            $rgb=[byte[]]::new($ppm.pixels*3)
            for ($y=0;$y -lt $ppm.height;++$y) { for ($x=0;$x -lt $ppm.width;++$x) {
                $pixel=$bitmap.GetPixel($x,$y); $at=($y*$ppm.width+$x)*3
                if ($pixel.A -ne 255) { throw 'HUD paper decoded PNG alpha is not opaque' }
                $rgb[$at]=$pixel.R; $rgb[$at+1]=$pixel.G; $rgb[$at+2]=$pixel.B
            } }
        } finally { $bitmap.Dispose() }
        for ($i=0;$i -lt $rgb.Length;++$i) { if ($rgb[$i] -ne $ppmBytes[15+$i]) { throw 'HUD paper decoded PNG differs from full retained PPM RGB' } }
        $digest=Get-HPSha $rgb
        if ($digest -cne $ppm.rgb_payload_sha256) { throw 'HUD paper decoded PNG RGB fingerprint differs' }
        $metadata=[ordered]@{name=[IO.Path]::GetFileName($path);source_ppm=$ppm.name;scope=('hud-paper-'+$ppm.panel+'-panel-context');width=$ppm.width;height=$ppm.height;pixels=$ppm.pixels;bit_depth=[int]$bytes[24];colour_type=[int]$bytes[25];ppm_sha256=$ppm.sha256;png_sha256=(Get-HPSha $bytes);decoded_rgb_sha256=$digest;decoded_rgb_matches_ppm=$true;alpha_opaque=$true}
        $metadata|ConvertTo-Json -Depth 5|Set-Content -LiteralPath "$path.metadata.json" -Encoding utf8
        $metadata
    })
    $script:hudPaperObserved.png_crops=$evidence; $script:hudPaperObserved.png_count=18
    $script:hudPaperObserved.png_pixels=898584; $script:hudPaperObserved.png_rgb_bytes=2695752; $script:hudPaperObserved.png_complete=$true
}