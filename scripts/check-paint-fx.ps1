[CmdletBinding()]
param(
    [ValidateSet('x64-debug', 'x64-release')]
    [string] $Preset = 'x64-debug',
    [switch] $SkipBuild,
    [string] $OutputDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'check-path.ps1')

# Windows PowerShell 5.1 does not set PSScriptRoot during parameter defaults.
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $PSScriptRoot '../out/validation/paint-fx'
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

# Lossless PPMs and native-size crops remain available. Each sheet contains
# one resolution/palette/role so reading it never enlarges the effects.
function Write-ContactSheet {
    param([string] $CaptureDirectory)
    Add-Type -AssemblyName System.Drawing
    if (-not ('ColourWarsPaintFxPpm' -as [type])) {
        Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
public static class ColourWarsPaintFxPpm {
    public static void Convert(string source, string target) {
        byte[] bytes = File.ReadAllBytes(source);
        string header = Encoding.ASCII.GetString(bytes, 0, Math.Min(64, bytes.Length));
        Match match = Regex.Match(header, @"^P6\n(\d+) (\d+)\n255\n");
        if (!match.Success) throw new InvalidDataException("Unsupported PPM header: " + source);
        int width = int.Parse(match.Groups[1].Value);
        int height = int.Parse(match.Groups[2].Value);
        if (bytes.Length != match.Length + width * height * 3)
            throw new InvalidDataException("PPM length mismatch: " + source);
        using (Bitmap bitmap = new Bitmap(width, height, PixelFormat.Format24bppRgb)) {
            BitmapData data = bitmap.LockBits(new Rectangle(0, 0, width, height),
                ImageLockMode.WriteOnly, PixelFormat.Format24bppRgb);
            try {
                byte[] pixels = new byte[data.Stride * height];
                int offset = match.Length;
                for (int y = 0; y < height; ++y) {
                    for (int x = 0; x < width; ++x) {
                        int destination = y * data.Stride + x * 3;
                        pixels[destination] = bytes[offset + 2];
                        pixels[destination + 1] = bytes[offset + 1];
                        pixels[destination + 2] = bytes[offset];
                        offset += 3;
                    }
                }
                Marshal.Copy(pixels, 0, data.Scan0, pixels.Length);
            }
            finally { bitmap.UnlockBits(data); }
            bitmap.Save(target, ImageFormat.Png);
        }
    }
}
"@
    }
    $groups = @{}
    foreach ($file in Get-ChildItem -LiteralPath $CaptureDirectory -Filter '*.ppm' -File | Sort-Object Name) {
        $png = [IO.Path]::ChangeExtension($file.FullName, '.png')
        [ColourWarsPaintFxPpm]::Convert($file.FullName, $png)
        if ($file.BaseName -match '^(\d+x\d+-(?:blue_orange|pink_green|blue_yellow))-(shots|impact|runoff)-.*-crop$') {
            $key = $Matches[1] + '-' + $Matches[2]
            if (-not $groups.ContainsKey($key)) { $groups[$key] = [Collections.Generic.List[object]]::new() }
            $groups[$key].Add([pscustomobject]@{
                name = $file.BaseName.Substring($Matches[1].Length + 1).Replace('-crop', '')
                path = $png
            })
        }
    }
    if ($groups.Count -eq 0) { throw 'No paint-effect crops were captured.' }
    foreach ($key in $groups.Keys | Sort-Object) {
        $items = $groups[$key]
        $sheet = [Drawing.Bitmap]::new(232 * 4, 200 * [int][Math]::Ceiling($items.Count / 4.0))
        $graphics = $null
        $font = $null
        try {
            $graphics = [Drawing.Graphics]::FromImage($sheet)
            $graphics.Clear([Drawing.Color]::FromArgb(238, 238, 238))
            $font = [Drawing.Font]::new('Arial', 8)
            for ($index = 0; $index -lt $items.Count; ++$index) {
                $x = ($index % 4) * 232 + 8
                $y = [int][Math]::Floor($index / 4.0) * 200 + 4
                $graphics.DrawString($items[$index].name, $font, [Drawing.Brushes]::Black,
                    [Drawing.RectangleF]::new($x, $y, 216, 32))
                $bitmap = [Drawing.Bitmap]::FromFile($items[$index].path)
                try { $graphics.DrawImageUnscaled($bitmap, $x, $y + 32) }
                finally { $bitmap.Dispose() }
            }
            $sheet.Save((Join-Path $CaptureDirectory ($key + '-contact.png')), [Drawing.Imaging.ImageFormat]::Png)
        }
        finally {
            if ($null -ne $font) { $font.Dispose() }
            if ($null -ne $graphics) { $graphics.Dispose() }
            $sheet.Dispose()
        }
    }
}

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$build = Join-Path $repo "out/build/$Preset"
$outputRoot = Resolve-CheckFileSystemPath $OutputDirectory
$run = Join-Path (Join-Path $outputRoot $Preset) ([Guid]::NewGuid().ToString('N'))
$captures = Join-Path $run 'captures'
$working = Join-Path $run 'working-directory'
$profile = Join-Path $run 'profile'
$null = New-Item -ItemType Directory -Path $run, $captures, $working, $profile -Force

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
    preset = $Preset
    game_revision = (& git rev-parse HEAD)
    engine_revision = (& git -C external/labrador rev-parse HEAD)
    worktree_status = @(& git status --short)
    stage = 'movement_course'
    player_counts = @(1, 4)
    resolutions = @('1280x720', '1920x1080', '2560x1440')
    camera_scales = @(0.6666667, 1.0, 1.3333333)
    palettes = @('blue_orange', 'pink_green', 'blue_yellow')
    weapons = @('sprayer', 'sniper', 'mister', 'bucket', 'roller')
    projectile_types = @('spray', 'jet', 'rolling', 'mist', 'ball')
    surface_faces = @('top', 'bottom', 'left', 'right')
    inputs = 'Scripted pad-slot inputs at the real 60 Hz simulation step; no physical-device claim.'
    audio = 'Muted; this check establishes no listening acceptance.'
    checks = 'Actual four-player Level context at every resolution/palette; controlled matrices submit four render views at the actual gameplay camera scale. Four flying builder projectile kinds, both team colours, cardinal and diagonal velocities, birth and aged frame; raster bounds and repeated const draws. Five builder projectile kinds contact real StructurePaintable objects on all four faces. Friendly repaint restarts pixels without stealing credit; enemy repaint immediately equals fresh-team tint; wet impact settles; mixed adjacent friendly/enemy paint. Wall runoff adds native-scale one/four-view matrices in all palettes/resolutions, early/late moving frames and live Reduce effects baseline subtraction. Every changed runoff raster sample is checked against independent same-team exposed wall rectangles excluding the inward keyline, neutral/enemy paint and fractional occlusion; bounded narrow fragments, corner contacts, const redraw, unchanged paint credit and hidden aging/retirement. Representative playing-size crops, full matrices and scene captures; roller only represented by surface contact.'
    profile = $profile
    diagnostic = ''
}
try {
    if (-not $SkipBuild) {
        Write-Host "Configure and build $Preset; logs: $run"
        Invoke-Logged -Command 'cmake' -Arguments @('--preset', $Preset) `
            -LogPath (Join-Path $run 'configure.log')
        Invoke-Logged -Command 'cmake' -Arguments @('--build', '--preset', $Preset, '--parallel', '1') `
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
    $installed = Join-Path $cache.VCPKG_INSTALLED_DIR $cache.VCPKG_TARGET_TRIPLET
    $dependencyLibraries = if ($debug) { Join-Path $installed 'debug/lib' } else { Join-Path $installed 'lib' }
    # The game library and both platform adapters, as the executable links them.
    $libraries = @(
        (Join-Path $gameDirectory 'colourwars_game.lib'),
        (Join-Path $gameDirectory 'colourwars_rumble_xinput.lib'),
        (Join-Path $gameDirectory 'colourwars_display_awake.lib'),
        (Join-Path $build 'external/labrador/engine/LabradorEngine.lib'),
        (Join-Path $build 'external/labrador/engine/math/MattMath.lib'),
        (Join-Path $dependencyLibraries 'DirectXTK.lib'),
        (Join-Path $dependencyLibraries 'xaudio2_9redist.lib')
    )
    foreach ($path in @($cache.CMAKE_CXX_COMPILER) + $libraries +
        @((Join-Path $gameDirectory 'ColourWars.exe'), (Join-Path $gameDirectory 'presentation.json'))) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing build output: $path" }
    }
    $source = Join-Path $PSScriptRoot 'acceptance/paint_fx_visual.cpp'
    $executable = Join-Path $run 'paint_fx_visual.exe'
    $responseFile = Join-Path $run 'compile.rsp'
    $arguments = @('/nologo', '/EHsc', '/std:c++20', '/W4', '/WX', '/permissive-',
        '/DDIRECTX_TOOLKIT_IMPORT', '/DUSING_XAUDIO2_REDIST', '/DUSING_XINPUT',
        '/DUNICODE', '/D_UNICODE', '/external:W0')
    if ($debug) { $arguments += '/MDd' } else { $arguments += @('/MD', '/DNDEBUG') }
    foreach ($include in @($repo, (Join-Path $repo 'external/labrador'), (Join-Path $build 'generated'))) {
        $arguments += '/I' + (Quote-Path $include)
    }
    foreach ($include in @((Join-Path $installed 'include/directxtk'),
        (Join-Path $installed 'include/xaudio2redist'), (Join-Path $installed 'include'))) {
        $arguments += '/external:I' + (Quote-Path $include)
    }
    $arguments += @((Quote-Path $source), ('/Fe:' + (Quote-Path $executable)),
        ('/Fo:' + (Quote-Path (Join-Path $run 'paint_fx_visual.obj'))))
    $arguments += @($libraries | ForEach-Object { Quote-Path $_ })
    $arguments += @('d3d11.lib', 'dxgi.lib', 'dxguid.lib', 'kernel32.lib', 'user32.lib',
        'gdi32.lib', 'winspool.lib', 'shell32.lib', 'ole32.lib', 'oleaut32.lib',
        'uuid.lib', 'comdlg32.lib', 'advapi32.lib', 'comctl32.lib', 'Xinput.lib')
    $arguments | Out-File -LiteralPath $responseFile -Encoding unicode
    Write-Host "Compile paint effects visual probe ($Preset)"
    Invoke-Logged -Command $cache.CMAKE_CXX_COMPILER -Arguments @("@$responseFile") `
        -LogPath (Join-Path $run 'compile.log')

    $result.staged_content_hashes = @(
        'manifest.json', 'tuning.json', 'presentation.json', 'levels/movement_course.json',
        'textures/painter.dds', 'textures/painter.json',
        'textures/weapon_paint.dds', 'textures/weapon_paint.json',
        'textures/paint_fx.dds', 'textures/paint_fx.json' |
        ForEach-Object {
            [ordered]@{ path = $_; sha256 = (Get-FileHash -LiteralPath (Join-Path $gameDirectory $_)).Hash }
        }
    )
    $env:PATH = "$gameDirectory;$previousPath"
    $env:LOCALAPPDATA = $profile
    $process = Start-Process -FilePath $executable -WindowStyle Hidden -PassThru `
        -WorkingDirectory $working -ArgumentList @((Quote-Path $gameDirectory),
            (Quote-Path $captures)) `
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
    foreach ($line in Get-Content -LiteralPath (Join-Path $run 'run.err')) {
        if ($line.Trim() -ne '' -and
            $line -notmatch '^no audio: .*The manifest marks this bank optional, so it plays nothing\.') {
            throw "Unexpected probe diagnostic: $line"
        }
    }
    if (Test-Path -LiteralPath (Join-Path $profile 'ColourWars/save_data.json')) {
        throw 'The visual probe unexpectedly created a save.'
    }
    Write-ContactSheet -CaptureDirectory $captures
    $result.outcome = 'passed'
    $result.diagnostic = 'Paint effect builder/contact matrices, immediate repaint tint, paint credit, frozen draws, settled impacts and raster culling bounds passed. One/four-view wall runoff moves at native gameplay scale, obeys exact ownership/keyline/occlusion raster bounds, suppresses live with Reduce effects and retires while hidden. Contact sheets preserve playing size. Human art review, physical devices and listening acceptance remain separate.'
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
    Write-Host "Paint effects evidence: $run"
    Pop-Location
}
if ($null -ne $failure) { throw $failure }
