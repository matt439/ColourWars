# Dot-sourced into the scope of scripts/check-painter.ps1. The guard
# if ($CombatTuneColoursOnly) stays there; this is the leaf's post-run evidence check.

$combatConsumerSummaryPath = Join-Path $captures 'combat-colour-consumer-summary.json'
$combatConsumerRowsPath = Join-Path $captures 'combat-colour-consumers.csv'
$combatConsumerSummary = Get-Content -LiteralPath $combatConsumerSummaryPath -Raw | ConvertFrom-Json
$combatConsumerRows = @(Import-Csv -LiteralPath $combatConsumerRowsPath)
$combatCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'combat-*.ppm' -File | Sort-Object Name)
$combatExpectedColumns = @('palette', 'background', 'normal_match', 'team_centres',
    'frozen_geometry_timers_rng', 'control', 'scope')
$combatExpectedNames = @(foreach ($palette in @('default', 'rgb', 'rgba128', 'alpha0')) {
    foreach ($background in @('black', 'nonblack')) { "combat-colour-$palette-$background.ppm" }
})
$combatExpectedPairs = @(foreach ($palette in @('default', 'rgb', 'rgba128', 'alpha0')) {
    foreach ($background in @('black', 'nonblack')) { "$palette/$background" }
})
$combatObservedPairs = @($combatConsumerRows | ForEach-Object { "$($_.palette)/$($_.background)" })
if ($combatConsumerSummary.scope -cne 'consumer-only' -or $combatConsumerSummary.state_updates -ne 0 -or
    $combatConsumerSummary.level_updates -ne 0 -or $combatConsumerSummary.effect_preparation_updates -ne 2 -or
    $combatConsumerSummary.readbacks -ne 32 -or $combatConsumerSummary.csv_rows -ne 8 -or
    $combatConsumerSummary.crops -ne 8 -or $combatConsumerSummary.controls -ne 8 -or
    $combatConsumerRows.Count -ne 8 -or
    (($combatConsumerRows[0].PSObject.Properties.Name) -join '|') -cne ($combatExpectedColumns -join '|') -or
    @($combatConsumerRows | Where-Object {
        $_.normal_match -cne 'true' -or $_.team_centres -cne 'true' -or
        $_.frozen_geometry_timers_rng -cne 'true' -or $_.scope -cne 'consumer-only' -or
        ($_.palette -ceq 'default' -and $_.control -cne 'opaque-legacy-parity-and-double-fade-rejected') -or
        ($_.palette -ceq 'rgb' -and $_.control -cne 'changed-rgb') -or
        ($_.palette -cin @('rgba128', 'alpha0') -and $_.control -cne 'legacy-straight-alpha-rejected')
    }).Count -ne 0 -or $combatCrops.Count -ne 8 -or
    (($combatObservedPairs | Sort-Object) -join '|') -cne (($combatExpectedPairs | Sort-Object) -join '|') -or
    (($combatCrops.Name | Sort-Object) -join '|') -cne (($combatExpectedNames | Sort-Object) -join '|')) {
    throw 'The combat consumer evidence does not match its frozen literal-oracle/update/readback/CSV/crop/control budgets.'
}

# Check the actual binary prefix and complete payload size, rather than only
# interpreting three text lines from a potentially truncated or oversized PPM.
$combatPpmHeader = [Text.Encoding]::ASCII.GetBytes("P6`n420 360`n255`n")
$combatCropPixels = 0
$combatCropBytes = 0
$combatCropEvidence = @(foreach ($crop in $combatCrops) {
    $bytes = [IO.File]::ReadAllBytes($crop.FullName)
    if ($bytes.Length -ne $combatPpmHeader.Length + (420 * 360 * 3)) {
        throw "Combat crop $($crop.Name) does not have exactly its 420x360 RGB payload and canonical header."
    }
    for ($byte = 0; $byte -lt $combatPpmHeader.Length; ++$byte) {
        if ($bytes[$byte] -ne $combatPpmHeader[$byte]) {
            throw "Combat crop $($crop.Name) does not have its canonical P6/420x360/255 binary header."
        }
    }
    $combatCropPixels += 420 * 360
    $combatCropBytes += $bytes.Length
    [ordered]@{
        name = $crop.Name
        width = 420
        height = 360
        pixels = 420 * 360
        payload_bytes = 420 * 360 * 3
        bytes = $bytes.Length
        sha256 = (Get-FileHash -LiteralPath $crop.FullName -Algorithm SHA256).Hash
    }
})
if ($combatCropPixels -ne 1209600) {
    throw 'The combat colour crops do not match their frozen total pixel budget.'
}

$combatRunLines = @(Get-Content -LiteralPath (Join-Path $run 'run.log'))
$combatSaveCalls = @($combatRunLines | Where-Object { $_ -match 'Saving\.\.\.$' }).Count
$combatSaveSuccesses = @($combatRunLines | Where-Object { $_ -ceq 'Save was successful.' }).Count
$combatSaveFailures = @(Get-Content -LiteralPath (Join-Path $run 'run.err') |
    Where-Object { $_ -match '^save failure:' }).Count
if ($combatSaveFailures -ne 0) {
    throw 'The combat colour leaf must not perform a controlled or unexpected failed Save.'
}
$combatArtifacts = [ordered]@{
    consumer_summary = [ordered]@{
        name = 'combat-colour-consumer-summary.json'
        bytes = (Get-Item -LiteralPath $combatConsumerSummaryPath).Length
        sha256 = (Get-FileHash -LiteralPath $combatConsumerSummaryPath -Algorithm SHA256).Hash
    }
    consumer_rows = [ordered]@{
        name = 'combat-colour-consumers.csv'
        bytes = (Get-Item -LiteralPath $combatConsumerRowsPath).Length
        sha256 = (Get-FileHash -LiteralPath $combatConsumerRowsPath -Algorithm SHA256).Hash
    }
    crops = $combatCropEvidence
}
$combatObserved = [ordered]@{
    consumer = $combatConsumerSummary
    state_updates = 0
    level_updates = 0
    effect_preparation_updates = 2
    readbacks = 32
    csv_rows = 8
    crops = $combatCrops.Count
    controls = 8
    crop_pixels = $combatCropPixels
    ppm_bytes = $combatCropBytes
    private_save_calls = $combatSaveCalls
    successful_private_saves = $combatSaveSuccesses
    failed_private_saves = $combatSaveFailures
    artifacts = $combatArtifacts
}
if ($debug) {
    $combatTuneSummaryPath = Join-Path $captures 'combat-tune-summary.json'
    $combatTuneRowsPath = Join-Path $captures 'combat-tune-colours.csv'
    $combatTuneTranscriptPath = Join-Path $captures 'combat-tune-commands.log'
    $combatTuneSummary = Get-Content -LiteralPath $combatTuneSummaryPath -Raw | ConvertFrom-Json
    $combatTuneRows = @(Import-Csv -LiteralPath $combatTuneRowsPath)
    $combatTuneTranscript = Get-Item -LiteralPath $combatTuneTranscriptPath
    $combatExpectedTuneColumns = @('state', 'background', 'normal_match', 'team_centres',
        'frozen_geometry_timers_rng', 'private_bytes', 'adoption')
    $combatExpectedStates = @('baseline', 'named', 'reset-original', 'rgb', 'rgba128', 'saved-rgba',
        'edited-after-save', 'reset-saved-rgba', 'alpha0', 'reset-alpha0-to-saved')
    $combatExpectedTunePairs = @(foreach ($state in $combatExpectedStates) {
        foreach ($background in @('black', 'nonblack')) { "$state/$background" }
    })
    $combatObservedTunePairs = @($combatTuneRows | ForEach-Object { "$($_.state)/$($_.background)" })
    if ($combatTuneSummary.scope -cne 'debug-validated-commands' -or $combatTuneSummary.state_updates -ne 0 -or
        $combatTuneSummary.level_updates -ne 0 -or $combatTuneSummary.effect_preparation_updates -ne 2 -or
        $combatTuneSummary.commands -ne 15 -or $combatTuneSummary.readbacks -ne 60 -or
        $combatTuneSummary.csv_rows -ne 20 -or $combatTuneSummary.crops -ne 0 -or
        $combatTuneRows.Count -ne 20 -or $combatTuneTranscript.Length -le 0 -or
        !(Test-Path -LiteralPath (Join-Path $captures 'combat-tune-scratch') -PathType Container) -or
        (($combatTuneRows[0].PSObject.Properties.Name) -join '|') -cne ($combatExpectedTuneColumns -join '|') -or
        $combatSaveCalls -ne 2 -or $combatSaveSuccesses -ne 2 -or
        @($combatTuneRows | Where-Object {
            $_.normal_match -cne 'true' -or $_.team_centres -cne 'true' -or
            $_.frozen_geometry_timers_rng -cne 'true' -or $_.private_bytes -cne 'true' -or $_.adoption -cne 'live'
        }).Count -ne 0 -or
        (($combatObservedTunePairs | Sort-Object) -join '|') -cne (($combatExpectedTunePairs | Sort-Object) -join '|')) {
        throw 'The combat Debug evidence does not match its frozen validated-command/private-save/update/readback/CSV budgets.'
    }
    $combatArtifacts['developer_summary'] = [ordered]@{
        name = $combatTuneSummaryPath | Split-Path -Leaf
        bytes = (Get-Item -LiteralPath $combatTuneSummaryPath).Length
        sha256 = (Get-FileHash -LiteralPath $combatTuneSummaryPath -Algorithm SHA256).Hash
    }
    $combatArtifacts['developer_rows'] = [ordered]@{
        name = $combatTuneRowsPath | Split-Path -Leaf
        bytes = (Get-Item -LiteralPath $combatTuneRowsPath).Length
        sha256 = (Get-FileHash -LiteralPath $combatTuneRowsPath -Algorithm SHA256).Hash
    }
    $combatArtifacts['developer_transcript'] = [ordered]@{
        name = $combatTuneTranscript.Name
        bytes = $combatTuneTranscript.Length
        sha256 = (Get-FileHash -LiteralPath $combatTuneTranscriptPath -Algorithm SHA256).Hash
    }
    $combatObserved['developer_commands'] = $combatTuneSummary
    $combatObserved.effect_preparation_updates = 4
    $combatObserved.readbacks = 92
    $combatObserved.csv_rows = 28
}
elseif ($combatSaveCalls -ne 0 -or $combatSaveSuccesses -ne 0 -or
    @(Get-ChildItem -LiteralPath $captures -Filter 'combat-tune-*').Count -ne 0) {
    throw 'Release combat evidence must remain consumer-only without developer artifacts or save calls.'
}
$result['observed_counts'] = $combatObserved
