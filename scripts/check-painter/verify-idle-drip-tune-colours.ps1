# Dot-sourced into the scope of scripts/check-painter.ps1. The guard
# if ($IdleDripTuneColoursOnly) stays there; this is the leaf's post-run evidence check.

$idleDripConsumerSummaryPath = Join-Path $captures 'idle-drip-colour-consumer-summary.json'
$idleDripConsumerRowsPath = Join-Path $captures 'idle-drip-colour-consumers.csv'
$idleDripGuardRowsPath = Join-Path $captures 'idle-drip-colour-guards.csv'
$idleDripConsumerSummary = Get-Content -LiteralPath $idleDripConsumerSummaryPath -Raw | ConvertFrom-Json
$idleDripConsumerRows = @(Import-Csv -LiteralPath $idleDripConsumerRowsPath)
$idleDripGuardRows = @(Import-Csv -LiteralPath $idleDripGuardRowsPath)
$idleDripCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'idle-drip-*.ppm' -File | Sort-Object Name)
$idleDripExpectedColumns = @('palette', 'background', 'normal_match', 'captured_paint',
    'all_profiles_public_state_rng', 'control', 'scope')
$idleDripExpectedNames = @(foreach ($palette in @('default', 'rgb', 'rgba128', 'alpha0')) {
    foreach ($background in @('black', 'nonblack')) { "idle-drip-colour-$palette-$background.ppm" }
})
$idleDripExpectedPairs = @(foreach ($palette in @('default', 'rgb', 'rgba128', 'alpha0')) {
    foreach ($background in @('black', 'nonblack')) { "$palette/$background" }
})
$idleDripObservedPairs = @($idleDripConsumerRows | ForEach-Object { "$($_.palette)/$($_.background)" })
if ($idleDripConsumerSummary.scope -cne 'consumer-only' -or $idleDripConsumerSummary.state_updates -ne 0 -or
    $idleDripConsumerSummary.level_updates -ne 0 -or $idleDripConsumerSummary.effect_preparation_updates -ne 15 -or
    $idleDripConsumerSummary.readbacks -ne 46 -or $idleDripConsumerSummary.csv_rows -ne 10 -or
    $idleDripConsumerSummary.crops -ne 8 -or $idleDripConsumerSummary.controls -ne 8 -or
    $idleDripConsumerSummary.colour_rows -ne 8 -or $idleDripConsumerSummary.guard_rows -ne 2 -or
    $idleDripConsumerSummary.guard_checks -ne 6 -or $idleDripConsumerSummary.profiles -ne 5 -or
    $idleDripConsumerSummary.consumers -ne 6 -or
    $idleDripConsumerSummary.private_sequence_scope -cne 'source-constness-only' -or
    $idleDripConsumerRows.Count -ne 8 -or
    (($idleDripConsumerRows[0].PSObject.Properties.Name) -join '|') -cne ($idleDripExpectedColumns -join '|') -or
    @($idleDripConsumerRows | Where-Object {
        $_.normal_match -cne 'true' -or $_.captured_paint -cne 'true' -or
        $_.all_profiles_public_state_rng -cne 'true' -or $_.scope -cne 'consumer-only' -or
        ($_.palette -ceq 'default' -and $_.control -cne 'opaque-legacy-parity') -or
        ($_.palette -ceq 'rgb' -and $_.control -cne 'changed-rgb') -or
        ($_.palette -ceq 'rgba128' -and $_.control -cne 'straight-and-squared-base-alpha-rejected') -or
        ($_.palette -ceq 'alpha0' -and $_.control -cne 'straight-alpha-rejected-and-paint-retained')
    }).Count -ne 0 -or $idleDripCrops.Count -ne 8 -or
    (($idleDripObservedPairs | Sort-Object) -join '|') -cne (($idleDripExpectedPairs | Sort-Object) -join '|') -or
    (($idleDripCrops.Name | Sort-Object) -join '|') -cne (($idleDripExpectedNames | Sort-Object) -join '|')) {
    throw 'The idle-drip consumer evidence does not match its frozen literal-oracle/update/readback/CSV/crop/control budgets.'
}
$idleDripExpectedGuardColumns = @('background', 'disabled_blank', 'reduced_blank', 'restored_match', 'profiles_state_restored', 'scope')
if ($idleDripGuardRows.Count -ne 2 -or
    (($idleDripGuardRows[0].PSObject.Properties.Name) -join '|') -cne ($idleDripExpectedGuardColumns -join '|') -or
    (($idleDripGuardRows.background | Sort-Object) -join '|') -cne 'black|nonblack' -or
    @($idleDripGuardRows | Where-Object {
        $_.disabled_blank -cne 'true' -or $_.reduced_blank -cne 'true' -or $_.restored_match -cne 'true' -or
        $_.profiles_state_restored -cne 'true' -or $_.scope -cne 'consumer-guards'
    }).Count -ne 0) {
    throw 'The idle-drip enabled/Reduce Effects evidence does not match its two fixed underlays and restoration budget.'
}

# Check the actual binary prefix and complete payload size, rather than only
# interpreting three text lines from a potentially truncated or oversized PPM.
$idleDripPpmHeader = [Text.Encoding]::ASCII.GetBytes("P6`n380 260`n255`n")
$idleDripCropPixels = 0
$idleDripCropBytes = 0
$idleDripCropEvidence = @(foreach ($crop in $idleDripCrops) {
    $bytes = [IO.File]::ReadAllBytes($crop.FullName)
    if ($bytes.Length -ne $idleDripPpmHeader.Length + (380 * 260 * 3)) {
        throw "Idle drip crop $($crop.Name) does not have exactly its 380x260 RGB payload and canonical header."
    }
    for ($byte = 0; $byte -lt $idleDripPpmHeader.Length; ++$byte) {
        if ($bytes[$byte] -ne $idleDripPpmHeader[$byte]) {
            throw "Idle drip crop $($crop.Name) does not have its canonical P6/380x260/255 binary header."
        }
    }
    $idleDripCropPixels += 380 * 260
    $idleDripCropBytes += $bytes.Length
    [ordered]@{
        name = $crop.Name
        width = 380
        height = 260
        pixels = 380 * 260
        payload_bytes = 380 * 260 * 3
        bytes = $bytes.Length
        sha256 = (Get-FileHash -LiteralPath $crop.FullName -Algorithm SHA256).Hash
    }
})
if ($idleDripCropPixels -ne 790400) {
    throw 'The idle-drip colour crops do not match their frozen total pixel budget.'
}

$idleDripRunLines = @(Get-Content -LiteralPath (Join-Path $run 'run.log'))
$idleDripSaveCalls = @($idleDripRunLines | Where-Object { $_ -match 'Saving\.\.\.$' }).Count
$idleDripSaveSuccesses = @($idleDripRunLines | Where-Object { $_ -ceq 'Save was successful.' }).Count
$idleDripSaveFailures = @(Get-Content -LiteralPath (Join-Path $run 'run.err') |
    Where-Object { $_ -match '^save failure:' }).Count
if ($idleDripSaveFailures -ne 0) {
    throw 'The idle-drip colour leaf must not perform a controlled or unexpected failed Save.'
}
$idleDripArtifacts = [ordered]@{
    consumer_summary = [ordered]@{
        name = 'idle-drip-colour-consumer-summary.json'
        bytes = (Get-Item -LiteralPath $idleDripConsumerSummaryPath).Length
        sha256 = (Get-FileHash -LiteralPath $idleDripConsumerSummaryPath -Algorithm SHA256).Hash
    }
    consumer_rows = [ordered]@{
        name = 'idle-drip-colour-consumers.csv'
        bytes = (Get-Item -LiteralPath $idleDripConsumerRowsPath).Length
        sha256 = (Get-FileHash -LiteralPath $idleDripConsumerRowsPath -Algorithm SHA256).Hash
    }
    guard_rows = [ordered]@{
        name = 'idle-drip-colour-guards.csv'
        bytes = (Get-Item -LiteralPath $idleDripGuardRowsPath).Length
        sha256 = (Get-FileHash -LiteralPath $idleDripGuardRowsPath -Algorithm SHA256).Hash
    }
    crops = $idleDripCropEvidence
}
$idleDripObserved = [ordered]@{
    consumer = $idleDripConsumerSummary
    state_updates = 0
    level_updates = 0
    effect_preparation_updates = 15
    readbacks = 46
    csv_rows = 10
    crops = $idleDripCrops.Count
    controls = 8
    guard_checks = 6
    profiles = 5
    consumers = 6
    colour_rows = 8
    guard_rows = 2
    private_sequence_scope = 'source-constness-only'
    crop_pixels = $idleDripCropPixels
    ppm_bytes = $idleDripCropBytes
    private_save_calls = $idleDripSaveCalls
    successful_private_saves = $idleDripSaveSuccesses
    failed_private_saves = $idleDripSaveFailures
    artifacts = $idleDripArtifacts
}
if ($debug) {
    $idleDripTuneSummaryPath = Join-Path $captures 'idle-drip-tune-summary.json'
    $idleDripTuneRowsPath = Join-Path $captures 'idle-drip-tune-colours.csv'
    $idleDripTuneTranscriptPath = Join-Path $captures 'idle-drip-tune-commands.log'
    $idleDripTuneSummary = Get-Content -LiteralPath $idleDripTuneSummaryPath -Raw | ConvertFrom-Json
    $idleDripTuneRows = @(Import-Csv -LiteralPath $idleDripTuneRowsPath)
    $idleDripTuneTranscript = Get-Item -LiteralPath $idleDripTuneTranscriptPath
    $idleDripExpectedTuneColumns = @('state', 'background', 'normal_match', 'captured_paint',
        'all_profiles_public_state_rng', 'private_bytes', 'adoption')
    $idleDripExpectedStates = @('baseline', 'named', 'reset-original', 'rgb', 'rgba128', 'saved-rgba',
        'edited-after-save', 'reset-saved-rgba', 'alpha0', 'reset-alpha0-to-saved')
    $idleDripExpectedTunePairs = @(foreach ($state in $idleDripExpectedStates) {
        foreach ($background in @('black', 'nonblack')) { "$state/$background" }
    })
    $idleDripObservedTunePairs = @($idleDripTuneRows | ForEach-Object { "$($_.state)/$($_.background)" })
    if ($idleDripTuneSummary.scope -cne 'debug-validated-commands' -or $idleDripTuneSummary.state_updates -ne 0 -or
        $idleDripTuneSummary.level_updates -ne 0 -or $idleDripTuneSummary.effect_preparation_updates -ne 15 -or
        $idleDripTuneSummary.commands -ne 9 -or $idleDripTuneSummary.readbacks -ne 60 -or
        $idleDripTuneSummary.csv_rows -ne 20 -or $idleDripTuneSummary.crops -ne 0 -or
        $idleDripTuneSummary.profiles -ne 5 -or $idleDripTuneSummary.consumers -ne 6 -or
        $idleDripTuneSummary.private_sequence_scope -cne 'source-constness-only' -or
        $idleDripTuneRows.Count -ne 20 -or $idleDripTuneTranscript.Length -le 0 -or
        !(Test-Path -LiteralPath (Join-Path $captures 'idle-drip-tune-scratch') -PathType Container) -or
        (($idleDripTuneRows[0].PSObject.Properties.Name) -join '|') -cne ($idleDripExpectedTuneColumns -join '|') -or
        $idleDripSaveCalls -ne 2 -or $idleDripSaveSuccesses -ne 2 -or
        @($idleDripTuneRows | Where-Object {
            $_.normal_match -cne 'true' -or $_.captured_paint -cne 'true' -or
            $_.all_profiles_public_state_rng -cne 'true' -or $_.private_bytes -cne 'true' -or $_.adoption -cne 'live'
        }).Count -ne 0 -or
        (($idleDripObservedTunePairs | Sort-Object) -join '|') -cne (($idleDripExpectedTunePairs | Sort-Object) -join '|')) {
        throw 'The idle-drip Debug evidence does not match its frozen validated-command/private-save/update/readback/CSV budgets.'
    }
    $idleDripArtifacts['developer_summary'] = [ordered]@{
        name = $idleDripTuneSummaryPath | Split-Path -Leaf
        bytes = (Get-Item -LiteralPath $idleDripTuneSummaryPath).Length
        sha256 = (Get-FileHash -LiteralPath $idleDripTuneSummaryPath -Algorithm SHA256).Hash
    }
    $idleDripArtifacts['developer_rows'] = [ordered]@{
        name = $idleDripTuneRowsPath | Split-Path -Leaf
        bytes = (Get-Item -LiteralPath $idleDripTuneRowsPath).Length
        sha256 = (Get-FileHash -LiteralPath $idleDripTuneRowsPath -Algorithm SHA256).Hash
    }
    $idleDripArtifacts['developer_transcript'] = [ordered]@{
        name = $idleDripTuneTranscript.Name
        bytes = $idleDripTuneTranscript.Length
        sha256 = (Get-FileHash -LiteralPath $idleDripTuneTranscriptPath -Algorithm SHA256).Hash
    }
    $idleDripObserved['developer_commands'] = $idleDripTuneSummary
    $idleDripObserved.effect_preparation_updates = 30
    $idleDripObserved.readbacks = 106
    $idleDripObserved.csv_rows = 30
}
elseif ($idleDripSaveCalls -ne 0 -or $idleDripSaveSuccesses -ne 0 -or
    @(Get-ChildItem -LiteralPath $captures -Filter 'idle-drip-tune-*').Count -ne 0) {
    throw 'Release idle-drip evidence must remain consumer-only without developer artifacts or save calls.'
}
$result['observed_counts'] = $idleDripObserved
