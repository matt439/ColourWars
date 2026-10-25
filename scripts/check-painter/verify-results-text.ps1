# Dot-sourced into the scope of scripts/check-painter.ps1. The guard
# if ($ResultsTextOnly) stays there; this is the leaf's post-run evidence check.

        $resultsTextSummary = Get-Content -LiteralPath (Join-Path $captures 'results-text-summary.json') -Raw | ConvertFrom-Json
        $resultsTextRows = @(Import-Csv -LiteralPath (Join-Path $captures 'results-text.csv'))
        $resultsTextCases = @('plain-initial', 'plain-fill-crossed', 'plain-details', 'plain-prompt-crossed',
            'plain-ready', 'series-initial', 'series-ready', 'old-ready', 'retained-old', 'old-1080',
            'old-return720', 'reopened-new', 'retained-new', 'new-1080', 'new-return720', 'new-series', 'restored-default')
        $resultsTextRoles = @('heading', 'contribution', 'proceed')
        $resultsTextExpectedPairs = @(
            foreach ($case in $resultsTextCases) {
                foreach ($role in $resultsTextRoles) { "$case|$role" }
            }
        )
        $resultsTextExpectedColumns = @('case', 'session', 'role', 'width', 'height', 'revealed', 'details_visible',
            'proceed_visible', 'series', 'round', 'a_wins', 'b_wins', 'expected_value', 'font', 'raw_width',
            'raw_height', 'x', 'y', 'scale', 'shadow_x', 'shadow_y', 'visible', 'full_region_match',
            'frozen_session_setup_save_rng')
        if ($resultsTextSummary.outcome -ne 'passed' -or $resultsTextSummary.scope -ne 'results-text-only' -or
            $resultsTextSummary.updates -ne 43 -or $resultsTextSummary.readbacks -ne 52 -or
            $resultsTextSummary.csv_rows -ne 51 -or $resultsTextSummary.compact_crops -ne 8 -or
            $resultsTextSummary.page_openings -ne 6 -or $resultsTextSummary.proceed_pops -ne 6 -or
            $resultsTextSummary.font_negative_controls -ne 3 -or $resultsTextSummary.resize_updates -ne 0 -or
            $resultsTextSummary.save_calls -ne 1 -or $resultsTextSummary.crop_pixels -ne 1428000 -or
            $resultsTextRows.Count -ne 51 -or
            (($resultsTextRows[0].PSObject.Properties.Name) -join '|') -ne ($resultsTextExpectedColumns -join '|') -or
            (($resultsTextRows | ForEach-Object { "$($_.case)|$($_.role)" } | Sort-Object) -join ';') -ne
                (($resultsTextExpectedPairs | Sort-Object) -join ';')) {
            throw 'The Results text leaf does not match its frozen summary, CSV schema, cases or update/readback/proceed budgets.'
        }
        $resultsTextDefaultValues = @{
            'heading' = 'Results'; 'contribution' = 'Surface held (% of arena)'; 'proceed' = 'Press A/Enter to proceed'
        }
        $resultsTextOldValues = @{
            'heading' = 'Results~ {1}'; 'contribution' = 'Held~ {area}'; 'proceed' = 'Continue~ {A}'
        }
        $resultsTextNewValues = @{
            'heading' = 'Score {2}'; 'contribution' = 'Owned {area}'; 'proceed' = 'Proceed {B}'
        }
        $resultsTextDefaultFonts = @{
            'heading' = 'lato_black_72'; 'contribution' = 'lato_black_24'; 'proceed' = 'lato_black_36'
        }
        $resultsTextNewFonts = @{
            'heading' = 'results_text_heading_letters'; 'contribution' = 'results_text_contribution_letters'
            'proceed' = 'results_text_detail_letters'
        }
        foreach ($row in $resultsTextRows) {
            $isSeries = $row.case -in @('series-initial', 'series-ready', 'new-series')
            $expectedSession = if ($isSeries) { 'series' } else { 'single' }
            $expectedSeries = if ($isSeries) { 'true' } else { 'false' }
            $expectedRound = if ($isSeries) { '2' } else { '1' }
            $expectedWidth = if ($row.case -in @('old-1080', 'new-1080')) { '1920' } else { '1280' }
            $expectedHeight = if ($expectedWidth -eq '1920') { '1080' } else { '720' }
            $expectedRevealed = if ($row.case -like 'plain-*') { 'false' } else { 'true' }
            $expectedDetails = if ($row.case -in @('plain-initial', 'plain-fill-crossed')) { 'false' } else { 'true' }
            $expectedProceed = if ($row.case -in @('plain-initial', 'plain-fill-crossed', 'plain-details',
                'plain-prompt-crossed', 'series-initial')) { 'false' } else { 'true' }
            $expectedVisible = if ($row.role -eq 'contribution') { $expectedDetails }
                elseif ($row.role -eq 'proceed') { $expectedProceed } else { 'true' }
            $newCopy = $row.case -in @('reopened-new', 'retained-new', 'new-1080', 'new-return720', 'new-series')
            $oldCopy = $row.case -in @('old-ready', 'retained-old', 'old-1080', 'old-return720')
            $expectedValue = if ($isSeries -and $row.role -eq 'heading') { 'Round 2' }
                elseif ($newCopy) { $resultsTextNewValues[$row.role] }
                elseif ($oldCopy) { $resultsTextOldValues[$row.role] } else { $resultsTextDefaultValues[$row.role] }
            $expectedFont = if ($newCopy) { $resultsTextNewFonts[$row.role] } else { $resultsTextDefaultFonts[$row.role] }
            if ($row.session -ne $expectedSession -or $row.series -ne $expectedSeries -or
                $row.round -ne $expectedRound -or $row.a_wins -ne '1' -or $row.b_wins -ne '0' -or
                $row.width -ne $expectedWidth -or $row.height -ne $expectedHeight -or
                $row.revealed -ne $expectedRevealed -or $row.details_visible -ne $expectedDetails -or
                $row.proceed_visible -ne $expectedProceed -or $row.visible -ne $expectedVisible -or
                $row.expected_value -cne $expectedValue -or $row.font -ne $expectedFont -or
                $row.scale -ne '1' -or $row.shadow_x -ne '0' -or $row.shadow_y -ne '0' -or
                $row.full_region_match -ne 'true' -or $row.frozen_session_setup_save_rng -ne 'true') {
                throw "Results text case $($row.case)/$($row.role) changed its literal copy, session/reveal state, capture or geometry contract."
            }
            foreach ($metric in @('raw_width', 'raw_height', 'x', 'y')) {
                $value = 0.0
                if (-not [double]::TryParse($row.$metric, [Globalization.NumberStyles]::Float,
                    [Globalization.CultureInfo]::InvariantCulture, [ref]$value) -or
                    [double]::IsNaN($value) -or [double]::IsInfinity($value) -or
                    ($metric -in @('raw_width', 'raw_height') -and $value -le 0.0) -or
                    ($metric -in @('x', 'y') -and $value -lt 0.0)) {
                    throw "Results text case $($row.case)/$($row.role) contains an invalid $metric."
                }
            }
        }
        $resultsTextExpectedCrops = [ordered]@{
            'results-text-default-single.ppm' = @(800, 170)
            'results-text-default-series.ppm' = @(800, 170)
            'results-text-old-720.ppm' = @(800, 170)
            'results-text-new-720.ppm' = @(800, 170)
            'results-text-new-series.ppm' = @(800, 170)
            'results-text-restored-default.ppm' = @(800, 170)
            'results-text-old-1080.ppm' = @(1200, 255)
            'results-text-new-1080.ppm' = @(1200, 255)
        }
        $resultsTextCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'results-text-*.ppm' -File)
        $resultsTextExpectedNames = @($resultsTextExpectedCrops.Keys) + @('results-text.csv', 'results-text-summary.json')
        $resultsTextNames = @(Get-ChildItem -LiteralPath $captures -Filter 'results-text*' -File | Select-Object -ExpandProperty Name)
        if ($resultsTextCrops.Count -ne 8 -or
            (($resultsTextNames | Sort-Object) -join '|') -ne (($resultsTextExpectedNames | Sort-Object) -join '|')) {
            throw 'The Results text leaf emitted unexpected or missing compact crops, CSV or summary files.'
        }
        foreach ($crop in $resultsTextCrops) {
            $size = $resultsTextExpectedCrops[$crop.Name]
            $header = @(Get-Content -LiteralPath $crop.FullName -TotalCount 3)
            $expectedHeader = "P6`n$($size[0]) $($size[1])`n255`n"
            $expectedBytes = [Text.Encoding]::ASCII.GetByteCount($expectedHeader) + $size[0] * $size[1] * 3
            if ($header.Count -ne 3 -or $header[0] -ne 'P6' -or $header[1] -ne "$($size[0]) $($size[1])" -or
                $header[2] -ne '255' -or $crop.Length -ne $expectedBytes) {
                throw "Results text crop $($crop.Name) differs from its exact dimensions or RGB payload budget."
            }
        }
        $resultsTextSaveCalls = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -match 'Saving\.\.\.$' }).Count
        $resultsTextSaveSuccesses = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -eq 'Save was successful.' }).Count
        if ($resultsTextSaveCalls -ne 1 -or $resultsTextSaveSuccesses -ne 1) {
            throw 'The Results text leaf must perform exactly its one successful private seed Save.'
        }
        $result['observed_counts'] = [ordered]@{
            fixture = $resultsTextSummary
            updates = 43
            readbacks = 52
            csv_rows = 51
            crops = 8
            private_save_calls = $resultsTextSaveCalls
            successful_private_saves = $resultsTextSaveSuccesses
        }
