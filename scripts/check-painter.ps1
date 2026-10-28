# Painter visual probe runner. Helpers live beside it in check-painter/ and are
# dot-sourced into this script's scope. A focused leaf has seven integration points:
# the param list, the one-leaf guard below, check-painter/evidence-record.ps1,
# the probe-argument chain, a check-painter/verify-<leaf>.ps1 with its guarded
# dot-source line (if it has post-run evidence), check-painter/diagnostics.ps1,
# and the check-painter/contact-sheet.ps1 selector for any new capture family.
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
    [switch] $PressTowerOnly,
    [switch] $TurbulenceBlockoutOnly,
    [switch] $TurbulenceMoverOnly,
    [switch] $TurbulenceShoulderOnly,
    [switch] $TurbulenceSlabOnly,
    [string] $TrialPath,
    [switch] $GatePermissionOnly,
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
    [string] $OutputDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
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
    $trialResolved = [IO.Path]::GetFullPath($TrialPath)
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
if (([int]$PracticeReplayOnly.IsPresent + [int]$RumbleOnly.IsPresent + [int]$KeyboardJumpOnly.IsPresent + [int]$RouteWarningOnly.IsPresent + [int]$CrossfeedOnly.IsPresent + [int]$PressTowerOnly.IsPresent + [int]$TurbulenceBlockoutOnly.IsPresent + [int]$TurbulenceMoverOnly.IsPresent + [int]$TurbulenceShoulderOnly.IsPresent + [int]$TurbulenceSlabOnly.IsPresent + [int]$GatePermissionOnly.IsPresent + [int]$SurfaceFinishOnly.IsPresent + [int]$CqBlockoutOnly.IsPresent + [int]$ResultPercentageOnly.IsPresent + [int]$ContributionTeamOnly.IsPresent + [int]$CompetitiveWinnerOnly.IsPresent + [int]$RoundHeadingOnly.IsPresent + [int]$SeriesScoreOnly.IsPresent + [int]$SessionResultOnly.IsPresent + [int]$ResultsTextOnly.IsPresent + [int]$EndContinuationOnly.IsPresent + [int]$ResolutionTextOnly.IsPresent + [int]$DurationTextOnly.IsPresent + [int]$ExposedColourAlphaOnly.IsPresent + [int]$FeedbackTuneColoursOnly.IsPresent + [int]$CombatTuneColoursOnly.IsPresent + [int]$BlockedShotTuneColoursOnly.IsPresent + [int]$IdleDripTuneColoursOnly.IsPresent + [int]$RespawnPenTuneColoursOnly.IsPresent + [int]$CursorTuneColoursOnly.IsPresent + [int]$SilhouetteTuneColoursOnly.IsPresent + [int]$RevealTuneColoursOnly.IsPresent + [int]$CursorMaterialTuneColoursOnly.IsPresent + [int]$DesktopTuneColourOnly.IsPresent + [int]$PowerLightTuneColourOnly.IsPresent + [int]$ProgramTuneColourOnly.IsPresent + [int]$ProgramLevelVisibilityOnly.IsPresent + [int]$ActingTuneColoursOnly.IsPresent + [int]$GlassTuneColourOnly.IsPresent + [int]$SilhouetteBodyTuneColourOnly.IsPresent + [int]$ScribbleTuneColourOnly.IsPresent + [int]$HudHealthTuneColourOnly.IsPresent + [int]$HudTrackTuneColourOnly.IsPresent + [int]$HudTimerTuneColourOnly.IsPresent + [int]$HudClosingMaterialsOnly.IsPresent + [int]$HudClosingShadeOnly.IsPresent + [int]$HudPaperTuneColoursOnly.IsPresent + [int]$HudLabelTuneColoursOnly.IsPresent + [int]$StagePreviewBackgroundTuneColourOnly.IsPresent + [int]$CrackTuneColourOnly.IsPresent + [int]$LimbTuneColoursOnly.IsPresent + [int]$PercentageTextOnly.IsPresent + [int]$ParallaxOnly.IsPresent + [int]$IdentityOnly.IsPresent + [int]$QuietOnly.IsPresent + [int]$DriftOnly.IsPresent + [int]$MiniatureTerrainOnly.IsPresent + [int]$LandmarkOnly.IsPresent + [int]$TuneColoursOnly.IsPresent + [int]$MonitorOnly.IsPresent + [int]$CountdownOnly.IsPresent + [int]$ClosingOnly.IsPresent + [int]$SupplyOnly.IsPresent + [int]$HudOnly.IsPresent + [int]$ResumeOnly.IsPresent + [int]$EndMenuOnly.IsPresent + [int]$PauseMenuOnly.IsPresent + [int]$ResultsMenuOnly.IsPresent + [int]$MainMenuOnly.IsPresent + [int]$WeaponCatalogOnly.IsPresent + [int]$MenuTextOnly.IsPresent + [int]$MenuMotionOnly.IsPresent + [int]$SetupMotionOnly.IsPresent + [int]$JoinStageMotionOnly.IsPresent + [int]$SetupPaperOnly.IsPresent + [int]$JoinPaperOnly.IsPresent + [int]$TeamPaperOnly.IsPresent + [int]$WeaponPaperOnly.IsPresent + [int]$StagePaperOnly.IsPresent + [int]$HomeReturnFocusOnly.IsPresent + [int]$HomeExitOnly.IsPresent + [int]$OptionsDefaultsOnly.IsPresent + [int]$DisplayTrialOnly.IsPresent + [int]$PausedOptionsOnly.IsPresent + [int]$SettingsMenuOnly.IsPresent + [int]$ControlsMenuOnly.IsPresent + [int]$StickSettingsOnly.IsPresent + [int]$TeamSetupOnly.IsPresent + [int]$RememberedSetupOnly.IsPresent + [int]$BotPlayersOnly.IsPresent + [int]$ConnectionTextOnly.IsPresent + [int]$StickReadoutsOnly.IsPresent + [int]$TeamFocusOnly.IsPresent + [int]$TeamTuneColoursOnly.IsPresent + [int]$WeaponTuneColoursOnly.IsPresent + [int]$MenuLabelsOnly.IsPresent + [int]$StageSummaryOnly.IsPresent + [int]$StageCountsOnly.IsPresent) -gt 1) {
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
if ($RumbleOnly -and $Preset -eq 'x64-debug') {
    $result.background_supplement = 'Debug-only actual GameLevel script.background checks: native private-HWND facts; synthetic StateContext activation and injected hard gates; neutral physical actions, truthful presence, script/bot/replay, Music/SFX group mute before shots, recording motors and unchanged private Save. No physical pad, listening, real OS focus/power-sleep, CI1 or performance acceptance.'
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
            # Every painter mode supplies a fake factory, so no existing or
            # future GameLevel probe can contact physical rumble motors.
            if ($_.Name -in @('main.cpp.obj', 'rumble_xinput.cpp.obj')) { return $false }
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
    if ($RumbleOnly -or $KeyboardJumpOnly -or $StageSummaryOnly -or $WeaponTuneColoursOnly -or $MenuLabelsOnly -or $TeamTuneColoursOnly -or $TeamFocusOnly -or $StickReadoutsOnly -or $ConnectionTextOnly -or $RememberedSetupOnly -or $BotPlayersOnly -or $TeamSetupOnly -or $MenuTextOnly -or $SetupMotionOnly -or $JoinStageMotionOnly -or $JoinPaperOnly -or $TeamPaperOnly -or $WeaponPaperOnly -or $StagePaperOnly -or $StagePreviewBackgroundTuneColourOnly -or $HomeReturnFocusOnly -or $HomeExitOnly -or $OptionsDefaultsOnly -or $DisplayTrialOnly -or $PausedOptionsOnly) { $arguments += '/DCOLOURWARS_PAINTER_SCRIPTED_PADS' }
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
        'uuid.lib', 'comdlg32.lib', 'advapi32.lib', 'comctl32.lib')
    $arguments | Out-File -LiteralPath $responseFile -Encoding unicode
    Write-Host "Compile painter visual probe ($Preset)"
    Invoke-Logged -Command $cache.CMAKE_CXX_COMPILER -Arguments @("@$responseFile") `
        -LogPath (Join-Path $run 'compile.log')

    $result.staged_content_hashes = @(
        'manifest.json', 'tuning.json', 'presentation.json', 'levels/movement_course.json',
        'levels/close_quarters.json', 'levels/king_of_the_hill.json', 'levels/turbulence.json', 'levels/crossfeed.json',
        'levels/press_tower.json', 'levels/turbulence_blockout.json',
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
    elseif ($PressTowerOnly) { $probeArguments += 'press-tower' }
    elseif ($TurbulenceBlockoutOnly) { $probeArguments += 'turbulence-blockout' }
    elseif ($TurbulenceMoverOnly) { $probeArguments += @('turbulence-mover', (Quote-Path $trialResolved)) }
    elseif ($TurbulenceShoulderOnly) { $probeArguments += @('turbulence-shoulder', (Quote-Path $trialResolved)) }
    elseif ($TurbulenceSlabOnly) { $probeArguments += @('turbulence-slab', (Quote-Path $trialResolved)) }
    elseif ($GatePermissionOnly) { $probeArguments += 'gate-permission' }
    elseif ($SurfaceFinishOnly) { $probeArguments += 'surface-finish' }
    elseif ($RouteWarningOnly) { $probeArguments += 'route-warning' }
    elseif ($LandmarkOnly) { $probeArguments += 'landmark' }
    elseif ($ParallaxOnly) { $probeArguments += 'parallax' }
    elseif ($TuneColoursOnly) { $probeArguments += 'tune-colours' }
    elseif ($FeedbackTuneColoursOnly) { $probeArguments += 'feedback-tune-colours' }
    elseif ($CombatTuneColoursOnly) { $probeArguments += 'combat-tune-colours' }
    elseif ($BlockedShotTuneColoursOnly) { $probeArguments += 'blocked-shot-tune-colours' }
    elseif ($IdleDripTuneColoursOnly) { $probeArguments += 'idle-drip-tune-colours' }
    elseif ($RespawnPenTuneColoursOnly) { $probeArguments += 'respawn-pen-tune-colours' }
    elseif ($CursorTuneColoursOnly) { $probeArguments += 'cursor-tune-colours' }
    elseif ($SilhouetteTuneColoursOnly) { $probeArguments += 'silhouette-tune-colours' }
    elseif ($RevealTuneColoursOnly) { $probeArguments += 'reveal-tune-colours' }
    elseif ($CursorMaterialTuneColoursOnly) { $probeArguments += 'cursor-material-tune-colours' }
    elseif ($DesktopTuneColourOnly) { $probeArguments += 'desktop-tune-colour' }
    elseif ($PowerLightTuneColourOnly) { $probeArguments += 'power-light-tune-colour' }
    elseif ($ProgramTuneColourOnly) { $probeArguments += 'program-tune-colour' }
    elseif ($ProgramLevelVisibilityOnly) { $probeArguments += 'program-level-visibility' }
    elseif ($ActingTuneColoursOnly) { $probeArguments += 'acting-tune-colours' }
    elseif ($GlassTuneColourOnly) { $probeArguments += 'glass-tune-colour' }
    elseif ($SilhouetteBodyTuneColourOnly) { $probeArguments += 'silhouette-body-tune-colour' }
    elseif ($ScribbleTuneColourOnly) { $probeArguments += 'scribble-tune-colour' }
    elseif ($HudHealthTuneColourOnly) { $probeArguments += 'hud-health-tune-colour' }
    elseif ($HudTrackTuneColourOnly) { $probeArguments += 'hud-track-tune-colour' }
    elseif ($HudTimerTuneColourOnly) { $probeArguments += 'hud-timer-tune-colour' }
    elseif ($HudClosingMaterialsOnly) { $probeArguments += 'hud-closing-materials' }
    elseif ($HudClosingShadeOnly) { $probeArguments += 'hud-closing-shade' }
    elseif ($HudPaperTuneColoursOnly) { $probeArguments += 'hud-paper-tune-colours' }
    elseif ($HudLabelTuneColoursOnly) { $probeArguments += 'hud-label-tune-colours' }
    elseif ($StagePreviewBackgroundTuneColourOnly) { $probeArguments += 'stage-preview-background-tune-colour' }
    elseif ($CrackTuneColourOnly) { $probeArguments += 'crack-tune-colour' }
    elseif ($LimbTuneColoursOnly) { $probeArguments += 'limb-tune-colours' }
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
    elseif ($PracticeReplayOnly) { $probeArguments += 'practice-replay' }
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
    elseif ($RumbleOnly) { $probeArguments += 'rumble' }
    elseif ($KeyboardJumpOnly) { $probeArguments += 'keyboard-jump' }
    elseif ($PercentageTextOnly) { $probeArguments += 'percentage-text' }
    elseif ($StickSettingsOnly) { $probeArguments += 'stick-settings' }
    elseif ($TeamSetupOnly) { $probeArguments += 'team-setup' }
    elseif ($RememberedSetupOnly) { $probeArguments += 'remembered-setup' }
    elseif ($BotPlayersOnly) { $probeArguments += 'bot-players' }
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
    if ($PracticeReplayOnly) {
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
    if ($RumbleOnly) {
        $line = 'save failure: Could not write ' + (Join-Path $captures 'rumble-settings-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 1
    }
    if ($KeyboardJumpOnly) {
        $line = 'save failure: Could not write ' + (Join-Path $captures 'keyboard-jump-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 2
        $line = 'save failure: Could not write ' + (Join-Path $captures 'pad-jump-profile/ColourWars/save_data.json') + '.'
        $expectedSaveFailures[$line] = 1
    }
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
    if ($RumbleOnly -and $debug) {
        $background = Get-Content -LiteralPath (Join-Path $captures 'debug-background-summary.json') -Raw | ConvertFrom-Json
        $backgroundRows = @(Import-Csv -LiteralPath (Join-Path $captures 'debug-background.csv'))
        $backgroundCases = @('default-off-inactive', 'actual-hidden-hard-gate',
            'synthetic-inactive-neutral-presence-muted-before-step', 'foreground-zero-run-edge-discarded',
            'inactive-actual-damage-motors-zero', 'inactive-script-wait',
            'manual-debug-pause-and-script-self-disable-barrier', 'covered-toggle-hard-gate',
            'injected-hidden', 'injected-minimized', 'injected-power-suspended', 'off-inactive-freezes',
            'off-then-activation-ordinary-countin-current-preferences', 'actual-pause-menu-hard-gate',
            'resume-countin-hard-gate', 'missing-pad-real-pause', 'inactive-bot-paid-shot-record',
            'inactive-replay-final-fingerprint', 'new-inactive-level-enabled', 'new-inactive-level-disabled')
        if ($background.outcome -cne 'passed' -or $background.cases -ne 20 -or $backgroundRows.Count -ne 20 -or
            $background.recorded_updates -ne 420 -or $background.native_motor_calls -ne 0 -or
            $background.save_unchanged -ne $true -or
            @($backgroundRows.case | Sort-Object -Unique).Count -ne 20 -or
            @(Compare-Object -ReferenceObject $backgroundCases -DifferenceObject @($backgroundRows.case)).Count -ne 0) {
            throw 'Debug background supplement did not produce its complete bounded 20-case/420-update receipts.'
        }
        $result.background_receipt = $background
    }
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
    $result | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $run 'result.json') -Encoding utf8
    Write-Host "Painter evidence: $run"
    Pop-Location
}
if ($null -ne $failure) { throw $failure }
