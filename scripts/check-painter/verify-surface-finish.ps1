# Exact bounded evidence for the focused surface finish fixture.
$surfaceSummary = Get-Content -LiteralPath (Join-Path $captures 'surface-finish-summary.json') -Raw | ConvertFrom-Json
$surfaceRows = @(Import-Csv -LiteralPath (Join-Path $captures 'surface-finish.csv'))
if ($surfaceSummary.outcome -cne 'passed' -or $surfaceSummary.full_frames -ne 20 -or
    $surfaceSummary.pane_crops -ne 50 -or $surfaceSummary.synthetic_crops -ne 2 -or
    $surfaceSummary.readbacks -ne 52 -or $surfaceSummary.level_updates -le 0 -or
    $surfaceSummary.level_updates -ge 7000 -or $surfaceSummary.heavy_contacts -le 0 -or $surfaceRows.Count -ne 50) {
    throw 'Surface finish fixture exceeded or missed its bounded evidence counts.'
}
$surfaceExpected = @{}
foreach ($stage in @('close_quarters', 'king_of_the_hill', 'turbulence', 'movement_course', 'crossfeed')) {
    foreach ($count in @(1, 4)) {
        foreach ($paint in @('clean', 'heavy')) {
            $stem = "surface-finish-$stage-$($count)p-$paint"
            $surfaceExpected["$stem-context.ppm"] = @(1280, 720)
            $rows = @($surfaceRows | Where-Object { $_.stage -ceq $stage -and [int]$_.players -eq $count -and $_.paint -ceq $paint })
            if ($rows.Count -ne $count -or (@($rows.view | Sort-Object -Unique).Count -ne $count)) { throw 'Surface pane rows are missing or duplicated.' }
            foreach ($row in $rows) {
                $view = [int]$row.view
                $width = if ($count -eq 1) { 1280 } else { 640 }
                $height = if ($count -eq 1) { 720 } else { 360 }
                $left = if ($count -eq 4 -and $view % 2 -ne 0) { 640 } else { 0 }
                $top = if ($count -eq 4 -and $view -ge 2) { 360 } else { 0 }
                if ($view -lt 0 -or $view -ge $count -or [int]$row.pane_width -ne $width -or
                    [int]$row.pane_height -ne $height -or [int]$row.pane_x -ne $left -or [int]$row.pane_y -ne $top -or
                    [double]$row.scale -le 0 -or ($paint -ceq 'clean' -and [double]$row.owned_fraction -ne 0) -or
                    ($paint -ceq 'heavy' -and ([double]$row.owned_fraction -le 0.99 -or [double]$row.owned_fraction -gt 1))) {
                    throw 'Surface native pane dimensions, layout or measured ownership are invalid.'
                }
                $surfaceExpected["$stem-player$($view + 1).ppm"] = @($width, $height)
            }
        }
    }
}
foreach ($count in @(1, 4)) { $surfaceExpected["surface-finish-board-$($count)p-detail.ppm"] = @(640, 360) }
$surfaceFiles = @(Get-ChildItem -LiteralPath $captures -Filter '*.ppm' -File)
if ($surfaceFiles.Count -ne 72 -or (($surfaceFiles.Name | Sort-Object) -join '|') -cne
    (($surfaceExpected.Keys | Sort-Object) -join '|')) { throw 'Unexpected surface finish native capture set.' }
foreach ($file in $surfaceFiles) {
    $size = $surfaceExpected[$file.Name]
    $bytes = [IO.File]::ReadAllBytes($file.FullName)
    $header = "P6`n$($size[0]) $($size[1])`n255`n"
    if ($bytes.Length -ne $header.Length + 3 * $size[0] * $size[1] -or
        [Text.Encoding]::ASCII.GetString($bytes, 0, $header.Length) -cne $header) {
        throw "Invalid surface native PPM dimensions: $($file.Name)"
    }
}
$result.surface_finish = $surfaceSummary
