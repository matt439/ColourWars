# Dot-sourced into the scope of scripts/check-painter.ps1. The guard
# if ($RumbleOnly -and $debug) stays there; this is the leaf's Debug background-supplement evidence check.

        $background = Get-Content -LiteralPath (Join-Path $captures 'debug-background-summary.json') -Raw | ConvertFrom-Json
        $backgroundRows = @(Import-Csv -LiteralPath (Join-Path $captures 'debug-background.csv'))
        $backgroundCases = @('default-off-inactive', 'actual-hidden-hard-gate',
            'synthetic-inactive-neutral-presence-muted-before-step', 'foreground-zero-run-edge-discarded',
            'inactive-actual-damage-motors-zero', 'inactive-script-wait',
            'manual-debug-pause-and-script-self-disable-barrier', 'covered-toggle-hard-gate',
            'injected-hidden', 'injected-minimized', 'injected-power-suspended', 'off-inactive-freezes',
            'off-then-activation-ordinary-countin-current-preferences', 'actual-pause-menu-hard-gate',
            'resume-countin-hard-gate', 'missing-pad-not-judged-in-background', 'missing-pad-real-pause',
            'inactive-bot-paid-shot-record',
            'inactive-replay-final-fingerprint', 'new-inactive-level-enabled', 'new-inactive-level-disabled')
        if ($background.outcome -cne 'passed' -or $background.cases -ne 21 -or $backgroundRows.Count -ne 21 -or
            $background.recorded_updates -ne 420 -or
            $background.save_unchanged -ne $true -or
            @($backgroundRows.case | Sort-Object -Unique).Count -ne 21 -or
            @(Compare-Object -ReferenceObject $backgroundCases -DifferenceObject @($backgroundRows.case)).Count -ne 0) {
            throw 'Debug background supplement did not produce its complete bounded 21-case/420-update receipts.'
        }
        $result.background_receipt = $background
