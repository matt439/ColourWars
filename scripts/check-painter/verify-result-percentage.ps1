# Dot-sourced into the scope of scripts/check-painter.ps1. The guard
# if ($ResultPercentageOnly) stays there; this is the leaf's post-run evidence check.

        $resultPercentageSummary = Get-Content -LiteralPath (Join-Path $captures 'result-percentage-summary.json') -Raw | ConvertFrom-Json
        $resultPercentageRows = @(Import-Csv -LiteralPath (Join-Path $captures 'result-percentage.csv'))
        $resultPercentageResultCases = @(
            foreach ($case in @('legacy-initial', 'legacy-fill-crossed', 'legacy-details', 'legacy-prompt-crossed', 'legacy-ready')) {
                "$case|solo-a"
            }
            'competitive-initial|competitive'
            'competitive-ready|competitive'
            'solo-b-ready|solo-b'
            'default-1080|solo-a'
            'default-return720|solo-a'
            foreach ($case in @('old-primary', 'retained-old', 'reopened-new', 'retained-new',
                'old-1080', 'old-return720', 'new-1080', 'new-return720')) {
                foreach ($fixture in @('solo-a', 'competitive')) { "$case|$fixture" }
            }
            'restored-default|solo-a'
        )
        $resultPercentageMonitorCases = @('default-primary', 'default-1080', 'default-return720',
            'old-primary', 'retained-old', 'old-1080', 'old-return720', 'new-primary', 'retained-new',
            'new-1080', 'new-return720', 'restored-retained-default')
        $resultPercentageExpectedPairs = @(
            foreach ($case in $resultPercentageResultCases) {
                foreach ($role in @('winner', 'team-a', 'team-b')) { "results|$case|$role" }
            }
            foreach ($case in $resultPercentageMonitorCases) {
                foreach ($role in @('team-a', 'team-b')) { "monitor|$case|2p-roller|$role" }
            }
        )
        $resultPercentageExpectedColumns = @('consumer', 'case', 'fixture', 'role', 'width', 'height', 'expected_value',
            'font', 'raw_width', 'raw_height', 'x', 'y', 'scale', 'visible', 'revealed', 'details_visible',
            'proceed_visible', 'region_match', 'neighbor_match', 'frozen_state_setup_save_rng')
        $resultPercentageResultRows = @($resultPercentageRows | Where-Object { $_.consumer -ceq 'results' })
        $resultPercentageMonitorRows = @($resultPercentageRows | Where-Object { $_.consumer -ceq 'monitor' })
        if ($resultPercentageSummary.outcome -cne 'passed' -or $resultPercentageSummary.scope -cne 'result-percentage-only' -or
            $resultPercentageSummary.updates -ne 60 -or $resultPercentageSummary.level_updates -ne 240 -or
            $resultPercentageSummary.readbacks -ne 120 -or $resultPercentageSummary.csv_rows -ne 105 -or
            $resultPercentageSummary.results_rows -ne 81 -or $resultPercentageSummary.monitor_rows -ne 24 -or
            $resultPercentageSummary.compact_crops -ne 8 -or $resultPercentageSummary.page_openings -ne 8 -or
            $resultPercentageSummary.proceed_pops -ne 8 -or $resultPercentageSummary.font_negative_controls -ne 6 -or
            $resultPercentageSummary.resize_updates -ne 0 -or $resultPercentageSummary.save_calls -ne 1 -or
            $resultPercentageSummary.level_builds -ne 3 -or $resultPercentageSummary.crop_pixels -ne 681072 -or
            $resultPercentageRows.Count -ne 105 -or $resultPercentageResultRows.Count -ne 81 -or
            $resultPercentageMonitorRows.Count -ne 24 -or
            (($resultPercentageRows[0].PSObject.Properties.Name) -join '|') -cne ($resultPercentageExpectedColumns -join '|') -or
            (($resultPercentageRows | ForEach-Object { "$($_.consumer)|$($_.case)|$($_.fixture)|$($_.role)" } | Sort-Object) -join ';') -cne
                (($resultPercentageExpectedPairs | Sort-Object) -join ';')) {
            throw 'The Result percentage leaf does not match its frozen summary, CSV schema, case-role pairs, separate state/Level update counts or readback/proceed budgets.'
        }
        $resultPercentageDefaultValues = @{
            'solo-a|winner' = '72.0% Painted'; 'solo-a|team-a' = 'A 72.0%'; 'solo-a|team-b' = 'B 0.0%'
            'competitive|winner' = 'Team A Wins!'; 'competitive|team-a' = 'A 60.0%'; 'competitive|team-b' = 'B 40.0%'
            'solo-b|winner' = '72.0% Painted'; 'solo-b|team-a' = 'A 0.0%'; 'solo-b|team-b' = 'B 72.0%'
        }
        $resultPercentageOldValues = @{
            'solo-a|winner' = 'Old~ 72.0%'; 'solo-a|team-a' = 'A~ 72.0%'; 'solo-a|team-b' = 'B~ 0.0%'
            'competitive|winner' = 'Team A Wins!'; 'competitive|team-a' = 'A~ 60.0%'; 'competitive|team-b' = 'B~ 40.0%'
        }
        $resultPercentageNewValues = @{
            'solo-a|winner' = '^solo 72.0%'; 'solo-a|team-a' = '^72.0% A'; 'solo-a|team-b' = '^0.0% B'
            'competitive|winner' = 'Team A Wins!'; 'competitive|team-a' = '^60.0% A'; 'competitive|team-b' = '^40.0% B'
        }
        $resultPercentageMonitorDefaultValues = @{ 'team-a' = 'A 100.0%'; 'team-b' = 'B 0.0%' }
        $resultPercentageMonitorOldValues = @{ 'team-a' = 'A~ 100.0%'; 'team-b' = 'B~ 0.0%' }
        $resultPercentageMonitorNewValues = @{ 'team-a' = '^100.0% A'; 'team-b' = '^0.0% B' }
        foreach ($row in $resultPercentageRows) {
            $isResults = $row.consumer -ceq 'results'
            $expectedWidth = if ($row.case -like '*-1080') { '1920' } else { '1280' }
            $expectedHeight = if ($expectedWidth -eq '1920') { '1080' } else { '720' }
            if ($isResults) {
                $newCopy = $row.case -in @('reopened-new', 'retained-new', 'new-1080', 'new-return720')
                $oldCopy = $row.case -in @('old-primary', 'retained-old', 'old-1080', 'old-return720')
                $literalKey = "$($row.fixture)|$($row.role)"
                $expectedValue = if ($newCopy) { $resultPercentageNewValues[$literalKey] }
                    elseif ($oldCopy) { $resultPercentageOldValues[$literalKey] } else { $resultPercentageDefaultValues[$literalKey] }
                $expectedFont = if ($row.role -ceq 'winner') {
                    if ($newCopy) { 'result_percentage_item_caret' } else { 'lato_black_48' }
                }
                else { if ($newCopy) { 'result_percentage_detail_caret' } else { 'lato_black_36' } }
                $expectedRevealed = if ($row.fixture -ceq 'competitive' -or $row.case -ceq 'solo-b-ready') { 'true' } else { 'false' }
                $expectedDetails = if ($row.case -in @('legacy-initial', 'legacy-fill-crossed')) { 'false' } else { 'true' }
                $expectedProceed = if ($row.case -in @('legacy-initial', 'legacy-fill-crossed', 'legacy-details',
                    'legacy-prompt-crossed', 'competitive-initial')) { 'false' } else { 'true' }
                $visible = if ($row.role -ceq 'winner') { $expectedDetails -ceq 'true' -and $expectedRevealed -ceq 'false' }
                    elseif ($row.role -ceq 'team-a') { $expectedDetails -ceq 'true' -and $row.fixture -cne 'solo-b' }
                    else { $expectedDetails -ceq 'true' -and $row.fixture -cne 'solo-a' }
                $expectedVisible = if ($visible) { 'true' } else { 'false' }
            }
            else {
                $newCopy = $row.case -in @('new-primary', 'retained-new', 'new-1080', 'new-return720')
                $oldCopy = $row.case -in @('old-primary', 'retained-old', 'old-1080', 'old-return720')
                $expectedValue = if ($newCopy) { $resultPercentageMonitorNewValues[$row.role] }
                    elseif ($oldCopy) { $resultPercentageMonitorOldValues[$row.role] } else { $resultPercentageMonitorDefaultValues[$row.role] }
                $expectedFont = if ($newCopy) { 'result_percentage_monitor_caret' } else { 'bowlby_one_144' }
                $expectedRevealed = 'true'; $expectedDetails = 'true'; $expectedProceed = 'false'; $expectedVisible = 'true'
            }
            if ($row.width -cne $expectedWidth -or $row.height -cne $expectedHeight -or
                $row.expected_value -cne $expectedValue -or $row.font -cne $expectedFont -or
                $row.visible -cne $expectedVisible -or $row.revealed -cne $expectedRevealed -or
                $row.details_visible -cne $expectedDetails -or $row.proceed_visible -cne $expectedProceed -or
                $row.region_match -cne 'true' -or $row.neighbor_match -cne 'true' -or
                $row.frozen_state_setup_save_rng -cne 'true') {
                throw "Result percentage case $($row.consumer)/$($row.case)/$($row.fixture)/$($row.role) changed its literal text, font, reveal, capture or display contract."
            }
            $metricValues = @{}
            foreach ($metric in @('raw_width', 'raw_height', 'x', 'y', 'scale')) {
                $value = 0.0
                if (-not [double]::TryParse($row.$metric, [Globalization.NumberStyles]::Float,
                    [Globalization.CultureInfo]::InvariantCulture, [ref]$value) -or
                    [double]::IsNaN($value) -or [double]::IsInfinity($value) -or
                    ($metric -in @('raw_width', 'raw_height', 'scale') -and $value -le 0.0) -or
                    ($metric -in @('x', 'y') -and $value -lt 0.0)) {
                    throw "Result percentage case $($row.consumer)/$($row.case)/$($row.fixture)/$($row.role) contains an invalid $metric."
                }
                $metricValues[$metric] = $value
            }
            if ($isResults) {
                $expectedX = if ($row.role -ceq 'winner') { (1920.0 - $metricValues.raw_width) / 2.0 }
                    elseif ($row.role -ceq 'team-a') { 620.0 } else { 1300.0 - $metricValues.raw_width }
                $expectedY = if ($row.role -ceq 'winner') { 255.0 } else { 500.0 }
                if ($row.scale -cne '1' -or [Math]::Abs($metricValues.x - $expectedX) -gt 0.01 -or
                    [Math]::Abs($metricValues.y - $expectedY) -gt 0.01) {
                    throw "Result percentage case $($row.case)/$($row.fixture)/$($row.role) changed its unscaled design placement."
                }
            }
        }
        $resultPercentageExpectedCrops = [ordered]@{
            'result-percentage-results-default-720.ppm' = @(496, 126)
            'result-percentage-results-old-720.ppm' = @(496, 126)
            'result-percentage-results-new-720.ppm' = @(496, 126)
            'result-percentage-results-new-1080.ppm' = @(744, 189)
            'result-percentage-monitor-default-720.ppm' = @(764, 88)
            'result-percentage-monitor-old-720.ppm' = @(764, 88)
            'result-percentage-monitor-new-720.ppm' = @(764, 88)
            'result-percentage-monitor-new-1080.ppm' = @(1146, 132)
        }
        $resultPercentageCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'result-percentage-*.ppm' -File)
        $resultPercentageExpectedNames = @($resultPercentageExpectedCrops.Keys) + @('result-percentage.csv', 'result-percentage-summary.json')
        $resultPercentageNames = @(Get-ChildItem -LiteralPath $captures -Filter 'result-percentage*' -File | Select-Object -ExpandProperty Name)
        if ($resultPercentageCrops.Count -ne 8 -or
            (($resultPercentageNames | Sort-Object) -join '|') -cne (($resultPercentageExpectedNames | Sort-Object) -join '|')) {
            throw 'The Result percentage leaf emitted unexpected or missing compact crops, CSV or summary files.'
        }
        foreach ($crop in $resultPercentageCrops) {
            $size = $resultPercentageExpectedCrops[$crop.Name]
            $header = @(Get-Content -LiteralPath $crop.FullName -TotalCount 3)
            $expectedHeader = "P6`n$($size[0]) $($size[1])`n255`n"
            $expectedBytes = [Text.Encoding]::ASCII.GetByteCount($expectedHeader) + $size[0] * $size[1] * 3
            if ($header.Count -ne 3 -or $header[0] -cne 'P6' -or $header[1] -cne "$($size[0]) $($size[1])" -or
                $header[2] -cne '255' -or $crop.Length -ne $expectedBytes) {
                throw "Result percentage crop $($crop.Name) differs from its exact dimensions or RGB payload budget."
            }
        }
        $resultPercentageSaveCalls = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -match 'Saving\.\.\.$' }).Count
        $resultPercentageSaveSuccesses = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -ceq 'Save was successful.' }).Count
        if ($resultPercentageSaveCalls -ne 1 -or $resultPercentageSaveSuccesses -ne 1) {
            throw 'The Result percentage leaf must perform exactly its one successful private seed Save.'
        }
        $result['observed_counts'] = [ordered]@{
            fixture = $resultPercentageSummary
            updates = 60
            level_updates = 240
            level_builds = 3
            readbacks = 120
            csv_rows = 105
            results_rows = 81
            monitor_rows = 24
            crops = 8
            private_save_calls = $resultPercentageSaveCalls
            successful_private_saves = $resultPercentageSaveSuccesses
        }
