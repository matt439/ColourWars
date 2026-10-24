# Exact native schemas precede contact export; decoded PNG parity follows it.
# Source-derived regions retain the full multiline identity paper lower strip.
$labelUpdates=@('local_updates','player_updates','level_updates','scene_updates','state_updates','input_updates','time_updates','input_polls','observation_weapon_updates')
$labelStates=@('default','rgb','partial','zero')
function Get-LabelSha([byte[]]$Bytes) { [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($Bytes)) }
function Assert-LabelKeys($Object,[string[]]$Expected,[string]$Scope) {
    if ($Object -isnot [pscustomobject] -or (($Object.PSObject.Properties.Name|Sort-Object)-join'|')-cne(($Expected|Sort-Object)-join'|')) { throw "HUD label exact $Scope schema differs" }
}
function Assert-LabelInteger($Object,[string]$Key,[long]$Expected) {
    $value=$Object.$Key
    if (($value-isnot[int] -and $value-isnot[long]) -or $value-ne$Expected) { throw "HUD label exact integer differs: $Key" }
}
function Get-LabelCsvInteger($Row,[string]$Key) {
    $text=[string]$Row.$Key; $value=0L
    if ($text-cnotmatch'^(0|[1-9][0-9]*)$' -or ![long]::TryParse($text,[ref]$value)) { throw "HUD label canonical CSV integer differs: $Key" }
    return $value
}
function Assert-LabelColumns($Rows,[string[]]$Expected,[int]$Count,[string]$Scope) {
    if ($Rows.Count-ne$Count -or ($Rows[0].PSObject.Properties.Name-join'|')-cne($Expected-join'|')) { throw "HUD label exact $Scope CSV shape differs" }
}
function Assert-LabelFlags($Row,[string[]]$Flags) {
    foreach($flag in $Flags) { if($Row.$flag-cne'true') { throw "HUD label native flag differs: $flag" } }
}
$labelInputs=[Collections.Generic.List[string]]::new()
$labelSummaryPath=Join-Path $captures 'hud-label-colour-consumer-summary.json'
$labelMainPath=Join-Path $captures 'hud-label-colour-main.csv'
foreach($path in @($labelSummaryPath,$labelMainPath,(Join-Path $run 'run.log'),(Join-Path $run 'run.err'))) {
    if(!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'HUD label required native evidence missing' }; $labelInputs.Add($path)
}
if(@(Get-ChildItem -LiteralPath $captures -Filter 'hud-label-failure-*').Count) { throw 'HUD label native source-hypothesis failure artifacts remain' }
$labelSummary=Get-Content -LiteralPath $labelSummaryPath -Raw|ConvertFrom-Json
$labelMain=@(Import-Csv -LiteralPath $labelMainPath)
$labelCounts=[ordered]@{readbacks=39;csv_rows=8;foreground_readbacks=19;shadow_readbacks=20;crops=16;crop_pixels=345096;ppm_bytes=1035520;roi_pixels=46374;independent_observations=8;roi_pixel_checks=370992;repeat_checks=8;copied_adoption_checks=8;controls=7;roi_control_discriminations=13;shadow_meter_invariance_checks=6;calibration_pixel_checks=1248;private_game_saves=0}
Assert-LabelKeys $labelSummary (@($labelCounts.Keys)+$labelUpdates+@('scope','context','control_discriminating_pixels')) 'shared'
foreach($key in $labelCounts.Keys) { Assert-LabelInteger $labelSummary $key $labelCounts[$key] }
foreach($key in $labelUpdates) { Assert-LabelInteger $labelSummary $key 0 }
$labelControlPixels=$labelSummary.control_discriminating_pixels
if(($labelControlPixels-isnot[int] -and $labelControlPixels-isnot[long]) -or $labelControlPixels-le0 -or $labelControlPixels-gt298722 -or $labelSummary.scope-cne'three-foreground-labels-and-identity-shadow' -or $labelSummary.context-cne'prepared-normal-123.25') { throw 'HUD label shared scope/context or observed control support differs' }
Assert-LabelColumns $labelMain @('role','state','raw_rgba_match','adoption_match','repeat_match','locality_match','frozen_style_resources_rng','controls','roi_discriminations','identity_control_pixels','health_control_pixels','paint_control_pixels','scope') 8 'main'
$labelSumPixels=0L; $labelSumControls=0L; $labelSumDiscriminations=0L
for($i=0;$i-lt8;++$i) {
    $row=$labelMain[$i]; $role=if($i-lt4){'foreground'}else{'shadow'}; $state=$labelStates[$i%4]
    Assert-LabelFlags $row @('raw_rgba_match','adoption_match','repeat_match','locality_match','frozen_style_resources_rng')
    $controls=if($state-ceq'partial'){if($role-ceq'foreground'){2}else{3}}elseif($state-ceq'zero'){1}else{0}
    $discriminations=if($role-ceq'foreground'){$controls*3}else{$controls}
    $identity=Get-LabelCsvInteger $row 'identity_control_pixels'; $health=Get-LabelCsvInteger $row 'health_control_pixels'; $paint=Get-LabelCsvInteger $row 'paint_control_pixels'
    if($row.role-cne$role -or $row.state-cne$state -or $row.scope-cne'ordered-hud-label-glyph-paper-rois' -or (Get-LabelCsvInteger $row 'controls')-ne$controls -or (Get-LabelCsvInteger $row 'roi_discriminations')-ne$discriminations) { throw 'HUD label role/palette/control applicability differs' }
    if($identity-gt39900*$controls -or ($controls-gt0 -and $identity-le0)) { throw 'HUD identity tint/order did not separately discriminate' }
    if($role-ceq'foreground') {
        if($health-gt3666*$controls -or $paint-gt2808*$controls -or ($controls-gt0 -and ($health-le0 -or $paint-le0))) { throw 'Both HEALTH and PAINT must separately discriminate every applicable tint control' }
    } elseif($health-ne0 -or $paint-ne0) { throw 'Identity-only shadow control changed meter labels' }
    $labelSumPixels+=$identity+$health+$paint; $labelSumControls+=$controls; $labelSumDiscriminations+=$discriminations
}
if($labelSumPixels-ne$labelControlPixels -or $labelSumControls-ne7 -or $labelSumDiscriminations-ne13) { throw 'HUD label per-region observed controls do not reconcile' }
$labelCropPlan=@(foreach($role in @('foreground','shadow')) { foreach($state in $labelStates) {
    $regions=if($role-ceq'foreground'){@('identity','health','paint')}else{@('identity')}
    foreach($region in $regions) {
        $width=if($region-ceq'identity'){380}elseif($region-ceq'health'){141}else{108}
        $height=if($region-ceq'identity'){105}else{26}
        [ordered]@{name="hud-label-colour-$role-$state-$region.ppm";role=$role;region=$region;width=$width;height=$height;scope="$role-$region-source-glyph-paper-ROI"}
    }
} })
$labelPpms=@(Get-ChildItem -LiteralPath $captures -Filter 'hud-label-colour-*.ppm' -File)
if($labelPpms.Count-ne16 -or (($labelPpms.Name|Sort-Object)-join'|')-cne(($labelCropPlan.name|Sort-Object)-join'|')) { throw 'HUD label exact sixteen-crop PPM inventory differs' }
$labelPpmEvidence=@(foreach($crop in $labelCropPlan) {
    $path=Join-Path $captures $crop.name; $bytes=[IO.File]::ReadAllBytes($path)
    $header=[Text.Encoding]::ASCII.GetBytes("P6`n$($crop.width) $($crop.height)`n255`n"); $rgbBytes=$crop.width*$crop.height*3
    if($bytes.Length-ne($header.Length+$rgbBytes)) { throw 'HUD label canonical native P6 length differs' }
    for($i=0;$i-lt$header.Length;++$i) { if($bytes[$i]-ne$header[$i]) { throw 'HUD label canonical native P6 header differs' } }
    $rgb=[byte[]]::new($rgbBytes); [Array]::Copy($bytes,$header.Length,$rgb,0,$rgbBytes); $labelInputs.Add($path)
    [ordered]@{name=$crop.name;role=$crop.role;region=$crop.region;scope=$crop.scope;width=$crop.width;height=$crop.height;header_bytes=$header.Length;bytes=$bytes.Length;sha256=(Get-LabelSha $bytes);rgb_payload_sha256=(Get-LabelSha $rgb)}
})
if(($labelPpmEvidence.bytes|Measure-Object -Sum).Sum-ne1035520) { throw 'HUD label native total P6 bytes differ' }
if(@(Get-Content -LiteralPath (Join-Path $run 'run.log')|Where-Object{$_-match'Saving\.\.\.$' -or $_-ceq'Save was successful.'}).Count -or @(Get-Content -LiteralPath (Join-Path $run 'run.err')|Where-Object{$_-match'^save failure:'}).Count) { throw 'HUD label scope unexpectedly called or failed a game Save' }
$script:hudLabelObserved=[ordered]@{consumer=$labelSummary;foreground49='identity, HEALTH, PAINT';shadow50='identity only';readbacks=39;csv_rows=8;crops=16;crop_pixels=345096;ppm_bytes=1035520;controls=7;roi_control_discriminations=13;control_discriminating_pixels=$labelControlPixels;calibration_pixel_checks=1248;private_game_saves=0;reference_scope='same-font explicit-channel glyphs; identity ordered shadow/foreground over paper plus source lower strip; flat meter paper and no meter shadow; copied HUD adoption only';png_count=0;png_crops=@();png_complete=$false}
if($debug) {
    $labelTuneSummaryPath=Join-Path $captures 'hud-label-tune-summary.json'; $labelTuneCsvPath=Join-Path $captures 'hud-label-tune.csv'; $labelTranscript=Join-Path $captures 'hud-label-tune-commands.log'
    foreach($path in @($labelTuneSummaryPath,$labelTuneCsvPath,$labelTranscript)) {
        if(!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'HUD label required Debug evidence missing' }; $labelInputs.Add($path)
    }
    $tune=Get-Content -LiteralPath $labelTuneSummaryPath -Raw|ConvertFrom-Json; $rows=@(Import-Csv -LiteralPath $labelTuneCsvPath)
    $counts=[ordered]@{commands=15;states=15;readbacks=60;csv_rows=15;crops=0;saved_tokens=2;presentation_save_commands=1;roi_pixel_checks=695610;repeat_checks=15;copied_adoption_checks=15;sibling_checks=15;calibration_pixel_checks=1920;private_game_saves=0}
    Assert-LabelKeys $tune (@($counts.Keys)+$labelUpdates+@('scope')) 'Debug'
    foreach($key in $counts.Keys) { Assert-LabelInteger $tune $key $counts[$key] }; foreach($key in $labelUpdates) { Assert-LabelInteger $tune $key 0 }
    if($tune.scope-cne'debug-paired-hud-label-live') { throw 'HUD label Debug paired scope differs' }
    Assert-LabelColumns $rows @('state','foreground_token','shadow_token','raw_rgba_match','adoption_match','repeat_match','sibling_preserved','private_bytes','frozen_style_resources_rng','scope') 15 'Debug'
    $states=@('baseline','named-foreground','reset-original-foreground','named-shadow','reset-original-shadow','zero-foreground','reset-zero-foreground','zero-shadow','reset-zero-shadow','foreground-pending','joint-pending','after-save-foreground','reset-saved-foreground','after-save-shadow','reset-saved-shadow')
    $foregroundTokens=@('141414','RED','141414','141414','141414','A532D600','141414','141414','141414','2060C080','2060C080','35BCE1','2060C080','2060C080','2060C080')
    $shadowTokens=@('00000022','00000022','00000022','RED','00000022','00000022','00000022','A532D600','00000022','00000022','E17C3580','E17C3580','E17C3580','35BCE1','E17C3580')
    for($i=0;$i-lt15;++$i) {
        $row=$rows[$i]; Assert-LabelFlags $row @('raw_rgba_match','adoption_match','repeat_match','sibling_preserved','private_bytes','frozen_style_resources_rng')
        if($row.state-cne$states[$i] -or $row.foreground_token-cne$foregroundTokens[$i] -or $row.shadow_token-cne$shadowTokens[$i] -or $row.scope-cne'debug-paired-hud-label-live') { throw 'HUD label actual paired token/sibling observation differs' }
    }
    if((Get-Item -LiteralPath $labelTranscript).Length-le0 -or (Test-Path -LiteralPath (Join-Path $captures 'hud-label-tune-scratch/profile/ColourWars/save_data.json'))) { throw 'HUD label transcript/no-game-Save guard differs' }
    foreach($name in @('source/tuning.json','source/presentation.json','staged/tuning.json','staged/presentation.json')) { $labelInputs.Add((Join-Path $captures "hud-label-tune-scratch/$name")) }
    $script:hudLabelObserved.developer_commands=$tune; $script:hudLabelObserved.readbacks=99; $script:hudLabelObserved.csv_rows=23; $script:hudLabelObserved.calibration_pixel_checks=3168
} elseif(@(Get-ChildItem -LiteralPath $captures -Filter 'hud-label-tune-*').Count) { throw 'Release HUD label leaf must contain no Debug artifacts' }
$script:hudLabelVerified=[ordered]@{directory=[IO.Path]::GetFullPath($captures);ppms=$labelPpmEvidence;inputs=@(foreach($path in $labelInputs){[ordered]@{path=$path;sha256=(Get-FileHash -LiteralPath $path).Hash}})}
$result.observed_counts=$script:hudLabelObserved
function Complete-HudLabelColourEvidence {
    param([Parameter(Mandatory)][string]$Directory)
    $context=$script:hudLabelVerified
    if($null-eq$context -or ![StringComparer]::OrdinalIgnoreCase.Equals([IO.Path]::GetFullPath($Directory),$context.directory)) { throw 'HUD label PNG completion directory differs' }
    foreach($inputFile in $context.inputs) {
        if(!(Test-Path -LiteralPath $inputFile.path -PathType Leaf) -or (Get-FileHash -LiteralPath $inputFile.path).Hash-cne$inputFile.sha256) { throw 'HUD label verified input changed before PNG completion' }
    }
    $pngs=@(Get-ChildItem -LiteralPath $Directory -Filter 'hud-label-colour-*.png' -File)
    $expected=@($context.ppms|ForEach-Object{[IO.Path]::ChangeExtension($_.name,'.png')})
    if($pngs.Count-ne16 -or (($pngs.Name|Sort-Object)-join'|')-cne(($expected|Sort-Object)-join'|')) { throw 'HUD label exact native PNG inventory differs' }
    Add-Type -AssemblyName System.Drawing
    $evidence=@(foreach($ppm in $context.ppms) {
        $ppmPath=Join-Path $Directory $ppm.name; $ppmBytes=[IO.File]::ReadAllBytes($ppmPath)
        if((Get-LabelSha $ppmBytes)-cne$ppm.sha256) { throw 'HUD label PPM changed before PNG decode' }
        $path=[IO.Path]::ChangeExtension($ppmPath,'.png'); $bytes=[IO.File]::ReadAllBytes($path); $signature=@(137,80,78,71,13,10,26,10)
        if($bytes.Length-lt33) { throw 'HUD label PNG header truncated' }
        for($i=0;$i-lt8;++$i) { if($bytes[$i]-ne$signature[$i]) { throw 'HUD label PNG signature differs' } }
        if(($bytes[8..11]-join',')-cne'0,0,0,13' -or [Text.Encoding]::ASCII.GetString($bytes,12,4)-cne'IHDR') { throw 'HUD label PNG initial IHDR differs' }
        $width=0L; $height=0L
        for($i=0;$i-lt4;++$i) { $width=$width*256+$bytes[16+$i]; $height=$height*256+$bytes[20+$i] }
        if($width-ne$ppm.width -or $height-ne$ppm.height -or $bytes[24]-ne8 -or $bytes[25]-notin@(2,6) -or $bytes[26]-ne0 -or $bytes[27]-ne0 -or $bytes[28]-notin@(0,1)) { throw 'HUD label native RGB/RGBA PNG IHDR differs' }
        $bitmap=[Drawing.Bitmap]::new($path)
        try {
            if($bitmap.Width-ne$ppm.width -or $bitmap.Height-ne$ppm.height) { throw 'HUD label decoded PNG dimensions differ' }
            $rgb=[byte[]]::new($ppm.width*$ppm.height*3)
            for($y=0;$y-lt$ppm.height;++$y) { for($x=0;$x-lt$ppm.width;++$x) {
                $pixel=$bitmap.GetPixel($x,$y); $at=($y*$ppm.width+$x)*3
                if($pixel.A-ne255) { throw 'HUD label decoded PNG alpha is not opaque' }
                $rgb[$at]=$pixel.R; $rgb[$at+1]=$pixel.G; $rgb[$at+2]=$pixel.B
            } }
        } finally { $bitmap.Dispose() }
        for($i=0;$i-lt$rgb.Length;++$i) { if($rgb[$i]-ne$ppmBytes[$ppm.header_bytes+$i]) { throw 'HUD label decoded PNG differs from complete retained PPM RGB' } }
        $digest=Get-LabelSha $rgb
        if($digest-cne$ppm.rgb_payload_sha256) { throw 'HUD label decoded PNG RGB fingerprint differs' }
        $metadata=[ordered]@{name=[IO.Path]::GetFileName($path);source_ppm=$ppm.name;scope=$ppm.scope;width=$ppm.width;height=$ppm.height;pixels=($ppm.width*$ppm.height);bit_depth=[int]$bytes[24];colour_type=[int]$bytes[25];ppm_sha256=$ppm.sha256;png_sha256=(Get-LabelSha $bytes);decoded_rgb_sha256=$digest;decoded_rgb_matches_ppm=$true;alpha_opaque=$true}
        $metadata|ConvertTo-Json -Depth 5|Set-Content -LiteralPath "$path.metadata.json" -Encoding utf8
        $metadata
    })
    $script:hudLabelObserved.png_crops=$evidence; $script:hudLabelObserved.png_count=16; $script:hudLabelObserved.png_pixels=345096
    $script:hudLabelObserved.png_rgb_bytes=1035288; $script:hudLabelObserved.png_complete=$true
}
