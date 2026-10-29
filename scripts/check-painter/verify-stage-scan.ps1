# ROAD#32's isolated scans, bounded native page receipts and capture inventory.
$stageScanSummary = Get-Content -LiteralPath (Join-Path $captures 'stage-scan-summary.json') -Raw | ConvertFrom-Json
$stageScanRows = @(Import-Csv -LiteralPath (Join-Path $captures 'stage-scan.csv'))
if ($stageScanSummary.outcome -cne 'passed' -or $stageScanSummary.updates -le 0 -or $stageScanSummary.updates -gt 500 -or
    $stageScanSummary.level_updates -le 0 -or $stageScanSummary.level_updates -gt 400 -or
    $stageScanSummary.readbacks -ne 238 -or $stageScanSummary.scans -ne 6 -or $stageScanSummary.stderr_lines -ne 3 -or
    $stageScanSummary.comparisons -ne 156 -or $stageScanSummary.rows -ne 156 -or $stageScanRows.Count -ne 156 -or
    $stageScanSummary.full_frames -ne 10 -or $stageScanSummary.native_crops -ne 5 -or $stageScanSummary.sizes -ne 6 -or
    $stageScanSummary.long_pages -lt 3 -or $stageScanSummary.long_pages -gt 6) {
    throw 'Stage-scan native evidence missed its declared bounded inventory.'
}
$stageScanSizes = @(@(1280,720), @(1920,1080), @(2560,1440), @(1600,1200), @(1920,1200), @(2560,1080))
$stageScanStates = @('long-first', 'long-next', 'long-last', 'invalid-bytes', 'file-wrap-reset')
$stageScanExpected = @{}
foreach ($size in $stageScanSizes) {
    $stageScanExpected["stage-scan-long-first-$($size[0])x$($size[1])-context.ppm"] = $size
    foreach ($state in $stageScanStates) {
        foreach ($role in @('heading','unicode','body','progress','prompt')) {
            $found = @($stageScanRows | Where-Object { $_.case -ceq $state -and $_.role -ceq $role -and
                [int]$_.width -eq $size[0] -and [int]$_.height -eq $size[1] })
            if ($found.Count -ne 1) { throw 'Missing or repeated stage-scan native state/role/size receipt.' }
        }
    }
}
foreach ($row in $stageScanRows) {
    if ($row.raster_equal -cne 'true' -or [double]$row.text_width -le 0 -or [double]$row.text_height -le 0 -or
        [int]$row.crop_width -le 0 -or [int]$row.crop_height -le 0 -or [int]$row.crop_x -lt 0 -or [int]$row.crop_y -lt 0 -or
        [int]$row.crop_x + [int]$row.crop_width -gt [int]$row.width -or [int]$row.crop_y + [int]$row.crop_height -gt [int]$row.height) {
        throw 'A stage-scan text comparison or projected complete-font bound is invalid.'
    }
}
foreach ($state in @('actual-refusal','long-last','invalid-bytes')) {
    $stageScanExpected["stage-scan-$state-1280x720-context.ppm"] = @(1280,720)
}
$stageScanExpected['stage-scan-parent-status-1280x720-context.ppm'] = @(1280,720)
$status = @($stageScanRows | Where-Object { $_.case -ceq 'parent-status' -and $_.role -ceq 'status' -and [int]$_.width -eq 1280 -and [int]$_.height -eq 720 })
if ($status.Count -ne 1) { throw 'Missing independent Stage Select status literal/font/fit receipt.' }
$stageScanExpected['stage-scan-parent-status-detail.ppm'] = @([int]$status[0].crop_width, [int]$status[0].crop_height)
foreach ($state in @('actual-refusal','long-first','long-last','invalid-bytes')) {
    $body = @($stageScanRows | Where-Object { $_.case -ceq $state -and $_.role -ceq 'body' -and [int]$_.width -eq 1280 -and [int]$_.height -eq 720 })
    if ($body.Count -ne 1) { throw 'Missing stage-scan compact native-body crop receipt.' }
    $stageScanExpected["stage-scan-$state-detail.ppm"] = @([int]$body[0].crop_width, [int]$body[0].crop_height)
}
$stageScanFiles = @(Get-ChildItem -LiteralPath $captures -Filter '*.ppm' -File)
if ($stageScanExpected.Count -ne 15 -or $stageScanFiles.Count -ne 15 -or
    (($stageScanFiles.Name | Sort-Object) -join '|') -cne (($stageScanExpected.Keys | Sort-Object) -join '|')) {
    throw 'Unexpected stage-scan native capture inventory.'
}
foreach ($file in $stageScanFiles) {
    $size = $stageScanExpected[$file.Name]; $bytes = [IO.File]::ReadAllBytes($file.FullName)
    $header = "P6`n$($size[0]) $($size[1])`n255`n"
    if ($bytes.Length -ne $header.Length + 3 * $size[0] * $size[1] -or
        [Text.Encoding]::ASCII.GetString($bytes, 0, $header.Length) -cne $header) {
        throw "Invalid stage-scan PPM dimensions: $($file.Name)"
    }
}
$stageScanDiagnostics = @(Get-Content -LiteralPath (Join-Path $captures 'stage-scan-stderr.txt'))
if ($stageScanDiagnostics.Count -ne 3 -or @($stageScanDiagnostics | Where-Object { $_ -notmatch '^user stage refused: .*broken_stage.json: ' }).Count -ne 0 -or
    $stageScanDiagnostics[0] -cne $stageScanDiagnostics[2] -or $stageScanDiagnostics[0] -ceq $stageScanDiagnostics[1]) {
    throw 'Isolated stage-scan stderr lost exact repeat suppression or changed-reason reporting.'
}
$result.stage_scan = $stageScanSummary
