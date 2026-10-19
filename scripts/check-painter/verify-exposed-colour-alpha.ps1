# Dot-sourced into the scope of scripts/check-painter.ps1. The guard
# if ($ExposedColourAlphaOnly) stays there; this is the leaf's post-run evidence check.

        $alphaExpectedCsvNames = @('alpha-home.csv', 'alpha-movement.csv')
        $alphaExpectedSummaryNames = @('alpha-home-summary.json', 'alpha-movement-summary.json')
        $alphaHomeSummary = Get-Content -LiteralPath (Join-Path $captures 'alpha-home-summary.json') -Raw | ConvertFrom-Json
        $alphaHomeRows = @(Import-Csv -LiteralPath (Join-Path $captures 'alpha-home.csv'))
        $alphaHomeSites = @('home', 'mode', 'player-count', 'join', 'credits', 'options', 'exit-confirmation',
            'display-confirmation', 'team', 'weapon', 'stage', 'controls', 'sticks')
        $alphaHomeStates = @('default/black', 'default/nonblack', 'rgba128/black', 'rgba128/nonblack',
            'alpha0/black', 'alpha0/nonblack', 'retained-default/black', 'retained-rgba128/nonblack',
            'retained-alpha0/black', 'restored-default/black')
        $alphaHomeExpected = @(foreach ($site in $alphaHomeSites) {
            foreach ($state in $alphaHomeStates) { "$site/$state" }
        })
        $alphaHomeObserved = @($alphaHomeRows | ForEach-Object { "$($_.site)/$($_.state)/$($_.background)" })
        if ($alphaHomeSummary.scope -ne 'consumer-only' -or $alphaHomeSummary.state_updates -ne 0 -or $alphaHomeSummary.readbacks -ne 235 -or
            $alphaHomeSummary.page_openings -ne 52 -or $alphaHomeSummary.csv_rows -ne 130 -or
            $alphaHomeSummary.crops -ne 4 -or $alphaHomeSummary.controls -ne 8 -or
            $alphaHomeSummary.sites -ne 13 -or $alphaHomeSummary.save_calls -ne 2 -or
            $alphaHomeRows.Count -ne 130 -or @($alphaHomeRows | Where-Object {
                $_.expected_corner -ne 'true' -or $_.frozen_state_rng -ne 'true' -or $_.outcome -ne 'passed' -or
                ($_.state -like 'retained-*' -and ($_.captured_retained -ne 'true' -or $_.reopened -ne 'false')) -or
                ($_.state -notlike 'retained-*' -and ($_.captured_retained -ne 'false' -or $_.reopened -ne 'true'))
            }).Count -ne 0 -or
            (($alphaHomeObserved | Sort-Object) -join '|') -ne (($alphaHomeExpected | Sort-Object) -join '|')) {
            throw 'The exposed-alpha Home evidence does not match its frozen sites, retention flags and readback/CSV/crop budgets.'
        }
        $alphaMovementSummary = Get-Content -LiteralPath (Join-Path $captures 'alpha-movement-summary.json') -Raw | ConvertFrom-Json
        $alphaMovementRows = @(Import-Csv -LiteralPath (Join-Path $captures 'alpha-movement.csv'))
        $alphaMovementExpected = @(foreach ($consumer in @('movement', 'debug-text')) {
            foreach ($palette in @('opaque', 'rgba128', 'alpha0')) {
                foreach ($background in @('black', 'nonblack')) { "$consumer/$palette/$background" }
            }
        })
        $alphaMovementObserved = @($alphaMovementRows | ForEach-Object { "$($_.consumer)/$($_.palette)/$($_.background)" })
        if ($alphaMovementSummary.scope -ne 'movement-debug-text-consumer-alpha' -or
            $alphaMovementSummary.state_updates -ne 0 -or $alphaMovementSummary.fixture_effect_updates -ne 10 -or
            $alphaMovementSummary.readbacks -ne 50 -or $alphaMovementSummary.csv_rows -ne 12 -or
            $alphaMovementSummary.movement_cases -ne 6 -or $alphaMovementSummary.debug_text_cases -ne 6 -or
            $alphaMovementSummary.negative_controls -ne 10 -or $alphaMovementSummary.legacy_parity_controls -ne 4 -or
            $alphaMovementSummary.crops -ne 8 -or $alphaMovementSummary.captured_paint_variants -ne 3 -or
            $alphaMovementSummary.crop_pixels -ne 1112640 -or
            $alphaMovementRows.Count -ne 12 -or @($alphaMovementRows | Where-Object {
                $_.match -ne 'true' -or $_.frozen_state_bounds_rng -ne 'true' -or
                ($_.palette -eq 'opaque' -and ($_.legacy_parity -ne 'true' -or $_.straight_alpha_rejected -ne 'not-sampled')) -or
                ($_.palette -ne 'opaque' -and ($_.legacy_parity -ne 'not-sampled' -or $_.straight_alpha_rejected -ne 'true')) -or
                ($_.consumer -eq 'movement' -and ($_.captured_paint -ne 'opaque-rgba128-alpha0-retained' -or $_.effect_count -ne '20' -or
                    ($_.palette -eq 'opaque' -and $_.double_fade_rejected -ne 'true') -or
                    ($_.palette -ne 'opaque' -and $_.double_fade_rejected -ne 'not-sampled'))) -or
                ($_.consumer -eq 'debug-text' -and ($_.captured_paint -ne 'not-applicable' -or
                    $_.effect_count -ne '0' -or $_.double_fade_rejected -ne 'not-applicable'))
            }).Count -ne 0 -or
            (($alphaMovementObserved | Sort-Object) -join '|') -ne (($alphaMovementExpected | Sort-Object) -join '|')) {
            throw 'The exposed-alpha movement/debug-text evidence does not match its frozen literal-oracle, control and readback/CSV budgets.'
        }
        $alphaExpectedCrops = [ordered]@{}
        foreach ($palette in @('rgba128', 'alpha0')) {
            foreach ($background in @('black', 'nonblack')) {
                $alphaExpectedCrops["alpha-home-$palette-$background.ppm"] = @(128, 96)
                $alphaExpectedCrops["alpha-movement-$palette-$background.ppm"] = @(1280, 192)
                $alphaExpectedCrops["alpha-debug-text-$palette-$background.ppm"] = @(300, 108)
            }
        }
        $alphaSaveCalls = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -match 'Saving\.\.\.$' }).Count
        $alphaSaveSuccesses = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -eq 'Save was successful.' }).Count
        $alphaObserved = [ordered]@{
            home = $alphaHomeSummary
            movement_debug_text = $alphaMovementSummary
            state_updates = 0
            readbacks = 285
            csv_rows = 142
            crops = 12
            private_save_calls = $alphaSaveCalls
            successful_private_saves = $alphaSaveSuccesses
        }
        if ($alphaSaveCalls -ne 2 -or $alphaSaveSuccesses -ne 2) {
            throw 'The exposed-alpha leaf must perform exactly its two successful private Home seed Saves.'
        }
        if ($debug) {
            $alphaConsoleSummary = Get-Content -LiteralPath (Join-Path $captures 'alpha-console-summary.json') -Raw | ConvertFrom-Json
            $alphaConsoleRows = @(Import-Csv -LiteralPath (Join-Path $captures 'alpha-console.csv'))
            $alphaConsoleTranscript = Get-Item -LiteralPath (Join-Path $captures 'alpha-console-commands.log')
            $alphaConsoleExpected = @(foreach ($case in @('baseline', 'foreground-partial', 'foreground-zero', 'shadow-partial', 'shadow-zero', 'reset')) {
                foreach ($consumer in @('frozen', 'live', 'panels', 'perf')) {
                    foreach ($background in @('black', 'blue')) { "$case/$consumer/$background" }
                }
            })
            $alphaConsoleObserved = @($alphaConsoleRows | ForEach-Object { "$($_.case)/$($_.consumer)/$($_.background)" })
            if ($alphaConsoleSummary.outcome -ne 'passed' -or $alphaConsoleSummary.readbacks -ne 104 -or
                $alphaConsoleSummary.oracle_comparisons -ne 40 -or $alphaConsoleSummary.reset_comparisons -ne 8 -or
                $alphaConsoleSummary.frozen_repeats -ne 16 -or $alphaConsoleSummary.csv_rows -ne 48 -or
                $alphaConsoleSummary.commands -ne 8 -or $alphaConsoleSummary.crops -ne 4 -or
                $alphaConsoleSummary.gameplay_updates -ne 0 -or $alphaConsoleSummary.save_constructions -ne 1 -or
                $alphaConsoleSummary.save_write_commands -ne 0 -or $alphaConsoleSummary.backgrounds -ne 2 -or
                $alphaConsoleSummary.consumers -ne 4 -or $alphaConsoleRows.Count -ne 48 -or
                $alphaConsoleTranscript.Length -le 0 -or @($alphaConsoleRows | Where-Object {
                    $_.oracle_equal -ne 'true' -or
                    ($_.case -in @('baseline', 'reset') -and $_.pending_edits -ne '0') -or
                    ($_.case -notin @('baseline', 'reset') -and $_.pending_edits -ne '1') -or
                    ($_.case -in @('baseline', 'foreground-partial') -and $_.repeat_checked -ne 'true') -or
                    ($_.case -notin @('baseline', 'foreground-partial') -and $_.repeat_checked -ne 'false')
                }).Count -ne 0 -or
                (($alphaConsoleObserved | Sort-Object) -join '|') -ne (($alphaConsoleExpected | Sort-Object) -join '|')) {
                throw 'The exposed-alpha Debug console evidence does not match its frozen command, oracle and readback/CSV budgets.'
            }
            $alphaExpectedCrops['alpha-console-frozen-partial.ppm'] = @(640, 160)
            $alphaExpectedCrops['alpha-console-live-partial.ppm'] = @(640, 160)
            $alphaExpectedCrops['alpha-console-panels-shadow-partial.ppm'] = @(1280, 96)
            $alphaExpectedCrops['alpha-console-perf-shadow-partial.ppm'] = @(640, 240)
            $alphaObserved['console'] = $alphaConsoleSummary
            $alphaExpectedCsvNames += 'alpha-console.csv'
            $alphaExpectedSummaryNames += 'alpha-console-summary.json'
            $alphaObserved.readbacks = 389
            $alphaObserved.csv_rows = 190
            $alphaObserved.crops = 16
        }
        elseif (@(Get-ChildItem -LiteralPath $captures -Filter 'alpha-console-*').Count -ne 0) {
            throw 'Release exposed-alpha evidence must not contain Debug console artifacts.'
        }
        $alphaCsvNames = @(Get-ChildItem -LiteralPath $captures -Filter 'alpha-*.csv' -File | Select-Object -ExpandProperty Name)
        $alphaSummaryNames = @(Get-ChildItem -LiteralPath $captures -Filter 'alpha-*-summary.json' -File | Select-Object -ExpandProperty Name)
        if ((($alphaCsvNames | Sort-Object) -join '|') -ne (($alphaExpectedCsvNames | Sort-Object) -join '|') -or
            (($alphaSummaryNames | Sort-Object) -join '|') -ne (($alphaExpectedSummaryNames | Sort-Object) -join '|')) {
            throw 'The exposed-alpha leaf emitted unexpected or missing CSV/summary files.'
        }
        $alphaCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'alpha-*.ppm' -File)
        if ($alphaCrops.Count -ne $alphaExpectedCrops.Count -or
            (($alphaCrops.Name | Sort-Object) -join '|') -ne (($alphaExpectedCrops.Keys | Sort-Object) -join '|')) {
            throw 'The exposed-alpha leaf emitted unexpected or missing compact crops.'
        }
        foreach ($crop in $alphaCrops) {
            $size = $alphaExpectedCrops[$crop.Name]
            $header = @(Get-Content -LiteralPath $crop.FullName -TotalCount 3)
            $expectedHeader = "P6`n$($size[0]) $($size[1])`n255`n"
            $expectedBytes = [Text.Encoding]::ASCII.GetByteCount($expectedHeader) + $size[0] * $size[1] * 3
            if ($header.Count -ne 3 -or $header[0] -ne 'P6' -or $header[1] -ne "$($size[0]) $($size[1])" -or
                $header[2] -ne '255' -or $crop.Length -ne $expectedBytes) {
                throw "Exposed-alpha crop $($crop.Name) differs from its exact dimensions or RGB payload budget."
            }
        }
        $result['observed_counts'] = $alphaObserved
