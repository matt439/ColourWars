# Dot-sourced into the scope of scripts/check-painter.ps1. The guard
# if ($ResolutionTextOnly) stays there; this is the leaf's post-run evidence check.

        $resolutionSummary = Get-Content -LiteralPath (Join-Path $captures 'resolution-summary.json') -Raw | ConvertFrom-Json
        $resolutionRows = @(Import-Csv -LiteralPath (Join-Path $captures 'resolution-values.csv'))
        $resolutionExpectedCases = @('saved-windowed-preset', 'left-1280', 'left-wrap-3840', 'right-wrap-1280',
            'right-2560', 'pending-on-enabled', 'pending-on-cycle', 'old-width-height', 'retained-old-source',
            'old-short-height', 'reopened-new-width', 'retained-new-reversal', 'new-short-reset', 'new-long-refit',
            'new-short-reset-again', 'resized-1080', 'resized-720', 'restored-windowed', 'fullscreen-live-client',
            'pending-off-disabled', 'disabled-skip-back', 'fullscreen-retained-size', 'fullscreen-defaults-refresh',
            'fullscreen-reopened-client', 'metadata-windowed-preset')
        $resolutionExpectedValues = @('1920x1080', '1280x720', '3840x2160', '1280x720', '2560x1440',
            '2560x1440', '1920x1080', 'OLD~ display pixels: 1920x1080', 'OLD~ display pixels: 1920x1080',
            'OLD~ display pixels: 1280x720', 'Next display pixels: 1920x1080.', 'Next display pixels: 1920x1080.',
            'Next display pixels: 1280x720.', 'Next display pixels: 3840x2160.', 'Next display pixels: 1280x720.',
            'Next display pixels: 1280x720.', 'Next display pixels: 1280x720.', '1920x1080',
            'Next display pixels: 1600x900.', 'Next display pixels: 1600x900.', 'Next display pixels: 1600x900.',
            'Next display pixels: 1600x900.', 'Next display pixels: 1001x777.', '1001x777', '3840x2160')
        $resolutionExpectedColumns = @('case', 'width', 'height', 'focus', 'saved_preset', 'saved_fullscreen',
            'pending_fullscreen', 'enabled', 'expected_value', 'font', 'raw_width', 'raw_height', 'width_scale',
            'height_scale', 'scale', 'row_width', 'row_height', 'fit', 'full_row_match', 'ornament_inside', 'frozen_state_rng')
        if ($resolutionSummary.outcome -ne 'passed' -or $resolutionSummary.scope -ne 'resolution-text-only' -or
            $resolutionSummary.updates -ne 43 -or $resolutionSummary.readbacks -ne 76 -or
            $resolutionSummary.csv_rows -ne 25 -or $resolutionSummary.compact_crops -ne 6 -or
            $resolutionSummary.page_openings -ne 7 -or $resolutionSummary.font_negative_controls -ne 1 -or
            $resolutionSummary.resize_updates -ne 0 -or $resolutionSummary.save_calls -ne 3 -or
            $resolutionSummary.crop_pixels -ne 1050840 -or $resolutionSummary.metadata_only -ne $true -or
            $resolutionSummary.windowed_application -ne $true -or $resolutionRows.Count -ne 25 -or
            (($resolutionRows[0].PSObject.Properties.Name) -join '|') -ne ($resolutionExpectedColumns -join '|') -or
            (($resolutionRows.case | Sort-Object) -join '|') -ne (($resolutionExpectedCases | Sort-Object) -join '|')) {
            throw 'The resolution leaf does not match its frozen summary, CSV schema, cases or update/readback budgets.'
        }
        foreach ($row in $resolutionRows) {
            $caseIndex = [array]::IndexOf($resolutionExpectedCases, $row.case) + 1
            $expectedWidth = 1280
            $expectedHeight = 720
            if ($caseIndex -eq 16) { $expectedWidth = 1920; $expectedHeight = 1080 }
            elseif ($caseIndex -eq 19) { $expectedWidth = 1600; $expectedHeight = 900 }
            elseif ($caseIndex -ge 20) { $expectedWidth = 1001; $expectedHeight = 777 }
            $expectedFocus = if ($caseIndex -in @(19, 20, 22, 24)) { '1' }
                elseif ($caseIndex -eq 21) { '6' } elseif ($caseIndex -eq 23) { '4' } else { '0' }
            $expectedPreset = if ($caseIndex -ge 19) { '3840x2160' } else { '1920x1080' }
            $expectedSavedFullscreen = if ($caseIndex -ge 19 -and $caseIndex -le 24) { 'true' } else { 'false' }
            $expectedPendingFullscreen = if ($caseIndex -in @(6, 7, 19, 22, 24)) { 'true' } else { 'false' }
            $expectedEnabled = if ($expectedSavedFullscreen -eq 'true') { 'false' } else { 'true' }
            $expectedFont = if ($caseIndex -ge 8 -and $caseIndex -le 10) { 'resolution_tall_tilde' }
                elseif (($caseIndex -ge 11 -and $caseIndex -le 17) -or
                    ($caseIndex -ge 19 -and $caseIndex -le 23)) { 'resolution_numeric_shift' } else { 'lato_black_48' }
            $expectedFit = if ($caseIndex -in @(8, 9)) { 'width_then_height' } elseif ($caseIndex -eq 10) { 'height' }
                elseif ($caseIndex -in @(11, 12, 14)) { 'width' }
                elseif ($caseIndex -in @(13, 15, 16, 17)) { 'reset' } else { 'none' }
            if ($row.width -ne [string]$expectedWidth -or $row.height -ne [string]$expectedHeight -or
                $row.focus -ne $expectedFocus -or $row.saved_preset -ne $expectedPreset -or
                $row.saved_fullscreen -ne $expectedSavedFullscreen -or $row.pending_fullscreen -ne $expectedPendingFullscreen -or
                $row.enabled -ne $expectedEnabled -or $row.font -ne $expectedFont -or $row.fit -ne $expectedFit -or
                $row.expected_value -ne $resolutionExpectedValues[$caseIndex - 1] -or
                $row.full_row_match -ne 'true' -or $row.ornament_inside -ne 'true' -or $row.frozen_state_rng -ne 'true' -or
                [string]::IsNullOrWhiteSpace($row.expected_value) -or $row.expected_value -match '[^\x20-\x7e]') {
                throw "Resolution case $($row.case) changed its fixed viewport/focus, capture, fullscreen-metadata or oracle contract."
            }
            foreach ($metric in @('raw_width', 'raw_height', 'width_scale', 'height_scale', 'scale', 'row_width', 'row_height')) {
                $value = 0.0
                if (-not [double]::TryParse($row.$metric, [Globalization.NumberStyles]::Float,
                    [Globalization.CultureInfo]::InvariantCulture, [ref]$value) -or
                    [double]::IsNaN($value) -or [double]::IsInfinity($value) -or $value -le 0.0 -or
                    ($metric -eq 'scale' -and $value -gt 1.0)) {
                    throw "Resolution case $($row.case) contains an invalid $metric."
                }
            }
        }
        $resolutionExpectedCrops = [ordered]@{
            'resolution-text-default.ppm' = @(1112, 144)
            'resolution-text-wrap.ppm' = @(1112, 144)
            'resolution-text-old-fit.ppm' = @(1112, 144)
            'resolution-text-new-width.ppm' = @(1112, 144)
            'resolution-text-new-reset.ppm' = @(1112, 144)
            'resolution-text-fullscreen.ppm' = @(1390, 180)
        }
        $resolutionCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'resolution-*.ppm' -File)
        $resolutionExpectedNames = @($resolutionExpectedCrops.Keys) + @('resolution-values.csv', 'resolution-summary.json')
        $resolutionNames = @(Get-ChildItem -LiteralPath $captures -Filter 'resolution-*' -File | Select-Object -ExpandProperty Name)
        if ($resolutionCrops.Count -ne 6 -or
            (($resolutionNames | Sort-Object) -join '|') -ne (($resolutionExpectedNames | Sort-Object) -join '|')) {
            throw 'The resolution leaf emitted unexpected or missing compact crops, CSV or summary files.'
        }
        foreach ($crop in $resolutionCrops) {
            $size = $resolutionExpectedCrops[$crop.Name]
            $header = @(Get-Content -LiteralPath $crop.FullName -TotalCount 3)
            $expectedHeader = "P6`n$($size[0]) $($size[1])`n255`n"
            $expectedBytes = [Text.Encoding]::ASCII.GetByteCount($expectedHeader) + $size[0] * $size[1] * 3
            if ($header.Count -ne 3 -or $header[0] -ne 'P6' -or $header[1] -ne "$($size[0]) $($size[1])" -or
                $header[2] -ne '255' -or $crop.Length -ne $expectedBytes) {
                throw "Resolution crop $($crop.Name) differs from its exact dimensions or RGB payload budget."
            }
        }
        $resolutionSaveCalls = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -match 'Saving\.\.\.$' }).Count
        $resolutionSaveSuccesses = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -eq 'Save was successful.' }).Count
        if ($resolutionSaveCalls -ne 3 -or $resolutionSaveSuccesses -ne 3) {
            throw 'The resolution leaf must perform exactly its three successful private metadata seed Saves.'
        }
        $result['observed_counts'] = [ordered]@{
            fixture = $resolutionSummary
            updates = 43
            readbacks = 76
            csv_rows = 25
            crops = 6
            private_save_calls = $resolutionSaveCalls
            successful_private_saves = $resolutionSaveSuccesses
        }
