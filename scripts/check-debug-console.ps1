[CmdletBinding()]
param(
    [ValidateSet('x64-debug')]
    [string]$Preset = 'x64-debug',
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '../out/validation/debug-console'),
    [ValidateRange(4, 120)]
    [int]$StartupSeconds = 12
)

# Opt-in, local: drives the real Debug executable through the developer console
# with real key events and takes screenshots for a person to look at. Not in CI:
# hosted runners give no reliable foreground focus, and Release has no console
# (scripts/acceptance/debug_console.cpp covers both presets on the CPU side).
#
#   1. launch with +stage movement_course +players 4 +visual posed
#   2. ` status Enter      (console open over four panes)
#   3. ` F8 F9             (visual A/B and feedback toggle without the console)
#   4. ` quit Enter        (clean exit, transcript complete)

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

Add-Type -AssemblyName System.Drawing
if (-not ('ColourWars.ConsoleDriver' -as [type])) {
    Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace ColourWars
{
    public static class ConsoleDriver
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

        public static void Type(string text)
        {
            foreach (char c in text)
            {
                if (c == ' ') { Tap(0x20, false); continue; }
                if (c >= 'a' && c <= 'z') { Tap((byte)char.ToUpperInvariant(c), false); continue; }
                if (c >= '0' && c <= '9') { Tap((byte)c, false); continue; }
                throw new ArgumentException("Unsupported character: " + c);
            }
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

$grave = 0xC0; $enter = 0x0D; $f8 = 0x77; $f9 = 0x78
$driver = [ColourWars.ConsoleDriver]

$originalLocalAppData = $env:LOCALAPPDATA
$process = $null
try {
    $env:LOCALAPPDATA = $profileDirectory
    try {
        $arguments = @('+logfile', ('"' + $transcript + '"'), '+stage', 'movement_course',
            '+players', '4', '+visual', 'posed')
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
    $driver::Screenshot($window, (Join-Path $output '1-launch.png'))

    $driver::Tap($grave, $false); Start-Sleep -Milliseconds 300
    $driver::Type('status'); $driver::Tap($enter, $false); Start-Sleep -Milliseconds 500
    $driver::Screenshot($window, (Join-Path $output '2-console-status.png'))

    $driver::Tap($grave, $false); Start-Sleep -Milliseconds 300
    $driver::Tap($f8, $false); Start-Sleep -Milliseconds 800
    $driver::Tap($f9, $false); Start-Sleep -Milliseconds 800
    $driver::Screenshot($window, (Join-Path $output '3-after-f8-f9.png'))

    $driver::Tap($grave, $false); Start-Sleep -Milliseconds 300
    $driver::Type('quit'); $driver::Tap($enter, $false)
    if (-not $process.WaitForExit(15000)) { throw 'The game did not quit.' }
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) { throw "The game exited with $($process.ExitCode)." }

    $log = Get-Content -LiteralPath $transcript -Raw
    foreach ($expected in @(
        'match: starting movement_course, 4 player(s), visual posed',
        'match: stage movement_course, 4 player(s), visual posed',
        '[F8] visual next',
        'match: movement_course, 4 player(s), visual original',
        '[F9] feedback toggle',
        'feedback = off',
        '] quit')) {
        if (-not $log.Contains($expected)) { throw "The transcript lacks: $expected" }
    }
    Write-Host "PASS debug console driven: launch arguments, status, F8, F9, quit. Screenshots: $output"
}
finally {
    if ($process -ne $null -and -not $process.HasExited) { $process.Kill() }
}
