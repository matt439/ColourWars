# Initial dot-source checks native schemas and retained capture bytes before PNG creation.
# Design205D9327 / contract69BBC21A / geometry2CBD2C63: v2 corrects four saturated-WHITE witnesses only.
# C++ owns complete actual/raw21142RGBA across12literal procedural ROIs, controls,
# calibration and private commands. Full1280x480 crops are adoption captures only.
# Partial-alpha values are NOT predicted analytically; no source-model machinery.
$limbStates=@('default','rgb','rgba128','alpha0')
$limbUpdates=@('local_updates','player_updates','level_updates','scene_updates','state_updates','input_updates','time_updates','input_polls','observation_weapon_updates')
$limbFlags=@('roi_rgba_match','adoption_match','repeat_match','opaque_alpha','active_lids','quiet_disabled_blank','captured_layers_unchanged','delta_support','frozen_resources_inputs_rng')
function Get-LimbSha([byte[]]$Bytes){[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($Bytes))}
function Assert-LimbKeys($Object,[string[]]$Keys,[string]$Scope){
    if($Object-isnot[pscustomobject]-or(($Object.PSObject.Properties.Name|Sort-Object)-join'|')-cne(($Keys|Sort-Object)-join'|')){throw "Limb exact $Scope JSON schema differs"}
}
function Assert-LimbInteger($Object,[string]$Key,[long]$Expected){
    $v=$Object.$Key;if(($v-isnot[int]-and$v-isnot[long])-or$v-ne$Expected){throw "Limb exact integer differs: $Key"}
}
function Get-LimbDiagnostic($Object,[string]$Key){
    $v=$Object.$Key;if(($v-isnot[int]-and$v-isnot[long])-or$v-lt0){throw "Limb nonnegative observed integer differs: $Key"};return [long]$v
}
function Get-LimbCsvCount($Row,[string]$Key){
    $v=[string]$Row.$Key;$parsed=0L;if($v-cnotmatch'^(0|[1-9][0-9]*)$'-or![long]::TryParse($v,[ref]$parsed)){throw "Limb canonical CSV count differs: $Key"};return $parsed
}
function Assert-LimbUpdates($Object){foreach($key in $limbUpdates){Assert-LimbInteger $Object $key 0}}
function Assert-LimbStates($Rows,[string[]]$Names){
    if($Rows.Count-ne$Names.Count-or(($Rows.state|Sort-Object)-join'|')-cne(($Names|Sort-Object)-join'|')){throw 'Limb exact state inventory differs'}
}
function Assert-LimbFlags($Row){
    if($Row.background-cne'canvas32-64-96'){throw 'Limb Canvas differs'}
    foreach($flag in $limbFlags){if($Row.$flag-cne'true'){throw "Limb native flag differs: $flag"}}
}

function Complete-LimbTuneColourEvidence {
    param([Parameter(Mandatory)][string]$Directory)
    $context = $script:limbVerifiedContext
    if ($null -eq $context -or ![StringComparer]::OrdinalIgnoreCase.Equals([IO.Path]::GetFullPath($Directory), $context.directory)) { throw 'Limb PNG completion directory differs from initial verified evidence' }
    foreach ($inputFile in $context.inputs) {
        if (!(Test-Path -LiteralPath $inputFile.path -PathType Leaf) -or (Get-FileHash -LiteralPath $inputFile.path).Hash -cne $inputFile.sha256) { throw 'Limb verified input changed before PNG completion' }
    }
    $pngs = @(Get-ChildItem -LiteralPath $Directory -Filter 'limb-colour-*.png' -File | Sort-Object Name)
    $expected = @($context.ppms | ForEach-Object { [IO.Path]::ChangeExtension($_.name, '.png') })
    if ($pngs.Count -ne 4 -or (($pngs.Name | Sort-Object) -join '|') -cne (($expected | Sort-Object) -join '|')) { throw 'Limb exact PNG inventory differs' }
    Add-Type -AssemblyName System.Drawing
    $pngEvidence = @(foreach ($ppm in $context.ppms) {
        $ppmPath = Join-Path $Directory $ppm.name
        if (!(Test-Path -LiteralPath $ppmPath -PathType Leaf)) { throw 'Limb PPM changed between initial validation and PNG completion' }
        $ppmBytes = [IO.File]::ReadAllBytes($ppmPath)
        if ((Get-LimbSha $ppmBytes) -cne $ppm.sha256) { throw 'Limb PPM changed between initial validation and PNG completion' }
        $path = [IO.Path]::ChangeExtension($ppmPath, '.png'); $bytes = [IO.File]::ReadAllBytes($path)
        $signature = @(137,80,78,71,13,10,26,10)
        if ($bytes.Length -lt 33) { throw 'Limb PNG header truncated' }
        for ($index = 0; $index -lt 8; ++$index) { if ($bytes[$index] -ne $signature[$index]) { throw 'Limb PNG signature differs' } }
        if (($bytes[8..11] -join ',') -cne '0,0,0,13' -or [Text.Encoding]::ASCII.GetString($bytes,12,4) -cne 'IHDR') { throw 'Limb PNG initial IHDR differs' }
        $width = 0L; $height = 0L
        for ($index = 0; $index -lt 4; ++$index) { $width = $width * 256 + $bytes[16+$index]; $height = $height * 256 + $bytes[20+$index] }
        if ($width -ne 1280 -or $height -ne 480 -or $bytes[24] -ne 8 -or $bytes[25] -notin @(2,6) -or $bytes[26] -ne 0 -or $bytes[27] -ne 0 -or $bytes[28] -notin @(0,1)) { throw 'Limb PNG native dimensions/8bit RGB-RGBA IHDR differs' }
        try { $bitmap = [Drawing.Bitmap]::new($path) } catch { throw 'Limb PNG decode failed' }
        try {
            if ($bitmap.Width -ne 1280 -or $bitmap.Height -ne 480) { throw 'Limb decoded PNG native dimensions differ' }
            $rgb = [byte[]]::new(1843200)
            for ($y = 0; $y -lt 480; ++$y) { for ($x = 0; $x -lt 1280; ++$x) {
                $pixel = $bitmap.GetPixel($x,$y); $at = ($y*1280+$x)*3
                if ($pixel.A -ne 255) { throw 'Limb PNG decoded alpha is not opaque' }
                $rgb[$at]=$pixel.R; $rgb[$at+1]=$pixel.G; $rgb[$at+2]=$pixel.B
            } }
        } finally { $bitmap.Dispose() }
        for ($index = 0; $index -lt 1843200; ++$index) { if ($rgb[$index] -ne $ppmBytes[16+$index]) { throw 'Limb PNG decoded RGB differs from retained PPM' } }
        $digest = Get-LimbSha $rgb
        if ($digest -cne $ppm.rgb_payload_sha256) { throw 'Limb PNG complete RGB retained-PPM digest differs' }
        [ordered]@{name=[IO.Path]::GetFileName($path);width=1280;height=480;pixels=614400;bit_depth=[int]$bytes[24];colour_type=[int]$bytes[25];png_bytes=$bytes.Length;png_sha256=(Get-LimbSha $bytes);ppm_sha256=$ppm.sha256;decoded_rgb_sha256=$digest;decoded_rgb_matches_ppm=$true;alpha_opaque=$true;reference_scope='procedural-ROIs-independent/full-crop-adoption-only'}
    })
    $context.observed.png_crops = $pngEvidence
    $context.observed.png_count = 4
    $context.observed.png_pixels = 2457600
    $context.observed.png_rgb_bytes = 7372800
    $context.observed.png_complete = $true
}

$limbSummaryPath=Join-Path $captures 'limb-colour-consumer-summary.json'
$limbCsvPath=Join-Path $captures 'limb-colour-consumers.csv'
$limbInputs=[Collections.Generic.List[string]]::new()
foreach($path in @($limbSummaryPath,$limbCsvPath,(Join-Path $run 'run.log'),(Join-Path $run 'run.err'))){
    if(!(Test-Path -LiteralPath $path -PathType Leaf)){throw 'Limb required evidence file missing'};$limbInputs.Add($path)
}
if(@(Get-ChildItem -LiteralPath $captures -Filter 'limb-failure-*').Count){throw 'Limb native failure artifacts remain'}
$limbSummary=Get-Content -LiteralPath $limbSummaryPath -Raw|ConvertFrom-Json
$limbRows=@(Import-Csv -LiteralPath $limbCsvPath)
$limbCounts=[ordered]@{readbacks=22;csv_rows=4;crops=4;crop_width=1280;crop_height=480;crop_pixels=2457600;ppm_bytes=7372864;palettes=4;armed_contexts=10;blink_contexts=2;
    quiet_disabled_contexts=2;captured_sprite_guards=4;roi_pixels=21142;literal_rectangles=396;base_case_reads=16;actual_reads=8;adoption_reads=4;raw_positive_reads=4;controls=6;
    partial_arm_straight_reads=1;partial_arm_squared_reads=1;partial_hand_straight_reads=1;partial_hand_squared_reads=1;alpha0_arm_straight_reads=1;alpha0_hand_straight_reads=1;
    control_witness_checks=252;control_discriminating_witness_checks=248;control_saturated_white_checks=4;calibration_pixels_per_read=32;calibration_pixel_checks=704;preparation_weapon_updates=10;preparation_weapon_dt=0;preparation_projectiles=0;
    arm_support_pixels=7232;hand_support_pixels=1280;arm_witnesses=64;hand_witnesses=20}
Assert-LimbKeys $limbSummary (@($limbCounts.Keys)+$limbUpdates+@('control_changed_pixels','control_unchanged_pixels','scope','raw_reference','whole_frame_reference','capture_kind','event_fade','background','viewport')) 'shared'
foreach($key in $limbCounts.Keys){Assert-LimbInteger $limbSummary $key $limbCounts[$key]};Assert-LimbUpdates $limbSummary
if($limbSummary.scope-cne'direct-consumer-only'-or$limbSummary.raw_reference-cne'12-literal-procedural-ROIs-engine-only'-or$limbSummary.whole_frame_reference-cne'copied-retained-fixture-adoption-only'-or
    $limbSummary.capture_kind-cne'native-contiguous-fixture-crop'-or$limbSummary.background-cne'canvas32-64-96'-or$limbSummary.viewport-cne'1280x720'-or$limbSummary.event_fade-isnot[bool]-or$limbSummary.event_fade-ne$false){throw 'Limb direct fixture/oracle scope differs'}
$limbChanged=Get-LimbDiagnostic $limbSummary 'control_changed_pixels';$limbUnchanged=Get-LimbDiagnostic $limbSummary 'control_unchanged_pixels'
if($limbChanged-le0-or$limbChanged-gt25536-or$limbUnchanged-lt101316-or$limbChanged+$limbUnchanged-ne126852){throw 'Limb observed control diagnostics exceed source support/accounting bounds'}
Assert-LimbStates $limbRows $limbStates
$limbColumns=@('state','background','roi_rgba_match','adoption_match','repeat_match','opaque_alpha','active_lids','quiet_disabled_blank','captured_layers_unchanged','delta_support','frozen_resources_inputs_rng','controls','control_witness_checks','control_discriminating_witness_checks','control_saturated_white_checks','control_changed_pixels','control_unchanged_pixels','scope')
if(($limbRows[0].PSObject.Properties.Name-join'|')-cne($limbColumns-join'|')){throw 'Limb shared CSV columns differ'}
$limbSumChanged=0L;$limbSumUnchanged=0L;$limbSumControls=0;$limbSumWitnesses=0;$limbSumDiscriminating=0L;$limbSumSaturated=0L
foreach($row in $limbRows){
    Assert-LimbFlags $row
    $controls=switch($row.state){rgba128{4}alpha0{2}default{0}};$witnesses=switch($row.state){rgba128{168}alpha0{84}default{0}}
    $discriminating=switch($row.state){rgba128{168}alpha0{80}default{0}};$saturated=switch($row.state){alpha0{4}default{0}}
    $observedDiscriminating=Get-LimbCsvCount $row 'control_discriminating_witness_checks';$observedSaturated=Get-LimbCsvCount $row 'control_saturated_white_checks'
    $maximum=switch($row.state){rgba128{17024}alpha0{8512}default{0}}
    $changed=Get-LimbCsvCount $row 'control_changed_pixels';$unchanged=Get-LimbCsvCount $row 'control_unchanged_pixels'
    if($row.controls-cne[string]$controls-or$row.control_witness_checks-cne[string]$witnesses-or$observedDiscriminating-ne$discriminating-or$observedSaturated-ne$saturated-or$observedDiscriminating+$observedSaturated-ne$witnesses-or$row.scope-cne'direct-consumer-only'-or
       $changed+$unchanged-ne$controls*21142-or$changed-gt$maximum-or($controls-gt0-and$changed-le0)){throw 'Limb per-case observed control support/witness accounting differs'}
    $limbSumChanged+=$changed;$limbSumUnchanged+=$unchanged;$limbSumControls+=$controls;$limbSumWitnesses+=$witnesses;$limbSumDiscriminating+=$observedDiscriminating;$limbSumSaturated+=$observedSaturated
}
if($limbSumChanged-ne$limbChanged-or$limbSumUnchanged-ne$limbUnchanged-or$limbSumControls-ne6-or$limbSumWitnesses-ne252-or$limbSumDiscriminating-ne248-or$limbSumSaturated-ne4){throw 'Limb CSV observed diagnostics do not sum to native summary'}
$limbPpms=@(Get-ChildItem -LiteralPath $captures -Filter 'limb-colour-*.ppm' -File)
$limbExpectedNames=@($limbStates|ForEach-Object{"limb-colour-$_.ppm"})
if($limbPpms.Count-ne4-or(($limbPpms.Name|Sort-Object)-join'|')-cne(($limbExpectedNames|Sort-Object)-join'|')){throw 'Limb exact native PPM inventory differs'}
$limbHeader=[Text.Encoding]::ASCII.GetBytes("P6`n1280 480`n255`n")
$limbPpmEvidence=@(foreach($state in $limbStates){
    $name="limb-colour-$state.ppm";$path=Join-Path $captures $name;$bytes=[IO.File]::ReadAllBytes($path)
    if($bytes.Length-ne1843216){throw 'Limb native P6 byte length differs'}
    for($index=0;$index-lt16;++$index){if($bytes[$index]-ne$limbHeader[$index]){throw 'Limb canonical P6 header differs'}}
    $rgb=[byte[]]::new(1843200);[Array]::Copy($bytes,16,$rgb,0,1843200)
    # Full crop bytes record capture/export provenance. They are not an
    # independent full-character/source-payload oracle or precision model.
    [ordered]@{name=$name;state=$state;width=1280;height=480;pixels=614400;bytes=1843216;sha256=(Get-LimbSha $bytes);rgb_payload_sha256=(Get-LimbSha $rgb);reference_scope='procedural-ROIs-independent/full-crop-adoption-only'}
})
$limbLog=@(Get-Content -LiteralPath (Join-Path $run 'run.log'));$limbSaving=@($limbLog|Where-Object{$_-match'Saving\.\.\.$'}).Count;$limbSaved=@($limbLog|Where-Object{$_-ceq'Save was successful.'}).Count
if(@(Get-Content -LiteralPath (Join-Path $run 'run.err')|Where-Object{$_-match'^save failure:'}).Count){throw 'Limb unexpected private Save failure'}
$limbArtifacts=[ordered]@{};foreach($path in @($limbSummaryPath,$limbCsvPath)){$limbArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}};$limbArtifacts.crops=$limbPpmEvidence
$limbObserved=[ordered]@{consumer=$limbSummary;readbacks=22;csv_rows=4;crops=4;controls=6;control_witness_checks=252;control_discriminating_witness_checks=248;control_saturated_white_checks=4;control_changed_pixels=$limbChanged;control_unchanged_pixels=$limbUnchanged;
    crop_pixels=2457600;ppm_bytes=7372864;independent_RGBA_pixels_per_case=21142;reference_scope='procedural-ROIs-independent/full-crop-adoption-only';control_scope='C++ frozen field supports/witnesses with observed diagnostic counts, not an analytical precision oracle';
    preparation_weapon_updates=10;preparation_weapon_dt=0;preparation_projectiles=0;preparation_positive_time_advances=0;local_updates=0;player_updates=0;level_updates=0;scene_updates=0;state_updates=0;input_updates=0;time_updates=0;input_polls=0;observation_weapon_updates=0;
    calibration_pixels_per_read=32;calibration_pixel_checks=704;private_save_calls=$limbSaving;successful_private_saves=$limbSaved;failed_private_saves=0;artifacts=$limbArtifacts;png_crops=@();png_count=0;png_pixels=0;png_rgb_bytes=0;png_complete=$false}
if($debug){
    $tunePath=Join-Path $captures 'limb-colour-tune-summary.json';$tuneCsv=Join-Path $captures 'limb-colour-tune.csv';$transcript=Join-Path $captures 'limb-colour-tune-commands.log';$prefs=Join-Path $captures 'limb-colour-tune-scratch/profile/ColourWars/save_data.json'
    foreach($path in @($tunePath,$tuneCsv,$transcript,$prefs)){if(!(Test-Path -LiteralPath $path -PathType Leaf)){throw 'Limb Debug required evidence missing'};$limbInputs.Add($path)}
    $tune=Get-Content -LiteralPath $tunePath -Raw|ConvertFrom-Json;$tuneRows=@(Import-Csv -LiteralPath $tuneCsv)
    $tuneCounts=[ordered]@{commands=9;states=9;contexts=12;readbacks=36;csv_rows=9;crops=0;saved_tokens=2;private_seed_saves=2;named_arm_token_checks=1;calibration_pixels_per_read=32;calibration_pixel_checks=1152;preparation_weapon_updates=10;preparation_weapon_dt=0;preparation_projectiles=0}
    Assert-LimbKeys $tune (@($tuneCounts.Keys)+$limbUpdates+@('scope','event_fade','background','adoption')) 'Debug'
    foreach($key in $tuneCounts.Keys){Assert-LimbInteger $tune $key $tuneCounts[$key]};Assert-LimbUpdates $tune
    $expectedStates=@('baseline','named-arm','reset-original','partial-arm','partial-both','zero-arm','zero-both','reset-arm-saved','reset-hand-saved')
    $armTokens=@('BLACK','RED','BLACK','2060C080','2060C080','A532D600','A532D600','2060C080','2060C080')
    $handTokens=@('WHITE','WHITE','WHITE','WHITE','C0406080','C0406080','D65A3200','D65A3200','C0406080')
    Assert-LimbStates $tuneRows $expectedStates
    $tuneColumns=@('state','background','roi_rgba_match','adoption_match','repeat_match','opaque_alpha','active_lids','quiet_disabled_blank','captured_layers_unchanged','delta_support','frozen_resources_inputs_rng','private_bytes','adoption','named_arm_token_preserved','arm_token','hand_token')
    if(($tuneRows[0].PSObject.Properties.Name-join'|')-cne($tuneColumns-join'|')-or$tune.scope-cne'debug-validated-commands'-or$tune.event_fade-isnot[bool]-or$tune.event_fade-ne$false-or$tune.background-cne'canvas32-64-96'-or$tune.adoption-cne'live'){throw 'Limb Debug exact schema/scope differs'}
    foreach($row in $tuneRows){Assert-LimbFlags $row;$index=[Array]::IndexOf($expectedStates,[string]$row.state);$named=if($row.state-ceq'named-arm'){'true'}else{'not-applicable'}
        if($row.private_bytes-cne'true'-or$row.adoption-cne'live'-or$row.named_arm_token_preserved-cne$named-or$row.arm_token-cne$armTokens[$index]-or$row.hand_token-cne$handTokens[$index]){throw 'Limb Debug pair token/private/sibling-reset guard differs'}
    }
    if((Get-Item -LiteralPath $transcript).Length-le0-or(Get-Item -LiteralPath $prefs).Length-le0-or$limbSaving-ne2-or$limbSaved-ne2){throw 'Limb Debug transcript/preference seed Save evidence differs'}
    # Native owns exact two-token private source/staging Save/rebase, JumpJ and
    # preference defaults, queued work and actual full-frame saved resets.
    foreach($path in @($tunePath,$tuneCsv,$transcript,$prefs)){$limbArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}}
    $limbObserved.developer_commands=$tune;$limbObserved.readbacks=58;$limbObserved.csv_rows=13;$limbObserved.calibration_pixel_checks=1856;$limbObserved.preparation_weapon_updates=20
}elseif($limbSaving-ne0-or$limbSaved-ne0-or@(Get-ChildItem -LiteralPath $captures -Filter 'limb-colour-tune-*').Count){throw 'Release Limb proof must remain consumer-only without Debug artifacts/Saves'}
$result.observed_counts=$limbObserved
$script:limbVerifiedContext=[ordered]@{directory=[IO.Path]::GetFullPath($captures);observed=$limbObserved;ppms=$limbPpmEvidence;inputs=@(foreach($path in $limbInputs){[ordered]@{path=$path;sha256=(Get-FileHash -LiteralPath $path).Hash}})}
