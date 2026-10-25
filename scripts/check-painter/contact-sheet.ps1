# Dot-sourced into the scope of scripts/check-painter.ps1.
# The playing-size PNG contact sheets written after a passing run.

# Bound every page, including the complete matrix, before allocating a bitmap.
# GDI+ cannot save an arbitrarily tall aggregate even when every crop is valid.
function Write-ContactSheetPages {
    param([object[]] $Images, [int] $CellWidth, [int] $CellHeight,
        [string] $CaptureDirectory, [string] $Stem)
    if ($Images.Count -eq 0) { return }
    $pageWidth = $CellWidth * 4
    $rows = [int][Math]::Min(16, [Math]::Floor(8192.0 / $CellHeight))
    $rows = [int][Math]::Min($rows, [Math]::Floor(16777216.0 / ($pageWidth * $CellHeight)))
    if ($pageWidth -gt 16384 -or $rows -lt 1) { throw 'A native-size painter crop exceeds the review page bounds.' }
    $perPage = $rows * 4
    $font = [Drawing.Font]::new('Arial', 9)
    try {
        for ($start = 0; $start -lt $Images.Count; $start += $perPage) {
            $count = [Math]::Min($perPage, $Images.Count - $start)
            $sheet = $null
            $graphics = $null
            try {
                $sheet = [Drawing.Bitmap]::new($pageWidth, $CellHeight * [int][Math]::Ceiling($count / 4.0))
                $graphics = [Drawing.Graphics]::FromImage($sheet)
                $graphics.Clear([Drawing.Color]::FromArgb(238, 238, 238))
                for ($index = 0; $index -lt $count; ++$index) {
                    $x = ($index % 4) * $CellWidth + 8
                    $y = [int][Math]::Floor($index / 4.0) * $CellHeight + 8
                    $item = $Images[$start + $index]
                    $graphics.DrawString($item.name, $font, [Drawing.Brushes]::Black,
                        [Drawing.RectangleF]::new($x, $y, $CellWidth - 16, 34))
                    $graphics.DrawImageUnscaled($item.bitmap, $x, $y + 36)
                }
                $page = [int]($start / $perPage) + 1
                $name = if ($page -eq 1) { "$Stem.png" } else { '{0}-{1:D3}.png' -f $Stem, $page }
                $sheet.Save((Join-Path $CaptureDirectory $name), [Drawing.Imaging.ImageFormat]::Png)
                Write-Output $name
            }
            finally {
                if ($null -ne $graphics) { $graphics.Dispose() }
                if ($null -ne $sheet) { $sheet.Dispose() }
            }
        }
    }
    finally { $font.Dispose() }
}

# Keep the probes' lossless PPMs, and provide labelled playing-size PNG pages.
# Crops are never enlarged for the readability review.
function Write-ContactSheet {
    param([string] $CaptureDirectory)
    Add-Type -AssemblyName System.Drawing
    $images = [Collections.Generic.List[object]]::new()
    try {
        $cellWidth = 320
        $cellHeight = 160
        $monitorCellWidth = 320
        $monitorCellHeight = 160
        foreach ($file in Get-ChildItem -LiteralPath $CaptureDirectory -Filter '*.ppm' -File |
            Where-Object { $_.Name -like '*-player*.ppm' -or $_.Name -like 'monitor-*.ppm' -or
                $_.Name -like 'hidden-inspection-cq-*-native.ppm' -or
                $_.Name -like 'hidden-inspection-crossfeed-*-native.ppm' -or
                $_.Name -like 'hidden-inspection-press-tower-*-native.ppm' -or
                $_.Name -like 'surface-finish-board-*-detail.ppm' -or
                $_.Name -like 'identity-background-*-badges.ppm' -or
                $_.Name -like 'drift-*-detail.ppm' -or $_.Name -like 'miniature-terrain-*-detail.ppm' -or
                $_.Name -like 'tune-colours-*-detail.ppm' -or $_.Name -like 'parallax-*.ppm' -or
                $_.Name -like 'countdown-*-detail.ppm' -or $_.Name -like 'closing-*.ppm' -or
                $_.Name -like 'supply-*-detail.ppm' -or $_.Name -like 'hud-*-detail.ppm' -or
                $_.Name -like 'resume-*-detail.ppm' -or $_.Name -like 'end-menu-*-detail.ppm' -or
                $_.Name -like 'pause-menu-*-detail.ppm' -or $_.Name -like 'results-menu-*-detail.ppm' -or
                $_.Name -like 'main-menu-*-detail.ppm' -or $_.Name -like 'settings-menu-*-detail.ppm' -or
                $_.Name -like 'controls-menu-*-detail.ppm' -or $_.Name -like 'stick-settings-*-detail.ppm' -or
                ($_.Name -like 'keyboard-jump-*.ppm' -and $_.Name -notlike '*-context.ppm') -or
                $_.Name -like 'percentage-*.ppm' -or $_.Name -like 'feedback-colour-*.ppm' -or $_.Name -like 'combat-colour-*.ppm' -or $_.Name -like 'blocked-colour-*.ppm' -or $_.Name -like 'idle-drip-colour-*.ppm' -or $_.Name -like 'respawn-pen-*.ppm' -or $_.Name -like 'cursor-colour-*.ppm' -or $_.Name -like 'silhouette-colour-*.ppm' -or $_.Name -like 'reveal-colour-*.ppm' -or $_.Name -like 'cursor-material-*.ppm' -or $_.Name -like 'desktop-colour-*.ppm' -or $_.Name -like 'power-light-colour-*.ppm' -or $_.Name -like 'program-colour-*.ppm' -or $_.Name -like 'acting-colour-*.ppm' -or $_.Name -like 'glass-colour-*.ppm' -or $_.Name -like 'silhouette-body-colour-*.ppm' -or $_.Name -like 'scribble-colour-*.ppm' -or $_.Name -like 'hud-health-colour-*.ppm' -or $_.Name -like 'hud-track-colour-*.ppm' -or $_.Name -like 'hud-timer-colour-*.ppm' -or $_.Name -like 'hud-label-colour-*.ppm' -or $_.Name -like 'hud-timer-shadow-colour-*.ppm' -or $_.Name -like 'hud-closing-text-colour-*.ppm' -or $_.Name -like 'crack-colour-*.ppm' -or $_.Name -like 'limb-colour-*.ppm' -or $_.Name -like 'quiet-face-colour-*.ppm' -or $_.Name -like 'program-level-*.ppm' -or $_.Name -like 'alpha-*.ppm' -or $_.Name -like 'duration-*.ppm' -or $_.Name -like 'resolution-*.ppm' -or $_.Name -like 'end-continuation-*.ppm' -or $_.Name -like 'results-text-*.ppm' -or $_.Name -like 'session-result-*.ppm' -or $_.Name -like 'series-score-*.ppm' -or $_.Name -like 'round-heading-*.ppm' -or $_.Name -like 'competitive-winner-*.ppm' -or $_.Name -like 'contribution-team-*.ppm' -or $_.Name -like 'result-percentage-*.ppm' -or
                $_.Name -like 'hud-closing-materials-*.ppm' -or
                $_.Name -like 'closing-shade-colour-*.ppm' -or
                $_.Name -like 'hud-paper-colour-*.ppm' -or
                $_.Name -like 'stage-preview-background-colour-*.ppm' -or
                $_.Name -like 'stage-count-*-detail.ppm' -or $_.Name -like 'stage-summary-*-detail.ppm' -or $_.Name -like 'landmark-*-detail.ppm' -or $_.Name -like 'menu-labels-*-detail.ppm' -or $_.Name -like 'weapon-tune-colours-*-detail.ppm' -or $_.Name -like 'team-tune-colours-*-detail.ppm' -or $_.Name -like 'team-focus-*-detail.ppm' -or $_.Name -like 'stick-readouts-*-detail.ppm' -or $_.Name -like 'connection-text-*-detail.ppm' -or $_.Name -like 'remembered-setup-*-detail.ppm' -or $_.Name -like 'team-setup-*-detail.ppm' -or $_.Name -like 'weapon-catalog-*-detail.ppm' -or
                $_.Name -like 'menu-text-*-detail.ppm' -or
                $_.Name -like 'menu-motion-*-detail.ppm' -or
                $_.Name -like 'setup-motion-*-detail.ppm' -or
                $_.Name -like 'join-stage-motion-*-detail.ppm' -or
                $_.Name -like 'setup-paper-*-detail.ppm' -or
                $_.Name -like 'join-paper-*-detail.ppm' -or
                $_.Name -like 'team-paper-*-detail.ppm' -or
                $_.Name -like 'weapon-paper-*-detail.ppm' -or
                $_.Name -like 'stage-paper-*-detail.ppm' -or
                $_.Name -like 'home-return-focus-*-detail.ppm' -or
                $_.Name -like 'home-exit-*-detail.ppm' -or
                $_.Name -like 'options-defaults-*-detail.ppm' -or
                $_.Name -like 'display-trial-*-detail.ppm' -or
                $_.Name -like 'paused-options-*-detail.ppm' -or
                $_.Name -eq 'controls-menu-pad-sticks-1280x720.ppm' } | Sort-Object Name) {
            $bytes = [IO.File]::ReadAllBytes($file.FullName)
            $header = [Text.Encoding]::ASCII.GetString($bytes, 0, [Math]::Min(64, $bytes.Length))
            if ($header -notmatch '^P6\n(\d+) (\d+)\n255\n') { throw "Unsupported capture header: $($file.FullName)" }
            $width = [int] $Matches[1]
            $height = [int] $Matches[2]
            $source = $Matches[0].Length
            $bitmap = [Drawing.Bitmap]::new($width, $height, [Drawing.Imaging.PixelFormat]::Format24bppRgb)
            $images.Add([pscustomobject]@{ name = $file.BaseName; bitmap = $bitmap })
            $data = $bitmap.LockBits([Drawing.Rectangle]::new(0, 0, $width, $height),
                [Drawing.Imaging.ImageLockMode]::WriteOnly, [Drawing.Imaging.PixelFormat]::Format24bppRgb)
            try {
                $pixels = [byte[]]::new($data.Stride * $height)
                for ($y = 0; $y -lt $height; ++$y) {
                    for ($x = 0; $x -lt $width; ++$x) {
                        $destination = $y * $data.Stride + $x * 3
                        $pixels[$destination] = $bytes[$source + 2]
                        $pixels[$destination + 1] = $bytes[$source + 1]
                        $pixels[$destination + 2] = $bytes[$source]
                        $source += 3
                    }
                }
                [Runtime.InteropServices.Marshal]::Copy($pixels, 0, $data.Scan0, $pixels.Length)
            }
            finally { $bitmap.UnlockBits($data) }
            $bitmap.Save([IO.Path]::ChangeExtension($file.FullName, '.png'), [Drawing.Imaging.ImageFormat]::Png)
            if ($file.BaseName -like 'monitor-*' -or $file.BaseName -like 'closing-story-*') {
                $monitorCellWidth = [Math]::Max($monitorCellWidth, $width + 16)
                $monitorCellHeight = [Math]::Max($monitorCellHeight, $height + 48)
            }
            else {
                $cellWidth = [Math]::Max($cellWidth, $width + 16)
                $cellHeight = [Math]::Max($cellHeight, $height + 48)
            }
        }
        if ($images.Count -eq 0) { throw 'No painter or monitor captures were produced.' }
        $painters = @($images | Where-Object { $_.name -notlike 'monitor-*' -and $_.name -notlike 'closing-story-*' })
        Write-ContactSheetPages -Images $painters -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-contact'
        # Keep the complete matrix above, and compact action review pages
        # with one native-size figure per action/frame/facing or 4P palette.
        # Selecting view 1 avoids repeating the four panes on this summary;
        # every player crop remains available beside it for teammate review.
        $actions = @($images | Where-Object { $_.name -match '^[14]p-action-.*-player1$' })
        Write-ContactSheetPages -Images $actions -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-actions-contact'
        $tools = @($images | Where-Object {
            $_.name -match '^1p-tool-.*-(idle|paid-shot|held-fire|release|recovery|travel|lift)-player1$' -or
            $_.name -match '^4p-tool-mixed-.*-(idle|held-fire|release)-player[1-4]$' -or
            $_.name -match '^4p-tool-roller-.*-(travel|lift)-player1$'
        })
        Write-ContactSheetPages -Images $tools -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-tools-contact'
        $weapons = @($images | Where-Object {
            $_.name -match '^1p-weapon-.*-player1$' -or
            $_.name -match '^4p-weapon-(mixed|controlled)-.*-player1$'
        })
        Write-ContactSheetPages -Images $weapons -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-weapons-contact'
        $quiet = @($images | Where-Object {
            $_.name -match '^[14]p-quiet-.*-detail-player1$' -or
            $_.name -match '^4p-quiet-blue_orange-(breath|blink)-1280-detail-player[2-4]$'
        })
        Write-ContactSheetPages -Images $quiet -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-quiet-contact'
        $drift = @($images | Where-Object { $_.name -match '^drift-[14]p-1280-.*-view1-detail$' })
        Write-ContactSheetPages -Images $drift -CellWidth 304 -CellHeight 160 `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-cloud-drift-contact'
        $markers = @($images | Where-Object { $_.name -match '^4p-aim-marker-.*-player[1-4]$' })
        Write-ContactSheetPages -Images $markers -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-aim-markers-contact'
        $blocked = @($images | Where-Object { $_.name -match '^blocked-.*-context-player[1-4]$' })
        Write-ContactSheetPages -Images $blocked -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-blocked-shots-contact'
        $identity = @($images | Where-Object {
            $_.name -match '^[24]p-identity-.*-context-player[1-4]$' -or
            $_.name -match '^4p-identity-glyph-.*-player[1-4]$'
        })
        Write-ContactSheetPages -Images $identity -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-player-identity-contact'
        $identityBackground = @($images | Where-Object { $_.name -like 'identity-background-*-badges' })
        if ($identityBackground.Count -gt 0) {
            # The four paper-colour crops keep native size and a compact page,
            # independent of larger world/context cells in the full matrix.
            $identityBackgroundWidth = [int](($identityBackground | ForEach-Object { $_.bitmap.Width } |
                Measure-Object -Maximum).Maximum) + 16
            $identityBackgroundHeight = [int](($identityBackground | ForEach-Object { $_.bitmap.Height } |
                Measure-Object -Maximum).Maximum) + 48
            Write-ContactSheetPages -Images $identityBackground -CellWidth $identityBackgroundWidth `
                -CellHeight $identityBackgroundHeight -CaptureDirectory $CaptureDirectory `
                -Stem 'painter-identity-background-contact'
        }
        $monitors = @($images | Where-Object { $_.name -like 'monitor-*' })
        Write-ContactSheetPages -Images $monitors -CellWidth $monitorCellWidth -CellHeight $monitorCellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-monitor-contact'
        $countdowns = @($images | Where-Object { $_.name -like 'countdown-*' })
        Write-ContactSheetPages -Images $countdowns -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-countdown-contact'
        $closing = @($images | Where-Object { $_.name -like 'closing-*' -and $_.name -notlike 'closing-story-*' })
        Write-ContactSheetPages -Images $closing -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-closing-contact'
        $closingStories = @($images | Where-Object { $_.name -like 'closing-story-*' })
        Write-ContactSheetPages -Images $closingStories -CellWidth $monitorCellWidth -CellHeight $monitorCellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-closing-story-contact'
        $supply = @($images | Where-Object { $_.name -like 'supply-*' })
        Write-ContactSheetPages -Images $supply -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-supply-contact'
        $mainMenu = @($images | Where-Object { $_.name -like 'main-menu-*-1280x720-detail' })
        Write-ContactSheetPages -Images $mainMenu -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-main-menu-contact'
        $weaponCatalog = @($images | Where-Object { $_.name -like 'weapon-catalog-*-1280x720-detail' })
        Write-ContactSheetPages -Images $weaponCatalog -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-weapon-catalog-contact'
        $menuText = @($images | Where-Object { $_.name -like 'menu-text-*-1280x720-detail' })
        Write-ContactSheetPages -Images $menuText -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-menu-text-contact'
        $percentages = @($images | Where-Object { $_.name -like 'percentage-*' })
        Write-ContactSheetPages -Images $percentages -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-percentage-contact'
        $feedbackColours = @($images | Where-Object { $_.name -like 'feedback-colour-*' })
        Write-ContactSheetPages -Images $feedbackColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-feedback-colour-contact'
        $combatColours = @($images | Where-Object { $_.name -like 'combat-colour-*' })
        Write-ContactSheetPages -Images $combatColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-combat-colour-contact'
        $blockedColours = @($images | Where-Object { $_.name -like 'blocked-colour-*' })
        Write-ContactSheetPages -Images $blockedColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-blocked-colour-contact'
        $programColours = @($images | Where-Object { $_.name -like 'program-colour-*' })
        Write-ContactSheetPages -Images $programColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-program-colour-contact'
        $actingColours = @($images | Where-Object { $_.name -like 'acting-colour-*' })
        Write-ContactSheetPages -Images $actingColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-acting-colour-contact'
        $limbColours = @($images | Where-Object { $_.name -like 'limb-colour-*' })
        Write-ContactSheetPages -Images $limbColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-limb-colour-contact'
        $crackColours = @($images | Where-Object { $_.name -like 'crack-colour-*' })
        $quietFaceColours = @($images | Where-Object { $_.name -like 'quiet-face-colour-*' })
        Write-ContactSheetPages -Images $quietFaceColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-quiet-face-colour-contact'
        Write-ContactSheetPages -Images $crackColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-crack-colour-contact'
        $hudClosingTextColours = @($images | Where-Object { $_.name -like 'hud-closing-text-colour-*' })
        Write-ContactSheetPages -Images $hudClosingTextColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-hud-closing-text-colour-contact'
        $hudTimerShadowColours = @($images | Where-Object { $_.name -like 'hud-timer-shadow-colour-*' })
        Write-ContactSheetPages -Images $hudTimerShadowColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-hud-timer-shadow-colour-contact'
        $hudTimerColours = @($images | Where-Object { $_.name -like 'hud-timer-colour-*' })
        Write-ContactSheetPages -Images $hudTimerColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-hud-timer-colour-contact'
        $closingMaterialColours = @($images | Where-Object { $_.name -like 'hud-closing-materials-*' })
        Write-ContactSheetPages -Images $closingMaterialColours -CellWidth 218 -CellHeight 148 `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-closing-mat'
        $closingShadeColours = @($images | Where-Object { $_.name -like 'closing-shade-colour-*' })
        Write-ContactSheetPages -Images $closingShadeColours -CellWidth 218 -CellHeight 148 `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-closing-shade'
        $hudPaperColours = @($images | Where-Object { $_.name -like 'hud-paper-colour-*' })
        Write-ContactSheetPages -Images $hudPaperColours -CellWidth 507 -CellHeight 228 `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-hud-paper-colours'
        $stagePreviewBackgroundColours = @($images | Where-Object { $_.name -like 'stage-preview-background-colour-*' })
        Write-ContactSheetPages -Images $stagePreviewBackgroundColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-stage-preview-background-colour-contact'
        $hudLabelColours = @($images | Where-Object { $_.name -like 'hud-label-colour-*' })
        Write-ContactSheetPages -Images $hudLabelColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-hud-label-colour-contact'
        $hudTrackColours = @($images | Where-Object { $_.name -like 'hud-track-colour-*' })
        Write-ContactSheetPages -Images $hudTrackColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-hud-track-colour-contact'
        $hudHealthColours = @($images | Where-Object { $_.name -like 'hud-health-colour-*' })
        Write-ContactSheetPages -Images $hudHealthColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-hud-health-colour-contact'
        $scribbleColours = @($images | Where-Object { $_.name -like 'scribble-colour-*' })
        Write-ContactSheetPages -Images $scribbleColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-scribble-colour-contact'
        $silhouetteBodyColours = @($images | Where-Object { $_.name -like 'silhouette-body-colour-*' })
        Write-ContactSheetPages -Images $silhouetteBodyColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-silhouette-body-colour-contact'
        $glassColours = @($images | Where-Object { $_.name -like 'glass-colour-*' })
        Write-ContactSheetPages -Images $glassColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-glass-colour-contact'
        $programLevels = @($images | Where-Object { $_.name -like 'program-level-*' })
        Write-ContactSheetPages -Images $programLevels -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-program-level-contact'
        $powerLightColours = @($images | Where-Object { $_.name -like 'power-light-colour-*' })
        Write-ContactSheetPages -Images $powerLightColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-power-light-colour-contact'
        $desktopColours = @($images | Where-Object { $_.name -like 'desktop-colour-*' })
        Write-ContactSheetPages -Images $desktopColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-desktop-colour-contact'
        $cursorMaterialColours = @($images | Where-Object { $_.name -like 'cursor-material-*' })
        Write-ContactSheetPages -Images $cursorMaterialColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-cursor-material-colour-contact'
        $revealColours = @($images | Where-Object { $_.name -like 'reveal-colour-*' })
        Write-ContactSheetPages -Images $revealColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-reveal-colour-contact'
        $silhouetteColours = @($images | Where-Object { $_.name -like 'silhouette-colour-*' })
        Write-ContactSheetPages -Images $silhouetteColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-silhouette-colour-contact'
        $cursorColours = @($images | Where-Object { $_.name -like 'cursor-colour-*' })
        Write-ContactSheetPages -Images $cursorColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-cursor-colour-contact'
        $respawnPenColours = @($images | Where-Object { $_.name -like 'respawn-pen-*' })
        Write-ContactSheetPages -Images $respawnPenColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-respawn-pen-colour-contact'
        $idleDripColours = @($images | Where-Object { $_.name -like 'idle-drip-colour-*' })
        Write-ContactSheetPages -Images $idleDripColours -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-idle-drip-colour-contact'
        $exposedAlpha = @($images | Where-Object { $_.name -like 'alpha-*' })
        Write-ContactSheetPages -Images $exposedAlpha -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-exposed-alpha-contact'
        $duration = @($images | Where-Object { $_.name -like 'duration-*' })
        Write-ContactSheetPages -Images $duration -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-duration-contact'
        $resolution = @($images | Where-Object { $_.name -like 'resolution-*' })
        Write-ContactSheetPages -Images $resolution -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-resolution-contact'
        $endContinuation = @($images | Where-Object { $_.name -like 'end-continuation-*' })
        Write-ContactSheetPages -Images $endContinuation -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-end-continuation-contact'
        $resultsText = @($images | Where-Object { $_.name -like 'results-text-*' })
        Write-ContactSheetPages -Images $resultsText -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-results-text-contact'
        $sessionResult = @($images | Where-Object { $_.name -like 'session-result-*' })
        Write-ContactSheetPages -Images $sessionResult -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-session-result-contact'
        $seriesScore = @($images | Where-Object { $_.name -like 'series-score-*' })
        Write-ContactSheetPages -Images $seriesScore -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-series-score-contact'
        $roundHeading = @($images | Where-Object { $_.name -like 'round-heading-*' })
        Write-ContactSheetPages -Images $roundHeading -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-round-heading-contact'
        $competitiveWinner = @($images | Where-Object { $_.name -like 'competitive-winner-*' })
        Write-ContactSheetPages -Images $competitiveWinner -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-competitive-winner-contact'
        $contributionTeam = @($images | Where-Object { $_.name -like 'contribution-team-*' })
        Write-ContactSheetPages -Images $contributionTeam -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-contribution-team-contact'
        $resultPercentage = @($images | Where-Object { $_.name -like 'result-percentage-*' })
        Write-ContactSheetPages -Images $resultPercentage -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-result-percentage-contact'
        $menuMotion = @($images | Where-Object { $_.name -like 'menu-motion-*-1280x720-detail' })
        Write-ContactSheetPages -Images $menuMotion -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-menu-motion-contact'
        $setupMotion = @($images | Where-Object { $_.name -like 'setup-motion-*-1280x720-detail' })
        Write-ContactSheetPages -Images $setupMotion -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-setup-motion-contact'
        $joinStageMotion = @($images | Where-Object { $_.name -like 'join-stage-motion-*-1280x720-detail' })
        Write-ContactSheetPages -Images $joinStageMotion -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-join-stage-motion-contact'
        $setupPaper = @($images | Where-Object { $_.name -like 'setup-paper-*-1280x720-detail' })
        Write-ContactSheetPages -Images $setupPaper -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-setup-paper-contact'
        $joinPaper = @($images | Where-Object { $_.name -like 'join-paper-*-1280x720-detail' })
        Write-ContactSheetPages -Images $joinPaper -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-join-paper-contact'
        $teamPaper = @($images | Where-Object { $_.name -like 'team-paper-*-1280x720-detail' })
        Write-ContactSheetPages -Images $teamPaper -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-team-paper-contact'
        $weaponPaper = @($images | Where-Object { $_.name -like 'weapon-paper-*-1280x720-detail' })
        Write-ContactSheetPages -Images $weaponPaper -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-weapon-paper-contact'
        $stagePaper = @($images | Where-Object { $_.name -like 'stage-paper-*-1280x720-detail' })
        Write-ContactSheetPages -Images $stagePaper -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-stage-paper-contact'
        $homeReturnFocus = @($images | Where-Object { $_.name -like 'home-return-focus-*-1280x720-detail' })
        Write-ContactSheetPages -Images $homeReturnFocus -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-home-return-focus-contact'
        $homeExit = @($images | Where-Object { $_.name -like 'home-exit-*-1280x720-detail' })
        Write-ContactSheetPages -Images $homeExit -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-home-exit-contact'
        $optionsDefaults = @($images | Where-Object { $_.name -like 'options-defaults-*-1280x720-detail' })
        Write-ContactSheetPages -Images $optionsDefaults -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-options-defaults-contact'
        $pausedOptions = @($images | Where-Object { $_.name -like 'paused-options-*-1280x720-detail' })
        Write-ContactSheetPages -Images $pausedOptions -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-paused-options-contact'
        $displayTrial = @($images | Where-Object { $_.name -like 'display-trial-*-1280x720-detail' })
        Write-ContactSheetPages -Images $displayTrial -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-display-trial-contact'
        $controlsMenu = @($images | Where-Object { $_.name -like 'controls-menu-*-1280x720-detail' })
        Write-ContactSheetPages -Images $controlsMenu -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-controls-menu-contact'
        $stickSettings = @($images | Where-Object { $_.name -like 'stick-settings-*-1280x720-detail' })
        Write-ContactSheetPages -Images $stickSettings -CellWidth $cellWidth -CellHeight $cellHeight `
            -CaptureDirectory $CaptureDirectory -Stem 'painter-stick-settings-contact'
    }
    finally {
        foreach ($item in $images) { $item.bitmap.Dispose() }
    }
}
