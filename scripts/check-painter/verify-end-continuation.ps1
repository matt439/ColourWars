# Dot-sourced into the scope of scripts/check-painter.ps1. The guard
# if ($EndContinuationOnly) stays there; this is the leaf's post-run evidence check.

        $endContinuationSummary = Get-Content -LiteralPath (Join-Path $captures 'end-continuation-summary.json') -Raw | ConvertFrom-Json
        $endContinuationRows = @(Import-Csv -LiteralPath (Join-Path $captures 'end-continuation.csv'))
        $endContinuationSessions = @('single-solo', 'single-two-team', 'series3-complete-a',
            'series5-complete-b', 'series3-draw', 'series5-next-a')
        $endContinuationCapturedSessions = @('single-two-team', 'series3-complete-a', 'series3-draw', 'series5-next-a')
        $endContinuationExpectedPairs = @(
            foreach ($case in @('legacy-primary', 'legacy-secondary')) {
                foreach ($session in $endContinuationSessions) { "$case|$session" }
            }
            foreach ($case in @('old-primary', 'retained-old', 'reopened-new', 'retained-new')) {
                foreach ($session in $endContinuationCapturedSessions) { "$case|$session" }
            }
            'pending-music|single-two-team'
            'resized-1080|series5-next-a'
            'resized-720|series5-next-a'
        )
        $endContinuationExpectedColumns = @('case', 'session', 'width', 'height', 'focus', 'series', 'complete',
            'round', 'a_wins', 'b_wins', 'music_pending', 'expected_value', 'font', 'raw_width', 'raw_height',
            'scale', 'primary_persistent', 'full_ornament_match', 'frozen_session_setup_save_rng')
        if ($endContinuationSummary.outcome -ne 'passed' -or $endContinuationSummary.scope -ne 'end-continuation-only' -or
            $endContinuationSummary.updates -ne 68 -or $endContinuationSummary.readbacks -ne 94 -or
            $endContinuationSummary.csv_rows -ne 31 -or $endContinuationSummary.compact_crops -ne 6 -or
            $endContinuationSummary.page_openings -ne 14 -or $endContinuationSummary.font_negative_controls -ne 1 -or
            $endContinuationSummary.resize_updates -ne 0 -or $endContinuationSummary.save_calls -ne 1 -or
            $endContinuationSummary.crop_pixels -ne 311040 -or $endContinuationSummary.typed_actions -ne 14 -or
            $endContinuationSummary.restart_actions -ne 13 -or $endContinuationSummary.exit_actions -ne 1 -or
            $endContinuationSummary.deferred_actions -ne 4 -or $endContinuationSummary.music_starts -ne 4 -or
            $endContinuationRows.Count -ne 31 -or
            (($endContinuationRows[0].PSObject.Properties.Name) -join '|') -ne ($endContinuationExpectedColumns -join '|') -or
            (($endContinuationRows | ForEach-Object { "$($_.case)|$($_.session)" } | Sort-Object) -join ';') -ne
                (($endContinuationExpectedPairs | Sort-Object) -join ';')) {
            throw 'The End continuation leaf does not match its frozen summary, CSV schema, cases or action/update/readback budgets.'
        }
        $endContinuationSessionState = @{
            'single-solo' = @('false', 'true', '1', '0', '0')
            'single-two-team' = @('false', 'true', '1', '0', '1')
            'series3-complete-a' = @('true', 'true', '2', '2', '0')
            'series5-complete-b' = @('true', 'true', '3', '0', '3')
            'series3-draw' = @('true', 'false', '1', '0', '0')
            'series5-next-a' = @('true', 'false', '1', '1', '0')
        }
        $endContinuationLegacyValues = @{
            'single-solo' = 'Rematch'; 'single-two-team' = 'Rematch'
            'series3-complete-a' = 'Play Series Again'; 'series5-complete-b' = 'Play Series Again'
            'series3-draw' = 'Replay Draw'; 'series5-next-a' = 'Next Round'
        }
        $endContinuationOldValues = @{
            'single-two-team' = 'Rematch~ {1}'; 'series3-complete-a' = 'Again~ {3}'
            'series3-draw' = 'Draw~ {0}'; 'series5-next-a' = 'Next~ {2}'
        }
        $endContinuationNewValues = @{
            'single-two-team' = 'Replay {1}'; 'series3-complete-a' = 'Series {3}'
            'series3-draw' = 'Draw {0}'; 'series5-next-a' = 'Round {2}'
        }
        foreach ($row in $endContinuationRows) {
            $expectedWidth = if ($row.case -eq 'resized-1080') { '1920' } else { '1280' }
            $expectedHeight = if ($row.case -eq 'resized-1080') { '1080' } else { '720' }
            $expectedFocus = if ($row.case -eq 'legacy-secondary') { '1' } else { '0' }
            $expectedPending = if ($row.case -eq 'pending-music') { 'true' } else { 'false' }
            $newCopy = $row.case -in @('reopened-new', 'retained-new', 'resized-1080', 'resized-720')
            $expectedFont = if ($newCopy) { 'end_continuation_numeric_letters' } else { 'gill_sans_mt_bold_48' }
            $expectedValue = if ($newCopy) { $endContinuationNewValues[$row.session] }
                elseif ($row.case -in @('old-primary', 'retained-old', 'pending-music')) { $endContinuationOldValues[$row.session] }
                else { $endContinuationLegacyValues[$row.session] }
            $expectedState = $endContinuationSessionState[$row.session]
            if ($row.width -ne $expectedWidth -or $row.height -ne $expectedHeight -or
                $row.focus -ne $expectedFocus -or $row.series -ne $expectedState[0] -or
                $row.complete -ne $expectedState[1] -or $row.round -ne $expectedState[2] -or
                $row.a_wins -ne $expectedState[3] -or $row.b_wins -ne $expectedState[4] -or
                $row.music_pending -ne $expectedPending -or $row.expected_value -cne $expectedValue -or
                $row.font -ne $expectedFont -or $row.scale -ne '1' -or
                $row.primary_persistent -ne 'true' -or $row.full_ornament_match -ne 'true' -or
                $row.frozen_session_setup_save_rng -ne 'true') {
                throw "End continuation case $($row.case)/$($row.session) changed its literal copy, session ledger, capture or ornament contract."
            }
            foreach ($metric in @('raw_width', 'raw_height')) {
                $value = 0.0
                if (-not [double]::TryParse($row.$metric, [Globalization.NumberStyles]::Float,
                    [Globalization.CultureInfo]::InvariantCulture, [ref]$value) -or
                    [double]::IsNaN($value) -or [double]::IsInfinity($value) -or $value -le 0.0) {
                    throw "End continuation case $($row.case)/$($row.session) contains an invalid $metric."
                }
            }
        }
        $endContinuationExpectedCrops = @('end-continuation-rematch.ppm', 'end-continuation-series-again.ppm',
            'end-continuation-replay-draw.ppm', 'end-continuation-next-round.ppm',
            'end-continuation-old.ppm', 'end-continuation-new.ppm')
        $endContinuationCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'end-continuation-*.ppm' -File)
        $endContinuationExpectedNames = $endContinuationExpectedCrops + @('end-continuation.csv', 'end-continuation-summary.json')
        $endContinuationNames = @(Get-ChildItem -LiteralPath $captures -Filter 'end-continuation*' -File | Select-Object -ExpandProperty Name)
        if ($endContinuationCrops.Count -ne 6 -or
            (($endContinuationNames | Sort-Object) -join '|') -ne (($endContinuationExpectedNames | Sort-Object) -join '|')) {
            throw 'The End continuation leaf emitted unexpected or missing compact crops, CSV or summary files.'
        }
        foreach ($crop in $endContinuationCrops) {
            $header = @(Get-Content -LiteralPath $crop.FullName -TotalCount 3)
            $expectedHeader = "P6`n540 96`n255`n"
            $expectedBytes = [Text.Encoding]::ASCII.GetByteCount($expectedHeader) + 540 * 96 * 3
            if ($header.Count -ne 3 -or $header[0] -ne 'P6' -or $header[1] -ne '540 96' -or
                $header[2] -ne '255' -or $crop.Length -ne $expectedBytes) {
                throw "End continuation crop $($crop.Name) differs from its exact dimensions or RGB payload budget."
            }
        }
        $endContinuationSaveCalls = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -match 'Saving\.\.\.$' }).Count
        $endContinuationSaveSuccesses = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -eq 'Save was successful.' }).Count
        if ($endContinuationSaveCalls -ne 1 -or $endContinuationSaveSuccesses -ne 1) {
            throw 'The End continuation leaf must perform exactly its one successful private Play-settings seed Save.'
        }
        $result['observed_counts'] = [ordered]@{
            fixture = $endContinuationSummary
            updates = 68
            readbacks = 94
            csv_rows = 31
            crops = 6
            private_save_calls = $endContinuationSaveCalls
            successful_private_saves = $endContinuationSaveSuccesses
        }
