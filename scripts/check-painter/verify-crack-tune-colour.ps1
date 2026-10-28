# Initial dot-source follows native execution and precedes Write-ContactSheet.
# Source-only frozen design54E23ACC6A50CB74106B630D9E6D56C3BC720A0AAC75905A569CB38E4218CEE4,
# models3E7F7C05F670CC04758F3826924C94991FE2B2E2F48D812A414E50F5BD0A4A09.
# All4 hypotheses coincide in each declared complete ROI. Hash admission checks
# every retained byte; it cannot distinguish mixtures of identical hypotheses.
# C++ owns actual/raw3910RGBA first, ordered masks, calibration and private Save.
$crackContexts = @('left', 'right')
$crackStates = @('default', 'rgb', 'rgba128', 'alpha0')
$crackModels = @('F_AF', 'Q_AF', 'F_AQ', 'Q_AQ')
$crackModelNames = 'F_AF|Q_AF|F_AQ|Q_AQ'
$crackSourcePins = @{
    'left/default' = @('6E838012252982690F14D48226147EBE4D6E9CDBC3FB7AA3D894C2879399ECBE', '4D85EA9C576584F155E1D60568E29FF680BBBE22568E13F74BF37E563813D5BF')
    'left/rgb' = @('727D8DB9C4B637F70E371A42A6889E621C0123BEB2DEC2B11D667DF1146E7783', '1EAA02923E31FF4EBE62E504C71C68D13E4ED5F6DA0AFDC5E0A18A11E50675FF')
    'left/rgba128' = @('50744AA8E4C4A8EA28BF58CDEFA8E4607CA75063C8355E46AAC72963F9FEACCA', '48D5DE002FACB38E83DD577660E8FE72D776A5AC24D4CCCD1413C6381391927B')
    'left/alpha0' = @('FA560CC2EF0E160202CDD6AAFDE1C64379648DFFA4CA6DEB38D0C3F3CD858D49', 'A1B581695C3212E37DCCA031517F56770108C3B714B8A249493344153D0CDA11')
    'right/default' = @('4342E03161283688552B78968FA9B3A55E85668FE339B9EE91232525BD9719D3', 'E021B41A4BD09AED14D2A741E52FC824977C24A5D35C5544C32E3E132E66EDEE')
    'right/rgb' = @('F692E0DAC4867287C5A18BEB04413C91D3782E4395718402AE9652F3B9E07729', '6E9FA1BD9B9742FB8D3A189EBB82419E261988EB14540C4E124BC881CBE9E1BA')
    'right/rgba128' = @('2CDEB329F9286136E261A4E9ACC021603379115EE210FAE1F5DEC2CF13CF89F4', '0BC0DF11F5E9D10DBAE2D9BD6B02F3171574A339EB4A2235A81C20D069C322E0')
    'right/alpha0' = @('1F78A117CFAF0204B221C0E2A2EA9C9804024F19BCB0B6A2BB8695D6064BE20F', '79157A1C43865724C2C804AA7763B16A1424F50F4FDD891450FB617A7EA811E9')
}
function Get-CrackSha([byte[]]$Bytes) { Get-PainterSha256 $Bytes }
function Assert-CrackKeys($Object, [string[]]$Keys, [string]$Scope) {
    if ($Object -isnot [pscustomobject] -or (($Object.PSObject.Properties.Name | Sort-Object) -join '|') -cne (($Keys | Sort-Object) -join '|')) { throw "Crack exact $Scope JSON schema differs" }
}
function Assert-CrackInteger($Object, [string]$Key, [long]$Expected) {
    $value = $Object.$Key
    if (($value -isnot [int] -and $value -isnot [long]) -or $value -ne $Expected) { throw "Crack exact integer differs: $Key" }
}
function Assert-CrackRoi($Object, [string]$Key, [int[]]$Expected) {
    $value = @($Object.$Key)
    if ($value.Count -ne 4 -or @($value | Where-Object { $_ -isnot [int] -and $_ -isnot [long] }).Count -or ($value -join ',') -cne ($Expected -join ',')) { throw "Crack literal integer ROI differs: $Key" }
}
function Assert-CrackPairs($Rows, [string[]]$States, [string[]]$Contexts) {
    $expected = @(foreach ($state in $States) { foreach ($context in $Contexts) { "$state/$context" } })
    if ($Rows.Count -ne $expected.Count -or (($Rows | ForEach-Object { "$($_.state)/$($_.context)" } | Sort-Object) -join '|') -cne (($expected | Sort-Object) -join '|')) { throw 'Crack exact state/context pairs differ' }
}

# PNGs do not exist at initial dot-source time. The focused wrapper alone calls
# this function after its existing lossless PPM conversion, before success.
function Complete-CrackTuneColourEvidence {
    param([Parameter(Mandatory)][string]$Directory)
    $context = $script:crackVerifiedContext
    if ($null -eq $context -or ![StringComparer]::OrdinalIgnoreCase.Equals([IO.Path]::GetFullPath($Directory), $context.directory)) { throw 'Crack PNG completion directory differs from initial verified evidence' }
    foreach ($inputFile in $context.inputs) {
        if (!(Test-Path -LiteralPath $inputFile.path -PathType Leaf) -or (Get-FileHash -LiteralPath $inputFile.path).Hash -cne $inputFile.sha256) { throw 'Crack verified input changed before PNG completion' }
    }
    $pngs = @(Get-ChildItem -LiteralPath $Directory -Filter 'crack-colour-*.png' -File | Sort-Object Name)
    $expected = @($context.ppms | ForEach-Object { [IO.Path]::ChangeExtension($_.name, '.png') })
    if ($pngs.Count -ne 8 -or (($pngs.Name | Sort-Object) -join '|') -cne (($expected | Sort-Object) -join '|')) { throw 'Crack exact PNG inventory differs' }
    Add-Type -AssemblyName System.Drawing
    $pngEvidence = @(foreach ($ppm in $context.ppms) {
        $ppmPath = Join-Path $Directory $ppm.name
        if (!(Test-Path -LiteralPath $ppmPath -PathType Leaf)) { throw 'Crack PPM changed between initial validation and PNG completion' }
        $ppmBytes = [IO.File]::ReadAllBytes($ppmPath)
        if ((Get-CrackSha $ppmBytes) -cne $ppm.sha256) { throw 'Crack PPM changed between initial validation and PNG completion' }
        $path = [IO.Path]::ChangeExtension($ppmPath, '.png'); $bytes = [IO.File]::ReadAllBytes($path)
        $signature = @(137,80,78,71,13,10,26,10)
        if ($bytes.Length -lt 33) { throw 'Crack PNG header truncated' }
        for ($index = 0; $index -lt 8; ++$index) { if ($bytes[$index] -ne $signature[$index]) { throw 'Crack PNG signature differs' } }
        if (($bytes[8..11] -join ',') -cne '0,0,0,13' -or [Text.Encoding]::ASCII.GetString($bytes,12,4) -cne 'IHDR') { throw 'Crack PNG initial IHDR differs' }
        $width = 0L; $height = 0L
        for ($index = 0; $index -lt 4; ++$index) { $width = $width * 256 + $bytes[16+$index]; $height = $height * 256 + $bytes[20+$index] }
        if ($width -ne 115 -or $height -ne 34 -or $bytes[24] -ne 8 -or $bytes[25] -notin @(2,6) -or $bytes[26] -ne 0 -or $bytes[27] -ne 0 -or $bytes[28] -notin @(0,1)) { throw 'Crack PNG native dimensions/8bit RGB-RGBA IHDR differs' }
        try { $bitmap = [Drawing.Bitmap]::new($path) } catch { throw 'Crack PNG decode failed' }
        try {
            if ($bitmap.Width -ne 115 -or $bitmap.Height -ne 34) { throw 'Crack decoded PNG native dimensions differ' }
            $rgb = [byte[]]::new(11730)
            for ($y = 0; $y -lt 34; ++$y) { for ($x = 0; $x -lt 115; ++$x) {
                $pixel = $bitmap.GetPixel($x,$y); $at = ($y*115+$x)*3
                if ($pixel.A -ne 255) { throw 'Crack PNG decoded alpha is not opaque' }
                $rgb[$at]=$pixel.R; $rgb[$at+1]=$pixel.G; $rgb[$at+2]=$pixel.B
            } }
        } finally { $bitmap.Dispose() }
        for ($index = 0; $index -lt 11730; ++$index) { if ($rgb[$index] -ne $ppmBytes[14+$index]) { throw 'Crack PNG decoded RGB differs from retained PPM' } }
        $digest = Get-CrackSha $rgb
        if ($digest -cne $ppm.source_payload_sha256) { throw 'Crack PNG complete RGB source digest differs' }
        [ordered]@{name=[IO.Path]::GetFileName($path);width=115;height=34;pixels=3910;bit_depth=[int]$bytes[24];colour_type=[int]$bytes[25];png_bytes=$bytes.Length;png_sha256=(Get-CrackSha $bytes);ppm_sha256=$ppm.sha256;decoded_rgb_sha256=$digest;decoded_rgb_matches_ppm=$true;alpha_opaque=$true;matched_models='F_AF|Q_AF|F_AQ|Q_AQ'}
    })
    $context.observed.png_crops = $pngEvidence
    $context.observed.png_count = 8
    $context.observed.png_pixels = 31280
    $context.observed.png_rgb_bytes = 93840
    $context.observed.png_complete = $true
}

$crackUpdates = @('local_updates','player_updates','level_updates','scene_updates','state_updates','input_updates','time_updates','input_polls')
$crackFlags = @('roi_rgba_match','adoption_match','repeat_match','literal_channels','effective_delta','frozen_inputs_resources_rng','calibration_rgba')
function Assert-CrackUpdates($Object) { foreach ($field in $crackUpdates) { Assert-CrackInteger $Object $field 0 } }
function Assert-CrackFlags($Row) {
    if ($Row.background -cne 'canvas32-64-96' -or $Row.source_models -cne 'F_AF|Q_AF|F_AQ|Q_AQ') { throw 'Crack native Canvas/all4 source aliases differ' }
    foreach ($field in $crackFlags) { if ($Row.$field -cne 'true') { throw "Crack native flag differs: $field" } }
}
$crackSummaryPath = Join-Path $captures 'crack-colour-consumer-summary.json'
$crackCsvPath = Join-Path $captures 'crack-colour-consumers.csv'
$crackInputs = [Collections.Generic.List[string]]::new()
foreach ($path in @($crackSummaryPath,$crackCsvPath,(Join-Path $run 'run.log'),(Join-Path $run 'run.err'))) {
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Crack required evidence file missing' }
    $crackInputs.Add($path)
}
if (@(Get-ChildItem -LiteralPath $captures -Filter 'crack-failure-*').Count) { throw 'Crack native failure artifacts remain' }
$crackSummary = Get-Content -LiteralPath $crackSummaryPath -Raw | ConvertFrom-Json
$crackRows = @(Import-Csv -LiteralPath $crackCsvPath)
$crackCounts = [ordered]@{readbacks=42;csv_rows=8;crops=8;crop_width=115;crop_height=34;crop_pixels=31280;ppm_bytes=93952;contexts=2;palettes=4;roi_pixels=3910;
    base_case_reads=32;actual_reads=16;adoption_reads=8;raw_positive_reads=8;blank_reads=0;calibration_only_reads=0;calibration_pixels_per_read=32;calibration_pixel_checks=1344;
    controls=10;partial_straight_reads=2;partial_squared_reads=2;collapsed_reads=2;all_ink_first_reads=2;alpha0_straight_reads=2;changed_control_pixels=1274;unchanged_control_pixels=37826;
    ink_submissions_per_path=77;selected_submissions_per_path=165;left_effective_pixels=179;right_effective_pixels=178;ever_selected_pixels_per_path=182}
Assert-CrackKeys $crackSummary (@($crackCounts.Keys)+$crackUpdates+@('scope','viewport','background','event_fade','actual_entry','raw_reference','whole_frame_reference','left_roi_half_open','right_roi_half_open','capture_kind','source_models')) 'shared'
foreach ($field in $crackCounts.Keys) { Assert-CrackInteger $crackSummary $field $crackCounts[$field] }
Assert-CrackUpdates $crackSummary
Assert-CrackRoi $crackSummary 'left_roi_half_open' @(2,472,117,506)
Assert-CrackRoi $crackSummary 'right_roi_half_open' @(1162,472,1277,506)
if ($crackSummary.scope -cne 'consumer-only' -or $crackSummary.viewport -cne '1280x720' -or $crackSummary.background -cne 'canvas32-64-96' -or
    $crackSummary.event_fade -isnot [bool] -or $crackSummary.event_fade -ne $false -or $crackSummary.source_models -cne $crackModelNames -or
    $crackSummary.actual_entry -cne 'Monitor::draw_front_details-only-plus-calibration' -or $crackSummary.raw_reference -cne 'ordered-segment-ink-then-thin-literal115x34-interior-only' -or
    $crackSummary.whole_frame_reference -cne 'copied-complete-frontdetails-adoption-only' -or $crackSummary.capture_kind -cne 'native-contiguous-ROI') { throw 'Crack oracle/calibration/source scope differs' }
Assert-CrackPairs $crackRows $crackStates $crackContexts
$crackColumns = @('state','context','background','roi_rgba_match','adoption_match','repeat_match','literal_channels','effective_delta','frozen_inputs_resources_rng','calibration_rgba','source_models','controls','changed_control_pixels','unchanged_control_pixels','scope')
if (($crackRows[0].PSObject.Properties.Name -join '|') -cne ($crackColumns -join '|')) { throw 'Crack shared CSV columns differ' }
$crackControlTotal=0; $crackChangedTotal=0; $crackUnchangedTotal=0
foreach ($row in $crackRows) {
    Assert-CrackFlags $row
    $controls=0; $changed=0; $unchanged=0
    if ($row.state -ceq 'rgba128') { $controls=4; $changed=if ($row.context -ceq 'left') {459} else {458} }
    elseif ($row.state -ceq 'alpha0') { $controls=1; $changed=if ($row.context -ceq 'left') {179} else {178} }
    $unchanged=$controls*3910-$changed
    if ($row.controls -cne [string]$controls -or $row.changed_control_pixels -cne [string]$changed -or $row.unchanged_control_pixels -cne [string]$unchanged -or $row.scope -cne 'consumer-only') { throw 'Crack per-case ordered/effective/erasure control-mask counts differ' }
    $crackControlTotal+=$controls; $crackChangedTotal+=$changed; $crackUnchangedTotal+=$unchanged
}
if ($crackControlTotal -ne 10 -or $crackChangedTotal -ne 1274 -or $crackUnchangedTotal -ne 37826) { throw 'Crack control-mask totals differ' }
$crackExpectedPpms = @(foreach ($context in $crackContexts) { foreach ($state in $crackStates) { "crack-colour-$context-$state.ppm" } })
$crackPpms = @(Get-ChildItem -LiteralPath $captures -Filter 'crack-colour-*.ppm' -File)
if ($crackPpms.Count -ne 8 -or (($crackPpms.Name | Sort-Object) -join '|') -cne (($crackExpectedPpms | Sort-Object) -join '|')) { throw 'Crack exact PPM inventory differs' }
$crackHeader = [Text.Encoding]::ASCII.GetBytes("P6`n115 34`n255`n")
$crackPpmEvidence = @(foreach ($context in $crackContexts) { foreach ($state in $crackStates) {
    $name="crack-colour-$context-$state.ppm"; $path=Join-Path $captures $name; $bytes=[IO.File]::ReadAllBytes($path)
    if ($bytes.Length -ne 11744) { throw 'Crack native P6 byte length differs' }
    for ($index=0; $index -lt 14; ++$index) { if ($bytes[$index] -ne $crackHeader[$index]) { throw 'Crack canonical P6 header differs' } }
    $payload=[byte[]]::new(11730); [Array]::Copy($bytes,14,$payload,0,11730)
    $payloadHash=Get-CrackSha $payload; $p6Hash=Get-CrackSha $bytes; $pin=$crackSourcePins["$context/$state"]
    if ($payloadHash -cne $pin[0] -or $p6Hash -cne $pin[1]) { throw "Crack complete retained RGB/P6 differs from frozen source: $context/$state" }
    [ordered]@{name=$name;width=115;height=34;pixels=3910;bytes=11744;sha256=$p6Hash;context=$context;state=$state;source_payload_sha256=$payloadHash;
        matching_source_models='F_AF|Q_AF|F_AQ|Q_AQ';source_model_equivalence_class=$payloadHash+':F_AF|Q_AF|F_AQ|Q_AQ';admission='exact whole3910RGB source ROI; no tolerance or fitting'}
} })
$crackLog=@(Get-Content -LiteralPath (Join-Path $run 'run.log'))
$crackSaving=@($crackLog | Where-Object { $_ -match 'Saving\.\.\.$' }).Count
$crackSaved=@($crackLog | Where-Object { $_ -ceq 'Save was successful.' }).Count
if (@(Get-Content -LiteralPath (Join-Path $run 'run.err') | Where-Object { $_ -match '^save failure:' }).Count) { throw 'Crack unexpected private Save failure' }
$crackArtifacts=[ordered]@{}
foreach ($path in @($crackSummaryPath,$crackCsvPath)) { $crackArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash} }
$crackArtifacts.crops=$crackPpmEvidence
$crackObserved=[ordered]@{consumer=$crackSummary;readbacks=42;csv_rows=8;crops=8;controls=10;changed_control_pixels=1274;unchanged_control_pixels=37826;crop_pixels=31280;retained_rgb_pixels=31280;retained_rgb_payload_bytes=93840;ppm_bytes=93952;
    source_precision_models='F_AF|Q_AF|F_AQ|Q_AQ';source_equivalence_classes=8;precision_admission='one exact complete ROI; all4 fixed hypotheses equivalent, no internal/universalGPU or indistinguishable-mixture claim';
    raw_scope='C++ complete3910RGBA first; ordered ink/thin independent interiors';full_frame_scope='copied complete frontdetails adoption/repeat/effective-exterior only';
    calibration_pixels_per_read=32;calibration_pixel_checks=1344;local_updates=0;player_updates=0;level_updates=0;scene_updates=0;state_updates=0;input_updates=0;time_updates=0;input_polls=0;
    private_save_calls=$crackSaving;successful_private_saves=$crackSaved;failed_private_saves=0;artifacts=$crackArtifacts;png_crops=@();png_count=0;png_pixels=0;png_rgb_bytes=0;png_complete=$false}
if ($debug) {
    $tunePath=Join-Path $captures 'crack-colour-tune-summary.json'; $tuneCsv=Join-Path $captures 'crack-colour-tune.csv'
    $transcript=Join-Path $captures 'crack-colour-tune-commands.log'; $prefs=Join-Path $captures 'crack-colour-tune-scratch/profile/ColourWars/save_data.json'
    foreach ($path in @($tunePath,$tuneCsv,$transcript,$prefs)) { if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Crack Debug evidence file missing' }; $crackInputs.Add($path) }
    $tune=Get-Content -LiteralPath $tunePath -Raw | ConvertFrom-Json; $tuneRows=@(Import-Csv -LiteralPath $tuneCsv)
    $tuneCounts=[ordered]@{commands=9;states=9;contexts=1;readbacks=36;csv_rows=9;crops=0;saved_tokens=1;private_seed_saves=2;named_token_checks=1;calibration_pixels_per_read=32;calibration_pixel_checks=1152}
    Assert-CrackKeys $tune (@($tuneCounts.Keys)+$crackUpdates+@('scope','event_fade','context','source_models')) 'Debug'
    foreach ($field in $tuneCounts.Keys) { Assert-CrackInteger $tune $field $tuneCounts[$field] }
    Assert-CrackUpdates $tune
    Assert-CrackPairs $tuneRows @('baseline','named','reset-original','rgb','rgba128','alpha0','reset-saved','rgb-after-save','reset-all-saved') @('left')
    $tuneColumns=@('state','context','background','roi_rgba_match','adoption_match','repeat_match','literal_channels','effective_delta','frozen_inputs_resources_rng','calibration_rgba','source_models','private_bytes','adoption','named_token_preserved')
    if (($tuneRows[0].PSObject.Properties.Name -join '|') -cne ($tuneColumns -join '|') -or $tune.scope -cne 'debug-validated-commands' -or $tune.event_fade -isnot [bool] -or $tune.event_fade -ne $false -or $tune.context -cne 'left' -or $tune.source_models -cne $crackModelNames) { throw 'Crack Debug schema/scope differs' }
    foreach ($row in $tuneRows) {
        Assert-CrackFlags $row
        $named=if ($row.state -ceq 'named') {'true'} else {'not-applicable'}
        if ($row.private_bytes -cne 'true' -or $row.adoption -cne 'live' -or $row.named_token_preserved -cne $named) { throw 'Crack Debug private-byte/live/named-token guard differs' }
    }
    if ((Get-Item -LiteralPath $transcript).Length -le 0 -or (Get-Item -LiteralPath $prefs).Length -le 0 -or $crackSaving -ne 2 -or $crackSaved -ne 2) { throw 'Crack Debug transcript/preference seed Save evidence differs' }
    # C++ owns exact one-token source/staged Save/rebase, RED token retention,
    # default preferences/JumpJ, empty queues and full-frame original/saved resets.
    foreach ($path in @($tunePath,$tuneCsv,$transcript,$prefs)) { $crackArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash} }
    $crackObserved.developer_commands=$tune; $crackObserved.readbacks=78; $crackObserved.csv_rows=17; $crackObserved.calibration_pixel_checks=2496
} elseif ($crackSaving -ne 0 -or $crackSaved -ne 0 -or @(Get-ChildItem -LiteralPath $captures -Filter 'crack-colour-tune-*').Count) { throw 'Release Crack proof must remain consumer-only without Debug artifacts or private Saves' }
$result.observed_counts=$crackObserved
$script:crackVerifiedContext=[ordered]@{directory=[IO.Path]::GetFullPath($captures);observed=$crackObserved;ppms=$crackPpmEvidence;
    inputs=@(foreach ($path in $crackInputs) { [ordered]@{path=$path;sha256=(Get-FileHash -LiteralPath $path).Hash} })}
