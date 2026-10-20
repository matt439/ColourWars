# Dot-sourced into the scope of scripts/check-painter.ps1. The guard
# if ($BlockedShotTuneColoursOnly) stays there; this is the leaf's post-run evidence check.

$blockedConsumerSummaryPath = Join-Path $captures 'blocked-colour-consumer-summary.json'
$blockedConsumerRowsPath = Join-Path $captures 'blocked-colour-consumers.csv'
$blockedConsumerSummary = Get-Content -LiteralPath $blockedConsumerSummaryPath -Raw | ConvertFrom-Json
$blockedConsumerRows = @(Import-Csv -LiteralPath $blockedConsumerRowsPath)
$blockedCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'blocked-*.ppm' -File | Sort-Object Name)
$blockedExpectedColumns = @('palette', 'background', 'normal_match', 'stored_team_beads',
    'complete_owner_cue_bounds_rng', 'control', 'scope')
$blockedExpectedNames = @(foreach ($palette in @('default', 'rgb', 'rgba128', 'alpha0')) {
    foreach ($background in @('black', 'nonblack')) { "blocked-colour-$palette-$background.ppm" }
})
$blockedExpectedPairs = @(foreach ($palette in @('default', 'rgb', 'rgba128', 'alpha0')) {
    foreach ($background in @('black', 'nonblack')) { "$palette/$background" }
})
$blockedObservedPairs = @($blockedConsumerRows | ForEach-Object { "$($_.palette)/$($_.background)" })
if ($blockedConsumerSummary.scope -cne 'consumer-only' -or $blockedConsumerSummary.state_updates -ne 0 -or
    $blockedConsumerSummary.level_updates -ne 0 -or $blockedConsumerSummary.effect_preparation_updates -ne 1 -or
    $blockedConsumerSummary.owner_budgets_per_instance -ne 10 -or $blockedConsumerSummary.readbacks -ne 32 -or $blockedConsumerSummary.csv_rows -ne 8 -or
    $blockedConsumerSummary.crops -ne 8 -or $blockedConsumerSummary.controls -ne 8 -or
    $blockedConsumerRows.Count -ne 8 -or
    (($blockedConsumerRows[0].PSObject.Properties.Name) -join '|') -cne ($blockedExpectedColumns -join '|') -or
    @($blockedConsumerRows | Where-Object {
        $_.normal_match -cne 'true' -or $_.stored_team_beads -cne 'true' -or
        $_.complete_owner_cue_bounds_rng -cne 'true' -or $_.scope -cne 'consumer-only' -or
        ($_.palette -ceq 'default' -and $_.control -cne 'opaque-legacy-parity') -or
        ($_.palette -ceq 'rgb' -and $_.control -cne 'double-fade-rejected-and-changed-rgb') -or
        ($_.palette -cin @('rgba128', 'alpha0') -and $_.control -cne 'legacy-straight-alpha-rejected')
    }).Count -ne 0 -or $blockedCrops.Count -ne 8 -or
    (($blockedObservedPairs | Sort-Object) -join '|') -cne (($blockedExpectedPairs | Sort-Object) -join '|') -or
    (($blockedCrops.Name | Sort-Object) -join '|') -cne (($blockedExpectedNames | Sort-Object) -join '|')) {
    throw 'The blocked consumer evidence does not match its frozen literal-oracle/update/readback/CSV/crop/control budgets.'
}

# Check the actual binary prefix and complete payload size, rather than only
# interpreting three text lines from a potentially truncated or oversized PPM.
$blockedPpmHeader = [Text.Encoding]::ASCII.GetBytes("P6`n440 240`n255`n")
$blockedCropPixels = 0
$blockedCropBytes = 0
$blockedCropEvidence = @(foreach ($crop in $blockedCrops) {
    $bytes = [IO.File]::ReadAllBytes($crop.FullName)
    if ($bytes.Length -ne $blockedPpmHeader.Length + (440 * 240 * 3)) {
        throw "Blocked crop $($crop.Name) does not have exactly its 440x240 RGB payload and canonical header."
    }
    for ($byte = 0; $byte -lt $blockedPpmHeader.Length; ++$byte) {
        if ($bytes[$byte] -ne $blockedPpmHeader[$byte]) {
            throw "Blocked crop $($crop.Name) does not have its canonical P6/440x240/255 binary header."
        }
    }
    $blockedCropPixels += 440 * 240
    $blockedCropBytes += $bytes.Length
    [ordered]@{
        name = $crop.Name
        width = 440
        height = 240
        pixels = 440 * 240
        payload_bytes = 440 * 240 * 3
        bytes = $bytes.Length
        sha256 = (Get-FileHash -LiteralPath $crop.FullName -Algorithm SHA256).Hash
    }
})
if ($blockedCropPixels -ne 844800) {
    throw 'The blocked colour crops do not match their frozen total pixel budget.'
}

$blockedRunLines = @(Get-Content -LiteralPath (Join-Path $run 'run.log'))
$blockedSaveCalls = @($blockedRunLines | Where-Object { $_ -match 'Saving\.\.\.$' }).Count
$blockedSaveSuccesses = @($blockedRunLines | Where-Object { $_ -ceq 'Save was successful.' }).Count
$blockedSaveFailures = @(Get-Content -LiteralPath (Join-Path $run 'run.err') |
    Where-Object { $_ -match '^save failure:' }).Count
if ($blockedSaveFailures -ne 0) {
    throw 'The blocked colour leaf must not perform a controlled or unexpected failed Save.'
}
$blockedArtifacts = [ordered]@{
    consumer_summary = [ordered]@{
        name = 'blocked-colour-consumer-summary.json'
        bytes = (Get-Item -LiteralPath $blockedConsumerSummaryPath).Length
        sha256 = (Get-FileHash -LiteralPath $blockedConsumerSummaryPath -Algorithm SHA256).Hash
    }
    consumer_rows = [ordered]@{
        name = 'blocked-colour-consumers.csv'
        bytes = (Get-Item -LiteralPath $blockedConsumerRowsPath).Length
        sha256 = (Get-FileHash -LiteralPath $blockedConsumerRowsPath -Algorithm SHA256).Hash
    }
    crops = $blockedCropEvidence
}
$blockedObserved = [ordered]@{
    consumer = $blockedConsumerSummary
    state_updates = 0
    level_updates = 0
    owner_budgets_per_instance = 10
    effect_preparation_updates = 1
    readbacks = 32
    csv_rows = 8
    crops = $blockedCrops.Count
    controls = 8
    crop_pixels = $blockedCropPixels
    ppm_bytes = $blockedCropBytes
    private_save_calls = $blockedSaveCalls
    successful_private_saves = $blockedSaveSuccesses
    failed_private_saves = $blockedSaveFailures
    artifacts = $blockedArtifacts
}
if ($debug) {
    $blockedTuneSummaryPath = Join-Path $captures 'blocked-tune-summary.json'
    $blockedTuneRowsPath = Join-Path $captures 'blocked-tune-colours.csv'
    $blockedTuneTranscriptPath = Join-Path $captures 'blocked-tune-commands.log'
    $blockedTuneSummary = Get-Content -LiteralPath $blockedTuneSummaryPath -Raw | ConvertFrom-Json
    $blockedTuneRows = @(Import-Csv -LiteralPath $blockedTuneRowsPath)
    $blockedTuneTranscript = Get-Item -LiteralPath $blockedTuneTranscriptPath
    $blockedExpectedTuneColumns = @('state', 'background', 'normal_match', 'stored_team_beads',
        'complete_owner_cue_bounds_rng', 'private_bytes', 'adoption')
    $blockedExpectedStates = @('baseline', 'named', 'reset-original', 'rgb', 'rgba128', 'saved-rgba',
        'edited-after-save', 'reset-saved-rgba', 'alpha0', 'reset-alpha0-to-saved')
    $blockedExpectedTunePairs = @(foreach ($state in $blockedExpectedStates) {
        foreach ($background in @('black', 'nonblack')) { "$state/$background" }
    })
    $blockedObservedTunePairs = @($blockedTuneRows | ForEach-Object { "$($_.state)/$($_.background)" })
    if ($blockedTuneSummary.scope -cne 'debug-validated-commands' -or $blockedTuneSummary.state_updates -ne 0 -or
        $blockedTuneSummary.level_updates -ne 0 -or $blockedTuneSummary.effect_preparation_updates -ne 1 -or
        $blockedTuneSummary.owner_budgets_per_instance -ne 10 -or $blockedTuneSummary.commands -ne 15 -or $blockedTuneSummary.readbacks -ne 60 -or
        $blockedTuneSummary.csv_rows -ne 20 -or $blockedTuneSummary.crops -ne 0 -or
        $blockedTuneRows.Count -ne 20 -or $blockedTuneTranscript.Length -le 0 -or
        !(Test-Path -LiteralPath (Join-Path $captures 'blocked-tune-scratch') -PathType Container) -or
        (($blockedTuneRows[0].PSObject.Properties.Name) -join '|') -cne ($blockedExpectedTuneColumns -join '|') -or
        $blockedSaveCalls -ne 2 -or $blockedSaveSuccesses -ne 2 -or
        @($blockedTuneRows | Where-Object {
            $_.normal_match -cne 'true' -or $_.stored_team_beads -cne 'true' -or
            $_.complete_owner_cue_bounds_rng -cne 'true' -or $_.private_bytes -cne 'true' -or $_.adoption -cne 'live'
        }).Count -ne 0 -or
        (($blockedObservedTunePairs | Sort-Object) -join '|') -cne (($blockedExpectedTunePairs | Sort-Object) -join '|')) {
        throw 'The blocked Debug evidence does not match its frozen validated-command/private-save/update/readback/CSV budgets.'
    }
    $blockedArtifacts['developer_summary'] = [ordered]@{
        name = $blockedTuneSummaryPath | Split-Path -Leaf
        bytes = (Get-Item -LiteralPath $blockedTuneSummaryPath).Length
        sha256 = (Get-FileHash -LiteralPath $blockedTuneSummaryPath -Algorithm SHA256).Hash
    }
    $blockedArtifacts['developer_rows'] = [ordered]@{
        name = $blockedTuneRowsPath | Split-Path -Leaf
        bytes = (Get-Item -LiteralPath $blockedTuneRowsPath).Length
        sha256 = (Get-FileHash -LiteralPath $blockedTuneRowsPath -Algorithm SHA256).Hash
    }
    $blockedArtifacts['developer_transcript'] = [ordered]@{
        name = $blockedTuneTranscript.Name
        bytes = $blockedTuneTranscript.Length
        sha256 = (Get-FileHash -LiteralPath $blockedTuneTranscriptPath -Algorithm SHA256).Hash
    }
    $blockedObserved['developer_commands'] = $blockedTuneSummary
    $blockedObserved.effect_preparation_updates = 2
    $blockedObserved.readbacks = 92
    $blockedObserved.csv_rows = 28
}
elseif ($blockedSaveCalls -ne 0 -or $blockedSaveSuccesses -ne 0 -or
    @(Get-ChildItem -LiteralPath $captures -Filter 'blocked-tune-*').Count -ne 0) {
    throw 'Release blocked evidence must remain consumer-only without developer artifacts or save calls.'
}
$result['observed_counts'] = $blockedObserved
