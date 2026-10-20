# Dot-sourced only for -CqBlockoutOnly, before native PNG contact sheets.
# Inspect this run's bounded receipts and lossless files; no game rerun.
$cqRows = @(Import-Csv -LiteralPath (Join-Path $captures 'cq-blockout.csv'))
$cqColumns = @('capture', 'source', 'route', 'view', 'input_slot', 'frame_width', 'frame_height',
    'player_x', 'player_y', 'camera_x', 'camera_y', 'camera_scale', 'crop_x', 'crop_y',
    'crop_width', 'crop_height', 'frozen_pixels', 'public_state', 'debug_fingerprint', 'rng', 'evidence_scope')
$cqPrefix = 'hidden-inspection-cq-'
$cqExpected = @{
    '1p-main-720p|main-contest' = @('actual-Level', '1', '3')
    '1p-lower-720p|lower-bypass' = @('actual-Level', '1', '3')
    '4p-routes-720p|main-contest' = @('actual-Level', '1', '3')
    '4p-routes-720p|lower-bypass' = @('actual-Level', '2', '1')
    '4p-routes-720p|upper-route' = @('actual-Level', '3', '2')
    '4p-routes-720p|spawn-b' = @('actual-Level', '4', '0')
    'runtime-overview-720p|whole-stage-and-spawns' = @('actual-Level-overview', '1', '3')
    'actual-stage-preview-720p|menu-miniature' = @('actual-StagePreview-isolated', '1', '-1')
}
if ($cqRows.Count -ne 8 -or (($cqRows[0].PSObject.Properties.Name) -join '|') -cne ($cqColumns -join '|')) {
    throw 'CQ native evidence needs its exact eight route/source rows and CSV schema.'
}
$cqSeen = @{}
$cqDimensions = @{}
$cqCropPixels = 0L
foreach ($row in $cqRows) {
    if (-not $row.capture.StartsWith($cqPrefix, [StringComparison]::Ordinal)) {
        throw 'CQ native evidence has an unexpected capture prefix.'
    }
    $key = $row.capture.Substring($cqPrefix.Length) + '|' + $row.route
    if (-not $cqExpected.ContainsKey($key) -or $cqSeen.ContainsKey($key)) {
        throw "CQ native evidence has an unexpected or duplicated route row: $key"
    }
    $cqSeen[$key] = $true
    $expected = $cqExpected[$key]
    $isWidget = $row.source -ceq 'actual-StagePreview-isolated'
    $expectedPublic = if ($isWidget) { 'static-widget-bounds' } else { '1' }
    $expectedFingerprint = if (-not $debug) { 'not-available-in-Release' }
        elseif ($isWidget) { 'not-applicable-widget' } else { '1' }
    if ($row.source -cne $expected[0] -or $row.view -cne $expected[1] -or $row.input_slot -cne $expected[2] -or
        $row.frame_width -cne '1280' -or $row.frame_height -cne '720' -or
        $row.frozen_pixels -cne '1' -or $row.public_state -cne $expectedPublic -or
        $row.debug_fingerprint -cne $expectedFingerprint -or $row.rng -cne '1' -or
        $row.evidence_scope -cne 'hidden-native-inspection') {
        throw "CQ native evidence changed its source, view/slot, 720p or frozen inspection contract: $key"
    }
    foreach ($metric in @('player_x', 'player_y', 'camera_x', 'camera_y', 'camera_scale')) {
        $value = 0.0
        if (-not [double]::TryParse($row.$metric, [Globalization.NumberStyles]::Float,
            [Globalization.CultureInfo]::InvariantCulture, [ref]$value) -or
            [double]::IsNaN($value) -or [double]::IsInfinity($value) -or
            ($metric -ceq 'camera_scale' -and $value -le 0.0)) {
            throw "CQ native evidence has an invalid $metric for $key"
        }
    }
    $crop = @()
    foreach ($metric in @('crop_x', 'crop_y', 'crop_width', 'crop_height')) {
        $value = 0
        if (-not [int]::TryParse($row.$metric, [ref]$value) -or $value -lt 0) {
            throw "CQ native evidence has an invalid $metric for $key"
        }
        $crop += $value
    }
    if ($crop[2] -le 0 -or $crop[3] -le 0 -or $crop[0] + $crop[2] -gt 1280 -or $crop[1] + $crop[3] -gt 720) {
        throw "CQ native crop escapes its full frame: $key"
    }
    $cropName = if ($row.source -ceq 'actual-Level') { $row.capture + '-' + $row.route + '-native.ppm' }
        else { $row.capture -creplace '-720p$', '-native.ppm' }
    $cqDimensions[$row.capture + '.ppm'] = @(1280, 720)
    $cqDimensions[$cropName] = @($crop[2], $crop[3])
    $cqCropPixels += [long]$crop[2] * $crop[3]
}
$cqFiles = @(Get-ChildItem -LiteralPath $captures -Filter '*.ppm' -File)
if ($cqDimensions.Count -ne 13 -or $cqFiles.Count -ne 13 -or
    (($cqFiles.Name | Sort-Object) -join '|') -cne (($cqDimensions.Keys | Sort-Object) -join '|')) {
    throw 'CQ native inspection needs exactly five full frames and eight native crops, without another painter matrix.'
}
$cqPpmBytes = 0L
foreach ($file in $cqFiles) {
    $size = $cqDimensions[$file.Name]
    $header = @(Get-Content -LiteralPath $file.FullName -TotalCount 3)
    $expectedHeader = "P6`n$($size[0]) $($size[1])`n255`n"
    $expectedBytes = [Text.Encoding]::ASCII.GetByteCount($expectedHeader) + [long]$size[0] * $size[1] * 3
    if ($header.Count -ne 3 -or $header[0] -cne 'P6' -or $header[1] -cne "$($size[0]) $($size[1])" -or
        $header[2] -cne '255' -or $file.Length -ne $expectedBytes) {
        throw "CQ native file differs from its full-frame/CSV dimensions or exact RGB byte count: $($file.Name)"
    }
    $cqPpmBytes += $file.Length
}
$cqScope = Get-Content -LiteralPath (Join-Path $captures 'cq-blockout-scope.txt') -Raw
if ($cqScope -notmatch 'Updates: (\d+); readbacks: (\d+)\.' -or
    [int]$Matches[1] -le 0 -or [int]$Matches[1] -gt 1800 -or [int]$Matches[2] -ne 23) {
    throw 'CQ native scope receipt lost its bounded Level update or exact 23-readback count.'
}
$result['observed_counts'] = [ordered]@{
    full_frames = 5; native_crops = 8; csv_rows = 8; readbacks = 23
    level_updates = [int]$Matches[1]; native_crop_pixels = $cqCropPixels; ppm_bytes = $cqPpmBytes
}
