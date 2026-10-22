[CmdletBinding()]
param(
    [ValidateSet('x64-debug')]
    [string]$Preset = 'x64-debug',
    [string]$OutputDirectory,
    [ValidateRange(4, 120)]
    [int]$StartupSeconds = 12,
    # The lone-Alt probe is informational: a lone Alt is an ordinary key to the
    # engine's window, and the probe records whether the game kept running.
    [switch]$SkipAltProbe,
    # Run only the phase-3 launch (bots, tune, record/play, tuning menu).
    [switch]$Phase3Only,
    # Run only the phase-4 launches (scripts, binds, history, live console, perf).
    [switch]$Phase4Only,
    # Phase 3 also runs tune.save against the SOURCE game/content/tuning.json,
    # checks with git that exactly one value changed, and writes the saved
    # original bytes back (git checkout would not: it renormalises the line
    # endings of a working copy that mixes CRLF and LF). Refused unless the
    # file is clean in git first.
    [switch]$SaveToSource
)

# Opt-in, local: drives the real Debug executable and takes screenshots for a
# person to look at. Not in CI: hosted runners give no reliable foreground
# focus, and Release has no console (scripts/acceptance/debug_console.cpp
# covers both presets on the CPU side). The pad chord (Back+Start) needs a pad
# and is covered there too.
#
# Since phase 4 most steps are console scripts (`exec`, or `+exec` at launch)
# written into each launch's isolated profile, and the transcript's `echo`
# markers say when they are done. Key presses remain only where the keyboard
# itself is what is checked: the console (grave, typing, Enter, Escape), F1-F10,
# the debug menu, Shift+F5 on a bind, the live console, Alt.
#
# Phase 1-2 (one launch, +stage movement_course +players 2 +overlay all)
#    1. ` status Enter                     (console over two panes, overlays on)
#    2. F9                                 (default feedback bind)
#    3. F10, view, overlay.collision Right (the debug menu steps a variable), Escape, F10
#    4. F10, match, players, 3, Escape, Apply   (staged, then one rebuild)
#    5. ` overlay none, F1, then F2 F3 F4  (overlay keys; P1's own F1 text survives them)
#    6. ` exec check_drive                 (goto, status, restart+goto, cheats, screenshot)
#    7. Escape (pause menu), ` stage close_quarters   (the pause menu unwinds)
#    8. ` exec check_menus                 (menu, then a match started from the menus)
#    9. Alt alone (informational), then ` quit Enter
#   10. a second launch on a 1280x720 isolated save: four seats, every overlay,
#       ammo inf, timescale 0.5, paused in the countdown - the status strip must
#       clear the paint panel and the KEYBOARD + MOUSE card (narrow-panels.png),
#       and the debug menu's header P1's HUD box (narrow-debug-menu.png)
#
# Phase 3 (one launch): bots, live tuning, record and play, the tuning menu.
# Phase 4 (four launches): the shipped review scripts with +exec, autoexec,
# binds and history across a restart, the live console, the perf overlay and
# a tune edit that leaves nothing in an isolated %TEMP%.

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# Windows PowerShell 5.1 does not set PSScriptRoot during parameter defaults.
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $PSScriptRoot '../out/validation/debug-console'
}

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$executable = Join-Path $repo "out/build/$Preset/game/ColourWars.exe"
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
    throw "Build $Preset first: $executable is missing."
}
$output = [IO.Path]::GetFullPath($OutputDirectory)
$profileDirectory = Join-Path $output 'profile'
$null = New-Item -ItemType Directory -Path $output -Force

. (Join-Path $PSScriptRoot 'check-debug-console/driver.ps1')
$transcript = Join-Path $output 'console.log'
$window = [IntPtr]::Zero

# The values the checks use come from the file the game loads, so a balance
# pass on tuning.json never breaks this script.
$stagedTuningText = [IO.File]::ReadAllText((Join-Path (Split-Path $executable) 'tuning.json'))
function Get-PlayerToken([string]$pattern) {
    $match = [regex]::Match($stagedTuningText, $pattern)
    if (-not $match.Success) { throw "tuning.json has no match for $pattern" }
    return $match.Groups[1].Value
}
# A value written the way tune_format writes it after `previous`.
function Format-Token([double]$value, [string]$previous) {
    $invariant = [Globalization.CultureInfo]::InvariantCulture
    if ($value -eq [math]::Floor($value)) {
        $whole = ([long]$value).ToString($invariant)
        if ($previous -match '[.eE]') { return "$whole.0" } else { return $whole }
    }
    return $value.ToString('R', $invariant)
}
$invariantCulture = [Globalization.CultureInfo]::InvariantCulture
$velocityLine = [regex]::Match($stagedTuningText, '"max_velocity": \{ "x": [0-9.]+, "y": [0-9.]+ \}').Value
$velocityToken = Get-PlayerToken '"max_velocity": \{ "x": ([0-9.]+), "y": [0-9.]+ \}'
$fastToken = Format-Token ([double]::Parse($velocityToken, $invariantCulture) * 2) $velocityToken
$savedToken = Format-Token ([double]::Parse($velocityToken, $invariantCulture) + 100) $velocityToken
$accelerationToken = Get-PlayerToken '"x_acceleration": ([0-9.]+)'

$process = $null
if (-not $Phase3Only -and -not $Phase4Only) {
try {
    Remove-Item -LiteralPath $profileDirectory -Recurse -Force -ErrorAction SilentlyContinue
    $null = New-Item -ItemType Directory -Path $profileDirectory -Force
    Remove-Item -LiteralPath $transcript -ErrorAction SilentlyContinue
    $commandShot = Join-Path $profileDirectory 'ColourWars/screens/drive.png'

    # Step 6 and 8 as scripts: the console runs one `exec` line each.
    Write-Script $profileDirectory 'check_drive' @(
        '// The phase-1/2 driven steps that do not test keys: goto, status, restart, cheats, screenshot.',
        'goto high_step',
        '// Settle on the step before status reads the position.',
        'wait 90',
        'status',
        '// A goto behind a queued rebuild waits for it instead of being undone.',
        'restart; goto running_gap',
        'wait match',
        'wait 30',
        'god 1; ammo inf; respawn all',
        'wait 30',
        'screenshot drive',
        'echo check-drive-done')
    Write-Script $profileDirectory 'check_menus' @(
        '// Leave for the menus, then start a match from them.',
        'menu',
        'wait frames 30',
        'stage movement_course',
        'wait match',
        'echo check-menus-done')

    $arguments = @('+logfile', ('"' + $transcript + '"'), '+stage', 'movement_course',
        '+players', '2', '+overlay', 'all')
    $process = Start-Game $profileDirectory $arguments
    Start-Sleep -Seconds $StartupSeconds
    $window = Find-Window $process
    Confirm-Focus; Start-Sleep -Milliseconds 400
    Save-Shot '01-launch-overlays.png'

    # 1-2: the console and F9. The first line is checked against the
    # transcript: keys sent in the first moments after focusing can be lost.
    Invoke-Checked 'status' $false
    Save-Shot '02-console-status.png'
    Close-Console
    $driver::Tap($f9, $false); Start-Sleep -Milliseconds 800
    Save-Shot '03-after-f9.png'

    # 3: debug menu by F10. Root rows with nothing queued: match, player, view,
    # time, content, console. View, by name: feedback, overlay, overlay.camera,
    # overlay.collision, overlay.paint, overlay.perf, overlay.player.
    $driver::Tap($f10, $false); Start-Sleep -Milliseconds 500
    Save-Shot '04-debug-menu.png'
    $driver::Tap($down, $true); $driver::Tap($down, $true)
    $driver::Tap($enter, $false); Start-Sleep -Milliseconds 300
    $driver::Tap($down, $true); $driver::Tap($down, $true); $driver::Tap($down, $true)
    $driver::Tap($right, $true); Start-Sleep -Milliseconds 400
    Save-Shot '05-debug-menu-view.png'
    $driver::Tap($escape, $false); Start-Sleep -Milliseconds 300
    $driver::Tap($f10, $false); Start-Sleep -Milliseconds 600
    Save-Shot '06-collision-off.png'

    # 4: staging. Match, by name: intro, menu, players, restart, stage,
    # weapon. The players picker opens on 1; two Down presses choose 3.
    $driver::Tap($f10, $false); Start-Sleep -Milliseconds 500
    $driver::Tap($enter, $false); Start-Sleep -Milliseconds 300
    $driver::Tap($down, $true); $driver::Tap($down, $true)
    $driver::Tap($enter, $false); Start-Sleep -Milliseconds 300
    $driver::Tap($down, $true); $driver::Tap($down, $true)
    $driver::Tap($enter, $false); Start-Sleep -Milliseconds 400
    $driver::Tap($escape, $false); Start-Sleep -Milliseconds 400
    Save-Shot '07-staged-apply.png'
    $staged = Get-Transcript
    if ($staged.Contains('match: movement_course, 3 player(s)')) {
        throw 'The staged roster rebuilt the match before Apply.'
    }
    $driver::Tap($enter, $false); Start-Sleep -Milliseconds 1500
    # Restore the two-pane fixture after verifying the staged roster applies.
    Invoke-Checked 'players 2' $false 1500

    # 5: overlay keys.
    # F1 turns on P1's own debug text first; F2-F4 must leave it showing.
    # Players read F1 only once the rebuilt match's countdown is over.
    Invoke-ConsoleLine 'overlay none'
    Close-Console
    Start-Sleep -Seconds 4
    $driver::Tap($f1, $false); Start-Sleep -Milliseconds 300
    $driver::Tap($f2, $false); $driver::Tap($f3, $false); $driver::Tap($f4, $false)
    Start-Sleep -Milliseconds 600
    Save-Shot '08-overlay-keys.png'

    # 6: goto, status, restart+goto, cheats and the screenshot command, as a script.
    Invoke-Checked 'exec check_drive' $false 300
    if (-not (Wait-Transcript 'check-drive-done' 40)) { throw 'check_drive.cfg did not finish.' }
    Save-Shot '09-after-check-drive.png'
    if (-not (Test-Path -LiteralPath $commandShot -PathType Leaf)) {
        throw "screenshot did not write $commandShot"
    }
    Copy-Item -LiteralPath $commandShot -Destination (Join-Path $output '10-screenshot-command.png') -Force
    $bytes = [IO.File]::ReadAllBytes($commandShot)
    $signature = [byte[]](0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A)
    for ($i = 0; $i -lt 8; ++$i) {
        if ($bytes[$i] -ne $signature[$i]) { throw 'drive.png has no PNG signature.' }
    }
    $pngWidth = ([int]$bytes[16] -shl 24) -bor ([int]$bytes[17] -shl 16) -bor ([int]$bytes[18] -shl 8) -bor [int]$bytes[19]
    $pngHeight = ([int]$bytes[20] -shl 24) -bor ([int]$bytes[21] -shl 16) -bor ([int]$bytes[22] -shl 8) -bor [int]$bytes[23]
    $client = $driver::ClientSize($window)
    if ($pngWidth -ne $client[0] -or $pngHeight -ne $client[1]) {
        throw "drive.png is ${pngWidth}x${pngHeight}; the client area is $($client[0])x$($client[1])."
    }
    $image = [Drawing.Bitmap]::FromFile($commandShot)
    try {
        $a = $image.GetPixel([int]($pngWidth * 0.25), [int]($pngHeight * 0.5))
        $b = $image.GetPixel([int]($pngWidth * 0.75), [int]($pngHeight * 0.15))
        $c = $image.GetPixel([int]($pngWidth * 0.5), [int]($pngHeight * 0.9))
        if ($a.ToArgb() -eq $b.ToArgb() -and $b.ToArgb() -eq $c.ToArgb()) {
            throw 'drive.png is one flat colour.'
        }
    }
    finally { $image.Dispose() }

    # 7: the pause menu unwinds for a queued stage.
    $driver::Tap($escape, $false); Start-Sleep -Milliseconds 700
    Save-Shot '13-pause-menu.png'
    Invoke-ConsoleLine 'stage close_quarters' 3000
    Save-Shot '14-unwound-close-quarters.png'
    Invoke-ConsoleLine 'status'
    Save-Shot '15-status-after-unwind.png'
    Close-Console

    # 8: back to the menus, and a match started from them, as a script.
    Invoke-Checked 'exec check_menus' $false 300
    if (-not (Wait-Transcript 'check-menus-done' 40)) { throw 'check_menus.cfg did not finish.' }
    Save-Shot '17-started-from-menus.png'

    # 9: a lone Alt tap, which must not enter the system menu loop, then quit.
    if (-not $SkipAltProbe) {
        $before = Count-Transcript '] status'
        $driver::Tap($alt, $false); Start-Sleep -Milliseconds 500
        Invoke-ConsoleLine 'status'
        if ((Count-Transcript '] status') -gt $before) {
            Set-Content -LiteralPath (Join-Path $output 'alt-probe.txt') -Value 'A lone Alt tap did not freeze the game.'
            Close-Console
        }
        else {
            Set-Content -LiteralPath (Join-Path $output 'alt-probe.txt') -Value 'A lone Alt tap entered the system menu loop: the keys after it were lost.'
            $driver::Tap($escape, $false); Start-Sleep -Milliseconds 300
            $driver::Tap($escape, $false); Start-Sleep -Milliseconds 300
        }
    }

    Invoke-ConsoleLine 'quit' 0
    Wait-Exit $process 15 'The game'

    $log = Get-Transcript
    Assert-Transcript @(
        'match: starting movement_course, 2 player(s)',
        'match: stage movement_course, 2 player(s)',
        'overlays: player collision camera paint',
        '[F9] feedback toggle',
        'feedback = off',
        'debug menu: open',
        '[menu] overlay.collision toggle',
        'overlay.collision = off',
        'debug menu: closed',
        '[menu] players 3',
        'players 3: queued; rebuilds when the debug menu closes',
        'match: movement_course, 3 player(s)',
        '[F2] overlay collision',
        '[F3] overlay camera',
        '[F4] overlay paint',
        '] exec check_drive',
        'check_drive.cfg:2] goto high_step',
        'goto high_step: P1 at (4404, 535)',
        'goto running_gap: applies after the queued restart',
        'goto running_gap: P1 at (',
        'god = on',
        'ammo = inf',
        'respawn: P1 P2',
        'screenshot: ',
        'drive.png (',
        'script check_drive.cfg done',
        '] stage close_quarters',
        'match: close_quarters',
        'stack depth',
        'menu: leaving the match',
        'match: starting movement_course',
        'script check_menus.cfg done',
        '] quit') 'phase-1/2'
    # Staged, then applied once: the rebuild follows the menu closing.
    $picked = $log.IndexOf('[menu] players 3')
    $rebuilt = $log.IndexOf('match: movement_course, 3 player(s)')
    $closed = $log.IndexOf('debug menu: closed', $picked)
    if (-not ($picked -lt $closed -and $closed -lt $rebuilt)) {
        throw 'The staged roster did not wait for Apply.'
    }
    # The player settled on the high step: centre (4430, 600) on the ground.
    if (-not ($log -match 'P1 kbm team a \w+\s+at \(4430, (59[5-9]|600)\) on_ground')) {
        throw 'After goto high_step, P1 is not at rest on the high step.'
    }
    # The queued goto applied after the restart's rebuild, not before it.
    $waited = $log.IndexOf('goto running_gap: applies after the queued restart')
    $placed = $log.IndexOf('goto running_gap: P1 at (')
    if (-not ($waited -lt $placed)) {
        throw 'The goto queued behind a restart did not wait for the rebuild.'
    }

    # 10: a narrow window, four seats, both bottom panels as wide as they get,
    # paused in the countdown with the opening cards up.
    $narrowProfile = Join-Path $output 'narrow-profile'
    Remove-Item -LiteralPath $narrowProfile -Recurse -Force -ErrorAction SilentlyContinue
    $narrowSaveDirectory = Join-Path $narrowProfile 'ColourWars'
    $null = New-Item -ItemType Directory -Path $narrowSaveDirectory -Force
    Set-Content -LiteralPath (Join-Path $narrowSaveDirectory 'save_data.json') -Encoding ASCII -Value `
        '{"resolution":{"x":1280,"y":720},"fullscreen":false,"team_palette":"random","aim_sensitivity":3,"music_volume":100,"sfx_volume":100,"view_shake":true,"aim_marker":false}'
    $narrowShot = Join-Path $narrowSaveDirectory 'screens/narrow.png'
    $transcript = Join-Path $output 'narrow.log'
    Remove-Item -LiteralPath $transcript -ErrorAction SilentlyContinue
    $window = [IntPtr]::Zero
    $process = Start-Game $narrowProfile @('+logfile', ('"' + $transcript + '"'), '+stage', 'movement_course',
        '+players', '4', '+overlay', 'all', '+ammo', 'inf', '+timescale', '0.5', '+pause', 'on',
        '+screenshot', 'narrow')
    Start-Sleep -Seconds $StartupSeconds
    $window = Find-Window $process
    Confirm-Focus; Start-Sleep -Milliseconds 400
    if (-not (Wait-Transcript 'narrow.png (' 10)) { throw "+screenshot narrow did not write $narrowShot" }
    Copy-Item -LiteralPath $narrowShot -Destination (Join-Path $output 'narrow-panels.png') -Force
    $driver::Tap($f10, $false); Start-Sleep -Milliseconds 600
    Save-Shot 'narrow-debug-menu.png'
    $driver::Tap($f10, $false); Start-Sleep -Milliseconds 400
    Invoke-ConsoleLine 'quit' 0
    Wait-Exit $process 15 'The narrow game'
    $transcript = Join-Path $output 'console.log'

    Write-Host "PASS debug console driven: launch, status, F9-F10, debug menu, staging, overlays, check_drive.cfg (goto direct and behind a restart, cheats, screenshot), unwinding, check_menus.cfg, quit; narrow-window panels in narrow-panels.png. Screenshots: $output"
}
finally {
    if ($process -ne $null -and -not $process.HasExited) { $process.Kill() }
}
}

# ---------------------------------------------------------------- phase 3
#   p1. launch +seed 7 +stage movement_course +players 4 +bot all paint:
#       seat 1 keyboard/mouse, three idle dummies driven by paint bots;
#       then, still launch statements, the scripts below
#   p2. check_p3_tune.cfg: status (three seats say bot paint), a live value
#       (tune tuning.player.max_velocity.x, twice the file's) with an in-game
#       screenshot, tune.diff, tune.reset
#   p3. two presentation edits startup's resource checks refuse
#       (camera.reference_height, match.countdown_size.y) - launch
#       statements, because a refused line would stop a script
#   p4. (-SaveToSource) tune.save tuning on the source tree, git checks the
#       diff is that one value; the bytes read before the save are written
#       back (in a finally, so a failed run restores them too) and git diff
#       --quiet confirms the file is clean
#   p5. check_p3_replay.cfg: record drive 600, wait record, play drive, wait
#       replay, checksum - no key or focus change (a console or focus loss
#       mid-recording is a suspend)
#   p6. F10 tuning > tuning > player, Right on x_acceleration
#   p7. ` quit
if (-not $Phase4Only) {
$process = $null
$window = [IntPtr]::Zero
$transcript = Join-Path $output 'phase3.log'
Remove-Item -LiteralPath $transcript -ErrorAction SilentlyContinue
$phase3Profile = Join-Path $output 'phase3-profile'
Remove-Item -LiteralPath $phase3Profile -Recurse -Force -ErrorAction SilentlyContinue
$null = New-Item -ItemType Directory -Path $phase3Profile -Force

Write-Script $phase3Profile 'check_p3_tune' @(
    '// Phase 3: status with the bots, then a live tuning edit, tune.diff and tune.reset.',
    'wait match',
    'wait 60',
    'screenshot p3-01-bots-4p',
    'status',
    "tune tuning.player.max_velocity.x $fastToken",
    '// The bots run twice as fast for two seconds.',
    'wait 120',
    'screenshot p3-04-tuned-speed',
    'tune.diff',
    'tune.reset',
    'echo check-p3-tune-done')
Write-Script $phase3Profile 'check_p3_replay' @(
    '// Phase 3: record 600 updates, play them back, compare.',
    'record drive 600',
    'wait record',
    'screenshot p3-07-recorded',
    'play drive',
    'wait replay',
    'checksum',
    'screenshot p3-08-replay-end',
    'pause off',
    'echo check-p3-replay-done')

try {
    $arguments = @('+logfile', ('"' + $transcript + '"'), '+seed', '7', '+stage', 'movement_course',
        '+players', '4', '+bot', 'all', 'paint', '+exec', 'check_p3_tune',
        '+tune', 'presentation.camera.reference_height', '-1',
        '+tune', 'presentation.match.countdown_size.y', '61')
    if (-not $SaveToSource) { $arguments += @('+exec', 'check_p3_replay') }
    $process = Start-Game $phase3Profile $arguments
    Start-Sleep -Seconds $StartupSeconds
    $window = Find-Window $process
    $driver::Focus($window); Start-Sleep -Milliseconds 1000
    if (-not (Wait-Transcript 'check-p3-tune-done' 60)) { throw 'check_p3_tune.cfg did not finish.' }

    # p4
    if ($SaveToSource) {
        $sourceTuning = Join-Path $repo 'game/content/tuning.json'
        $stagedTuning = Join-Path (Split-Path $executable) 'tuning.json'
        & git -C $repo diff --quiet -- game/content/tuning.json
        $worktreeClean = $LASTEXITCODE -eq 0
        & git -C $repo diff --cached --quiet -- game/content/tuning.json
        if (-not $worktreeClean -or $LASTEXITCODE -ne 0) {
            throw 'game/content/tuning.json has local changes; -SaveToSource refuses to touch it.'
        }
        $before = [IO.File]::ReadAllBytes($sourceTuning)
        $stagedBefore = [IO.File]::ReadAllBytes($stagedTuning)
        try {
            Invoke-Checked "tune tuning.player.max_velocity.x $savedToken"
            Invoke-Checked 'tune.save tuning' $false
            Save-Shot 'p3-06-tune-save.png'
            Close-Console
            if (-not (Wait-Transcript 'tune.save: 1 value(s) written to' 5)) { throw 'tune.save did not report a write.' }
            $numstat = (& git -C $repo diff --numstat -- game/content/tuning.json) -join "`n"
            $patch = (& git -C $repo diff -U0 -- game/content/tuning.json) -join "`n"
            Set-Content -LiteralPath (Join-Path $output 'p3-tune-save.diff') -Value $patch -Encoding UTF8
            $changed = @($patch -split "`n" | Where-Object { $_ -match '^[+-]\s' -or $_ -match '^[+-]\s*"' })
            $savedLine = $velocityLine.Replace('"x": ' + $velocityToken + ',', '"x": ' + $savedToken + ',')
            if ($numstat -notmatch '^1\s+1\s' -or $changed.Count -ne 2 -or
                -not $changed[0].Contains($velocityLine) -or -not $changed[1].Contains($savedLine)) {
                throw "tune.save changed more than the one value: $patch"
            }
        }
        finally {
            # Byte for byte: the bytes read before the save, not a git checkout,
            # whether or not the checks above passed.
            [IO.File]::WriteAllBytes($sourceTuning, $before)
            [IO.File]::WriteAllBytes($stagedTuning, $stagedBefore)
            # The rewrite changed the file's timestamp: refresh git's stat cache
            # for it (the blob is the committed one, so nothing is staged).
            & { $ErrorActionPreference = 'Continue'; & git -C $repo add -- game/content/tuning.json 2>&1 | Out-Null }
        }
        $after = [IO.File]::ReadAllBytes($sourceTuning)
        if ([Convert]::ToBase64String($before) -ne [Convert]::ToBase64String($after)) {
            throw 'tuning.json was not restored byte for byte.'
        }
        & git -C $repo diff --quiet -- game/content/tuning.json
        if ($LASTEXITCODE -ne 0) { throw 'git still sees a change in tuning.json after the restore.' }
        Invoke-Checked 'reload' $false 1500
        if (-not (Wait-Transcript 'reload: applied tuning.json and presentation.json' 5)) {
            throw 'reload did not apply the restored tuning.json.'
        }
        Invoke-Checked 'exec check_p3_replay' $false 300
    }

    # p5: no key and no focus change until the replay has finished.
    if (-not (Wait-Transcript 'check-p3-replay-done' 90)) {
        Save-Shot 'p3-08-replay-timeout.png'
        throw 'check_p3_replay.cfg did not finish.'
    }
    Save-Shot 'p3-09-replay-done.png'

    # p6: root rows match, player, view, time, tuning, replay, content,
    # console; tuning opens on tuning, presentation, tune.diff, tune.reset,
    # tune.save; the tuning file on input, match, paint, player, weapons,
    # projectiles; player on size, max_velocity, x_initial_velocity,
    # x_acceleration, ...
    $driver::Tap($f10, $false); Start-Sleep -Milliseconds 500
    foreach ($i in 1..4) { $driver::Tap($down, $true) }
    $driver::Tap($enter, $false); Start-Sleep -Milliseconds 300
    $driver::Tap($enter, $false); Start-Sleep -Milliseconds 300
    foreach ($i in 1..3) { $driver::Tap($down, $true) }
    $driver::Tap($enter, $false); Start-Sleep -Milliseconds 300
    foreach ($i in 1..3) { $driver::Tap($down, $true) }
    $driver::Tap($right, $true); Start-Sleep -Milliseconds 500
    Save-Shot 'p3-10-menu-tuning.png'
    $driver::Tap($f10, $false); Start-Sleep -Milliseconds 400
    Invoke-Checked 'tune.reset'

    Invoke-ConsoleLine 'quit' 0
    Wait-Exit $process 15 'The phase-3 game'

    $log = Get-Transcript
    Assert-Transcript @(
        'launch: seed 7',
        'bot all paint: applies to the next match',
        'match: starting movement_course, 4 player(s)',
        ' idle team b sprayer bot paint',
        "tune: tuning.player.max_velocity.x $velocityToken -> $fastToken (live)",
        "tuning.player.max_velocity.x  $velocityToken -> $fastToken  [live]",
        'tune.reset: 1 value(s) back to the file''s',
        "'reference_height' must be finite and greater than zero",
        'match.countdown_size must leave room below the GET READY heading',
        'record drive: restarting the match to record from its first update',
        'record drive: recording 600 updates',
        '(600 updates, checksum',
        'replay drive: playing 600 updates',
        'replay drive: MATCH (600 updates, 10 checkpoints',
        'checksum ',
        'script check_p3_replay.cfg done',
        '[menu] tune tuning.player.x_acceleration next',
        "tune: tuning.player.x_acceleration $accelerationToken -> ",
        '] quit') 'phase-3'
    if ($log.Contains('DIVERGED')) { throw 'The driven replay diverged.' }
    $bots = ([regex]::Matches($log, [regex]::Escape(' bot paint'))).Count
    if ($bots -lt 3) { throw "status listed $bots paint bots; three expected." }
    $replay = Join-Path $phase3Profile 'ColourWars/replays/drive.json'
    if (-not (Test-Path -LiteralPath $replay -PathType Leaf)) { throw "No replay at $replay" }
    Copy-Item -LiteralPath $replay -Destination (Join-Path $output 'p3-drive-replay.json') -Force
    foreach ($shot in Get-ChildItem -LiteralPath (Join-Path $phase3Profile 'ColourWars/screens') -Filter 'p3-*.png') {
        Copy-Item -LiteralPath $shot.FullName -Destination (Join-Path $output $shot.Name) -Force
    }
    Write-Host "PASS debug console driven, phase 3: four-seat paint-bot match, status, live tune and tune.diff/reset$(if ($SaveToSource) { ', tune.save on the source file (one value, checked with git; restored byte for byte)' } else { '' }), record 600 and play MATCH (as scripts), tuning menu Right; screenshots p3-*.png in $output"
}
finally {
    if ($process -ne $null -and -not $process.HasExited) { $process.Kill() }
}
}

# ---------------------------------------------------------------- phase 4
#   q1. +exec review_feedback_ab +quit, with an autoexec.cfg in the profile:
#       autoexec runs before the launch statements; six PNGs, each -on pair
#       differs from its -off (the strip is off, so only the world can);
#       three checksums, each with bursts on screen at the expected spacing.
#   q2. +exec station_tour +quit: ten PNGs.
#   q3. +exec bots_4p +quit: one PNG.
#   q4. one launch on an isolated profile and TEMP: bind shift+f5 at the
#       console, Shift+F5, F5 still restarts; a tune edit writes no candidate
#       file to TEMP (it is parsed in memory); console.live 1,
#       typing in the live console leaves P1 where it was while the match
#       runs, Escape closes it without a pause menu; overlay perf and perf.
#   q5. a second launch on the same profile: binds.cfg is read back, and the
#       history recalls the last line of the first session (quit).
if (-not $Phase3Only) {
$process = $null

function Start-ScriptRun([string]$name, [string]$profile, [int]$seconds) {
    $script:transcript = Join-Path $output "p4-$name.log"
    Remove-Item -LiteralPath $script:transcript -ErrorAction SilentlyContinue
    $script:window = [IntPtr]::Zero
    $run = Start-Game $profile @('+logfile', ('"' + $script:transcript + '"'), '+exec', $name, '+quit')
    $script:process = $run
    $script:window = Find-Window $run
    $driver::Focus($script:window)
    Wait-Exit $run $seconds "exec $name"
    $folder = @(Get-ChildItem -LiteralPath (Join-Path $profile 'ColourWars/screens') -Directory)
    if ($folder.Count -ne 1) { throw "exec $name left $($folder.Count) screenshot folders." }
    $evidence = Join-Path $output "p4-$name"
    Remove-Item -LiteralPath $evidence -Recurse -Force -ErrorAction SilentlyContinue
    Copy-Item -LiteralPath $folder[0].FullName -Destination $evidence -Recurse
    return $folder[0].FullName
}

function Test-SameBytes([string]$a, [string]$b) {
    return (Get-FileHash -LiteralPath $a).Hash -eq (Get-FileHash -LiteralPath $b).Hash
}

try {
    # q1: one command, one review.
    $reviewProfile = Join-Path $output 'p4-review-profile'
    Remove-Item -LiteralPath $reviewProfile -Recurse -Force -ErrorAction SilentlyContinue
    $null = New-Item -ItemType Directory -Path $reviewProfile -Force
    Write-Script $reviewProfile 'autoexec' @('// Runs before the launch statements.', 'echo autoexec-ran')
    $folder = Start-ScriptRun 'review_feedback_ab' $reviewProfile 180
    $log = Get-Transcript
    Assert-Transcript @('autoexec-ran', 'launch: exec review_feedback_ab',
        'review_feedback_ab: done (3 frames x on/off)', 'script review_feedback_ab.cfg done',
        'launch: quit') 'review'
    if ($log.IndexOf('autoexec-ran') -gt $log.IndexOf('launch: ')) { throw 'autoexec.cfg did not run first.' }
    if ($log -match '(?m)^error: ') { throw 'The review logged an error.' }
    $shots = @(Get-ChildItem -LiteralPath $folder -Filter '*.png')
    if ($shots.Count -ne 6) { throw "review_feedback_ab wrote $($shots.Count) PNGs, not 6." }
    foreach ($frame in 't040', 't096', 't156') {
        $on = Join-Path $folder "$frame-on.png"; $off = Join-Path $folder "$frame-off.png"
        if (-not (Test-Path $on) -or -not (Test-Path $off)) { throw "Missing $frame pair." }
        if (Test-SameBytes $on $off) { throw "${frame}: feedback on and off drew the same picture." }
    }
    $checksums = @([regex]::Matches($log, 'checksum ([0-9a-f]{16}) at update (\d+), paint [^\r\n]*, bursts (\d+)'))
    if ($checksums.Count -ne 3) { throw "The review logged $($checksums.Count) checksums, not 3." }
    # Checksums count from the build, including the countdown; the named
    # t040/t096/t156 frames count from GO. Check their relative spacing.
    $expectedOffsets = @(0, 56, 116)
    $firstUpdate = [int]$checksums[0].Groups[2].Value
    $report = @()
    for ($frame = 0; $frame -lt $checksums.Count; ++$frame) {
        $sum = $checksums[$frame]
        if ([int]$sum.Groups[3].Value -lt 1) { throw "No burst on screen at update $($sum.Groups[2].Value)." }
        if ([int]$sum.Groups[2].Value -ne $firstUpdate + $expectedOffsets[$frame]) {
            throw "Review frame $frame is at update $($sum.Groups[2].Value), expected offset $($expectedOffsets[$frame]) from $firstUpdate."
        }
        $report += "update $($sum.Groups[2].Value): checksum $($sum.Groups[1].Value), bursts $($sum.Groups[3].Value)"
    }
    Set-Content -LiteralPath (Join-Path $output 'p4-review-checksums.txt') -Value $report
    $report | ForEach-Object { Write-Host "review checksums: $_" }

    # q2, q3
    $tourProfile = Join-Path $output 'p4-tour-profile'
    Remove-Item -LiteralPath $tourProfile -Recurse -Force -ErrorAction SilentlyContinue
    $null = New-Item -ItemType Directory -Path $tourProfile -Force
    $folder = Start-ScriptRun 'station_tour' $tourProfile 120
    if (@(Get-ChildItem -LiteralPath $folder -Filter '*.png').Count -ne 10) { throw 'station_tour did not write 10 PNGs.' }
    Assert-Transcript @('station_tour: done (10 stations)') 'station tour'
    $botsProfile = Join-Path $output 'p4-bots-profile'
    Remove-Item -LiteralPath $botsProfile -Recurse -Force -ErrorAction SilentlyContinue
    $null = New-Item -ItemType Directory -Path $botsProfile -Force
    $folder = Start-ScriptRun 'bots_4p' $botsProfile 120
    if (@(Get-ChildItem -LiteralPath $folder -Filter '*.png').Count -ne 1) { throw 'bots_4p did not write its PNG.' }
    Assert-Transcript @('bots_4p: done', 'checksum ') 'bots_4p'

    # q4: binds, a tune edit in an isolated TEMP, the live console, perf.
    $liveProfile = Join-Path $output 'p4-live-profile'
    Remove-Item -LiteralPath $liveProfile -Recurse -Force -ErrorAction SilentlyContinue
    $temp = Join-Path $liveProfile 'temp'
    $null = New-Item -ItemType Directory -Path $temp -Force
    $transcript = Join-Path $output 'p4-live.log'
    Remove-Item -LiteralPath $transcript -ErrorAction SilentlyContinue
    $window = [IntPtr]::Zero
    $process = Start-Game $liveProfile @('+logfile', ('"' + $transcript + '"'), '+stage', 'movement_course',
        '+players', '1', '+timescale', '4') $temp
    Start-Sleep -Seconds $StartupSeconds
    $window = Find-Window $process
    Confirm-Focus; Start-Sleep -Milliseconds 400

    Invoke-Checked 'timescale 1'
    Invoke-Checked 'bind shift+f5 echo bound-shift-f5'
    Confirm-Focus
    $driver::Chord($shift, $f5); Start-Sleep -Milliseconds 500
    if (-not (Wait-Transcript '[SHIFT+F5] echo bound-shift-f5' 5)) { throw 'Shift+F5 did not run its bind.' }
    if ((Get-Transcript).Contains('[F5] restart')) { throw 'Shift+F5 also ran plain F5.' }
    $driver::Tap($f5, $false); Start-Sleep -Milliseconds 800
    if (-not (Wait-Transcript '[F5] restart' 5)) { throw 'F5 no longer restarts.' }
    Start-Sleep -Seconds 4

    # A tune edit is parsed in memory and writes nothing to (this run's) TEMP.
    Invoke-Checked "tune tuning.player.max_velocity.x $fastToken"
    Invoke-Checked 'tune.reset'
    if (@(Get-ChildItem -LiteralPath $temp -Filter 'ColourWars-tune-*').Count -ne 0) { throw 'A tune candidate was left in TEMP.' }

    # The live console: the match runs, the keyboard seat stands still.
    Invoke-Checked 'console.live 1'
    Confirm-Focus
    $driver::Tap($grave, $false); Start-Sleep -Milliseconds 300
    $driver::Type('status'); $driver::Tap($enter, $false); Start-Sleep -Milliseconds 400
    $driver::Type('ddddddddaaaa '); Start-Sleep -Milliseconds 1000
    Save-Shot 'p4-live-console.png'
    $driver::Tap($enter, $false); Start-Sleep -Milliseconds 200
    $driver::Type('status'); $driver::Tap($enter, $false); Start-Sleep -Milliseconds 400
    $driver::Tap($escape, $false); Start-Sleep -Milliseconds 800
    $driver::Tap($grave, $false); Start-Sleep -Milliseconds 300
    $driver::Type('status'); $driver::Tap($enter, $false); Start-Sleep -Milliseconds 400
    $driver::Type('overlay perf'); $driver::Tap($enter, $false); Start-Sleep -Milliseconds 300
    $driver::Type('perf'); $driver::Tap($enter, $false); Start-Sleep -Milliseconds 300
    $driver::Tap($grave, $false); Start-Sleep -Milliseconds 1500
    Save-Shot 'p4-perf-overlay.png'
    Invoke-ConsoleLine 'quit' 0
    Wait-Exit $process 15 'The live-console game'

    $log = Get-Transcript
    Assert-Transcript @('console.live = on', 'console: live (the match keeps running',
        'console: live closed', 'overlay.perf = on', 'CPU ms    min    avg    max') 'live'
    $states = @([regex]::Matches($log, 'P1 kbm team a \w+\s+at \((-?\d+), (-?\d+)\)'))
    $updates = @([regex]::Matches($log, 'updates (\d+) since the build'))
    $depths = @([regex]::Matches($log, 'stack depth (\d+), in a match'))
    if ($states.Count -lt 3 -or $updates.Count -lt 3) { throw 'The live console did not print three statuses.' }
    $n = $states.Count
    if ($states[$n - 3].Groups[1].Value -ne $states[$n - 2].Groups[1].Value) {
        throw "P1 moved while the live console had the keyboard: x $($states[$n - 3].Groups[1].Value) -> $($states[$n - 2].Groups[1].Value)."
    }
    if ([int]$updates[$updates.Count - 2].Groups[1].Value -le [int]$updates[$updates.Count - 3].Groups[1].Value + 30) {
        throw 'The match did not keep running under the live console.'
    }
    if ($depths[$depths.Count - 1].Groups[1].Value -ne '2') { throw 'Escape out of the live console opened a menu.' }
    $frame = [regex]::Match($log, 'frame\s+([0-9.]+)\s+([0-9.]+)\s+([0-9.]+)')
    if (-not $frame.Success -or [double]::Parse($frame.Groups[2].Value, $invariantCulture) -le 0) {
        throw 'perf printed no frame time.'
    }
    Write-Host "NOTE live console: P1 x $($states[$n - 3].Groups[1].Value) before and after typing; updates $($updates[$updates.Count - 3].Groups[1].Value) -> $($updates[$updates.Count - 2].Groups[1].Value); perf $($frame.Value)"

    # q5: the same profile again: binds.cfg and history.txt carry over.
    $transcript = Join-Path $output 'p4-relaunch.log'
    Remove-Item -LiteralPath $transcript -ErrorAction SilentlyContinue
    $window = [IntPtr]::Zero
    $process = Start-Game $liveProfile @('+logfile', ('"' + $transcript + '"'), '+binds') $temp
    Start-Sleep -Seconds $StartupSeconds
    $window = Find-Window $process
    Confirm-Focus; Start-Sleep -Milliseconds 400
    Assert-Transcript @('binds: 1 change(s) from', 'SHIFT+F5  echo bound-shift-f5  *') 'relaunch'
    # Up recalls the last line typed in the first session, which was quit.
    $driver::Tap($grave, $false); Start-Sleep -Milliseconds 300
    $driver::Tap($up, $true); Start-Sleep -Milliseconds 200
    Save-Shot 'p4-history-up.png'
    $driver::Tap($enter, $false)
    Wait-Exit $process 15 'The relaunched game (history Up, Enter: quit)'
    Assert-Transcript @('] quit') 'relaunch'

    Write-Host "PASS debug console driven, phase 4: +exec review_feedback_ab (6 PNGs, on/off differ, bursts on screen; autoexec first), station_tour (10), bots_4p (1); bind Shift+F5 and F5; tune writes nothing to TEMP; live console (P1 still, match running, Escape without a pause menu); perf; binds.cfg and history.txt across a restart. Evidence: $output"
}
finally {
    if ($process -ne $null -and -not $process.HasExited) { $process.Kill() }
}
}
