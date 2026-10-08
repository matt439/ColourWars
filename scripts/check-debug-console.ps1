[CmdletBinding()]
param(
    [ValidateSet('x64-debug')]
    [string]$Preset = 'x64-debug',
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '../out/validation/debug-console'),
    [ValidateRange(4, 120)]
    [int]$StartupSeconds = 12,
    # The lone-Alt probe is informational (engine report 11) and leaves the
    # window in the system menu loop for a moment if Alt still opens it.
    [switch]$SkipAltProbe,
    # Run only the phase-3 launch (bots, tune, record/play, tuning menu).
    [switch]$Phase3Only,
    # Phase 3 also runs tune.save against the SOURCE game/content/tuning.json,
    # checks with git that exactly one value changed, and writes the saved
    # original bytes back (git checkout would not: it renormalises the line
    # endings of a working copy that mixes CRLF and LF). Refused unless the
    # file is clean in git first.
    [switch]$SaveToSource
)

# Opt-in, local: drives the real Debug executable through the developer console
# and the debug menu with real key events and takes screenshots for a person to
# look at. Not in CI: hosted runners give no reliable foreground focus, and
# Release has no console (scripts/acceptance/debug_console.cpp covers both
# presets on the CPU side). The pad chord (Back+Start) needs a pad and is
# covered there too.
#
#    1. launch +stage movement_course +players 2 +visual posed +overlay all
#    2. ` status Enter                     (console over two panes, overlays on)
#    3. ` F8 F9                            (visual A/B and feedback without the console)
#    4. F10, view, overlay.collision Right (the debug menu steps a variable), Escape, F10
#    5. F10, match, visual, rotating, Escape, Apply   (staged, then one rebuild)
#    6. ` overlay none, F1, then F2 F3 F4  (overlay keys; P1's own F1 text survives them)
#    7. ` goto high_step, ` status         (a station, at rest on its surface)
#       ` restart; goto running_gap        (a goto waits for the queued rebuild)
#    8. ` god 1; ammo inf; respawn all     (cheats on the strip)
#    9. ` screenshot drive                 (a PNG of the window in the isolated profile)
#   10. Escape (pause menu), ` stage close_quarters   (the pause menu unwinds)
#   11. ` menu, ` stage movement_course    (menu-side start)
#   12. Alt alone (informational), then ` quit Enter
#   13. a second launch on a 1280x720 isolated save: four seats, every overlay,
#       ammo inf, timescale 0.5 - the status strip must not share the paint
#       panel's row (narrow-panels.png, for a person to look at)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$executable = Join-Path $repo "out/build/$Preset/game/ColourWars.exe"
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
    throw "Build $Preset first: $executable is missing."
}
$output = [IO.Path]::GetFullPath($OutputDirectory)
$profileDirectory = Join-Path $output 'profile'
$null = New-Item -ItemType Directory -Path $output, $profileDirectory -Force
$transcript = Join-Path $output 'console.log'
Remove-Item -LiteralPath $transcript -ErrorAction SilentlyContinue
$commandShot = Join-Path $profileDirectory 'ColourWars/screens/drive.png'
Remove-Item -LiteralPath $commandShot -ErrorAction SilentlyContinue

Add-Type -AssemblyName System.Drawing
if (-not ('ColourWars.ConsoleDriver3' -as [type])) {
    Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace ColourWars
{
    public static class ConsoleDriver3
    {
        [StructLayout(LayoutKind.Sequential)]
        public struct Rect { public int Left, Top, Right, Bottom; }

        private delegate bool EnumWindowProc(IntPtr window, IntPtr parameter);
        [DllImport("user32.dll")] private static extern bool EnumWindows(EnumWindowProc callback, IntPtr parameter);
        [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);
        [DllImport("user32.dll", CharSet = CharSet.Unicode)] private static extern int GetClassName(IntPtr window, StringBuilder name, int maximum);
        [DllImport("user32.dll")] private static extern IntPtr GetForegroundWindow();
        [DllImport("user32.dll")] private static extern bool AttachThreadInput(uint attach, uint to, bool doAttach);
        [DllImport("kernel32.dll")] private static extern uint GetCurrentThreadId();
        [DllImport("user32.dll")] private static extern bool BringWindowToTop(IntPtr window);
        [DllImport("user32.dll")] private static extern bool SetForegroundWindow(IntPtr window);
        [DllImport("user32.dll")] private static extern IntPtr SetFocus(IntPtr window);
        [DllImport("user32.dll")] private static extern bool GetWindowRect(IntPtr window, out Rect rect);
        [DllImport("user32.dll")] private static extern bool GetClientRect(IntPtr window, out Rect rect);
        [DllImport("user32.dll")] private static extern uint MapVirtualKey(uint code, uint type);
        [DllImport("user32.dll")] private static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);

        public static IntPtr FindGame(int processId)
        {
            IntPtr result = IntPtr.Zero;
            EnumWindows((window, parameter) => {
                uint owner;
                GetWindowThreadProcessId(window, out owner);
                if (owner != processId) return true;
                var name = new StringBuilder(256);
                GetClassName(window, name, name.Capacity);
                if (name.ToString() != "ColourWarsWindowClass") return true;
                result = window;
                return false;
            }, IntPtr.Zero);
            return result;
        }

        // SetForegroundWindow alone is refused for a background process.
        public static void Focus(IntPtr window)
        {
            uint ignored;
            uint foreground = GetWindowThreadProcessId(GetForegroundWindow(), out ignored);
            uint self = GetCurrentThreadId();
            AttachThreadInput(foreground, self, true);
            BringWindowToTop(window);
            SetForegroundWindow(window);
            SetFocus(window);
            AttachThreadInput(foreground, self, false);
        }

        // Held for ~120 ms: a press and release inside one frame is never seen.
        public static void Tap(byte vk, bool extended)
        {
            byte scan = (byte)MapVirtualKey(vk, 0);
            uint flags = extended ? 1u : 0u;
            keybd_event(vk, scan, flags, UIntPtr.Zero);
            Thread.Sleep(120);
            keybd_event(vk, scan, flags | 2u, UIntPtr.Zero);
            Thread.Sleep(60);
        }

        private static void Shifted(byte vk)
        {
            byte shift = (byte)MapVirtualKey(0x10, 0);
            keybd_event(0x10, shift, 0u, UIntPtr.Zero);
            Thread.Sleep(30);
            Tap(vk, false);
            keybd_event(0x10, shift, 2u, UIntPtr.Zero);
            Thread.Sleep(30);
        }

        public static void Type(string text)
        {
            foreach (char c in text)
            {
                if (c == ' ') { Tap(0x20, false); continue; }
                if (c >= 'a' && c <= 'z') { Tap((byte)char.ToUpperInvariant(c), false); continue; }
                if (c >= '0' && c <= '9') { Tap((byte)c, false); continue; }
                if (c == ';') { Tap(0xBA, false); continue; }
                if (c == '_') { Shifted(0xBD); continue; }
                if (c == '.') { Tap(0xBE, false); continue; }
                if (c == '-') { Tap(0xBD, false); continue; }
                if (c == '=') { Tap(0xBB, false); continue; }
                throw new ArgumentException("Unsupported character: " + c);
            }
        }

        public static int[] ClientSize(IntPtr window)
        {
            Rect rect;
            GetClientRect(window, out rect);
            return new int[] { rect.Right - rect.Left, rect.Bottom - rect.Top };
        }

        public static void Screenshot(IntPtr window, string path)
        {
            Rect rect;
            GetWindowRect(window, out rect);
            using (var bitmap = new Bitmap(rect.Right - rect.Left, rect.Bottom - rect.Top))
            {
                using (var graphics = Graphics.FromImage(bitmap))
                {
                    graphics.CopyFromScreen(rect.Left, rect.Top, 0, 0, bitmap.Size);
                }
                bitmap.Save(path, ImageFormat.Png);
            }
        }
    }
}
'@
}

$grave = 0xC0; $enter = 0x0D; $escape = 0x1B; $alt = 0x12
$f1 = 0x70; $f2 = 0x71; $f3 = 0x72; $f4 = 0x73; $f8 = 0x77; $f9 = 0x78; $f10 = 0x79
$down = 0x28; $right = 0x27
$driver = [ColourWars.ConsoleDriver3]

function Get-Transcript {
    if (-not (Test-Path -LiteralPath $transcript)) { return '' }
    # The game holds the transcript open for writing.
    $stream = [IO.File]::Open($transcript, 'Open', 'Read', 'ReadWrite')
    try {
        $reader = New-Object IO.StreamReader($stream)
        return $reader.ReadToEnd()
    }
    finally { $stream.Dispose() }
}

# Runs one console line: open, type, Enter. Commands that close the console
# (rebuilds, goto, respawn, screenshot) leave it closed; others leave it open.
function Invoke-ConsoleLine([string]$line, [int]$settleMilliseconds = 500) {
    $driver::Tap($grave, $false); Start-Sleep -Milliseconds 300
    $driver::Type($line); $driver::Tap($enter, $false)
    Start-Sleep -Milliseconds $settleMilliseconds
}

function Close-Console {
    $driver::Tap($grave, $false); Start-Sleep -Milliseconds 300
}

function Save-Shot([string]$name) {
    $driver::Screenshot($window, (Join-Path $output $name))
}

function Wait-Transcript([string]$text, [int]$seconds) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline) {
        if ((Get-Transcript).Contains($text)) { return $true }
        Start-Sleep -Milliseconds 250
    }
    return $false
}

function Count-Transcript([string]$text) {
    return ([regex]::Matches((Get-Transcript), [regex]::Escape($text))).Count
}

# One console line that must reach the console: the transcript echoes it as
# "] <line>". A toggle lost to a focus change leaves the console open when
# this expects it closed, so the first grave closes it and the line is lost;
# then a second attempt opens it and runs the line. `keepsOpen` lines (those
# that do not ask the console to close) are followed by a closing grave.
function Invoke-Checked([string]$line, [bool]$keepsOpen = $true, [int]$settleMilliseconds = 500) {
    $before = Count-Transcript "] $line"
    for ($attempt = 0; $attempt -lt 2; ++$attempt) {
        Invoke-ConsoleLine $line $settleMilliseconds
        if ((Count-Transcript "] $line") -gt $before) {
            if ($keepsOpen) { Close-Console }
            return
        }
    }
    throw "The console never ran: $line"
}

$originalLocalAppData = $env:LOCALAPPDATA
$process = $null
if (-not $Phase3Only) {
try {
    $env:LOCALAPPDATA = $profileDirectory
    try {
        $arguments = @('+logfile', ('"' + $transcript + '"'), '+stage', 'movement_course',
            '+players', '2', '+visual', 'posed', '+overlay', 'all')
        $process = Start-Process -FilePath $executable -ArgumentList $arguments -PassThru `
            -WorkingDirectory $output
        $null = $process.Handle
    }
    finally {
        $env:LOCALAPPDATA = $originalLocalAppData
    }

    Start-Sleep -Seconds $StartupSeconds
    $window = $driver::FindGame($process.Id)
    if ($window -eq [IntPtr]::Zero) { throw 'The game window did not appear.' }

    $driver::Focus($window); Start-Sleep -Milliseconds 400
    Save-Shot '01-launch-overlays.png'

    # 2-3: phase-1 console, F8 and F9. The first line is checked against the
    # transcript: keys sent in the first moments after focusing can be lost.
    Invoke-Checked 'status' $false
    Save-Shot '02-console-status.png'
    Close-Console
    $driver::Tap($f8, $false); Start-Sleep -Milliseconds 800
    $driver::Tap($f9, $false); Start-Sleep -Milliseconds 800
    Save-Shot '03-after-f8-f9.png'

    # 4: debug menu by F10. Root rows with nothing queued: match, player, view,
    # time, content, console. View, by name: feedback, overlay, overlay.camera,
    # overlay.collision, overlay.paint, overlay.player.
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

    # 5: staging. Match, by name: menu, players, restart, stage, visual,
    # weapon. The visual picker opens on the current value (original after
    # F8), so one Down is rotating.
    $driver::Tap($f10, $false); Start-Sleep -Milliseconds 500
    $driver::Tap($enter, $false); Start-Sleep -Milliseconds 300
    foreach ($i in 1..4) { $driver::Tap($down, $true) }
    $driver::Tap($enter, $false); Start-Sleep -Milliseconds 300
    $driver::Tap($down, $true)
    $driver::Tap($enter, $false); Start-Sleep -Milliseconds 400
    $driver::Tap($escape, $false); Start-Sleep -Milliseconds 400
    Save-Shot '07-staged-apply.png'
    $staged = Get-Transcript
    if ($staged.Contains('match: movement_course, 2 player(s), visual rotating')) {
        throw 'The staged visual rebuilt the match before Apply.'
    }
    $driver::Tap($enter, $false); Start-Sleep -Milliseconds 1500

    # 6: overlay keys.
    # F1 turns on P1's own debug text first; F2-F4 must leave it showing.
    # Players read F1 only once the rebuilt match's countdown is over.
    Invoke-ConsoleLine 'overlay none'
    Close-Console
    Start-Sleep -Seconds 4
    $driver::Tap($f1, $false); Start-Sleep -Milliseconds 300
    $driver::Tap($f2, $false); $driver::Tap($f3, $false); $driver::Tap($f4, $false)
    Start-Sleep -Milliseconds 600
    Save-Shot '08-overlay-keys.png'

    # 7: goto, then status once the player has settled.
    Invoke-ConsoleLine 'goto high_step' 1500
    Invoke-ConsoleLine 'status'
    Save-Shot '09-goto-status.png'
    Close-Console
    Save-Shot '10-goto.png'

    # A goto behind a queued rebuild waits for it instead of being undone.
    Invoke-ConsoleLine 'restart; goto running_gap' 1500
    Save-Shot '10b-goto-after-restart.png'

    # 8: cheats.
    Invoke-ConsoleLine 'god 1; ammo inf; respawn all' 800
    Save-Shot '11-cheats.png'

    # 9: the screenshot command.
    Invoke-ConsoleLine 'screenshot drive' 1500
    if (-not (Test-Path -LiteralPath $commandShot -PathType Leaf)) {
        throw "screenshot did not write $commandShot"
    }
    Copy-Item -LiteralPath $commandShot -Destination (Join-Path $output '12-screenshot-command.png') -Force
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

    # 10: the pause menu unwinds for a queued stage.
    $driver::Tap($escape, $false); Start-Sleep -Milliseconds 700
    Save-Shot '13-pause-menu.png'
    Invoke-ConsoleLine 'stage close_quarters' 3000
    Save-Shot '14-unwound-close-quarters.png'
    Invoke-ConsoleLine 'status'
    Save-Shot '15-status-after-unwind.png'
    Close-Console

    # 11: back to the menus, and a match started from them.
    Invoke-ConsoleLine 'menu' 1500
    Save-Shot '16-menus.png'
    Invoke-ConsoleLine 'stage movement_course' 3000
    Save-Shot '17-started-from-menus.png'

    # 12: a lone Alt tap (evidence for engine report 11), then quit.
    if (-not $SkipAltProbe) {
        $before = ([regex]::Matches((Get-Transcript), [regex]::Escape('] status'))).Count
        $driver::Tap($alt, $false); Start-Sleep -Milliseconds 500
        Invoke-ConsoleLine 'status'
        $after = ([regex]::Matches((Get-Transcript), [regex]::Escape('] status'))).Count
        if ($after -gt $before) {
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
    if (-not $process.WaitForExit(15000)) { throw 'The game did not quit.' }
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) { throw "The game exited with $($process.ExitCode)." }

    $log = Get-Transcript
    $expected = @(
        'match: starting movement_course, 2 player(s), visual posed',
        'match: stage movement_course, 2 player(s), visual posed',
        'overlays: player collision camera paint',
        '[F8] visual next',
        'match: movement_course, 2 player(s), visual original',
        '[F9] feedback toggle',
        'feedback = off',
        'debug menu: open',
        '[menu] overlay.collision toggle',
        'overlay.collision = off',
        'debug menu: closed',
        '[menu] visual rotating',
        'match: movement_course, 2 player(s), visual rotating',
        '[F2] overlay collision',
        '[F3] overlay camera',
        '[F4] overlay paint',
        'goto high_step: P1 at (4404, 535)',
        'goto running_gap: applies after the queued restart',
        'goto running_gap: P1 at (',
        'god = on',
        'ammo = inf',
        'respawn: P1 P2',
        'screenshot: ',
        'drive.png (',
        '] stage close_quarters',
        'match: close_quarters',
        'stack depth',
        'menu: leaving the match',
        'match: starting movement_course',
        '] quit')
    foreach ($line in $expected) {
        if (-not $log.Contains($line)) { throw "The transcript lacks: $line" }
    }
    # Staged, then applied once: the rebuild follows the menu closing.
    $picked = $log.IndexOf('[menu] visual rotating')
    $rebuilt = $log.IndexOf('match: movement_course, 2 player(s), visual rotating')
    $closed = $log.IndexOf('debug menu: closed', $picked)
    if (-not ($picked -lt $closed -and $closed -lt $rebuilt)) {
        throw 'The staged visual did not wait for Apply.'
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

    # 13: a narrow window, four seats, both bottom panels as wide as they get.
    $narrowProfile = Join-Path $output 'narrow-profile'
    $narrowSaveDirectory = Join-Path $narrowProfile 'ColourWars'
    $null = New-Item -ItemType Directory -Path $narrowSaveDirectory -Force
    Set-Content -LiteralPath (Join-Path $narrowSaveDirectory 'save_data.json') -Encoding ASCII -Value `
        '{"resolution":{"x":1280,"y":720},"fullscreen":false,"team_palette":"random","aim_sensitivity":3,"music_volume":100,"sfx_volume":100}'
    $narrowShot = Join-Path $narrowSaveDirectory 'screens/narrow.png'
    Remove-Item -LiteralPath $narrowShot -ErrorAction SilentlyContinue
    $env:LOCALAPPDATA = $narrowProfile
    try {
        $process = Start-Process -FilePath $executable -PassThru -WorkingDirectory $output `
            -ArgumentList @('+stage', 'movement_course', '+players', '4', '+overlay', 'all',
                '+ammo', 'inf', '+timescale', '0.5', '+screenshot', 'narrow')
        $null = $process.Handle
    }
    finally {
        $env:LOCALAPPDATA = $originalLocalAppData
    }
    Start-Sleep -Seconds $StartupSeconds
    $window = $driver::FindGame($process.Id)
    if ($window -eq [IntPtr]::Zero) { throw 'The narrow game window did not appear.' }
    $driver::Focus($window); Start-Sleep -Milliseconds 400
    if (-not (Test-Path -LiteralPath $narrowShot -PathType Leaf)) {
        throw "+screenshot narrow did not write $narrowShot"
    }
    Copy-Item -LiteralPath $narrowShot -Destination (Join-Path $output 'narrow-panels.png') -Force
    Invoke-ConsoleLine 'quit' 0
    if (-not $process.WaitForExit(15000)) { throw 'The narrow game did not quit.' }
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) { throw "The narrow game exited with $($process.ExitCode)." }

    Write-Host "PASS debug console driven: launch, status, F8-F10, debug menu, staging, overlays, goto (direct and behind a restart), cheats, screenshot, unwinding, quit; narrow-window panels in narrow-panels.png. Screenshots: $output"
}
finally {
    if ($process -ne $null -and -not $process.HasExited) { $process.Kill() }
}
}

# ---------------------------------------------------------------- phase 3
#   p1. launch +seed 7 +stage movement_course +players 4 +bot all paint:
#       seat 1 keyboard/mouse, three idle dummies driven by paint bots
#   p2. ` status                         (three seats say bot paint)
#   p3. ` tune tuning.player.max_velocity.x (twice the file's), tune.diff,
#       then tune.reset; two presentation edits startup's resource checks
#       refuse (workshop_player.pixel_size, match.countdown_size.y)
#   p4. (-SaveToSource) tune.save tuning on the source tree, git checks the
#       diff is that one value; the bytes read before the save are written
#       back (in a finally, so a failed run restores them too) and git diff
#       --quiet confirms the file is clean
#   p5. ` record drive 600, wait for record: saved with no key or focus
#       change (a console or focus loss mid-recording is a suspend)
#   p6. ` play drive, wait for replay drive: MATCH
#   p7. F10 tuning > tuning > player, Right on x_acceleration
#   p8. ` quit
$process = $null
$transcript = Join-Path $output 'phase3.log'
Remove-Item -LiteralPath $transcript -ErrorAction SilentlyContinue
$phase3Profile = Join-Path $output 'phase3-profile'
Remove-Item -LiteralPath $phase3Profile -Recurse -Force -ErrorAction SilentlyContinue
$null = New-Item -ItemType Directory -Path $phase3Profile -Force

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


try {
    $env:LOCALAPPDATA = $phase3Profile
    try {
        $arguments = @('+logfile', ('"' + $transcript + '"'), '+seed', '7', '+stage', 'movement_course',
            '+players', '4', '+bot', 'all', 'paint')
        $process = Start-Process -FilePath $executable -ArgumentList $arguments -PassThru `
            -WorkingDirectory $output
        $null = $process.Handle
    }
    finally {
        $env:LOCALAPPDATA = $originalLocalAppData
    }
    Start-Sleep -Seconds $StartupSeconds
    $window = $driver::FindGame($process.Id)
    if ($window -eq [IntPtr]::Zero) { throw 'The phase-3 game window did not appear.' }
    $driver::Focus($window); Start-Sleep -Milliseconds 1000
    Save-Shot 'p3-01-bots-4p.png'
    Start-Sleep -Seconds 3
    Save-Shot 'p3-02-bots-4p-later.png'

    # p2: status with the console left open for the shot.
    Invoke-Checked 'status' $false
    Save-Shot 'p3-03-status.png'
    Close-Console

    # p3: a live value, seen at once (the bots run twice as fast).
    Invoke-Checked "tune tuning.player.max_velocity.x $fastToken"
    Start-Sleep -Seconds 2
    Save-Shot 'p3-04-tuned-speed.png'
    Invoke-Checked 'tune.diff' $false
    Save-Shot 'p3-05-tune-diff.png'
    Close-Console
    Invoke-Checked 'tune.reset'
    # Startup's checks against the loaded frames and fonts judge a presentation
    # edit too: neither is accepted, so a tune.save could never write them.
    Invoke-Checked 'tune presentation.workshop_player.pixel_size next'
    Invoke-Checked 'tune presentation.match.countdown_size.y 61'

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
    }

    # p5: no key and no focus change until the recording is saved.
    Invoke-Checked 'record drive 600' $false 300
    if (-not (Wait-Transcript 'record: saved' 30)) { throw 'record drive 600 did not save.' }
    Save-Shot 'p3-07-recorded.png'

    # p6
    Invoke-Checked 'play drive' $false 300
    if (-not (Wait-Transcript 'replay drive: MATCH' 40)) {
        Save-Shot 'p3-08-replay-end.png'
        throw 'play drive did not report MATCH.'
    }
    Save-Shot 'p3-08-replay-end.png'
    Invoke-Checked 'checksum' $false
    Save-Shot 'p3-09-replay-checksum.png'
    Close-Console
    Invoke-Checked 'pause off'

    # p7: root rows match, player, view, time, tuning, replay, content,
    # console; tuning opens on tuning, presentation, tune.diff, tune.reset,
    # tune.save; the tuning file on match, paint, player, weapons,
    # projectiles; player on size, max_velocity, x_initial_velocity,
    # x_acceleration, ...
    $driver::Tap($f10, $false); Start-Sleep -Milliseconds 500
    foreach ($i in 1..4) { $driver::Tap($down, $true) }
    $driver::Tap($enter, $false); Start-Sleep -Milliseconds 300
    $driver::Tap($enter, $false); Start-Sleep -Milliseconds 300
    $driver::Tap($down, $true); $driver::Tap($down, $true)
    $driver::Tap($enter, $false); Start-Sleep -Milliseconds 300
    foreach ($i in 1..3) { $driver::Tap($down, $true) }
    $driver::Tap($right, $true); Start-Sleep -Milliseconds 500
    Save-Shot 'p3-10-menu-tuning.png'
    $driver::Tap($f10, $false); Start-Sleep -Milliseconds 400
    Invoke-Checked 'tune.reset'

    Invoke-ConsoleLine 'quit' 0
    if (-not $process.WaitForExit(15000)) { throw 'The phase-3 game did not quit.' }
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) { throw "The phase-3 game exited with $($process.ExitCode)." }

    $log = Get-Transcript
    $expected = @(
        'launch: seed 7',
        'bot all paint: applies to the next match',
        'match: starting movement_course, 4 player(s)',
        ' idle team b sprayer bot paint',
        "tune: tuning.player.max_velocity.x $velocityToken -> $fastToken (live)",
        "tuning.player.max_velocity.x  $velocityToken -> $fastToken  [live]",
        'tune.reset: 1 value(s) back to the file''s',
        'must use the authored native texel density',
        'match.countdown_size must leave room below the GET READY heading',
        'record drive: restarting the match to record from its first update',
        'record drive: recording 600 updates',
        '(600 updates, checksum',
        'replay drive: playing 600 updates',
        'replay drive: MATCH (600 updates, 10 checkpoints',
        'checksum ',
        '[menu] tune tuning.player.x_acceleration next',
        "tune: tuning.player.x_acceleration $accelerationToken -> ",
        '] quit')
    foreach ($line in $expected) {
        if (-not $log.Contains($line)) { throw "The phase-3 transcript lacks: $line" }
    }
    if ($log.Contains('DIVERGED')) { throw 'The driven replay diverged.' }
    $bots = ([regex]::Matches($log, [regex]::Escape(' bot paint'))).Count
    if ($bots -lt 3) { throw "status listed $bots paint bots; three expected." }
    $replay = Join-Path $phase3Profile 'ColourWars/replays/drive.json'
    if (-not (Test-Path -LiteralPath $replay -PathType Leaf)) { throw "No replay at $replay" }
    Copy-Item -LiteralPath $replay -Destination (Join-Path $output 'p3-drive-replay.json') -Force
    Write-Host "PASS debug console driven, phase 3: four-seat paint-bot match, status, live tune and tune.diff/reset$(if ($SaveToSource) { ', tune.save on the source file (one value, checked with git; restored byte for byte)' } else { '' }), record 600 and play MATCH, tuning menu Right; screenshots p3-*.png in $output"
}
finally {
    $env:LOCALAPPDATA = $originalLocalAppData
    if ($process -ne $null -and -not $process.HasExited) { $process.Kill() }
}
