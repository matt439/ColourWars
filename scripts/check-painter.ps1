# Painter visual probe runner. Helpers live beside it in check-painter/ and are
# dot-sourced into this script's scope. A focused leaf registers its param,
# native argument below, check-painter/evidence-record.ps1 entry, guarded
# check-painter/verify-<leaf>.ps1 dot-source (if it has post-run evidence),
# check-painter/diagnostics.ps1 entry and contact-sheet selector for any new
# capture family. The one-leaf guard discovers bound *Only switches automatically;
# CompileOnly is the one *Only switch that is not a focused matrix (CI#2).
[CmdletBinding()]
param(
    [ValidateSet('x64-debug', 'x64-release')]
    [string] $Preset = 'x64-debug',
    [switch] $SkipBuild,
    # CI#2: compile the probe with both pad readers, then stop. No probe runs.
    [switch] $CompileOnly,
    [switch] $OmitWorktreeStatus,
    [switch] $IdentityOnly,
    [switch] $QuietOnly,
    [switch] $DriftOnly,
    [switch] $MiniatureTerrainOnly,
    [switch] $CqBlockoutOnly,
    [switch] $CrossfeedOnly,
    [switch] $PressTowerOnly,
    [switch] $TurbulenceBlockoutOnly,
    [switch] $TurbulenceMoverOnly,
    [switch] $TurbulenceShoulderOnly,
    [switch] $TurbulenceSlabOnly,
    [string] $TrialPath,
    [switch] $GatePermissionOnly,
    [switch] $FerryDirectionOnly,
    [switch] $SurfaceFinishOnly,
    [switch] $RouteWarningOnly,
    [switch] $LandmarkOnly,
    [switch] $ParallaxOnly,
    [switch] $TuneColoursOnly,
    [switch] $FeedbackTuneColoursOnly,
    [switch] $CombatTuneColoursOnly,
    [switch] $BlockedShotTuneColoursOnly,
    [switch] $IdleDripTuneColoursOnly,
    [switch] $RespawnPenTuneColoursOnly,
    [switch] $CursorTuneColoursOnly,
    [switch] $SilhouetteTuneColoursOnly,
    [switch] $RevealTuneColoursOnly,
    [switch] $CursorMaterialTuneColoursOnly,
    [switch] $DesktopTuneColourOnly,
    [switch] $PowerLightTuneColourOnly,
    [switch] $ProgramTuneColourOnly,
    [switch] $ProgramLevelVisibilityOnly,
    [switch] $ActingTuneColoursOnly,
    [switch] $GlassTuneColourOnly,
    [switch] $SilhouetteBodyTuneColourOnly,
    [switch] $ScribbleTuneColourOnly,
    [switch] $HudHealthTuneColourOnly,
    [switch] $HudTrackTuneColourOnly,
    [switch] $HudTimerTuneColourOnly,
    [switch] $HudClosingMaterialsOnly,
    [switch] $HudClosingShadeOnly,
    [switch] $HudPaperTuneColoursOnly,
    [switch] $HudLabelTuneColoursOnly,
    [switch] $StagePreviewBackgroundTuneColourOnly,
    [switch] $CrackTuneColourOnly,
    [switch] $LimbTuneColoursOnly,
    [switch] $ExposedColourAlphaOnly,
    [switch] $DurationTextOnly,
    [switch] $ResolutionTextOnly,
    [switch] $EndContinuationOnly,
    [switch] $ResultsTextOnly,
    [switch] $SessionResultOnly,
    [switch] $SeriesScoreOnly,
    [switch] $RoundHeadingOnly,
    [switch] $CompetitiveWinnerOnly,
    [switch] $ContributionTeamOnly,
    [switch] $ResultPercentageOnly,
    [switch] $MonitorOnly,
    [switch] $CountdownOnly,
    [switch] $ClosingOnly,
    [switch] $SupplyOnly,
    [switch] $HudOnly,
    [switch] $ResumeOnly,
    [switch] $PracticeReplayOnly,
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
    [switch] $KeyboardJumpOnly,
    [switch] $RumbleOnly,
    [switch] $PercentageTextOnly,
    [switch] $StickSettingsOnly,
    [switch] $TeamSetupOnly,
    [switch] $RememberedSetupOnly,
    [switch] $BotPlayersOnly,
    [switch] $ConnectionTextOnly,
    [switch] $StickReadoutsOnly,
    [switch] $TeamFocusOnly,
    [switch] $TeamTuneColoursOnly,
    [switch] $WeaponTuneColoursOnly,
    [switch] $MenuLabelsOnly,
    [switch] $StageSummaryOnly,
    [switch] $StageCountsOnly,
    [switch] $StageScanOnly,
    [string] $OutputDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
# A caller may pass another PowerShell edition's modules through PSModulePath.
# Load this host's standard file-hashing command from its own installation.
Import-Module (Join-Path $PSHOME 'Modules/Microsoft.PowerShell.Utility/Microsoft.PowerShell.Utility.psd1') -ErrorAction Stop
. (Join-Path $PSScriptRoot 'check-path.ps1')
. (Join-Path $PSScriptRoot 'probe-build.ps1')
$trialResolved = $null
$trialHashBefore = $null
$trialHashAfterAttempted = $false
$trialMode = $TurbulenceMoverOnly -or $TurbulenceShoulderOnly -or $TurbulenceSlabOnly
if ($PSBoundParameters.ContainsKey('TrialPath') -and -not $trialMode) {
    throw 'TrialPath is supported only with TurbulenceMoverOnly, TurbulenceShoulderOnly or TurbulenceSlabOnly.'
}
if ($trialMode) {
    $trialModeName = if ($TurbulenceSlabOnly) { 'TurbulenceSlabOnly' } elseif ($TurbulenceShoulderOnly) { 'TurbulenceShoulderOnly' } else { 'TurbulenceMoverOnly' }
    if ([string]::IsNullOrWhiteSpace($TrialPath)) { throw "$trialModeName requires an explicit TrialPath." }
    $trialResolved = Resolve-CheckFileSystemPath $TrialPath
    if (-not (Test-Path -LiteralPath $trialResolved -PathType Leaf)) { throw "TrialPath is not a readable existing file: $trialResolved" }
    $trialHashBefore = (Get-FileHash -LiteralPath $trialResolved -Algorithm SHA256).Hash
}
if ($TuneColoursOnly -and $Preset -ne 'x64-debug') {
    throw 'TuneColoursOnly requires x64-debug; Release developer-tool exclusion is checked by CPU acceptance.'
}
if ($TeamTuneColoursOnly -and $Preset -ne 'x64-debug') {
    throw 'TeamTuneColoursOnly requires x64-debug; Release developer-tool exclusion is checked by CPU acceptance.'
}
if ($WeaponTuneColoursOnly -and $Preset -ne 'x64-debug') {
    throw 'WeaponTuneColoursOnly requires x64-debug; Release developer-tool exclusion is checked by CPU acceptance.'
}
$focusedSelectors = @(
    $PSBoundParameters.GetEnumerator() | Where-Object {
        $_.Key.EndsWith('Only', [StringComparison]::OrdinalIgnoreCase) -and
        $_.Key -ine 'CompileOnly' -and
        $_.Value -is [System.Management.Automation.SwitchParameter] -and
        $_.Value.IsPresent
    } | ForEach-Object { $_.Key }
)
if ($focusedSelectors.Count -gt 1) {
    throw 'Choose only one focused painter matrix.'
}
if ($CompileOnly -and $focusedSelectors.Count -gt 0) {
    throw 'CompileOnly compiles both pad readers and takes no focused painter selector.'
}
# Each entry is one compile: $true selects the scripted gamepad reader.
$painterCompileVariants = if ($CompileOnly) { @($false, $true) } else {
    @([bool]($RumbleOnly -or $KeyboardJumpOnly -or $StageSummaryOnly -or $WeaponTuneColoursOnly -or $MenuLabelsOnly -or $TeamTuneColoursOnly -or $TeamFocusOnly -or $StickReadoutsOnly -or $ConnectionTextOnly -or $RememberedSetupOnly -or $BotPlayersOnly -or $TeamSetupOnly -or $MenuTextOnly -or $SetupMotionOnly -or $JoinStageMotionOnly -or $JoinPaperOnly -or $TeamPaperOnly -or $WeaponPaperOnly -or $StagePaperOnly -or $StagePreviewBackgroundTuneColourOnly -or $HomeReturnFocusOnly -or $HomeExitOnly -or $OptionsDefaultsOnly -or $DisplayTrialOnly -or $PausedOptionsOnly))
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

$probeArguments = @()
if ($focusedSelectors.Count -eq 1) {
    $selector = $focusedSelectors[0] -creplace 'Only$', ''
    $probeArguments += (($selector -creplace '([a-z0-9])([A-Z])', '$1-$2').ToLowerInvariant())
    if ($trialMode) { $probeArguments += Quote-Path $trialResolved }
}

. (Join-Path $PSScriptRoot 'check-painter/hash.ps1')
. (Join-Path $PSScriptRoot 'check-painter/evidence.ps1')
. (Join-Path $PSScriptRoot 'check-painter/contact-sheet.ps1')

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$build = Join-Path $repo "out/build/$Preset"
$outputRoot = Resolve-CheckFileSystemPath $OutputDirectory
$run = Join-Path (Join-Path $outputRoot $Preset) ([Guid]::NewGuid().ToString('N'))
$captures = Join-Path $run 'captures'
$working = Join-Path $run 'working-directory'
$privateProfile = Join-Path $run 'profile'
$null = New-Item -ItemType Directory -Path $run, $captures, $working, $privateProfile -Force
$saveDirectory = Join-Path $privateProfile 'ColourWars'
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

# Loaded after the tool preflight: the selection checker's fault copy stops there
# and mirrors only the helpers needed to reach it.
. (Join-Path $PSScriptRoot 'quiet-game.ps1')
. (Join-Path $PSScriptRoot 'check-diagnostics.ps1')
Push-Location -LiteralPath $repo
$previousPath = $env:PATH
$previousLocalAppData = $env:LOCALAPPDATA
$process = $null
$failure = $null
try {
    . (Join-Path $PSScriptRoot 'check-painter/evidence-record.ps1')
    if ($CompileOnly) {
        # CI#2: a compile-only receipt, passed or failed, keeps the run's
        # identity but none of the full matrix's scope or check descriptions.
        $compileRecord = [ordered]@{ outcome = 'failed'; scope = 'compile-only' }
        foreach ($key in 'preset', 'game_revision', 'engine_revision', 'worktree_status',
            'worktree_status_omitted', 'profile') { $compileRecord[$key] = $result[$key] }
        $compileRecord.compiled_variants = @()
        $compileRecord.diagnostic = ''
        $result = $compileRecord
    }
    if ($RumbleOnly -and $Preset -eq 'x64-debug') {
        $result.background_supplement = 'Debug-only actual GameLevel script.background checks: native private-HWND facts; synthetic StateContext activation and injected hard gates; neutral physical actions, truthful presence, script/bot/replay, Music/SFX group mute before shots, recording motors and unchanged private Save. No physical pad, listening, real OS focus/power-sleep, CI#1 or performance acceptance.'
    }
}
catch {
    Pop-Location
    throw
}
$previousQuietGame = Enter-QuietGame
try {
    if (-not $SkipBuild) {
        Write-Host "Configure and build $Preset; logs: $run"
        Invoke-Logged -Command 'cmake' -Arguments @('--preset', $Preset) `
            -LogPath (Join-Path $run 'configure.log')
        Invoke-Logged -Command 'cmake' -Arguments @('--build', '--preset', $Preset, '--parallel', '1') `
            -LogPath (Join-Path $run 'build.log')
    }
    # Every painter mode supplies a fake rumble factory; never link the motor adapter.
    $probeBuild = Get-ProbeBuild -Repo $repo -Preset $Preset -Adapters @('display_awake') -RequiredContent @('presentation.json')
    $cache = $probeBuild.Cache
    $debug = $probeBuild.Debug
    $gameDirectory = $probeBuild.GameDirectory
    $installed = $probeBuild.Installed
    $libraries = $probeBuild.Libraries
    $source = Join-Path $PSScriptRoot 'acceptance/painter_visual.cpp'
    $compiledVariants = @()
    foreach ($scriptedPads in $painterCompileVariants) {
        # CompileOnly keeps each reader's outputs apart; a probe run keeps its names.
        $variantSuffix = if (-not $CompileOnly) { '' } elseif ($scriptedPads) { '-scripted-pads' } else { '-normal-pads' }
        $executable = Join-Path $run "painter_visual$variantSuffix.exe"
        $responseFile = Join-Path $run "compile$variantSuffix.rsp"
        $arguments = @($probeBuild.CompileArguments)
        if ($scriptedPads) { $arguments += '/DCOLOURWARS_PAINTER_SCRIPTED_PADS' }
        if ($StagePreviewBackgroundTuneColourOnly -and ($arguments -notcontains '/DCOLOURWARS_PAINTER_SCRIPTED_PADS')) {
            throw 'StagePreviewBackgroundTuneColourOnly requires the scripted gamepad reader.'
        }
        if ($HudLabelTuneColoursOnly -and ($arguments -contains '/DCOLOURWARS_PAINTER_SCRIPTED_PADS')) {
            throw 'HudLabelTuneColoursOnly requires the normal gamepad reader.'
        }
        if ($HudClosingMaterialsOnly -and ($arguments -contains '/DCOLOURWARS_PAINTER_SCRIPTED_PADS')) {
            throw 'HudClosingMaterialsOnly requires the normal gamepad reader.'
        }
        if ($HudClosingShadeOnly -and ($arguments -contains '/DCOLOURWARS_PAINTER_SCRIPTED_PADS')) {
            throw 'HudClosingShadeOnly requires the normal gamepad reader.'
        }
        if ($HudPaperTuneColoursOnly -and ($arguments -contains '/DCOLOURWARS_PAINTER_SCRIPTED_PADS')) {
            throw 'HudPaperTuneColoursOnly requires the normal gamepad reader.'
        }
        $arguments += @((Quote-Path $source), ('/Fe:' + (Quote-Path $executable)),
            ('/Fo:' + (Quote-Path (Join-Path $run "painter_visual$variantSuffix.obj"))))
        $arguments += @($libraries | ForEach-Object { Quote-Path $_ })
        $arguments += $probeBuild.WindowsLibraries
        $arguments | Out-File -LiteralPath $responseFile -Encoding unicode
        $reader = if ($scriptedPads) { 'scripted pads' } else { 'normal pads' }
        Write-Host "Compile painter visual probe ($Preset, $reader)"
        Invoke-Logged -Command $cache.CMAKE_CXX_COMPILER -Arguments @("@$responseFile") `
            -LogPath (Join-Path $run "compile$variantSuffix.log")
        $compiledVariants += [ordered]@{
            reader = $reader
            executable = $executable
            executable_sha256 = (Get-FileHash -LiteralPath $executable).Hash
        }
        # A failed second reader still records the first one it compiled.
        if ($CompileOnly) { $result.compiled_variants = $compiledVariants }
    }
    if ($CompileOnly) {
        # Return runs the finally below: it writes result.json and restores state.
        $result.outcome = 'compiled'
        $result.diagnostic = 'Probe compiled with both pad readers; no probe launched, no capture or GPU claim.'
        return
    }

    $result.staged_content_hashes = @(
        'manifest.json', 'tuning.json', 'presentation.json', 'levels/movement_course.json',
        'levels/close_quarters.json', 'levels/king_of_the_hill.json', 'levels/turbulence.json', 'levels/crossfeed.json',
        'levels/press_tower.json', 'levels/turbulence_blockout.json',
        'textures/painter.png', 'textures/painter.json',
        'textures/weapon_paint.png', 'textures/weapon_paint.json',
        'textures/terrain.png', 'textures/terrain.json' |
        ForEach-Object {
            [ordered]@{ path = $_; sha256 = (Get-FileHash -LiteralPath (Join-Path $gameDirectory $_)).Hash }
        }
    )
    $env:PATH = "$gameDirectory;$previousPath"
    $env:LOCALAPPDATA = $privateProfile
    $probeArguments = @((Quote-Path $gameDirectory), (Quote-Path $captures)) + $probeArguments
    $process = Start-Process -FilePath $executable -WindowStyle Hidden -PassThru `
        -WorkingDirectory $working -ArgumentList $probeArguments `
        -RedirectStandardOutput (Join-Path $run 'run.log') `
        -RedirectStandardError (Join-Path $run 'run.err')
    $null = $process.Handle
    $env:PATH = $previousPath
    $env:LOCALAPPDATA = $previousLocalAppData
    $timer = [Diagnostics.Stopwatch]::StartNew()
    while (-not $process.WaitForExit(1000)) {
        if ($timer.Elapsed.TotalSeconds -gt 2400) { throw 'Visual probe exceeded its 2400-second runtime limit.' }
    }
    $process.WaitForExit()
    Get-Content -LiteralPath (Join-Path $run 'run.log') | ForEach-Object { Write-Host $_ }
    if ($process.ExitCode -ne 0) {
        Get-Content -LiteralPath (Join-Path $run 'run.err') | ForEach-Object { Write-Host $_ }
        throw "Visual probe exited with $($process.ExitCode)."
    }
    if ($PracticeReplayOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-practice-replay.ps1') }
    if ($RememberedSetupOnly) {
        # A new OS process reloads the writer's nested private save. The outer
        # aim_marker=false runner fixture remains immutable in both launches.
        $writerProcessId = $process.Id
        $process.Dispose()
        $process = $null
        $env:PATH = "$gameDirectory;$previousPath"
        $env:LOCALAPPDATA = $privateProfile
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
        Assert-CheckDiagnostics -Lines @(Get-Content -LiteralPath (Join-Path $run 'reader.err')) -Context 'remembered setup reader'
        $result['process_relaunch'] = [ordered]@{
            outcome = 'passed'; writer_pid = $writerProcessId; reader_pid = $process.Id; reader_exit_code = $process.ExitCode
            writer_log = Join-Path $run 'run.log'; reader_log = Join-Path $run 'reader.log'
            profile = Join-Path $captures 'remembered-setup-profile'
        }
    }
    $expectedSaveFailures = @{}
    if ($RumbleOnly) {
        $line = 'save failure: Could not write ' + (Get-PainterSavePath (Join-Path $captures 'rumble-settings-profile')) + '.'
        $expectedSaveFailures[$line] = 1
    }
    if ($KeyboardJumpOnly) {
        $line = 'save failure: Could not write ' + (Get-PainterSavePath (Join-Path $captures 'keyboard-jump-profile')) + '.'
        $expectedSaveFailures[$line] = 2
        $line = 'save failure: Could not write ' + (Get-PainterSavePath (Join-Path $captures 'pad-jump-profile')) + '.'
        $expectedSaveFailures[$line] = 1
    }
    if ($PercentageTextOnly) {
        $line = 'save failure: Could not write ' + (Get-PainterSavePath (Join-Path $captures 'percentage-profile')) + '.'
        $expectedSaveFailures[$line] = 1
    }
    if ($PausedOptionsOnly) {
        $line = 'save failure: Could not write ' + (Get-PainterSavePath (Join-Path $captures 'paused-options-profile')) + '.'
        $expectedSaveFailures[$line] = 1
    }
    if ($DisplayTrialOnly) {
        $line = 'save failure: Could not write ' + (Get-PainterSavePath (Join-Path $captures 'display-trial-profile')) + '.'
        $expectedSaveFailures[$line] = 1
    }
    if ($OptionsDefaultsOnly) {
        $line = 'save failure: Could not write ' + (Get-PainterSavePath (Join-Path $captures 'options-defaults-profile')) + '.'
        $expectedSaveFailures[$line] = 6
    }
    if ($SettingsMenuOnly -or $result.scope -eq 'full-painter-matrix') {
        $line = 'save failure: Could not write ' + (Get-PainterSavePath (Join-Path $captures 'settings-profile')) + '.'
        $expectedSaveFailures[$line] = 1
        $line = 'save failure: Could not write ' + (Get-PainterSavePath (Join-Path $captures 'status-options-profile')) + '.'
        $expectedSaveFailures[$line] = 8
        $line = 'save failure: Could not write ' + (Get-PainterSavePath (Join-Path $captures 'values-options-profile')) + '.'
        $expectedSaveFailures[$line] = 2
    }
    if ($ControlsMenuOnly -or $result.scope -eq 'full-painter-matrix') {
        $line = 'save failure: Could not write ' + (Get-PainterSavePath (Join-Path $captures 'controls-profile')) + '.'
        $expectedSaveFailures[$line] = 1
        $line = 'save failure: Could not write ' + (Get-PainterSavePath (Join-Path $captures 'status-controls-profile')) + '.'
        $expectedSaveFailures[$line] = 16
        $line = 'save failure: Could not write ' + (Get-PainterSavePath (Join-Path $captures 'values-controls-profile')) + '.'
        $expectedSaveFailures[$line] = 2
    }
    if ($ControlsMenuOnly -or $StickSettingsOnly -or $result.scope -eq 'full-painter-matrix') {
        $line = 'save failure: Could not write ' + (Get-PainterSavePath (Join-Path $captures 'stick-profile')) + '.'
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
            -not (Test-OptionalBankDiagnostic $line)) {
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
    if ($RumbleOnly -and $debug) { . (Join-Path $PSScriptRoot 'check-painter/verify-rumble.ps1') }
    if ($trialMode) {
        $trialHashAfterAttempted = $true
        $result.trial_source.sha256_after = (Get-FileHash -LiteralPath $trialResolved -Algorithm SHA256).Hash
        $result.trial_source.unchanged = $result.trial_source.sha256_after -ceq $trialHashBefore
        if (-not $result.trial_source.unchanged) { throw 'The native probe changed its explicitly supplied private trial.' }
    }
    if ($CqBlockoutOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-cq-blockout.ps1') }
    if ($CrossfeedOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-crossfeed.ps1') }
    if ($PressTowerOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-press-tower.ps1') }
    if ($TurbulenceBlockoutOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-turbulence-blockout.ps1') }
    if ($TurbulenceMoverOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-turbulence-mover.ps1') }
    if ($TurbulenceShoulderOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-turbulence-shoulder.ps1') }
    if ($TurbulenceSlabOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-turbulence-slab.ps1') }
    if ($GatePermissionOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-gate-permission.ps1') }
    if ($FerryDirectionOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-ferry-direction.ps1') }
    if ($StageScanOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-stage-scan.ps1') }
    if ($SurfaceFinishOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-surface-finish.ps1') }
    if ($RouteWarningOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-route-warning.ps1') }
    if ($ResultPercentageOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-result-percentage.ps1') }
    if ($ContributionTeamOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-contribution-team.ps1') }
    if ($CompetitiveWinnerOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-competitive-winner.ps1') }
    if ($RoundHeadingOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-round-heading.ps1') }
    if ($SeriesScoreOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-series-score.ps1') }
    if ($SessionResultOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-session-result.ps1') }
    if ($ResultsTextOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-results-text.ps1') }
    if ($EndContinuationOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-end-continuation.ps1') }
    if ($ResolutionTextOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-resolution-text.ps1') }
    if ($DurationTextOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-duration-text.ps1') }
    if ($ExposedColourAlphaOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-exposed-colour-alpha.ps1') }
    if ($FeedbackTuneColoursOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-feedback-tune-colours.ps1') }
    if ($CombatTuneColoursOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-combat-tune-colours.ps1') }
    if ($BlockedShotTuneColoursOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-blocked-shot-tune-colours.ps1') }
    if ($IdleDripTuneColoursOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-idle-drip-tune-colours.ps1') }
    if ($RespawnPenTuneColoursOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-respawn-pen-tune-colours.ps1') }
    if ($CursorTuneColoursOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-cursor-tune-colours.ps1') }
    if ($SilhouetteTuneColoursOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-silhouette-tune-colours.ps1') }
    if ($RevealTuneColoursOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-reveal-tune-colours.ps1') }
    if ($CursorMaterialTuneColoursOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-cursor-material-tune-colours.ps1') }
    if ($DesktopTuneColourOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-desktop-tune-colour.ps1') }
    if ($PowerLightTuneColourOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-power-light-tune-colour.ps1') }
    if ($ProgramTuneColourOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-program-tune-colour.ps1') }
    if ($ProgramLevelVisibilityOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-program-level-visibility.ps1') }
    if ($ActingTuneColoursOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-acting-tune-colours.ps1') }
    if ($GlassTuneColourOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-glass-tune-colour.ps1') }
    if ($SilhouetteBodyTuneColourOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-silhouette-body-tune-colour.ps1') }
    if ($ScribbleTuneColourOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-scribble-tune-colour.ps1') }
    if ($HudHealthTuneColourOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-hud-health-tune-colour.ps1') }
    if ($HudTrackTuneColourOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-hud-track-tune-colour.ps1') }
    if ($HudTimerTuneColourOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-hud-timer-tune-colour.ps1') }
    if ($HudClosingMaterialsOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-hud-closing-materials.ps1') }
    if ($HudClosingShadeOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-hud-closing-shade.ps1') }
    if ($HudPaperTuneColoursOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-hud-paper-tune-colours.ps1') }
    if ($HudLabelTuneColoursOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-hud-label-tune-colours.ps1') }
    if ($StagePreviewBackgroundTuneColourOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-stage-preview-background-tune-colour.ps1') }
    if ($CrackTuneColourOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-crack-tune-colour.ps1') }
    if ($LimbTuneColoursOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-limb-tune-colours.ps1') }
    if ($PercentageTextOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-percentage-text.ps1') }
    if (-not $PracticeReplayOnly) { Write-ContactSheet -CaptureDirectory $captures }
    if ($HudTimerTuneColourOnly) { Complete-HudTimerColourEvidence -Directory $captures }
    if ($HudClosingMaterialsOnly) { Complete-HudClosingMaterialsEvidence -Directory $captures }
    if ($HudClosingShadeOnly) { Complete-HudClosingShadeEvidence -Directory $captures }
    if ($HudPaperTuneColoursOnly) { Complete-HudPaperTuneColourEvidence -Directory $captures }
    if ($HudLabelTuneColoursOnly) { Complete-HudLabelColourEvidence -Directory $captures }
    if ($StagePreviewBackgroundTuneColourOnly) { Complete-StagePreviewBackgroundColourEvidence -Directory $captures }
    if ($CrackTuneColourOnly) { Complete-CrackTuneColourEvidence -Directory $captures }
    if ($LimbTuneColoursOnly) { Complete-LimbTuneColourEvidence -Directory $captures }
    $result.outcome = 'passed'
    . (Join-Path $PSScriptRoot 'check-painter/diagnostics.ps1')
}
catch {
    $failure = $_
    $result.diagnostic = $_.Exception.Message
}
finally {
    $env:PATH = $previousPath
    $env:LOCALAPPDATA = $previousLocalAppData
    Exit-QuietGame $previousQuietGame
    if ($null -ne $process) {
        if (-not $process.HasExited) {
            # Only this isolated probe; it owns no player save or session.
            $process.Kill()
            $null = $process.WaitForExit(3000)
        }
        $process.Dispose()
    }
    if ($trialMode -and -not $trialHashAfterAttempted) {
        $trialHashAfterAttempted = $true
        try {
            $result.trial_source.sha256_after = (Get-FileHash -LiteralPath $trialResolved -Algorithm SHA256).Hash
            $result.trial_source.unchanged = $result.trial_source.sha256_after -ceq $trialHashBefore
            if (-not $result.trial_source.unchanged) { throw 'The failed native run changed its explicitly supplied private trial.' }
        }
        catch {
            if ($null -eq $failure) { $failure = $_ }
            $result.outcome = 'failed'
            $result.diagnostic += ' Trial source check: ' + $_.Exception.Message
        }
    }
    # HYG#93: a passing run keeps its evidence, not the probe it compiled (about
    # 97% of a leaf's folder). A failed or compile-only run keeps both.
    if ($null -eq $failure -and $result.outcome -eq 'passed') {
        $removedProbe = [ordered]@{ files = @(); bytes = 0L }
        foreach ($file in @(Get-ChildItem -LiteralPath $run -File -Filter 'painter_visual*' |
                Where-Object { @('.exe', '.obj', '.pdb', '.ilk') -contains $_.Extension.ToLowerInvariant() })) {
            try {
                Remove-Item -LiteralPath $file.FullName -Force
                $removedProbe.files += $file.Name
                $removedProbe.bytes += $file.Length
            }
            catch { Write-Warning "Could not remove $($file.FullName): $($_.Exception.Message)" }
        }
        $result.removed_after_pass = $removedProbe
    }
    $result | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $run 'result.json') -Encoding utf8
    Write-Host "Painter evidence: $run"
    Pop-Location
}
if ($null -ne $failure) { throw $failure }
