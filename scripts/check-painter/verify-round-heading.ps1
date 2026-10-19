# Dot-sourced into the scope of scripts/check-painter.ps1. The guard
# if ($RoundHeadingOnly) stays there; this is the leaf's post-run evidence check.

        $roundHeadingSummary = Get-Content -LiteralPath (Join-Path $captures 'round-heading-summary.json') -Raw | ConvertFrom-Json
        $roundHeadingRows = @(Import-Csv -LiteralPath (Join-Path $captures 'round-heading.csv'))
        $roundHeadingSessions = @('series-round1', 'series-round10', 'series-round100')
        $roundHeadingExpectedPairs = @(
            foreach ($case in @('legacy-initial', 'legacy-fill-crossed', 'legacy-details', 'legacy-prompt-crossed')) {
                "$case|series-round1"
            }
            foreach ($session in $roundHeadingSessions) { "legacy-ready|$session" }
            'revealed-initial|series-round10'
            'single-control|single'
            foreach ($case in @('old-primary', 'retained-old', 'reopened-new', 'retained-new')) {
                foreach ($session in $roundHeadingSessions) { "$case|$session" }
            }
            foreach ($case in @('old-1080', 'old-return720', 'new-1080', 'new-return720', 'restored-default')) {
                "$case|series-round1"
            }
        )
        $roundHeadingExpectedColumns = @('case', 'session', 'width', 'height', 'best_of', 'target', 'complete', 'round',
            'a_wins', 'b_wins', 'revealed', 'details_visible', 'proceed_visible', 'expected_value', 'font',
            'raw_width', 'raw_height', 'x', 'y', 'scale', 'shadow_x', 'shadow_y', 'visible', 'score_literal',
            'outcome_literal', 'row_neighbor_match', 'heading_match', 'frozen_session_setup_save_rng')
        if ($roundHeadingSummary.outcome -ne 'passed' -or $roundHeadingSummary.scope -ne 'round-heading-only' -or
            $roundHeadingSummary.updates -ne 66 -or $roundHeadingSummary.readbacks -ne 81 -or
            $roundHeadingSummary.csv_rows -ne 26 -or $roundHeadingSummary.compact_crops -ne 8 -or
            $roundHeadingSummary.page_openings -ne 11 -or $roundHeadingSummary.proceed_pops -ne 11 -or
            $roundHeadingSummary.font_negative_controls -ne 3 -or $roundHeadingSummary.resize_updates -ne 0 -or
            $roundHeadingSummary.save_calls -ne 1 -or $roundHeadingSummary.crop_pixels -ne 722400 -or
            $roundHeadingRows.Count -ne 26 -or
            (($roundHeadingRows[0].PSObject.Properties.Name) -join '|') -ne ($roundHeadingExpectedColumns -join '|') -or
            (($roundHeadingRows | ForEach-Object { "$($_.case)|$($_.session)" } | Sort-Object) -join ';') -ne
                (($roundHeadingExpectedPairs | Sort-Object) -join ';')) {
            throw 'The Round heading leaf does not match its frozen summary, CSV schema, cases or update/readback/proceed budgets.'
        }
        $roundHeadingState = @{
            'series-round1' = @('3', '2', 'false', '1', '0', '0')
            'series-round10' = @('3', '2', 'false', '10', '0', '0')
            'series-round100' = @('3', '2', 'false', '100', '0', '0')
            'single' = @('1', '1', 'true', '1', '1', '0')
        }
        $roundHeadingDefaultValues = @{
            'series-round1' = 'Round 1'; 'series-round10' = 'Round 10'; 'series-round100' = 'Round 100'
            'single' = 'Results'
        }
        $roundHeadingOldValues = @{
            'series-round1' = 'Old~ 1'; 'series-round10' = 'Old~ 10'; 'series-round100' = 'Old~ 100'
        }
        $roundHeadingNewValues = @{
            'series-round1' = '1 ^ Round'; 'series-round10' = '10 ^ Round'; 'series-round100' = '100 ^ Round'
            'single' = 'Results'
        }
        foreach ($row in $roundHeadingRows) {
            $expectedState = $roundHeadingState[$row.session]
            $expectedWidth = if ($row.case -in @('old-1080', 'new-1080')) { '1920' } else { '1280' }
            $expectedHeight = if ($expectedWidth -eq '1920') { '1080' } else { '720' }
            $expectedRevealed = if ($row.session -eq 'series-round1' -and $row.case -like 'legacy-*') { 'false' } else { 'true' }
            $expectedDetails = if ($row.case -in @('legacy-initial', 'legacy-fill-crossed')) { 'false' } else { 'true' }
            $expectedProceed = if ($row.case -in @('legacy-initial', 'legacy-fill-crossed', 'legacy-details',
                'legacy-prompt-crossed', 'revealed-initial')) { 'false' } else { 'true' }
            $newCopy = $row.case -in @('reopened-new', 'retained-new', 'new-1080', 'new-return720', 'single-control')
            $oldCopy = $row.case -in @('old-primary', 'retained-old', 'old-1080', 'old-return720')
            $expectedValue = if ($newCopy) { $roundHeadingNewValues[$row.session] }
                elseif ($oldCopy) { $roundHeadingOldValues[$row.session] } else { $roundHeadingDefaultValues[$row.session] }
            $expectedFont = if ($newCopy) { 'round_heading_caret' } else { 'gill_sans_mt_bold_72' }
            $expectedScore = if ($row.session -eq 'single') { 'Wins: A 1 - B 0  |  First to 1' }
                else { 'Wins: A 0 - B 0  |  First to 2' }
            $expectedOutcome = if ($row.session -eq 'single') { 'Team A wins the series!' }
                else { 'Draw - no win awarded' }
            if ($row.width -ne $expectedWidth -or $row.height -ne $expectedHeight -or
                $row.best_of -ne $expectedState[0] -or $row.target -ne $expectedState[1] -or
                $row.complete -ne $expectedState[2] -or $row.round -ne $expectedState[3] -or
                $row.a_wins -ne $expectedState[4] -or $row.b_wins -ne $expectedState[5] -or
                $row.score_literal -cne $expectedScore -or $row.outcome_literal -cne $expectedOutcome -or
                $row.revealed -ne $expectedRevealed -or $row.details_visible -ne $expectedDetails -or
                $row.proceed_visible -ne $expectedProceed -or $row.visible -ne 'true' -or
                $row.expected_value -cne $expectedValue -or $row.font -ne $expectedFont -or
                $row.scale -ne '1' -or $row.shadow_x -ne '0' -or $row.shadow_y -ne '0' -or
                $row.row_neighbor_match -ne 'true' -or $row.heading_match -ne 'true' -or
                $row.frozen_session_setup_save_rng -ne 'true') {
                throw "Round heading case $($row.case)/$($row.session) changed its literal heading, ledger, reveal, capture or geometry contract."
            }
            foreach ($metric in @('raw_width', 'raw_height', 'x', 'y')) {
                $value = 0.0
                if (-not [double]::TryParse($row.$metric, [Globalization.NumberStyles]::Float,
                    [Globalization.CultureInfo]::InvariantCulture, [ref]$value) -or
                    [double]::IsNaN($value) -or [double]::IsInfinity($value) -or
                    ($metric -in @('raw_width', 'raw_height') -and $value -le 0.0) -or
                    ($metric -in @('x', 'y') -and $value -lt 0.0)) {
                    throw "Round heading case $($row.case)/$($row.session) contains an invalid $metric."
                }
            }
        }
        $roundHeadingExpectedCrops = [ordered]@{
            'round-heading-round1.ppm' = @(800, 86)
            'round-heading-round10.ppm' = @(800, 86)
            'round-heading-round100.ppm' = @(800, 86)
            'round-heading-single.ppm' = @(800, 86)
            'round-heading-old-720.ppm' = @(800, 86)
            'round-heading-new-720.ppm' = @(800, 86)
            'round-heading-old-1080.ppm' = @(1200, 129)
            'round-heading-new-1080.ppm' = @(1200, 129)
        }
        $roundHeadingCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'round-heading-*.ppm' -File)
        $roundHeadingExpectedNames = @($roundHeadingExpectedCrops.Keys) + @('round-heading.csv', 'round-heading-summary.json')
        $roundHeadingNames = @(Get-ChildItem -LiteralPath $captures -Filter 'round-heading*' -File | Select-Object -ExpandProperty Name)
        if ($roundHeadingCrops.Count -ne 8 -or
            (($roundHeadingNames | Sort-Object) -join '|') -ne (($roundHeadingExpectedNames | Sort-Object) -join '|')) {
            throw 'The Round heading leaf emitted unexpected or missing compact crops, CSV or summary files.'
        }
        foreach ($crop in $roundHeadingCrops) {
            $size = $roundHeadingExpectedCrops[$crop.Name]
            $header = @(Get-Content -LiteralPath $crop.FullName -TotalCount 3)
            $expectedHeader = "P6`n$($size[0]) $($size[1])`n255`n"
            $expectedBytes = [Text.Encoding]::ASCII.GetByteCount($expectedHeader) + $size[0] * $size[1] * 3
            if ($header.Count -ne 3 -or $header[0] -ne 'P6' -or $header[1] -ne "$($size[0]) $($size[1])" -or
                $header[2] -ne '255' -or $crop.Length -ne $expectedBytes) {
                throw "Round heading crop $($crop.Name) differs from its exact dimensions or RGB payload budget."
            }
        }
        $roundHeadingSaveCalls = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -match 'Saving\.\.\.$' }).Count
        $roundHeadingSaveSuccesses = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -eq 'Save was successful.' }).Count
        if ($roundHeadingSaveCalls -ne 1 -or $roundHeadingSaveSuccesses -ne 1) {
            throw 'The Round heading leaf must perform exactly its one successful private seed Save.'
        }
        $result['observed_counts'] = [ordered]@{
            fixture = $roundHeadingSummary
            updates = 66
            readbacks = 81
            csv_rows = 26
            crops = 8
            private_save_calls = $roundHeadingSaveCalls
            successful_private_saves = $roundHeadingSaveSuccesses
        }
