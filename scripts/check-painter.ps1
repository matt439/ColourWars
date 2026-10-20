# Painter visual probe runner. Helpers live beside it in check-painter/ and are
# dot-sourced into this script's scope. A new focused leaf touches six places:
# the param list, the one-leaf guard below, check-painter/evidence-record.ps1,
# the probe-argument chain, a check-painter/verify-<leaf>.ps1 with its guarded
# dot-source line (if it has post-run evidence), and the diagnostics chain.
[CmdletBinding()]
param(
    [ValidateSet('x64-debug', 'x64-release')]
    [string] $Preset = 'x64-debug',
    [switch] $SkipBuild,
    [switch] $IdentityOnly,
    [switch] $QuietOnly,
    [switch] $DriftOnly,
    [switch] $MiniatureTerrainOnly,
    [switch] $CqBlockoutOnly,
    [switch] $CrossfeedOnly,
    [switch] $LandmarkOnly,
    [switch] $ParallaxOnly,
    [switch] $TuneColoursOnly,
    [switch] $FeedbackTuneColoursOnly,
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
if (([int]$CrossfeedOnly.IsPresent + [int]$CqBlockoutOnly.IsPresent + [int]$ResultPercentageOnly.IsPresent + [int]$ContributionTeamOnly.IsPresent + [int]$CompetitiveWinnerOnly.IsPresent + [int]$RoundHeadingOnly.IsPresent + [int]$SeriesScoreOnly.IsPresent + [int]$SessionResultOnly.IsPresent + [int]$ResultsTextOnly.IsPresent + [int]$EndContinuationOnly.IsPresent + [int]$ResolutionTextOnly.IsPresent + [int]$DurationTextOnly.IsPresent + [int]$ExposedColourAlphaOnly.IsPresent + [int]$FeedbackTuneColoursOnly.IsPresent + [int]$PercentageTextOnly.IsPresent + [int]$ParallaxOnly.IsPresent + [int]$IdentityOnly.IsPresent + [int]$QuietOnly.IsPresent + [int]$DriftOnly.IsPresent + [int]$MiniatureTerrainOnly.IsPresent + [int]$LandmarkOnly.IsPresent + [int]$TuneColoursOnly.IsPresent + [int]$MonitorOnly.IsPresent + [int]$CountdownOnly.IsPresent + [int]$ClosingOnly.IsPresent + [int]$SupplyOnly.IsPresent + [int]$HudOnly.IsPresent + [int]$ResumeOnly.IsPresent + [int]$EndMenuOnly.IsPresent + [int]$PauseMenuOnly.IsPresent + [int]$ResultsMenuOnly.IsPresent + [int]$MainMenuOnly.IsPresent + [int]$WeaponCatalogOnly.IsPresent + [int]$MenuTextOnly.IsPresent + [int]$MenuMotionOnly.IsPresent + [int]$SetupMotionOnly.IsPresent + [int]$JoinStageMotionOnly.IsPresent + [int]$SetupPaperOnly.IsPresent + [int]$JoinPaperOnly.IsPresent + [int]$TeamPaperOnly.IsPresent + [int]$WeaponPaperOnly.IsPresent + [int]$StagePaperOnly.IsPresent + [int]$HomeReturnFocusOnly.IsPresent + [int]$HomeExitOnly.IsPresent + [int]$OptionsDefaultsOnly.IsPresent + [int]$DisplayTrialOnly.IsPresent + [int]$PausedOptionsOnly.IsPresent + [int]$SettingsMenuOnly.IsPresent + [int]$ControlsMenuOnly.IsPresent + [int]$StickSettingsOnly.IsPresent + [int]$TeamSetupOnly.IsPresent + [int]$RememberedSetupOnly.IsPresent + [int]$ConnectionTextOnly.IsPresent + [int]$StickReadoutsOnly.IsPresent + [int]$TeamFocusOnly.IsPresent + [int]$TeamTuneColoursOnly.IsPresent + [int]$WeaponTuneColoursOnly.IsPresent + [int]$MenuLabelsOnly.IsPresent + [int]$StageSummaryOnly.IsPresent + [int]$StageCountsOnly.IsPresent) -gt 1) {
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

. (Join-Path $PSScriptRoot 'check-painter/contact-sheet.ps1')

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
. (Join-Path $PSScriptRoot 'check-painter/evidence-record.ps1')
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
        'levels/close_quarters.json', 'levels/king_of_the_hill.json', 'levels/turbulence.json', 'levels/crossfeed.json',
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
    elseif ($CqBlockoutOnly) { $probeArguments += 'cq-blockout' }
    elseif ($CrossfeedOnly) { $probeArguments += 'crossfeed' }
    elseif ($LandmarkOnly) { $probeArguments += 'landmark' }
    elseif ($ParallaxOnly) { $probeArguments += 'parallax' }
    elseif ($TuneColoursOnly) { $probeArguments += 'tune-colours' }
    elseif ($FeedbackTuneColoursOnly) { $probeArguments += 'feedback-tune-colours' }
    elseif ($ExposedColourAlphaOnly) { $probeArguments += 'exposed-colour-alpha' }
    elseif ($DurationTextOnly) { $probeArguments += 'duration-text' }
    elseif ($ResolutionTextOnly) { $probeArguments += 'resolution-text' }
    elseif ($EndContinuationOnly) { $probeArguments += 'end-continuation' }
    elseif ($ResultsTextOnly) { $probeArguments += 'results-text' }
    elseif ($SessionResultOnly) { $probeArguments += 'session-result' }
    elseif ($SeriesScoreOnly) { $probeArguments += 'series-score' }
    elseif ($RoundHeadingOnly) { $probeArguments += 'round-heading' }
    elseif ($CompetitiveWinnerOnly) { $probeArguments += 'competitive-winner' }
    elseif ($ContributionTeamOnly) { $probeArguments += 'contribution-team' }
    elseif ($ResultPercentageOnly) { $probeArguments += 'result-percentage' }
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
        if ($timer.Elapsed.TotalSeconds -gt 2400) { throw 'Visual probe exceeded its 2400-second runtime limit.' }
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
    if ($CqBlockoutOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-cq-blockout.ps1') }
    if ($CrossfeedOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-crossfeed.ps1') }
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
    if ($PercentageTextOnly) { . (Join-Path $PSScriptRoot 'check-painter/verify-percentage-text.ps1') }
    Write-ContactSheet -CaptureDirectory $captures
    $result.outcome = 'passed'
    $identityDiagnostic = 'Player identity passed all-palette same-team overlap, scripted crossing, sparse/reordered slots, native P1-P4 glyph distinction, quiet/emphasis/reduced effects, own-pane world-only bounds, dead/respawn/story suppression and frozen state. Human playing-size review remains separate.'
    $stageEntryDiagnostic = 'Stage entry passed all four authored names, a three-player roster, bounded title fitting at narrow/wide aspects, opening/monitor skips, static reduced-effects information and hiding before countdown and at GO.'
    if ($ResultPercentageOnly) {
        $result.diagnostic = 'Result percentage text passed independent solo coverage and A/B percentage labels on actual Results pages and inner cards on three seeded public LevelBuilder monitor fixtures. Captured templates/fonts, retained pages, reopening/reversal, reveal/proceed transitions and zero-update resize passed. Drawing and resize preserved Session/Level state and RNG; private preferences and bytes remained unchanged. The three monitor fixtures intentionally advanced eighty Level updates each. Exactly 60 StateContext updates plus a separate 240 Level updates, 120 readbacks, 105 target CSV rows (81 Results and 24 monitor), eight Results openings and proceed pops, six font role controls from three wrong-font frames, eight compact crops (681072 pixels) and one successful private seed Save. Both presets ran the same shared consumer scope without developer targets or commands. Monitor evidence covers the inner cards. No broad monitor-story or menu matrix, foreground/global input, physical-device, listening, human readability or performance acceptance.'
    }
    elseif ($ContributionTeamOnly) {
        $result.diagnostic = 'Contribution team text passed independent complete rows for both separators on a sparse four-player Results roster, preserving owner-credit percentages and badge context. Captured copy/font assignment, retained pages, reopening/reversal, actual reveal/proceed transitions, frozen Session/setup/private Save/RNG and zero-update resize passed. Exactly 45 updates, 56 readbacks, 72 CSV rows across eighteen four-row observations, five page openings and proceed pops, four font row controls, four wrong-view-credit row controls, six compact crops (599508 pixels) and one successful private seed Save. Both presets ran the same shared consumer scope without developer targets or commands. No broad menu matrix, match launch, foreground/global input, physical-device, listening, human readability or performance acceptance.'
    }
    elseif ($CompetitiveWinnerOnly) {
        $result.diagnostic = 'Competitive winner text passed independent complete Team A, Team B and Draw labels, unchanged solo copy and explicit hidden-winner controls. Captured copy/item-font assignment, retained pages, reopening/reversal, actual reveal/proceed transitions, frozen Session/setup/private Save/RNG and zero-update resize passed. Exactly 102 updates, 99 readbacks, 32 CSV rows, fourteen page openings and proceed pops, three font negative controls, eight compact crops (312480 pixels) and one successful private seed Save. Both presets ran the same shared consumer scope without developer targets or commands. No broad menu matrix, match launch, foreground/global input, physical-device, listening, human readability or performance acceptance.'
    }
    elseif ($RoundHeadingOnly) {
        $result.diagnostic = 'Round heading text passed independent complete literal headings on actual all-draw Session rounds 1, 10 and 100, plus the unchanged single-match heading under the replacement template/font. Captured prefix/suffix template and heading-font assignment, retained pages, reopening/reversal, unchanged literal score/outcome controls, actual reveal/proceed transitions, frozen Session/setup/private Save/RNG and zero-update resize passed. Exactly 66 updates, 81 readbacks, 26 CSV rows, eleven page openings and proceed pops, three font negative controls, eight compact crops (722400 pixels) and one successful private seed Save. Both presets ran the same shared consumer scope without developer targets or commands. No broad menu matrix, match launch, foreground/global input, physical-device, listening, human readability or performance acceptance.'
    }
    elseif ($SeriesScoreOnly) {
        $result.diagnostic = 'Series score text passed independent complete literal scores and outcome/heading controls across scoreless draw, ongoing series and both maximum-round winners, plus single-match score absence. Captured template/font assignment, reordered tokens, retained pages, reopening/reversal, actual reveal/proceed transitions, frozen Session/setup/private Save/RNG and zero-update resize passed. Exactly 81 updates, 97 readbacks, 31 CSV rows, fourteen page openings and proceed pops, four font negative controls, eight compact crops (710640 pixels) and one successful private seed Save. Both presets ran the same shared consumer scope without developer targets or commands. No broad menu matrix, match launch, foreground/global input, physical-device, listening, human readability or performance acceptance.'
    }
    elseif ($SessionResultOnly) {
        $result.diagnostic = 'Session result text passed four independent literal actual Results outcomes, full row/score-neighbor pixels and literal Round headings through completed A/B, drawn and ongoing series. Captured copy/font assignment, retained pages, reopening/reversal, actual reveal/proceed transitions, frozen Session/setup/private Save/RNG and zero-update resize passed. Exactly 76 updates, 94 readbacks, 30 CSV rows, thirteen page openings and proceed pops, four font negative controls, eight compact crops (710640 pixels) and one successful private seed Save. Both presets ran the same shared consumer scope without developer targets or commands. No broad menu matrix, match launch, foreground/global input, physical-device, listening, human readability or performance acceptance.'
    }
    elseif ($ResultsTextOnly) {
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
    elseif ($CrossfeedOnly) {
        $result.diagnostic = 'Crossfeed focused native inspection passed: clean/heavy 1/2/3/4-player moving approach panes, four-player home exits, actual runtime overviews and StagePreview. Twenty-one full frames, 51 native crops and 43 readbacks retain camera/visibility observations and frozen pixels/state/RNG/Debug fingerprint. Controlled station placement and heavy-paint contacts are labeled; human readability, balance, pressure and physical-pad acceptance remain open.'
    }
    elseif ($CqBlockoutOnly) {
        $result.diagnostic = 'Close Quarters first playable layout: bounded native720p main, lower, mixed four-pane, runtime overview and actual StagePreview captures; frozen simulation, replay fingerprint and draw RNG checked. Route, respawn and tool contact checks live in CPU acceptance. Hidden rendering establishes no attended play, human readability, physical-pad, listening or competitive balance acceptance.'
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
