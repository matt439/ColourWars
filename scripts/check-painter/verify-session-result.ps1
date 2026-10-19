# Dot-sourced into the scope of scripts/check-painter.ps1. The guard
# if ($SessionResultOnly) stays there; this is the leaf's post-run evidence check.

        $sessionResultSummary = Get-Content -LiteralPath (Join-Path $captures 'session-result-summary.json') -Raw | ConvertFrom-Json
        $sessionResultRows = @(Import-Csv -LiteralPath (Join-Path $captures 'session-result.csv'))
        $sessionResultSessions = @('series3-complete-a', 'series5-complete-b', 'series3-draw', 'series5-continues')
        $sessionResultExpectedPairs = @(
            foreach ($case in @('legacy-initial', 'legacy-fill-crossed', 'legacy-details', 'legacy-prompt-crossed')) {
                "$case|series3-complete-a"
            }
            foreach ($session in $sessionResultSessions) { "legacy-ready|$session" }
            'revealed-initial|series5-complete-b'
            foreach ($case in @('old-primary', 'retained-old', 'reopened-new', 'retained-new')) {
                foreach ($session in $sessionResultSessions) { "$case|$session" }
            }
            foreach ($case in @('old-1080', 'old-return720', 'new-1080', 'new-return720', 'restored-default')) {
                "$case|series3-complete-a"
            }
        )
        $sessionResultExpectedColumns = @('case', 'session', 'width', 'height', 'best_of', 'complete', 'round',
            'a_wins', 'b_wins', 'revealed', 'details_visible', 'proceed_visible', 'expected_value', 'font',
            'raw_width', 'raw_height', 'x', 'y', 'scale', 'shadow_x', 'shadow_y', 'visible', 'score_literal',
            'heading_literal', 'row_neighbor_match', 'heading_match', 'frozen_session_setup_save_rng')
        if ($sessionResultSummary.outcome -ne 'passed' -or $sessionResultSummary.scope -ne 'session-result-only' -or
            $sessionResultSummary.updates -ne 76 -or $sessionResultSummary.readbacks -ne 94 -or
            $sessionResultSummary.csv_rows -ne 30 -or $sessionResultSummary.compact_crops -ne 8 -or
            $sessionResultSummary.page_openings -ne 13 -or $sessionResultSummary.proceed_pops -ne 13 -or
            $sessionResultSummary.font_negative_controls -ne 4 -or $sessionResultSummary.resize_updates -ne 0 -or
            $sessionResultSummary.save_calls -ne 1 -or $sessionResultSummary.crop_pixels -ne 710640 -or
            $sessionResultRows.Count -ne 30 -or
            (($sessionResultRows[0].PSObject.Properties.Name) -join '|') -ne ($sessionResultExpectedColumns -join '|') -or
            (($sessionResultRows | ForEach-Object { "$($_.case)|$($_.session)" } | Sort-Object) -join ';') -ne
                (($sessionResultExpectedPairs | Sort-Object) -join ';')) {
            throw 'The Session result leaf does not match its frozen summary, CSV schema, cases or update/readback/proceed budgets.'
        }
        $sessionResultState = @{
            'series3-complete-a' = @('3', 'true', '2', '2', '0', 'Wins: A 2 - B 0  |  First to 2', 'Round 2')
            'series5-complete-b' = @('5', 'true', '3', '0', '3', 'Wins: A 0 - B 3  |  First to 3', 'Round 3')
            'series3-draw' = @('3', 'false', '2', '1', '0', 'Wins: A 1 - B 0  |  First to 2', 'Round 2')
            'series5-continues' = @('5', 'false', '1', '0', '1', 'Wins: A 0 - B 1  |  First to 3', 'Round 1')
        }
        $sessionResultDefaultValues = @{
            'series3-complete-a' = 'Team A wins the series!'; 'series5-complete-b' = 'Team B wins the series!'
            'series3-draw' = 'Draw - no win awarded'; 'series5-continues' = 'Series continues'
        }
        $sessionResultOldValues = @{
            'series3-complete-a' = 'A wins~ {A}'; 'series5-complete-b' = 'B wins~ {B}'
            'series3-draw' = 'Draw~ {D}'; 'series5-continues' = 'More~ {N}'
        }
        $sessionResultNewValues = @{
            'series3-complete-a' = 'A again {A}'; 'series5-complete-b' = 'B again {B}'
            'series3-draw' = 'No win {D}'; 'series5-continues' = 'Continue {N}'
        }
        foreach ($row in $sessionResultRows) {
            $expectedState = $sessionResultState[$row.session]
            $expectedWidth = if ($row.case -in @('old-1080', 'new-1080')) { '1920' } else { '1280' }
            $expectedHeight = if ($expectedWidth -eq '1920') { '1080' } else { '720' }
            $expectedRevealed = if ($row.session -eq 'series3-complete-a' -and $row.case -like 'legacy-*') { 'false' } else { 'true' }
            $expectedDetails = if ($row.case -in @('legacy-initial', 'legacy-fill-crossed')) { 'false' } else { 'true' }
            $expectedProceed = if ($row.case -in @('legacy-initial', 'legacy-fill-crossed', 'legacy-details',
                'legacy-prompt-crossed', 'revealed-initial')) { 'false' } else { 'true' }
            $newCopy = $row.case -in @('reopened-new', 'retained-new', 'new-1080', 'new-return720')
            $oldCopy = $row.case -in @('old-primary', 'retained-old', 'old-1080', 'old-return720')
            $expectedValue = if ($newCopy) { $sessionResultNewValues[$row.session] }
                elseif ($oldCopy) { $sessionResultOldValues[$row.session] } else { $sessionResultDefaultValues[$row.session] }
            $expectedFont = if ($newCopy) { 'session_result_braces' } else { 'gill_sans_mt_bold_36' }
            if ($row.width -ne $expectedWidth -or $row.height -ne $expectedHeight -or
                $row.best_of -ne $expectedState[0] -or $row.complete -ne $expectedState[1] -or
                $row.round -ne $expectedState[2] -or $row.a_wins -ne $expectedState[3] -or $row.b_wins -ne $expectedState[4] -or
                $row.score_literal -cne $expectedState[5] -or $row.heading_literal -cne $expectedState[6] -or
                $row.revealed -ne $expectedRevealed -or $row.details_visible -ne $expectedDetails -or
                $row.proceed_visible -ne $expectedProceed -or $row.visible -ne $expectedDetails -or
                $row.expected_value -cne $expectedValue -or $row.font -ne $expectedFont -or
                $row.scale -ne '1' -or $row.shadow_x -ne '0' -or $row.shadow_y -ne '0' -or
                $row.row_neighbor_match -ne 'true' -or $row.heading_match -ne 'true' -or
                $row.frozen_session_setup_save_rng -ne 'true') {
                throw "Session result case $($row.case)/$($row.session) changed its literal copy, ledger, reveal, capture or geometry contract."
            }
            foreach ($metric in @('raw_width', 'raw_height', 'x', 'y')) {
                $value = 0.0
                if (-not [double]::TryParse($row.$metric, [Globalization.NumberStyles]::Float,
                    [Globalization.CultureInfo]::InvariantCulture, [ref]$value) -or
                    [double]::IsNaN($value) -or [double]::IsInfinity($value) -or
                    ($metric -in @('raw_width', 'raw_height') -and $value -le 0.0) -or
                    ($metric -in @('x', 'y') -and $value -lt 0.0)) {
                    throw "Session result case $($row.case)/$($row.session) contains an invalid $metric."
                }
            }
        }
        $sessionResultExpectedCrops = [ordered]@{
            'session-result-a3.ppm' = @(720, 94)
            'session-result-b5.ppm' = @(720, 94)
            'session-result-draw3.ppm' = @(720, 94)
            'session-result-continues5.ppm' = @(720, 94)
            'session-result-old-720.ppm' = @(720, 94)
            'session-result-new-720.ppm' = @(720, 94)
            'session-result-old-1080.ppm' = @(1080, 141)
            'session-result-new-1080.ppm' = @(1080, 141)
        }
        $sessionResultCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'session-result-*.ppm' -File)
        $sessionResultExpectedNames = @($sessionResultExpectedCrops.Keys) + @('session-result.csv', 'session-result-summary.json')
        $sessionResultNames = @(Get-ChildItem -LiteralPath $captures -Filter 'session-result*' -File | Select-Object -ExpandProperty Name)
        if ($sessionResultCrops.Count -ne 8 -or
            (($sessionResultNames | Sort-Object) -join '|') -ne (($sessionResultExpectedNames | Sort-Object) -join '|')) {
            throw 'The Session result leaf emitted unexpected or missing compact crops, CSV or summary files.'
        }
        foreach ($crop in $sessionResultCrops) {
            $size = $sessionResultExpectedCrops[$crop.Name]
            $header = @(Get-Content -LiteralPath $crop.FullName -TotalCount 3)
            $expectedHeader = "P6`n$($size[0]) $($size[1])`n255`n"
            $expectedBytes = [Text.Encoding]::ASCII.GetByteCount($expectedHeader) + $size[0] * $size[1] * 3
            if ($header.Count -ne 3 -or $header[0] -ne 'P6' -or $header[1] -ne "$($size[0]) $($size[1])" -or
                $header[2] -ne '255' -or $crop.Length -ne $expectedBytes) {
                throw "Session result crop $($crop.Name) differs from its exact dimensions or RGB payload budget."
            }
        }
        $sessionResultSaveCalls = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -match 'Saving\.\.\.$' }).Count
        $sessionResultSaveSuccesses = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -eq 'Save was successful.' }).Count
        if ($sessionResultSaveCalls -ne 1 -or $sessionResultSaveSuccesses -ne 1) {
            throw 'The Session result leaf must perform exactly its one successful private seed Save.'
        }
        $result['observed_counts'] = [ordered]@{
            fixture = $sessionResultSummary
            updates = 76
            readbacks = 94
            csv_rows = 30
            crops = 8
            private_save_calls = $sessionResultSaveCalls
            successful_private_saves = $sessionResultSaveSuccesses
        }
