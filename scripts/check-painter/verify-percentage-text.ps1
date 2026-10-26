# Dot-sourced into the scope of scripts/check-painter.ps1. The guard
# if ($PercentageTextOnly) stays there; this is the leaf's post-run evidence check.

        $percentageSummary = Get-Content -LiteralPath (Join-Path $captures 'percentage-summary.json') -Raw | ConvertFrom-Json
        $percentageRows = @(Import-Csv -LiteralPath (Join-Path $captures 'percentage-values.csv'))
        $percentageCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'percentage-*.ppm' -File)
        if ($percentageSummary.outcome -ne 'passed' -or $percentageSummary.updates -ne 132 -or
            $percentageSummary.readbacks -ne 87 -or $percentageSummary.value_csv_rows -ne 52 -or
            $percentageSummary.status_comparisons -ne 4 -or $percentageSummary.compact_crops -ne 8 -or
            $percentageRows.Count -ne 52 -or @($percentageRows | Where-Object { $_.match -ne 'true' }).Count -ne 0 -or
            $percentageCrops.Count -ne 8) {
            throw 'The percentage leaf evidence does not match its frozen update/readback/CSV/crop budgets.'
        }
        # Controls gained Keyboard Jump and Pad Jump before the volume rows. Sticks keeps
        # its original Move/Aim rows; every observed page still emits one pair.
        foreach ($observation in @($percentageRows | Group-Object -Property case, page, width, height)) {
            $observed = @($observation.Group)
            $expectedRows = switch ($observed[0].page) {
                'controls' { @(3, 4) }
                'sticks' { @(1, 2) }
                default { throw 'The percentage leaf contains an unknown settings page.' }
            }
            $actualRows = @($observed | ForEach-Object { [int]$_.row } | Sort-Object)
            if ($actualRows.Count -ne 2 -or $actualRows[0] -ne $expectedRows[0] -or
                $actualRows[1] -ne $expectedRows[1]) {
                throw 'The percentage leaf does not retain Controls rows3/4 and Sticks rows1/2 in each observation.'
            }
        }
        $result['observed_counts'] = $percentageSummary
