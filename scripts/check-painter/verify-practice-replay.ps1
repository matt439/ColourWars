# Dot-sourced into the scope of scripts/check-painter.ps1. The guard
# if ($PracticeReplayOnly) stays there; this is the leaf's post-run evidence check.

        $practice = Get-Content -LiteralPath (Join-Path $captures 'practice-replay-summary.json') -Raw | ConvertFrom-Json
        $practiceRows = @(Import-Csv -LiteralPath (Join-Path $captures 'practice-replay.csv'))
        $expectedPracticeSteps = if ($debug) { 4920 } else { 4800 }
        $expectedPracticeCases = @('sustained-practice-60-seconds', 'practice-pause-focus-resume', 'practice-restart',
            'timed-menu-solo', 'timed-developer-course', 'timed-multiplayer', 'timed-to-practice-clears-old-tape')
        if ($debug) { $expectedPracticeCases += 'independent-debug-recorder' }
        if ($practice.outcome -cne 'passed' -or $practice.scope -cne 'actual-GameLevel-practice-replay' -or
            $practice.practice_steps -ne $expectedPracticeSteps -or $practice.timed_cases -ne 3 -or
            $practice.reproduced_steps -lt 500 -or $practice.state_updates -le 4800 -or
            $practice.practice_steps_retained -ne 0 -or $practice.practice_gaps_retained -ne 0 -or
            $practice.GPU_draws -ne 0 -or $practice.GPU_reads -ne 0 -or $practice.physical_pad_claim -ne $false -or
            $practice.debug_recorder -ne $debug -or $practiceRows.Count -ne $expectedPracticeCases.Count -or
            @($practiceRows.case | Sort-Object -Unique).Count -ne $expectedPracticeCases.Count -or
            @(Compare-Object -ReferenceObject $expectedPracticeCases -DifferenceObject @($practiceRows.case)).Count -ne 0) {
            throw 'Actual GameLevel practice/replay receipts differ from the bounded complete cases.'
        }
        foreach ($row in $practiceRows) {
            if ($row.practice -eq '1' -and ($row.tape_steps -cne '0' -or $row.gaps -cne '0')) {
                throw 'Actual practice retained shipping tape inputs or gaps.'
            }
            if ($row.case -like 'timed-*' -and $row.case -ne 'timed-to-practice-clears-old-tape' -and
                ([int]$row.tape_steps -lt 170 -or [int]$row.gaps -lt 2 -or $row.reproduced_steps -cne $row.tape_steps)) {
                throw 'A timed actual GameLevel control lost steps, gaps or its replay.'
            }
        }
        if (@(Get-ChildItem -LiteralPath $captures -File | Where-Object { $_.Extension -in @('.ppm', '.png', '.rgba') }).Count) {
            throw 'The zero-raster practice replay scope emitted a raster artifact.'
        }
        $result.practice_replay_receipt = $practice
