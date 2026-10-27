# Dot-sourced into the scope of scripts/check-painter.ps1. The guard
# if ($DurationTextOnly) stays there; this is the leaf's post-run evidence check.

        $durationSummary = Get-Content -LiteralPath (Join-Path $captures 'duration-summary.json') -Raw | ConvertFrom-Json
        $durationRows = @(Import-Csv -LiteralPath (Join-Path $captures 'duration-values.csv'))
        $durationExpectedCases = @('default-240', 'cycle-60', 'cycle-120', 'explicit-240', 'left-120',
            'confirm-240', 'wrapped-default', 'fractional-90', 'fractional-61', 'scientific-off-choice',
            'off-choice-right', 'numeric-live-before', 'live-explicit-90', 'live-default-90', 'old-default',
            'retained-old-after-assignment', 'old-live-explicit', 'old-live-default', 'reopened-new-default',
            'retained-new-after-reversal', 'new-live-explicit', 'new-live-default', 'resized-1080',
            'resized-720', 'restored-default')
        if ($durationSummary.outcome -ne 'passed' -or $durationSummary.scope -ne 'duration-text-only' -or
            $durationSummary.updates -ne 86 -or $durationSummary.readbacks -ne 76 -or
            $durationSummary.csv_rows -ne 25 -or $durationSummary.compact_crops -ne 5 -or
            $durationSummary.page_openings -ne 8 -or $durationSummary.font_negative_controls -ne 1 -or
            $durationSummary.resize_updates -ne 0 -or $durationSummary.save_calls -ne 1 -or
            $durationSummary.crop_pixels -ne 420480 -or $durationRows.Count -ne 25 -or
            @($durationRows | Where-Object {
                $_.focus -ne '3' -or $_.scale -ne '1' -or $_.full_row_match -ne 'true' -or
                $_.ornament_inside -ne 'true' -or $_.frozen_state_rng -ne 'true' -or
                ($_.case -eq 'resized-1080' -and ($_.width -ne '1920' -or $_.height -ne '1080')) -or
                ($_.case -ne 'resized-1080' -and ($_.width -ne '1280' -or $_.height -ne '720'))
            }).Count -ne 0 -or
            (($durationRows.case | Sort-Object) -join '|') -ne (($durationExpectedCases | Sort-Object) -join '|')) {
            throw 'The duration leaf evidence does not match its frozen update, row, font-control and readback/crop budgets.'
        }
        foreach ($row in $durationRows) {
            foreach ($metric in @('value_width', 'value_height', 'row_width', 'row_height')) {
                $value = 0.0
                if (-not [double]::TryParse($row.$metric, [Globalization.NumberStyles]::Float,
                    [Globalization.CultureInfo]::InvariantCulture, [ref]$value) -or
                    [double]::IsNaN($value) -or [double]::IsInfinity($value) -or $value -le 0.0) {
                    throw "Duration case $($row.case) contains an invalid $metric."
                }
            }
        }
        $durationExpectedNames = @('duration-text-default.ppm', 'duration-text-fraction.ppm',
            'duration-text-scientific.ppm', 'duration-text-old-long.ppm', 'duration-text-new-long.ppm')
        $durationCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'duration-*.ppm' -File)
        $durationCsvNames = @(Get-ChildItem -LiteralPath $captures -Filter 'duration-*.csv' -File | Select-Object -ExpandProperty Name)
        $durationSummaryNames = @(Get-ChildItem -LiteralPath $captures -Filter 'duration-*.json' -File | Select-Object -ExpandProperty Name)
        $durationExpectedSummaryNames = @('duration-summary.json')
        if ($debug) {
            $durationTargetSummary = Get-Content -LiteralPath (Join-Path $captures 'duration-target-summary.json') -Raw | ConvertFrom-Json
            if ($durationTargetSummary.outcome -ne 'passed' -or $durationTargetSummary.scope -ne 'duration-targets-only' -or
                $durationTargetSummary.targets -ne 2 -or $durationTargetSummary.legacy_candidate_acceptances -ne 2 -or
                $durationTargetSummary.paired_refusals -ne 2 -or $durationTargetSummary.primed_candidates -ne 2 -or
                $durationTargetSummary.target_refusals -ne 2 -or $durationTargetSummary.failed_apply_inert -ne 2 -or
                $durationTargetSummary.valid_retries -ne 2 -or $durationTargetSummary.consumed_candidates -ne 2 -or
                $durationTargetSummary.target_validate_calls -ne 6 -or $durationTargetSummary.candidate_writes -ne 6 -or
                $durationTargetSummary.state_updates -ne 0 -or $durationTargetSummary.readbacks -ne 0 -or
                $durationTargetSummary.save_calls -ne 0 -or $durationTargetSummary.real_content_unchanged -ne $true -or
                $durationTargetSummary.frozen_rng -ne $true) {
                throw 'The duration Debug target evidence does not match its frozen resource-refusal/retry contract.'
            }
            $durationExpectedSummaryNames += 'duration-target-summary.json'
        }
        elseif (@(Get-ChildItem -LiteralPath $captures -Filter 'duration-target-*').Count -ne 0) {
            throw 'Release duration evidence must not contain Debug target artifacts.'
        }
        if ($durationCrops.Count -ne 5 -or
            (($durationCrops.Name | Sort-Object) -join '|') -ne (($durationExpectedNames | Sort-Object) -join '|') -or
            ($durationCsvNames -join '|') -ne 'duration-values.csv' -or
            (($durationSummaryNames | Sort-Object) -join '|') -ne (($durationExpectedSummaryNames | Sort-Object) -join '|')) {
            throw 'The duration leaf emitted unexpected or missing compact crops, CSV or summary files.'
        }
        foreach ($crop in $durationCrops) {
            $header = @(Get-Content -LiteralPath $crop.FullName -TotalCount 3)
            $expectedBytes = [Text.Encoding]::ASCII.GetByteCount("P6`n1168 72`n255`n") + 1168 * 72 * 3
            if ($header.Count -ne 3 -or $header[0] -ne 'P6' -or $header[1] -ne '1168 72' -or
                $header[2] -ne '255' -or $crop.Length -ne $expectedBytes) {
                throw "Duration crop $($crop.Name) differs from its exact 1168x72 RGB payload budget."
            }
        }
        $durationSaveCalls = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -match 'Saving\.\.\.$' }).Count
        $durationSaveSuccesses = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -eq 'Save was successful.' }).Count
        if ($durationSaveCalls -ne 1 -or $durationSaveSuccesses -ne 1) {
            throw 'The duration leaf must perform exactly its one successful private Play-settings seed Save.'
        }
        $result['observed_counts'] = [ordered]@{
            fixture = $durationSummary
            updates = 86
            readbacks = 76
            csv_rows = 25
            crops = 5
            private_save_calls = $durationSaveCalls
            successful_private_saves = $durationSaveSuccesses
        }
        if ($debug) { $result.observed_counts['target_validation'] = $durationTargetSummary }
