# Dot-sourced by the existing Limb verifier. Old43/44 files and budgets are
# checked unchanged first; this separate scenario adds exact path45 evidence.
function Complete-QuietFaceColourEvidence {
    param([Parameter(Mandatory)][string]$Directory)
    $context = $script:quietFaceVerifiedContext
    if ($null -eq $context -or ![StringComparer]::OrdinalIgnoreCase.Equals([IO.Path]::GetFullPath($Directory), $context.directory)) { throw 'Quiet face PNG completion directory differs' }
    foreach ($inputFile in $context.inputs) {
        if (!(Test-Path -LiteralPath $inputFile.path -PathType Leaf) -or (Get-FileHash -LiteralPath $inputFile.path).Hash -cne $inputFile.sha256) { throw 'Quiet face verified input changed before PNG completion' }
    }
    $pngs = @(Get-ChildItem -LiteralPath $Directory -Filter 'quiet-face-colour-*.png' -File)
    $expected = @($context.ppms | ForEach-Object { [IO.Path]::ChangeExtension($_.name, '.png') })
    if ($pngs.Count -ne 10 -or (($pngs.Name | Sort-Object) -join '|') -cne (($expected | Sort-Object) -join '|')) { throw 'Quiet face exact PNG inventory differs' }
    Add-Type -AssemblyName System.Drawing
    $pngEvidence = @(foreach ($ppm in $context.ppms) {
        $ppmPath = Join-Path $Directory $ppm.name
        $ppmBytes = [IO.File]::ReadAllBytes($ppmPath)
        if ((Get-LimbSha $ppmBytes) -cne $ppm.sha256) { throw 'Quiet face PPM changed after initial verification' }
        $path = [IO.Path]::ChangeExtension($ppmPath, '.png'); $bytes = [IO.File]::ReadAllBytes($path)
        $signature = @(137,80,78,71,13,10,26,10)
        if ($bytes.Length -lt 33) { throw 'Quiet face PNG header truncated' }
        for ($i = 0; $i -lt 8; ++$i) { if ($bytes[$i] -ne $signature[$i]) { throw 'Quiet face PNG signature differs' } }
        if (($bytes[8..11] -join ',') -cne '0,0,0,13' -or [Text.Encoding]::ASCII.GetString($bytes,12,4) -cne 'IHDR') { throw 'Quiet face initial IHDR differs' }
        $width = 0L; $height = 0L
        for ($i = 0; $i -lt 4; ++$i) { $width = $width*256+$bytes[16+$i]; $height = $height*256+$bytes[20+$i] }
        if ($width -ne 176 -or $height -ne 24 -or $bytes[24] -ne 8 -or $bytes[25] -notin @(2,6) -or $bytes[26] -ne 0 -or $bytes[27] -ne 0 -or $bytes[28] -notin @(0,1)) { throw 'Quiet face PNG native RGB/RGBA IHDR differs' }
        $bitmap = [Drawing.Bitmap]::new($path)
        try {
            if ($bitmap.Width -ne 176 -or $bitmap.Height -ne 24) { throw 'Quiet face decoded PNG size differs' }
            $rgb = [byte[]]::new(12672)
            for ($y = 0; $y -lt 24; ++$y) { for ($x = 0; $x -lt 176; ++$x) {
                $pixel = $bitmap.GetPixel($x,$y); $at = ($y*176+$x)*3
                if ($pixel.A -ne 255) { throw 'Quiet face decoded PNG alpha differs' }
                $rgb[$at]=$pixel.R; $rgb[$at+1]=$pixel.G; $rgb[$at+2]=$pixel.B
            } }
        } finally { $bitmap.Dispose() }
        for ($i = 0; $i -lt 12672; ++$i) { if ($rgb[$i] -ne $ppmBytes[14+$i]) { throw 'Quiet face decoded PNG differs from complete retained PPM RGB' } }
        $digest = Get-LimbSha $rgb
        if ($digest -cne $ppm.rgb_payload_sha256) { throw 'Quiet face decoded RGB fingerprint differs' }
        $metadata = [ordered]@{name=[IO.Path]::GetFileName($path);source_ppm=$ppm.name;scope='four-literal-blink-eyes-independent';width=176;height=24;pixels=4224;bit_depth=[int]$bytes[24];colour_type=[int]$bytes[25];ppm_sha256=$ppm.sha256;png_sha256=(Get-LimbSha $bytes);decoded_rgb_sha256=$digest;decoded_rgb_matches_ppm=$true;alpha_opaque=$true}
        $metadata | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath "$path.metadata.json" -Encoding utf8
        $metadata
    })
    $observed = $context.observed
    $observed.limb43_44.png_crops = $observed.png_crops
    $observed.limb43_44.png_count = 4
    $observed.quiet_face.png_crops = $pngEvidence
    $observed.quiet_face.png_count = 10
    $observed.quiet_face.png_complete = $true
    $observed.png_crops = @($observed.png_crops) + $pngEvidence
    $observed.png_count = 14; $observed.png_pixels = 2499840; $observed.png_rgb_bytes = 7499520
}

$quietSummaryPath = Join-Path $captures 'quiet-face-colour-consumer-summary.json'
$quietCsvPath = Join-Path $captures 'quiet-face-colour-consumers.csv'
$quietInputFiles = [Collections.Generic.List[string]]::new()
foreach ($path in @($quietSummaryPath,$quietCsvPath)) {
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Quiet face required shared evidence missing' }
    $quietInputFiles.Add($path)
}
$quietSummary = Get-Content -LiteralPath $quietSummaryPath -Raw | ConvertFrom-Json
$quietRows = @(Import-Csv -LiteralPath $quietCsvPath)
$quietCounts = [ordered]@{readbacks=38;csv_rows=10;crops=10;crop_width=176;crop_height=24;crop_pixels=42240;ppm_bytes=126860;states=5;backgrounds=2;active_contexts=2;disabled_contexts=2;null_contexts=2;roi_pixels=128;face_pixels=64;lid_pixels=64;roi_pixel_checks=1280;repeat_checks=10;controls=8;discriminating_pixels=512;calibration_pixel_checks=1216;private_game_saves=0}
Assert-LimbKeys $quietSummary (@($quietCounts.Keys)+$limbUpdates+@('scope')) 'quiet face shared'
foreach ($key in $quietCounts.Keys) { Assert-LimbInteger $quietSummary $key $quietCounts[$key] }
Assert-LimbUpdates $quietSummary
if ($quietSummary.scope -cne 'direct-consumer-only') { throw 'Quiet face shared scope differs' }
$quietColumns = @('state','background','raw_rgba_match','repeat_match','disabled_null_blank','frozen_geometry_resources_rng','controls','discriminating_pixels','scope')
if ($quietRows.Count -ne 10 -or ($quietRows[0].PSObject.Properties.Name -join '|') -cne ($quietColumns -join '|')) { throw 'Quiet face exact shared CSV shape differs' }
$quietFlags = @('raw_rgba_match','repeat_match','disabled_null_blank','frozen_geometry_resources_rng')
$quietNames = @('default','rgb','rgba128','alpha0','joint'); $quietBackgrounds = @('black','blue')
$quietExpected = @(foreach ($state in $quietNames) { foreach ($background in $quietBackgrounds) { "$state|$background" } })
if ((@($quietRows | ForEach-Object { "$($_.state)|$($_.background)" }) -join ';') -cne ($quietExpected -join ';')) { throw 'Quiet face exact state/background order differs' }
foreach ($row in $quietRows) {
    foreach ($flag in $quietFlags) { if ($row.$flag -cne 'true') { throw 'Quiet face raw/blank/frozen flag differs' } }
    $controls = switch ($row.state) { rgba128 {2} alpha0 {1} joint {1} default {0} }
    if ((Get-LimbCsvCount $row 'controls') -ne $controls -or (Get-LimbCsvCount $row 'discriminating_pixels') -ne $controls*64 -or $row.scope -cne 'direct-consumer-only') { throw 'Quiet face exact role discrimination differs' }
}
$quietPpms = @(Get-ChildItem -LiteralPath $captures -Filter 'quiet-face-colour-*.ppm' -File)
$quietExpectedNames = @($quietExpected | ForEach-Object { 'quiet-face-colour-'+$_.Replace('|','-')+'.ppm' })
if ($quietPpms.Count -ne 10 -or (($quietPpms.Name|Sort-Object)-join '|') -cne (($quietExpectedNames|Sort-Object)-join '|')) { throw 'Quiet face exact PPM inventory differs' }
$quietHeader = [Text.Encoding]::ASCII.GetBytes("P6`n176 24`n255`n")
$quietPpmEvidence = @(foreach ($name in $quietExpectedNames) {
    $path = Join-Path $captures $name; $bytes = [IO.File]::ReadAllBytes($path)
    if ($bytes.Length -ne 12686) { throw 'Quiet face canonical native P6 length differs' }
    for ($i=0; $i -lt 14; ++$i) { if ($bytes[$i] -ne $quietHeader[$i]) { throw 'Quiet face canonical P6 header differs' } }
    $rgb = [byte[]]::new(12672); [Array]::Copy($bytes,14,$rgb,0,12672); $quietInputFiles.Add($path)
    [ordered]@{name=$name;bytes=12686;sha256=(Get-LimbSha $bytes);rgb_payload_sha256=(Get-LimbSha $rgb)}
})
$quietObserved = [ordered]@{consumer=$quietSummary;readbacks=38;csv_rows=10;crops=10;controls=8;discriminating_pixels=512;calibration_pixel_checks=1216;private_game_saves=0;png_count=0;png_crops=@();png_complete=$false}
if ($debug) {
    $quietTunePath=Join-Path $captures 'quiet-face-tune-summary.json'; $quietTuneCsv=Join-Path $captures 'quiet-face-tune.csv'; $quietTranscript=Join-Path $captures 'quiet-face-tune-commands.log'
    foreach ($path in @($quietTunePath,$quietTuneCsv,$quietTranscript)) {
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Quiet face required Debug evidence missing' }
        $quietInputFiles.Add($path)
    }
    $quietTune = Get-Content -LiteralPath $quietTunePath -Raw | ConvertFrom-Json
    $quietTuneRows = @(Import-Csv -LiteralPath $quietTuneCsv)
    $quietTuneCounts = [ordered]@{commands=9;states=9;backgrounds=2;readbacks=54;csv_rows=18;crops=0;saved_tokens=2;named_face_checks=1;roi_pixel_checks=2304;repeat_checks=18;calibration_pixel_checks=1728;private_game_saves=0}
    Assert-LimbKeys $quietTune (@($quietTuneCounts.Keys)+$limbUpdates+@('scope','adoption')) 'quiet face Debug'
    foreach ($key in $quietTuneCounts.Keys) { Assert-LimbInteger $quietTune $key $quietTuneCounts[$key] }
    Assert-LimbUpdates $quietTune
    if ($quietTune.scope -cne 'debug-validated-commands' -or $quietTune.adoption -cne 'live') { throw 'Quiet face Debug scope differs' }
    $quietStates=@('baseline','named-face','reset-original','partial-arm','partial-both','zero-face','zero-both','reset-face-saved','reset-arm-saved')
    $quietFaceTokens=@('WHITE','RED','WHITE','WHITE','C0406080','A532D600','A532D600','C0406080','C0406080')
    $quietArmTokens=@('BLACK','BLACK','BLACK','2060C080','2060C080','2060C080','D65A3200','D65A3200','2060C080')
    $quietTuneColumns=@('state','background','raw_rgba_match','repeat_match','disabled_null_blank','frozen_geometry_resources_rng','private_bytes','adoption','face_token','arm_token')
    if ($quietTuneRows.Count -ne 18 -or ($quietTuneRows[0].PSObject.Properties.Name -join '|') -cne ($quietTuneColumns -join '|')) { throw 'Quiet face exact Debug CSV shape differs' }
    for ($index=0; $index -lt 18; ++$index) {
        $row=$quietTuneRows[$index]; $stateIndex=[int][Math]::Floor($index/2); $background=$quietBackgrounds[$index%2]
        foreach ($flag in $quietFlags) { if ($row.$flag -cne 'true') { throw 'Quiet face Debug pixel/frozen flag differs' } }
        if ($row.state -cne $quietStates[$stateIndex] -or $row.background -cne $background -or $row.private_bytes -cne 'true' -or $row.adoption -cne 'live' -or $row.face_token -cne $quietFaceTokens[$stateIndex] -or $row.arm_token -cne $quietArmTokens[$stateIndex]) { throw 'Quiet face exact Debug state/token/private-byte observation differs' }
    }
    if ((Get-Item -LiteralPath $quietTranscript).Length -le 0 -or (Test-Path -LiteralPath (Join-Path $captures 'quiet-face-tune-scratch/profile/ColourWars/save_data.json'))) { throw 'Quiet face Debug transcript/game-Save invariant differs' }
    $quietObserved.developer_commands=$quietTune; $quietObserved.readbacks=92; $quietObserved.csv_rows=28; $quietObserved.calibration_pixel_checks=2944
} elseif (@(Get-ChildItem -LiteralPath $captures -Filter 'quiet-face-tune-*').Count) { throw 'Release quiet face scope must have no Debug artifacts' }
$limbObserved.limb43_44=[ordered]@{readbacks=$limbObserved.readbacks;csv_rows=$limbObserved.csv_rows;crops=4;controls=6;control_witness_checks=252;control_discriminating_witness_checks=248;control_saturated_white_checks=4;calibration_pixel_checks=$limbObserved.calibration_pixel_checks;png_count=0;png_crops=@()}
$limbObserved.quiet_face=$quietObserved
$limbObserved.readbacks += $quietObserved.readbacks; $limbObserved.csv_rows += $quietObserved.csv_rows
$limbObserved.crops=14; $limbObserved.controls=14; $limbObserved.crop_pixels=2499840; $limbObserved.ppm_bytes=7499724
$limbObserved.calibration_pixel_checks += $quietObserved.calibration_pixel_checks
$limbObserved.control_witness_scope='limb43/44-only; quiet-face pixel inequalities are separately counted'
$limbObserved.reference_scope='separately counted limb43/44 procedural ROIs and quiet-face45 literal ordered rectangles'
$script:quietFaceVerifiedContext=[ordered]@{directory=[IO.Path]::GetFullPath($captures);observed=$limbObserved;ppms=$quietPpmEvidence;inputs=@(foreach($path in $quietInputFiles){[ordered]@{path=$path;sha256=(Get-FileHash -LiteralPath $path).Hash}})}
foreach ($path in $quietInputFiles) { $limbInputs.Add($path) }
