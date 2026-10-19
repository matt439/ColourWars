# Dot-sourced into the scope of scripts/check-painter.ps1. The guard
# if ($PercentageTextOnly) stays there; this is the leaf's post-run evidence check.

        $percentageSummary = Get-Content -LiteralPath (Join-Path $captures 'percentage-summary.json') -Raw | ConvertFrom-Json
        $percentageRows = @(Import-Csv -LiteralPath (Join-Path $captures 'percentage-values.csv'))
        $percentageCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'percentage-*.ppm' -File)
        if ($percentageSummary.outcome -ne 'passed' -or $percentageSummary.updates -ne 114 -or
            $percentageSummary.readbacks -ne 87 -or $percentageSummary.value_csv_rows -ne 52 -or
            $percentageSummary.status_comparisons -ne 4 -or $percentageSummary.compact_crops -ne 8 -or
            $percentageRows.Count -ne 52 -or @($percentageRows | Where-Object { $_.match -ne 'true' }).Count -ne 0 -or
            $percentageCrops.Count -ne 8) {
            throw 'The percentage leaf evidence does not match its frozen update/readback/CSV/crop budgets.'
        }
        $result['observed_counts'] = $percentageSummary
