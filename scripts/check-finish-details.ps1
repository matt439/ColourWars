[CmdletBinding()]
param(
    [ValidateSet('x64-debug')]
    [string]$Preset = 'x64-debug',
    [string]$OutputDirectory,
    [ValidateRange(4, 120)]
    [int]$StartupSeconds = 12,
    [ValidateRange(60, 3600)]
    [int]$RunSeconds = 900
)

# Opt-in, local (POL#43): runs the finish-review detail A/B,
# `ColourWars.exe +exec review_finish_details +quit`, on the real Debug
# executable with an isolated profile, and checks what it prepares for the
# attended finish review (POL#24):
#   - the game exits cleanly, the script reaches its end, nothing logs an error;
#   - 33 PNGs: frames t07s, t17s and t54s for all_on and for each of the ten
#     presentation `enabled` flags turned off (no_<detail>);
#   - one checksum per frame per variant, and each frame's hash, update and
#     paint are identical across all eleven variants, so the pictures differ
#     only in presentation (`bursts` counts feedback, which is presentation,
#     and is reported, not compared);
#   - the closing `tune.diff presentation` reports no edits.
# It reports which variants' frames differ from all_on by bytes; that is
# evidence for a person, not a pass condition, and it decides nothing about
# which details stay. Not in CI: hosted runners give no reliable foreground
# focus, and the script stops whenever the window is not in front.

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'check-path.ps1')

# Windows PowerShell 5.1 does not set PSScriptRoot during parameter defaults.
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $PSScriptRoot '../out/validation/finish-details'
}

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$executable = Join-Path $repo "out/build/$Preset/game/ColourWars.exe"
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
    throw "Build $Preset first: $executable is missing."
}
$output = Resolve-CheckFileSystemPath $OutputDirectory
$null = New-Item -ItemType Directory -Path $output -Force

. (Join-Path $PSScriptRoot 'check-debug-console/driver.ps1')
$transcript = Join-Path $output 'review-finish-details.log'
$window = [IntPtr]::Zero

if (-not ('ColourWars.FinishDetailsPointer' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
namespace ColourWars {
public static class FinishDetailsPointer {
    [StructLayout(LayoutKind.Sequential)] public struct Point { public int X; public int Y; }
    [StructLayout(LayoutKind.Sequential)] struct Rect { public int Left; public int Top; public int Right; public int Bottom; }
    [DllImport("user32.dll")] static extern bool GetClientRect(IntPtr window, out Rect rect);
    [DllImport("user32.dll")] static extern bool ClientToScreen(IntPtr window, ref Point point);
    [DllImport("user32.dll")] static extern bool SetCursorPos(int x, int y);
    // Moves the pointer to a fraction of the window's client area; returns
    // the client position it chose.
    public static bool Park(IntPtr window, double x, double y, out Point client) {
        client = new Point();
        Rect rect;
        if (!GetClientRect(window, out rect) || rect.Right <= 0 || rect.Bottom <= 0) { return false; }
        client.X = (int)(rect.Right * x); client.Y = (int)(rect.Bottom * y);
        Point screen = client;
        return ClientToScreen(window, ref screen) && SetCursorPos(screen.X, screen.Y);
    }
}
}
'@
}

$frames = @('t07s', 't17s', 't54s')
$details = @('hud_cursor_marker', 'hud_teammate_marker', 'player_identity', 'combat_feedback',
    'blocked_shot', 'idle_drip', 'weapon_feedback', 'movement_feedback', 'tool_motion', 'figure_quiet')
$variants = @('all_on') + @($details | ForEach-Object { "no_$_" })

$profileDirectory = Join-Path $output 'profile'
Remove-Item -LiteralPath $profileDirectory -Recurse -Force -ErrorAction SilentlyContinue
$null = New-Item -ItemType Directory -Path $profileDirectory -Force
Remove-Item -LiteralPath $transcript -ErrorAction SilentlyContinue

$process = $null
try {
    $process = Start-Game $profileDirectory @('+logfile', ('"' + $transcript + '"'),
        '+exec', 'review_finish_details', '+quit')
    $window = Find-Window $process
    $driver::Focus($window)
    # The cursor marker follows the real pointer, so every variant's frames
    # need it in one place: park it in the second pane's sky (a quarter down,
    # just right of centre) and leave it there for the run.
    Start-Sleep -Milliseconds 500
    $client = New-Object ColourWars.FinishDetailsPointer+Point
    if (-not [ColourWars.FinishDetailsPointer]::Park($window, 0.56, 0.25, [ref]$client)) {
        throw 'The pointer could not be parked in the game window.'
    }
    Write-Host "pointer parked at client ($($client.X), $($client.Y))"
    Wait-Exit $process $RunSeconds 'exec review_finish_details'
}
finally {
    if ($process -ne $null -and -not $process.HasExited) { $process.Kill() }
}

$log = Get-Transcript
Assert-Transcript @('launch: exec review_finish_details',
    'review_finish_details: done (3 frames x 11 variants)', 'script review_finish_details.cfg done',
    'launch: quit') 'finish details'
if ($log -match '(?m)^error: ') { throw 'The review logged an error.' }
if ($log -match 'stopped: ') { throw 'A script stopped early.' }

# The closing tune.diff presentation comes after the last checksum.
$diffAt = $log.LastIndexOf("`ntune.diff: no edits")
if ($diffAt -lt 0 -or $diffAt -lt $log.LastIndexOf('checksum ')) {
    throw 'tune.diff presentation did not report "no edits" after the review.'
}

$folders = @(Get-ChildItem -LiteralPath (Join-Path $profileDirectory 'ColourWars/screens') -Directory)
if ($folders.Count -ne 1) { throw "The review left $($folders.Count) screenshot folders." }
$folder = $folders[0].FullName
$shots = @(Get-ChildItem -LiteralPath $folder -Filter '*.png')
if ($shots.Count -ne $frames.Count * $variants.Count) {
    throw "The review wrote $($shots.Count) PNGs, not $($frames.Count * $variants.Count)."
}
foreach ($variant in $variants) {
    foreach ($frame in $frames) {
        if (-not (Test-Path -LiteralPath (Join-Path $folder "$frame-$variant.png"))) {
            throw "Missing $frame-$variant.png."
        }
    }
}
$evidence = Join-Path $output 'screens'
Remove-Item -LiteralPath $evidence -Recurse -Force -ErrorAction SilentlyContinue
Copy-Item -LiteralPath $folder -Destination $evidence -Recurse

# Each pass's echo marker is followed by that pass's three checksums.
$passes = @([regex]::Matches($log, '(?m)^finish-details: pass (\w+)'))
$names = @($passes | ForEach-Object { $_.Groups[1].Value })
if (($names -join ',') -ne ($variants -join ',')) {
    throw "The passes ran as [$($names -join ', ')], not [$($variants -join ', ')]."
}
$pattern = 'checksum ([0-9a-f]{16}) at update (\d+), (paint [^\r\n]*?), bursts (\d+)'
$table = @{}
for ($index = 0; $index -lt $passes.Count; ++$index) {
    $from = $passes[$index].Index
    $to = if ($index + 1 -lt $passes.Count) { $passes[$index + 1].Index } else { $log.Length }
    $sums = @([regex]::Matches($log.Substring($from, $to - $from), $pattern))
    if ($sums.Count -ne $frames.Count) {
        throw "Pass $($names[$index]) logged $($sums.Count) checksums, not $($frames.Count)."
    }
    $table[$names[$index]] = $sums
}

$report = @("finish details: $folder", '')
$mismatches = @()
for ($frame = 0; $frame -lt $frames.Count; ++$frame) {
    $base = $table['all_on'][$frame]
    $report += "$($frames[$frame]): checksum $($base.Groups[1].Value) at update $($base.Groups[2].Value), $($base.Groups[3].Value)"
    foreach ($variant in $variants) {
        $sum = $table[$variant][$frame]
        foreach ($group in 1, 2, 3) {
            if ($sum.Groups[$group].Value -ne $base.Groups[$group].Value) {
                $mismatches += "$($frames[$frame]) ${variant}: '$($sum.Value)' against all_on '$($base.Value)'"
                break
            }
        }
    }
}
$report += ''
$report += 'variant: frames whose PNG differs from all_on; bursts per frame'
foreach ($variant in $variants) {
    $differ = @()
    for ($frame = 0; $frame -lt $frames.Count; ++$frame) {
        $a = (Get-FileHash -LiteralPath (Join-Path $folder "$($frames[$frame])-all_on.png")).Hash
        $b = (Get-FileHash -LiteralPath (Join-Path $folder "$($frames[$frame])-$variant.png")).Hash
        if ($a -ne $b) { $differ += $frames[$frame] }
    }
    $bursts = @($table[$variant] | ForEach-Object { $_.Groups[4].Value }) -join '/'
    $shown = if ($variant -eq 'all_on') { '-' } elseif ($differ.Count -eq 0) { 'none' } else { $differ -join ' ' }
    $report += "  ${variant}: $shown; bursts $bursts"
}
Set-Content -LiteralPath (Join-Path $output 'finish-details-report.txt') -Value $report
$report | ForEach-Object { Write-Host $_ }

if ($mismatches.Count -ne 0) {
    $mismatches | ForEach-Object { Write-Host "MISMATCH $_" }
    throw "$($mismatches.Count) frame checksum(s) differ across variants: presentation changed the simulation."
}
Write-Host "PASS finish details: 11 variants x 3 frames, every frame's checksum equal across variants, tune.diff presentation empty. Evidence: $output"
