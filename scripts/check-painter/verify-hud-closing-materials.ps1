# Standalone isolated leaf: exact schemas before export, decoded PNG bytes after.
# Uses the official caller's $captures/$run/$debug/$gameDirectory/$result only.
$cmUpdates = @('local_updates','player_updates','level_updates','scene_updates','state_updates','input_updates','time_updates','input_polls','observation_weapon_updates')
function Get-CMSha([byte[]]$Bytes) { [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($Bytes)) }
function Assert-CMKeys($Object, [string[]]$Expected, [string]$Scope) {
    if ($Object -isnot [pscustomobject] -or (($Object.PSObject.Properties.Name | Sort-Object) -join '|') -cne (($Expected | Sort-Object) -join '|')) { throw "Closing materials exact $Scope schema differs" }
}
function Assert-CMInt($Object, [string]$Key, [long]$Expected) {
    $value = $Object.$Key
    if (($value -isnot [int] -and $value -isnot [long]) -or $value -ne $Expected) { throw "Closing materials exact integer differs: $Key" }
}
function Get-CMCsvInt($Row, [string]$Key) {
    $text = [string]$Row.$Key; $value = 0L
    if ($text -cnotmatch '^(0|[1-9][0-9]*)$' -or ![long]::TryParse($text, [ref]$value)) { throw "Closing materials canonical CSV integer differs: $Key" }
    return $value
}
function Assert-CMColumns($Rows, [string[]]$Expected, [int]$Count, [string]$Scope) {
    if ($Rows.Count -ne $Count -or ($Rows[0].PSObject.Properties.Name -join '|') -cne ($Expected -join '|')) { throw "Closing materials exact $Scope CSV shape differs" }
}
function Assert-CMFlags($Row, [string[]]$Flags) {
    foreach ($flag in $Flags) { if ($Row.$flag -cne 'true') { throw "Closing materials native flag differs: $flag" } }
}
function Replace-CMClosingToken([string]$Text, [string]$Key, [string]$Token) {
    $blocks = [regex]::Matches($Text, '"closing_clock"\s*:\s*\{[^{}]*\}')
    if ($blocks.Count -ne 1) { throw 'Closing material independent Save check requires one flat closing-clock object' }
    $block = $blocks[0]; $leaves = [regex]::Matches($block.Value, '"' + [regex]::Escape($Key) + '"\s*:\s*("[^"\\]*")')
    if ($leaves.Count -ne 1) { throw 'Closing material independent Save token is ambiguous' }
    $leaf = $leaves[0].Groups[1]; $at = $block.Index + $leaf.Index
    return $Text.Substring(0, $at) + '"' + $Token + '"' + $Text.Substring($at + $leaf.Length)
}
$cmInputs = [Collections.Generic.List[string]]::new()
$cmSummaryPath = Join-Path $captures 'hud-closing-materials-consumer-summary.json'
$cmMainPath = Join-Path $captures 'hud-closing-materials-main.csv'
$cmBranchPath = Join-Path $captures 'hud-closing-materials-branches.csv'
$cmControlPath = Join-Path $captures 'hud-closing-materials-controls.csv'
foreach ($path in @($cmSummaryPath, $cmMainPath, $cmBranchPath, $cmControlPath, (Join-Path $run 'run.log'), (Join-Path $run 'run.err'))) {
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Closing material required evidence missing' }; $cmInputs.Add($path)
}
if (@(Get-ChildItem -LiteralPath $captures -Filter 'hud-closing-materials-failure-*').Count) { throw 'Closing material source-hypothesis failure artifacts remain' }
$cmSummary = Get-Content -LiteralPath $cmSummaryPath -Raw | ConvertFrom-Json
$cmCounts = [ordered]@{readbacks=64;csv_rows=27;main_rows=5;branch_rows=16;control_rows=6;crops=9;crop_width=202;crop_height=100;crop_pixels=181800;ppm_bytes=545535;patches=12;patch_pixels=264;independent_observations=11;patch_pixel_checks=2904;repeat_checks=21;copied_adoption_checks=5;normal_invariance_checks=5;accent_invariance_checks=2;settled_reduced_checks=3;beat_counterexamples=1;controls=6;patch_control_discriminations=24;calibration_pixel_checks=2048;private_game_saves=0}
Assert-CMKeys $cmSummary (@($cmCounts.Keys) + $cmUpdates + @('scope','control_discriminating_pixels')) 'shared'
foreach ($key in $cmCounts.Keys) { Assert-CMInt $cmSummary $key $cmCounts[$key] }
foreach ($key in $cmUpdates) { Assert-CMInt $cmSummary $key 0 }
$cmControlPixels = $cmSummary.control_discriminating_pixels
if (($cmControlPixels -isnot [int] -and $cmControlPixels -isnot [long]) -or $cmControlPixels -le 0 -or $cmControlPixels -gt 528 -or $cmSummary.scope -cne 'opaque-closing-material-patches-and-applicability') { throw 'Closing material actual control support or scope differs' }
$cmMain = @(Import-Csv -LiteralPath $cmMainPath); $cmBranches = @(Import-Csv -LiteralPath $cmBranchPath); $cmControls = @(Import-Csv -LiteralPath $cmControlPath)
Assert-CMColumns $cmMain @('state','raw_rgba_match','adoption_match','repeat_match','locality_match','opaque_selected','frozen_style_resources_rng','scope') 5 'main'
$cmStates = @('default','ink','paper','accent','all')
for ($i=0; $i -lt 5; ++$i) {
    $row = $cmMain[$i]; Assert-CMFlags $row @('raw_rgba_match','adoption_match','repeat_match','locality_match','opaque_selected','frozen_style_resources_rng')
    if ($row.state -cne $cmStates[$i] -or $row.scope -cne 'opaque-closing-material-patches') { throw 'Closing material tuple order or main scope differs' }
}
Assert-CMColumns $cmBranches @('state','context','raw_rgba_match','repeat_match','selection_guard','opaque_selected','frozen_style_resources_rng','scope') 16 'branch'
$cmNormalContexts = @('normal','above-ten','zero','stopped-short','reduced-normal')
for ($i=0; $i -lt 16; ++$i) {
    $row = $cmBranches[$i]; Assert-CMFlags $row @('repeat_match','opaque_selected','frozen_style_resources_rng')
    if ($i -lt 10) {
        $state = if ($i % 2 -eq 0) {'default'} else {'all'}; $context = $cmNormalContexts[[int][math]::Floor($i/2)]
        $raw = 'not-run-normal-invariance-only'; $guard = if ($state -ceq 'default') {'baseline'} else {'normal-exact'}
    } else {
        $at = $i - 10; $state = @('default','all','accent')[$at % 3]
        $context = if ($at -lt 3) {'fractional-closing'} else {'reduced-closing'}
        $raw = 'true'; $guard = switch ($state) { default {'baseline'} all {'body-locality'} accent {'accent-exact'} }
    }
    if ($row.state -cne $state -or $row.context -cne $context -or $row.raw_rgba_match -cne $raw -or $row.selection_guard -cne $guard -or $row.scope -cne 'closing-material-applicability') { throw 'Closing material exact normal/settled/reduced applicability differs' }
}
Assert-CMColumns $cmControls @('material','control','patch_discriminations','discriminating_pixels','exterior_unchanged','scope') 6 'control'
$cmSumPixels = 0L; $cmSumDiscriminations = 0L
for ($i=0; $i -lt 6; ++$i) {
    $row = $cmControls[$i]; $material = @('ink','paper','accent')[[int][math]::Floor($i/2)]
    $mode = if ($i % 2 -eq 0) {'wrong-opaque'} else {'omitted'}
    $sites = switch ($material) { ink {6} paper {2} accent {4} }; $budget = switch ($material) { ink {104} paper {112} accent {48} }
    $pixels = Get-CMCsvInt $row 'discriminating_pixels'; $sitesObserved = Get-CMCsvInt $row 'patch_discriminations'
    if ($row.material -cne $material -or $row.control -cne $mode -or $sitesObserved -ne $sites -or $pixels -le 0 -or $pixels -gt $budget -or $row.exterior_unchanged -cne 'true' -or $row.scope -cne 'opaque-material-counterexample') { throw 'Closing wrong/omitted material per-site discrimination differs' }
    $cmSumPixels += $pixels; $cmSumDiscriminations += $sitesObserved
}
if ($cmSumPixels -ne $cmControlPixels -or $cmSumDiscriminations -ne 24) { throw 'Closing material observed negative controls do not reconcile' }
$cmCropNames = @($cmStates | ForEach-Object { "hud-closing-materials-$_.ppm" })
$cmCropNames += @('fractional-closing','reduced-closing') | ForEach-Object { $context = $_; @('all','accent') | ForEach-Object { "hud-closing-materials-$context-$_.ppm" } }
$script:closingMaterialsObserved = [ordered]@{consumer=$cmSummary;readbacks=64;csv_rows=27;crops=9;crop_pixels=181800;ppm_bytes=545535;controls=6;control_discriminating_pixels=$cmControlPixels;calibration_pixel_checks=2048;private_game_saves=0;opacity_policy='alpha exactly 1; equal opaque siblings valid';reference_scope='12 literal opaque fill patches only; copied full HUD adoption and full-frame applicability controls are separate';png_count=0;png_crops=@();png_complete=$false}
if ($debug) {
    $tunePath = Join-Path $captures 'hud-closing-materials-tune-summary.json'; $tuneCsv = Join-Path $captures 'hud-closing-materials-tune.csv'
    $refusalPath = Join-Path $captures 'hud-closing-materials-tune-refusals.json'; $transcript = Join-Path $captures 'hud-closing-materials-tune-commands.log'
    foreach ($path in @($tunePath,$tuneCsv,$refusalPath,$transcript)) {
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Closing material required Debug evidence missing' }; $cmInputs.Add($path)
    }
    $tune = Get-Content -LiteralPath $tunePath -Raw | ConvertFrom-Json
    $tuneCounts = [ordered]@{commands=17;accepted_commands=14;queries=3;refused_commands=3;states=10;readbacks=40;csv_rows=10;refusal_json_records=3;refusal_raster_reads=0;crops=4;crop_pixels=80800;ppm_bytes=242460;saved_tokens=2;presentation_save_commands=1;patch_pixel_checks=2640;repeat_checks=10;copied_adoption_checks=10;sibling_checks=10;saved_frame_equalities=3;calibration_pixel_checks=1280;private_game_saves=0}
    Assert-CMKeys $tune (@($tuneCounts.Keys) + $cmUpdates + @('scope')) 'Debug'
    foreach ($key in $tuneCounts.Keys) { Assert-CMInt $tune $key $tuneCounts[$key] }
    foreach ($key in $cmUpdates) { Assert-CMInt $tune $key 0 }
    if ($tune.scope -cne 'debug-live-opaque-closing-materials') { throw 'Closing material Debug scope differs' }
    $acceptedRows = @(Import-Csv -LiteralPath $tuneCsv)
    Assert-CMColumns $acceptedRows @('state','ink_token','paper_token','accent_token','raw_rgba_match','adoption_match','repeat_match','siblings_preserved','private_bytes','opaque_selected','frozen_style_resources_rng','scope') 10 'Debug'
    $states = @('baseline','ink-edit','paired-pending','paired-saved','ink-accent-overwrite','reset-ink','reset-accent','paper-accent-overwrite','reset-paper','final-saved')
    $inks = @('141414','RED','RED','RED','2060C0FF','RED','RED','RED','RED','RED')
    $papers = @('FFF5DF','FFF5DF','35BCE1FF','35BCE1FF','35BCE1FF','35BCE1FF','35BCE1FF','A532D6FF','35BCE1FF','35BCE1FF')
    $accents = @('FFD34D','FFD34D','FFD34D','FFD34D','E17C35','E17C35','FFD34D','35BCE1FF','35BCE1FF','FFD34D')
    for ($i=0; $i -lt 10; ++$i) {
        $row = $acceptedRows[$i]; Assert-CMFlags $row @('raw_rgba_match','adoption_match','repeat_match','siblings_preserved','private_bytes','opaque_selected','frozen_style_resources_rng')
        if ($row.state -cne $states[$i] -or $row.ink_token -cne $inks[$i] -or $row.paper_token -cne $papers[$i] -or $row.accent_token -cne $accents[$i] -or $row.scope -cne 'debug-live-opaque-closing-materials') { throw 'Closing material Debug exact state/sibling token differs' }
    }
    $refusal = Get-Content -LiteralPath $refusalPath -Raw | ConvertFrom-Json
    Assert-CMKeys $refusal @('scope','records') 'refusal envelope'
    if ($refusal.scope -cne 'atomic-opaque-reader-refusals-no-raster-read' -or $refusal.records.Count -ne 3) { throw 'Closing material refusal envelope differs' }
    for ($i=0; $i -lt 3; ++$i) {
        $record = $refusal.records[$i]
        Assert-CMKeys $record @('path','input','accepted','atomic_preserved','opaque_selected','documents_preserved','bytes_preserved','resources_rng_preserved','raster_checked') 'refusal record'
        foreach ($key in @('atomic_preserved','opaque_selected','documents_preserved','bytes_preserved','resources_rng_preserved')) { if ($record.$key -isnot [bool] -or !$record.$key) { throw 'Closing material atomic refusal flag differs' } }
        foreach ($key in @('accepted','raster_checked')) { if ($record.$key -isnot [bool] -or $record.$key) { throw 'Closing material refusal fabricated raster or acceptance evidence' } }
        if ($record.path -cne ('presentation.hud.closing_clock.' + @('ink','paper','accent')[$i]) -or $record.input -cne '2060C080') { throw 'Closing material refusal exact key/input differs' }
    }
    $originalPresentation = Join-Path $gameDirectory 'presentation.json'; $originalTuning = Join-Path $gameDirectory 'tuning.json'
    foreach ($path in @($originalPresentation,$originalTuning)) { if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Closing material original staged source missing' }; $cmInputs.Add($path) }
    $sourceText = [IO.File]::ReadAllText($originalPresentation)
    $savedText = Replace-CMClosingToken (Replace-CMClosingToken $sourceText 'ink' 'RED') 'paper' '35BCE1FF'
    $expectedPresentation = [Text.Encoding]::UTF8.GetBytes($savedText); $originalTuningBytes = [IO.File]::ReadAllBytes($originalTuning)
    foreach ($directory in @('source','staged')) {
        foreach ($name in @('presentation.json','tuning.json')) {
            $path = Join-Path $captures "hud-closing-materials-tune-scratch/$directory/$name"
            if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Closing material private saved file absent' }
            $expected = if ($name -ceq 'presentation.json') {$expectedPresentation} else {$originalTuningBytes}
            if ((Get-CMSha ([IO.File]::ReadAllBytes($path))) -cne (Get-CMSha $expected)) { throw 'Closing material private saved bytes differ from two exact independent tokens' }; $cmInputs.Add($path)
        }
    }
    if (Test-Path -LiteralPath (Join-Path $captures 'hud-closing-materials-tune-scratch/profile/ColourWars/save_data.json')) { throw 'Closing material fixture wrote a private game Save' }
    $log = @(Get-Content -LiteralPath $transcript)
    foreach ($key in @('ink','paper','accent')) {
        if (@($log | Where-Object { $_ -cmatch ('^presentation\.hud\.closing_clock\.' + $key + ' = "[A-Z0-9]+"  \[live\]$') }).Count -ne 1) { throw 'Closing material actual value-less live query transcript differs' }
    }
    if (@($log | Where-Object { $_ -cmatch '^tune: presentation\.hud\.closing_clock\.(ink|paper|accent) ' }).Count -ne 6 -or
        @($log | Where-Object { $_ -cmatch '^tune\.reset: presentation\.hud\.closing_clock\.(ink|paper|accent) ' }).Count -ne 4 -or
        @($log | Where-Object { $_ -cmatch '^tune\.save: 2 value\(s\) written to ' }).Count -ne 1 -or
        @($log | Where-Object { $_ -cmatch 'opaque ink/paper/text/accent' }).Count -ne 3) { throw 'Closing material real edit/reset/Save/refusal transcript counts differ' }
    $cmCropNames += @('baseline','paired-pending','ink-accent-overwrite','final-saved') | ForEach-Object { "hud-closing-materials-tune-$_.ppm" }
    $script:closingMaterialsObserved.developer_commands = $tune; $script:closingMaterialsObserved.readbacks = 104; $script:closingMaterialsObserved.csv_rows = 37
    $script:closingMaterialsObserved.crops = 13; $script:closingMaterialsObserved.crop_pixels = 262600; $script:closingMaterialsObserved.ppm_bytes = 787995; $script:closingMaterialsObserved.calibration_pixel_checks = 3328
} elseif (@(Get-ChildItem -LiteralPath $captures -Filter 'hud-closing-materials-tune-*').Count) { throw 'Release closing material leaf must contain no Debug artifacts' }
$actualPpms = @(Get-ChildItem -LiteralPath $captures -Filter 'hud-closing-materials-*.ppm' -File)
if ($actualPpms.Count -ne $cmCropNames.Count -or (($actualPpms.Name | Sort-Object) -join '|') -cne (($cmCropNames | Sort-Object) -join '|')) { throw 'Closing material exact native PPM inventory differs' }
$header = [Text.Encoding]::ASCII.GetBytes("P6`n202 100`n255`n")
$cmPpms = @(foreach ($name in $cmCropNames) {
    $path = Join-Path $captures $name; $bytes = [IO.File]::ReadAllBytes($path)
    if ($bytes.Length -ne 60615 -or $header.Length -ne 15) { throw 'Closing material canonical P6 length differs' }
    for ($i=0; $i -lt 15; ++$i) { if ($bytes[$i] -ne $header[$i]) { throw 'Closing material canonical P6 header differs' } }
    $rgb = [byte[]]::new(60600); [Array]::Copy($bytes,15,$rgb,0,60600); $cmInputs.Add($path)
    [ordered]@{name=$name;width=202;height=100;pixels=20200;header_bytes=15;bytes=60615;sha256=(Get-CMSha $bytes);rgb_payload_sha256=(Get-CMSha $rgb)}
})
if (@(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -match 'Saving\.\.\.$' -or $_ -ceq 'Save was successful.' }).Count -or
    @(Get-Content -LiteralPath (Join-Path $run 'run.err') | Where-Object { $_ -match '^save failure:' }).Count) { throw 'Closing material leaf unexpectedly called or failed a game Save' }
$script:closingMaterialsVerified = [ordered]@{directory=[IO.Path]::GetFullPath($captures);ppms=$cmPpms;inputs=@(foreach ($path in $cmInputs) {[ordered]@{path=$path;sha256=(Get-FileHash -LiteralPath $path).Hash}})}
$result.observed_counts = $script:closingMaterialsObserved
function Complete-HudClosingMaterialsEvidence {
    param([Parameter(Mandatory)][string]$Directory)
    $context = $script:closingMaterialsVerified
    if ($null -eq $context -or ![StringComparer]::OrdinalIgnoreCase.Equals([IO.Path]::GetFullPath($Directory),$context.directory)) { throw 'Closing material PNG completion directory differs' }
    foreach ($inputFile in $context.inputs) {
        if (!(Test-Path -LiteralPath $inputFile.path -PathType Leaf) -or (Get-FileHash -LiteralPath $inputFile.path).Hash -cne $inputFile.sha256) { throw 'Closing material verified input changed before PNG completion' }
    }
    $pngs = @(Get-ChildItem -LiteralPath $Directory -Filter 'hud-closing-materials-*.png' -File)
    $expected = @($context.ppms | ForEach-Object { [IO.Path]::ChangeExtension($_.name,'.png') })
    if ($pngs.Count -ne $expected.Count -or (($pngs.Name | Sort-Object) -join '|') -cne (($expected | Sort-Object) -join '|')) { throw 'Closing material exact native PNG inventory differs' }
    Add-Type -AssemblyName System.Drawing
    $evidence = @(foreach ($ppm in $context.ppms) {
        $ppmPath = Join-Path $Directory $ppm.name; $ppmBytes = [IO.File]::ReadAllBytes($ppmPath)
        if ((Get-CMSha $ppmBytes) -cne $ppm.sha256) { throw 'Closing material PPM changed before PNG decode' }
        $path = [IO.Path]::ChangeExtension($ppmPath,'.png'); $bytes = [IO.File]::ReadAllBytes($path); $signature = @(137,80,78,71,13,10,26,10)
        if ($bytes.Length -lt 33) { throw 'Closing material PNG header truncated' }
        for ($i=0; $i -lt 8; ++$i) { if ($bytes[$i] -ne $signature[$i]) { throw 'Closing material PNG signature differs' } }
        if (($bytes[8..11] -join ',') -cne '0,0,0,13' -or [Text.Encoding]::ASCII.GetString($bytes,12,4) -cne 'IHDR') { throw 'Closing material PNG initial IHDR differs' }
        $width=0L; $height=0L
        for ($i=0; $i -lt 4; ++$i) { $width=$width*256+$bytes[16+$i]; $height=$height*256+$bytes[20+$i] }
        if ($width -ne 202 -or $height -ne 100 -or $bytes[24] -ne 8 -or $bytes[25] -notin @(2,6) -or $bytes[26] -ne 0 -or $bytes[27] -ne 0 -or $bytes[28] -notin @(0,1)) { throw 'Closing material native RGB/RGBA PNG IHDR differs' }
        $bitmap = [Drawing.Bitmap]::new($path)
        try {
            if ($bitmap.Width -ne 202 -or $bitmap.Height -ne 100) { throw 'Closing material decoded PNG dimensions differ' }
            $rgb = [byte[]]::new(60600)
            for ($y=0; $y -lt 100; ++$y) { for ($x=0; $x -lt 202; ++$x) {
                $pixel=$bitmap.GetPixel($x,$y); $at=($y*202+$x)*3
                if ($pixel.A -ne 255) { throw 'Closing material decoded PNG alpha is not opaque' }
                $rgb[$at]=$pixel.R; $rgb[$at+1]=$pixel.G; $rgb[$at+2]=$pixel.B
            } }
        } finally { $bitmap.Dispose() }
        for ($i=0; $i -lt 60600; ++$i) { if ($rgb[$i] -ne $ppmBytes[15+$i]) { throw 'Closing material decoded PNG differs from complete PPM RGB' } }
        $digest = Get-CMSha $rgb
        if ($digest -cne $ppm.rgb_payload_sha256) { throw 'Closing material decoded PNG RGB fingerprint differs' }
        $metadata=[ordered]@{name=[IO.Path]::GetFileName($path);source_ppm=$ppm.name;scope='closing-material-clock-envelope-only';width=202;height=100;pixels=20200;bit_depth=[int]$bytes[24];colour_type=[int]$bytes[25];ppm_sha256=$ppm.sha256;png_sha256=(Get-CMSha $bytes);decoded_rgb_sha256=$digest;decoded_rgb_matches_ppm=$true;alpha_opaque=$true}
        $metadata | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath "$path.metadata.json" -Encoding utf8
        $metadata
    })
    $script:closingMaterialsObserved.png_crops=$evidence; $script:closingMaterialsObserved.png_count=$evidence.Count
    $script:closingMaterialsObserved.png_pixels=$evidence.Count*20200; $script:closingMaterialsObserved.png_rgb_bytes=$evidence.Count*60600
    $script:closingMaterialsObserved.png_complete=$true
}