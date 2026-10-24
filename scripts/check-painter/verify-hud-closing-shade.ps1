# Isolated shade55 leaf: strict producer contracts before contact export,
# complete decoded native PNG equality afterward. No retained code is executed.
$csUpdates = @('local_updates','player_updates','level_updates','scene_updates','state_updates','input_updates','time_updates','input_polls','observation_weapon_updates')
function Get-CSha([byte[]]$Bytes) { [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($Bytes)) }
function Assert-CSKeys($Object, [string[]]$Expected, [string]$Scope) {
    if ($Object -isnot [pscustomobject] -or (($Object.PSObject.Properties.Name | Sort-Object) -join '|') -cne (($Expected | Sort-Object) -join '|')) { throw "Closing shade exact $Scope schema differs" }
}
function Assert-CSInt($Object, [string]$Key, [long]$Expected) {
    $value = $Object.$Key
    if (($value -isnot [int] -and $value -isnot [long]) -or $value -ne $Expected) { throw "Closing shade exact integer differs: $Key" }
}
function Get-CSCsvInt($Row, [string]$Key) {
    $text = [string]$Row.$Key; $value = 0L
    if ($text -cnotmatch '^(0|[1-9][0-9]*)$' -or ![long]::TryParse($text, [ref]$value)) { throw "Closing shade canonical CSV integer differs: $Key" }
    return $value
}
function Assert-CSColumns($Rows, [string[]]$Expected, [int]$Count, [string]$Scope) {
    if ($Rows.Count -ne $Count -or ($Rows[0].PSObject.Properties.Name -join '|') -cne ($Expected -join '|')) { throw "Closing shade exact $Scope CSV shape differs" }
}
function Assert-CSFlags($Row, [string[]]$Flags) {
    foreach ($flag in $Flags) { if ($Row.$flag -cne 'true') { throw "Closing shade native flag differs: $flag" } }
}
function Replace-CSShadeToken([string]$Text) {
    $blocks = [regex]::Matches($Text, '"closing_clock"\s*:\s*\{[^{}]*\}')
    if ($blocks.Count -ne 1) { throw 'Closing shade independent Save check requires one flat closing-clock object' }
    $block = $blocks[0]; $leaves = [regex]::Matches($block.Value, '"shade"\s*:\s*("[^"\\]*")')
    if ($leaves.Count -ne 1 -or $leaves[0].Groups[1].Value -cne '"00000055"') { throw 'Closing shade exact authored source token differs' }
    $leaf = $leaves[0].Groups[1]; $at = $block.Index + $leaf.Index
    return $Text.Substring(0, $at) + '"2060C080"' + $Text.Substring($at + $leaf.Length)
}
$csInputs = [Collections.Generic.List[string]]::new()
$csSummaryPath = Join-Path $captures 'closing-shade-consumer-summary.json'
$csMainPath = Join-Path $captures 'closing-shade-main.csv'
$csBranchPath = Join-Path $captures 'closing-shade-branches.csv'
$csControlPath = Join-Path $captures 'closing-shade-controls.csv'
foreach ($path in @($csSummaryPath,$csMainPath,$csBranchPath,$csControlPath,(Join-Path $run 'run.log'),(Join-Path $run 'run.err'))) {
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Closing shade required evidence missing' }; $csInputs.Add($path)
}
if (@(Get-ChildItem -LiteralPath $captures -Filter 'closing-shade-failure-*').Count) { throw 'Closing shade source-hypothesis failure artifacts remain' }
$csSummary = Get-Content -LiteralPath $csSummaryPath -Raw | ConvertFrom-Json
$csCounts = [ordered]@{main_observations=8;main_reads=32;branch_observations=14;branch_reads=28;raw_references=8;controls=8;control_reads=8;control_site_discriminations=48;control_pixel_upper_bound=768;readbacks=68;main_rows=8;branch_rows=14;control_rows=8;csv_rows=30;repeat_checks=22;copied_adoption_checks=8;independent_patch_observations=12;patch_pixel_checks=1536;normal_invariance_checks=5;normal_reduced_relations=2;settled_reduced_relations=2;beat_counterexamples=2;crops=12;calibration_pixel_checks=2176;band_canvas_invariance_checks=4;band_canvas_pixel_checks=384;base_canvas_site_relations=8;base_canvas_relation_pixels=128;patches=8;patch_pixels=128;crop_width=202;crop_height=100;crop_pixels=242400;ppm_bytes=727380;private_game_saves=0}
foreach ($key in $csUpdates) { $csCounts[$key] = 0 }
Assert-CSKeys $csSummary (@($csCounts.Keys) + @('scope','control_discriminating_pixels')) 'shared'
foreach ($key in $csCounts.Keys) { Assert-CSInt $csSummary $key $csCounts[$key] }
$csControlPixels = $csSummary.control_discriminating_pixels
if (($csControlPixels -isnot [int] -and $csControlPixels -isnot [long]) -or $csControlPixels -le 0 -or $csControlPixels -gt 768 -or $csSummary.scope -cne 'closing-shade-material-patches-and-applicability') { throw 'Closing shade shared scope or observed control support differs' }
$csMain = @(Import-Csv -LiteralPath $csMainPath); $csBranches = @(Import-Csv -LiteralPath $csBranchPath); $csControls = @(Import-Csv -LiteralPath $csControlPath)
Assert-CSColumns $csMain @('state','background','raw_rgba_match','adoption_match','repeat_match','locality_match','positive_alpha','frozen_style_resources_rng','scope') 8 'main'
$csStates = @('default','opaque-red','partial','low-alpha'); $csBackgrounds = @('black','nonblack')
for ($i=0; $i -lt 8; ++$i) {
    $row = $csMain[$i]; Assert-CSFlags $row @('raw_rgba_match','adoption_match','repeat_match','locality_match','positive_alpha','frozen_style_resources_rng')
    if ($row.state -cne $csStates[[int][math]::Floor($i/2)] -or $row.background -cne $csBackgrounds[$i%2] -or $row.scope -cne 'closing-shade-material-patches') { throw 'Closing shade exact palette/background order differs' }
}
Assert-CSColumns $csBranches @('state','context','raw_rgba_match','repeat_match','selection_guard','positive_alpha','frozen_style_resources_rng','scope') 14 'branch'
$csContexts = @('normal','above-ten','fractional-closing','zero','stopped-short','reduced-normal','reduced-closing')
for ($i=0; $i -lt 14; ++$i) {
    $row = $csBranches[$i]; $state = if ($i%2 -eq 0) {'default'} else {'partial'}; $context = $csContexts[[int][math]::Floor($i/2)]
    $closing = $context -cin @('fractional-closing','reduced-closing')
    $raw = if ($closing) {'true'} else {'not-run-normal-invariance-only'}
    $guard = if ($state -ceq 'default') {'baseline'} elseif ($closing) {'closing-locality'} else {'normal-exact'}
    Assert-CSFlags $row @('repeat_match','positive_alpha','frozen_style_resources_rng')
    if ($row.state -cne $state -or $row.context -cne $context -or $row.raw_rgba_match -cne $raw -or $row.selection_guard -cne $guard -or $row.scope -cne 'closing-shade-applicability') { throw 'Closing shade exact normal/closing branch applicability differs' }
}
Assert-CSColumns $csControls @('control','background','patch_discriminations','discriminating_pixels','exterior_unchanged','scope') 8 'control'
$csControlNames = @('straight','straight','double-submitted-alpha','double-submitted-alpha','forced-black-band','omitted-band-scale','omitted-middle-bottom','omitted-middle-right')
$csControlBackgrounds = @('black','nonblack','black','nonblack','nonblack','nonblack','nonblack','nonblack')
$csSites = @(8,8,8,8,6,6,2,2); $csBounds = @(128,128,128,128,96,96,32,32)
$csSumPixels=0L; $csSumSites=0L
for ($i=0; $i -lt 8; ++$i) {
    $row=$csControls[$i]; $sites=Get-CSCsvInt $row 'patch_discriminations'; $pixels=Get-CSCsvInt $row 'discriminating_pixels'
    if ($row.control -cne $csControlNames[$i] -or $row.background -cne $csControlBackgrounds[$i] -or $sites -ne $csSites[$i] -or $pixels -le 0 -or $pixels -gt $csBounds[$i] -or $row.exterior_unchanged -cne 'true' -or $row.scope -cne 'closing-shade-material-counterexample') { throw 'Closing shade wrong-material per-site applicability/support differs' }
    $csSumPixels += $pixels; $csSumSites += $sites
}
if ($csSumPixels -ne $csControlPixels -or $csSumSites -ne 48) { throw 'Closing shade observed control totals do not reconcile' }
$csCropNames = @(foreach ($state in $csStates) { foreach ($background in $csBackgrounds) { "closing-shade-colour-$state-$background.ppm" } })
$csCropNames += @(foreach ($context in @('fractional-closing','reduced-closing')) { foreach ($state in @('default','partial')) { "closing-shade-colour-$context-$state.ppm" } })
$script:closingShadeObserved = [ordered]@{consumer=$csSummary;readbacks=68;csv_rows=30;crops=12;crop_pixels=242400;ppm_bytes=727380;controls=8;control_site_discriminations=48;control_discriminating_pixels=$csControlPixels;calibration_pixel_checks=2176;private_game_saves=0;default_token='00000055';opacity_policy='positive alpha; zero refused; band submitted alpha separately scales by .12';reference_scope='eight source material patches only; base Canvas and opaque-paper band underlays; full RGB retained in both; complete copied HUD proves adoption separately';png_count=0;png_crops=@();png_complete=$false}
if ($debug) {
    $tunePath=Join-Path $captures 'closing-shade-tune-summary.json'; $tuneCsv=Join-Path $captures 'closing-shade-tune.csv'
    $refusalPath=Join-Path $captures 'closing-shade-tune-refusal.json'; $transcript=Join-Path $captures 'closing-shade-tune-commands.log'
    foreach ($path in @($tunePath,$tuneCsv,$refusalPath,$transcript)) {
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Closing shade required Debug evidence missing' }; $csInputs.Add($path)
    }
    $tune = Get-Content -LiteralPath $tunePath -Raw | ConvertFrom-Json
    $tuneCounts=[ordered]@{commands=10;query_commands=1;tune_commands=5;reset_commands=2;presentation_save_commands=1;refused_commands=1;accepted_rendered_states=8;accepted_state_reads=24;refusal_reads=1;readbacks=25;accepted_rows=8;refusal_rows=1;csv_rows=9;repeat_checks=8;copied_adoption_checks=8;raw_gpu_reads=0;cached_patch_observations=8;patch_pixel_checks=1024;saved_tokens=1;crops=0;calibration_pixel_checks=800;refusal_json_records=1;refusal_raster_reads=1;saved_frame_equalities=2;private_game_saves=0}
    foreach ($key in $csUpdates) { $tuneCounts[$key] = 0 }
    Assert-CSKeys $tune (@($tuneCounts.Keys) + @('scope')) 'Debug'
    foreach ($key in $tuneCounts.Keys) { Assert-CSInt $tune $key $tuneCounts[$key] }
    if ($tune.scope -cne 'debug-live-positive-alpha-closing-shade') { throw 'Closing shade Debug scope differs' }
    $rows=@(Import-Csv -LiteralPath $tuneCsv)
    Assert-CSColumns $rows @('state','token','raw_rgba_match','adoption_match','repeat_match','unchanged_frame','private_bytes','positive_alpha','frozen_style_resources_rng','scope') 9 'Debug'
    $states=@('baseline','named','reset-original','partial','low-alpha','restored-partial','zero-refused','rgb','reset-saved')
    $tokens=@('00000055','RED','00000055','2060C080','A532D601','2060C080','2060C080','FF0000','2060C080')
    for ($i=0; $i -lt 9; ++$i) {
        $row=$rows[$i]; Assert-CSFlags $row @('private_bytes','positive_alpha','frozen_style_resources_rng')
        $refused=$i -eq 6; $raw=if($refused){'not-run-refusal-unchanged-frame-only'}else{'true'}
        $unchanged=if($refused){'true'}else{'not-run-accepted-state'}
        $scope=if($refused){'debug-closing-shade-zero-refusal'}else{'debug-live-closing-shade'}
        if ($row.state -cne $states[$i] -or $row.token -cne $tokens[$i] -or $row.raw_rgba_match -cne $raw -or $row.adoption_match -cne $raw -or $row.repeat_match -cne $raw -or $row.unchanged_frame -cne $unchanged -or $row.scope -cne $scope) { throw 'Closing shade Debug accepted/refused semantics or exact token differ' }
    }
    $refusal=Get-Content -LiteralPath $refusalPath -Raw|ConvertFrom-Json
    Assert-CSKeys $refusal @('scope','path','input','accepted','atomic_preserved','positive_alpha','documents_preserved','bytes_preserved','resources_rng_preserved','raster_checked','raster_reads') 'refusal'
    foreach ($key in @('atomic_preserved','positive_alpha','documents_preserved','bytes_preserved','resources_rng_preserved','raster_checked')) { if ($refusal.$key -isnot [bool] -or !$refusal.$key) { throw 'Closing shade atomic/refusal raster evidence flag differs' } }
    if ($refusal.accepted -isnot [bool] -or $refusal.accepted -or $refusal.scope -cne 'atomic-positive-alpha-reader-refusal-with-one-held-frame-read' -or $refusal.path -cne 'presentation.hud.closing_clock.shade' -or $refusal.input -cne 'A532D600') { throw 'Closing shade refusal exact domain or input differs' }
    Assert-CSInt $refusal 'raster_reads' 1
    $originalPresentation=Join-Path $gameDirectory 'presentation.json'; $originalTuning=Join-Path $gameDirectory 'tuning.json'
    foreach ($path in @($originalPresentation,$originalTuning)) { if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Closing shade original staged source missing' }; $csInputs.Add($path) }
    $originalBytes=[IO.File]::ReadAllBytes($originalPresentation); $sourceText=[IO.File]::ReadAllText($originalPresentation)
    if ((Get-CSha $originalBytes) -cne (Get-CSha ([Text.Encoding]::UTF8.GetBytes($sourceText)))) { throw 'Closing shade source encoding is not exact UTF8 without BOM' }
    $expectedPresentation=[Text.Encoding]::UTF8.GetBytes((Replace-CSShadeToken $sourceText)); $originalTuningBytes=[IO.File]::ReadAllBytes($originalTuning)
    foreach ($directory in @('source','staged')) { foreach ($name in @('presentation.json','tuning.json')) {
        $path=Join-Path $captures "closing-shade-tune-scratch/$directory/$name"
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Closing shade private saved file missing' }
        $expected=if($name -ceq 'presentation.json'){$expectedPresentation}else{$originalTuningBytes}
        if ((Get-CSha ([IO.File]::ReadAllBytes($path))) -cne (Get-CSha $expected)) { throw 'Closing shade saved bytes differ from one independent exact token' }; $csInputs.Add($path)
    } }
    if (Test-Path -LiteralPath (Join-Path $captures 'closing-shade-tune-scratch/profile/ColourWars/save_data.json')) { throw 'Closing shade fixture wrote a private game Save' }
    $log=@(Get-Content -LiteralPath $transcript)
    if (@($log|Where-Object{$_ -ceq 'presentation.hud.closing_clock.shade = "00000055"  [live]'}).Count -ne 1 -or
        @($log|Where-Object{$_ -cmatch '^tune: presentation\.hud\.closing_clock\.shade "[A-Z0-9]+" -> "[A-Z0-9]+" \(live\)$'}).Count -ne 5 -or
        @($log|Where-Object{$_ -cmatch '^tune\.reset: presentation\.hud\.closing_clock\.shade "[A-Z0-9]+" -> "[A-Z0-9]+" \(live\)$'}).Count -ne 2 -or
        @($log|Where-Object{$_ -cmatch '^tune\.save: 1 value\(s\) written to '}).Count -ne 1 -or
        @($log|Where-Object{$_ -cmatch 'opaque ink/paper/text/accent and visible shade required\.'}).Count -ne 1) { throw 'Closing shade actual query/edit/reset/Save/reader refusal transcript differs' }
    $script:closingShadeObserved.developer_commands=$tune; $script:closingShadeObserved.readbacks=93
    $script:closingShadeObserved.csv_rows=39; $script:closingShadeObserved.calibration_pixel_checks=2976
} elseif (@(Get-ChildItem -LiteralPath $captures -Filter 'closing-shade-tune-*').Count) { throw 'Release closing shade must contain no Debug artifacts' }
$actualPpms=@(Get-ChildItem -LiteralPath $captures -Filter 'closing-shade-colour-*.ppm' -File)
if ($actualPpms.Count -ne 12 -or (($actualPpms.Name|Sort-Object)-join'|') -cne (($csCropNames|Sort-Object)-join'|')) { throw 'Closing shade exact native PPM inventory differs' }
$header=[Text.Encoding]::ASCII.GetBytes("P6`n202 100`n255`n")
$csPpms=@(foreach ($name in $csCropNames) {
    $path=Join-Path $captures $name; $bytes=[IO.File]::ReadAllBytes($path)
    if ($bytes.Length -ne 60615 -or $header.Length -ne 15) { throw 'Closing shade canonical P6 length differs' }
    for ($i=0;$i -lt 15;++$i) { if ($bytes[$i] -ne $header[$i]) { throw 'Closing shade canonical P6 header differs' } }
    $rgb=[byte[]]::new(60600); [Array]::Copy($bytes,15,$rgb,0,60600); $csInputs.Add($path)
    [ordered]@{name=$name;width=202;height=100;pixels=20200;header_bytes=15;bytes=60615;sha256=(Get-CSha $bytes);rgb_payload_sha256=(Get-CSha $rgb)}
})
if (@(Get-Content -LiteralPath (Join-Path $run 'run.log')|Where-Object{$_ -match 'Saving\.\.\.$' -or $_ -ceq 'Save was successful.'}).Count -or
    @(Get-Content -LiteralPath (Join-Path $run 'run.err')|Where-Object{$_ -match '^save failure:'}).Count) { throw 'Closing shade leaf called or failed a game Save' }
$script:closingShadeVerified=[ordered]@{directory=[IO.Path]::GetFullPath($captures);ppms=$csPpms;inputs=@(foreach($path in $csInputs){[ordered]@{path=$path;sha256=(Get-FileHash -LiteralPath $path).Hash}})}
$result.observed_counts=$script:closingShadeObserved
function Complete-HudClosingShadeEvidence {
    param([Parameter(Mandatory)][string]$Directory)
    $context=$script:closingShadeVerified
    if ($null -eq $context -or ![StringComparer]::OrdinalIgnoreCase.Equals([IO.Path]::GetFullPath($Directory),$context.directory)) { throw 'Closing shade PNG completion directory differs' }
    foreach ($inputFile in $context.inputs) {
        if (!(Test-Path -LiteralPath $inputFile.path -PathType Leaf) -or (Get-FileHash -LiteralPath $inputFile.path).Hash -cne $inputFile.sha256) { throw 'Closing shade verified input changed before PNG completion' }
    }
    $pngs=@(Get-ChildItem -LiteralPath $Directory -Filter 'closing-shade-colour-*.png' -File)
    $expected=@($context.ppms|ForEach-Object{[IO.Path]::ChangeExtension($_.name,'.png')})
    if ($pngs.Count -ne 12 -or (($pngs.Name|Sort-Object)-join'|') -cne (($expected|Sort-Object)-join'|')) { throw 'Closing shade exact native PNG inventory differs' }
    Add-Type -AssemblyName System.Drawing
    $evidence=@(foreach ($ppm in $context.ppms) {
        $ppmPath=Join-Path $Directory $ppm.name; $ppmBytes=[IO.File]::ReadAllBytes($ppmPath)
        if ((Get-CSha $ppmBytes) -cne $ppm.sha256) { throw 'Closing shade PPM changed before PNG decode' }
        $path=[IO.Path]::ChangeExtension($ppmPath,'.png'); $bytes=[IO.File]::ReadAllBytes($path); $signature=@(137,80,78,71,13,10,26,10)
        if ($bytes.Length -lt 33) { throw 'Closing shade PNG header truncated' }
        for ($i=0;$i -lt 8;++$i) { if ($bytes[$i] -ne $signature[$i]) { throw 'Closing shade PNG signature differs' } }
        if (($bytes[8..11]-join',') -cne '0,0,0,13' -or [Text.Encoding]::ASCII.GetString($bytes,12,4) -cne 'IHDR') { throw 'Closing shade PNG initial IHDR differs' }
        $width=0L; $height=0L
        for ($i=0;$i -lt 4;++$i) { $width=$width*256+$bytes[16+$i]; $height=$height*256+$bytes[20+$i] }
        if ($width -ne 202 -or $height -ne 100 -or $bytes[24] -ne 8 -or $bytes[25] -notin @(2,6) -or $bytes[26] -ne 0 -or $bytes[27] -ne 0 -or $bytes[28] -notin @(0,1)) { throw 'Closing shade native RGB/RGBA PNG IHDR differs' }
        $bitmap=[Drawing.Bitmap]::new($path)
        try {
            if ($bitmap.Width -ne 202 -or $bitmap.Height -ne 100) { throw 'Closing shade decoded PNG dimensions differ' }
            $rgb=[byte[]]::new(60600)
            for ($y=0;$y -lt 100;++$y) { for ($x=0;$x -lt 202;++$x) {
                $pixel=$bitmap.GetPixel($x,$y); $at=($y*202+$x)*3
                if ($pixel.A -ne 255) { throw 'Closing shade decoded PNG alpha is not opaque' }
                $rgb[$at]=$pixel.R; $rgb[$at+1]=$pixel.G; $rgb[$at+2]=$pixel.B
            } }
        } finally { $bitmap.Dispose() }
        for ($i=0;$i -lt 60600;++$i) { if ($rgb[$i] -ne $ppmBytes[15+$i]) { throw 'Closing shade decoded PNG differs from full retained PPM RGB' } }
        $digest=Get-CSha $rgb
        if ($digest -cne $ppm.rgb_payload_sha256) { throw 'Closing shade decoded PNG RGB fingerprint differs' }
        $metadata=[ordered]@{name=[IO.Path]::GetFileName($path);source_ppm=$ppm.name;scope='closing-shade-clock-envelope-context';width=202;height=100;pixels=20200;bit_depth=[int]$bytes[24];colour_type=[int]$bytes[25];ppm_sha256=$ppm.sha256;png_sha256=(Get-CSha $bytes);decoded_rgb_sha256=$digest;decoded_rgb_matches_ppm=$true;alpha_opaque=$true}
        $metadata|ConvertTo-Json -Depth 5|Set-Content -LiteralPath "$path.metadata.json" -Encoding utf8
        $metadata
    })
    $script:closingShadeObserved.png_crops=$evidence; $script:closingShadeObserved.png_count=12
    $script:closingShadeObserved.png_pixels=242400; $script:closingShadeObserved.png_rgb_bytes=727200; $script:closingShadeObserved.png_complete=$true
}