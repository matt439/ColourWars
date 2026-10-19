[CmdletBinding()]
param(
    [ValidateSet('x64-debug', 'x64-release')]
    [string] $Preset = 'x64-debug',
    [switch] $SkipBuild,
    [switch] $IdentityOnly,
    [switch] $QuietOnly,
    [switch] $DriftOnly,
    [switch] $MiniatureTerrainOnly,
    [switch] $LandmarkOnly,
    [switch] $ParallaxOnly,
    [switch] $TuneColoursOnly,
    [switch] $FeedbackTuneColoursOnly,
    [switch] $ExposedColourAlphaOnly,
    [switch] $DurationTextOnly,
    [switch] $ResolutionTextOnly,
    [switch] $EndContinuationOnly,
    [switch] $ResultsTextOnly,
    [switch] $MonitorOnly,
    [switch] $CountdownOnly,
    [switch] $ClosingOnly,
    [switch] $SupplyOnly,
    [switch] $HudOnly,
    [switch] $ResumeOnly,
    [switch] $EndMenuOnly,
    [switch] $PauseMenuOnly,
    [switch] $ResultsMenuOnly,
    [switch] $MainMenuOnly,
    [switch] $WeaponCatalogOnly,
    [switch] $MenuTextOnly,
    [switch] $MenuMotionOnly,
    [switch] $SetupMotionOnly,
    [switch] $JoinStageMotionOnly,
    [switch] $SetupPaperOnly,
    [switch] $JoinPaperOnly,
    [switch] $TeamPaperOnly,
    [switch] $WeaponPaperOnly,
    [switch] $StagePaperOnly,
    [switch] $HomeReturnFocusOnly,
    [switch] $HomeExitOnly,
    [switch] $OptionsDefaultsOnly,
    [switch] $DisplayTrialOnly,
    [switch] $PausedOptionsOnly,
    [switch] $SettingsMenuOnly,
    [switch] $ControlsMenuOnly,
    [switch] $PercentageTextOnly,
    [switch] $StickSettingsOnly,
    [switch] $TeamSetupOnly,
    [switch] $RememberedSetupOnly,
    [switch] $ConnectionTextOnly,
    [switch] $StickReadoutsOnly,
    [switch] $TeamFocusOnly,
    [switch] $TeamTuneColoursOnly,
    [switch] $WeaponTuneColoursOnly,
    [switch] $MenuLabelsOnly,
    [switch] $StageSummaryOnly,
    [switch] $StageCountsOnly,
    [string] $OutputDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($TuneColoursOnly -and $Preset -ne 'x64-debug') {
    throw 'TuneColoursOnly requires x64-debug; Release developer-tool exclusion is checked by CPU acceptance.'
}
if ($TeamTuneColoursOnly -and $Preset -ne 'x64-debug') {
    throw 'TeamTuneColoursOnly requires x64-debug; Release developer-tool exclusion is checked by CPU acceptance.'
}
if ($WeaponTuneColoursOnly -and $Preset -ne 'x64-debug') {
    throw 'WeaponTuneColoursOnly requires x64-debug; Release developer-tool exclusion is checked by CPU acceptance.'
}
if (([int]$ResultsTextOnly.IsPresent + [int]$EndContinuationOnly.IsPresent + [int]$ResolutionTextOnly.IsPresent + [int]$DurationTextOnly.IsPresent + [int]$ExposedColourAlphaOnly.IsPresent + [int]$FeedbackTuneColoursOnly.IsPresent + [int]$PercentageTextOnly.IsPresent + [int]$ParallaxOnly.IsPresent + [int]$IdentityOnly.IsPresent + [int]$QuietOnly.IsPresent + [int]$DriftOnly.IsPresent + [int]$MiniatureTerrainOnly.IsPresent + [int]$LandmarkOnly.IsPresent + [int]$TuneColoursOnly.IsPresent + [int]$MonitorOnly.IsPresent + [int]$CountdownOnly.IsPresent + [int]$ClosingOnly.IsPresent + [int]$SupplyOnly.IsPresent + [int]$HudOnly.IsPresent + [int]$ResumeOnly.IsPresent + [int]$EndMenuOnly.IsPresent + [int]$PauseMenuOnly.IsPresent + [int]$ResultsMenuOnly.IsPresent + [int]$MainMenuOnly.IsPresent + [int]$WeaponCatalogOnly.IsPresent + [int]$MenuTextOnly.IsPresent + [int]$MenuMotionOnly.IsPresent + [int]$SetupMotionOnly.IsPresent + [int]$JoinStageMotionOnly.IsPresent + [int]$SetupPaperOnly.IsPresent + [int]$JoinPaperOnly.IsPresent + [int]$TeamPaperOnly.IsPresent + [int]$WeaponPaperOnly.IsPresent + [int]$StagePaperOnly.IsPresent + [int]$HomeReturnFocusOnly.IsPresent + [int]$HomeExitOnly.IsPresent + [int]$OptionsDefaultsOnly.IsPresent + [int]$DisplayTrialOnly.IsPresent + [int]$PausedOptionsOnly.IsPresent + [int]$SettingsMenuOnly.IsPresent + [int]$ControlsMenuOnly.IsPresent + [int]$StickSettingsOnly.IsPresent + [int]$TeamSetupOnly.IsPresent + [int]$RememberedSetupOnly.IsPresent + [int]$ConnectionTextOnly.IsPresent + [int]$StickReadoutsOnly.IsPresent + [int]$TeamFocusOnly.IsPresent + [int]$TeamTuneColoursOnly.IsPresent + [int]$WeaponTuneColoursOnly.IsPresent + [int]$MenuLabelsOnly.IsPresent + [int]$StageSummaryOnly.IsPresent + [int]$StageCountsOnly.IsPresent) -gt 1) {
    throw 'Choose only one focused painter matrix.'
}

# Windows PowerShell 5.1 does not set PSScriptRoot during parameter defaults.
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $PSScriptRoot '../out/validation/painter'
}

function Invoke-Logged {
    param([string] $Command, [string[]] $Arguments, [string] $LogPath)
    $ErrorActionPreference = 'Continue'
    $output = & $Command @Arguments 2>&1
    $code = $LASTEXITCODE
    $output | Out-File -LiteralPath $LogPath -Encoding utf8
    if ($code -ne 0) {
        $output | ForEach-Object { Write-Host $_ }
        throw "$Command exited with $code. See $LogPath"
    }
}

function Quote-Path {
    param([string] $Path)
    if ($Path.Contains('"') -or $Path.Contains("`r") -or $Path.Contains("`n")) {
        throw 'A path contains an unsupported quote or newline.'
    }
    return '"' + [IO.Path]::GetFullPath($Path) + '"'
}

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
                $_.Name -like 'drift-*-detail.ppm' -or $_.Name -like 'miniature-terrain-*-detail.ppm' -or
                $_.Name -like 'tune-colours-*-detail.ppm' -or $_.Name -like 'parallax-*.ppm' -or
                $_.Name -like 'countdown-*-detail.ppm' -or $_.Name -like 'closing-*.ppm' -or
                $_.Name -like 'supply-*-detail.ppm' -or $_.Name -like 'hud-*-detail.ppm' -or
                $_.Name -like 'resume-*-detail.ppm' -or $_.Name -like 'end-menu-*-detail.ppm' -or
                $_.Name -like 'pause-menu-*-detail.ppm' -or $_.Name -like 'results-menu-*-detail.ppm' -or
                $_.Name -like 'main-menu-*-detail.ppm' -or $_.Name -like 'settings-menu-*-detail.ppm' -or
                $_.Name -like 'controls-menu-*-detail.ppm' -or $_.Name -like 'stick-settings-*-detail.ppm' -or
                $_.Name -like 'percentage-*.ppm' -or $_.Name -like 'feedback-colour-*.ppm' -or $_.Name -like 'alpha-*.ppm' -or $_.Name -like 'duration-*.ppm' -or $_.Name -like 'resolution-*.ppm' -or $_.Name -like 'end-continuation-*.ppm' -or $_.Name -like 'results-text-*.ppm' -or
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

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$build = Join-Path $repo "out/build/$Preset"
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
$run = Join-Path (Join-Path $outputRoot $Preset) ([Guid]::NewGuid().ToString('N'))
$captures = Join-Path $run 'captures'
$working = Join-Path $run 'working-directory'
$profile = Join-Path $run 'profile'
$null = New-Item -ItemType Directory -Path $run, $captures, $working, $profile -Force
$saveDirectory = Join-Path $profile 'ColourWars'
$null = New-Item -ItemType Directory -Path $saveDirectory -Force
$saveFixture = Join-Path $saveDirectory 'save_data.json'
[IO.File]::WriteAllText($saveFixture, '{"aim_marker":false}', [Text.UTF8Encoding]::new($false))
$saveFixtureHash = (Get-FileHash -LiteralPath $saveFixture).Hash

foreach ($command in 'cmake', 'cl', 'git') {
    if (-not (Get-Command $command -ErrorAction SilentlyContinue)) {
        throw "Run this script from an x64 Visual Studio Developer shell ($command is missing)."
    }
}
if (-not $env:INCLUDE -or -not $env:LIB -or
    ($env:VSCMD_ARG_TGT_ARCH -and $env:VSCMD_ARG_TGT_ARCH -ne 'x64')) {
    throw 'This probe requires the x64 Visual Studio Developer environment.'
}

Push-Location -LiteralPath $repo
$previousPath = $env:PATH
$previousLocalAppData = $env:LOCALAPPDATA
$process = $null
$failure = $null
$result = [ordered]@{
    outcome = 'failed'
    scope = if ($IdentityOnly) { 'player-identity-only' } elseif ($QuietOnly) { 'quiet-only' } elseif ($MonitorOnly) { 'monitor-only' } elseif ($CountdownOnly) { 'countdown-only' } elseif ($ClosingOnly) { 'closing-only' } elseif ($SupplyOnly) { 'supply-only' } elseif ($HudOnly) { 'gameplay-hud-only' } elseif ($ResumeOnly) { 'resume-only' } elseif ($EndMenuOnly) { 'end-menu-only' } elseif ($PauseMenuOnly) { 'pause-menu-only' } elseif ($ResultsMenuOnly) { 'results-menu-only' } elseif ($MainMenuOnly) { 'main-menu-only' } elseif ($WeaponCatalogOnly) { 'weapon-catalog-only' } elseif ($MenuTextOnly) { 'menu-text-only' } elseif ($MenuMotionOnly) { 'menu-motion-only' } elseif ($SettingsMenuOnly) { 'settings-menu-only' } elseif ($ControlsMenuOnly) { 'controls-menu-only' } elseif ($StickSettingsOnly) { 'stick-settings-only' } elseif ($TeamSetupOnly) { 'team-setup-only' } else { 'full-painter-matrix' }
    preset = $Preset
    game_revision = (& git rev-parse HEAD)
    engine_revision = (& git -C external/labrador rev-parse HEAD)
    worktree_status = @(& git status --short)
    stage = 'movement_course'
    player_counts = @(1, 4)
    palettes = @('blue_orange', 'pink_green', 'blue_yellow')
    weapons = @('sprayer', 'sniper', 'mister', 'bucket', 'roller')
    actions = @('takeoff', 'land', 'fire', 'hurt', 'elimination')
    input_slots = @(3, 1, 2, 0)
    inputs = 'Scripted pad-slot inputs at the real 60 Hz simulation step; no physical-device claim.'
    audio = 'Muted; this check establishes no listening acceptance.'
    hud_checks = 'Real InterfaceGameplay native full/half/empty health and paint, independently supplied quantities and negative/oversized clamps. Meter differences stay inside exact tracks; the half raster agrees with full/empty on either side of its midpoint and dark keylines remain outside the track. Dark text/pale paper raster contrast, sparse keyboard/pad-slot labels, 1P/2P/3P/4P720 and all three 1P/4P palettes; representative 4P1080/QHD, 4:3, portrait and ultrawide. Isolated owner-pane artwork remains clear of pane edges, including the unused three-player quadrant. Ordinary clock and static information match under Reduce effects, repeated drawing preserves pixels/draw RNG. Real 4P match frozen pixels, public state, Debug replay fingerprint and exact 720p/1080p resize round-trip. Existing final-ten clock boundary matrix rerun; reveal cards/story timing are outside the focused HUD run. Native empty/half/full detail PNGs and full-context PPMs retained. Raster checks establish no human readability, physical-device or listening acceptance.'
    contact_checks = 'Real resolved 4P dry steps/reversals; controlled step/skid initial/middle/expired rasters in all palettes at 720p/1080p/QHD. Dry/painted and left/right raster differences, enabled/disabled comparisons, frozen direction/tint/timers and culling bounds.'
    weapon_checks = 'Actual Weapon firing workflow with locally retained projectiles: 1P all five tools and 4P mixed tools in all three palettes, four aim quadrants, then live aim/flip changes while each paid-shot pulse remains active. Roller has no muzzle pulse. Controlled 4P muzzle/empty/recovered event triggers at initial/middle/expiry, enabled/disabled at the same simulation state, at 720p/1080p/QHD. Frozen redraw and observed feedback timers, current-nozzle attachment and raster culling bounds. Empty/recovery gameplay producers are covered by separate CPU acceptance checks. Mechanism raster comparisons suppress the new pulses.'
    quiet_checks = 'Actual builder/Scene 1P and 4P sprayers in all three palettes at 720p: settled breath crest, bead growth/stretch/release/fall, blink, movement interruption and left lean. Sniper residual beads and bucket rim runoff add growth/stretch/fall, same-bead continuous four-quadrant aim with authored lip/stripe anchors, both body facings, per-tool enabled suppression, and same-state reduced-effects hiding/restoration for attached and falling liquid in every 1P/4P720 palette. Representative attached-bead extents at 1080p/QHD. Mature bead raster touches the actual rendered tool silhouette within one pixel. Same-state enabled/disabled breathing/shift rasters and controlled blink-only comparison at playing size, distinct roster phases, frozen cosmetic timers/drops, separate Scene drip raster bounds and combined nozzle/rim-context crops. All runs use muted play settings; no listening acceptance.'
    aim_marker_checks = 'Isolated save loads aim_marker=false and remains byte-identical. Existing matrices disable markers. Actual four aimed weapons enter ready/recovering/empty through paid shots and strict cooldown, in all three palettes and four aim quadrants at 4P720; representative 4P1080/QHD. Marker fixture uses a silent bank while paying many shots without wall-clock audio updates. Same-state Level on/off raster changes lie only inside each owning pane marker bounds; all three shapes have distinct isolated native rasters. A mixed-angle/readiness roster checks independent owner panes. Repeated frozen draws, suspension, simulation state and replay fingerprint are checked. CPU acceptance separately verifies affordability boundaries, all lifecycle gates and gameplay equivalence.'
    blocked_shot_checks = 'Detached contacts in actual four-pane Level drawing; controlled opaque-cover/nozzle overlap and four outward normals at native 4P720 in every palette, plus representative 1080p/QHD. Initial/middle/expired, enabled/disabled pixels, projected cue and aggregate bounds, rebound/fade raster changes, repeated frozen redraws, cosmetic timers, simulation state and Debug replay fingerprint. CPU acceptance separately checks actual launch/flight contact classification and lifecycle.'
    player_identity_checks = 'Real same-team builder rosters with identical tools in all three 4P720 palettes; overlapping figures, actual scripted teammate crossing and sparse/reordered input slots. Quiet, requested emphasis and reduce-effects captures, representative 1080p/QHD extents, pause, dead/respawn and story suppression. Same-state zero-world-size comparison preserves pane HUD badges and constrains every changed pixel to the owning world badge bounds. Distinct shared P1-P4 native glyph rasters, native context and badge crops, frozen cosmetic state, repeated redraw and Debug replay fingerprint. CPU acceptance separately verifies lifecycle and gameplay equivalence; human reading acceptance remains separate.'
    monitor_checks = 'Bounded monitor matrix: six authored body shades, adult with mug/ring/crack/notes and child with stickers; manual picture-edge note overlap at 480p/768p/720p/1080p/QHD and one bounded glass push. All 26 Muted Day atlas swatches retain authored order, square cells and visible gutters, followed by one contained empty slot; saving preserves every existing cell. Both picker colours and the complete panel stay inside the program at narrow aspects. Foreground notes survive picture and preview, picture/palette interiors stay unchanged, saving fills the empty slot and closing replaces the picture with the desktop. Isolated adult/child/mug seated, sigh, stand and partial-leave foreground rasters at all three native sizes; full departure equals blank, rim/outline/shading differ from plain fill, offset-window translation is exact. Native cursor hotspot stays fixed, detached initial/mid-click changes stay local and leave every resting arrow pixel unchanged, negative/expired ages and reduced effects equal rest. Native foreground and pointer detail crops supplement actual sparse-slot 2P/4P shipped full-drift story frames. A second identical story uses a private stage copy with only decorative cloud drift removed; restoring those fields must reproduce the original JSON exactly. This isolates unchanged before/at/after winning-save and no-paint-close reduced-effects assertions from legitimate cloud-to-anchor movement, retaining response containment, frozen state/RNG and Debug fingerprints with compact cursor/card crops and CSV. Solo practice has no monitor. Frozen rasters, persona/pose/public simulation state, draw RNG and Debug replay fingerprint remain unchanged. Captures establish no listening or physical-device acceptance.'
    countdown_checks = 'Isolated 0-9/GO glyphs differ at native 720p, with representative 1080p/QHD. Animated and reduced/negative/expired ages stay inside the supplied number box; reduced effects retain the settled readable signal. Actual keyboard solo and mixed keyboard/sparse-pad 2P/4P full/quick/off openings cover native 720p/1080p/QHD, reminder-card bounds, frozen initial/beat/settled/pre-GO/GO/tail/expiry captures, paused and nonpositive-time draws, exact GO control-release/card-clear boundary and resize round-trip. All four authored stage names appear on fitted shared paper headers; a three-player roster and 4:3, portrait and ultrawide resize round-trips cover stage/title bounds and separation from countdown/device cards. Header information remains static with Reduce effects, hides during early story opening and at GO, and survives skips from both opening and monitor countdown. Same-state GO art on/off differences remain inside the former card, persist through its tail and disappear after one second. Frozen rasters, public simulation/cosmetic state, draw RNG and Debug replay fingerprint remain unchanged. Native stage-header, card and glyph crops accompany full-context PPMs. This is muted hardware rendering, not human readability acceptance.'
    closing_checks = 'Actual HUD last-ten-second clock at 1P/2P/4P720 and 4P1080/QHD: initial/early beat, settled, pre-closing and zero states, local normal/reduced-effects raster comparisons and frozen redraw. Same-state paper suppression checks the entire settled card gate in every pane; short ten/one-second clocks show neither card nor ticks until live play. Native settled/click A/B coverage cards in all three palettes at 720p, representative 1080p/QHD, negative/expired/reduced-effects equivalence, supplied bounds and draw RNG. Actual sparse-pad 2P A/B winners, 4P no-paint draw and solo finish, before/at/after click card gating, repeated frozen pixels, public state and Debug replay fingerprint. Native card/clock detail PNGs and full story contexts are preserved. Muted rendering establishes no listening, physical-device or human readability acceptance.'
    supply_checks = 'Sprayer and mister full/half/empty reservoirs at actual builder-owned camera scales and layouts: all three 1P/4P720 palettes, four aim quadrants, both facing inputs, all four poses, representative 4P1080/QHD and motion-disabled tools. Independent authored full/half-layer composition must match actual Weapon pixels exactly; half pixels stay within full supply and filling introduces no new silhouette pixels or changes to permanent hardware/nozzle paint. Negative/oversized ammo clamps, unchanged reduced-effects information, frozen pixels/weapon state/draw RNG, paid full-shot depletion, natural regeneration/reset and unchanged match observation/Debug fingerprint. Menu previews retain full supply and switching tools clears it. Native detail PNGs and full-context PPMs are preserved; human readability and physical-device acceptance remain separate.'
    checks = 'Real builder/Scene rosters, both movement facings and four aim quadrants at 720p; representative facings also at 1080p/QHD; three 4P palettes; four same-team figures in both facings. Five body action clips on actual events, both frames and distinct pinned body rasters. All five tools at 1P720 in both facings: paid fire/hold/release/recovery, hose/bucket travel and roller travel/lift. Three 4P palettes: right-facing mixed aimed tools and left-facing roller, plus 1080p/QHD extent captures. Sampled non-idle pinned tools differ from idle at gameplay zoom, including released roller travel/lift. Frozen draws byte-equal, public simulation/action/tool motion state and Debug replay fingerprint unchanged. Isolated player/tool raster output stays inside projected cull bounds. Death, outline, fill, respawn, tool-free squash and whole-dead story poses; early elimination whole-dead raster equals later idle whole-dead raster. Combat on/off comparisons at identical simulation state: hurt and elimination across all three 4P palettes at 720p; controlled 4P hurt initial/recovery/expiry and elimination initial/middle/late/expiry at 720p/1080p/QHD. Combat timers/death snapshot stay unchanged during drawing; whole-dead story figures suppress the burst.'
    profile = $profile
    diagnostic = ''
}
if ($ResultsTextOnly) {
    $result.scope = 'results-text-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @(2)
    $result.palettes = @()
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('three-independent-plain-results-labels', 'actual-results-reveal-and-proceed', 'captured-copy-and-fonts', 'reopen-and-reversal', 'series-heading-control', 'frozen-session-setup-save-and-rng', 'resize')
    $result.input_slots = @(4)
    $result.inputs = 'Actual ResultsMenuInitial with the normal GamepadReader and public local keyboard events through StateContext; local proceed callbacks do not launch a match.'
    $result.audio = 'Focused Results page fixture; this renderer probe establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated three-label Results leaf. Independent literal copy and raw-font references compare the actual single-match heading, proceed line and contribution heading with their surrounding paper; generated series headings retain their existing behavior. Captured copy/fonts, reopening and reversal, actual reveal/proceed transitions, frozen Session/setup/private Save/RNG and zero-update resize checks are bounded separately. Exactly 43 StateContext updates, 52 readbacks, 51 CSV rows (seventeen observations across three roles), six page openings, six proceed pops, three font negative controls, eight compact crops (1428000 pixels) and one successful private seed Save are checked with exact schema, case, metric and RGB-payload gates. Both presets run the same shared consumer scope without developer targets or commands. No broad menu matrix, match launch, foreground/global input, physical-device, listening, human readability or performance acceptance.'
}
elseif ($EndContinuationOnly) {
    $result.scope = 'end-continuation-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @(1, 2)
    $result.palettes = @()
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('four-literal-continuation-labels', 'actual-end-menu-keyboard-focus', 'captured-copy-and-fonts', 'reopen-and-reversal', 'typed-callbacks-and-deferred-music', 'frozen-session-setup-save-and-rng', 'resize')
    $result.input_slots = @(4)
    $result.inputs = 'Actual EndMenu with the normal GamepadReader and public local keyboard events through StateContext; typed callbacks terminate at a local dummy root without launching a match or leaving the fixture.'
    $result.audio = 'The fixture checks deferred music actions; this renderer probe establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated four-label continuation leaf. Independent literal copy and raw-font references compare the actual EndMenu primary label and complete primary ornament across single/series rematch, completed series, drawn round and next round. Captured copy/font assignment, reopening and reversal retain the existing geometry, persistent primary card and focus-arrow policy. Typed local callbacks prove restart/exit selection and deferred music actions without launching gameplay; frozen Session/setup/private Save/RNG and zero-update resize checks remain separate. Exactly 68 StateContext updates, 94 readbacks, 31 CSV rows, fourteen page openings, one font negative control, six 540x96 crops (311040 pixels), fourteen typed actions (thirteen restart and one exit), four deferred actions, four music starts and one successful private Play-settings seed Save are checked. Both presets run the same shared consumer scope without developer targets or commands. No broad menu matrix, match launch, foreground/global input, physical-device, listening, human readability or performance acceptance.'
}
elseif ($ResolutionTextOnly) {
    $result.scope = 'resolution-text-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @()
    $result.palettes = @()
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080', '1600x900', '1001x777')
    $result.actions = @('literal-resolution-rows', 'actual-options-keyboard-cycles', 'captured-copy-and-fonts', 'reopen-and-reversal', 'frozen-state-and-draw-rng', 'resize')
    $result.input_slots = @(4)
    $result.inputs = 'Actual Options page with the normal GamepadReader and public local keyboard events through StateContext; no scripted pad seam, foreground/global input or match launch.'
    $result.audio = 'Focused menu fixture; this renderer probe establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated resolution-template leaf. Independent literal copy and raw-font references compare the actual Options resolution row through normal keyboard changes, captured template/font assignment, reopening and reversal. Saved windowed presets and saved-fullscreen live client values retain their existing enablement and pending-toggle behavior; the application remains windowed, with no Apply or display trial. Exactly 43 StateContext updates, 76 readbacks, 25 CSV rows, seven page openings, one font negative control, six compact crops (1050840 pixels) and three successful private metadata seed Saves are checked with exact summary, case, metric and RGB-payload gates. Both presets run the same shared consumer scope without developer targets or commands. No broad menu matrix, display Apply, gameplay, foreground/global input, physical-device, listening, human readability or performance acceptance.'
}
elseif ($DurationTextOnly) {
    $result.scope = 'duration-text-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @()
    $result.palettes = @()
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('literal-duration-rows', 'compound-ornament', 'actual-mode-keyboard-cycles', 'captured-text-and-fonts', 'reopen-and-reversal', 'frozen-draw-state-and-rng')
    $result.input_slots = @(4)
    $result.inputs = 'Actual Mode page with the normal GamepadReader and public local keyboard events through StateContext; no scripted pad seam, foreground/global input or match launch.'
    $result.audio = 'Muted page fixture; this renderer probe establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated two-template duration leaf. Independent literal duration rows and the complete compound ornament are compared with actual Mode page drawing through keyboard cycling, explicit/default choices, fractional and scientific defaults, captured text/font replacement, reopening and reversal. Frozen redraw and RNG, one font negative control, byte-identical private preferences and zero-update 720/1080/720 resize checks are retained. Exactly 86 StateContext updates, 76 readbacks, 25 CSV rows, eight page openings, five 1168x80 crops (467200 pixels) and one successful private Play-settings seed Save per preset. No broad menu matrix, gameplay, foreground/global input, physical-device, listening, human readability or performance acceptance.'
    if ($Preset -eq 'x64-debug') {
        $result.checks += ' Debug separately exercises the real TuningTarget and PresentationTarget resource-only validation: two legacy stand-in acceptances, paired-content refusals, primed candidate refusals, inert failed applies, valid retries and consumed candidates. Six target validations and six private candidate writes add zero state updates, readbacks, Save calls or commands; actual content bytes and draw RNG remain unchanged.'
        $result['target_validation_scope'] = 'debug-resource-targets-only'
    }
    else {
        $result['target_validation_scope'] = 'not-run-release'
    }
}
elseif ($ExposedColourAlphaOnly) {
    $result.scope = 'exposed-colour-alpha-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @()
    $result.palettes = @('opaque-default', 'literal-rgba128', 'literal-alpha0-nonzero-rgb')
    $result.weapons = @()
    $result.resolutions = @('1280x720')
    $result.actions = @('thirteen-home-background-menu-sites', 'captured-retained-reopened-and-restored-copy', 'movement-feedback-alpha', 'debug-label-alpha', 'independent-straight-channel-references', 'frozen-draw-state-and-rng')
    $result.input_slots = @()
    $result.inputs = 'Normal GamepadReader backend with isolated native menu/drawable fixtures; no scripted pad seam, foreground/global input or active match route.'
    $result.audio = 'No gameplay audio events; this renderer probe establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated exposed-colour alpha leaf. Thirteen actual Home-background menu sites retain captured defaults/RGBA128/alpha0, adopt reopened values and restore defaults; literal channel references cover black/nonblack backgrounds. Home evidence is 235 readbacks, 52 page openings, 130 CSV rows, four 128x96 crops, eight controls and zero StateContext updates. Display confirmation uses the same 1280x720 windowed candidate with a private activation guard and rollback; no Keep action. Movement feedback and debug text compare independent raw-source/literal-channel references, opaque parity, straight-alpha/double-fade controls and retained captured paint; their evidence is 50 readbacks, 12 CSV rows, eight compact crops, ten negative controls and four opaque parity controls. Shared evidence totals 285 readbacks, 142 CSV rows, twelve crops, two successful private seed Saves and zero StateContext/gameplay updates. No broad painter matrix, gameplay, physical-device, listening, human readability or performance acceptance.'
    if ($Preset -eq 'x64-debug') {
        $result.actions += @('debug-console-alpha', 'validated-private-tune-reset')
        $result.checks += ' Debug additionally compares frozen/live console text, panel shadows and performance text against independent references through eight real private tune/reset commands: 104 readbacks, 48 CSV rows, four extra crops and zero Save write commands. Debug totals 389 readbacks, 190 CSV rows and sixteen crops.'
        $result['developer_command_scope'] = 'debug-validated-commands'
    }
    else {
        $result.checks += ' Release remains shared-consumer-only and emits no console command transcript, console summary, console CSV or console scratch tree.'
        $result['developer_command_scope'] = 'not-run-release-consumer-only'
    }
}
elseif ($FeedbackTuneColoursOnly) {
    $result.scope = 'feedback-tune-colours-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @()
    $result.palettes = @('opaque-default', 'literal-rgb', 'literal-rgba128', 'literal-alpha0-nonzero-rgb')
    $result.weapons = @('isolated-sprayer-feedback-only')
    $result.resolutions = @('1280x720')
    $result.actions = @('nine-isolated-event-and-readiness-cells', 'full-and-half-life', 'black-and-nonblack-backgrounds', 'independent-raw-source-premultiplied-channels', 'opaque-legacy-parity', 'double-fade-and-straight-alpha-negative-controls', 'reduce-effects-static-information', 'opaque-team-centres', 'frozen-timers-bounds-rng')
    $result.input_slots = @()
    $result.inputs = 'Normal GamepadReader backend; no reader sampling, keyboard events, scripted pads, StateContext updates or match objects. The nine-cell fixture directly calls existing WeaponFeedback consumers with frozen cue states and literal colours.'
    $result.audio = 'No gameplay audio events; this renderer probe establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Both presets render empty/recovered/Sprayer muzzle at full and half life plus all three readiness shapes over black and nonblack. Independent raw-source sprite geometry and explicit RGBA channels verify four literal palettes, opaque-default legacy parity, rejection of RGB double-fade and straight-alpha submissions, unchanged opaque team centres, Reduce Effects policy and frozen timers/bounds/draw RNG. Shared evidence is exactly zero StateContext updates,56 readbacks,eight CSV rows,eight360x280 crops and eight parity/negative controls. No gameplay, global input, physical-device, listening, human readability or performance acceptance.'
    if ($Preset -eq 'x64-debug') {
        $result.actions += @('fifteen-real-validated-named-rgb-rgba-reset-save-commands', 'two-token-private-save-and-rebase', 'unchanged-active-preferences-and-private-bytes')
        $result.inputs += ' Debug additionally uses a private DebugDirector and isolated source/staged/user/profile/TEMP/TMP paths; exactly two successful Save calls seed private preferences, with no controlled save failure.'
        $result.checks += ' Debug adds15 real validated named/RGB/RGBA/reset/save commands on the same preconstructed borrowed consumers,66 readbacks,20 CSV rows and zero extra crops. Exact two-token source/staged save and ledger rebase, unchanged tuning/actual content/private preferences and absence of queued match work are checked. Total Debug evidence is122 readbacks,28 CSV rows and eight crops.'
        $result['developer_command_scope'] = 'debug-validated-commands'
    }
    else {
        $result.inputs += ' Release runs consumer-only evidence and creates no developer director, command transcript or nested save.'
        $result.checks += ' Release evidence remains consumer-only:56 readbacks,eight CSV rows and eight crops. No developer commands are executed or claimed.'
        $result['developer_command_scope'] = 'not-run-release-consumer-only'
    }
}
elseif ($PercentageTextOnly) {
    $result.scope = 'percentage-text-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @()
    $result.palettes = @()
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('four-literal-percentage-value-rows', 'saved-volume-17-0-100', 'inherited-cutoff-rounded-100', 'explicit-deadzone-0-90', 'actual-music-sfx-movement-aim-adjustments', 'long-value-column-fitting', 'captured-template-and-visible-numeric-font-metrics', 'refresh-child-return-reopen-reversal', 'apply-reload-repeat', 'active-versus-disk-save-failure', 'pending-discard', 'frozen-preview-rng-resize')
    $result.input_slots = @(4)
    $result.inputs = 'Actual Controls/audio parent and Pad Sticks child with the normal GamepadReader and public local keyboard events. All four nested private pad profiles are seeded identically; the existing page chooses its initial slot. No scripted pad backend, foreground/global input, physical-device or gameplay claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated both-preset leaf for menu_text.values.percentage. Independent literal strings and UiText/OptionRow-equivalent linear references cover music/SFX and movement/aim values, saved17/0/100, inherited movement0.999 rounding to100 and explicit0/90. Existing captured controls item fonts and width-then-height value fitting remain intact. Old long copy with tilde, a new font ID missing tilde and distinct numeric bearings, real adjustments, child return, reopen/reversal and zero-update720/1080/720 drawing comparisons prove retention and adoption. Actual child Apply/reload/repeat preserves pending parent volumes and unrelated preferences; controlled private-save failure keeps active10/85 while disk remains5/85, and later Escape discards pending edits. Full private save bytes, observed status colours, unchanged current preview across source replacement, frozen redraw and draw RNG are checked. Exactly114 state updates(hard120),87 readbacks,52 value CSV rows,4 status comparisons and8 compact crops per preset. No broad Controls matrix, gameplay, foreground/global input, physical-pad, hardware qualification, listening, human readability or performance acceptance.'
}
elseif ($StageCountsOnly) {
    $result.scope = 'stage-counts-only'
    $result.Remove('stage')
    $result.stages = @('movement_course', 'close_quarters', 'king_of_the_hill', 'turbulence')
    $result.player_counts = @(1)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('bucket')
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('literal-navigation-hint-status-record-rasters', 'mixed-four-three-eligible', 'excluded-one-zero-eligible', 'mixed-two-one-eligible', 'observable-random-lock-unlock-concrete', 'positive-record-room-and-fit', 'random-record-absence', 'captured-templates-fonts-positions', 'cached-versus-refreshed-hint-fit-shadow', 'reopen-reversal', 'private-save-plan-keyboard-claim-rng')
    $result.input_slots = @(4)
    $result.inputs = 'Actual solo Stage Select with the normal gamepad reader and public local keyboard events only. Mixed four-stage/three-eligible, excluded singleton/zero-eligible and mixed two-stage/one-eligible lists exercise actual navigation and Random admission. A nested private profile seeds long practice records and muted Reduce effects preferences. No scripted pad backend, foreground/global input or physical-device claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated both-preset probe for menu_text.stage.numbered_navigation/random_hint at720/1080. Independent literal navigation/hint/status/record strings and UiTextDropShadow rasters verify all four actual mixed-list indices, eligible0/1/3, unchanged singular available-arenas wording, excluded-only navigation refusal, first Random lock/unlock and concrete return. Every state proves actual status, record presence or empty record column, finite positive record room before an independently right-aligned width fit, and full shadow/paper bounds. A private paper reference derives card expansion from the supplied literal hint and fixed solo rows, without the older hardcoded Random-count leaf or new production formatters. Old/new templates, separate visible-digit detail/brand font IDs and captured navigation position/shadow persist through presentation assignment and actual refresh; hints preserve constructor copy/font/position/ink while adopting live width-fit/shadow on slot/state changes, then reopen/reverse correctly. Exact private save/records, match plan, keyboard ownership and frozen/provisional RNG stay unchanged. Exactly104 updates(hard120),54 observed states with four literal roles each,216 CSV data rows and8 compact crops per preset. No final Random draw, match launch, broad matrix, physical-device, listening, human readability or performance acceptance.'
}
elseif ($StageSummaryOnly) {
    $result.scope = 'stage-summary-only'
    $result.Remove('stage')
    $result.stages = @('close_quarters', 'movement_course', 'eligible-random-browse-lock-only')
    $result.player_counts = @(1, 2, 3, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('bucket', 'sniper', 'sprayer', 'mister')
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('literal-composed-mode-roster-rasters', 'best-of-three-five', 'solo-best-five-precedence', 'single-match-preserved', 'all-four-sparse-rosters', 'captured-font-ids-visible-digit-bearing', 'clear-shadow-bounds', 'actual-browse-lock-unlock-refresh', 'literal-navigation-status-state-guards', 'captured-copy-reopen-reversal', 'private-save-plan-claims-input-rng')
    $result.input_slots = @(4, 3, 0, 2)
    $result.inputs = 'Actual Stage Select with public local keyboard events and neutral scripted connected sparse pad snapshots. Four contexts cover solo best-of-five precedence,2P best-of-three,3P best-of-five and4P Single Match. A nested private profile applies Reduce effects and muted volumes, keeping static text responses settled without extra timing updates. No foreground/global input or physical pad.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated compact both-preset probe for menu_text.stage.best_of_mode/solo_roster/versus_roster. Independent literal mode/roster strings and unscaled UiTextDropShadow references use the actual captured detail/brand fonts, positions, paper ink and zero-alpha shadow with retained full shadow bounds; no production summary formatter supplies expectations. Defaults cover every roster count and both best-of values plus solo precedence/Single Match at720/1080. Short old/new templates preserve old pages through actual concrete-to-Random browse, first lock, unlock and return refresh; reopening adopts the new copy and separate font IDs whose numeric bearings visibly differ, and reversal retains the reopened snapshot until another reopen restores defaults. Independent summary rows fit the authored paper and precede following rows; only the existing paper leaf is reused. Match settings/stage/duration/layout, sparse claims/teams/tools, neutral input, exact private save bytes and draw/provisional-action RNG stay unchanged. Exactly112 updates(hard120),60 summary comparisons,64 additional independent literal navigation/status raster guards after32 actual taps,120 CSV data rows with a refresh_state_verified column and8 compact two-row crops. No final Random confirmation, match launch, broad briefing/MenuText matrix, Arena counter/availability text, physical-device, listening, human readability or performance acceptance.'
}
elseif ($MenuLabelsOnly) {
    $result.scope = 'menu-labels-only'
    $result.Remove('stage')
    $result.player_counts = @(3, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('literal-player-device-labels', 'sparse-reordered-rosters', 'join-waiting-explicit-claim-reconnect', 'captured-copy-fonts-refresh-reopen-reversal', 'team-weapon-labels', 'pause-missing-packing', 'results-contribution-truncation', 'stick-fitted-device-value', 'stick-connected-disconnected-readout', 'frozen-rng-resize')
    $result.input_slots = @(3, 4, 0, 2, 1)
    $result.inputs = 'Actual Join/Team/Weapon/Pause/Results/Stick pages with deterministic scripted pad snapshots and local keyboard menu events. Sparse/reordered pad/keyboard claims and an all-pad missing list exercise device numbers independently of player numbers. A nested private profile preserves exact save bytes; no foreground/global input or physical-pad claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Bounded actual-page literal rasters cover the four authored label fields and every menu consumer without calling device_label/roster_label/player_label in the oracle. Defaults preserve Waiting to join. Short alternate pad/keyboard/waiting/row templates reorder device and player text; captured copy/fonts survive presentation assignment and actual Join/Pause/Stick refresh, adopt on reopen, and reverse on reopen. Independent UiText/UiTextDropShadow references preserve Join item shadow, Team item shadow/card, Weapon default2,2 transparent shadow, plain point-filtered Pause, zero-shadow Results contribution rows and fitted plain Stick option/help fonts. Explicit Join claim/reconnect preserves readiness/ownership, Pause keeps its three-space/two-per-line packing, and Results retain supplied canonical player numbers/Team suffixes plus literal fractional truncation12.375to12.3 and21.625to21.6. Save/settings/claims/tools/result data, frozen pixels and RNG stay unchanged. At most120 page updates, concise CSV and exactly eight small crops total per preset across720/1080. No broad matrices, full matches/series, developer commands, physical-device, human readability or performance acceptance.'
}
elseif ($WeaponTuneColoursOnly) {
    $result.scope = 'weapon-tune-colours-only'
    $result.Remove('stage')
    $result.player_counts = @(2)
    $result.palettes = @('blue-orange')
    $result.weapons = @('sprayer', 'mister')
    $result.resolutions = @('1280x720')
    $result.actions = @('validated-two-weapon-colour-commands', 'frozen-before-update-retention', 'inactive-missing-player-retention', 'neutral-reconnect-refresh', 'path-reset-reversal', 'independent-premultiplied-card-rasters', 'captured-canvas-and-outside-card-invariants', 'private-content-profile', 'frozen-draw-rng')
    $result.input_slots = @(3, 4)
    $result.inputs = 'Four actual DebugDirector tune/tune.reset commands use private source/staged/user/profile and TEMP/TMP directories. Actual Weapon Select uses sparse Pad4/keyboard seats, fixed Sprayer/Mister and blue-orange paint, with P1 ready and P2 unready. Only the GamepadReader seam is scripted; no foreground/global input or physical pad.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated Debug-only hidden720p proof for presentation.main_menu.weapon_select.selected/unselected. Two validated RGBA tune commands change both leaves; frozen draws, inactive updates and missing-claimed-device updates retain their prior card tints, then one neutral active connected update adopts both without another confirmation. Independent explicit RGBA inputs and fixed Sprayer/Mister artwork references verify premultiplied readiness paper separately from the unchanged opaque body/paint/supply canvases. Missing-device footer changes are checked within their own state; after reconnect every pixel outside the two separate card rectangles equals the connected baseline, preserving captured names/descriptions/fonts/catalog/paper. Sparse roster/tools/settings/neutral inputs, fixed palette, exact private save bytes and draw RNG remain unchanged. Two path-specific resets retain frozen widgets until refresh, then restore the exact same-page baseline with an empty edit ledger. Actual content and scratch source/staged bytes stay unchanged. At most16 bounded updates, exactly two compact crops, six-row CSV and echoed director transcript. No broad matrices, full matches or performance/human/physical-pad acceptance.'
}
elseif ($TeamTuneColoursOnly) {
    $result.scope = 'team-tune-colours-only'
    $result.Remove('stage')
    $result.player_counts = @(2)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @()
    $result.resolutions = @('1280x720')
    $result.actions = @('validated-two-team-colour-commands', 'frozen-before-update-retention', 'active-connected-refresh', 'path-reset-reversal', 'ready-unready-cursor-invariants', 'captured-frames-fonts-paper', 'private-content-profile', 'frozen-draw-rng')
    $result.input_slots = @(3, 4)
    $result.inputs = 'Actual DebugDirector tune/tune.reset commands use private source/staged/user/profile and TEMP/TMP directories. Actual Team Select has opposing sparse Pad4/keyboard seats, with P1 ready and P2 unready. Only the GamepadReader seam is scripted; no foreground/global input or physical pad.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated Debug-only hidden720p proof for presentation.main_menu.team_select.selected/unselected. Two real validated RGB tune commands change both leaves; frozen draws retain the previous refresh colours, then a neutral active connected page update adopts both. Independent fixed A/B and P1-ready/P2-unready icon/paper rasters verify colours without FocusGroup or production formatting; all outside-strip pixels, captured frames/fonts/lettering/paper, sparse roster/tools/settings/input snapshots, exact private save bytes and draw RNG remain unchanged. Two path-specific tune.reset commands retain frozen widgets until refresh and then restore the exact same-page baseline with an empty edit ledger. Actual content and scratch source/staged bytes remain unchanged. At most12 bounded page updates, exactly two small icon-strip crops, three-row CSV and echoed director transcript. Existing TeamFocusOnly and general TuneColoursOnly matrices, full matches, broad save/refusal tests and performance/human/physical-pad acceptance are not invoked.'
}
elseif ($TeamFocusOnly) {
    $result.scope = 'team-focus-only'
    $result.Remove('stage')
    $result.player_counts = @(1, 2, 3, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('semantic-team-cursors', 'nonwrapping-endpoints', 'sparse-reordered-slots', 'ready-seat-tint-retention', 'cursor-frame-local-motion', 'confirm-back-priority', 'shuffle-priority-readiness-reset', 'empty-team-refusal', 'reconnect-fresh-press', 'committed-weapon-route-return', 'captured-atlas-frames-text-fonts', 'live-colours-on-refresh', 'nonzero-origin-reversed-physical-order', 'frozen-draw-rng-resize')
    $result.input_slots = @(3, 4, 0, 2)
    $result.inputs = 'Actual Team Select and Weapon Select pages over a callback sentinel root. Deterministic local keyboard and scripted GamepadReader snapshots exercise sparse/reordered seats, including keyboard/mouse slot4. No physical pad, foreground/global input or full match.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Compact720/1080 native scope: independent semantic choices/readiness and plain icon/paper/title/label rasters cover1-4players, A/none/B nonwrap endpoints, no-team confirmation refusal, a locked ready seat while another cursor moves, local selection motion/reduction, simultaneous Confirm/Back and global Shuffle priority, empty-team final confirmation, missing/reconnected pad fresh-press gating, exact team commit into actual Weapon Select and unready return. Sparse claims/tools/stage/layout/duration/best-of and the ordinary callback sentinel remain intact. Captured atlas/frame names/text/fonts retain old widgets through assignment/reversal and adopt on reopen; existing live team colours adopt on visual refresh. An in-memory uniquely named sheet clones the six shipped64-square Team frames over the existing texture with A/B pivots-128/+128; their50-unit canvases exchange physical positions while semantic navigation stays unchanged. No source assets are written. Frozen drawing/RNG and resize checks, at most240 updates, concise CSV and exactly eight compact success crops total per preset. Broad TeamSetupOnly/TeamPaperOnly matrices, full matches/series and performance/human/physical-pad acceptance are not invoked.'
}
elseif ($StickReadoutsOnly) {
    $result.scope = 'stick-readouts-only'
    $result.Remove('stage')
    $result.player_counts = @()
    $result.palettes = @('runtime-save-default')
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('literal-stick-readouts', 'all-four-pad-slots', 'disconnected-zero', 'half-percent-rounding', 'signed-movement', 'radial-cutoff', 'active-aim-direction', 'inherited-pending-deadzones', 'discard-no-save', 'captured-help-font-templates', 'reopen-reversal', 'frozen-draw-rng')
    $result.input_slots = @(0, 1, 2, 3)
    $result.inputs = 'Actual StickSettingsMenu and local keyboard menu snapshots, with the focused scripted GamepadReader supplying deterministic raw stick vectors and connection state. A nested private profile holds the immutable preview/discard save fixture; no foreground/global input or physical-pad claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Small720/1080 readout scope with prewritten numeric/literal copy instead of production formatting or stick-response helpers. Actual Pad cycling covers every slot; zero and stale disconnected input, positive/negative half-percent rounding, signed movement, a 3-4-5 radial boundary/diagonal and active/inactive aim are compared as complete unscaled plain-text rasters in independent bounds over a standalone paper background. Pending inherited50-to55percent movement/aim cutoffs alter only preview readouts; refreshes and discard preserve active settings and exact private save bytes, and reopen restores inherited responses. All six custom templates and a replacement loaded help font remain captured through assignment and live input/connection refresh, adopt on reopen and reverse only on reopen; the existing Move-height anchor remains independent of template wording. At most180 zero-dt updates, concise CSV and exactly eight compact crops total per preset. Existing broad StickSettingsOnly navigation/Apply/save checks are not invoked. No physical-pad, listening, human readability or performance qualification.'
}
elseif ($ConnectionTextOnly) {
    $result.scope = 'connection-text-only'
    $result.Remove('stage')
    $result.player_counts = @(2, 3, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('literal-connection-copy', 'sparse-claims', 'neutral-no-autojoin', 'pause-player-numbering', 'restart-loss-gate', 'fresh-reconnect-resume', 'team-warning', 'captured-text-font-refresh', 'reopen-reversal', 'frozen-draw-rng')
    $result.input_slots = @(3, 4, 0, 2)
    $result.inputs = 'Actual Join/Pause/Team pages and local MenuInput snapshots; the focused GamepadReader seam supplies deterministic sparse connections/buttons. No foreground/global input or physical-device claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Bounded connection-copy scope at720/1080: independently supplied literal text, unscaled full text/shadow bounds and native rasters over a standalone paper background; fallback Pause title and all four visible player numbers use reordered device slots. Actual Join distinguishes unclaimed, lost, returned-neutral and explicitly claimed states. Actual Restart confirmation falls back on a missing participant, Resume/Restart labels disable, missing-player Back cannot resume, and a held/released reconnect action needs a fresh press. Team warning shows only when the last confirmation would leave one team empty and clears after a provisional valid edit. All eight customized connection strings and three replacement font roles remain captured across presentation assignment and real refreshes, adopt on reopen and reverse only on reopen. Choices/claims remain unchanged except the one accepted Join action; frozen drawing preserves pixels/RNG. At most300 zero-dt page updates, concise CSV and exactly eight compact crops total per preset; no broad painter matrix, duplicate fullscreen captures, human readability, listening or performance qualification.'
}
elseif ($RememberedSetupOnly) {
    $result.scope = 'remembered-setup-only'
    $result.Remove('stage')
    $result.stages = @('king_of_the_hill', 'turbulence', 'close_quarters', 'movement_course')
    $result.player_counts = @(2)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('sprayer', 'sniper', 'mister', 'bucket')
    $result.resolutions = @('1280x720')
    $result.actions = @('actual-normal-menu-start', 'last-match-save', 'editable-cancelled-choices', 'fresh-sparse-claims', 'fresh-confirmation', 'pause-restart', 'series-continuation', 'developer-bot-replay-exclusion', 'separate-process-relaunch')
    $result.input_slots = @(3, 4)
    if ($Preset -eq 'x64-debug') {
        $result.player_counts += 4
        $result.input_slots += -1
        $result.weapons += 'roller'
    }
    $result.inputs = 'Actual GameMenu owns setup pages and the typed normal/developer launch callback. The existing scripted GamepadReader seam and local keyboard snapshots drive menus without foreground or global input. All saves, developer files and replay evidence are under a nested private profile.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Two hidden720p processes share only a new private remembered-setup profile. The writer drives actual menus, compares fresh independent Mode/Count/Join/Team/Weapon/Stage page rasters, preserves the prior save through provisional/cancelled changes and the first arena confirmation, then saves resolved choices through the real normal GameMenu callback. A normal Pause Restart and two drawn best-of-three rounds update records independently of last_match. Debug additionally starts a sparse pad/keyboard/device-less bot match through an actual private script, records and replays it through the real director/menu path, and changes a normal menu choice while an explicitly claimed seat is bot-driven; all preserve the user setup. A separate native reader process reloads Save and GameMenu, restores choices, begins with a missing pad and fresh claims, reconnects without autojoining, requires each player to confirm again and cancels a changed arena without saving. Seven compact crops, CSV and command/replay evidence retain frozen page pixels/RNG. Release runs normal menus/relaunch and excludes developer branches. No physical-pad, listening, human readability or performance acceptance.'
}
elseif ($TuneColoursOnly) {
    $result.scope = 'tune-colours-only'
    $result.Remove('stage')
    $result.stages = @('movement_course')
    $result.player_counts = @(1)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('roller')
    $result.resolutions = @('1280x720')
    $result.actions = @('validated-colour-command', 'live-unpainted-movement', 'live-debug-text', 'live-station-rgba', 'hidden-station-zero', 'captured-home-mode', 'reopen-reset-reversal', 'resource-rejection', 'queued-reload-guards', 'staged-reload-fallback', 'scratch-save', 'frozen-state-rng')
    $result.input_slots = @(4)
    $result.inputs = 'Real DebugDirector validated commands and actual consumers/pages/match; no foreground input or physical-device claim. Source/staged/user/profile and candidate TEMP/TMP directories are private to this probe.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Debug-only hidden720p renderer proof for seven audited existing colour strings through the real validated tune path: unpainted movement fallback/highlight/keyline, ordinary DebugText foreground/shadow and existing station Label premultiplied RGBA update live; Home and Mode retain captured backgrounds until reopening, including reset reversal. Named/RGB/RGBA values, same-size station refresh and hidden zero tint, complete resource candidate rejection, actual queued scratch reload refusing tune/reset/save until rebuild/rebase, staged fallback when source files are absent, private save/reload and original content preservation. Compact native crops/CSV, repeated frozen pixels, draw RNG and real match observations/Debug fingerprint retained. No Release native run, physical-pad, human readability, listening or performance acceptance.'
}
elseif ($ParallaxOnly) {
    $result.scope = 'parallax-comparison-only'
    $result.Remove('stage')
    $result.stages = @('close_quarters', 'king_of_the_hill', 'turbulence', 'movement_course')
    $result.player_counts = @(1, 4)
    $result.palettes = @('blue_orange')
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('32-clean-heavy-gameplay-camera-cases', 'ordinary-scene-factor-zero-parity', 'independent-raw-atlas-offset-reference', '16-literal-clipped-edges', 'opaque-static-foreground-masks', 'matching-opaque-prop-reference-samples', 'live-reduced-static-parity', 'actual-static-stage-preview', 'resize-return', 'frozen-draw-rng')
    $result.input_slots = @()
    $result.inputs = 'Comparison fixture only: real builder-owned scenery and painted terrain in Scene; PlayerCamera::follow uses the actual full-screen-height gameplay scale for 1P/4P, authored spawns and explicitly placed landmark-near focus points with authored camera bounds. Independent panes, normal reader, no synthetic native activation or input. Placed points establish no route reachability.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Bounded static/A/B current-arena prototype only. A moves far sun/clouds/birds by2percent horizontal camera-centre lag capped32worldunits; B adds one shared1percent hill lag capped16, both snapped in4worldunits about each unchanged overview centre. Actual objects draw with local per-view cameras and matching culls; static props/shadows/ordinary terrain and paint restore the original camera. Ordinary Scene/factor0 parity precedes independent unculled raw-atlas/destination comparisons; frozen cloud phase is read through rectangle(), while isolated nondrifting Sun/hill edges use literal sources/displacements and negative/fractional origins outside the ordinary cull.32 clean/heavy cases, measured>99percent real paint ownership, opaque black/white foreground and matching prop-reference samples,4P independence, resize/back, Reduce effects plus cloud-anchor parity, neutral anchor and actual unchanged StagePreview.372 readbacks and exactly8 native1536x720 contact sheets, each512x360 cells: columns static/A/B, rows1Pspawn/4Pplaced-focus. Conservative far/hill cull-candidate counts do not imply raster contribution. Per-view CSV rows repeat framewide A/static and B/A deltas; count totals increment once per primary case. Far absence in gameplay panes is recorded, not hidden by changing zoom. No shipped parallax, new art/levels/schema, live-match transitions, route, human style/readability, physical-device, listening or performance acceptance.'
}
elseif ($LandmarkOnly) {
    $result.scope = 'static-landmarks-only'
    $result.Remove('stage')
    $result.stages = @('close_quarters', 'king_of_the_hill', 'turbulence', 'movement_course')
    $result.player_counts = @(1, 4)
    $result.palettes = @('blue_orange', 'blue_yellow', 'pink_green')
    $result.weapons = @('roller')
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('48-clean-heavy-pairs-96-primary-states', 'authored-overview-pane-framing', 'measured-heavy-owned-paint', 'positive-each-landmark-pixels', 'independent-raw-atlas-reference', 'terrain-paint-occlusion-order', 'native-scene-edge-parity', 'static-reduce-effects-update', 'actual-stage-preview', 'actual-level-monitor-overview', 'frozen-state-rng-debug-fingerprint', 'resize-return')
    $result.input_slots = @(3, 0, 1, 2)
    $result.inputs = 'Static actual LevelObjectBuilder/Scene geometry uses one/four pane layouts with unchanged authored overview cameras; it does not claim player gameplay framing. Supplemental actual LevelBuilder solo/four-player stories use neutral sparse PlayerInputData and private scratch profiles. Ordinary GamepadReader backend, no foreground/global input or physical-pad claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Exactly48 clean/heavy paired comparisons /96 primary coverage states across4 arenas x3 palettes x1P/4P pane layouts x720/1080. Heavy coverage is actual alternating-team projectile contact on every StructurePaintable, measured above99percent; each prop has positive surviving pixels. Literal native placement/WHITE/Visual/nonpaintable checks and raw atlas-source draws independently replace only the two scenery calls before actual terrain/paint, proving pixels, occlusion order and culling. Frozen drawing preserves geometry/paint attribution/RNG and native resize returns. Supplemental64 native clipped-edge cases compare actual Scene and miniature traversal to unculled raw atlas, with static update/Reduce effects silhouette invariance. Eight actual StagePreview size cases compare independently specified physical panes/cameras;24 actual Level monitor-preview/overview size cases compare identically seeded twins with a private stage copy omitting exactly the two props, preserving gameplay observations and frozen Debug replay fingerprints. Solo bypasses the monitor. All reference/repeat readbacks and paired story ticks are counted separately. Exactly8 compact native clean/heavy crops total,48-row primary CSV and bounded integration CSV; no broad older terrain matrix, full match gameplay route, human readability, listening, physical-device or performance acceptance.'
}
elseif ($MiniatureTerrainOnly) {
    $result.scope = 'miniature-terrain-only'
    $result.Remove('stage')
    $result.player_counts = @(1, 4)
    $result.stages = @('close_quarters', 'king_of_the_hill', 'turbulence', 'movement_course')
    $result.palettes = @('blue_orange')
    $result.weapons = @('roller')
    $result.actions = @('independent-edge-tick-pixels', 'masked-seam', 'lit-thin-ruler', 'ramp-silhouette', 'owned-paint-overlay', 'legacy-comparison', 'native-material-parity', 'native-scene-order-culling', 'opening-preview-push', 'active-pullback-overview', 'resize-return', 'frozen-state-rng')
    $result.input_slots = @(3, 1, 2, 0)
    $result.inputs = 'Direct material/Structure and actual builder-owned Level draws with neutral sparse PlayerInputData. Normal GamepadReader backend; no physical-device claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Independent complete exposed-edge/masked-top-span/body pixel oracle and actual major-ruler tick/light-gap, one-pixel light ruler and original filled ramp silhouette checks. Legacy and adapted material comparisons at 1P/4P720p/1080p/QHD; exact point-sampled material equality at one cell per pixel and larger. Actual projectile-owned paint remains above terrain without changing score or attribution. All four real arena overview compositions compare legacy/adapted rasters; native Scene versus client traversal preserves painter order/culling/pixels. Actual sparse-roster solo and four-player opening/preview/push/active/pullback/overview contexts at all three native sizes preserve frozen public observations, RNG and Debug replay fingerprints, with exact resize returns. Selected native detail/context crops accompany compact comparison CSV; no video or full painter matrix. Muted rendering under possible concurrent Unreal validation does not establish human readability/style, physical-pad, listening or performance acceptance.'
}
elseif ($DriftOnly) {
    $result.scope = 'cloud-drift-only'
    $result.player_counts = @(1, 4)
    $result.stages = @('close_quarters', 'king_of_the_hill', 'turbulence', 'movement_course')
    $result.palettes = @('blue_orange')
    $result.weapons = @('sprayer')
    $result.actions = @('cloud-anchor', 'cloud-right', 'cloud-left', 'cloud-loop', 'scene-edge-culling', 'reduce-effects', 'nonpositive-nonfinite-time', 'frozen-redraw', 'draw-update-rng', 'resize-round-trip')
    $result.input_slots = @(3, 0, 1, 2)
    $result.inputs = 'Real DriftingVisual in Scene with ordinary render fan-out and direct unculled reference; neutral scripted per-slot PlayerInputData for real arena context captures. Normal GamepadReader backend; no physical-device claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Native cloud_big art in real Scene at 1P/4P 720p/1080p/QHD: anchor exactly matches static Visual, both 48-unit extremes visibly differ, period closes exactly, all changed pixels lie inside projected cull bounds. Both edges compare Scene output against direct unculled drawing while the original anchor lies wholly outside the view. Immediate applied Reduce effects restores the anchor and freezes the phase; switching back restores the same displaced raster. Nonpositive/nonfinite time, repeated frozen drawing and draw/update RNG remain unchanged. Exact 720p resize round trip. Actual builder-owned solo contexts from all four arenas and a 4P1080 Close Quarters sky composition with explicitly placed players preserves frozen public simulation and Debug fingerprints. The 4P composition makes no gameplay-route or jump-reachability claim. Muted hardware rendering; human motion/readability, listening and performance acceptance remain separate.'
}
elseif ($IdentityOnly) {
    $result.player_counts = @(2, 4)
    $result.weapons = @('sprayer', 'sniper')
    $result.actions = @()
    foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'monitor_checks', 'countdown_checks', 'closing_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.player_identity_checks
}
elseif ($QuietOnly) {
    $result.weapons = @('sprayer', 'sniper', 'bucket')
    $result.actions = @()
    foreach ($key in 'contact_checks', 'weapon_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks', 'countdown_checks', 'closing_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.quiet_checks
}
elseif ($MonitorOnly) {
    $result.player_counts = @(1, 2, 4)
    $result.palettes = @('blue_orange')
    $result.weapons = @('roller')
    $result.actions = @()
    foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'countdown_checks', 'closing_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.monitor_checks
}
elseif ($CountdownOnly) {
    $result.player_counts = @(1, 2, 4)
    $result.palettes = @('blue_orange')
    $result.weapons = @('roller')
    $result.actions = @()
    $result.input_slots = @(4, 3, 1, 0)
    $result.inputs = 'Scripted keyboard/mouse seat and sparse pad-slot inputs; no physical-device claim.'
    foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks', 'closing_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.countdown_checks
}
elseif ($ClosingOnly) {
    $result.player_counts = @(1, 2, 4)
    $result.weapons = @('roller')
    $result.actions = @()
    foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks', 'countdown_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.closing_checks
}
elseif ($SupplyOnly) {
    $result.weapons = @('sprayer', 'mister')
    $result.actions = @()
    foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks', 'countdown_checks', 'closing_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.supply_checks
}
elseif ($TeamSetupOnly) {
    $result.player_counts = @(2, 3, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('sprayer', 'sniper', 'roller', 'bucket')
    $result.actions = @('shuffle', 'concurrent-confirm', 'reconfirm', 'weapon-return', 'manual-team-edit', 'held-entry', 'conflicting-actions', 'disconnect', 'focus-return', 'solo-no-op', 'warning-clear', 'pause-restart', 'series-next-round')
    $result.input_slots = @(4, 3, 0, 2)
    $result.inputs = 'Real MainMenuTeamSelect/MainMenuWeaponSelect, StateContext and MenuInput; test-only concrete GamepadReader backend supplies connected states and armed button releases. Public keyboard messages supply R/Enter/Escape. No physical-controller or native-foreground acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Actual 2/3/4-player seeded balanced shuffles, both three-player majority sides, requests from every joined seat, concurrent confirm discarded, all readiness cleared and every player required to reconfirm. Provisional teams match a fresh actual page raster and independently specified complete X/R legend; final confirmation commits just teams. Weapon return preserves teams, tools, sparse slots, stage, duration and best-of; manual edits, held entry, conflicting actions, missing pad, reconnect-held X and focus-return-held R are gated. Shuffle clears the empty-team warning; solo return hides Random teams and ignores R. Native 720p/1080p rows/footer captures and measured four-row/legend bounds, frozen page raster. One committed shuffled 2P roster retains actual Player teams, input slots and weapons through Pause Restart and two best-of-three rounds using ordinary Results/End continuation, with the ongoing-series primary card checked against its expected ledger. Physical pads, human readability and listening acceptance remain separate.'
}
elseif ($StickSettingsOnly) {
    $result.player_counts = @()
    $result.palettes = @('blue_yellow')
    $result.weapons = @()
    $result.actions = @('slot-selection', 'deadzone-clamps', 'apply', 'failed-save', 'back-discard', 'parent-apply')
    $result.input_slots = @(0, 1, 2, 3, 4)
    $result.inputs = 'Public keyboard through production Pad sticks, Controls/audio and StateContext, with a separate isolated save profile. Preview reads the current pad snapshot; no physical controller is required.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Actual Pad sticks child at 720p/1080p/QHD: five bounded focus cards/arrows, full labels/values/status and response/association copy, native linear ink and contrast, initial and pausing slot selection, slot/deadzone clamps and pending cutoff previews, Apply/reload/failure and Back/Escape discard. Suspended parent Apply preserves child profiles. Frozen redraw, draw RNG, static Reduce effects and resize. No physical-controller comfort, human readability or listening acceptance.'
}
elseif ($ControlsMenuOnly) {
    $result.player_counts = @(1)
    $result.palettes = @('blue_yellow')
    $result.weapons = @('sprayer')
    $result.actions = @('row-change', 'numeric-clamps', 'apply', 'failed-save', 'back-discard', 'options-return', 'pause-return', 'custom-labels', 'long-label-fit', 'controls-font-snapshot', 'focus-return', 'custom-help', 'multiline-help-fit', 'help-font-snapshot', 'preview-refresh', 'custom-status', 'status-font-snapshot', 'save-recovery', 'interrupted-apply')
    $result.actions += @('custom-setting-values', 'value-width-height-fit', 'value-font-snapshot', 'value-child-return')
    $result.input_slots = @(4)
    $result.inputs = 'Public keyboard through production Controls/audio, Options, Pause and actual GameLevel/StateContext, with a separate isolated save profile. Preview gain is zero for this raster check; saved music/SFX remain muted.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Controls/audio paper, all eight focus cards and detached arrows; measured full labels/values, bounded complete controller/keyboard/readiness/effects help and save status, native linear text and ink/paper contrast across 720p/1080p/QHD, 4:3, 16:10 and ultrawide. Actual clamps and pending edits, Apply with persisted reload, controlled save failure, Back discard, pending Options values/focus and actual paused GameLevel return. Actual Pad sticks child at 720p/1080p/QHD: five focus cards, full labels/values/status and response/association copy, initial and pausing slot selection, slot/deadzone clamps and pending cutoff previews, Apply/reload/failure and Back/Escape discard. Thirteen independent historical label substitutions across all six sizes/aspects; long OptionRow fitting, full label/value focus-card bounds, a separate Controls item font missing an old glyph, retained/reopened/reversed text/font snapshots, repeated child return and Apply, interrupted Enter requiring a fresh selection, save reload and pending-edit discard. Three independent static-help defaults; authored Controls and association blocks exercise both width and multiline height fitting. Unscaled aim caption uses the Move height probe and the full help column beyond its diagram width. A replacement help font missing an old glyph, retained/reopened/reversed help snapshots, value/preview refresh, old-parent/new-child return, repeated/interrupted Apply and discard pass at 720p. Independent pending/saved/failure status rasters pass across six sizes/aspects on Controls and Pad Sticks. Authored width fitting with compact help, scale reset to short saved copy, retained/reopened/reversed status callbacks using a font missing an old glyph, interrupted/repeated Apply, controlled failure/recovery, isolated reload, both discard routes and old-parent/new-child profile merge pass at 720p. Suspended parent Apply preserves child profiles. Frozen redraw/state, draw RNG, static Reduce effects and resize. No physical-controller, human readability, preview audition or listening acceptance.'
    $result.checks += ' Independent Controls On/Off defaults pass across six sizes/aspects. A Controls item-font fixture exercises width and tall-glyph height fitting, complete value/focus-card unions, refitting between width/height limits, reopened short copy and replacement without an old glyph. Retained/reopened/reversed callbacks, Left/Right boolean assignments, Pad sticks child return/profile merge, interrupted/repeated Apply, controlled failure/recovery, isolated reload and discard pass at 720p.'
}
elseif ($SettingsMenuOnly) {
    $result.player_counts = @()
    $result.palettes = @('blue_orange', 'pink_green', 'blue_yellow', 'random')
    $result.weapons = @()
    $result.actions = @('row-change', 'apply', 'failed-save', 'controls-return', 'discard', 'disabled-resolution', 'credits-back', 'custom-status', 'status-font-snapshot', 'save-recovery', 'interrupted-apply')
    $result.actions += @('custom-setting-values', 'value-width-height-fit', 'value-font-snapshot', 'value-child-return')
    $result.input_slots = @(4)
    $result.inputs = 'Public keyboard through production Options/Credits and StateContext, with a second isolated save profile; fullscreen metadata exercises disabled rows. Only false/Off is applied using an isolated offscreen, windowed fixture, which is immediately hidden again.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Options/Credits paper, all enabled focus cards/arrows, native ink/paper contrast, measured labels, all palette values, long failure status and complete authored credits at 720p/1080p/QHD, 4:3, 16:10 and ultrawide. Actual row edits, staged Apply, persisted reload and failed-write warning, Controls/audio return preserving pending edits/focus, Back discard, disabled fullscreen resolution and retained windowed preset, Credits Select/Back, frozen redraw, draw RNG, static Reduce effects and resize. Independent Options pending/saved/failure rasters pass across six sizes/aspects; authored width fitting and scale reset, missing-old-glyph status font, retained/reopened/reversed callbacks, interrupted/repeated Apply, controlled failure/recovery, isolated reload and both discard routes pass at 720p. No physical-controller, human readability or listening acceptance.'
    $result.checks += ' Independent Options On/Off/Random defaults pass across six sizes/aspects. An Options item-font fixture exercises width and tall-glyph height fitting, complete value/focus-card unions, Random-to-short-fixed-palette scale reset and replacement without an old glyph. Retained/reopened/reversed callbacks, both fullscreen toggle directions and Random palette cycles, one Controls child On/Off pair and return, interrupted/repeated windowed-Off Apply, controlled failure/recovery, isolated reload and discard pass at 720p.'
}
elseif ($PausedOptionsOnly) {
    $result.scope = 'paused-options-only'
    $result.player_counts = @(1, 2, 4)
    $result.palettes = @('blue_yellow', 'pink_green', 'random')
    $result.weapons = @('sprayer')
    $result.actions = @('actual-match-pause-options', 'all-seven-focus-rows', 'caption-font-snapshot', 'own-back-discard', 'palette-next-build', 'sparse-owner', 'joined-fallback', 'keyboard-recovery', 'held-reconnect', 'controls-sticks-merge', 'windowed-display-trial', 'revert', 'escape', 'timeout', 'focus-loss', 'cover', 'keep-failed-write-recovery', 'reduce-effects', 'nonpositive-time', 'developer-unwind', 'frozen-draw-rng')
    $result.input_slots = @(4, 3, 0, 2, 1)
    $result.inputs = 'Actual1/2/4 player GameLevel to Pause row2 Options, scripted sparse pad rosters and local keyboard. Nested isolated profile with nondefault shared preferences and four distinct deadzone profiles. Whole-fixture activation guard; hidden private windowed display changes only.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Independent complete seven-row paused Options paper, caption, values, status and footer at six native sizes/aspects. Retained Level identity, roster/layout/resume clock, full observation and actual Level/HUD raster at a common720 size cover painted match preservation; no direct private Session or timer-field claim. Owner-only input, joined fallback, keyboard recovery and reconnect arming propagate into actual Controls/Pad Sticks and display child. Separate nested saves preserve other preferences; palette-only Apply leaves the live arena unchanged and Restart uses its new palette. Private windowed trial safe Revert/Escape, elapsed timeout, focus/cover rollback, failed Keep preserving old JSON and recovery reload. Back returns retained Pause; only Pause Resume starts count-in. Captured caption/font, lifecycle, reduction and frozen draw RNG. No physical-pad, user-save, hardware-fullscreen, global-input, human or performance acceptance.'
}
elseif ($DisplayTrialOnly) {
    $result.scope = 'display-trial-only'
    $result.player_counts = @()
    $result.palettes = @('blue_yellow', 'random')
    $result.weapons = @()
    $result.actions = @('actual-options-apply', 'default-revert', 'keep', 'escape', 'controlled-timeout', 'wall-clock-timeout', 'focus-loss', 'cover-rollback', 'developer-dismiss-reset', 'clear-rollback', 'held-entry', 'pad-reconnect', 'conflicting-input', 'move-confirm-separation', 'back-priority', 'keep-failed-write', 'recovery', 'metadata-fullscreen-keep-off', 'reduce-effects', 'nonpositive-time', 'frozen-rng')
    $result.input_slots = @(4, 3)
    $result.inputs = 'Actual Options changed-resolution Apply and its pushed trial child; local keyboard and sparse scripted pad slot3. Separate nested profile with nondefault controls and four distinct pad profiles. One nonnested activation guard covers all private-window changes and rollbacks, including destruction. Actual tests stay windowed Off; fullscreen is saved metadata only.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Independent complete trial background, compact card geometry with captured Pause palette, centered Options/question, Keep/Revert card/arrow, countdown and footer at six native sizes/aspects. Explicit finite simulation time leads wall capture time under a configured60-second bound; the shipped15-second timeout is separately sampled after3.25 simulated seconds, and a1-second trial proves zero-dt wall expiry. Real Options Apply first changes only its private window to1920x1080; Save/file remain unchanged until fresh Keep. Default Revert/Escape, simulated exact deadline, wall deadline, focus loss, cover, developer dismiss/reset and clear restore the old configured1280 preset/prior actual windowed mode without preference/file writes. Cover remains present until explicitly popped and delivers one typed result afterward. Keep/reload, repeated unchanged Apply, one controlled failed Keep preserving old JSON and recovery preserve every shared preference and all four profiles. Keyboard/pad conflicts, held reconnect/entry, move-confirm separation, Back priority, immediate reduction and frozen pixels/RNG pass; nonpositive updates preserve nonclock geometry while the wall countdown may advance. Native backbuffer resizing for the six-aspect references does not replace configured rollback targets. No user-save, hardware-fullscreen, global input, physical-pad, human readability, listening or performance acceptance.'
}
elseif ($OptionsDefaultsOnly) {
    $result.scope = 'options-defaults-only'
    $result.player_counts = @()
    $result.palettes = @('blue_yellow', 'random')
    $result.weapons = @()
    $result.actions = @('all-seven-rows', 'staged-restore', 'repeat-restore', 'back-discard', 'escape-discard', 'controls-return', 'controls-save-merge', 'windowed-apply', 'repeat-apply', 'reload', 'failed-write', 'recovery', 'held-confirm-rearm', 'pad-reconnect', 'fullscreen-metadata-off', 'reduce-effects', 'lifecycle-reset', 'frozen-rng')
    $result.input_slots = @(4, 3)
    $result.inputs = 'Actual Options and Controls child in StateContext; public local keyboard messages and sparse scripted pad slot3. Nested isolated save profile with nondefault display/play settings and four distinct pad profiles. Windowed Apply uses the existing narrow CBT activation guard and offscreen hidden probe; fullscreen is metadata only.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Independent complete seven-row Options paper/cards/arrow, fitted labels/values, plain text, pending/saved/failure status and footer at six sizes/aspects. Canonical display/palette defaults remain staged with active/window/file/play/profile state unchanged until Apply; repeated restore, both discard routes and Controls return retain intended values/focus. Hidden windowed Apply/repeated Apply/reload and six controlled temporary-file write refusals/recovery preserve every unrelated play field and all four slot profiles. At720p keyboard deactivation and held pad reconnect require fresh Restore, reduced effects/lifecycle/developer reset remain bounded, applied-fullscreen metadata keeps the original disabled/live-resolution policy until Off Apply, and a Controls-saved preference merges into the pending display Apply. Frozen pixels and draw RNG; no user-save, hardware-fullscreen, physical-pad, human readability, listening or performance acceptance.'
}
elseif ($HomeExitOnly) {
    $result.scope = 'home-exit-only'
    $result.player_counts = @()
    $result.palettes = @()
    $result.weapons = @()
    $result.actions = @('home-exit-push', 'safe-no', 'back-no-yes', 'same-home-resume', 'fresh-reopen', 'held-entry', 'interrupted-enter', 'held-pad-reconnect', 'conflicting-actions', 'simultaneous-devices', 'back-priority', 'reduce-effects', 'nonpositive-time', 'cover-developer-reset', 'pending-music-latch', 'muted-immediate-quit', 'frozen-rng', 'resize')
    $result.input_slots = @(4, 3)
    $result.inputs = 'Actual Home Exit pushed child in StateContext, public local keyboard messages across six sizes/aspects and sparse scripted pad slot3 at720p. Hidden no-activate window; final deliberate Yes closes only this probe window. Intermediate Yes is held at the existing music callback seam. No save writes.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Independent complete Home Exit neutral background, stepped confirmation paper, unscaled centered Exit/question, Yes/No cards/arrow and footer across six native sizes/aspects. Safe No, Back from either answer and repeated fresh reopen resume the exact retained Home Exit frame; source-copy replacement verifies retained parent text. At720p keyboard deactivation and sparse-pad held reconnect reject stale Yes, same-device conflicting actions remain inert, simultaneous confirmations push/pop once, cross-device held entry is disarmed, navigation cannot confirm Yes in the same update and Back has priority. Ordinary motion, immediate reduction, nonpositive time, cover/activation/developer resets, safe developer dismissal, resize and frozen pixels/empty plan/draw RNG pass. Deliberate Yes starts one pending music exit, keeps outgoing pixels fixed and suppresses repeated input; developer dismissal cancels that pending exit. Final no-update/muted Yes closes only the hidden probe window. Scripted input establishes no physical-pad, human readability, listening or performance acceptance.'
}
elseif ($HomeReturnFocusOnly) {
    $result.scope = 'home-return-focus-only'
    $result.player_counts = @()
    $result.palettes = @()
    $result.weapons = @()
    $result.actions = @('initial-play', 'title-entry-play', 'mode-back-play', 'options-back-action', 'options-back-row', 'credits-back-action', 'credits-back-row', 'fresh-confirm-reopens-origin', 'initial-focus-api', 'held-enter-deactivation', 'held-pad-developer-reset', 'cover-return', 'reduce-effects', 'nonpositive-time', 'frozen-rng', 'resize')
    $result.input_slots = @(4, 3)
    $result.inputs = 'Actual Home/Title/Mode/Options/Credits in StateContext; public local keyboard messages across six sizes/aspects and sparse scripted pad slot3 at720p. The existing scripted GamepadReader link seam uses no physical devices. Hidden no-activate window; no save writes.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Complete independent Home background, outer paper, persistent Play card, selected-row card, detached arrow, plain text and footer rasters across six sizes/aspects. Initial Home, Title entry and Mode Back focus Play; Options and Credits action Back and authored Back-row returns restore their origin. Fresh confirmation without navigation reopens the exact real destination; repeated loops and explicit initial-focus API preserve the same rows. At720p keyboard deactivation cancels held Enter and sparse pad developer reset cancels held A, requiring a fresh confirmation. Ordinary cursor motion, nonpositive time, immediate Reduce effects, activation/cover/developer motion resets, static reduced entry/child return, resize and frozen pixels/empty plan/draw RNG pass. No phantom focus or press animation on return; no protected focus accessors, physical-pad, human readability, listening or performance acceptance.'
}
elseif ($StagePaperOnly) {
    $result.scope = 'stage-paper-only'
    $result.player_counts = @(1, 2, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('sprayer', 'sniper', 'roller', 'bucket')
    $result.stage = @('king_of_the_hill', 'turbulence', 'close_quarters', 'movement_course', 'eligible-random')
    $result.actions = @('all-arenas', 'eligible-random', 'both-wraps', 'solo-single-series-roster', 'lock-unlock', 'locked-arrows-refused', 'start-real-random', 'latched-music-exit', 'back-weapon', 'missing-reconnect', 'missing-back-count', 'accent-motion', 'reduce-effects', 'lifecycle-resets', 'captured-art-font-copy', 'live-name-status-ink-fit', 'overflow-navigation', 'preview-separation', 'no-eligible-random', 'resize')
    $result.input_slots = @(4, 3, 0, 2)
    $result.inputs = 'Actual Stage Select and ordinary Weapon/Count destinations in StateContext; local keyboard messages and the existing scripted GamepadReader link seam provide solo and sparse connected seats. The existing music-exit callback holds outgoing pages for exact raster checks. Hidden no-activate window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Independent complete outer paper, dynamically measured briefing paper, unscaled/fitted dark lettering and actual miniature StagePreview raster at its original physical pane/camera. Every shipped arena and eligible Random across six sizes/aspects in solo, two-player single/best-of-three and four-player best-of-five contexts. Both wraps, lock/unlock, refused locked arrows, immediate real/Random start with zero/one exact RNG draw, latched outgoing Random during pending music exit, one state pop, ordinary Weapon Back, missing/reconnect/held rearming and Count rescue. Native720p/1080p/QHD accent initial/middle/rest, rapid final lock/start, nonpositive time, immediate Reduce effects, activation/cover/developer resets, interrupted confirmation, fixed text/preview and resize. Captured art/font/copy survives source replacement, refresh/reopen/reversal while live name/status ink and name/hint fit remain explicit. Empty eligible set omits Random, a single excluded stage is stable, and resource-valid navigation overflow/text-padded preview overlap fail precise runtime geometry guards. Frozen pixels/plan/RNG; scripted reader only, no physical-pad, human readability, listening or performance acceptance.'
}
elseif ($WeaponPaperOnly) {
    $result.scope = 'weapon-paper-only'
    $result.player_counts = @(1, 2, 3, 4)
    $result.palettes = @('random-neutral', 'blue_orange', 'pink_green', 'blue_yellow')
    $result.weapons = @('sprayer', 'sniper', 'roller', 'mister', 'bucket', 'random')
    $result.actions = @('all-choices', 'both-wraps', 'ready-unready', 'confirmed-refusal', 'all-random-commit', 'ordered-rng', 'missing-reconnect', 'back-mode-team', 'commit-stage', 'stage-back', 'missing-back-count', 'choice-motion', 'reduce-effects', 'lifecycle-resets', 'captured-art-font-catalog', 'live-readiness-fill', 'overflow-name-description', 'resize')
    $result.input_slots = @(4, 3, 0, 2)
    $result.inputs = 'Actual Weapon Select and ordinary Mode/Team/Stage/Count destinations in StateContext; local keyboard messages and the existing scripted GamepadReader link seam provide sparse connected seats. Palette changes write only the isolated probe profile. Hidden no-activate window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Independent complete outer paper, individual tile paper, unscaled heading/player/name/description lettering, P1-P4 badge cells and fixed tool/paint/final-supply layers across six native sizes/aspects, one-to-four sparse seats and every choice. Right/left wrap, provisional browsing, readiness/unreadiness, refused confirmed input, exact ordered per-seat Random draws, immediate committed Stage and Back Mode/Team/Stage-return routes; missing-player footer, held reconnect and Count rescue. All fixed palettes and neutral Random preview preserve draw RNG. Native720p/1080p/QHD simultaneous/rapid tile contraction, initial/middle/rest, fixed inner art/masks/text/badges, nonpositive time, immediate Reduce effects, activation/cover/developer resets and resize. Captured art/font/catalog/description/palette snapshots survive refresh/reopen/reversal, including sparse custom catalog fill; icon readiness fill remains live while readable name inks stay captured. Actual unscaled name and last-row description overflow are rejected by precise runtime diagnostics. Frozen pixels/plan/RNG; scripted reader only, no physical-pad, human readability, listening or performance acceptance.'
}
elseif ($TeamPaperOnly) {
    $result.scope = 'team-paper-only'
    $result.player_counts = @(1, 2, 3, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('sprayer', 'sniper', 'roller', 'bucket')
    $result.actions = @('initial-roster', 'unassigned', 'team-a-b', 'ready-unready', 'both-teams-warning', 'warning-clear', 'seeded-shuffle', 'missing-reconnect', 'back-count', 'commit-weapon', 'choice-motion', 'reduce-effects', 'lifecycle-resets', 'captured-art-font-atlas', 'overflow-warning', 'resize')
    $result.input_slots = @(4, 3, 0, 2)
    $result.inputs = 'Actual Team Select and ordinary Count/Weapon destinations in StateContext; local keyboard messages and the existing scripted GamepadReader link seam provide sparse connected seats. Hidden no-activate window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Independent full paper/static row-card/text/warning/icon/badge rasters for solo and two/three/four sparse-player rosters at720p/1080p/QHD,4:3,16:10 and ultrawide; unchanged unscaled font anchors, logical shadow unions, native contrast, all three choices, readiness/unreadiness and refused no-team/same-team completion, warning recovery, seeded balanced shuffle with exact RNG boundary, ordinary committed Weapon and Back Count routes. Missing-player footer and held reconnect rearming preserve provisional settings. Native720p/1080p/QHD simultaneous choice contraction, middle/rest, readiness press, local fixed paper/text/badge pixels, immediate Reduce effects, nonpositive time, activation/cover/developer resets and resize. Captured paper art, fonts and atlas/frame groups survive source replacement, choice/shuffle refresh, reopen and reversal while existing icon tint policy remains live. Actual warning overflow is rejected by its specific runtime guard. Frozen pixels/plan/RNG; scripted reader only, no physical-pad, human readability, listening or performance acceptance.'
}
elseif ($JoinPaperOnly) {
    $result.scope = 'join-paper-only'
    $result.player_counts = @(1, 2, 3, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('sprayer')
    $result.actions = @('reserved-p1', 'join-claims', 'complete-roster', 'reserved-reconnect', 'reconnected', 'solo-weapon-route', 'multiplayer-team-route', 'p1-back', 'missing-p1-rescue', 'claim-acknowledgement', 'reduce-effects', 'lifecycle-resets', 'captured-copy-font-art', 'invalid-status-font', 'resize')
    $result.input_slots = @(4, 3, 0, 2)
    $result.inputs = 'Actual Join Players and ordinary Count/Team/Weapon destinations in StateContext. Local keyboard messages and the existing scripted GamepadReader link seam supply keyboard or pad P1 and sparse claims/reconnects. Hidden no-activate window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Independent complete paper, unscaled transparent-shadow heading/roster/status and dark acknowledgement-gutter rasters. One-to-four-player rosters with keyboard or pad P1 cover neutral reserved P1, all joined, missing reserved pads and reconnects at720p/1080p/QHD,4:3,16:10 and ultrawide; native paper containment, ordering, lettering contrast, frozen plan/pixels/RNG and immediate ordinary continuation. Native720p/1080p/QHD gutter initial/middle/rest, nonpositive time, applied Reduce effects, activation/cover/developer resets and resize. Heading/item/detail fonts and captured paper inks survive source replacement, refresh/reconnect, reopen and reversal. Fresh destination rasters verify owner Back, missing-P1 rescue and solo/multiplayer continuation; interrupted owner confirmation requires fresh input. An actual Join page rejects overflowing status-font bounds with its specific diagnostic. Scripted reader only; no physical-pad, human readability, listening or performance acceptance.'
}
elseif ($SetupPaperOnly) {
    $result.scope = 'setup-paper-only'
    $result.player_counts = @(1, 2, 3, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @()
    $result.actions = @('all-mode-focus', 'all-count-focus', 'disabled-solo', 'duration-cycle', 'mode-choice', 'count-join', 'back-routes', 'custom-copy-font-snapshots', 'captured-effective-art', 'invalid-duration-union', 'focus-motion', 'duration-press', 'rapid-navigation', 'reduce-effects', 'deactivation', 'covering-page-return', 'developer-reset', 'resize')
    $result.input_slots = @(4)
    $result.inputs = 'Local keyboard messages through actual Mode Select, Player Count, Join and Home transitions in StateContext; normal GamepadReader backend, hidden no-activate window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Independent complete paper/card/arrow and transparent-shadow text rasters for every Mode/Count focus and disabled-solo series at 720p/1080p/QHD, 4:3, 16:10 and ultrawide, with measured card/text containment and native ink/paper contrast. All duration cycles, three modes, one-to-four-player count/layout/keyboard claims and ordinary Back/Join/Home routes retain behavior; authored text/font snapshots survive replacement, reopen, reversal, duration and child return. Effective paper art, compact focus padding and shadow remain captured through source replacement, focus callbacks, reopening and reversal. An actual Mode page rejects an overflowing compound duration value with its specific diagnostic. Native Mode/Count arrow initial/middle/rest and duration paper press/release, local envelopes, fixed opaque label/value pixels, rapid navigation, nonpositive time, immediate Reduce effects, activation/cover/developer reset, frozen redraw/RNG and resize round-trip. No physical-pad, human readability, listening or performance acceptance.'
}
elseif ($JoinStageMotionOnly) {
    $result.scope = 'join-stage-motion-only'
    $result.stage = @('movement_course', 'close_quarters', 'random')
    $result.player_counts = @(4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('bucket', 'sprayer')
    $result.actions = @('join', 'release', 'stable-hole', 'simultaneous-claims', 'same-update-replacement', 'rapid-row-restart', 'duplicate-full-refusal', 'final-claim-barrier', 'missing-p1-rescue', 'browse', 'real-random-wrap', 'lock', 'unlock', 'locked-arrow-refusal', 'final-random-start', 'rapid-final-start', 'pending-fade', 'single-excluded-stage', 'reduce-effects', 'interruption-resets', 'held-reconnect', 'resize')
    $result.input_slots = @(4, 3, 0, 2, 1)
    $result.inputs = 'Real Join Players and Arena Stage Select pages in StateContext. Local keyboard messages and the existing scripted GamepadReader link seam provide sparse seats, claims, disconnects and held reconnects. Hidden no-activate window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Native initial/middle/rest Join gutter acknowledgements and Stage briefing stripe at 720p/1080p/QHD; each changed pixel stays inside independent row-gutter or briefing-padding envelopes, preserving all text, previews, camera, edges and other rows. Stable release holes, simultaneous claims, same-update release/replacement, rapid row restart, duplicate/full refusal, final-claim snapshot barrier and independent missing-P1 Back destination; real/Random wrapping, lock/unlock, refused locked arrows, no eligible Random and deliberate settled/rapid final eligible Random commits with exactly one RNG choice, settled outgoing Random raster and gated pending fade through focus interruption and repeated input. Immediate applied Reduce effects, nonpositive time, activation/covering-page/developer-input resets, missing/reconnect rearming, frozen redraw/plan/RNG and resize round-trip. Scripted reader only; no physical-controller, human readability, listening or performance acceptance.'
}
elseif ($SetupMotionOnly) {
    $result.scope = 'setup-motion-only'
    $result.player_counts = @(4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('sprayer', 'sniper', 'roller', 'mister', 'bucket', 'random')
    $result.actions = @('changed-focus', 'duration-press', 'independent-team-choice', 'independent-weapon-choice', 'ready', 'unready', 'shuffle', 'rejected-input', 'rapid-navigation', 'reduce-effects', 'deactivation', 'missing-reconnect', 'covering-page-return', 'resize')
    $result.input_slots = @(4, 3, 0, 2)
    $result.inputs = 'Real Mode Select, Player Count, Team Select, Weapon Select and StateContext. Public local keyboard messages and the existing scripted GamepadReader link seam supply sparse connected seats and armed button releases. Hidden no-activate window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Moving/middle/rest native setup ornaments at 720p/1080p/QHD with independent per-row pixel envelopes and fixed weapon artwork, masks, names, descriptions and player badges. Four-seat setup pages, initial settled pages, simultaneous and rapid choices, readiness/unreadiness, shuffle, refused confirmation, endpoint and confirmed-row input, immediate Count-to-2P Join and final Team/Weapon transitions, applied Reduce effects suppression, activation, missing/reconnect and test-pushed covering-page return, frozen redraw/RNG and resize. No physical-controller, human readability, listening or performance acceptance.'
}
elseif ($MenuMotionOnly) {
    $result.player_counts = @()
    $result.palettes = @()
    $result.weapons = @()
    $result.actions = @('changed-focus', 'adjust-value', 'apply', 'rapid-navigation', 'reduce-effects', 'deactivation', 'child-return', 'resize')
    $result.input_slots = @(4)
    $result.inputs = 'Public local keyboard messages through actual MainMenuHome, ControlsSettingsMenu, StickSettingsMenu and StateContext; normal GamepadReader backend. Hidden no-activate window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Initial focus stays still. Actual changed-focus arrows and accepted OptionRow paper presses have distinct initial/middle/rest native rasters at 720p/1080p/QHD; successful Apply has distinct pressed/rest rasters; all changed pixels stay inside independent ornament envelopes and label/value lettering remains fixed. Nonpositive time, rapid focus, immediate applied reduced-effects drawing, real Controls Apply/reload/reversal, deactivation, nested child return, frozen redraw/RNG and resize round-trip. No physical-pad, human readability, listening or performance acceptance.'
}
elseif ($MenuTextOnly) {
    $result.player_counts = @(1, 2, 3, 4)
    $result.palettes = @()
    $result.weapons = @()
    $result.actions = @('named-prompt-rebuild', 'arena-browse', 'arena-lock', 'arena-unlock', 'random-stage-copy', 'random-stage-width-fit', 'random-stage-eligible-start', 'home-options-credits-items', 'fitted-options-labels', 'mode-count-items', 'duration-cycle', 'mode-choice', 'count-join-back', 'solo-disabled', 'focus-round-trip', 'credits-select-home', 'reopen', 'presentation-replacement', 'title-home-fixed-copy', 'title-start-card', 'interrupted-title-home-enter')
    $result.input_slots = @(4, 0)
    $result.inputs = 'Public keyboard messages through actual Title, Home, Stage Select, Options, Credits, Mode Select, Player Count and Join Players plus a concrete MenuPage fixture. This focused binary uses the existing scripted GamepadReader link seam; pads remain neutral until the briefing fixture supplies one connected second seat. No physical-controller or native-foreground acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Independent shipped brand, all 11 static headings and all 17 formatted legends compared through real MenuPage construction/prompt rebuild against native text/chrome fixtures at 720p/1080p/QHD. Ten legacy static item roles compared on actual Home/Options/Credits at those sizes; long Options labels fit before their value columns, focus round trips and Credits Select retain routing. Eight Mode/Player Count labels and the solo hint match independent complete page rasters across six sizes/aspects, including duration-column placement and the existing item-shadow hint. Actual duration cycles/wrap/confirm, all three mode choices, disabled solo series, all 1-4-player count/layout/keyboard claims and Join/Mode/Home Back routes. Reachable Random-stage display uses an eligible real arena alongside excluded Movement Course; independent full text/shadow crops at six sizes, real/Random cycling without RNG or plan commits, long width-only fit/short reset, retained/reopened/reversed copy/item font and live width/negative-shadow fitting on refresh. Lock/unlock, interrupted Enter and deliberate two-release Random start preserve eligibility and the pending music-exit plan. Title wordmark/subtitle/Start and Home subtitle retain independent linear rasters and complete Start-card bounds across source assignments, reopen and reversal with separate title/item/detail fonts missing an old glyph. Repeated Title/Home/Back and Mode return, Home focus movement and interrupted Enter rearming are checked at 720p; the connection hint remains hidden under the existing always-connected keyboard policy, with its full independently measured absence crop. Custom brand/heading/prompt/item/hint/font/layout, old-page snapshots, repeated prompt rebuilds, duration edits, focus and child return, reopen adoption, frozen redraw and draw RNG. Native crops and CSV retained. No visible controller-loss, physical-controller, human readability, performance or listening acceptance.'
    $result.actions += @('arena-status-copy', 'arena-status-captured-geometry')
    $result.checks += ' Both fixed arena status strings have independent complete text/shadow crops on real/Random browse/locked pages across six sizes. Unscaled detail-font status stays inside the full briefing before its hint row. Forced cycling/lock/unlock after source replacement, reopen and reversal retain captured text/font/position/shadow offset/shadow colour while refresh uses live ink; short custom copy was measured in the shipped font first. Provisional plan and RNG remain unchanged.'
    $result.actions += @('fixed-stage-briefing', 'connected-scripted-versus-briefing', 'briefing-captured-ink')
    $result.checks += ' Four fixed stage-briefing strings and independently specified adjacent mode/roster have complete text/shadow crops at six sizes for solo and connected scripted 2P single/best-of-three/best-of-five. Unscaled brand-font copy fits the design and briefing vertical span before authored following rows; the full solo objective overhang is retained. Captured copy/brand font/position/shadow offset/shadow colour/ink survive source replacement/reopen/reversal and cycling/lock/unlock, with unchanged provisional plan and RNG. The normal-reader full matrix runs the solo briefing subset and explicitly reports the scripted 2P contexts skipped; ResumeOnly retains the real backend.'
    $result.actions += @('fixed-random-navigation', 'fixed-stage-hints', 'hint-width-offset-refresh')
    $result.checks += ' Fixed Random navigation and browse/locked hints have independent full glyph/shadow crops across real/Random browse/locked states at six sizes, alongside unchanged numbered arenas and the eligible Random-count branch. Navigation remains unscaled with constructor-owned detail font/position/ink/shadow. Hints retain captured copy/brand font/position/ink/shadow colour while width budget and signed detail-shadow offset refresh on slot/state changes; long hints, short reset, separate old-glyph-missing fonts, cached fit, reopen/reversal and locked-arrow refusal are covered without changing the plan or RNG.'
    $result.actions += @('fixed-stage-mode-labels', 'stage-mode-detail-font-snapshot')
    $result.checks += ' The two fixed mode labels use independent fixture words for solo/single-match cases, while BEST OF 3/5 remain independent composed-text literals. Native mode crops remain unscaled in the actual detail font before roster row two across six sizes. Custom old-glyph copy, a separate detail font missing that glyph, constructor-owned position/shadow/ink and source replacement/reopen/reversal through cycling/lock/unlock preserve the connected roster, plan and RNG.'
}
elseif ($WeaponCatalogOnly) {
    $result.player_counts = @(1)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('sprayer', 'sniper', 'roller', 'mister', 'bucket', 'random')
    $result.actions = @('browse', 'wrap-left', 'wrap-right', 'confirm-release', 'stage-back', 'mode-back', 'reopen')
    $result.input_slots = @(4)
    $result.inputs = 'Public keyboard input through actual Weapon/Stage/Mode Select pages on the normal GamepadReader backend.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Six historical default names and icon/body/paint/full-supply bindings compared with independent fixtures at 720p/1080p/QHD, both wraps, provisional settings, fresh release confirmation, seeded concrete Random primary, actual Stage/Mode Back preserving all roster and match settings. Edited name/body/paint/supply and a sparse alternate atlas/fill become visible on reopen; existing pages keep their snapshots across edits, reversals and cycling. Frozen raster and draw RNG, native detail/context images and CSV retained. No physical-controller, human readability, performance or listening acceptance.'
}
elseif ($MainMenuOnly) {
    $result.player_counts = @()
    $result.palettes = @()
    $result.weapons = @()
    $result.actions = @('title-start', 'home-play', 'home-options', 'home-credits', 'home-exit', 'back')
    $result.input_slots = @(4)
    $result.inputs = 'Public keyboard input through actual MainMenuTitle/MainMenuHome and StateContext; Exit closes only this probe hidden window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Title/home paper, measured centered title text, heading/subtitle/build bounds, native ink/paper contrast and all four Home focus cards/arrows at 720p/1080p/QHD, 4:3, 16:10 and ultrawide. Independent historical multiline wordmark, Title subtitle/Start and Home subtitle rasters plus complete Start-card bounds use native linear sampling at all six sizes. The connection hint retains the existing always-connected keyboard hidden policy; its complete independently measured extent matches paper, without a visible controller-loss claim. Actual keyboard Start/Back/Play/Options/Credits/Exit, frozen redraw, draw RNG, static Reduce effects and resize round-trip. No physical-controller, human readability or listening acceptance.'
}
elseif ($ResultsMenuOnly) {
    $result.player_counts = @(1, 2, 4)
    $result.palettes = @('blue_orange', 'pink_green', 'blue_yellow')
    $result.weapons = @()
    $result.actions = @('fill', 'revealed', 'solo', 'draw', 'series', 'contributions', 'continue')
    $result.input_slots = @(4, 3, 0, 2)
    $result.inputs = 'Public keyboard input through the actual ResultsMenuInitial and StateContext; sparse pad slots are roster fixtures.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Results paper, centered lettering and score cards at native 720p/1080p/QHD and differing aspects; competitive/solo score bars and percentage rendering, absent teams, sparse-slot contributions, revealed and fill timing gates, draws and series. Frozen redraw, static Reduce effects, draw RNG, resize and deliberate continuation. No human readability or listening acceptance.'
}
elseif ($PauseMenuOnly) {
    $result.player_counts = @(1, 2, 4)
    $result.palettes = @()
    $result.weapons = @()
    $result.actions = @('resume', 'restart', 'controls-audio', 'quit', 'confirm-yes', 'confirm-no', 'missing-controllers', 'custom-labels', 'custom-questions', 'font-snapshot', 'focus-return', 'missing-seat-gate-clear', 'restart-missing-seat-cancel')
    $result.input_slots = @(4, 0, 1, 2, 3)
    $result.inputs = 'Public keyboard input through actual pause/confirmation pages and StateContext; absent pad slots are roster fixtures, with no physical pad claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Pause and confirmation paper plus six independent legacy label and two independent legacy question rasters at native 720p/1080p/QHD, 4:3, 16:10 and ultrawide; actual initial Resume, safe No, deliberate Restart/Quit and Controls/audio return. Questions use the unscaled actual detail font and full-paper centering; their complete measured bounds are compared over the stepped paper with production point filtering. Custom labels/questions and replacement item/detail fonts missing an old glyph; retained/reopened/reversed snapshots through focus movement, Controls return, No/Back and repeated typed actions. Focus interruption and synthetic missing-seat Restart cancellation/gate clearing require fresh input, alongside missing-participant refusal, enabled navigation, all-absent keyboard rescue, frozen redraw/RNG, Reduce effects and resize. No physical reconnection, human readability or listening acceptance.'
}
elseif ($EndMenuOnly) {
    $result.player_counts = @()
    $result.palettes = @()
    $result.weapons = @()
    $result.actions = @('rematch', 'change-teams', 'change-weapons', 'change-stage', 'exit', 'next-round', 'replay-draw', 'play-series-again', 'custom-labels', 'custom-headings', 'font-snapshot', 'pending-music-exit', 'focus-return')
    $result.input_slots = @(4)
    $result.inputs = 'Public keyboard input through the actual EndMenuInitial and StateContext; no physical pad claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'End-menu paper, primary continuation, four independent legacy action labels and three independent legacy headings at native 720p/1080p/QHD, 4:3, 16:10 and ultrawide; single match, tied/continuing/completed series, both Exit rows and hidden setup choices. Complete measured heading bounds use the unscaled actual heading font, full-paper centering and a stepped-paper reference with production point filtering. Custom labels/headings and replacement item/heading fonts missing an old glyph; retained/reopened/reversed snapshots through focus movement and repeated typed actions preserve the Session ledger. Owner-controlled pending music exits reject further input and preserve their original action through focus interruption; interrupted Enter releases require fresh selection. Frozen redraw/RNG, Reduce effects and resize remain covered. No physical controllers, human readability or listening acceptance.'
}
elseif ($ResumeOnly) {
    $result.player_counts = @(1, 2, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('sprayer')
    $result.actions = @('pause', 'resume', 'jump', 'fire', 'drop', 'death', 'focus', 'restart', 'quit')
    $result.input_slots = @(4, -1)
    $result.inputs = 'Public keyboard/mouse message input through actual GameLevel, StateContext and pause pages; other seats are idle dummies, with idle bots in Debug. No physical pad claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Actual 1P/2P/4P pause-page Resume freezes the world through count-in, nonpositive time, re-pause, focus loss/return and a long GO-crossing update. Public player health/ammo/cooldown/dead respawn, live projectile count, detailed cosmetic observation and underlying world/HUD raster remain fixed. Held jump/fire/drop remain suppressed until neutral and a deliberate new request; fresh pause interrupts the count. Actual keyboard-driven Restart clears the gate and Quit leaves immediately. Shared RESUMING paper is bounded and frozen at native 720p/1080p/QHD with exact resize round-trip. Muted hardware rendering; no human readability or listening acceptance.'
}
elseif ($HudOnly) {
    $result.player_counts = @(1, 2, 3, 4)
    $result.weapons = @('sprayer', 'sniper', 'mister', 'bucket')
    $result.actions = @()
    $result.input_slots = @(4, 3, 1, 0)
    $result.inputs = 'Direct HUD quantities and sparse keyboard/pad-slot labels; one real four-player builder-owned frozen match. No physical-device claim.'
    foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks', 'countdown_checks', 'closing_checks', 'supply_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.hud_checks
}
if (-not $HudOnly -and ($IdentityOnly -or $QuietOnly -or $MonitorOnly -or $CountdownOnly -or $ClosingOnly -or $SupplyOnly)) {
    $result.Remove('hud_checks')
}
if (-not $SupplyOnly -and ($IdentityOnly -or $QuietOnly -or $MonitorOnly -or $CountdownOnly -or $ClosingOnly -or $HudOnly)) {
    $result.Remove('supply_checks')
}
if ($result.Contains('countdown_checks')) {
    $result.Remove('stage')
    $result.stages = @('movement_course', 'close_quarters', 'king_of_the_hill', 'turbulence')
    $result.player_counts = @(1, 2, 3, 4)
}
try {
    if (-not $SkipBuild) {
        Write-Host "Configure and build $Preset; logs: $run"
        Invoke-Logged -Command 'cmake' -Arguments @('--preset', $Preset) `
            -LogPath (Join-Path $run 'configure.log')
        Invoke-Logged -Command 'cmake' -Arguments @('--build', '--preset', $Preset) `
            -LogPath (Join-Path $run 'build.log')
    }
    $cachePath = Join-Path $build 'CMakeCache.txt'
    if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf)) {
        throw "No configured build at $build. Run without -SkipBuild first."
    }
    $cache = @{}
    foreach ($line in Get-Content -LiteralPath $cachePath) {
        if ($line -match '^([^/#][^:]*):[^=]*=(.*)$') { $cache[$Matches[1]] = $Matches[2] }
    }
    foreach ($key in 'CMAKE_BUILD_TYPE', 'CMAKE_CXX_COMPILER', 'CMAKE_HOME_DIRECTORY',
        'LABRADOR_RENDER_BACKEND', 'LABRADOR_AUDIO_BACKEND', 'VCPKG_INSTALLED_DIR',
        'VCPKG_TARGET_TRIPLET') {
        if (-not $cache.ContainsKey($key)) { throw "Configured build is missing $key." }
    }
    $debug = $Preset -eq 'x64-debug'
    $configuration = if ($debug) { 'Debug' } else { 'Release' }
    if ([IO.Path]::GetFullPath($cache.CMAKE_HOME_DIRECTORY) -ne $repo -or
        $cache.CMAKE_BUILD_TYPE -ne $configuration -or
        $cache.LABRADOR_RENDER_BACKEND -ne 'd3d11' -or
        $cache.LABRADOR_AUDIO_BACKEND -ne 'xaudio2' -or
        $cache.VCPKG_TARGET_TRIPLET -ne 'x64-windows') {
        throw 'The probe requires this checkout and the selected d3d11/xaudio2 x64-windows preset.'
    }
    $gameDirectory = Join-Path $build 'game'
    $objectDirectory = Join-Path $gameDirectory 'CMakeFiles/ColourWars.dir'
    $objects = @(Get-ChildItem -LiteralPath $objectDirectory `
        -Filter '*.obj' -File -Recurse | Where-Object {
            if ($_.Name -eq 'main.cpp.obj') { return $false }
            if ($_.Name -eq 'cmake_pch.cxx.obj') { return $true }
            # CMake may leave an old object behind after its source is retired.
            $sourceRelative = $_.FullName.Substring($objectDirectory.Length + 1)
            $sourceRelative = $sourceRelative.Substring(0, $sourceRelative.Length - '.obj'.Length)
            Test-Path -LiteralPath (Join-Path (Join-Path $repo 'game') $sourceRelative) -PathType Leaf
        } | Sort-Object FullName)
    if ($objects.Count -eq 0) { throw 'Game objects are missing. Run without -SkipBuild first.' }
    $installed = Join-Path $cache.VCPKG_INSTALLED_DIR $cache.VCPKG_TARGET_TRIPLET
    $dependencyLibraries = if ($debug) { Join-Path $installed 'debug/lib' } else { Join-Path $installed 'lib' }
    $libraries = @(
        (Join-Path $build 'external/labrador/engine/LabradorEngine.lib'),
        (Join-Path $build 'external/labrador/engine/math/MattMath.lib'),
        (Join-Path $dependencyLibraries 'DirectXTK.lib'),
        (Join-Path $dependencyLibraries 'xaudio2_9redist.lib')
    )
    foreach ($path in @($cache.CMAKE_CXX_COMPILER) + $libraries +
        @((Join-Path $gameDirectory 'ColourWars.exe'), (Join-Path $gameDirectory 'presentation.json'))) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing build output: $path" }
    }
    $source = Join-Path $PSScriptRoot 'acceptance/painter_visual.cpp'
    $executable = Join-Path $run 'painter_visual.exe'
    $responseFile = Join-Path $run 'compile.rsp'
    $arguments = @('/nologo', '/EHsc', '/std:c++20', '/W4', '/WX', '/permissive-',
        '/DDIRECTX_TOOLKIT_IMPORT', '/DUSING_XAUDIO2_REDIST', '/DUSING_XINPUT',
        '/DUNICODE', '/D_UNICODE', '/external:W0')
    if ($debug) { $arguments += '/MDd' } else { $arguments += @('/MD', '/DNDEBUG') }
    if ($StageSummaryOnly -or $WeaponTuneColoursOnly -or $MenuLabelsOnly -or $TeamTuneColoursOnly -or $TeamFocusOnly -or $StickReadoutsOnly -or $ConnectionTextOnly -or $RememberedSetupOnly -or $TeamSetupOnly -or $MenuTextOnly -or $SetupMotionOnly -or $JoinStageMotionOnly -or $JoinPaperOnly -or $TeamPaperOnly -or $WeaponPaperOnly -or $StagePaperOnly -or $HomeReturnFocusOnly -or $HomeExitOnly -or $OptionsDefaultsOnly -or $DisplayTrialOnly -or $PausedOptionsOnly) { $arguments += '/DCOLOURWARS_PAINTER_SCRIPTED_PADS' }
    foreach ($include in @($repo, (Join-Path $repo 'external/labrador'), (Join-Path $build 'generated'))) {
        $arguments += '/I' + (Quote-Path $include)
    }
    foreach ($include in @((Join-Path $repo 'external/labrador/external/rapidjson/include'),
        (Join-Path $installed 'include/directxtk'),
        (Join-Path $installed 'include/xaudio2redist'), (Join-Path $installed 'include'))) {
        $arguments += '/external:I' + (Quote-Path $include)
    }
    $arguments += @((Quote-Path $source), ('/Fe:' + (Quote-Path $executable)),
        ('/Fo:' + (Quote-Path (Join-Path $run 'painter_visual.obj'))))
    $arguments += @($objects | ForEach-Object { Quote-Path $_.FullName })
    $arguments += @($libraries | ForEach-Object { Quote-Path $_ })
    $arguments += @('d3d11.lib', 'dxgi.lib', 'dxguid.lib', 'kernel32.lib', 'user32.lib',
        'gdi32.lib', 'winspool.lib', 'shell32.lib', 'ole32.lib', 'oleaut32.lib',
        'uuid.lib', 'comdlg32.lib', 'advapi32.lib')
    $arguments | Out-File -LiteralPath $responseFile -Encoding unicode
    Write-Host "Compile painter visual probe ($Preset)"
    Invoke-Logged -Command $cache.CMAKE_CXX_COMPILER -Arguments @("@$responseFile") `
        -LogPath (Join-Path $run 'compile.log')

    $result.staged_content_hashes = @(
        'manifest.json', 'tuning.json', 'presentation.json', 'levels/movement_course.json',
        'levels/close_quarters.json', 'levels/king_of_the_hill.json', 'levels/turbulence.json',
        'textures/painter.dds', 'textures/painter.json',
        'textures/weapon_paint.dds', 'textures/weapon_paint.json',
        'textures/terrain.dds', 'textures/terrain.json' |
        ForEach-Object {
            [ordered]@{ path = $_; sha256 = (Get-FileHash -LiteralPath (Join-Path $gameDirectory $_)).Hash }
        }
    )
    $env:PATH = "$gameDirectory;$previousPath"
    $env:LOCALAPPDATA = $profile
    $probeArguments = @((Quote-Path $gameDirectory), (Quote-Path $captures))
    if ($IdentityOnly) { $probeArguments += 'identity' }
    elseif ($QuietOnly) { $probeArguments += 'quiet' }
    elseif ($DriftOnly) { $probeArguments += 'drift' }
    elseif ($MiniatureTerrainOnly) { $probeArguments += 'miniature-terrain' }
    elseif ($LandmarkOnly) { $probeArguments += 'landmark' }
    elseif ($ParallaxOnly) { $probeArguments += 'parallax' }
    elseif ($TuneColoursOnly) { $probeArguments += 'tune-colours' }
    elseif ($FeedbackTuneColoursOnly) { $probeArguments += 'feedback-tune-colours' }
    elseif ($ExposedColourAlphaOnly) { $probeArguments += 'exposed-colour-alpha' }
    elseif ($DurationTextOnly) { $probeArguments += 'duration-text' }
    elseif ($ResolutionTextOnly) { $probeArguments += 'resolution-text' }
    elseif ($EndContinuationOnly) { $probeArguments += 'end-continuation' }
    elseif ($ResultsTextOnly) { $probeArguments += 'results-text' }
    elseif ($MonitorOnly) { $probeArguments += 'monitor' }
    elseif ($CountdownOnly) { $probeArguments += 'countdown' }
    elseif ($ClosingOnly) { $probeArguments += 'closing' }
    elseif ($SupplyOnly) { $probeArguments += 'supply' }
    elseif ($HudOnly) { $probeArguments += 'hud' }
    elseif ($ResumeOnly) { $probeArguments += 'resume' }
    elseif ($EndMenuOnly) { $probeArguments += 'end-menu' }
    elseif ($PauseMenuOnly) { $probeArguments += 'pause-menu' }
    elseif ($ResultsMenuOnly) { $probeArguments += 'results-menu' }
    elseif ($MainMenuOnly) { $probeArguments += 'main-menu' }
    elseif ($WeaponCatalogOnly) { $probeArguments += 'weapon-catalog' }
    elseif ($MenuTextOnly) { $probeArguments += 'menu-text' }
    elseif ($MenuMotionOnly) { $probeArguments += 'menu-motion' }
    elseif ($SetupMotionOnly) { $probeArguments += 'setup-motion' }
    elseif ($JoinStageMotionOnly) { $probeArguments += 'join-stage-motion' }
    elseif ($SetupPaperOnly) { $probeArguments += 'setup-paper' }
    elseif ($JoinPaperOnly) { $probeArguments += 'join-paper' }
    elseif ($TeamPaperOnly) { $probeArguments += 'team-paper' }
    elseif ($WeaponPaperOnly) { $probeArguments += 'weapon-paper' }
    elseif ($StagePaperOnly) { $probeArguments += 'stage-paper' }
    elseif ($HomeReturnFocusOnly) { $probeArguments += 'home-return-focus' }
    elseif ($HomeExitOnly) { $probeArguments += 'home-exit' }
    elseif ($OptionsDefaultsOnly) { $probeArguments += 'options-defaults' }
    elseif ($DisplayTrialOnly) { $probeArguments += 'display-trial' }
    elseif ($PausedOptionsOnly) { $probeArguments += 'paused-options' }
    elseif ($SettingsMenuOnly) { $probeArguments += 'settings-menu' }
    elseif ($ControlsMenuOnly) { $probeArguments += 'controls-menu' }
    elseif ($PercentageTextOnly) { $probeArguments += 'percentage-text' }
    elseif ($StickSettingsOnly) { $probeArguments += 'stick-settings' }
    elseif ($TeamSetupOnly) { $probeArguments += 'team-setup' }
    elseif ($RememberedSetupOnly) { $probeArguments += 'remembered-setup' }
    elseif ($ConnectionTextOnly) { $probeArguments += 'connection-text' }
    elseif ($StickReadoutsOnly) { $probeArguments += 'stick-readouts' }
    elseif ($TeamFocusOnly) { $probeArguments += 'team-focus' }
    elseif ($WeaponTuneColoursOnly) { $probeArguments += 'weapon-tune-colours' }
    elseif ($TeamTuneColoursOnly) { $probeArguments += 'team-tune-colours' }
    elseif ($MenuLabelsOnly) { $probeArguments += 'menu-labels' }
    elseif ($StageSummaryOnly) { $probeArguments += 'stage-summary' }
    elseif ($StageCountsOnly) { $probeArguments += 'stage-counts' }
    $process = Start-Process -FilePath $executable -WindowStyle Hidden -PassThru `
        -WorkingDirectory $working -ArgumentList $probeArguments `
        -RedirectStandardOutput (Join-Path $run 'run.log') `
        -RedirectStandardError (Join-Path $run 'run.err')
    $null = $process.Handle
    $env:PATH = $previousPath
    $env:LOCALAPPDATA = $previousLocalAppData
    $timer = [Diagnostics.Stopwatch]::StartNew()
    while (-not $process.WaitForExit(1000)) {
        if ($timer.Elapsed.TotalSeconds -gt 600) { throw 'Visual probe exceeded its 600-second runtime limit.' }
    }
    $process.WaitForExit()
    Get-Content -LiteralPath (Join-Path $run 'run.log') | ForEach-Object { Write-Host $_ }
    if ($process.ExitCode -ne 0) {
        Get-Content -LiteralPath (Join-Path $run 'run.err') | ForEach-Object { Write-Host $_ }
        throw "Visual probe exited with $($process.ExitCode)."
    }
    if ($RememberedSetupOnly) {
        # A new OS process reloads the writer's nested private save. The outer
        # aim_marker=false runner fixture remains immutable in both launches.
        $writerProcessId = $process.Id
        $process.Dispose()
        $process = $null
        $env:PATH = "$gameDirectory;$previousPath"
        $env:LOCALAPPDATA = $profile
        $readerArguments = @((Quote-Path $gameDirectory), (Quote-Path $captures), 'remembered-setup-read')
        try {
            $process = Start-Process -FilePath $executable -WindowStyle Hidden -PassThru `
                -WorkingDirectory $working -ArgumentList $readerArguments `
                -RedirectStandardOutput (Join-Path $run 'reader.log') `
                -RedirectStandardError (Join-Path $run 'reader.err')
            $null = $process.Handle
        }
        finally {
            $env:PATH = $previousPath
            $env:LOCALAPPDATA = $previousLocalAppData
        }
        $readerTimer = [Diagnostics.Stopwatch]::StartNew()
        while (-not $process.WaitForExit(1000)) {
            if ($readerTimer.Elapsed.TotalSeconds -gt 120) { throw 'Remembered setup reader exceeded its 120-second runtime limit.' }
        }
        $process.WaitForExit()
        Get-Content -LiteralPath (Join-Path $run 'reader.log') | ForEach-Object { Write-Host $_ }
        if ($process.ExitCode -ne 0) {
            Get-Content -LiteralPath (Join-Path $run 'reader.err') | ForEach-Object { Write-Host $_ }
            throw "Remembered setup reader exited with $($process.ExitCode)."
        }
        foreach ($line in Get-Content -LiteralPath (Join-Path $run 'reader.err')) {
            if ($line.Trim() -ne '' -and $line -notmatch '^no audio: .*The manifest marks this bank optional, so it plays nothing\.') {
                throw "Unexpected remembered setup reader diagnostic: $line"
            }
        }
        $result['process_relaunch'] = [ordered]@{
            outcome = 'passed'; writer_pid = $writerProcessId; reader_pid = $process.Id; reader_exit_code = $process.ExitCode
            writer_log = Join-Path $run 'run.log'; reader_log = Join-Path $run 'reader.log'
            profile = Join-Path $captures 'remembered-setup-profile'
        }
    }
    $expectedSaveFailures = @{}
    if ($PercentageTextOnly) {
        $line = 'save failure: Could not write ' + (Join-Path $captures 'percentage-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 1
    }
    if ($PausedOptionsOnly) {
        $line = 'save failure: Could not write ' + (Join-Path $captures 'paused-options-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 1
    }
    if ($DisplayTrialOnly) {
        $line = 'save failure: Could not write ' + (Join-Path $captures 'display-trial-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 1
    }
    if ($OptionsDefaultsOnly) {
        $line = 'save failure: Could not write ' + (Join-Path $captures 'options-defaults-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 6
    }
    if ($SettingsMenuOnly -or $result.scope -eq 'full-painter-matrix') {
        $line = 'save failure: Could not write ' + (Join-Path $captures 'settings-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 1
        $line = 'save failure: Could not write ' + (Join-Path $captures 'status-options-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 8
        $line = 'save failure: Could not write ' + (Join-Path $captures 'values-options-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 2
    }
    if ($ControlsMenuOnly -or $result.scope -eq 'full-painter-matrix') {
        $line = 'save failure: Could not write ' + (Join-Path $captures 'controls-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 1
        $line = 'save failure: Could not write ' + (Join-Path $captures 'status-controls-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 16
        $line = 'save failure: Could not write ' + (Join-Path $captures 'values-controls-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 2
    }
    if ($ControlsMenuOnly -or $StickSettingsOnly -or $result.scope -eq 'full-painter-matrix') {
        $line = 'save failure: Could not write ' + (Join-Path $captures 'stick-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 1
    }
    $expectedSaveTail = 'Your settings will not be remembered.'
    $expectedTailCount = [int]($expectedSaveFailures.Values | Measure-Object -Sum).Sum
    $saveTailCount = 0
    foreach ($line in Get-Content -LiteralPath (Join-Path $run 'run.err')) {
        if ($expectedSaveFailures.ContainsKey($line) -and $expectedSaveFailures[$line] -gt 0) {
            --$expectedSaveFailures[$line]
            continue
        }
        if ($line -eq $expectedSaveTail) { ++$saveTailCount; continue }
        if ($line.Trim() -ne '' -and
            $line -notmatch '^no audio: .*The manifest marks this bank optional, so it plays nothing\.') {
            throw "Unexpected probe diagnostic: $line"
        }
    }
    if (@($expectedSaveFailures.Values | Where-Object { $_ -ne 0 }).Count -ne 0 -or
        $saveTailCount -ne $expectedTailCount) {
        throw 'The controlled isolated settings-save failures did not produce exactly their expected diagnostics.'
    }
    if (-not (Test-Path -LiteralPath $saveFixture -PathType Leaf) -or
        (Get-FileHash -LiteralPath $saveFixture).Hash -ne $saveFixtureHash) {
        throw 'The visual probe changed its isolated aim_marker=false save fixture.'
    }
    if ($ResultsTextOnly) {
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
            'heading' = 'gill_sans_mt_bold_72'; 'contribution' = 'gill_sans_mt_bold_24'; 'proceed' = 'gill_sans_mt_bold_36'
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
    }
    if ($EndContinuationOnly) {
        $endContinuationSummary = Get-Content -LiteralPath (Join-Path $captures 'end-continuation-summary.json') -Raw | ConvertFrom-Json
        $endContinuationRows = @(Import-Csv -LiteralPath (Join-Path $captures 'end-continuation.csv'))
        $endContinuationSessions = @('single-solo', 'single-two-team', 'series3-complete-a',
            'series5-complete-b', 'series3-draw', 'series5-next-a')
        $endContinuationCapturedSessions = @('single-two-team', 'series3-complete-a', 'series3-draw', 'series5-next-a')
        $endContinuationExpectedPairs = @(
            foreach ($case in @('legacy-primary', 'legacy-secondary')) {
                foreach ($session in $endContinuationSessions) { "$case|$session" }
            }
            foreach ($case in @('old-primary', 'retained-old', 'reopened-new', 'retained-new')) {
                foreach ($session in $endContinuationCapturedSessions) { "$case|$session" }
            }
            'pending-music|single-two-team'
            'resized-1080|series5-next-a'
            'resized-720|series5-next-a'
        )
        $endContinuationExpectedColumns = @('case', 'session', 'width', 'height', 'focus', 'series', 'complete',
            'round', 'a_wins', 'b_wins', 'music_pending', 'expected_value', 'font', 'raw_width', 'raw_height',
            'scale', 'primary_persistent', 'full_ornament_match', 'frozen_session_setup_save_rng')
        if ($endContinuationSummary.outcome -ne 'passed' -or $endContinuationSummary.scope -ne 'end-continuation-only' -or
            $endContinuationSummary.updates -ne 68 -or $endContinuationSummary.readbacks -ne 94 -or
            $endContinuationSummary.csv_rows -ne 31 -or $endContinuationSummary.compact_crops -ne 6 -or
            $endContinuationSummary.page_openings -ne 14 -or $endContinuationSummary.font_negative_controls -ne 1 -or
            $endContinuationSummary.resize_updates -ne 0 -or $endContinuationSummary.save_calls -ne 1 -or
            $endContinuationSummary.crop_pixels -ne 311040 -or $endContinuationSummary.typed_actions -ne 14 -or
            $endContinuationSummary.restart_actions -ne 13 -or $endContinuationSummary.exit_actions -ne 1 -or
            $endContinuationSummary.deferred_actions -ne 4 -or $endContinuationSummary.music_starts -ne 4 -or
            $endContinuationRows.Count -ne 31 -or
            (($endContinuationRows[0].PSObject.Properties.Name) -join '|') -ne ($endContinuationExpectedColumns -join '|') -or
            (($endContinuationRows | ForEach-Object { "$($_.case)|$($_.session)" } | Sort-Object) -join ';') -ne
                (($endContinuationExpectedPairs | Sort-Object) -join ';')) {
            throw 'The End continuation leaf does not match its frozen summary, CSV schema, cases or action/update/readback budgets.'
        }
        $endContinuationSessionState = @{
            'single-solo' = @('false', 'true', '1', '0', '0')
            'single-two-team' = @('false', 'true', '1', '0', '1')
            'series3-complete-a' = @('true', 'true', '2', '2', '0')
            'series5-complete-b' = @('true', 'true', '3', '0', '3')
            'series3-draw' = @('true', 'false', '1', '0', '0')
            'series5-next-a' = @('true', 'false', '1', '1', '0')
        }
        $endContinuationLegacyValues = @{
            'single-solo' = 'Rematch'; 'single-two-team' = 'Rematch'
            'series3-complete-a' = 'Play Series Again'; 'series5-complete-b' = 'Play Series Again'
            'series3-draw' = 'Replay Draw'; 'series5-next-a' = 'Next Round'
        }
        $endContinuationOldValues = @{
            'single-two-team' = 'Rematch~ {1}'; 'series3-complete-a' = 'Again~ {3}'
            'series3-draw' = 'Draw~ {0}'; 'series5-next-a' = 'Next~ {2}'
        }
        $endContinuationNewValues = @{
            'single-two-team' = 'Replay {1}'; 'series3-complete-a' = 'Series {3}'
            'series3-draw' = 'Draw {0}'; 'series5-next-a' = 'Round {2}'
        }
        foreach ($row in $endContinuationRows) {
            $expectedWidth = if ($row.case -eq 'resized-1080') { '1920' } else { '1280' }
            $expectedHeight = if ($row.case -eq 'resized-1080') { '1080' } else { '720' }
            $expectedFocus = if ($row.case -eq 'legacy-secondary') { '1' } else { '0' }
            $expectedPending = if ($row.case -eq 'pending-music') { 'true' } else { 'false' }
            $newCopy = $row.case -in @('reopened-new', 'retained-new', 'resized-1080', 'resized-720')
            $expectedFont = if ($newCopy) { 'end_continuation_numeric_letters' } else { 'gill_sans_mt_bold_48' }
            $expectedValue = if ($newCopy) { $endContinuationNewValues[$row.session] }
                elseif ($row.case -in @('old-primary', 'retained-old', 'pending-music')) { $endContinuationOldValues[$row.session] }
                else { $endContinuationLegacyValues[$row.session] }
            $expectedState = $endContinuationSessionState[$row.session]
            if ($row.width -ne $expectedWidth -or $row.height -ne $expectedHeight -or
                $row.focus -ne $expectedFocus -or $row.series -ne $expectedState[0] -or
                $row.complete -ne $expectedState[1] -or $row.round -ne $expectedState[2] -or
                $row.a_wins -ne $expectedState[3] -or $row.b_wins -ne $expectedState[4] -or
                $row.music_pending -ne $expectedPending -or $row.expected_value -cne $expectedValue -or
                $row.font -ne $expectedFont -or $row.scale -ne '1' -or
                $row.primary_persistent -ne 'true' -or $row.full_ornament_match -ne 'true' -or
                $row.frozen_session_setup_save_rng -ne 'true') {
                throw "End continuation case $($row.case)/$($row.session) changed its literal copy, session ledger, capture or ornament contract."
            }
            foreach ($metric in @('raw_width', 'raw_height')) {
                $value = 0.0
                if (-not [double]::TryParse($row.$metric, [Globalization.NumberStyles]::Float,
                    [Globalization.CultureInfo]::InvariantCulture, [ref]$value) -or
                    [double]::IsNaN($value) -or [double]::IsInfinity($value) -or $value -le 0.0) {
                    throw "End continuation case $($row.case)/$($row.session) contains an invalid $metric."
                }
            }
        }
        $endContinuationExpectedCrops = @('end-continuation-rematch.ppm', 'end-continuation-series-again.ppm',
            'end-continuation-replay-draw.ppm', 'end-continuation-next-round.ppm',
            'end-continuation-old.ppm', 'end-continuation-new.ppm')
        $endContinuationCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'end-continuation-*.ppm' -File)
        $endContinuationExpectedNames = $endContinuationExpectedCrops + @('end-continuation.csv', 'end-continuation-summary.json')
        $endContinuationNames = @(Get-ChildItem -LiteralPath $captures -Filter 'end-continuation*' -File | Select-Object -ExpandProperty Name)
        if ($endContinuationCrops.Count -ne 6 -or
            (($endContinuationNames | Sort-Object) -join '|') -ne (($endContinuationExpectedNames | Sort-Object) -join '|')) {
            throw 'The End continuation leaf emitted unexpected or missing compact crops, CSV or summary files.'
        }
        foreach ($crop in $endContinuationCrops) {
            $header = @(Get-Content -LiteralPath $crop.FullName -TotalCount 3)
            $expectedHeader = "P6`n540 96`n255`n"
            $expectedBytes = [Text.Encoding]::ASCII.GetByteCount($expectedHeader) + 540 * 96 * 3
            if ($header.Count -ne 3 -or $header[0] -ne 'P6' -or $header[1] -ne '540 96' -or
                $header[2] -ne '255' -or $crop.Length -ne $expectedBytes) {
                throw "End continuation crop $($crop.Name) differs from its exact dimensions or RGB payload budget."
            }
        }
        $endContinuationSaveCalls = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -match 'Saving\.\.\.$' }).Count
        $endContinuationSaveSuccesses = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -eq 'Save was successful.' }).Count
        if ($endContinuationSaveCalls -ne 1 -or $endContinuationSaveSuccesses -ne 1) {
            throw 'The End continuation leaf must perform exactly its one successful private Play-settings seed Save.'
        }
        $result['observed_counts'] = [ordered]@{
            fixture = $endContinuationSummary
            updates = 68
            readbacks = 94
            csv_rows = 31
            crops = 6
            private_save_calls = $endContinuationSaveCalls
            successful_private_saves = $endContinuationSaveSuccesses
        }
    }
    if ($ResolutionTextOnly) {
        $resolutionSummary = Get-Content -LiteralPath (Join-Path $captures 'resolution-summary.json') -Raw | ConvertFrom-Json
        $resolutionRows = @(Import-Csv -LiteralPath (Join-Path $captures 'resolution-values.csv'))
        $resolutionExpectedCases = @('saved-windowed-preset', 'left-1280', 'left-wrap-3840', 'right-wrap-1280',
            'right-2560', 'pending-on-enabled', 'pending-on-cycle', 'old-width-height', 'retained-old-source',
            'old-short-height', 'reopened-new-width', 'retained-new-reversal', 'new-short-reset', 'new-long-refit',
            'new-short-reset-again', 'resized-1080', 'resized-720', 'restored-windowed', 'fullscreen-live-client',
            'pending-off-disabled', 'disabled-skip-back', 'fullscreen-retained-size', 'fullscreen-defaults-refresh',
            'fullscreen-reopened-client', 'metadata-windowed-preset')
        $resolutionExpectedValues = @('1920x1080', '1280x720', '3840x2160', '1280x720', '2560x1440',
            '2560x1440', '1920x1080', 'OLD~ display pixels: 1920x1080', 'OLD~ display pixels: 1920x1080',
            'OLD~ display pixels: 1280x720', 'Next display pixels: 1920x1080.', 'Next display pixels: 1920x1080.',
            'Next display pixels: 1280x720.', 'Next display pixels: 3840x2160.', 'Next display pixels: 1280x720.',
            'Next display pixels: 1280x720.', 'Next display pixels: 1280x720.', '1920x1080',
            'Next display pixels: 1600x900.', 'Next display pixels: 1600x900.', 'Next display pixels: 1600x900.',
            'Next display pixels: 1600x900.', 'Next display pixels: 1001x777.', '1001x777', '3840x2160')
        $resolutionExpectedColumns = @('case', 'width', 'height', 'focus', 'saved_preset', 'saved_fullscreen',
            'pending_fullscreen', 'enabled', 'expected_value', 'font', 'raw_width', 'raw_height', 'width_scale',
            'height_scale', 'scale', 'row_width', 'row_height', 'fit', 'full_row_match', 'ornament_inside', 'frozen_state_rng')
        if ($resolutionSummary.outcome -ne 'passed' -or $resolutionSummary.scope -ne 'resolution-text-only' -or
            $resolutionSummary.updates -ne 43 -or $resolutionSummary.readbacks -ne 76 -or
            $resolutionSummary.csv_rows -ne 25 -or $resolutionSummary.compact_crops -ne 6 -or
            $resolutionSummary.page_openings -ne 7 -or $resolutionSummary.font_negative_controls -ne 1 -or
            $resolutionSummary.resize_updates -ne 0 -or $resolutionSummary.save_calls -ne 3 -or
            $resolutionSummary.crop_pixels -ne 1050840 -or $resolutionSummary.metadata_only -ne $true -or
            $resolutionSummary.windowed_application -ne $true -or $resolutionRows.Count -ne 25 -or
            (($resolutionRows[0].PSObject.Properties.Name) -join '|') -ne ($resolutionExpectedColumns -join '|') -or
            (($resolutionRows.case | Sort-Object) -join '|') -ne (($resolutionExpectedCases | Sort-Object) -join '|')) {
            throw 'The resolution leaf does not match its frozen summary, CSV schema, cases or update/readback budgets.'
        }
        foreach ($row in $resolutionRows) {
            $caseIndex = [array]::IndexOf($resolutionExpectedCases, $row.case) + 1
            $expectedWidth = 1280
            $expectedHeight = 720
            if ($caseIndex -eq 16) { $expectedWidth = 1920; $expectedHeight = 1080 }
            elseif ($caseIndex -eq 19) { $expectedWidth = 1600; $expectedHeight = 900 }
            elseif ($caseIndex -ge 20) { $expectedWidth = 1001; $expectedHeight = 777 }
            $expectedFocus = if ($caseIndex -in @(19, 20, 22, 24)) { '1' }
                elseif ($caseIndex -eq 21) { '6' } elseif ($caseIndex -eq 23) { '4' } else { '0' }
            $expectedPreset = if ($caseIndex -ge 19) { '3840x2160' } else { '1920x1080' }
            $expectedSavedFullscreen = if ($caseIndex -ge 19 -and $caseIndex -le 24) { 'true' } else { 'false' }
            $expectedPendingFullscreen = if ($caseIndex -in @(6, 7, 19, 22, 24)) { 'true' } else { 'false' }
            $expectedEnabled = if ($expectedSavedFullscreen -eq 'true') { 'false' } else { 'true' }
            $expectedFont = if ($caseIndex -ge 8 -and $caseIndex -le 10) { 'resolution_tall_tilde' }
                elseif (($caseIndex -ge 11 -and $caseIndex -le 17) -or
                    ($caseIndex -ge 19 -and $caseIndex -le 23)) { 'resolution_numeric_shift' } else { 'gill_sans_mt_bold_48' }
            $expectedFit = if ($caseIndex -in @(8, 9)) { 'width_then_height' } elseif ($caseIndex -eq 10) { 'height' }
                elseif ($caseIndex -in @(11, 12, 14)) { 'width' }
                elseif ($caseIndex -in @(13, 15, 16, 17)) { 'reset' } else { 'none' }
            if ($row.width -ne [string]$expectedWidth -or $row.height -ne [string]$expectedHeight -or
                $row.focus -ne $expectedFocus -or $row.saved_preset -ne $expectedPreset -or
                $row.saved_fullscreen -ne $expectedSavedFullscreen -or $row.pending_fullscreen -ne $expectedPendingFullscreen -or
                $row.enabled -ne $expectedEnabled -or $row.font -ne $expectedFont -or $row.fit -ne $expectedFit -or
                $row.expected_value -ne $resolutionExpectedValues[$caseIndex - 1] -or
                $row.full_row_match -ne 'true' -or $row.ornament_inside -ne 'true' -or $row.frozen_state_rng -ne 'true' -or
                [string]::IsNullOrWhiteSpace($row.expected_value) -or $row.expected_value -match '[^\x20-\x7e]') {
                throw "Resolution case $($row.case) changed its fixed viewport/focus, capture, fullscreen-metadata or oracle contract."
            }
            foreach ($metric in @('raw_width', 'raw_height', 'width_scale', 'height_scale', 'scale', 'row_width', 'row_height')) {
                $value = 0.0
                if (-not [double]::TryParse($row.$metric, [Globalization.NumberStyles]::Float,
                    [Globalization.CultureInfo]::InvariantCulture, [ref]$value) -or
                    [double]::IsNaN($value) -or [double]::IsInfinity($value) -or $value -le 0.0 -or
                    ($metric -eq 'scale' -and $value -gt 1.0)) {
                    throw "Resolution case $($row.case) contains an invalid $metric."
                }
            }
        }
        $resolutionExpectedCrops = [ordered]@{
            'resolution-text-default.ppm' = @(1112, 144)
            'resolution-text-wrap.ppm' = @(1112, 144)
            'resolution-text-old-fit.ppm' = @(1112, 144)
            'resolution-text-new-width.ppm' = @(1112, 144)
            'resolution-text-new-reset.ppm' = @(1112, 144)
            'resolution-text-fullscreen.ppm' = @(1390, 180)
        }
        $resolutionCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'resolution-*.ppm' -File)
        $resolutionExpectedNames = @($resolutionExpectedCrops.Keys) + @('resolution-values.csv', 'resolution-summary.json')
        $resolutionNames = @(Get-ChildItem -LiteralPath $captures -Filter 'resolution-*' -File | Select-Object -ExpandProperty Name)
        if ($resolutionCrops.Count -ne 6 -or
            (($resolutionNames | Sort-Object) -join '|') -ne (($resolutionExpectedNames | Sort-Object) -join '|')) {
            throw 'The resolution leaf emitted unexpected or missing compact crops, CSV or summary files.'
        }
        foreach ($crop in $resolutionCrops) {
            $size = $resolutionExpectedCrops[$crop.Name]
            $header = @(Get-Content -LiteralPath $crop.FullName -TotalCount 3)
            $expectedHeader = "P6`n$($size[0]) $($size[1])`n255`n"
            $expectedBytes = [Text.Encoding]::ASCII.GetByteCount($expectedHeader) + $size[0] * $size[1] * 3
            if ($header.Count -ne 3 -or $header[0] -ne 'P6' -or $header[1] -ne "$($size[0]) $($size[1])" -or
                $header[2] -ne '255' -or $crop.Length -ne $expectedBytes) {
                throw "Resolution crop $($crop.Name) differs from its exact dimensions or RGB payload budget."
            }
        }
        $resolutionSaveCalls = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -match 'Saving\.\.\.$' }).Count
        $resolutionSaveSuccesses = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -eq 'Save was successful.' }).Count
        if ($resolutionSaveCalls -ne 3 -or $resolutionSaveSuccesses -ne 3) {
            throw 'The resolution leaf must perform exactly its three successful private metadata seed Saves.'
        }
        $result['observed_counts'] = [ordered]@{
            fixture = $resolutionSummary
            updates = 43
            readbacks = 76
            csv_rows = 25
            crops = 6
            private_save_calls = $resolutionSaveCalls
            successful_private_saves = $resolutionSaveSuccesses
        }
    }
    if ($DurationTextOnly) {
        $durationSummary = Get-Content -LiteralPath (Join-Path $captures 'duration-summary.json') -Raw | ConvertFrom-Json
        $durationRows = @(Import-Csv -LiteralPath (Join-Path $captures 'duration-values.csv'))
        $durationExpectedCases = @('default-240', 'cycle-60', 'cycle-120', 'explicit-240', 'left-120',
            'confirm-240', 'wrapped-default', 'fractional-90', 'fractional-61', 'scientific-off-choice',
            'off-choice-right', 'numeric-live-before', 'live-explicit-90', 'live-default-90', 'old-default',
            'retained-old-after-assignment', 'old-live-explicit', 'old-live-default', 'reopened-new-default',
            'retained-new-after-reversal', 'new-live-explicit', 'new-live-default', 'resized-1080',
            'resized-720', 'restored-default')
        if ($durationSummary.outcome -ne 'passed' -or $durationSummary.scope -ne 'duration-text-only' -or
            $durationSummary.updates -ne 86 -or $durationSummary.readbacks -ne 76 -or
            $durationSummary.csv_rows -ne 25 -or $durationSummary.compact_crops -ne 5 -or
            $durationSummary.page_openings -ne 8 -or $durationSummary.font_negative_controls -ne 1 -or
            $durationSummary.resize_updates -ne 0 -or $durationSummary.save_calls -ne 1 -or
            $durationSummary.crop_pixels -ne 467200 -or $durationRows.Count -ne 25 -or
            @($durationRows | Where-Object {
                $_.focus -ne '3' -or $_.scale -ne '1' -or $_.full_row_match -ne 'true' -or
                $_.ornament_inside -ne 'true' -or $_.frozen_state_rng -ne 'true' -or
                ($_.case -eq 'resized-1080' -and ($_.width -ne '1920' -or $_.height -ne '1080')) -or
                ($_.case -ne 'resized-1080' -and ($_.width -ne '1280' -or $_.height -ne '720'))
            }).Count -ne 0 -or
            (($durationRows.case | Sort-Object) -join '|') -ne (($durationExpectedCases | Sort-Object) -join '|')) {
            throw 'The duration leaf evidence does not match its frozen update, row, font-control and readback/crop budgets.'
        }
        foreach ($row in $durationRows) {
            foreach ($metric in @('value_width', 'value_height', 'row_width', 'row_height')) {
                $value = 0.0
                if (-not [double]::TryParse($row.$metric, [Globalization.NumberStyles]::Float,
                    [Globalization.CultureInfo]::InvariantCulture, [ref]$value) -or
                    [double]::IsNaN($value) -or [double]::IsInfinity($value) -or $value -le 0.0) {
                    throw "Duration case $($row.case) contains an invalid $metric."
                }
            }
        }
        $durationExpectedNames = @('duration-text-default.ppm', 'duration-text-fraction.ppm',
            'duration-text-scientific.ppm', 'duration-text-old-long.ppm', 'duration-text-new-long.ppm')
        $durationCrops = @(Get-ChildItem -LiteralPath $captures -Filter 'duration-*.ppm' -File)
        $durationCsvNames = @(Get-ChildItem -LiteralPath $captures -Filter 'duration-*.csv' -File | Select-Object -ExpandProperty Name)
        $durationSummaryNames = @(Get-ChildItem -LiteralPath $captures -Filter 'duration-*.json' -File | Select-Object -ExpandProperty Name)
        $durationExpectedSummaryNames = @('duration-summary.json')
        if ($debug) {
            $durationTargetSummary = Get-Content -LiteralPath (Join-Path $captures 'duration-target-summary.json') -Raw | ConvertFrom-Json
            if ($durationTargetSummary.outcome -ne 'passed' -or $durationTargetSummary.scope -ne 'duration-targets-only' -or
                $durationTargetSummary.targets -ne 2 -or $durationTargetSummary.legacy_candidate_acceptances -ne 2 -or
                $durationTargetSummary.paired_refusals -ne 2 -or $durationTargetSummary.primed_candidates -ne 2 -or
                $durationTargetSummary.target_refusals -ne 2 -or $durationTargetSummary.failed_apply_inert -ne 2 -or
                $durationTargetSummary.valid_retries -ne 2 -or $durationTargetSummary.consumed_candidates -ne 2 -or
                $durationTargetSummary.target_validate_calls -ne 6 -or $durationTargetSummary.candidate_writes -ne 6 -or
                $durationTargetSummary.state_updates -ne 0 -or $durationTargetSummary.readbacks -ne 0 -or
                $durationTargetSummary.save_calls -ne 0 -or $durationTargetSummary.real_content_unchanged -ne $true -or
                $durationTargetSummary.frozen_rng -ne $true) {
                throw 'The duration Debug target evidence does not match its frozen resource-refusal/retry contract.'
            }
            $durationExpectedSummaryNames += 'duration-target-summary.json'
        }
        elseif (@(Get-ChildItem -LiteralPath $captures -Filter 'duration-target-*').Count -ne 0) {
            throw 'Release duration evidence must not contain Debug target artifacts.'
        }
        if ($durationCrops.Count -ne 5 -or
            (($durationCrops.Name | Sort-Object) -join '|') -ne (($durationExpectedNames | Sort-Object) -join '|') -or
            ($durationCsvNames -join '|') -ne 'duration-values.csv' -or
            (($durationSummaryNames | Sort-Object) -join '|') -ne (($durationExpectedSummaryNames | Sort-Object) -join '|')) {
            throw 'The duration leaf emitted unexpected or missing compact crops, CSV or summary files.'
        }
        foreach ($crop in $durationCrops) {
            $header = @(Get-Content -LiteralPath $crop.FullName -TotalCount 3)
            $expectedBytes = [Text.Encoding]::ASCII.GetByteCount("P6`n1168 80`n255`n") + 1168 * 80 * 3
            if ($header.Count -ne 3 -or $header[0] -ne 'P6' -or $header[1] -ne '1168 80' -or
                $header[2] -ne '255' -or $crop.Length -ne $expectedBytes) {
                throw "Duration crop $($crop.Name) differs from its exact 1168x80 RGB payload budget."
            }
        }
        $durationSaveCalls = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -match 'Saving\.\.\.$' }).Count
        $durationSaveSuccesses = @(Get-Content -LiteralPath (Join-Path $run 'run.log') | Where-Object { $_ -eq 'Save was successful.' }).Count
        if ($durationSaveCalls -ne 1 -or $durationSaveSuccesses -ne 1) {
            throw 'The duration leaf must perform exactly its one successful private Play-settings seed Save.'
        }
        $result['observed_counts'] = [ordered]@{
            fixture = $durationSummary
            updates = 86
            readbacks = 76
            csv_rows = 25
            crops = 5
            private_save_calls = $durationSaveCalls
            successful_private_saves = $durationSaveSuccesses
        }
        if ($debug) { $result.observed_counts['target_validation'] = $durationTargetSummary }
    }
    if ($ExposedColourAlphaOnly) {
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
    }
    if ($FeedbackTuneColoursOnly) {
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
    }
    if ($PercentageTextOnly) {
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
    }
    Write-ContactSheet -CaptureDirectory $captures
    $result.outcome = 'passed'
    $identityDiagnostic = 'Player identity passed all-palette same-team overlap, scripted crossing, sparse/reordered slots, native P1-P4 glyph distinction, quiet/emphasis/reduced effects, own-pane world-only bounds, dead/respawn/story suppression and frozen state. Human playing-size review remains separate.'
    $stageEntryDiagnostic = 'Stage entry passed all four authored names, a three-player roster, bounded title fitting at narrow/wide aspects, opening/monitor skips, static reduced-effects information and hiding before countdown and at GO.'
    if ($ResultsTextOnly) {
        $result.diagnostic = 'Results text passed three independent literal actual-page labels and their paper context, captured copy/fonts, retained pages, reopening/reversal, unchanged generated series headings, actual reveal/proceed transitions and frozen Session/setup/private Save/RNG. Zero-update resize checks passed. Exactly 43 updates, 52 readbacks, 51 CSV rows across seventeen three-role observations, six page openings, six proceed pops, three font negative controls, eight compact crops (1428000 pixels) and one successful private seed Save. Both presets ran the same shared consumer scope without developer targets or commands. No broad menu matrix, match launch, foreground/global input, physical-device, listening, human readability or performance acceptance.'
    }
    elseif ($EndContinuationOnly) {
        $result.diagnostic = 'End continuation passed four independent literal primary labels, full primary ornaments and existing focus-arrow geometry across single/series and complete/drawn/next-round states. Captured copy/font assignment, retained pages, reopening/reversal, actual local typed restart/exit callbacks, deferred music completion, frozen Session/setup/private Save/RNG and zero-update resize checks passed. Exactly 68 updates, 94 readbacks, 31 CSV rows, fourteen page openings, one font negative control, six 540x96 crops (311040 pixels), fourteen typed actions (thirteen restart and one exit), four deferred actions, four music starts and one successful private Play-settings seed Save. Both presets ran the same shared consumer scope without developer targets or commands. No broad menu matrix, match launch, foreground/global input, physical-device, listening, human readability or performance acceptance.'
    }
    elseif ($ResolutionTextOnly) {
        $result.diagnostic = 'Resolution text passed independent literal actual Options rows, raw-font fitting, captured copy/font assignment, reopening and reversal, frozen state/draw RNG, private preferences and bounded resize checks. Saved fullscreen metadata retained its existing disabled/live-client row behavior while the application stayed windowed; no display Apply or trial ran. Exactly 43 updates, 76 readbacks, 25 CSV rows, seven page openings, one font negative control, six compact crops (1050840 pixels) and three successful private metadata seed Saves. Both presets ran the same shared consumer scope without developer targets or commands. No broad menu matrix, display Apply, match launch, foreground/global input, physical-device, listening, human readability or performance acceptance.'
    }
    elseif ($DurationTextOnly) {
        $result.diagnostic = 'Duration text passed independent literal Mode rows and complete compound ornaments through actual keyboard cycles, explicit/default choices, fractional/scientific defaults, captured copy and numeric font metrics, reopening and reversal. Frozen redraw/RNG, exact private preferences and zero-update 720/1080/720 resize checks passed. Exactly 86 updates, 76 readbacks, 25 CSV rows, eight page openings, one font negative control, five 1168x80 crops and one successful private Play-settings seed Save. No broad menu matrix, match launch, foreground/global input, physical-device, listening, human readability or performance acceptance.'
        if ($debug) {
            $result.diagnostic += ' Debug resource-only TuningTarget/PresentationTarget proof also passed two legacy-candidate acceptances, paired refusals, primed-target refusals, inert failed applies, valid retries and consumed candidates. Six target validations and six private candidate writes added zero updates, readbacks, Save calls or commands; actual content and draw RNG were unchanged.'
        }
    }
    elseif ($ExposedColourAlphaOnly) {
        $result.diagnostic = 'Exposed-colour alpha passed thirteen actual Home-background menu sites through default/RGBA128/alpha0 capture, retained pages, reopening and restoration over black/nonblack underlays. Independent literal-channel references, frozen state/draw RNG and same-mode private Display confirmation rollback passed. Movement/debug-text consumers passed independent raw-source references, opaque parity, straight-alpha/double-fade negative controls, captured-paint retention and frozen effect state/bounds/RNG. Shared evidence: 285 readbacks, 142 CSV rows, twelve exact compact crops, two successful private seed Saves and zero StateContext/gameplay updates.'
        if ($debug) {
            $result.diagnostic += ' Debug also passed frozen/live console text, panel shadows and performance text with eight validated private tune/reset commands, 104 readbacks, 48 CSV rows and four crops; no Save write commands. Debug totals: 389 readbacks, 190 CSV rows, sixteen crops.'
        }
        else {
            $result.diagnostic += ' Release ran shared-consumer-only evidence and emitted no Debug console artifacts.'
        }
        $result.diagnostic += ' No active match route, foreground/global input, physical-device, listening, human readability or performance acceptance.'
    }
    elseif ($FeedbackTuneColoursOnly) {
        $result.diagnostic = 'Feedback colours passed nine isolated empty/recovered/Sprayer muzzle/readiness cells, full/half life, black/nonblack backgrounds, independent raw-source premultiplied channels, opaque-default legacy parity, double-fade/straight-alpha negative controls, static Reduce Effects information, opaque team centres and frozen timers/bounds/RNG. Shared consumer evidence is56 readbacks,eight CSV rows,eight360x280 crops and eight parity/negative controls, with zero StateContext updates.'
        if ($debug) {
            $result.diagnostic += ' Debug also passed15 real validated named/RGB/RGBA/reset/save commands on preconstructed borrowed consumers, exact two-token private source/staged save and ledger rebase, unchanged actual content/tuning/preferences/private bytes and no queued match work. Exactly two successful private Save calls and no controlled save failure. Debug totals:122 readbacks,28 CSV rows,eight crops.'
        }
        else {
            $result.diagnostic += ' Release ran consumer-only evidence:56 readbacks,eight CSV rows,eight crops; no developer commands, transcript or nested save.'
        }
        $result.diagnostic += ' No gameplay route, foreground/global input, physical-device, listening, human readability or performance acceptance.'
    }
    elseif ($PercentageTextOnly) {
        $result.diagnostic = 'Preference percentages passed independent literal actual music/SFX/movement/aim rasters at720/1080, saved17/0/100, inherited movement rounded100 and explicit0/90; long OptionRow fitting, captured text and visibly distinct numeric font metrics through real adjustments, child return, reopen/reversal and zero-update resize returns. Child Apply/reload/repeat, active10/85 versus persisted5/85 after the controlled private-save failure, pending parent10/90 preservation and Escape discard retain unrelated preferences/private bytes. Current preview across source replacement and frozen draw RNG are unchanged. Exactly114 state updates(hard120),87 readbacks,52 value CSV rows,4 status comparisons and8 compact crops. No broad matrix, gameplay, foreground/global input, physical-pad, hardware qualification, listening, human readability or performance acceptance.'
    }
    elseif ($StageCountsOnly) {
        $result.diagnostic = 'Stage counts passed independent literal actual navigation/hint/status/record rasters at720/1080 for mixed4/eligible3, excluded1/eligible0 and mixed2/eligible1 lists; all mixed indices, unchanged singular grammar, observable Random browse/first lock/unlock/concrete return, positive record room and fitted long records or Random absence. Supplied-literal hint paper expansion, constructor text/font/position/ink/navigation-shadow capture, live refresh-only hint width/shadow, cached fit, reopen/reversal, exact private save/records/keyboard claim/settings and frozen/provisional RNG passed. Exactly104 updates(hard120),54 observed states,216 four-role CSV rows and8 compact crops. No final Random draw, match launch, broad matrix, foreground/global input, physical-pad, listening, human readability or performance acceptance.'
    }
    elseif ($StageSummaryOnly) {
        $result.diagnostic = 'Stage summaries passed independent literal mode/roster rasters for all1-4 sparse rosters, BEST OF3/5, solo-bestof5 precedence and Single Match at720/1080; captured detail/brand font IDs with visible numeric-bearing differences, clear full-shadow/paper bounds, old-page actual browse/Random-lock/unlock refresh, reopen adoption and reversal, unchanged plan/claims/private save/input/frozen and provisional RNG. Exactly112 updates,60 summary comparisons,64 independent literal navigation/status raster guards after32 taps,120 CSV rows and8 compact crops. No final Random draw, match launch, broad matrix, Arena counter/count slice, foreground/global input, physical-pad, listening, human readability or performance acceptance.'
    }
    elseif ($MenuLabelsOnly) {
        $result.diagnostic = 'Menu labels passed compact720/1080 actual Join/Team/Weapon/Pause/Results/Stick literal rasters, sparse player/device mappings, waiting/explicit claim/reconnect, preserved missing-pad packing/result truncation, captured copy and actual fonts through refresh/assignment/reopen/reversal, fitted Stick value and connection readout, save/settings/RNG and resize checks. Eight small crops and CSV retained. No broad matrix, full match/series, developer command, foreground/global input, physical-pad, listening, human readability or performance acceptance.'
    }
    elseif ($WeaponTuneColoursOnly) {
        $result.diagnostic = 'Weapon tune colours passed the dedicated Debug720p four validated tune/reset commands, actual sparse2P P1-ready/P2-unready Sprayer/Mister, frozen/inactive/missing retention and neutral reconnect adoption, independent premultiplied RGBA cards, unchanged opaque body/paint/supply and all outside-card pixels, exact same-page reset reversal, captured names/fonts/catalog/paper and roster/settings/input/palette/save/RNG preservation. Two compact crops, six-row CSV and private transcript retained. No Release native, broad matrix, full match, foreground/global input, physical-pad, listening, human readability or performance acceptance.'
    }
    elseif ($TeamTuneColoursOnly) {
        $result.diagnostic = 'Team tune colours passed the dedicated Debug720p actual validated commands for both Team tints: P1-ready/P2-unready independent icon rasters, frozen retention before update, active connected refresh adoption, exact same-page reset reversal, captured frames/fonts/paper and roster/settings/input/save/RNG preservation. Two compact crops, three-row CSV and private transcript retained. No Release native, broad TeamFocus/TuneColours matrix, full match, foreground/global input, physical-pad, listening, human readability or performance acceptance.'
    }
    elseif ($TeamFocusOnly) {
        $result.diagnostic = 'Team focus passed compact720/1080 independent semantic choice/readiness icon and captured text/font rasters, sparse1-4-seat rosters, nonwrap endpoints, ready-seat colour retention, local cursor motion, Confirm/Back/Shuffle priority, empty-team refusal, reconnect fresh press, exact Weapon commit/return, captured atlas/frame/text/font versus live colour refresh policy, nonzero-origin reversed physical order and frozen RNG/resize. Eight compact crops and concise CSV retained. No broad matrix, full match/series, foreground/global input, physical-pad, listening, human readability or performance acceptance.'
    }
    elseif ($StickReadoutsOnly) {
        $result.diagnostic = 'Stick readouts passed compact720/1080 independent literal/numeric rasters, all four slots, disconnected zeroing, signed half-percent rounding, movement/radial aim responses, pending inherited deadzone preview/discard without persistence, captured six templates/help font through refresh/assignment/reopen/reversal and frozen drawing/RNG. Eight compact crops total and concise CSV retained. Broad navigation/Apply/save, foreground/global input, physical-pad, listening, human readability and performance checks were not run.'
    }
    elseif ($ConnectionTextOnly) {
        $result.diagnostic = 'Connection text passed bounded720/1080 literal/bounds rasters, sparse Join states and explicit claims, fallback/all-four Pause numbering, interrupted Restart and reconnect fresh-press gates, Team warning, captured custom copy/fonts through refresh/assignment/reopen/reversal and frozen drawing/RNG. Eight compact crops total and concise CSV retained; no broad matrix, foreground/global input, physical-pad, listening, human readability or performance claim.'
    }
    elseif ($RememberedSetupOnly) {
        $result.diagnostic = 'Remembered setup passed actual normal-menu save timing, changed/cancelled choices, independent fresh-page rasters, sparse explicit claims, Pause Restart, two drawn series rounds and a separate hidden process reloading choices while requiring fresh claims/confirmation. Debug also passed actual developer-script/sparse-bot record/replay launches and normal-menu bot exclusion; Release excluded those developer branches. Seven compact crops and CSV/replay/transcript evidence remain in the private run. No foreground/global input or physical-pad/listening/human readability/performance claim.'
    }
    elseif ($TuneColoursOnly) {
        $result.diagnostic = 'Tune-colours-only Debug D3D11 hardware checks passed: seven audited named/RGB/RGBA commands reached existing live movement/ordinary DebugText/StationLabel consumers or captured-and-reopened Home/Mode as documented. Reset reversal, premultiplied and hidden-zero station tint, full resource rejection, queued scratch reload guards/rebuild/rebase, private save, original content, frozen state/RNG/fingerprint and compact evidence passed. No Release native, foreground input, physical-pad, listening, human readability or performance acceptance.'
    }
    elseif ($ParallaxOnly) {
        $result.diagnostic = 'Parallax comparison-only fixture passed32 clean/heavy current-arena gameplay-camera cases, ordinary Scene/factor0 parity, independent raw-atlas offsets and16 literal negative/fractional clipped entries, opaque static foreground/matching prop-reference samples, reduced static/cloud-anchor parity,4 actual StagePreviews and resize returns. Exactly372 readbacks and8 compact native static/A/B1P/4P context sheets retained. Zero far differences in natural gameplay panes are legitimate observations. Production backgrounds, preview/editor/monitor/overview, art, content, simulation and engine remain unchanged; no gameplay-route, human style/readability, listening, physical-device or performance acceptance.'
    }
    elseif ($LandmarkOnly) {
        $result.diagnostic = 'Static landmarks passed48 clean/heavy pairs /96 primary Scene states over all4 arenas,3 palettes,1P/4P overview pane layouts and720/1080; measured>99percent actual paint ownership, positive per-prop pixels, independent raw-atlas/order/culling equality and frozen state/RNG/resize.64 native edge cases,8 actual StagePreview size cases and24 actual Level monitor/overview size cases passed with omission-only scratch twins and frozen Debug fingerprints. Exactly8 compact native clean/heavy crops and bounded CSV receipts retained; reference/repeat render counts are separate. No broad older matrices, gameplay-route, human readability, physical-device, listening or performance acceptance.'
    }
    elseif ($MiniatureTerrainOnly) {
        $result.diagnostic = 'Miniature-terrain-only D3D11 hardware checks passed: independent exposed/masked edge, lit body, major ruler tick and ramp silhouette pixel oracles; actual owned paint overlay, exact native material/Scene parity and all four real 1P/4P arena overview/story contexts at720p/1080p/QHD. Legacy/adapted native crops, resize returns, frozen simulation/RNG and Debug fingerprints retained. Other painter matrices, human readability/style, physical pads, listening and performance acceptance were not run.'
    }
    elseif ($DriftOnly) {
        $result.diagnostic = 'Cloud-drift-only D3D11 hardware check passed: real cloud art/static reference, distinct loop/extreme rasters, 1P/4P 720p/1080p/QHD, both Scene entry edges against unculled drawings, projected bounds, immediate Reduce effects and retained phase, nonpositive/nonfinite time, frozen redraw and draw/update RNG, exact resize return. All four actual solo arena contexts and a 4P1080 Close Quarters sky composition with explicitly placed players retained simulation/Debug fingerprints; the driven composition establishes no gameplay-route or jump-reachability acceptance. Other painter matrices and human motion/readability, listening and performance acceptance were not run.'
    }
    elseif ($IdentityOnly) {
        $result.diagnostic = "Identity-only visual check passed. $identityDiagnostic Other painter matrices were not run."
    }
    elseif ($QuietOnly) {
        $result.diagnostic = 'Quiet-only visual check passed: sprayer quiet character matrix, sniper residual bead and bucket rim runoff; all 1P/4P720 palettes, continuous aim/flip attachment, representative 1080p/QHD, frozen redraw/state, raster bounds, per-tool enabled and reduced-effects suppression/restoration. Other painter matrices were not run.'
    }
    elseif ($MonitorOnly) {
        $result.diagnostic = 'Monitor-only visual check passed: six body shades, adult/mug and child/stickers; native 480p/768p/720p/1080p/QHD and pushed framing, all 26 ordered Muted Day atlas cells plus the empty slot and contained picker panel, foreground notes survive scene/preview, untinted picture/palette interiors, saved empty slot and closed desktop. Adult/child/mug seated/sigh/stand/leave art, full departure and offset-window translation passed; native pointer hotspot, local detached click ticks, expiry and reduced-effects suppression passed. Actual shipped full-drift 2P/4P story beats and solo practice retain frozen raster/persona/pose/state/fingerprint and draw RNG checks. A second otherwise-identical story removes only decorative drift in a verified private JSON copy; before/at/after winning-save and no-paint-close click comparisons retain all timing, containment and reduced-effects assertions, with compact CSV/crops and unchanged state/RNG/fingerprints. Other painter matrices were not run.'
    }
    elseif ($CountdownOnly) {
        $result.diagnostic = "Countdown-only visual check passed: distinct 0-9/GO glyphs, bounded accents and reduced/negative/expired ages at native 720p/1080p/QHD. Real keyboard solo and mixed-device 2P/4P full/quick/off openings, reminder geometry, pause, nonpositive-time updates, GO boundary, resize round-trip and frozen pixels/state/draw RNG/Debug fingerprint passed. $stageEntryDiagnostic Other painter matrices were not run."
    }
    elseif ($ClosingOnly) {
        $result.diagnostic = 'Closing-only visual check passed: actual last-ten clock beats/reduced/settled/zero at 1P/2P/4P720 and 4P1080/QHD, entire-card gating and short-clock suppression before live play; bounded native A/B result cards in every palette and reduced/negative/expired settled states; real A/B winner, no-paint draw and solo card gating. Frozen pixels/state/Debug fingerprint and draw RNG checks passed. Other painter matrices were not run.'
    }
    elseif ($SupplyOnly) {
        $result.diagnostic = 'Supply-only visual check passed: sprayer/mister full, half and empty native reservoirs; exact authored-layer registration, unchanged silhouette and permanent hardware, all 1P/4P720 palettes/aim quadrants/facing inputs/poses and representative 4P1080/QHD. Reduced effects, motion-disabled rendering, ammo clamps, paid depletion, natural regeneration/reset, menu previews and tool switching, frozen pixels/weapon state/draw RNG and unchanged match observation/Debug fingerprint passed. Other painter matrices were not run.'
    }
    elseif ($TeamSetupOnly) {
        $result.diagnostic = 'Team-setup-only hardware check passed: actual 2/3/4-player pages, seeded balanced teams and both three-player majority sides, all joined requesters, readiness reset/reconfirmation, independent X/R footer raster, settings-preserving Weapon return, manual edits, held/conflict/disconnect/focus gates, warning clear, solo no-op and native 720p/1080p four-row/footer bounds. One committed shuffled 2P roster preserved actual teams, input slots and weapons through Pause Restart and two best-of-three rounds via ordinary Results/End continuation; ongoing-series primary card matched its expected ledger. This binary used only the scripted GamepadReader link seam; normal painter binaries retain the real backend. No physical-controller, human readability or listening acceptance.'
    }
    elseif ($StickSettingsOnly) {
        $result.diagnostic = 'Stick-settings-only hardware check passed: actual child navigation, initial/pausing slot selection, clamps, pending cutoff previews, Apply/reload/failure, Back/Escape discard and suspended parent Apply preserving profiles. Full measured response/association copy, five bounded focus cards/arrows, native linear ink/paper contrast, frozen redraw, draw RNG, static Reduce effects and resize passed at 720p/1080p/QHD. Other painter matrices, physical controllers, human readability and listening acceptance were not run.'
    }
    elseif ($ControlsMenuOnly) {
        $result.diagnostic = 'Controls-menu-only hardware check passed: actual Controls/audio edits/clamps, delayed Apply, persisted reload and failed-save warning, Back discard, pending Options values/focus and actual GameLevel Pause return. Complete fitted help, measured labels/values/status, native linear ink, paper/cards/arrows/contrast, frozen redraw/state, draw RNG, static Reduce effects and resize passed across six sizes/aspects. Actual Pad sticks child passed at 720p/1080p/QHD: full response/association copy, focus, initial/pausing slot selection, clamps, pending cutoff previews, Apply/reload/failure, Back/Escape discard and suspended parent Apply preserving profiles. Thirteen independent historical labels across six sizes/aspects, long fitted options, full focus-card label/value unions, separate Controls item-font replacement, retained/reopened/reversed snapshots, repeated child return/Apply, interrupted Enter rearming and custom-label save/discard flows passed. Three independent static-help defaults; Controls/association width and multiline-height fits, unscaled Move-positioned aim caption across the full help column, a help font missing an old glyph, retained/reopened/reversed snapshots through value/preview refresh, old-parent/new-child return, repeated/interrupted Apply, reload and discard passed at 720p. Independent Controls/Sticks pending/saved/failure rasters at six sizes/aspects, authored width fitting with compact help, short-copy scale reset, missing-old-glyph fonts, retained/reopened/reversed callbacks, interrupted/repeated Apply, controlled failure/recovery, isolated reload, both discard routes and old-parent/new-child merge passed. Other painter matrices, physical controllers, human readability, preview audition and listening acceptance were not run.'
        $result.diagnostic += ' Controls On/Off defaults at six sizes/aspects, width/height fitting and refitting, complete focus unions, reopened short copy, a Controls font missing an old glyph, retained/reopened/reversed callbacks, Left/Right boolean assignments, Pad sticks child return/profile merge, interrupted/repeated Apply, failure/recovery, isolated reload and discard passed.'
    }
    elseif ($SettingsMenuOnly) {
        $result.diagnostic = 'Settings-menu-only hardware check passed: actual Options/Credits navigation, row edits, delayed Apply, persisted reload and failed-save warning, Controls/audio return, Back discard, disabled fullscreen resolution and retained windowed preset. Measured full copy/values/status, native paper/cards/arrows/contrast, frozen redraw, draw RNG, static Reduce effects and resize passed across six sizes/aspects. Independent Options pending/saved/failure rasters at six sizes/aspects, authored width fitting and short-copy scale reset, missing-old-glyph font, retained/reopened/reversed callbacks, interrupted/repeated Apply, controlled failure/recovery, isolated reload and both discard routes passed. Other painter matrices, physical controllers, human readability and listening acceptance were not run.'
        $result.diagnostic += ' Options On/Off/Random defaults at six sizes/aspects, width/height fitting, complete focus unions, Random-to-short-fixed-palette scale reset, an Options font missing an old glyph, retained/reopened/reversed callbacks, both fullscreen toggle directions/Random cycles, one Controls child On/Off pair and return, interrupted/repeated windowed-Off Apply, failure/recovery, isolated reload and discard passed.'
    }
    elseif ($PausedOptionsOnly) {
        $result.diagnostic = 'Paused-options-only native check passed: actual1/2/4 player painted matches retained across owned Options/Controls/Sticks/display children; independent complete six-aspect paused Options/caption/all-focus references, sparse owner/fallback/recovery/rearm, own Back and count-in separation, palette next-build behavior, private windowed trial rollback/failed Keep/recovery, profile merge, lifecycle/reduction/frozen draw RNG. No hardware fullscreen, global input, user-save, physical-pad or human acceptance.'
    }
    elseif ($DisplayTrialOnly) {
        $result.diagnostic = 'Display-trial-only native check passed: actual Options temporary private-window change with no pre-Keep save, independent complete trial paper/choices/countdown/footer across six native sizes/aspects, safe Revert/Keep, elapsed and wall deadlines, all lifecycle/destructor rollback paths, cover ownership and typed result, unchanged preferences/files on rollback, Keep reload/failure/recovery, keyboard/pad rearming/conflicts/priority, metadata fullscreen kept Off, reduction/nonpositive/frozen RNG. Whole-fixture guard; actual windowed Off only, no user-save/global-input/hardware-fullscreen/physical-pad or human acceptance.'
    }
    elseif ($OptionsDefaultsOnly) {
        $result.diagnostic = 'Options-defaults-only native check passed: independent complete seven-row Options across six sizes/aspects, staged/repeated restore without active/file/window changes, hidden windowed Apply/repeat/reload, six exact controlled failed writes and recovery, all unrelated play fields/four slot profiles preserved, both discard paths, Controls pending return and saved-preference merge, keyboard/pad rearming, fullscreen metadata without hardware fullscreen, reduction/lifecycle and frozen draw RNG. Scripted reader seam; no user-save, physical-pad, human readability, listening or performance acceptance.'
    }
    elseif ($HomeExitOnly) {
        $result.diagnostic = 'Home-exit-only native check passed: independent complete confirmation paper/question/answers/footer at six sizes/aspects; safe No/Back, exact retained Home Exit return/reopen, interruption/held reconnect/conflicts/simultaneous-device safety, reduction/lifecycle/developer reset and dismissal, resize and frozen pixels/empty plan/draw RNG. One pending music-exit begin latched outgoing pixels and suppressed repeated input until safe developer cancellation; final deliberate no-update/muted Yes closed only this hidden window. Scripted reader seam; no physical-pad, human readability, listening or performance acceptance.'
    }
    elseif ($HomeReturnFocusOnly) {
        $result.diagnostic = 'Home-return-focus-only native check passed: independent complete Home paper/cards/arrow/text/footer at six sizes/aspects; default/Title/Mode Play and both Options/Credits returns, repeated fresh-input exact child destinations, explicit initial-focus API, keyboard/sparse-pad held-action rearming, lifecycle/cover/developer resets, immediate reduction, nonpositive time, no phantom entry animation, resize and frozen pixels/empty plan/draw RNG. Scripted reader seam; no physical-pad, human readability, listening or performance acceptance.'
    }
    elseif ($StagePaperOnly) {
        $result.diagnostic = 'Stage-paper-only native check passed: independent outer/dynamic briefing paper and complete dark copy plus actual miniature StagePreview pane/camera at six sizes/aspects; all shipped arenas and eligible Random, solo/2P/4P modes/rosters, lock/unlock/refused arrows, exact real/Random start RNG and latched music exit, recovery/ordinary destinations, native local accent motion, reduction/lifecycle/interrupted rearming/resize, captured art/font/copy with explicit live ink/fit refresh, no-eligible Random and resource-valid navigation-overflow/text-padded preview-separation guards. Scripted reader seam; no physical-pad, human readability, listening or performance acceptance.'
    }
    elseif ($WeaponPaperOnly) {
        $result.diagnostic = 'Weapon-paper-only native check passed: independent outer/tile paper, unscaled dark lettering, original fixed tools/paint/final-supply masks and P1-P4 badge cells across six sizes/aspects and one-to-four sparse seats. Every choice, both wraps, all palettes, readiness/refusal, ordered Random RNG/commit, missing/reconnect and ordinary Mode/Team/Stage/Count routes; native local tile motion with fixed art/text/badges, reduction/lifecycle/resize, captured art/font/catalog/description/palette refresh/reopen/reversal including sparse fill, live icon readiness fill and name/description overflow guards passed. Scripted reader seam; no physical-pad, human readability, listening or performance acceptance.'
    }
    elseif ($TeamPaperOnly) {
        $result.diagnostic = 'Team-paper-only native check passed: independent outer paper, static per-player row cards, unscaled dark lettering, original icon frames/tints and P1-P4 badge cells across six sizes/aspects and one-to-four sparse seats. Choices/readiness/warning recovery, seeded shuffle/RNG, missing/reconnect and ordinary Count/Weapon routes; native icon motion with fixed paper/text/badges, reduction/lifecycle/resize, captured art/font/atlas refresh/reopen/reversal, live icon tint policy and warning-overflow guard passed. Scripted reader seam; no physical-pad, human readability, listening or performance acceptance.'
    }
    elseif ($JoinPaperOnly) {
        $result.diagnostic = 'Join-paper-only native check passed: independent complete paper/text/status/gutter rasters for one-to-four seats with keyboard/pad P1 across six sizes/aspects; neutral, ready, reserved reconnect and restored states, unscaled shadow-union geometry, containment/order/contrast, native acknowledgement progression, reduction/lifecycle, captured copy/font/art refresh/reopen/reversal, ordinary continuation/recovery destinations, interrupted-owner rearming, overflow rejection, frozen plan/pixels/RNG and resize. Scripted reader seam; no physical-pad, human readability, listening or performance acceptance.'
    }
    elseif ($SetupPaperOnly) {
        $result.diagnostic = 'Setup-paper-only native check passed: every Mode/Count focus and disabled-solo series at six sizes/aspects matched independent complete paper, card, arrow and fixed-text fixtures, measured containment and ink/paper contrast. Duration cycles, all mode/count/layout/keyboard claim and ordinary routes, copy/font snapshots, native arrow and duration press motion, fixed lettering, reduction/lifecycle reset, frozen redraw/RNG and resize passed. Normal reader, keyboard-driven only; no physical-pad, human readability, listening or performance acceptance.'
    }
    elseif ($JoinStageMotionOnly) {
        $result.diagnostic = 'Join/Stage-motion-only native check passed: actual Join row-gutter initial/middle/rest responses, stable holes, simultaneous and refused claims, final snapshot barrier and missing-P1 rescue; Stage briefing accent with fixed text/preview/edge/camera, real/Random wraps, lock/unlock and locked refusal, deliberate one-choice eligible Random start with its outgoing Random raster latched through pending fade, and single excluded-stage no-op. Native 720p/1080p/QHD, applied Reduce effects, nonpositive time, lifecycle/device/cover/developer resets, frozen pixels/plan/RNG and resize passed. Scripted reader seam; no physical-pad, human readability, listening or performance acceptance.'
    }
    elseif ($SetupMotionOnly) {
        $result.diagnostic = 'Setup-motion-only native check passed: real Mode/Count focus accent and duration press, independent Team selected-icon and Weapon tile motion at 720p/1080p/QHD; fixed weapon art/paint/supply, names/descriptions and badges, local envelopes, simultaneous/rapid input, readiness, shuffle, rejected actions, lifecycle resets, immediate applied Reduce effects, frozen redraw/RNG and resize. Scripted reader seam; no physical-pad, human readability, listening or performance acceptance.'
    }
    elseif ($MenuMotionOnly) {
        $result.diagnostic = 'Menu-motion-only hardware check passed: actual Home changed-focus arrows and Controls OptionRow paper initial/middle/rest, plus Apply pressed/rest native rasters at 720p/1080p/QHD, unchanged label/value lettering and local ornament bounds; first focus stays still, nonpositive time, rapid navigation, immediate applied Reduce effects, actual Controls Apply/reload/reversal, deactivation, nested Sticks return, frozen redraw/RNG and resize. Normal GamepadReader backend; other painter matrices, physical pads, human readability, listening and performance acceptance were not run.'
    }
    elseif ($MenuTextOnly) {
        $result.diagnostic = 'Menu-text-only hardware check passed: independent brand, 11 headings, 17 legends and eleven static item roles at 720p/1080p/QHD; eight Mode/Count labels and solo hint at six sizes/aspects; reachable Random-stage full text/shadow rasters, width-only long/short fitting, real/Random cycles without RNG or plan commits, captured copy/item font across replacement/reopen/reversal, live fit geometry and deliberate eligible Random start with pending music-exit plan. Actual arena lock/unlock, fitted Options labels, duration cycle/wrap/confirm, all modes/counts/layouts/keyboard claims, solo-disabled series, Join/Mode/Home Back and Credits return passed. Title/Home wordmark and fixed copy, complete Start card, separate title/item/detail fonts, retained/reopened/reversed snapshots, repeated Title/Home/Back, Mode return and interrupted Enter rearming passed at 720p. The no-controller hint remained hidden under the keyboard policy and matched its full independently measured paper crop. Custom text/font snapshots through prompt rebuilds, focus, duration and child return, frozen redraw and draw RNG passed. Other painter matrices, visible controller-loss, physical controllers, human readability, performance and listening acceptance were not run.'
        $result.diagnostic += ' Both arena status messages passed independent full-shadow raster checks across real/Random browse/locked states at six sizes, with captured detail font/position/shadow retained through forced refresh/reopen/reversal and live ink updates; provisional plan and RNG remained unchanged.'
        $result.diagnostic += ' Four fixed briefing strings plus adjacent mode/roster passed independent complete-shadow crops for solo and connected scripted 2P single/best-three/best-five at six sizes. Captured brand font/text/position/shadow/ink survived replacement/reopen/reversal and selection refresh, including the full solo overhang. This focused binary used the scripted reader seam, with neutral pads outside the briefing fixture; no physical pad acceptance.'
        $result.diagnostic += ' Fixed Random navigation and both fixed hints passed full-shadow rasters across real/Random browse/locked states at six sizes, including unchanged numbered arenas/Random count. Unscaled navigation, width-only hint fit/short reset, separate detail/brand font snapshots, cached versus live fit refresh/reopen/reversal and ignored locked arrows preserved the provisional plan and RNG.'
        $result.diagnostic += ' Solo/single-match mode labels passed independent fixed-copy and separate detail-font snapshot checks at six sizes, with captured unscaled mode position/shadow/ink retained through refresh/reopen/reversal; independently specified BEST OF 3/5, connected roster, plan and RNG remained unchanged.'
    }
    elseif ($WeaponCatalogOnly) {
        $result.diagnostic = 'Weapon-catalog-only hardware check passed: six historical labels and body/paint/full-supply rasters at 720p/1080p/QHD, both wraps, provisional browsing, fresh release confirmation, seeded Random primary, Stage/Mode Back preserving settings, custom layers and sparse alternate atlas/fill, reopen adoption and retained page snapshots, frozen redraw and draw RNG. Normal GamepadReader backend; other painter matrices, physical controllers, human readability, performance and listening acceptance were not run.'
    }
    elseif ($MainMenuOnly) {
        $result.diagnostic = 'Main-menu-only hardware check passed: actual keyboard Start/Back/Play/Options/Credits/Exit; measured text/paper bounds, independent historical Title wordmark/subtitle/Start and Home subtitle rasters, complete Start-card bounds, native linear text, ink/paper contrast, all four Home focus cards and detached arrows. The connection hint stayed hidden under the keyboard policy across its full independently measured crop. Frozen redraw, draw RNG, static Reduce effects and resize passed across six display sizes/aspects. Other painter matrices, visible controller-loss, physical controllers, human readability and listening acceptance were not run.'
    }
    elseif ($ResultsMenuOnly) {
        $result.diagnostic = 'Results-menu-only hardware check passed: actual results page timing and keyboard continuation, paper and score rendering, solo/absent-team and revealed/draw/series states, sparse-slot contributions, native display/aspect matrix, frozen redraw, Reduce effects and resize. Other painter matrices, human readability and listening acceptance were not run.'
    }
    elseif ($PauseMenuOnly) {
        $result.diagnostic = 'Pause-menu-only hardware check passed: six independent legacy labels and both independent legacy confirmation questions on actual Pause/Restart/Quit pages across six display sizes/aspects; unscaled detail-font question bounds/full-paper centering and production point filtering, custom labels/questions and item/detail fonts, retained/reopened/reversed snapshots, Controls return, safe No/Back, deliberate Resume/Restart/Quit, interrupted Enter focus and synthetic missing-seat Restart cancellation/gate clearing without automatic Resume. Existing missing-controller recovery routes, native paper/enabled focus cards, static Reduce effects, frozen redraw/RNG and resize passed. Other painter matrices, physical controllers/reconnection, human readability and listening acceptance were not run.'
    }
    elseif ($EndMenuOnly) {
        $result.diagnostic = 'End-menu-only hardware check passed: four independent historical action labels, continuation and three independent fixed headings on actual single/ongoing/draw/completed-series pages across six native display sizes/aspects; both Exit placements and hidden setup rows. Complete unscaled heading bounds/full-paper centering and production point filtering, custom labels/headings and item/heading fonts, retained/reopened/reversed snapshots, unchanged Session ledger, repeated typed actions, pending music-exit gating, focus interruption and held Enter rearming passed alongside native paper/focus rendering, frozen redraw/RNG, Reduce effects and resize. Other painter matrices, physical controllers, actual audio fade/listening and human readability acceptance were not run.'
    }
    elseif ($ResumeOnly) {
        $result.diagnostic = 'Resume-only actual GameLevel hardware check passed: 1P/2P/4P keyboard-driven pause/resume/re-pause, focus loss/return, full world/HUD raster and detailed player/projectile/cosmetic freeze, dead respawn, held-action rearming, long final count-in step, Restart and Quit. Native bounded RESUMING paper, repeated frozen drawing and exact 720p/1080p/QHD resize round-trip passed. Other painter matrices, physical controllers, human readability and listening acceptance were not run.'
    }
    elseif ($HudOnly) {
        $result.diagnostic = 'HUD-only visual check passed: exact full/half/empty and clamped health/paint tracks, exterior keylines, dark-text/pale-paper raster contrast, 1P/2P/3P/4P720, all three 1P/4P palettes and representative 1080p/QHD/4:3/portrait/ultrawide. Own-pane bounds, static Reduce effects information, frozen pixels/draw RNG, real-match observation/Debug fingerprint and resize round-trip passed. Existing final-ten clock matrix passed. Other painter matrices and human readability acceptance were not run.'
    }
    else {
        $result.diagnostic = "Painter base/action, five-tool mechanism, weapon paid-shot/aim-follow and controlled muzzle/empty/recovered on/off, step/skid contact, and actual idle drip/quiet character matrices passed. Gameplay HUD full/half/empty and clamped tracks, independent health/paint, native text contrast, pane bounds, palettes/aspects, static Reduce effects, frozen real-match state and exact resize round-trip passed. Aim readiness markers passed actual ready/recovering/empty, all palette/quadrant own-pane on/off bounds, isolated marker raster, frozen redraw and pause checks; the disabled isolated-save fixture remained unchanged. Local view shake preserved other panes, opaque HUD ink, base camera/aim, frozen redraw and simulation fingerprint, including suspension and resume. Distinct pinned body/tool and quiet on/off/blink rasters, frozen motion/feedback/quiet timers/redraw, separate player/drip raster culling bounds, elimination/redraw/respawn and idle whole-story poses passed. $identityDiagnostic $stageEntryDiagnostic"
    }
}
catch {
    $failure = $_
    $result.diagnostic = $_.Exception.Message
}
finally {
    $env:PATH = $previousPath
    $env:LOCALAPPDATA = $previousLocalAppData
    if ($null -ne $process) {
        if (-not $process.HasExited) {
            # Only this isolated probe; it owns no player save or session.
            $process.Kill()
            $null = $process.WaitForExit(3000)
        }
        $process.Dispose()
    }
    $result | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $run 'result.json') -Encoding utf8
    Write-Host "Painter evidence: $run"
    Pop-Location
}
if ($null -ne $failure) { throw $failure }
