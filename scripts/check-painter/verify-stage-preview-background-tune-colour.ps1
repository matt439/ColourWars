function Get-PreviewBackingSha([byte[]]$Bytes) { [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($Bytes)) }
function Assert-PreviewBackingKeys($Object,[string[]]$Expected,[string]$Scope) {
    if ($Object -isnot [pscustomobject] -or (($Object.PSObject.Properties.Name|Sort-Object)-join'|')-cne(($Expected|Sort-Object)-join'|')) { throw "Preview backing exact $Scope schema differs" }
}
function Assert-PreviewBackingInteger($Object,[string]$Key,[long]$Expected) {
    $value=$Object.$Key
    if (($value-isnot[int] -and $value-isnot[long]) -or $value-ne$Expected) { throw "Preview backing exact integer differs: $Key" }
}
function Get-PreviewBackingCsvInteger($Row,[string]$Key) {
    $text=[string]$Row.$Key; $value=0L
    if ($text-cnotmatch'^(0|[1-9][0-9]*)$' -or ![long]::TryParse($text,[ref]$value)) { throw "Preview backing canonical CSV integer differs: $Key" }
    return $value
}
function Assert-PreviewBackingColumns($Rows,[string[]]$Expected,[int]$Count,[string]$Scope) {
    if ($Rows.Count-ne$Count -or ($Rows[0].PSObject.Properties.Name-join'|')-cne($Expected-join'|')) { throw "Preview backing exact $Scope CSV shape differs" }
}
function Assert-PreviewBackingFlags($Row,[string[]]$Flags) {
    foreach($flag in $Flags) { if($Row.$flag-cne'true') { throw "Preview backing native flag differs: $flag" } }
}

# This verifies a bounded public-page material and capture-lifetime fixture.
# Retained images are data; no artifact is executed by the verifier.
$previewBackingZeros=@('positive_time_updates','level_updates','player_updates','scene_updates','gameplay_weapon_updates','private_game_saves')
$previewBackingStates=@('default','rgb','partial','zero')
$previewBackingInputs=[Collections.Generic.List[string]]::new()
$previewBackingSummaryPath=Join-Path $captures 'stage-preview-background-colour-consumer-summary.json'
$previewBackingMainPath=Join-Path $captures 'stage-preview-background-colour-main.csv'
$previewBackingLifetimePath=Join-Path $captures 'stage-preview-background-lifetime.csv'
foreach($path in @($previewBackingSummaryPath,$previewBackingMainPath,$previewBackingLifetimePath,(Join-Path $run 'run.log'),(Join-Path $run 'run.err'))) {
    if(!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Preview backing required evidence missing' }; $previewBackingInputs.Add($path)
}
if(@(Get-ChildItem -LiteralPath $captures -Filter 'stage-preview-background-failure-*').Count) { throw 'Preview backing source-hypothesis failure artifacts remain' }
$previewBackingSummary=Get-Content -LiteralPath $previewBackingSummaryPath -Raw|ConvertFrom-Json
$previewBackingMain=@(Import-Csv -LiteralPath $previewBackingMainPath)
$previewBackingLifetime=@(Import-Csv -LiteralPath $previewBackingLifetimePath)
$previewBackingCounts=[ordered]@{readbacks=68;csv_rows=26;main_rows=16;lifetime_rows=10;crops=16;crop_width=64;crop_height=32;crop_pixels=32768;ppm_bytes=98512;roi_pixels=2048;roi_observations=25;roi_pixel_checks=51200;repeat_checks=26;raw_references=8;controls=8;full_page_lifetime_checks=9;weapon_transition_nonoracle_checks=1;explicit_stage_pushes=9;back_weapon_proceed_routes=1;tap_gestures=21;local_menu_updates=63;gamepad_polls=83;keyboard_polls=83;mouse_polls=83;input_polls=249;calibration_pixel_checks=2176}
Assert-PreviewBackingKeys $previewBackingSummary (@($previewBackingCounts.Keys)+$previewBackingZeros+@('scope','control_discriminating_pixels')) 'shared'
foreach($key in $previewBackingCounts.Keys) { Assert-PreviewBackingInteger $previewBackingSummary $key $previewBackingCounts[$key] }
foreach($key in $previewBackingZeros) { Assert-PreviewBackingInteger $previewBackingSummary $key 0 }
$previewBackingControlPixels=$previewBackingSummary.control_discriminating_pixels
if(($previewBackingControlPixels-isnot[int] -and $previewBackingControlPixels-isnot[long]) -or $previewBackingControlPixels-le0 -or $previewBackingControlPixels-gt16384 -or $previewBackingSummary.scope-cne'captured-preview-backing-two-underlays') { throw 'Preview backing scope or observed control support differs' }
Assert-PreviewBackingColumns $previewBackingMain @('state','context','readiness','raw_rgba_match','repeat_match','locality_match','frozen_resources_plan_rng','controls','control_pixels','scope') 16 'main'
$previewBackingSumPixels=0L; $previewBackingSumControls=0L
for($i=0;$i-lt16;++$i) {
    $row=$previewBackingMain[$i]; $state=$previewBackingStates[[int][Math]::Floor($i/4)]
    $context=if(($i%4)-lt2){'crossfeed'}else{'random'}; $readiness=if(($i%2)-eq0){'browse'}else{'locked'}
    $controls=if($context-ceq'random'){0}elseif($state-ceq'default'){1}elseif($state-ceq'partial'){2}elseif($state-ceq'zero'){1}else{0}
    Assert-PreviewBackingFlags $row @('raw_rgba_match','repeat_match','locality_match','frozen_resources_plan_rng')
    $pixels=Get-PreviewBackingCsvInteger $row 'control_pixels'
    if($row.state-cne$state -or $row.context-cne$context -or $row.readiness-cne$readiness -or $row.scope-cne'actual-page-backing-underlay-roi' -or (Get-PreviewBackingCsvInteger $row 'controls')-ne$controls -or $pixels-gt$controls*2048 -or ($controls-gt0 -and $pixels-le0)) { throw 'Preview primary palette/context/readiness/control evidence differs' }
    $previewBackingSumPixels+=$pixels; $previewBackingSumControls+=$controls
}
if($previewBackingSumPixels-ne$previewBackingControlPixels -or $previewBackingSumControls-ne8) { throw 'Preview backing observed controls do not reconcile' }
Assert-PreviewBackingColumns $previewBackingLifetime @('state','source_token','captured_token','context','readiness','repeat_match','raw_rgba_match','full_page_guard','frozen_resources_plan_rng','scope') 10 'lifetime'
$previewBackingLifeStates=@('authored-open','source-edited-retained','cycled-random-retained','random-locked-retained','locked-arrow-ignored','random-unlocked-retained','cycled-crossfeed-retained','crossfeed-locked-retained','back-to-weapon','weapon-proceed-reopened')
$previewBackingLifeContexts=@('crossfeed','crossfeed','random','random','random','random','crossfeed','crossfeed','weapon','crossfeed')
$previewBackingLifeReadiness=@('browse','browse','browse','locked','locked','browse','browse','locked','not-stage-page','browse')
for($i=0;$i-lt10;++$i) {
    $row=$previewBackingLifetime[$i]; $source=if($i-eq0){'121827EE'}else{'2060C080'}
    $captured=if($i-eq8){'not-stage-page'}elseif($i-eq9){'2060C080'}else{'121827EE'}
    $raw=if($i-eq8){'not-run-weapon-transition'}else{'true'}
    $guard=if($i-eq8){'page-changed'}elseif($i-eq9){'exact-fresh-partial'}else{'exact-captured-authored'}
    Assert-PreviewBackingFlags $row @('repeat_match','frozen_resources_plan_rng')
    if($row.state-cne$previewBackingLifeStates[$i] -or $row.source_token-cne$source -or $row.captured_token-cne$captured -or $row.context-cne$previewBackingLifeContexts[$i] -or $row.readiness-cne$previewBackingLifeReadiness[$i] -or $row.raw_rgba_match-cne$raw -or $row.full_page_guard-cne$guard -or $row.scope-cne'captured-next-screen-lifetime') { throw 'Preview actual capture/reopen/Weapon nonoracle evidence differs' }
}
$previewBackingPlan=@(foreach($state in $previewBackingStates) { foreach($context in @('crossfeed','random')) { foreach($readiness in @('browse','locked')) {
    [ordered]@{name="stage-preview-background-colour-$state-$context-$readiness.ppm";scope="$context-$readiness-captured-preview-backing-material-ROI";width=64;height=32}
} } })
$previewBackingPpms=@(Get-ChildItem -LiteralPath $captures -Filter 'stage-preview-background-colour-*.ppm' -File)
if($previewBackingPpms.Count-ne16 -or (($previewBackingPpms.Name|Sort-Object)-join'|')-cne(($previewBackingPlan.name|Sort-Object)-join'|')) { throw 'Preview backing exact sixteen-crop PPM inventory differs' }
$previewBackingPpmEvidence=@(foreach($crop in $previewBackingPlan) {
    $path=Join-Path $captures $crop.name; $bytes=[IO.File]::ReadAllBytes($path); $header=[Text.Encoding]::ASCII.GetBytes("P6`n64 32`n255`n")
    if($bytes.Length-ne6157 -or $header.Length-ne13) { throw 'Preview backing canonical native P6 length differs' }
    for($i=0;$i-lt13;++$i) { if($bytes[$i]-ne$header[$i]) { throw 'Preview backing canonical P6 header differs' } }
    $rgb=[byte[]]::new(6144); [Array]::Copy($bytes,13,$rgb,0,6144); $previewBackingInputs.Add($path)
    [ordered]@{name=$crop.name;scope=$crop.scope;width=64;height=32;header_bytes=13;bytes=6157;sha256=(Get-PreviewBackingSha $bytes);rgb_payload_sha256=(Get-PreviewBackingSha $rgb)}
})
if(($previewBackingPpmEvidence.bytes|Measure-Object -Sum).Sum-ne98512) { throw 'Preview backing total native P6 bytes differ' }
if(@(Get-Content -LiteralPath (Join-Path $run 'run.log')|Where-Object{$_-match'Saving\.\.\.$' -or $_-ceq'Save was successful.'}).Count -or @(Get-Content -LiteralPath (Join-Path $run 'run.err')|Where-Object{$_-match'^save failure:'}).Count) { throw 'Preview scope unexpectedly called or failed a game Save' }
$script:previewBackingObserved=[ordered]@{consumer=$previewBackingSummary;readbacks=68;csv_rows=26;crops=16;crop_pixels=32768;ppm_bytes=98512;controls=8;control_discriminating_pixels=$previewBackingControlPixels;calibration_pixel_checks=2176;local_menu_updates=63;gamepad_polls=83;keyboard_polls=83;mouse_polls=83;input_polls=249;private_game_saves=0;default_token='121827EE';old_default_pixel_parity=$false;underlays=[ordered]@{browse='445268';locked='75EDD7'};reference_scope='small actual-page backing ROI: opaque filled edge then explicit once-premultiplied selected tint; matching-state complete-page equality proves capture/adoption only; Weapon transition is not a raw colour oracle';png_count=0;png_crops=@();png_complete=$false}
if($debug) {
    $summaryPath=Join-Path $captures 'stage-preview-background-tune-summary.json'; $csvPath=Join-Path $captures 'stage-preview-background-tune.csv'; $transcript=Join-Path $captures 'stage-preview-background-tune-commands.log'
    foreach($path in @($summaryPath,$csvPath,$transcript)) {
        if(!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Preview required Debug evidence missing' }; $previewBackingInputs.Add($path)
    }
    $tune=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json; $rows=@(Import-Csv -LiteralPath $csvPath)
    $counts=[ordered]@{commands=7;query_commands=1;tune_commands=3;reset_commands=2;presentation_save_commands=1;saved_tokens=1;states=12;readbacks=24;csv_rows=12;crops=0;roi_observations=12;roi_pixel_checks=24576;repeat_checks=12;explicit_stage_pushes=1;back_weapon_proceed_routes=4;tap_gestures=9;local_menu_updates=34;gamepad_polls=38;keyboard_polls=38;mouse_polls=38;input_polls=114;calibration_pixel_checks=768}
    Assert-PreviewBackingKeys $tune (@($counts.Keys)+$previewBackingZeros+@('scope')) 'Debug'
    foreach($key in $counts.Keys) { Assert-PreviewBackingInteger $tune $key $counts[$key] }; foreach($key in $previewBackingZeros) { Assert-PreviewBackingInteger $tune $key 0 }
    if($tune.scope-cne'debug-captured-next-screen-preview') { throw 'Preview Debug next-screen scope differs' }
    Assert-PreviewBackingColumns $rows @('state','source_token','captured_token','context','document_revision','ledger_entries','repeat_match','raw_rgba_match','full_page_match','private_bytes','frozen_resources_plan_rng','scope') 12 'Debug'
    $states=@('baseline','edit-retained','cycle-retained','edit-reopened','reset-original-retained','reset-original-reopened','save-pending-retained','saved-retained','saved-reopened','post-save-edit-retained','reset-saved-retained','reset-saved-reopened')
    $sourceTokens=@('121827EE','2060C080','2060C080','2060C080','121827EE','121827EE','2060C080','2060C080','2060C080','35BCE1','2060C080','2060C080')
    $capturedTokens=@('121827EE','121827EE','121827EE','2060C080','2060C080','121827EE','121827EE','121827EE','2060C080','2060C080','2060C080','2060C080')
    $revisions=@(0,1,1,1,2,2,3,4,4,5,6,6); $ledgers=@(0,1,1,1,0,0,1,0,0,1,0,0)
    for($i=0;$i-lt12;++$i) {
        $row=$rows[$i]; $context=if($i-eq2){'random'}else{'crossfeed'}
        Assert-PreviewBackingFlags $row @('repeat_match','raw_rgba_match','full_page_match','private_bytes','frozen_resources_plan_rng')
        if($row.state-cne$states[$i] -or $row.source_token-cne$sourceTokens[$i] -or $row.captured_token-cne$capturedTokens[$i] -or $row.context-cne$context -or (Get-PreviewBackingCsvInteger $row 'document_revision')-ne$revisions[$i] -or (Get-PreviewBackingCsvInteger $row 'ledger_entries')-ne$ledgers[$i] -or $row.scope-cne'debug-captured-next-screen-preview') { throw 'Preview actual source/captured token, document revision/ledger or retained/fresh observation differs' }
    }
    if((Get-Item -LiteralPath $transcript).Length-le0 -or (Test-Path -LiteralPath (Join-Path $captures 'stage-preview-background-tune-scratch/profile/ColourWars/save_data.json'))) { throw 'Preview transcript or no-game-Save guard differs' }
    foreach($name in @('source/tuning.json','source/presentation.json','staged/tuning.json','staged/presentation.json')) {
        $path=Join-Path $captures "stage-preview-background-tune-scratch/$name"; if(!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Preview private source/staging evidence missing' }; $previewBackingInputs.Add($path)
    }
    foreach($name in @('tuning.json','presentation.json')) {
        $a=Join-Path $captures "stage-preview-background-tune-scratch/source/$name"; $b=Join-Path $captures "stage-preview-background-tune-scratch/staged/$name"
        if((Get-FileHash -LiteralPath $a).Hash-cne(Get-FileHash -LiteralPath $b).Hash) { throw 'Preview saved source/staged bytes differ' }
    }
    $script:previewBackingObserved.developer_commands=$tune; $script:previewBackingObserved.readbacks=92; $script:previewBackingObserved.csv_rows=38
    $script:previewBackingObserved.calibration_pixel_checks=2944; $script:previewBackingObserved.local_menu_updates=97
    $script:previewBackingObserved.gamepad_polls=121; $script:previewBackingObserved.keyboard_polls=121; $script:previewBackingObserved.mouse_polls=121; $script:previewBackingObserved.input_polls=363
} elseif(@(Get-ChildItem -LiteralPath $captures -Filter 'stage-preview-background-tune-*').Count) { throw 'Release preview leaf must contain no developer artifacts' }
$script:previewBackingVerified=[ordered]@{directory=[IO.Path]::GetFullPath($captures);ppms=$previewBackingPpmEvidence;inputs=@(foreach($path in $previewBackingInputs){[ordered]@{path=$path;sha256=(Get-FileHash -LiteralPath $path).Hash}})}
$result.observed_counts=$script:previewBackingObserved

function Complete-StagePreviewBackgroundColourEvidence {
    param([Parameter(Mandatory)][string]$Directory)
    $context=$script:previewBackingVerified
    if($null-eq$context -or ![StringComparer]::OrdinalIgnoreCase.Equals([IO.Path]::GetFullPath($Directory),$context.directory)) { throw 'Preview backing PNG completion directory differs' }
    foreach($inputFile in $context.inputs) {
        if(!(Test-Path -LiteralPath $inputFile.path -PathType Leaf) -or (Get-FileHash -LiteralPath $inputFile.path).Hash-cne$inputFile.sha256) { throw 'Preview backing verified input changed before PNG completion' }
    }
    $pngs=@(Get-ChildItem -LiteralPath $Directory -Filter 'stage-preview-background-colour-*.png' -File)
    $expected=@($context.ppms|ForEach-Object{[IO.Path]::ChangeExtension($_.name,'.png')})
    if($pngs.Count-ne16 -or (($pngs.Name|Sort-Object)-join'|')-cne(($expected|Sort-Object)-join'|')) { throw 'Preview backing exact native PNG inventory differs' }
    Add-Type -AssemblyName System.Drawing
    $evidence=@(foreach($ppm in $context.ppms) {
        $ppmPath=Join-Path $Directory $ppm.name; $ppmBytes=[IO.File]::ReadAllBytes($ppmPath)
        if((Get-PreviewBackingSha $ppmBytes)-cne$ppm.sha256) { throw 'Preview backing PPM changed before PNG decode' }
        $path=[IO.Path]::ChangeExtension($ppmPath,'.png'); $bytes=[IO.File]::ReadAllBytes($path); $signature=@(137,80,78,71,13,10,26,10)
        if($bytes.Length-lt33) { throw 'Preview backing PNG header truncated' }
        for($i=0;$i-lt8;++$i) { if($bytes[$i]-ne$signature[$i]) { throw 'Preview backing PNG signature differs' } }
        if(($bytes[8..11]-join',')-cne'0,0,0,13' -or [Text.Encoding]::ASCII.GetString($bytes,12,4)-cne'IHDR') { throw 'Preview backing PNG initial IHDR differs' }
        $width=0L; $height=0L
        for($i=0;$i-lt4;++$i) { $width=$width*256+$bytes[16+$i]; $height=$height*256+$bytes[20+$i] }
        if($width-ne$ppm.width -or $height-ne$ppm.height -or $bytes[24]-ne8 -or $bytes[25]-notin@(2,6) -or $bytes[26]-ne0 -or $bytes[27]-ne0 -or $bytes[28]-notin@(0,1)) { throw 'Preview backing native RGB/RGBA PNG IHDR differs' }
        $bitmap=[Drawing.Bitmap]::new($path)
        try {
            if($bitmap.Width-ne$ppm.width -or $bitmap.Height-ne$ppm.height) { throw 'Preview backing decoded PNG dimensions differ' }
            $rgb=[byte[]]::new($ppm.width*$ppm.height*3)
            for($y=0;$y-lt$ppm.height;++$y) { for($x=0;$x-lt$ppm.width;++$x) {
                $pixel=$bitmap.GetPixel($x,$y); $at=($y*$ppm.width+$x)*3
                if($pixel.A-ne255) { throw 'Preview backing decoded PNG alpha is not opaque' }
                $rgb[$at]=$pixel.R; $rgb[$at+1]=$pixel.G; $rgb[$at+2]=$pixel.B
            } }
        } finally { $bitmap.Dispose() }
        for($i=0;$i-lt$rgb.Length;++$i) { if($rgb[$i]-ne$ppmBytes[$ppm.header_bytes+$i]) { throw 'Preview backing decoded PNG differs from complete retained PPM RGB' } }
        $digest=Get-PreviewBackingSha $rgb
        if($digest-cne$ppm.rgb_payload_sha256) { throw 'Preview backing decoded PNG RGB fingerprint differs' }
        $metadata=[ordered]@{name=[IO.Path]::GetFileName($path);source_ppm=$ppm.name;scope=$ppm.scope;width=$ppm.width;height=$ppm.height;pixels=($ppm.width*$ppm.height);bit_depth=[int]$bytes[24];colour_type=[int]$bytes[25];ppm_sha256=$ppm.sha256;png_sha256=(Get-PreviewBackingSha $bytes);decoded_rgb_sha256=$digest;decoded_rgb_matches_ppm=$true;alpha_opaque=$true}
        $metadata|ConvertTo-Json -Depth 5|Set-Content -LiteralPath "$path.metadata.json" -Encoding utf8
        $metadata
    })
    $script:previewBackingObserved.png_crops=$evidence; $script:previewBackingObserved.png_count=16; $script:previewBackingObserved.png_pixels=32768
    $script:previewBackingObserved.png_rgb_bytes=98304; $script:previewBackingObserved.png_complete=$true
}
