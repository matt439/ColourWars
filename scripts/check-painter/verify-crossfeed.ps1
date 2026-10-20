# Dot-sourced only for the focused Crossfeed leaf, before native PNG sheets.
# Visibility is an observation for H6 review, not an asserted readability result.
$crossfeedSummary = Get-Content -LiteralPath (Join-Path $captures 'crossfeed-summary.json') -Raw | ConvertFrom-Json
$crossfeedRows = @(Import-Csv -LiteralPath (Join-Path $captures 'crossfeed.csv'))
# Reconstruct known 720p pane/camera relationships from staged authoring inputs,
# independently of the CSV. StagePreview fits its fractional menu pane before
# Viewport::pixel_rect truncates each physical edge.
$crossfeedPresentation = Get-Content -LiteralPath (Join-Path $gameDirectory 'presentation.json') -Raw | ConvertFrom-Json
$crossfeedStage = Get-Content -LiteralPath (Join-Path $gameDirectory 'levels/crossfeed.json') -Raw | ConvertFrom-Json
$crossfeedWorld = $crossfeedStage.level_bounds.zoom_out_finish_bounds
$crossfeedLook = $crossfeedPresentation.main_menu.stage_select
$crossfeedMenuScale = [single][Math]::Min(1280.0 / 1920.0, 720.0 / 1080.0)
$crossfeedPreviewX = [single]([single]$crossfeedLook.preview_position.x * $crossfeedMenuScale)
$crossfeedPreviewY = [single]([single]$crossfeedLook.preview_position.y * $crossfeedMenuScale)
$crossfeedPreviewWidth = [single]([single]$crossfeedLook.preview_size.x * $crossfeedMenuScale)
$crossfeedPreviewHeight = [single]([single]$crossfeedLook.preview_size.y * $crossfeedMenuScale)
$crossfeedPreviewPixels = @([int][Math]::Truncate($crossfeedPreviewX), [int][Math]::Truncate($crossfeedPreviewY),
    ([int][Math]::Truncate([single]($crossfeedPreviewX + $crossfeedPreviewWidth)) - [int][Math]::Truncate($crossfeedPreviewX)),
    ([int][Math]::Truncate([single]($crossfeedPreviewY + $crossfeedPreviewHeight)) - [int][Math]::Truncate($crossfeedPreviewY)))
$crossfeedPreviewScale = [single][Math]::Min([single]($crossfeedPreviewWidth / $crossfeedWorld.width),
    [single]($crossfeedPreviewHeight / $crossfeedWorld.height))
$crossfeedPreviewCameraX = [single]($crossfeedWorld.x - [single]([single]([single]($crossfeedPreviewWidth /
    $crossfeedPreviewScale) - $crossfeedWorld.width) / 2.0))
$crossfeedPreviewCameraY = [single]($crossfeedWorld.y - [single]([single]([single]($crossfeedPreviewHeight /
    $crossfeedPreviewScale) - $crossfeedWorld.height) / 2.0))
$crossfeedColumns = @('capture', 'kind', 'phase', 'paint', 'players', 'view', 'input_slot',
    'player_x', 'player_y', 'velocity_x', 'velocity_y', 'camera_x', 'camera_y', 'camera_scale',
    'pane_x', 'pane_y', 'pane_width', 'pane_height', 'crop_x', 'crop_y', 'crop_width', 'crop_height',
    'owned_fraction', 'target_x', 'target_y', 'target_width', 'target_height', 'target_visible',
    'frozen_pixels', 'public_state', 'debug_fingerprint', 'rng', 'evidence_scope')
if ($crossfeedSummary.outcome -cne 'passed' -or $crossfeedSummary.scope -cne 'crossfeed-only' -or
    $crossfeedSummary.full_frames -ne 21 -or $crossfeedSummary.native_crops -ne 51 -or
    $crossfeedSummary.csv_rows -ne 51 -or $crossfeedSummary.readbacks -ne 43 -or
    $crossfeedSummary.level_builds -ne 10 -or $crossfeedSummary.level_updates -le 0 -or
    $crossfeedSummary.level_updates -gt 8000 -or $crossfeedSummary.controlled_heavy_contacts -le 0 -or
    $crossfeedSummary.scripted_input_steps -le 0 -or $crossfeedSummary.heavy_initial_fraction_min -le 0.99 -or
    $crossfeedSummary.heavy_initial_fraction_min -gt 1.0 -or $crossfeedRows.Count -ne 51 -or
    (($crossfeedRows[0].PSObject.Properties.Name) -join '|') -cne ($crossfeedColumns -join '|')) {
    throw 'Crossfeed native evidence changed its bounded summary, row count or CSV schema.'
}
$crossfeedExpected = @{}
$crossfeedSlots = @(3, 1, 2, 0)
foreach ($count in 1..4) {
    foreach ($paint in @('clean', 'heavy')) {
        foreach ($phase in @('approach', 'advance')) {
            $stem = "hidden-inspection-crossfeed-$($count)p-$paint-$phase-720p"
            foreach ($view in 1..$count) {
                $crossfeedExpected["$stem|$view"] = @('actual-Level-fixture', $phase, $paint,
                    [string]$count, [string]$crossfeedSlots[$view - 1])
            }
        }
    }
}
foreach ($paint in @('clean', 'heavy')) {
    $stem = "hidden-inspection-crossfeed-4p-$paint-home-exit-720p"
    foreach ($view in 1..4) {
        $crossfeedExpected["$stem|$view"] = @('actual-Level-fixture', 'home-exit', $paint,
            '4', [string]$crossfeedSlots[$view - 1])
    }
}
foreach ($paint in @('clean', 'heavy')) {
    $crossfeedExpected["hidden-inspection-crossfeed-overview-$paint-720p|1"] =
        @('actual-Level-overview', 'overview', $paint, '1', '3')
}
$crossfeedExpected['hidden-inspection-crossfeed-stage-preview-clean-720p|1'] =
    @('actual-StagePreview-isolated', 'miniature', 'clean', '0', '-1')
$crossfeedSeen = @{}; $crossfeedDimensions = @{}; $crossfeedCropPixels = 0L
foreach ($row in $crossfeedRows) {
    $key = $row.capture + '|' + $row.view
    if (-not $crossfeedExpected.ContainsKey($key) -or $crossfeedSeen.ContainsKey($key)) {
        throw "Crossfeed native evidence has an unexpected or duplicated row: $key"
    }
    $crossfeedSeen[$key] = $true; $expected = $crossfeedExpected[$key]
    $widget = $row.kind -ceq 'actual-StagePreview-isolated'
    $public = if ($widget) { 'static-widget-bounds' } else { '1' }
    $fingerprint = if (-not $debug) { 'not-available-in-Release' }
        elseif ($widget) { 'not-applicable-widget' } else { '1' }
    if ($row.kind -cne $expected[0] -or $row.phase -cne $expected[1] -or $row.paint -cne $expected[2] -or
        $row.players -cne $expected[3] -or $row.input_slot -cne $expected[4] -or
        $row.frozen_pixels -cne '1' -or $row.public_state -cne $public -or
        $row.debug_fingerprint -cne $fingerprint -or $row.rng -cne '1' -or
        $row.evidence_scope -cne 'hidden-native-inspection' -or $row.target_visible -cnotin @('0', '1')) {
        throw "Crossfeed native evidence changed its fixture, roster or frozen inspection contract: $key"
    }
    $metrics = @{}
    foreach ($metric in @('player_x', 'player_y', 'velocity_x', 'velocity_y', 'camera_x', 'camera_y',
        'camera_scale', 'owned_fraction', 'target_x', 'target_y', 'target_width', 'target_height')) {
        $value = 0.0
        if (-not [double]::TryParse($row.$metric, [Globalization.NumberStyles]::Float,
            [Globalization.CultureInfo]::InvariantCulture, [ref]$value) -or
            [double]::IsNaN($value) -or [double]::IsInfinity($value) -or
            ($metric -in @('camera_scale', 'target_width', 'target_height') -and $value -le 0.0)) {
            throw "Crossfeed native evidence has an invalid $metric for $key"
        }
        $metrics[$metric] = $value
    }
    if ($metrics.owned_fraction -lt 0.0 -or $metrics.owned_fraction -gt 1.0 -or
        ($row.paint -ceq 'heavy' -and $metrics.owned_fraction -lt 0.99) -or
        ($row.paint -ceq 'clean' -and $metrics.owned_fraction -gt 0.01)) {
        throw "Crossfeed clean/controlled-heavy ownership is outside its measured fixture bounds: $key"
    }
    $rects = @{}
    foreach ($metric in @('pane_x', 'pane_y', 'pane_width', 'pane_height', 'crop_x', 'crop_y', 'crop_width', 'crop_height')) {
        $value = 0
        if (-not [int]::TryParse($row.$metric, [ref]$value) -or $value -lt 0) {
            throw "Crossfeed native evidence has an invalid $metric for $key"
        }
        $rects[$metric] = $value
    }
    if ($rects.pane_width -le 0 -or $rects.pane_height -le 0 -or $rects.crop_width -le 0 -or $rects.crop_height -le 0 -or
        $rects.pane_x + $rects.pane_width -gt 1280 -or $rects.pane_y + $rects.pane_height -gt 720 -or
        $rects.crop_x -lt $rects.pane_x -or $rects.crop_y -lt $rects.pane_y -or
        $rects.crop_x + $rects.crop_width -gt $rects.pane_x + $rects.pane_width -or
        $rects.crop_y + $rects.crop_height -gt $rects.pane_y + $rects.pane_height) {
        throw "Crossfeed native crop escapes its pane or 720p frame: $key"
    }
    $pane = @($rects.pane_x, $rects.pane_y, $rects.pane_width, $rects.pane_height)
    if ($widget) {
        $crop = @($rects.crop_x, $rects.crop_y, $rects.crop_width, $rects.crop_height)
        if (($pane -join ',') -cne ($crossfeedPreviewPixels -join ',') -or
            ($crop -join ',') -cne ($crossfeedPreviewPixels -join ',') -or
            [Math]::Abs($metrics.camera_scale - $crossfeedPreviewScale) -gt 0.000001 -or
            [Math]::Abs($metrics.camera_x - $crossfeedPreviewCameraX) -gt 0.002 -or
            [Math]::Abs($metrics.camera_y - $crossfeedPreviewCameraY) -gt 0.002 -or
            $metrics.target_x -ne $crossfeedWorld.x -or $metrics.target_y -ne $crossfeedWorld.y -or
            $metrics.target_width -ne $crossfeedWorld.width -or $metrics.target_height -ne $crossfeedWorld.height) {
            throw 'Crossfeed StagePreview receipt differs from the staged fractional pane, edge-truncated crop or world-fit camera.'
        }
    }
    elseif ($row.kind -ceq 'actual-Level-fixture') {
        $viewIndex = [int]$row.view - 1
        $expectedPane = if ($row.players -ceq '1') { @(0, 0, 1280, 720) }
            elseif ($row.players -ceq '2') { @(0, ($viewIndex * 360), 1280, 360) }
            # ViewportManager's three-player order leaves the top-right
            # quadrant unused: top-left, bottom-right, then bottom-left.
            elseif ($row.players -ceq '3') {
                if ($viewIndex -eq 0) { @(0, 0, 640, 360) }
                elseif ($viewIndex -eq 1) { @(640, 360, 640, 360) }
                else { @(0, 360, 640, 360) }
            }
            else { @((($viewIndex % 2) * 640), ([int][Math]::Floor($viewIndex / 2.0) * 360), 640, 360) }
        $expectedScale = 720.0 / $crossfeedPresentation.camera.reference_height
        if (($pane -join ',') -cne ($expectedPane -join ',') -or
            [Math]::Abs($metrics.camera_scale - $expectedScale) -gt 0.000001) {
            throw "Crossfeed player pane differs from its native 720p layout or authored gameplay camera scale: $key"
        }
    }
    $crossfeedDimensions[$row.capture + '.ppm'] = @(1280, 720)
    $crossfeedDimensions[$row.capture + '-view' + $row.view + '-native.ppm'] = @($rects.crop_width, $rects.crop_height)
    $crossfeedCropPixels += [long]$rects.crop_width * $rects.crop_height
}
$crossfeedFiles = @(Get-ChildItem -LiteralPath $captures -Filter '*.ppm' -File)
if ($crossfeedDimensions.Count -ne 72 -or $crossfeedFiles.Count -ne 72 -or
    (($crossfeedFiles.Name | Sort-Object) -join '|') -cne (($crossfeedDimensions.Keys | Sort-Object) -join '|')) {
    throw 'Crossfeed native inspection needs exactly 21 full frames and 51 native crops, without another painter matrix.'
}
$crossfeedPpmBytes = 0L
foreach ($file in $crossfeedFiles) {
    $size = $crossfeedDimensions[$file.Name]
    $header = @(Get-Content -LiteralPath $file.FullName -TotalCount 3)
    $expectedHeader = "P6`n$($size[0]) $($size[1])`n255`n"
    $bytes = [Text.Encoding]::ASCII.GetByteCount($expectedHeader) + [long]$size[0] * $size[1] * 3
    if ($header.Count -ne 3 -or $header[0] -cne 'P6' -or $header[1] -cne "$($size[0]) $($size[1])" -or
        $header[2] -cne '255' -or $file.Length -ne $bytes) {
        throw "Crossfeed native file changed its full-frame/CSV dimensions or exact RGB payload: $($file.Name)"
    }
    $crossfeedPpmBytes += $file.Length
}
$result['observed_counts'] = [ordered]@{
    fixture = $crossfeedSummary; full_frames = 21; native_crops = 51; csv_rows = 51; readbacks = 43
    native_crop_pixels = $crossfeedCropPixels; ppm_bytes = $crossfeedPpmBytes
}
