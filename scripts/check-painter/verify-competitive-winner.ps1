# Dot-sourced into the scope of scripts/check-painter.ps1. The guard
# if ($CompetitiveWinnerOnly) stays there; this is the leaf's post-run evidence check.

        $competitiveWinnerSummary = Get-Content -LiteralPath (Join-Path $captures 'competitive-winner-summary.json') -Raw | ConvertFrom-Json
        $competitiveWinnerRows = @(Import-Csv -LiteralPath (Join-Path $captures 'competitive-winner.csv'))
        $competitiveWinnerSessions = @('competitive-a', 'competitive-b', 'competitive-draw')
        $competitiveWinnerExpectedPairs = @(
            foreach ($case in @('legacy-initial', 'legacy-fill-crossed', 'legacy-details', 'legacy-prompt-crossed')) {
                "$case|competitive-a"
            }
            foreach ($session in $competitiveWinnerSessions) { "legacy-ready|$session" }
            'revealed-initial|competitive-draw'
            'solo-control|solo'
            foreach ($case in @('old-primary', 'retained-old', 'reopened-new', 'retained-new')) {
                foreach ($session in $competitiveWinnerSessions) { "$case|$session" }
            }
            foreach ($case in @('old-1080', 'old-return720', 'new-1080', 'new-return720', 'restored-default')) {
                "$case|competitive-a"
            }
            foreach ($case in @('already-revealed-initial', 'already-revealed-ready')) {
                foreach ($session in @('competitive-a', 'competitive-b', 'solo')) { "$case|$session" }
            }
        )
        $competitiveWinnerExpectedColumns = @('case', 'session', 'width', 'height', 'players', 'best_of', 'target',
            'complete', 'round', 'a_wins', 'b_wins', 'revealed', 'details_visible', 'proceed_visible', 'winner_visible',
            'hidden_policy_control', 'expected_value', 'font', 'raw_width', 'raw_height', 'x', 'y', 'scale',
            'shadow_x', 'shadow_y', 'score_literal', 'outcome_literal', 'heading_literal', 'winner_match',
            'row_neighbor_match', 'heading_match', 'frozen_session_setup_save_rng')
        if ($competitiveWinnerSummary.outcome -ne 'passed' -or $competitiveWinnerSummary.scope -ne 'competitive-winner-only' -or
            $competitiveWinnerSummary.updates -ne 102 -or $competitiveWinnerSummary.readbacks -ne 99 -or
            $competitiveWinnerSummary.csv_rows -ne 32 -or $competitiveWinnerSummary.compact_crops -ne 8 -or
            $competitiveWinnerSummary.page_openings -ne 14 -or $competitiveWinnerSummary.proceed_pops -ne 14 -or
            $competitiveWinnerSummary.font_negative_controls -ne 3 -or $competitiveWinnerSummary.resize_updates -ne 0 -or
            $competitiveWinnerSummary.save_calls -ne 1 -or $competitiveWinnerSummary.crop_pixels -ne 312480 -or
            $competitiveWinnerRows.Count -ne 32 -or
            (($competitiveWinnerRows[0].PSObject.Properties.Name) -join '|') -ne ($competitiveWinnerExpectedColumns -join '|') -or
            (($competitiveWinnerRows | ForEach-Object { "$($_.case)|$($_.session)" } | Sort-Object) -join ';') -ne
                (($competitiveWinnerExpectedPairs | Sort-Object) -join ';')) {
            throw 'The Competitive winner leaf does not match its frozen summary, CSV schema, cases or update/readback/proceed budgets.'
        }
        $competitiveWinnerState = @{
            'competitive-a' = @('2', '3', '2', 'false', '1', '1', '0', 'Wins: A 1 - B 0  |  First to 2', 'Series continues', 'Round 1')
            'competitive-b' = @('2', '3', '2', 'false', '1', '0', '1', 'Wins: A 0 - B 1  |  First to 2', 'Series continues', 'Round 1')
            'competitive-draw' = @('2', '3', '2', 'false', '1', '0', '0', 'Wins: A 0 - B 0  |  First to 2', 'Draw - no win awarded', 'Round 1')
            'solo' = @('1', '1', '1', 'true', '1', '0', '0', 'Wins: A 0 - B 0  |  First to 1', 'Team B wins the series!', 'Results')
        }
        $competitiveWinnerDefaultValues = @{
            'competitive-a' = 'Team A Wins!'; 'competitive-b' = 'Team B Wins!'; 'competitive-draw' = 'Draw!'
            'solo' = '72.0% Painted'
        }
        $competitiveWinnerOldValues = @{
            'competitive-a' = 'A won~ {A}'; 'competitive-b' = 'B won~ {B}'; 'competitive-draw' = 'Draw~ {D}'
        }
        $competitiveWinnerNewValues = @{
            'competitive-a' = 'A wins {A}'; 'competitive-b' = 'B wins {B}'; 'competitive-draw' = 'Even {D}'
            'solo' = '72.0% Painted'
        }
        foreach ($row in $competitiveWinnerRows) {
            $expectedState = $competitiveWinnerState[$row.session]
            $expectedWidth = if ($row.case -in @('old-1080', 'new-1080')) { '1920' } else { '1280' }
            $expectedHeight = if ($expectedWidth -eq '1920') { '1080' } else { '720' }
            $hiddenPolicyControl = $row.case -in @('already-revealed-initial', 'already-revealed-ready')
            $expectedHiddenPolicy = if ($hiddenPolicyControl) { 'true' } else { 'false' }
            $expectedRevealed = if ($row.session -eq 'competitive-draw' -or $hiddenPolicyControl) { 'true' } else { 'false' }
            $expectedDetails = if ($row.case -in @('legacy-initial', 'legacy-fill-crossed')) { 'false' } else { 'true' }
            $expectedProceed = if ($row.case -in @('legacy-initial', 'legacy-fill-crossed', 'legacy-details',
                'legacy-prompt-crossed', 'revealed-initial', 'already-revealed-initial')) { 'false' } else { 'true' }
            $expectedWinnerVisible = if ($row.case -in @('legacy-initial', 'legacy-fill-crossed') -or $hiddenPolicyControl) { 'false' } else { 'true' }
            $newCopy = $row.case -in @('reopened-new', 'retained-new', 'new-1080', 'new-return720', 'solo-control') -or $hiddenPolicyControl
            $oldCopy = $row.case -in @('old-primary', 'retained-old', 'old-1080', 'old-return720')
            $expectedValue = if ($newCopy) { $competitiveWinnerNewValues[$row.session] }
                elseif ($oldCopy) { $competitiveWinnerOldValues[$row.session] } else { $competitiveWinnerDefaultValues[$row.session] }
            $expectedFont = if ($newCopy) { 'competitive_winner_braces' } else { 'gill_sans_mt_bold_48' }
            if ($row.width -ne $expectedWidth -or $row.height -ne $expectedHeight -or
                $row.players -ne $expectedState[0] -or $row.best_of -ne $expectedState[1] -or
                $row.target -ne $expectedState[2] -or $row.complete -ne $expectedState[3] -or
                $row.round -ne $expectedState[4] -or $row.a_wins -ne $expectedState[5] -or $row.b_wins -ne $expectedState[6] -or
                $row.score_literal -cne $expectedState[7] -or $row.outcome_literal -cne $expectedState[8] -or
                $row.heading_literal -cne $expectedState[9] -or
                $row.revealed -ne $expectedRevealed -or $row.details_visible -ne $expectedDetails -or
                $row.proceed_visible -ne $expectedProceed -or $row.winner_visible -ne $expectedWinnerVisible -or
                $row.hidden_policy_control -ne $expectedHiddenPolicy -or
                $row.expected_value -cne $expectedValue -or $row.font -ne $expectedFont -or
                $row.scale -ne '1' -or $row.shadow_x -ne '0' -or $row.shadow_y -ne '0' -or
                $row.winner_match -ne 'true' -or $row.row_neighbor_match -ne 'true' -or $row.heading_match -ne 'true' -or
                $row.frozen_session_setup_save_rng -ne 'true') {
                throw "Competitive winner case $($row.case)/$($row.session) changed its literal winner, ledger, reveal, capture or geometry contract."
            }
            foreach ($metric in @('raw_width', 'raw_height', 'x', 'y')) {
                $value = 0.0
                if (-not [double]::TryParse($row.$metric, [Globalization.NumberStyles]::Float,
                    [Globalization.CultureInfo]::InvariantCulture, [ref]$value) -or
                    [double]::IsNaN($value) -or [double]::IsInfinity($value) -or
                    ($metric -in @('raw_width', 'raw_height') -and $value -le 0.0) -or
                    ($metric -in @('x', 'y') -and $value -lt 0.0)) {
                    throw "Competitive winner case $($row.case)/$($row.session) contains an invalid $metric."
                }
            }
        }
        $competitiveWinnerExpectedCrops = [ordered]@{
            'competitive-winner-a.ppm' = @(480, 62)
            'competitive-winner-b.ppm' = @(480, 62)
            'competitive-winner-draw.ppm' = @(480, 62)
            'competitive-winner-solo.ppm' = @(480, 62)
            'competitive-winner-old-720.ppm' = @(480, 62)
            'competitive-winner-new-720.ppm' = @(480, 62)
            'competitive-winner-old-1080.ppm' = @(720, 93)
            'competitive-winner-new-1080.ppm' = @(720, 93)
        }
        $competitiveWinnerCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'competitive-winner-*.ppm' -File)
        $competitiveWinnerExpectedNames = @($competitiveWinnerExpectedCrops.Keys) + @('competitive-winner.csv', 'competitive-winner-summary.json')
        $competitiveWinnerNames = @(Get-ChildItem -LiteralPath $captures -Filter 'competitive-winner*' -File | Select-Object -ExpandProperty Name)
        if ($competitiveWinnerCrops.Count -ne 8 -or
            (($competitiveWinnerNames | Sort-Object) -join '|') -ne (($competitiveWinnerExpectedNames | Sort-Object) -join '|')) {
            throw 'The Competitive winner leaf emitted unexpected or missing compact crops, CSV or summary files.'
        }
        foreach ($crop in $competitiveWinnerCrops) {
            $size = $competitiveWinnerExpectedCrops[$crop.Name]
            $header = @(Get-Content -LiteralPath $crop.FullName -TotalCount 3)
            $expectedHeader = "P6`n$($size[0]) $($size[1])`n255`n"
            $expectedBytes = [Text.Encoding]::ASCII.GetByteCount($expectedHeader) + $size[0] * $size[1] * 3
            if ($header.Count -ne 3 -or $header[0] -ne 'P6' -or $header[1] -ne "$($size[0]) $($size[1])" -or
                $header[2] -ne '255' -or $crop.Length -ne $expectedBytes) {
                throw "Competitive winner crop $($crop.Name) differs from its exact dimensions or RGB payload budget."
            }
        }
        $competitiveWinnerSaveCalls = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -match 'Saving\.\.\.$' }).Count
        $competitiveWinnerSaveSuccesses = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -eq 'Save was successful.' }).Count
        if ($competitiveWinnerSaveCalls -ne 1 -or $competitiveWinnerSaveSuccesses -ne 1) {
            throw 'The Competitive winner leaf must perform exactly its one successful private seed Save.'
        }
        $result['observed_counts'] = [ordered]@{
            fixture = $competitiveWinnerSummary
            updates = 102
            readbacks = 99
            csv_rows = 32
            crops = 8
            private_save_calls = $competitiveWinnerSaveCalls
            successful_private_saves = $competitiveWinnerSaveSuccesses
        }
