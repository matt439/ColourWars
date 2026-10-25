# Dot-sourced into the scope of scripts/check-painter.ps1. The guard
# if ($SeriesScoreOnly) stays there; this is the leaf's post-run evidence check.

        $seriesScoreSummary = Get-Content -LiteralPath (Join-Path $captures 'series-score-summary.json') -Raw | ConvertFrom-Json
        $seriesScoreRows = @(Import-Csv -LiteralPath (Join-Path $captures 'series-score.csv'))
        $seriesScoreSessions = @('series3-draw', 'series3-ongoing', 'series5-max-a', 'series5-max-b')
        $seriesScoreExpectedPairs = @(
            foreach ($case in @('legacy-initial', 'legacy-fill-crossed', 'legacy-details', 'legacy-prompt-crossed')) {
                "$case|series3-draw"
            }
            foreach ($session in @($seriesScoreSessions) + @('single')) { "legacy-ready|$session" }
            'revealed-initial|series3-ongoing'
            foreach ($case in @('old-primary', 'retained-old', 'reopened-new', 'retained-new')) {
                foreach ($session in $seriesScoreSessions) { "$case|$session" }
            }
            foreach ($case in @('old-1080', 'old-return720', 'new-1080', 'new-return720', 'restored-default')) {
                "$case|series3-draw"
            }
        )
        $seriesScoreExpectedColumns = @('case', 'session', 'width', 'height', 'best_of', 'target', 'complete', 'round',
            'a_wins', 'b_wins', 'revealed', 'details_visible', 'proceed_visible', 'expected_value', 'font',
            'raw_width', 'raw_height', 'x', 'y', 'scale', 'shadow_x', 'shadow_y', 'visible', 'outcome_literal',
            'heading_literal', 'row_neighbor_match', 'heading_match', 'frozen_session_setup_save_rng')
        if ($seriesScoreSummary.outcome -ne 'passed' -or $seriesScoreSummary.scope -ne 'series-score-only' -or
            $seriesScoreSummary.updates -ne 81 -or $seriesScoreSummary.readbacks -ne 97 -or
            $seriesScoreSummary.csv_rows -ne 31 -or $seriesScoreSummary.compact_crops -ne 8 -or
            $seriesScoreSummary.page_openings -ne 14 -or $seriesScoreSummary.proceed_pops -ne 14 -or
            $seriesScoreSummary.font_negative_controls -ne 4 -or $seriesScoreSummary.resize_updates -ne 0 -or
            $seriesScoreSummary.save_calls -ne 1 -or $seriesScoreSummary.crop_pixels -ne 710640 -or
            $seriesScoreRows.Count -ne 31 -or
            (($seriesScoreRows[0].PSObject.Properties.Name) -join '|') -ne ($seriesScoreExpectedColumns -join '|') -or
            (($seriesScoreRows | ForEach-Object { "$($_.case)|$($_.session)" } | Sort-Object) -join ';') -ne
                (($seriesScoreExpectedPairs | Sort-Object) -join ';')) {
            throw 'The Series score leaf does not match its frozen summary, CSV schema, cases or update/readback/proceed budgets.'
        }
        $seriesScoreState = @{
            'series3-draw' = @('3', '2', 'false', '1', '0', '0', 'Draw - no win awarded', 'Round 1')
            'series3-ongoing' = @('3', '2', 'false', '1', '1', '0', 'Series continues', 'Round 1')
            'series5-max-a' = @('5', '3', 'true', '5', '3', '2', 'Team A wins the series!', 'Round 5')
            'series5-max-b' = @('5', '3', 'true', '5', '2', '3', 'Team B wins the series!', 'Round 5')
            'single' = @('1', '1', 'true', '1', '1', '0', 'Team A wins the series!', 'Results')
        }
        $seriesScoreDefaultValues = @{
            'series3-draw' = 'Wins: A 0 - B 0  |  First to 2'; 'series3-ongoing' = 'Wins: A 1 - B 0  |  First to 2'
            'series5-max-a' = 'Wins: A 3 - B 2  |  First to 3'; 'series5-max-b' = 'Wins: A 2 - B 3  |  First to 3'
            'single' = 'Wins: A 1 - B 0  |  First to 1'
        }
        $seriesScoreOldValues = @{
            'series3-draw' = 'Old~ A 0 / B 0 / goal 2'; 'series3-ongoing' = 'Old~ A 1 / B 0 / goal 2'
            'series5-max-a' = 'Old~ A 3 / B 2 / goal 3'; 'series5-max-b' = 'Old~ A 2 / B 3 / goal 3'
        }
        $seriesScoreNewValues = @{
            'series3-draw' = 'Goal 2 ^ B 0 / A 0'; 'series3-ongoing' = 'Goal 2 ^ B 0 / A 1'
            'series5-max-a' = 'Goal 3 ^ B 2 / A 3'; 'series5-max-b' = 'Goal 3 ^ B 3 / A 2'
        }
        foreach ($row in $seriesScoreRows) {
            $expectedState = $seriesScoreState[$row.session]
            $expectedWidth = if ($row.case -in @('old-1080', 'new-1080')) { '1920' } else { '1280' }
            $expectedHeight = if ($expectedWidth -eq '1920') { '1080' } else { '720' }
            $expectedRevealed = if ($row.session -eq 'series3-draw' -and $row.case -like 'legacy-*') { 'false' } else { 'true' }
            $expectedDetails = if ($row.case -in @('legacy-initial', 'legacy-fill-crossed')) { 'false' } else { 'true' }
            $expectedProceed = if ($row.case -in @('legacy-initial', 'legacy-fill-crossed', 'legacy-details',
                'legacy-prompt-crossed', 'revealed-initial')) { 'false' } else { 'true' }
            $expectedVisible = if ($row.session -eq 'single') { 'false' } else { $expectedDetails }
            $newCopy = $row.case -in @('reopened-new', 'retained-new', 'new-1080', 'new-return720')
            $oldCopy = $row.case -in @('old-primary', 'retained-old', 'old-1080', 'old-return720')
            $expectedValue = if ($newCopy) { $seriesScoreNewValues[$row.session] }
                elseif ($oldCopy) { $seriesScoreOldValues[$row.session] } else { $seriesScoreDefaultValues[$row.session] }
            $expectedFont = if ($newCopy) { 'series_score_caret' } else { 'lato_black_36' }
            if ($row.width -ne $expectedWidth -or $row.height -ne $expectedHeight -or
                $row.best_of -ne $expectedState[0] -or $row.target -ne $expectedState[1] -or
                $row.complete -ne $expectedState[2] -or $row.round -ne $expectedState[3] -or
                $row.a_wins -ne $expectedState[4] -or $row.b_wins -ne $expectedState[5] -or
                $row.outcome_literal -cne $expectedState[6] -or $row.heading_literal -cne $expectedState[7] -or
                $row.revealed -ne $expectedRevealed -or $row.details_visible -ne $expectedDetails -or
                $row.proceed_visible -ne $expectedProceed -or $row.visible -ne $expectedVisible -or
                $row.expected_value -cne $expectedValue -or $row.font -ne $expectedFont -or
                $row.scale -ne '1' -or $row.shadow_x -ne '0' -or $row.shadow_y -ne '0' -or
                $row.row_neighbor_match -ne 'true' -or $row.heading_match -ne 'true' -or
                $row.frozen_session_setup_save_rng -ne 'true') {
                throw "Series score case $($row.case)/$($row.session) changed its literal score, ledger, reveal, capture or geometry contract."
            }
            foreach ($metric in @('raw_width', 'raw_height', 'x', 'y')) {
                $value = 0.0
                if (-not [double]::TryParse($row.$metric, [Globalization.NumberStyles]::Float,
                    [Globalization.CultureInfo]::InvariantCulture, [ref]$value) -or
                    [double]::IsNaN($value) -or [double]::IsInfinity($value) -or
                    ($metric -in @('raw_width', 'raw_height') -and $value -le 0.0) -or
                    ($metric -in @('x', 'y') -and $value -lt 0.0)) {
                    throw "Series score case $($row.case)/$($row.session) contains an invalid $metric."
                }
            }
        }
        $seriesScoreExpectedCrops = [ordered]@{
            'series-score-draw3.ppm' = @(720, 94)
            'series-score-ongoing3.ppm' = @(720, 94)
            'series-score-max-a5.ppm' = @(720, 94)
            'series-score-max-b5.ppm' = @(720, 94)
            'series-score-old-720.ppm' = @(720, 94)
            'series-score-new-720.ppm' = @(720, 94)
            'series-score-old-1080.ppm' = @(1080, 141)
            'series-score-new-1080.ppm' = @(1080, 141)
        }
        $seriesScoreCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'series-score-*.ppm' -File)
        $seriesScoreExpectedNames = @($seriesScoreExpectedCrops.Keys) + @('series-score.csv', 'series-score-summary.json')
        $seriesScoreNames = @(Get-ChildItem -LiteralPath $captures -Filter 'series-score*' -File | Select-Object -ExpandProperty Name)
        if ($seriesScoreCrops.Count -ne 8 -or
            (($seriesScoreNames | Sort-Object) -join '|') -ne (($seriesScoreExpectedNames | Sort-Object) -join '|')) {
            throw 'The Series score leaf emitted unexpected or missing compact crops, CSV or summary files.'
        }
        foreach ($crop in $seriesScoreCrops) {
            $size = $seriesScoreExpectedCrops[$crop.Name]
            $header = @(Get-Content -LiteralPath $crop.FullName -TotalCount 3)
            $expectedHeader = "P6`n$($size[0]) $($size[1])`n255`n"
            $expectedBytes = [Text.Encoding]::ASCII.GetByteCount($expectedHeader) + $size[0] * $size[1] * 3
            if ($header.Count -ne 3 -or $header[0] -ne 'P6' -or $header[1] -ne "$($size[0]) $($size[1])" -or
                $header[2] -ne '255' -or $crop.Length -ne $expectedBytes) {
                throw "Series score crop $($crop.Name) differs from its exact dimensions or RGB payload budget."
            }
        }
        $seriesScoreSaveCalls = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -match 'Saving\.\.\.$' }).Count
        $seriesScoreSaveSuccesses = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -eq 'Save was successful.' }).Count
        if ($seriesScoreSaveCalls -ne 1 -or $seriesScoreSaveSuccesses -ne 1) {
            throw 'The Series score leaf must perform exactly its one successful private seed Save.'
        }
        $result['observed_counts'] = [ordered]@{
            fixture = $seriesScoreSummary
            updates = 81
            readbacks = 97
            csv_rows = 31
            crops = 8
            private_save_calls = $seriesScoreSaveCalls
            successful_private_saves = $seriesScoreSaveSuccesses
        }
