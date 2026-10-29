# Bounded inventory and native dimensions for the production gate-cue matrix.
$gateSummary = Get-Content -LiteralPath (Join-Path $captures 'gate-permission-summary.json') -Raw | ConvertFrom-Json
$gateRows = @(Import-Csv -LiteralPath (Join-Path $captures 'gate-permission.csv'))
if ($gateSummary.outcome -cne 'passed' -or $gateSummary.level_builds -ne 8 -or
    $gateSummary.level_updates -le 0 -or $gateSummary.level_updates -gt 5000 -or
    $gateSummary.readbacks -ne 492 -or $gateSummary.full_frames -ne 22 -or
    $gateSummary.native_crops -ne 155 -or $gateSummary.rows -ne 112 -or
    $gateSummary.palettes -ne 6 -or $gateSummary.states -ne 7 -or
    $gateSummary.miniature_scale -le 0 -or $gateRows.Count -ne 112) {
    throw 'Gate permission native matrix missed its declared bounded inventory.'
}
$gateExpected = @{}
$gateStates = @('unpainted', 'open-a', 'late-b', 'held-a', 'open-b', 'held-a-tie', 'closed-tie')
$gatePermissions = @(2, 0, 0, 0, 1, 0, 2)
$gateHolds = @(0, 0, 0, 1, 0, 1, 0)
$gateLeaders = @(2, 0, 1, 1, 1, 2, 2)
foreach ($palette in @('blue_orange', 'pink_green', 'blue_yellow', 'purple_gold', 'red_azure', 'magenta_forest')) {
    $counts = if ($palette -ceq 'blue_orange') { @(1, 2, 4) } else { @(4) }
    foreach ($count in $counts) {
        foreach ($path in @('native', 'miniature')) {
            foreach ($state in $gateStates) {
                $row = @($gateRows | Where-Object { $_.palette -ceq $palette -and
                    [int]$_.players -eq $count -and $_.path -ceq $path -and $_.state -ceq $state })
                if ($row.Count -ne 1) { throw 'A gate permission state row is absent or duplicated.' }
                $index = [Array]::IndexOf($gateStates, $state)
                if ([int]$row[0].permission -ne $gatePermissions[$index] -or
                    [int]$row[0].held -ne $gateHolds[$index] -or
                    [int]$row[0].paint_leader -ne $gateLeaders[$index] -or
                    [int]$row[0].crop_width -lt 7 -or [int]$row[0].crop_height -lt 14 -or
                    [int]$row[0].crop_x -lt 0 -or [int]$row[0].crop_y -lt 0 -or
                    [int]$row[0].crop_x + [int]$row[0].crop_width -gt 1280 -or
                    [int]$row[0].crop_y + [int]$row[0].crop_height -gt 720 -or
                    [int]$row[0].changed_pixels -le 0 -or [double]$row[0].scale -le 0 -or
                    ($path -ceq 'miniature' -and [Math]::Abs([double]$row[0].scale - [double]$gateSummary.miniature_scale) -gt 0.000001)) {
                    throw 'Gate cue state, native dimensions, scale or nonempty pixel witness is invalid.'
                }
                $stem = "gate-permission-$palette-$($count)p-$state-$path"
                $size = @([int]$row[0].crop_width, [int]$row[0].crop_height)
                $gateExpected["$stem-detail.ppm"] = $size
                if ($palette -ceq 'blue_orange') { $gateExpected["$stem-ink-detail.ppm"] = $size }
                if ($palette -ceq 'blue_orange' -and $count -eq 1 -and $path -ceq 'miniature' -and $state -ceq 'closed-tie') {
                    $gateExpected['gate-permission-stage-preview-detail.ppm'] = $size
                }
            }
        }
        if ($palette -ceq 'blue_orange') {
            foreach ($state in $gateStates) { $gateExpected["gate-permission-$($count)p-$state-context.ppm"] = @(1280, 720) }
        }
    }
}
$gateExpected['gate-permission-stage-preview-context.ppm'] = @(1280, 720)
$gateFiles = @(Get-ChildItem -LiteralPath $captures -Filter '*.ppm' -File)
if ($gateExpected.Count -ne 177 -or $gateFiles.Count -ne 177 -or
    (($gateFiles.Name | Sort-Object) -join '|') -cne (($gateExpected.Keys | Sort-Object) -join '|')) {
    throw 'Unexpected gate permission native capture inventory.'
}
foreach ($file in $gateFiles) {
    $size = $gateExpected[$file.Name]
    $bytes = [IO.File]::ReadAllBytes($file.FullName)
    $header = "P6`n$($size[0]) $($size[1])`n255`n"
    if ($bytes.Length -ne $header.Length + 3 * $size[0] * $size[1] -or
        [Text.Encoding]::ASCII.GetString($bytes, 0, $header.Length) -cne $header) {
        throw "Invalid gate permission native PPM dimensions: $($file.Name)"
    }
}
$result.gate_permission = $gateSummary
