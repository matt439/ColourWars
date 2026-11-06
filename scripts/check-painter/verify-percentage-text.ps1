# Dot-sourced into the scope of scripts/check-painter.ps1. The guard
# if ($PercentageTextOnly) stays there; this is the leaf's post-run evidence check.

        $percentageSummary = Get-Content -LiteralPath (Join-Path $captures 'percentage-summary.json') -Raw | ConvertFrom-Json
        $percentageRows = @(Import-Csv -LiteralPath (Join-Path $captures 'percentage-values.csv'))
        $percentageCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'percentage-*.ppm' -File)
        if ($percentageSummary.outcome -ne 'passed' -or $percentageSummary.updates -ne 189 -or
            $percentageSummary.readbacks -ne 87 -or $percentageSummary.value_csv_rows -ne 52 -or
            $percentageSummary.status_comparisons -ne 4 -or $percentageSummary.compact_crops -ne 8 -or
            $percentageRows.Count -ne 52 -or @($percentageRows | Where-Object { $_.match -ne 'true' }).Count -ne 0 -or
            $percentageCrops.Count -ne 8) {
            throw 'The percentage leaf evidence does not match its frozen update/readback/CSV/crop budgets.'
        }
        # The percentage rows of each page, as game/config/presentation/menu_geometry.h's settings_rows
        # names them: Controls Music/SFX after Sticks, Keyboard Jump, Pad Buttons and Profiles; Sticks
        # Move/Aim after Pad. Every observed page emits one pair, and the message names this table.
        $percentageValueRows = [ordered]@{ controls = @(4, 5); sticks = @(1, 2) }
        $percentageRowNames = @($percentageValueRows.Keys | ForEach-Object {
            '{0}{1} rows {2}' -f $_.Substring(0, 1).ToUpperInvariant(), $_.Substring(1), ($percentageValueRows[$_] -join '/')
        }) -join ' and '
        foreach ($observation in @($percentageRows | Group-Object -Property case, page, width, height)) {
            $observed = @($observation.Group)
            if (-not $percentageValueRows.Contains([string]$observed[0].page)) {
                throw 'The percentage leaf contains an unknown settings page.'
            }
            $expectedRows = $percentageValueRows[[string]$observed[0].page]
            $actualRows = @($observed | ForEach-Object { [int]$_.row } | Sort-Object)
            if ($actualRows.Count -ne 2 -or $actualRows[0] -ne $expectedRows[0] -or
                $actualRows[1] -ne $expectedRows[1]) {
                throw "The percentage leaf does not retain $percentageRowNames in each observation."
            }
        }
        $result['observed_counts'] = $percentageSummary
