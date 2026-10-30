# The Win32 input driver and the launch, focus and transcript helpers of
# scripts/check-debug-console.ps1, which dot-sources this file. They read that
# script's $output, $executable, $StartupSeconds, $window and $transcript at
# call time.

. (Join-Path $PSScriptRoot '..\drawing-references.ps1')
if (-not ('ColourWars.ConsoleDriver4' -as [type])) {
    Add-Type -ReferencedAssemblies $DrawingReferences -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace ColourWars
{
    public static class ConsoleDriver4
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

        public static bool IsForeground(IntPtr window)
        {
            return GetForegroundWindow() == window;
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

        // A key with a modifier held around it (0x10 Shift, 0x11 Ctrl).
        public static void Chord(byte modifier, byte vk)
        {
            byte scan = (byte)MapVirtualKey(modifier, 0);
            keybd_event(modifier, scan, 0u, UIntPtr.Zero);
            Thread.Sleep(40);
            Tap(vk, false);
            keybd_event(modifier, scan, 2u, UIntPtr.Zero);
            Thread.Sleep(40);
        }

        public static void Type(string text)
        {
            foreach (char c in text)
            {
                if (c == ' ') { Tap(0x20, false); continue; }
                if (c >= 'a' && c <= 'z') { Tap((byte)char.ToUpperInvariant(c), false); continue; }
                if (c >= '0' && c <= '9') { Tap((byte)c, false); continue; }
                if (c == ';') { Tap(0xBA, false); continue; }
                if (c == '_') { Chord(0x10, 0xBD); continue; }
                if (c == '+') { Chord(0x10, 0xBB); continue; }
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

$grave = 0xC0; $enter = 0x0D; $escape = 0x1B; $alt = 0x12; $shift = 0x10
$f1 = 0x70; $f2 = 0x71; $f3 = 0x72; $f4 = 0x73; $f5 = 0x74; $f9 = 0x78; $f10 = 0x79
$down = 0x28; $right = 0x27; $up = 0x26
$driver = [ColourWars.ConsoleDriver4]

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
# Keys go to whichever window is in front: bring the game back first.
function Confirm-Focus {
    for ($attempt = 0; $attempt -lt 20 -and -not $driver::IsForeground($window); ++$attempt) {
        $driver::Focus($window); Start-Sleep -Milliseconds 150
    }
    if (-not $driver::IsForeground($window)) { throw 'The game window cannot be brought to the front.' }
}

function Invoke-ConsoleLine([string]$line, [int]$settleMilliseconds = 500) {
    Confirm-Focus
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

# Waits for a transcript marker; a script stops with the match whenever the
# window loses focus (GameLevel updates only while active), so the window is
# brought back to the front while waiting.
function Wait-Transcript([string]$text, [int]$seconds) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline) {
        if ((Get-Transcript).Contains($text)) { return $true }
        if ($window -ne [IntPtr]::Zero -and -not $driver::IsForeground($window)) {
            $driver::Focus($window)
        }
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

# A script written into a profile's debug folder, where `exec` finds it.
function Write-Script([string]$profile, [string]$name, [string[]]$lines) {
    $folder = Join-Path $profile 'ColourWars/debug'
    $null = New-Item -ItemType Directory -Path $folder -Force
    Set-Content -LiteralPath (Join-Path $folder "$name.cfg") -Value $lines -Encoding ASCII
}

# Starts the game on an isolated profile (and, when given, an isolated TEMP).
function Start-Game([string]$profile, [string[]]$arguments, [string]$temp = '') {
    # These launches test the console, not the intro a match opens with:
    # every match starts at the split-screen countdown, after +logfile.
    $arguments = @($arguments[0], $arguments[1], '+intro', 'off') +
        @($arguments | Select-Object -Skip 2)
    $saved = @{ LOCALAPPDATA = $env:LOCALAPPDATA; TEMP = $env:TEMP; TMP = $env:TMP }
    $env:LOCALAPPDATA = $profile
    if ($temp) { $env:TEMP = $temp; $env:TMP = $temp }
    try {
        $started = Start-Process -FilePath $executable -ArgumentList $arguments -PassThru `
            -WorkingDirectory $output
        $null = $started.Handle
    }
    finally {
        $env:LOCALAPPDATA = $saved.LOCALAPPDATA; $env:TEMP = $saved.TEMP; $env:TMP = $saved.TMP
    }
    return $started
}

function Find-Window($process) {
    $deadline = (Get-Date).AddSeconds($StartupSeconds + 10)
    while ((Get-Date) -lt $deadline) {
        $found = $driver::FindGame($process.Id)
        if ($found -ne [IntPtr]::Zero) { return $found }
        Start-Sleep -Milliseconds 250
    }
    throw 'The game window did not appear.'
}

function Wait-Exit($process, [int]$seconds, [string]$what) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while (-not $process.HasExited -and (Get-Date) -lt $deadline) {
        if ($window -ne [IntPtr]::Zero -and -not $driver::IsForeground($window)) {
            try { $driver::Focus($window) } catch { }
        }
        Start-Sleep -Milliseconds 250
    }
    if (-not $process.HasExited) { throw "$what did not quit." }
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) { throw "$what exited with $($process.ExitCode)." }
}

function Assert-Transcript([string[]]$expected, [string]$what) {
    $log = Get-Transcript
    foreach ($line in $expected) {
        if (-not $log.Contains($line)) { throw "The $what transcript lacks: $line" }
    }
}
