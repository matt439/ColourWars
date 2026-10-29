# Bounded inventories, state receipts and native PPM sizes for ROAD#27.
$ferrySummary = Get-Content -LiteralPath (Join-Path $captures 'ferry-direction-summary.json') -Raw | ConvertFrom-Json
$ferryRows = @(Import-Csv -LiteralPath (Join-Path $captures 'ferry-direction.csv'))
if ($ferrySummary.outcome -cne 'passed' -or $ferrySummary.level_builds -ne 6 -or
    $ferrySummary.level_updates -le 0 -or $ferrySummary.level_updates -gt 5000 -or
    $ferrySummary.readbacks -ne 424 -or $ferrySummary.full_frames -ne 43 -or
    $ferrySummary.native_crops -ne 169 -or $ferrySummary.rows -ne 84 -or
    $ferrySummary.states -ne 7 -or $ferrySummary.headings -ne 2 -or
    $ferrySummary.miniature_scale -le 0 -or $ferryRows.Count -ne 84) {
    throw 'Ferry direction native matrix missed its declared bounded inventory.'
}
$ferryExpected = @{}
$ferryStates = @('unpainted', 'a-moving', 'late-b', 'b-moving', 'b-stop', 'tie', 'a-stop')
$ferryTeams = @(2, 0, 1, 1, 1, 2, 0)
foreach ($heading in @('right', 'left')) {
    $a = if ($heading -ceq 'right') { 1 } else { -1 }
    $demands = @(0, $a, -$a, -$a, -$a, 0, $a)
    $committed = @(0, $a, $a, -$a, -$a, 0, $a)
    $movements = @(0, $a, $a, -$a, 0, 0, 0)
    $motions = @(1, 2, 2, 2, $(if ($a -eq 1) { 3 } else { 4 }), 1, $(if ($a -eq 1) { 4 } else { 3 }))
    foreach ($count in @(1, 2, 4)) {
        foreach ($state in $ferryStates) {
            $index = [Array]::IndexOf($ferryStates, $state)
            $stem = "ferry-direction-$heading-$($count)p-$state"
            $ferryExpected["$stem-context.ppm"] = @(1280, 720)
            foreach ($path in @('native', 'miniature')) {
                $row = @($ferryRows | Where-Object { $_.heading -ceq $heading -and [int]$_.players -eq $count -and
                    $_.state -ceq $state -and $_.path -ceq $path })
                if ($row.Count -ne 1) { throw 'A ferry state row is absent or duplicated.' }
                $r = $row[0]
                if ([int]$r.demand_team -ne $ferryTeams[$index] -or [int]$r.demand_heading -ne $demands[$index] -or
                    [int]$r.committed_heading -ne $committed[$index] -or [int]$r.movement_heading -ne $movements[$index] -or
                    [int]$r.motion -ne $motions[$index] -or [int]$r.crop_width -lt 17 -or [int]$r.crop_height -lt 9 -or
                    [int]$r.crop_x -lt 0 -or [int]$r.crop_y -lt 0 -or [int]$r.crop_x + [int]$r.crop_width -gt 1280 -or
                    [int]$r.crop_y + [int]$r.crop_height -gt 720 -or [int]$r.changed_pixels -le 0 -or [double]$r.scale -le 0 -or
                    ($path -ceq 'miniature' -and [Math]::Abs([double]$r.scale - [double]$ferrySummary.miniature_scale) -gt 0.000001)) {
                    throw 'A ferry state receipt, native bounds, scale or nonempty pixel witness is invalid.'
                }
                $size = @([int]$r.crop_width, [int]$r.crop_height)
                $ferryExpected["$stem-$path-detail.ppm"] = $size
                $ferryExpected["$stem-$path-ink-detail.ppm"] = $size
                if ($heading -ceq 'right' -and $count -eq 1 -and $state -ceq 'unpainted' -and $path -ceq 'miniature') {
                    $ferryExpected['ferry-direction-stage-preview-detail.ppm'] = $size
                }
            }
        }
    }
}
$ferryExpected['ferry-direction-stage-preview-context.ppm'] = @(1280, 720)
$ferryFiles = @(Get-ChildItem -LiteralPath $captures -Filter '*.ppm' -File)
if ($ferryExpected.Count -ne 212 -or $ferryFiles.Count -ne 212 -or
    (($ferryFiles.Name | Sort-Object) -join '|') -cne (($ferryExpected.Keys | Sort-Object) -join '|')) {
    throw 'Unexpected ferry direction native capture inventory.'
}
foreach ($file in $ferryFiles) {
    $size = $ferryExpected[$file.Name]; $bytes = [IO.File]::ReadAllBytes($file.FullName)
    $header = "P6`n$($size[0]) $($size[1])`n255`n"
    if ($bytes.Length -ne $header.Length + 3 * $size[0] * $size[1] -or
        [Text.Encoding]::ASCII.GetString($bytes, 0, $header.Length) -cne $header) {
        throw "Invalid ferry direction native PPM dimensions: $($file.Name)"
    }
}
$result.ferry_direction = $ferrySummary
