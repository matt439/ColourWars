# Dot-sourced into the scope of scripts/check-painter.ps1. The guard
# if ($ContributionTeamOnly) stays there; this is the leaf's post-run evidence check.

        $contributionTeamSummary = Get-Content -LiteralPath (Join-Path $captures 'contribution-team-summary.json') -Raw | ConvertFrom-Json
        $contributionTeamRows = @(Import-Csv -LiteralPath (Join-Path $captures 'contribution-team.csv'))
        $contributionTeamCases = @('legacy-initial', 'legacy-fill-crossed', 'legacy-details', 'legacy-prompt-crossed',
            'legacy-ready', 'default-1080', 'default-return720', 'old-primary', 'retained-old', 'old-1080',
            'old-return720', 'reopened-new', 'retained-new', 'new-1080', 'new-return720', 'restored-default',
            'revealed-initial', 'revealed-ready')
        $contributionTeamExpectedPairs = @(
            foreach ($case in $contributionTeamCases) {
                foreach ($rowNumber in 0..3) { "$case|$rowNumber" }
            }
        )
        $contributionTeamExpectedColumns = @('case', 'row', 'width', 'height', 'device_slot', 'paint_owner', 'team',
            'revealed', 'details_visible', 'proceed_visible', 'expected_value', 'font', 'raw_width', 'raw_height',
            'x', 'y', 'badge_x', 'badge_y', 'badge_width', 'badge_height', 'scale', 'shadow_x', 'shadow_y',
            'percentage_literal', 'heading_literal', 'contribution_heading_literal', 'score_literal', 'outcome_literal',
            'row_match', 'badge_match', 'heading_match', 'neighbor_match', 'frozen_session_setup_save_rng')
        if ($contributionTeamSummary.outcome -ne 'passed' -or $contributionTeamSummary.scope -ne 'contribution-team-only' -or
            $contributionTeamSummary.updates -ne 45 -or $contributionTeamSummary.readbacks -ne 56 -or
            $contributionTeamSummary.csv_rows -ne 72 -or $contributionTeamSummary.compact_crops -ne 6 -or
            $contributionTeamSummary.page_openings -ne 5 -or $contributionTeamSummary.proceed_pops -ne 5 -or
            $contributionTeamSummary.font_negative_controls -ne 4 -or $contributionTeamSummary.owner_negative_controls -ne 4 -or
            $contributionTeamSummary.resize_updates -ne 0 -or $contributionTeamSummary.save_calls -ne 1 -or
            $contributionTeamSummary.crop_pixels -ne 599508 -or $contributionTeamRows.Count -ne 72 -or
            (($contributionTeamRows[0].PSObject.Properties.Name) -join '|') -ne ($contributionTeamExpectedColumns -join '|') -or
            (($contributionTeamRows | ForEach-Object { "$($_.case)|$($_.row)" } | Sort-Object) -join ';') -ne
                (($contributionTeamExpectedPairs | Sort-Object) -join ';')) {
            throw 'The Contribution team leaf does not match its frozen summary, CSV schema, rows or update/readback/proceed budgets.'
        }
        $contributionTeamState = @{
            '0' = @('4', '4', 'A', '18.7%', '680', '678')
            '1' = @('2', '2', 'B', '25.0%', '725', '723')
            '2' = @('-1', '7', 'A', '31.2%', '770', '768')
            '3' = @('0', '0', 'B', '25.0%', '815', '813')
        }
        $contributionTeamDefaultValues = @(
            'P1 - Keyboard/mouse | Team A | 18.7%',
            'P2 - Pad 3 | Team B | 25.0%',
            'P3 - Waiting to join | Team A | 31.2%',
            'P4 - Pad 1 | Team B | 25.0%'
        )
        $contributionTeamOldValues = @(
            'P1 - Keyboard/mouse | Old~ A {A} | 18.7%',
            'P2 - Pad 3 | Old~ B {B} | 25.0%',
            'P3 - Waiting to join | Old~ A {A} | 31.2%',
            'P4 - Pad 1 | Old~ B {B} | 25.0%'
        )
        $contributionTeamNewValues = @(
            'P1 - Keyboard/mouse | {A} team | 18.7%',
            'P2 - Pad 3 | {B} team | 25.0%',
            'P3 - Waiting to join | {A} team | 31.2%',
            'P4 - Pad 1 | {B} team | 25.0%'
        )
        foreach ($row in $contributionTeamRows) {
            $rowNumber = [int]$row.row
            $expectedState = $contributionTeamState[$row.row]
            $expectedWidth = if ($row.case -in @('default-1080', 'old-1080', 'new-1080')) { '1920' } else { '1280' }
            $expectedHeight = if ($expectedWidth -eq '1920') { '1080' } else { '720' }
            $expectedRevealed = if ($row.case -in @('revealed-initial', 'revealed-ready')) { 'true' } else { 'false' }
            $expectedDetails = if ($row.case -in @('legacy-initial', 'legacy-fill-crossed')) { 'false' } else { 'true' }
            $expectedProceed = if ($row.case -in @('legacy-initial', 'legacy-fill-crossed', 'legacy-details',
                'legacy-prompt-crossed', 'revealed-initial')) { 'false' } else { 'true' }
            $newCopy = $row.case -in @('reopened-new', 'retained-new', 'new-1080', 'new-return720')
            $oldCopy = $row.case -in @('old-primary', 'retained-old', 'old-1080', 'old-return720')
            $expectedValue = if ($newCopy) { $contributionTeamNewValues[$rowNumber] }
                elseif ($oldCopy) { $contributionTeamOldValues[$rowNumber] } else { $contributionTeamDefaultValues[$rowNumber] }
            $expectedFont = if ($newCopy) { 'contribution_team_braces' } else { 'lato_black_24' }
            if ($row.width -ne $expectedWidth -or $row.height -ne $expectedHeight -or
                $row.device_slot -ne $expectedState[0] -or $row.paint_owner -ne $expectedState[1] -or
                $row.team -cne $expectedState[2] -or $row.percentage_literal -cne $expectedState[3] -or
                $row.x -ne '480' -or $row.y -ne $expectedState[4] -or
                $row.badge_x -ne '414' -or $row.badge_y -ne $expectedState[5] -or
                $row.badge_width -ne '54' -or $row.badge_height -ne '42' -or
                $row.revealed -ne $expectedRevealed -or $row.details_visible -ne $expectedDetails -or
                $row.proceed_visible -ne $expectedProceed -or $row.expected_value -cne $expectedValue -or
                $row.font -ne $expectedFont -or $row.scale -ne '1' -or $row.shadow_x -ne '0' -or $row.shadow_y -ne '0' -or
                $row.heading_literal -cne 'Round 1' -or $row.contribution_heading_literal -cne 'Surface held (% of arena)' -or
                $row.score_literal -cne 'Wins: A 0 - B 0  |  First to 2' -or $row.outcome_literal -cne 'Draw - no win awarded' -or
                $row.row_match -ne 'true' -or $row.badge_match -ne 'true' -or $row.heading_match -ne 'true' -or
                $row.neighbor_match -ne 'true' -or $row.frozen_session_setup_save_rng -ne 'true') {
                throw "Contribution team case $($row.case)/row $($row.row) changed its literal row, owner credit, reveal, capture or geometry contract."
            }
            foreach ($metric in @('raw_width', 'raw_height', 'x', 'y', 'badge_x', 'badge_y', 'badge_width', 'badge_height')) {
                $value = 0.0
                if (-not [double]::TryParse($row.$metric, [Globalization.NumberStyles]::Float,
                    [Globalization.CultureInfo]::InvariantCulture, [ref]$value) -or
                    [double]::IsNaN($value) -or [double]::IsInfinity($value) -or
                    ($metric -in @('raw_width', 'raw_height', 'badge_width', 'badge_height') -and $value -le 0.0) -or
                    ($metric -in @('x', 'y', 'badge_x', 'badge_y') -and $value -lt 0.0)) {
                    throw "Contribution team case $($row.case)/row $($row.row) contains an invalid $metric."
                }
            }
        }
        $contributionTeamExpectedCrops = [ordered]@{
            'contribution-team-default-720.ppm' = @(504, 122)
            'contribution-team-default-1080.ppm' = @(756, 183)
            'contribution-team-old-720.ppm' = @(504, 122)
            'contribution-team-old-1080.ppm' = @(756, 183)
            'contribution-team-new-720.ppm' = @(504, 122)
            'contribution-team-new-1080.ppm' = @(756, 183)
        }
        $contributionTeamCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'contribution-team-*.ppm' -File)
        $contributionTeamExpectedNames = @($contributionTeamExpectedCrops.Keys) + @('contribution-team.csv', 'contribution-team-summary.json')
        $contributionTeamNames = @(Get-ChildItem -LiteralPath $captures -Filter 'contribution-team*' -File | Select-Object -ExpandProperty Name)
        if ($contributionTeamCrops.Count -ne 6 -or
            (($contributionTeamNames | Sort-Object) -join '|') -ne (($contributionTeamExpectedNames | Sort-Object) -join '|')) {
            throw 'The Contribution team leaf emitted unexpected or missing compact crops, CSV or summary files.'
        }
        foreach ($crop in $contributionTeamCrops) {
            $size = $contributionTeamExpectedCrops[$crop.Name]
            $header = @(Get-Content -LiteralPath $crop.FullName -TotalCount 3)
            $expectedHeader = "P6`n$($size[0]) $($size[1])`n255`n"
            $expectedBytes = [Text.Encoding]::ASCII.GetByteCount($expectedHeader) + $size[0] * $size[1] * 3
            if ($header.Count -ne 3 -or $header[0] -ne 'P6' -or $header[1] -ne "$($size[0]) $($size[1])" -or
                $header[2] -ne '255' -or $crop.Length -ne $expectedBytes) {
                throw "Contribution team crop $($crop.Name) differs from its exact dimensions or RGB payload budget."
            }
        }
        $contributionTeamSaveCalls = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -match 'Saving\.\.\.$' }).Count
        $contributionTeamSaveSuccesses = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -eq 'Save was successful.' }).Count
        if ($contributionTeamSaveCalls -ne 1 -or $contributionTeamSaveSuccesses -ne 1) {
            throw 'The Contribution team leaf must perform exactly its one successful private seed Save.'
        }
        $result['observed_counts'] = [ordered]@{
            fixture = $contributionTeamSummary
            updates = 45
            readbacks = 56
            csv_rows = 72
            crops = 6
            private_save_calls = $contributionTeamSaveCalls
            successful_private_saves = $contributionTeamSaveSuccesses
        }
