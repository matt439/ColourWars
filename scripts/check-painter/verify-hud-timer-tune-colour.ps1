# This leaf verifies its own exact schemas before contact export, then verifies
# native PNG bytes afterward. It does not execute code from retained artifacts.
$timerUpdates = @('local_updates','player_updates','level_updates','scene_updates','state_updates','input_updates','time_updates','input_polls','observation_weapon_updates')
$timerStates = @('default','rgb','rgba128','alpha0')
function Get-TimerSha([byte[]]$Bytes) { Get-PainterSha256 $Bytes }
function Assert-TimerKeys($Object,[string[]]$Expected,[string]$Scope) {
    if ($Object -isnot [pscustomobject] -or (($Object.PSObject.Properties.Name | Sort-Object) -join '|') -cne (($Expected | Sort-Object) -join '|')) { throw "Timer exact $Scope schema differs" }
}
function Assert-TimerInteger($Object,[string]$Key,[long]$Expected) {
    $value=$Object.$Key
    if (($value -isnot [int] -and $value -isnot [long]) -or $value -ne $Expected) { throw "Timer exact integer differs: $Key" }
}
function Get-TimerCsvInteger($Row,[string]$Key) {
    $text=[string]$Row.$Key; $value=0L
    if ($text -cnotmatch '^(0|[1-9][0-9]*)$' -or ![long]::TryParse($text,[ref]$value)) { throw "Timer canonical CSV integer differs: $Key" }
    return $value
}
function Assert-TimerCsvColumns($Rows,[string[]]$Expected,[int]$Count,[string]$Scope) {
    if ($Rows.Count -ne $Count -or ($Rows[0].PSObject.Properties.Name -join '|') -cne ($Expected -join '|')) { throw "Timer exact $Scope CSV shape differs" }
}
function Assert-TimerFlags($Row,[string[]]$Flags) {
    foreach ($flag in $Flags) { if ($Row.$flag -cne 'true') { throw "Timer native flag differs: $flag" } }
}
function Complete-HudTimerRoleEvidence {
    param([Parameter(Mandatory)][string]$Directory, [Parameter(Mandatory)]$Context)
    $context=$Context
    if ($null -eq $context -or ![StringComparer]::OrdinalIgnoreCase.Equals([IO.Path]::GetFullPath($Directory),$context.directory)) { throw 'Timer PNG completion directory differs' }
    foreach ($inputFile in $context.inputs) {
        if (!(Test-Path -LiteralPath $inputFile.path -PathType Leaf) -or (Get-FileHash -LiteralPath $inputFile.path).Hash -cne $inputFile.sha256) { throw 'Timer verified input changed before PNG completion' }
    }
    $pngs=@(Get-ChildItem -LiteralPath $Directory -Filter "$($context.prefix)-colour-*.png" -File)
    $expected=@($context.ppms | ForEach-Object { [IO.Path]::ChangeExtension($_.name,'.png') })
    if ($pngs.Count -ne 4 -or (($pngs.Name | Sort-Object) -join '|') -cne (($expected | Sort-Object) -join '|')) { throw 'Timer exact native PNG inventory differs' }
    Add-Type -AssemblyName System.Drawing
    $evidence=@(foreach ($ppm in $context.ppms) {
        $ppmPath=Join-Path $Directory $ppm.name; $ppmBytes=[IO.File]::ReadAllBytes($ppmPath)
        if ((Get-TimerSha $ppmBytes) -cne $ppm.sha256) { throw 'Timer PPM changed before PNG decode' }
        $path=[IO.Path]::ChangeExtension($ppmPath,'.png'); $bytes=[IO.File]::ReadAllBytes($path)
        $signature=@(137,80,78,71,13,10,26,10)
        if ($bytes.Length -lt 33) { throw 'Timer PNG header truncated' }
        for ($i=0; $i -lt 8; ++$i) { if ($bytes[$i] -ne $signature[$i]) { throw 'Timer PNG signature differs' } }
        if (($bytes[8..11] -join ',') -cne '0,0,0,13' -or [Text.Encoding]::ASCII.GetString($bytes,12,4) -cne 'IHDR') { throw 'Timer PNG initial IHDR differs' }
        $width=0L; $height=0L
        for ($i=0; $i -lt 4; ++$i) { $width=$width*256+$bytes[16+$i]; $height=$height*256+$bytes[20+$i] }
        if ($width -ne 131 -or $height -ne 52 -or $bytes[24] -ne 8 -or $bytes[25] -notin @(2,6) -or $bytes[26] -ne 0 -or $bytes[27] -ne 0 -or $bytes[28] -notin @(0,1)) { throw 'Timer PNG native RGB/RGBA IHDR differs' }
        $bitmap=[Drawing.Bitmap]::new($path)
        try {
            if ($bitmap.Width -ne 131 -or $bitmap.Height -ne 52) { throw 'Timer decoded PNG dimensions differ' }
            $rgb=[byte[]]::new(20436)
            for ($y=0; $y -lt 52; ++$y) { for ($x=0; $x -lt 131; ++$x) {
                $pixel=$bitmap.GetPixel($x,$y); $at=($y*131+$x)*3
                if ($pixel.A -ne 255) { throw 'Timer decoded PNG alpha is not opaque' }
                $rgb[$at]=$pixel.R; $rgb[$at+1]=$pixel.G; $rgb[$at+2]=$pixel.B
            } }
        } finally { $bitmap.Dispose() }
        for ($i=0; $i -lt 20436; ++$i) { if ($rgb[$i] -ne $ppmBytes[14+$i]) { throw 'Timer decoded PNG differs from complete retained PPM RGB' } }
        $digest=Get-TimerSha $rgb
        if ($digest -cne $ppm.rgb_payload_sha256) { throw 'Timer decoded PNG RGB fingerprint differs' }
        $metadata=[ordered]@{name=[IO.Path]::GetFileName($path);source_ppm=$ppm.name;scope=$context.png_scope;width=131;height=52;pixels=6812;bit_depth=[int]$bytes[24];colour_type=[int]$bytes[25];ppm_sha256=$ppm.sha256;png_sha256=(Get-TimerSha $bytes);decoded_rgb_sha256=$digest;decoded_rgb_matches_ppm=$true;alpha_opaque=$true}
        $metadata | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath "$path.metadata.json" -Encoding utf8
        $metadata
    })
    $context.observed.png_crops=$evidence
    $context.observed.png_count=4; $context.observed.png_pixels=27248; $context.observed.png_rgb_bytes=81744; $context.observed.png_complete=$true
}

function Read-HudTimerRoleEvidence {
    param([Parameter(Mandatory)][bool]$Shadow)
    $prefix=if ($Shadow) {'hud-timer-shadow'} else {'hud-timer'}
    $subject=if ($Shadow) {'shadow47'} else {'foreground46'}
    $sharedReads=if ($Shadow) {56} else {55}
    $controlCount=if ($Shadow) {4} else {3}
    $baselineToken=if ($Shadow) {'00000022'} else {'141414'}
    $sharedScope=if ($Shadow) {'normal-timer-shadow-glyph-roi-and-branch-invariance'} else {'normal-timer-glyph-roi-and-branch-invariance'}
    $mainScope=if ($Shadow) {'normal-timer-shadow-glyph-roi'} else {'normal-timer-glyph-roi'}
    $branchScope=if ($Shadow) {'public-hud-shadow-branch-counterexample'} else {'public-hud-branch-counterexample'}
    $debugScope=if ($Shadow) {'debug-live-normal-timer-shadow-and-closing-invariance'} else {'debug-live-normal-timer-and-closing-invariance'}
    $debugNormalScope=if ($Shadow) {'debug-live-normal-timer-shadow'} else {'debug-live-normal-timer'}
    $debugBranchScope=if ($Shadow) {'debug-shadow-closing-counterexample'} else {'debug-closing-counterexample'}
    $pngScope=if ($Shadow) {'normal-timer-shadow-ordered-glyph-paper-ROI'} else {'normal-timer-glyph-shadow-paper-ROI'}
    $timerInputs=[Collections.Generic.List[string]]::new()
    $timerSummaryPath=Join-Path $captures ($prefix+'-colour-consumer-summary.json')
    $timerMainPath=Join-Path $captures ($prefix+'-colour-main.csv')
    $timerBranchPath=Join-Path $captures ($prefix+'-colour-branches.csv')
    foreach ($path in @($timerSummaryPath,$timerMainPath,$timerBranchPath,(Join-Path $run 'run.log'),(Join-Path $run 'run.err'))) {
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Timer required evidence file missing' }
        $timerInputs.Add($path)
    }
    if (@(Get-ChildItem -LiteralPath $captures -Filter "$prefix-failure-*").Count) { throw 'Timer native failure artifacts remain' }
    $timerSummary=Get-Content -LiteralPath $timerSummaryPath -Raw | ConvertFrom-Json
    $timerMain=@(Import-Csv -LiteralPath $timerMainPath); $timerBranches=@(Import-Csv -LiteralPath $timerBranchPath)
    $timerCounts=[ordered]@{readbacks=$sharedReads;csv_rows=18;main_rows=4;branch_rows=14;crops=4;crop_width=131;crop_height=52;crop_pixels=27248;ppm_bytes=81800;roi_pixels=6812;independent_roi_observations=12;roi_pixel_checks=81744;repeat_checks=18;copied_adoption_checks=4;closing_colour_invariance_checks=3;reduce_effects_checks=6;controls=$controlCount;calibration_pixel_checks=($sharedReads*32);private_game_saves=0}
    Assert-TimerKeys $timerSummary (@($timerCounts.Keys)+$timerUpdates+@('scope','control_discriminating_pixels')) 'shared'
    foreach ($key in $timerCounts.Keys) { Assert-TimerInteger $timerSummary $key $timerCounts[$key] }
    foreach ($key in $timerUpdates) { Assert-TimerInteger $timerSummary $key 0 }
    $timerControlPixels=$timerSummary.control_discriminating_pixels
    if (($timerControlPixels -isnot [int] -and $timerControlPixels -isnot [long]) -or $timerControlPixels -le 0 -or $timerControlPixels -gt ($controlCount*6812) -or $timerSummary.scope -cne $sharedScope) { throw 'Timer shared scope or observed control support differs' }
    $timerMainColumns=@('state','raw_rgba_match','adoption_match','repeat_match','locality_match','frozen_style_resources_rng','controls','discriminating_pixels','scope')
    Assert-TimerCsvColumns $timerMain $timerMainColumns 4 'main'
    if (($timerMain.state -join '|') -cne ($timerStates -join '|')) { throw 'Timer main state order differs' }
    $timerSumPixels=0L
    foreach ($row in $timerMain) {
        Assert-TimerFlags $row @('raw_rgba_match','adoption_match','repeat_match','locality_match','frozen_style_resources_rng')
        $controls=switch ($row.state) { rgba128 {if ($Shadow) {3} else {2}} alpha0 {1} default {0} }
        $observed=Get-TimerCsvInteger $row 'discriminating_pixels'
        if ((Get-TimerCsvInteger $row 'controls') -ne $controls -or $observed -gt $controls*6812 -or ($controls -gt 0 -and $observed -le 0) -or $row.scope -cne $mainScope) { throw 'Timer main observed tint-control accounting differs' }
        $timerSumPixels += $observed
    }
    if ($timerSumPixels -ne $timerControlPixels) { throw 'Timer main control pixels do not sum to the native summary' }
    $timerBranchColumns=@('state','context','normal_branch','raw_rgba_match','repeat_match','selection_guard','frozen_style_resources_rng','scope')
    Assert-TimerCsvColumns $timerBranches $timerBranchColumns 14 'branch'
    $timerBranchNames=@('above-ten','exact-ten','fractional-closing','zero','stopped-short','reduced-normal','reduced-closing')
    $timerNormalContexts=@('above-ten','zero','stopped-short','reduced-normal')
    for ($i=0; $i -lt 14; ++$i) {
        $row=$timerBranches[$i]; $state=if ($i -lt 7) {'default'} else {'rgba128'}; $context=$timerBranchNames[$i%7]
        $normal=$context -cin $timerNormalContexts; $normalText=if ($normal) {'true'} else {'false'}
        $raw=if ($normal) {'true'} else {'not-run-closing-invariance-only'}
        $guard=if ($state -ceq 'default') {'baseline'} elseif ($normal) {'normal-locality'} else {'closing-exact'}
        Assert-TimerFlags $row @('repeat_match','frozen_style_resources_rng')
        if ($row.state -cne $state -or $row.context -cne $context -or $row.normal_branch -cne $normalText -or $row.raw_rgba_match -cne $raw -or $row.selection_guard -cne $guard -or $row.scope -cne $branchScope) { throw 'Timer exact public branch observation differs' }
    }
    $timerPpms=@(Get-ChildItem -LiteralPath $captures -Filter "$prefix-colour-*.ppm" -File)
    $timerExpectedNames=@($timerStates | ForEach-Object { "$prefix-colour-$_.ppm" })
    if ($timerPpms.Count -ne 4 -or (($timerPpms.Name | Sort-Object) -join '|') -cne (($timerExpectedNames | Sort-Object) -join '|')) { throw 'Timer exact native PPM inventory differs' }
    $timerHeader=[Text.Encoding]::ASCII.GetBytes("P6`n131 52`n255`n")
    $timerPpmEvidence=@(foreach ($name in $timerExpectedNames) {
        $path=Join-Path $captures $name; $bytes=[IO.File]::ReadAllBytes($path)
        if ($bytes.Length -ne 20450) { throw 'Timer canonical native P6 byte length differs' }
        for ($i=0; $i -lt 14; ++$i) { if ($bytes[$i] -ne $timerHeader[$i]) { throw 'Timer canonical native P6 header differs' } }
        $rgb=[byte[]]::new(20436); [Array]::Copy($bytes,14,$rgb,0,20436); $timerInputs.Add($path)
        [ordered]@{name=$name;bytes=20450;sha256=(Get-TimerSha $bytes);rgb_payload_sha256=(Get-TimerSha $rgb)}
    })
    $timerLog=@(Get-Content -LiteralPath (Join-Path $run 'run.log'))
    if (@($timerLog | Where-Object { $_ -match 'Saving\.\.\.$' -or $_ -ceq 'Save was successful.' }).Count -or
        @(Get-Content -LiteralPath (Join-Path $run 'run.err') | Where-Object { $_ -match '^save failure:' }).Count) { throw 'Timer scope unexpectedly called or failed a game Save' }
    $timerObserved=[ordered]@{consumer=$timerSummary;readbacks=$sharedReads;csv_rows=18;crops=4;crop_pixels=27248;ppm_bytes=81800;controls=$controlCount;control_discriminating_pixels=$timerControlPixels;calibration_pixel_checks=($sharedReads*32);private_game_saves=0;reference_scope='normal glyph/shadow/pure-paper ROI independent; copied full HUD adoption only; closing invariance only';png_count=0;png_crops=@();png_complete=$false}
    if ($debug) {
        $timerTunePath=Join-Path $captures ($prefix+'-tune-summary.json'); $timerTuneCsv=Join-Path $captures ($prefix+'-tune.csv')
        $timerTuneBranches=Join-Path $captures ($prefix+'-tune-branches.csv'); $timerTranscript=Join-Path $captures ($prefix+'-tune-commands.log')
        foreach ($path in @($timerTunePath,$timerTuneCsv,$timerTuneBranches,$timerTranscript)) {
            if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Timer required Debug evidence missing' }
            $timerInputs.Add($path)
        }
        $tune=Get-Content -LiteralPath $timerTunePath -Raw | ConvertFrom-Json
        $tuneRows=@(Import-Csv -LiteralPath $timerTuneCsv); $tuneBranchRows=@(Import-Csv -LiteralPath $timerTuneBranches)
        $tuneCounts=[ordered]@{commands=8;states=8;readbacks=34;csv_rows=10;normal_rows=8;branch_rows=2;crops=0;saved_tokens=1;presentation_save_commands=1;private_game_saves=0;roi_pixel_checks=54496;repeat_checks=8;copied_adoption_checks=8;closing_colour_invariance_checks=1;calibration_pixel_checks=1088}
        Assert-TimerKeys $tune (@($tuneCounts.Keys)+$timerUpdates+@('scope')) 'Debug'
        foreach ($key in $tuneCounts.Keys) { Assert-TimerInteger $tune $key $tuneCounts[$key] }
        foreach ($key in $timerUpdates) { Assert-TimerInteger $tune $key 0 }
        if ($tune.scope -cne $debugScope) { throw 'Timer Debug scope differs' }
        $tuneColumns=@('state','raw_rgba_match','adoption_match','repeat_match','private_bytes','frozen_style_resources_rng','token','scope')
        Assert-TimerCsvColumns $tuneRows $tuneColumns 8 'Debug normal'
        $states=@('baseline','named','reset-original','partial','zero','reset-saved','rgb','final-saved')
        $tokens=@($baselineToken,'RED',$baselineToken,'2060C080','A532D600','2060C080','35BCE1','2060C080')
        for ($i=0; $i -lt 8; ++$i) {
            $row=$tuneRows[$i]; Assert-TimerFlags $row @('raw_rgba_match','adoption_match','repeat_match','private_bytes','frozen_style_resources_rng')
            if ($row.state -cne $states[$i] -or $row.token -cne $tokens[$i] -or $row.scope -cne $debugNormalScope) { throw 'Timer Debug exact normal state/token observation differs' }
        }
        Assert-TimerCsvColumns $tuneBranchRows @('state','context','closing_unchanged','frozen_style_resources_rng','scope') 2 'Debug closing'
        for ($i=0; $i -lt 2; ++$i) {
            $row=$tuneBranchRows[$i]; $state=if ($i -eq 0) {'baseline'} else {'partial'}; $equal=if ($i -eq 0) {'baseline'} else {'true'}
            if ($row.state -cne $state -or $row.context -cne 'exact-ten' -or $row.closing_unchanged -cne $equal -or $row.frozen_style_resources_rng -cne 'true' -or $row.scope -cne $debugBranchScope) { throw 'Timer Debug exact closing observation differs' }
        }
        if ((Get-Item -LiteralPath $timerTranscript).Length -le 0 -or (Test-Path -LiteralPath (Join-Path $captures ($prefix+'-tune-scratch/profile/ColourWars/save_data.json')))) { throw 'Timer transcript/private game Save invariant differs' }
        foreach ($name in @('source/tuning.json','source/presentation.json','staged/tuning.json','staged/presentation.json')) { $timerInputs.Add((Join-Path $captures "$prefix-tune-scratch/$name")) }
        $timerObserved.developer_commands=$tune; $timerObserved.readbacks=($sharedReads+34); $timerObserved.csv_rows=28; $timerObserved.calibration_pixel_checks=($sharedReads*32+1088)
    } elseif (@(Get-ChildItem -LiteralPath $captures -Filter "$prefix-tune-*").Count) { throw 'Release timer proof must have no Debug artifacts' }
    return [ordered]@{subject=$subject;prefix=$prefix;png_scope=$pngScope;directory=[IO.Path]::GetFullPath($captures);observed=$timerObserved;ppms=$timerPpmEvidence;inputs=@(foreach ($path in $timerInputs) {[ordered]@{path=$path;sha256=(Get-FileHash -LiteralPath $path).Hash}})}
}

. (Join-Path $PSScriptRoot 'verify-hud-closing-text-colour.ps1')
$script:timerVerifiedContexts=@(
    Read-HudTimerRoleEvidence -Shadow $false
    Read-HudTimerRoleEvidence -Shadow $true
    Read-HudClosingTextEvidence
)
$foreground=$script:timerVerifiedContexts[0].observed
$shadow=$script:timerVerifiedContexts[1].observed
$closing=$script:timerVerifiedContexts[2].observed
$script:timerFamilyObserved=[ordered]@{
    foreground46=$foreground;shadow47=$shadow;closing_text48=$closing
    readbacks=($foreground.readbacks+$shadow.readbacks+$closing.readbacks);csv_rows=($foreground.csv_rows+$shadow.csv_rows+$closing.csv_rows)
    crops=12;crop_pixels=81744;ppm_bytes=245400;controls=9
    control_discriminating_pixels=($foreground.control_discriminating_pixels+$shadow.control_discriminating_pixels+$closing.control_discriminating_pixels)
    calibration_pixel_checks=($foreground.calibration_pixel_checks+$shadow.calibration_pixel_checks+$closing.calibration_pixel_checks)
    private_game_saves=0;reference_scope='normal foreground46/shadow47 plus opaque closing text48; explicit role applicability; closing reference omits normal shadow; copied HUD adoption only'
    png_count=0;png_crops=@();png_pixels=0;png_rgb_bytes=0;png_complete=$false
}
$result.observed_counts=$script:timerFamilyObserved

function Complete-HudTimerColourEvidence {
    param([Parameter(Mandatory)][string]$Directory)
    foreach ($context in $script:timerVerifiedContexts) {
        Complete-HudTimerRoleEvidence -Directory $Directory -Context $context
    }
    $foreground=$script:timerVerifiedContexts[0].observed
    $shadow=$script:timerVerifiedContexts[1].observed
    $closing=$script:timerVerifiedContexts[2].observed
    if (!$foreground.png_complete -or !$shadow.png_complete -or !$closing.png_complete) { throw 'All three timer roles must finish PNG verification' }
    $script:timerFamilyObserved.png_crops=@($foreground.png_crops)+@($shadow.png_crops)+@($closing.png_crops)
    $script:timerFamilyObserved.png_count=12;$script:timerFamilyObserved.png_pixels=81744
    $script:timerFamilyObserved.png_rgb_bytes=223992;$script:timerFamilyObserved.png_complete=$true
}
