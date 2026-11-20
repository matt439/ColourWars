[CmdletBinding()]
param(
    [string] $OutputDirectory,
    [ValidateRange(1, 600)]
    [int] $RunSeconds = 60,
    [ValidateSet('1280x720', '1920x1080', '2560x1440')]
    [string] $Resolution = '1280x720'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'check-path.ps1')
. (Join-Path $PSScriptRoot 'check-diagnostics.ps1')
. (Join-Path $PSScriptRoot 'quiet-game.ps1')

if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $PSScriptRoot '../out/validation/shader-playground'
}
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$executable = Join-Path $repo 'out/build/x64-debug/game/ColourWars.exe'
if (-not (Test-Path -LiteralPath $executable -PathType Leaf) -or
    -not (Get-Item -LiteralPath $executable).VersionInfo.IsDebug) {
    throw 'Build x64-debug first with scripts/build.ps1 -Preset x64-debug.'
}
$output = Join-Path (Resolve-CheckFileSystemPath $OutputDirectory) ([Guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Path $output -Force
$rasterLooks = @('vga', 'trinitron', 'arcade')
$looks = @('posterise', 'ink', 'halftone', 'crt', 'highcolour', 'dither', 'paint90') + $rasterLooks
$width, $height = $Resolution -split 'x'
$result = [ordered]@{
    outcome = 'failed'
    executable = $executable
    executable_sha256 = ''
    stage = 'movement_course'
    seed = 7
    width = [int]$width
    height = [int]$height
    launches = [Collections.Generic.List[object]]::new()
    checks = [Collections.Generic.List[object]]::new()
    diagnostic = ''
}

function Add-ShaderCheck([string] $Layout, [string] $Name, [bool] $Passed, $Evidence) {
    $result.checks.Add([ordered]@{ layout = $Layout; check = $Name; passed = $Passed; evidence = $Evidence })
}

function Invoke-ShaderLaunch([int] $Players, [bool] $Playground) {
    $layout = "${Players}p"
    $kind = if ($Playground) { 'playground' } else { 'ordinary' }
    $directory = Join-Path $output "$layout-$kind"
    $profile = Join-Path $directory 'profile'
    $debug = Join-Path $profile 'ColourWars/debug'
    $scratch = Join-Path $directory 'temp'
    $log = Join-Path $directory 'console.log'
    $stderr = Join-Path $directory 'stderr.log'
    $null = New-Item -ItemType Directory -Path $debug, $scratch -Force
    $launch = [ordered]@{
        layout = $layout
        kind = $kind
        pid = $null
        exited = $false
        exit_code = $null
        device = $null
        directory = $directory
        captures = Join-Path $profile 'ColourWars/screens'
    }
    $result.launches.Add($launch)

    $save = @{
        resolution = @{ x = $result.width; y = $result.height }
        fullscreen = $false; vsync = $false; reduce_effects = $false
        crt_monitor = $false; team_palette = 'blue_orange'
    }
    [IO.File]::WriteAllText((Join-Path $profile 'ColourWars/save_data.json'), ($save | ConvertTo-Json))
    $commands = @('script.background 1', 'screenshot.strip off', "seed $($result.seed)", 'intro off',
        "players $Players", "stage $($result.stage)", 'wait match', 'wait 30', 'pause 1', 'status')
    $arguments = @('+logfile', 'console.log', '+exec', 'shader_check')
    if ($Playground) {
        $commands += @('shader off', 'screenshot off', 'checksum')
        foreach ($look in $looks) {
            $commands += @("shader $look 0", "screenshot $look-zero", 'checksum',
                "shader $look", "screenshot $look-default", 'checksum')
        }
        foreach ($look in $rasterLooks) {
            $commands += @("shader $look 1 240", "screenshot $look-240", 'checksum',
                "shader $look 1 720", "screenshot $look-720", 'checksum')
        }
        # A refused script command stops the script. Launch statements resume
        # on that same paused match to query and capture the surviving look.
        $commands += @('shader off', 'screenshot off-again', 'checksum', 'shader ink 0.4 2',
            'screenshot before-invalid', 'checksum', 'shader crt 0.5 0')
        $arguments += @('+shader', '+screenshot', 'after-invalid', '+checksum',
            '+shader', 'vga', '1', '239', '+shader', 'arcade', '1', '721',
            '+shader', '+screenshot', 'after-invalid-raster', '+checksum')
    }
    else {
        $commands += @('screenshot baseline', 'checksum')
    }
    $arguments += @('+echo', 'shader-check-done', '+quit')
    Set-Content -LiteralPath (Join-Path $debug 'shader_check.cfg') -Value $commands -Encoding ASCII

    $environment = @{
        LOCALAPPDATA = $profile; TEMP = $scratch; TMP = $scratch
        COLOURWARS_SHADER_PLAYGROUND = $(if ($Playground) { '1' } else { '0' })
        COLOURWARS_CRT_TRIAL = '0'; COLOURWARS_LOCALE = $null
    }
    $previous = @{}
    $process = $null
    try {
        $previousQuietGame = Enter-QuietGame
        try {
            foreach ($name in $environment.Keys) {
                $previous[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
                [Environment]::SetEnvironmentVariable($name, $environment[$name], 'Process')
            }
            $process = Start-Process -FilePath $executable -ArgumentList $arguments -WindowStyle Hidden `
                -WorkingDirectory $directory -PassThru -RedirectStandardError $stderr `
                -RedirectStandardOutput (Join-Path $directory 'stdout.log')
            $launch.pid = $process.Id
            $null = $process.Handle
        }
        finally {
            foreach ($name in $previous.Keys) {
                [Environment]::SetEnvironmentVariable($name, $previous[$name], 'Process')
            }
            Exit-QuietGame $previousQuietGame
        }
        if (-not $process.WaitForExit($RunSeconds * 1000)) {
            throw "Game exceeded its $RunSeconds-second deadline."
        }
        $launch.exit_code = $process.ExitCode

        $transcript = Get-Content -LiteralPath $log -Raw -Encoding UTF8
        $device = [regex]::Match($transcript,
            '(?m)^renderer: backend=(\w+) kind=(\w+) vendor=(\d+) device=(\d+) name=([^\r\n]+)$')
        if ($device.Success) {
            $launch.device = [ordered]@{
                backend = $device.Groups[1].Value; kind = $device.Groups[2].Value
                vendor_id = [uint32]$device.Groups[3].Value; device_id = [uint32]$device.Groups[4].Value
                name = $device.Groups[5].Value
            }
        }
        Add-ShaderCheck $layout "$kind-adapter" ($device.Success -and $launch.device.backend -ceq 'd3d11' -and
            $launch.device.kind -ceq 'hardware') $launch.device
        Assert-CheckDiagnostics -Lines @(Get-Content -LiteralPath $stderr -Encoding UTF8) `
            -Context 'shader check' -AllowDebugAdapter `
            -AllowedPattern '^(Direct3D Adapter - WARP|WARNING: Direct3D Debug Device is not available)$'
        if ($launch.exit_code -ne 0 -or $transcript -cnotmatch '(?m)^shader-check-done$' -or
            $transcript -cnotmatch "(?m)^match: stage $($result.stage), $Players player\(s\)," -or
            $transcript -cnotmatch '(?m)^clock: [^\r\n]+, paused \(0 step\(s\) pending\)$' -or
            (($transcript -cmatch '(?m)^shader playground ready;') -ne $Playground)) {
            throw 'The script did not finish the expected paused match with the requested playground state.'
        }
        Add-ShaderCheck $layout "$kind-script" $true $log
        return [pscustomobject]@{ captures = $launch.captures; transcript = $transcript }
    }
    catch {
        Add-ShaderCheck $layout "$kind-script" $false $_.Exception.Message
    }
    finally {
        if ($null -ne $process) {
            if (-not $process.HasExited) {
                $process.Kill()
                $null = $process.WaitForExit(3000)
            }
            $launch.exited = $process.HasExited
            $process.Dispose()
        }
    }
}

function Compare-ShaderCapture([string] $Layout, [string] $Name, [string] $Baseline,
    [string] $Candidate, [bool] $Equal) {
    try {
        $changed = [ColourWars.ShaderRgb]::Changed($Baseline, $Candidate, $result.width, $result.height)
        Add-ShaderCheck $Layout $Name (($changed -eq 0) -eq $Equal) `
            ([ordered]@{ baseline = $Baseline; candidate = $Candidate; expect_equal = $Equal; changed_rgb_pixels = $changed })
    }
    catch {
        Add-ShaderCheck $Layout $Name $false $_.Exception.Message
    }
}

try {
    $result.executable_sha256 = (Get-FileHash -LiteralPath $executable -Algorithm SHA256).Hash
    . (Join-Path $PSScriptRoot 'drawing-references.ps1')
    if (-not ('ColourWars.ShaderRgb' -as [type])) {
        Add-Type -ReferencedAssemblies $DrawingReferences -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;

namespace ColourWars {
    public static class ShaderRgb {
        private static byte[] Read(string path, int width, int height) {
            using (var bitmap = new Bitmap(path)) {
                if (bitmap.Width != width || bitmap.Height != height)
                    throw new InvalidDataException("Unexpected capture dimensions in " + path);
                int rowBytes = width * 3;
                var pixels = new byte[rowBytes * height];
                BitmapData data = bitmap.LockBits(new Rectangle(0, 0, width, height),
                    ImageLockMode.ReadOnly, PixelFormat.Format24bppRgb);
                try {
                    for (int y = 0; y < height; ++y)
                        Marshal.Copy(IntPtr.Add(data.Scan0, y * data.Stride), pixels, y * rowBytes, rowBytes);
                }
                finally { bitmap.UnlockBits(data); }
                return pixels;
            }
        }

        public static int Changed(string baseline, string candidate, int width, int height) {
            byte[] before = Read(baseline, width, height);
            byte[] after = Read(candidate, width, height);
            int changed = 0;
            for (int i = 0; i < before.Length; i += 3) {
                if (before[i] != after[i] || before[i + 1] != after[i + 1] || before[i + 2] != after[i + 2])
                    ++changed;
            }
            return changed;
        }
    }
}
'@
    }

    foreach ($players in @(1, 4)) {
        $layout = "${players}p"
        $ordinary = Invoke-ShaderLaunch $players $false
        $playground = Invoke-ShaderLaunch $players $true
        if ($null -eq $ordinary -or $null -eq $playground) { continue }
        $baseline = Join-Path $ordinary.captures 'baseline.png'
        $shots = $playground.captures
        foreach ($name in @('off', 'off-again') + @($looks | ForEach-Object { "$_-zero" })) {
            Compare-ShaderCapture $layout $name $baseline (Join-Path $shots "$name.png") $true
        }
        foreach ($look in $looks) {
            Compare-ShaderCapture $layout "$look-default" $baseline (Join-Path $shots "$look-default.png") $false
        }
        $previousRasters = @()
        foreach ($look in $rasterLooks) {
            $coarseRaster = Join-Path $shots "$look-240.png"
            Compare-ShaderCapture $layout "$look-raster-detail" $coarseRaster (Join-Path $shots "$look-720.png") $false
            Compare-ShaderCapture $layout "$look-distinct-from-crt" (Join-Path $shots 'crt-default.png') (Join-Path $shots "$look-default.png") $false
            # Compare at identical strength and line count across the new
            # monitors so different preset defaults cannot mask a duplicate.
            foreach ($previous in $previousRasters) {
                Compare-ShaderCapture $layout "$look-distinct-from-$previous" $coarseRaster (Join-Path $shots "$previous-240.png") $false
            }
            $previousRasters += $look
        }
        $beforeInvalid = Join-Path $shots 'before-invalid.png'
        Compare-ShaderCapture $layout 'look-before-invalid' $baseline $beforeInvalid $false
        Compare-ShaderCapture $layout 'invalid-pixels' $beforeInvalid (Join-Path $shots 'after-invalid.png') $true
        Compare-ShaderCapture $layout 'invalid-raster-pixels' $beforeInvalid (Join-Path $shots 'after-invalid-raster.png') $true

        $checksum = '(?m)^checksum [0-9a-f]{16} at update \d+(?=,)'
        $ordinarySums = @([regex]::Matches($ordinary.transcript, $checksum) | ForEach-Object Value)
        $playgroundSums = @([regex]::Matches($playground.transcript, $checksum) | ForEach-Object Value)
        Add-ShaderCheck $layout 'paused-simulation' ($ordinarySums.Count -eq 1 -and
            $playgroundSums.Count -eq (2 * $looks.Count + 2 * $rasterLooks.Count + 5) -and
            (@(($ordinarySums + $playgroundSums) | Select-Object -Unique).Count -eq 1)) `
            ([ordered]@{ ordinary = $ordinarySums; playground = $playgroundSums })

        $errors = @([regex]::Matches($playground.transcript, '(?m)^error: [^\r\n]+') | ForEach-Object Value)
        $rejection = 'shader crt: detail must be a whole number from 4 to 12'
        $status = @([regex]::Matches($playground.transcript, '(?m)^shader ink strength 0\.400000 detail 2$') |
            ForEach-Object Value)
        Add-ShaderCheck $layout 'invalid-settings' ($errors.Count -eq 4 -and
            $errors[0] -ceq "error: $rejection" -and
            $errors[1] -cmatch ('^error: script shader_check\.cfg:\d+ stopped: ' + [regex]::Escape($rejection) + '$') -and
            $errors[2] -ceq 'error: shader vga: detail must be a whole number from 240 to 720' -and
            $errors[3] -ceq 'error: shader arcade: detail must be a whole number from 240 to 720' -and
            $status.Count -eq 3) ([ordered]@{ errors = $errors; status = $status })
        Add-ShaderCheck $layout 'ordinary-console' ($ordinary.transcript -cnotmatch '(?m)^error: ') 'Expect no console errors.'
    }

    $devices = @($result.launches | ForEach-Object { $_.device | ConvertTo-Json -Compress } | Select-Object -Unique)
    Add-ShaderCheck 'all' 'same-adapter' ($devices.Count -eq 1 -and $null -ne $result.launches[0].device) $devices
    $failed = @($result.checks | Where-Object { -not $_.passed })
    if ($failed.Count -gt 0) {
        throw "$($failed.Count) shader check(s) failed: $(($failed | ForEach-Object { "$($_.layout)/$($_.check)" }) -join ', ')."
    }
    $result.outcome = 'passed'
    Write-Output "PASS shader playground: $($result.checks.Count) checks across one and four panes."
}
catch {
    $result.diagnostic = $_.Exception.Message
    throw
}
finally {
    $result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'result.json') -Encoding UTF8
    Write-Output "Shader playground evidence: $output"
}
