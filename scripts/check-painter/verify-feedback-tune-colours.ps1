# Dot-sourced into the scope of scripts/check-painter.ps1. The guard
# if ($FeedbackTuneColoursOnly) stays there; this is the leaf's post-run evidence check.

        $feedbackConsumerSummary = Get-Content -LiteralPath (Join-Path $captures 'feedback-colour-consumer-summary.json') -Raw | ConvertFrom-Json
        $feedbackConsumerRows = @(Import-Csv -LiteralPath (Join-Path $captures 'feedback-colour-consumers.csv'))
        $feedbackCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'feedback-*.ppm' -File)
        $feedbackExpectedNames = @(foreach ($palette in @('default', 'rgb', 'rgba128', 'alpha0')) {
            foreach ($background in @('black', 'nonblack')) { "feedback-colour-$palette-$background.ppm" }
        })
        $feedbackExpectedPairs = @(foreach ($palette in @('default', 'rgb', 'rgba128', 'alpha0')) {
            foreach ($background in @('black', 'nonblack')) { "$palette/$background" }
        })
        $feedbackObservedPairs = @($feedbackConsumerRows | ForEach-Object { "$($_.palette)/$($_.background)" })
        if ($feedbackConsumerSummary.scope -ne 'consumer-only' -or $feedbackConsumerSummary.state_updates -ne 0 -or
            $feedbackConsumerSummary.readbacks -ne 56 -or $feedbackConsumerSummary.csv_rows -ne 8 -or
            $feedbackConsumerSummary.crops -ne 8 -or $feedbackConsumerSummary.controls -ne 8 -or
            $feedbackConsumerRows.Count -ne 8 -or @($feedbackConsumerRows | Where-Object {
                $_.normal_match -ne 'true' -or $_.reduced_match -ne 'true' -or $_.team_centres -ne 'true' -or
                $_.frozen_geometry_timers_rng -ne 'true' -or $_.scope -ne 'consumer-only' -or
                ($_.palette -eq 'default' -and $_.control -ne 'opaque-legacy-parity-and-double-fade-rejected') -or
                ($_.palette -eq 'rgb' -and $_.control -ne 'changed-rgb') -or
                ($_.palette -in @('rgba128', 'alpha0') -and $_.control -ne 'legacy-straight-alpha-rejected')
            }).Count -ne 0 -or $feedbackCrops.Count -ne 8 -or
            (($feedbackObservedPairs | Sort-Object) -join '|') -ne (($feedbackExpectedPairs | Sort-Object) -join '|') -or
            (($feedbackCrops.Name | Sort-Object) -join '|') -ne (($feedbackExpectedNames | Sort-Object) -join '|')) {
            throw 'The feedback consumer evidence does not match its frozen literal-oracle/readback/CSV/crop/control budgets.'
        }
        foreach ($crop in $feedbackCrops) {
            $header = @(Get-Content -LiteralPath $crop.FullName -TotalCount 3)
            if ($header.Count -ne 3 -or $header[0] -ne 'P6' -or $header[1] -ne '360 280' -or $header[2] -ne '255') {
                throw 'A feedback colour crop exceeds its frozen 360x280 pixel budget.'
            }
        }
        $feedbackSaveCalls = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -match 'Saving\.\.\.$' }).Count
        $feedbackSaveSuccesses = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -eq 'Save was successful.' }).Count
        $feedbackObserved = [ordered]@{
            consumer = $feedbackConsumerSummary
            state_updates = 0
            readbacks = 56
            csv_rows = 8
            crops = 8
            private_save_calls = $feedbackSaveCalls
            successful_private_saves = $feedbackSaveSuccesses
        }
        if ($debug) {
            $feedbackTuneSummary = Get-Content -LiteralPath (Join-Path $captures 'feedback-tune-summary.json') -Raw | ConvertFrom-Json
            $feedbackTuneRows = @(Import-Csv -LiteralPath (Join-Path $captures 'feedback-tune-colours.csv'))
            $feedbackTranscript = Get-Item -LiteralPath (Join-Path $captures 'feedback-tune-commands.log')
            $feedbackExpectedStates = @('baseline', 'named', 'reset-original', 'rgb', 'rgba128', 'saved-rgba',
                'edited-after-save', 'reset-saved-rgba', 'alpha0', 'reset-alpha0-to-saved')
            $feedbackExpectedTunePairs = @(foreach ($state in $feedbackExpectedStates) {
                foreach ($background in @('black', 'nonblack')) { "$state/$background" }
            })
            $feedbackObservedTunePairs = @($feedbackTuneRows | ForEach-Object { "$($_.state)/$($_.background)" })
            if ($feedbackTuneSummary.scope -ne 'debug-validated-commands' -or $feedbackTuneSummary.state_updates -ne 0 -or
                $feedbackTuneSummary.commands -ne 15 -or $feedbackTuneSummary.readbacks -ne 66 -or
                $feedbackTuneSummary.csv_rows -ne 20 -or $feedbackTuneSummary.crops -ne 0 -or
                $feedbackTuneRows.Count -ne 20 -or $feedbackTranscript.Length -le 0 -or
                $feedbackSaveCalls -ne 2 -or $feedbackSaveSuccesses -ne 2 -or
                @($feedbackTuneRows | Where-Object {
                    $_.normal_match -ne 'true' -or $_.team_centres -ne 'true' -or
                    $_.frozen_geometry_timers_rng -ne 'true' -or $_.private_bytes -ne 'true' -or $_.adoption -ne 'live' -or
                    ($_.state -eq 'rgba128' -and $_.reduced_match -ne 'true') -or
                    ($_.state -ne 'rgba128' -and $_.reduced_match -ne 'not-sampled')
                }).Count -ne 0 -or
                (($feedbackObservedTunePairs | Sort-Object) -join '|') -ne (($feedbackExpectedTunePairs | Sort-Object) -join '|')) {
                throw 'The feedback Debug evidence does not match its frozen validated-command/private-save/readback/CSV budgets.'
            }
            $feedbackObserved['developer_commands'] = $feedbackTuneSummary
            $feedbackObserved.readbacks = 122
            $feedbackObserved.csv_rows = 28
        }
        elseif ($feedbackSaveCalls -ne 0 -or $feedbackSaveSuccesses -ne 0 -or
            (Test-Path -LiteralPath (Join-Path $captures 'feedback-tune-summary.json')) -or
            (Test-Path -LiteralPath (Join-Path $captures 'feedback-tune-colours.csv')) -or
            (Test-Path -LiteralPath (Join-Path $captures 'feedback-tune-commands.log')) -or
            (Test-Path -LiteralPath (Join-Path $captures 'feedback-tune-scratch'))) {
            throw 'Release feedback evidence must remain consumer-only without developer artifacts or save calls.'
        }
        $result['observed_counts'] = $feedbackObserved
