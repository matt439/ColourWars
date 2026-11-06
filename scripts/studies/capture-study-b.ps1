[CmdletBinding()]
param(
    # Lists accept PowerShell arrays or one comma-separated word, so they work
    # through `powershell -File` too.
    [string[]]$Variants = @('V0', 'V1', 'V1b'),
    [string[]]$Resolutions = @('1280x720', '1920x1080'),
    [string[]]$Palettes = @('blue_orange', 'pink_green', 'blue_yellow', 'purple_gold', 'red_azure', 'magenta_forest'),
    [string[]]$Players = @('4', '2', '1'),
    [string[]]$Stages = @('crossfeed', 'press_tower'),
    [string[]]$IdentityCues = @('on', 'off'),
    [int]$Seed = 7,
    # The stage 1 revision (V1, V1b), and V0's: the last master commit before
    # stage 1. Pinned, because once stage 1 is in master no merge base finds it.
    [string]$Revision = 'HEAD',
    [string]$BaseRevision = '89b18cd84910966270d5c4e0fff18ed99738eef6',
    [switch]$SkipBuild,
    [switch]$SkipCapture,
    [string]$OutputDirectory,
    [ValidateRange(60, 3600)]
    [int]$RunTimeoutSeconds = 900
)

# Study B stimulus capture (docs/drawing-mistakes-research.md, section 8).
#
# Builds three variants of the game, each in its own detached git worktree
# under <OutputDirectory>/worktrees/<variant>, with the x64-debug preset (the
# console, scripts, seed and screenshot commands exist only in Debug):
#   V0  - -BaseRevision: the last master commit before stage 1;
#   V1  - -Revision: stage 1, one-cell brick slips;
#   V1b - -Revision with BRICK_SLIP_CELLS = 2.0f, edited in its worktree only.
# Then runs each variant once per resolution and team palette, on an isolated
# LOCALAPPDATA with a pre-written save, through the study_b_*.cfg scripts
# beside this file, and copies every PNG to
#   <OutputDirectory>/stimuli/<variant>/<res>/<n>p/<palette>/<stage>-<frame>-cues-<on|off>.png
# with stimuli/manifest.json listing them and stimuli/simulation.json holding
# each frame's checksum per variant. The run fails if any frame's checksum,
# update or paint differs between variants: the variants may differ in art
# only. Outside <OutputDirectory> the script writes only git's worktree
# records and the worktrees' submodule clones, both under .git/worktrees. It
# leaves the main repository's .git/config as it found it: the submodule URL
# is passed with `git -c` for one command, and the config's submodule entries
# are compared before and after and put back if git changed them. It does not
# touch the main working tree or the player's save. No participant data is
# involved.

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot '..\check-path.ps1')
. (Join-Path $PSScriptRoot '..\quiet-game.ps1')

# Splits comma lists, trims, refuses empty, unknown and repeated names, and
# returns each name in the allowed set's own spelling.
function Get-Selection([string]$name, [string[]]$given, [string[]]$allowed) {
    $chosen = @()
    foreach ($word in @($given | ForEach-Object { $_ -split ',' } | ForEach-Object { $_.Trim() })) {
        $match = @($allowed | Where-Object { $_ -eq $word })
        if (-not $word -or $match.Count -ne 1) { throw "-${name}: '$word' is not one of $($allowed -join ', ')." }
        if ($chosen -contains $match[0]) { throw "-${name}: '$word' is given twice." }
        $chosen += $match[0]
    }
    if ($chosen.Count -eq 0) { throw "-${name} is empty." }
    return ,$chosen
}
$Variants = Get-Selection 'Variants' $Variants @('V0', 'V1', 'V1b')
$Resolutions = Get-Selection 'Resolutions' $Resolutions @('1280x720', '1920x1080')
$Palettes = Get-Selection 'Palettes' $Palettes @('blue_orange', 'pink_green', 'blue_yellow', 'purple_gold', 'red_azure', 'magenta_forest')
$Players = Get-Selection 'Players' $Players @('1', '2', '3', '4')
$Stages = Get-Selection 'Stages' $Stages @('crossfeed', 'press_tower')
$IdentityCues = Get-Selection 'IdentityCues' $IdentityCues @('on', 'off')

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $repo 'out\study' }
$output = Resolve-CheckFileSystemPath $OutputDirectory
$null = New-Item -ItemType Directory -Path $output -Force
$logs = Join-Path $output 'logs'
$null = New-Item -ItemType Directory -Path $logs -Force

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$install = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($LASTEXITCODE -ne 0 -or -not $install) { throw 'Visual Studio with the x64 C++ tools is required.' }
$vcvars = Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
$vcpkg = if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { Join-Path $install 'VC/vcpkg' }
$preset = 'x64-debug'
$utf8 = New-Object Text.UTF8Encoding $false

function Invoke-Git([string[]]$arguments) {
    # Windows PowerShell 5.1 turns a native command's stderr into error
    # records, which 'Stop' would throw on; git reports progress there.
    $ErrorActionPreference = 'Continue'
    $result = & git @arguments 2>&1 | ForEach-Object { "$_" }
    $ErrorActionPreference = 'Stop'
    if ($LASTEXITCODE -ne 0) { throw "git $($arguments -join ' ') failed: $result" }
    return ($result | Out-String).Trim()
}

# Revisions are resolved once, so a commit landing during the run moves nothing.
$v1Sha = Invoke-Git @('-C', $repo, 'rev-parse', '--verify', "$Revision^{commit}")
$v0Sha = Invoke-Git @('-C', $repo, 'rev-parse', '--verify', "$BaseRevision^{commit}")
if ($v0Sha -eq $v1Sha) { throw "-Revision and -BaseRevision are the same commit: V0 and V1 would be the same build." }
& git -C $repo merge-base --is-ancestor $v0Sha $v1Sha
if ($LASTEXITCODE -ne 0) { throw "-BaseRevision $v0Sha is not an ancestor of -Revision $v1Sha." }
$definitions = [ordered]@{
    V0  = @{ revision = $v0Sha; slip_cells = $null; note = "$BaseRevision (before stage 1)" }
    V1  = @{ revision = $v1Sha; slip_cells = '1.0f'; note = 'stage 1: one-cell brick slips' }
    V1b = @{ revision = $v1Sha; slip_cells = '2.0f'; note = 'stage 1 with BRICK_SLIP_CELLS = 2.0f' }
}
Write-Host "V0 $v0Sha; V1 and V1b $v1Sha"

$slipFile = 'game\objects\terrain_material_drawings.cpp'
$slipPattern = 'constexpr float (?<name>BRICK_SLIP_CELLS|brick_slip_cells) = [0-9.]+f;'

function Get-Worktree([string]$variant) { return Join-Path $output "worktrees\$variant" }
function Get-Executable([string]$variant) {
    return Join-Path (Get-Worktree $variant) "out\build\$preset\game\ColourWars.exe"
}

# The main repository's own config file, which every linked worktree shares
# (extensions.worktreeConfig is not set). Its submodule.* entries, one
# 'name value' line each, sorted.
$mainConfig = Join-Path $repo '.git\config'
function Get-SubmoduleConfig {
    $ErrorActionPreference = 'Continue'
    $lines = & git config --file $mainConfig --get-regexp '^submodule\.' 2>$null
    $ErrorActionPreference = 'Stop'
    return ((@($lines) | Where-Object { $_ } | Sort-Object) -join "`n")
}
function Restore-SubmoduleConfig([string]$before, [string]$after) {
    $old = @{}
    foreach ($line in ($before -split "`n" | Where-Object { $_ })) { $pair = $line -split ' ', 2; $old[$pair[0]] = $pair[1] }
    foreach ($line in ($after -split "`n" | Where-Object { $_ })) {
        $name = ($line -split ' ', 2)[0]
        if (-not $old.ContainsKey($name)) { $null = Invoke-Git @('config', '--file', $mainConfig, '--unset-all', $name) }
    }
    foreach ($name in $old.Keys) { $null = Invoke-Git @('config', '--file', $mainConfig, '--replace-all', $name, $old[$name]) }
}

function Initialize-Worktree([string]$variant) {
    $definition = $definitions[$variant]
    $tree = Get-Worktree $variant
    if (-not (Test-Path -LiteralPath (Join-Path $tree '.git'))) {
        $null = New-Item -ItemType Directory -Path (Split-Path $tree) -Force
        $null = Invoke-Git @('-C', $repo, 'worktree', 'add', '--detach', $tree, $definition.revision)
    }
    else {
        # Back to the exact revision; drops a previous run's V1b edit.
        $null = Invoke-Git @('-C', $tree, 'checkout', '--force', '--detach', $definition.revision)
    }
    # The submodule comes from the main checkout's own clone of it, so no
    # network is needed when the pin is already there. The URL is given for
    # this one command only: a linked worktree shares the main repository's
    # .git/config, so `git config` or `submodule init` here would write there.
    # The main config's submodule entries are compared before and after, and
    # put back, with a warning, if git changed them anyway.
    $modules = Join-Path $repo '.git\modules\external\labrador'
    $update = @('-C', $tree, '-c', 'protocol.file.allow=always')
    if (Test-Path -LiteralPath $modules) { $update += @('-c', "submodule.external/labrador.url=$modules") }
    $update += @('submodule', 'update', '--init', '--recursive', '--force')
    $before = Get-SubmoduleConfig
    try { $null = Invoke-Git $update }
    finally {
        $after = Get-SubmoduleConfig
        if ($after -ne $before) {
            Restore-SubmoduleConfig $before $after
            Write-Warning "git changed the main repository's submodule config while updating the $variant worktree; it has been put back."
        }
    }
    if (-not (Test-Path -LiteralPath (Join-Path $tree 'external\labrador\CMakeLists.txt'))) {
        throw "$variant worktree has no external/labrador checkout."
    }

    if ($definition.slip_cells) {
        $path = Join-Path $tree $slipFile
        $text = [IO.File]::ReadAllText($path)
        $found = [regex]::Matches($text, $slipPattern)
        if ($found.Count -ne 1) { throw "$variant ${slipFile}: expected one BRICK_SLIP_CELLS line, found $($found.Count)." }
        $wanted = "constexpr float $($found[0].Groups['name'].Value) = $($definition.slip_cells);"
        if ($found[0].Value -ne $wanted) {
            [IO.File]::WriteAllText($path, $text.Replace($found[0].Value, $wanted), $utf8)
        }
    }
    $diff = Invoke-Git @('-C', $tree, 'diff', '--stat')
    Write-Host "$variant worktree at $($definition.revision)$(if ($diff) { "; local edit: $diff" })"
}

function Build-Variant([string]$variant) {
    $tree = Get-Worktree $variant
    $bat = Join-Path $output "build-$variant.bat"
    $log = Join-Path $logs "build-$variant.log"
    $configure = if (Test-Path -LiteralPath (Join-Path $tree "out\build\$preset\CMakeCache.txt")) { '' } else { "cmake --preset $preset || exit /b 1" }
    $lines = @(
        '@echo off',
        "call `"$vcvars`" >nul",
        "set `"VCPKG_ROOT=$vcpkg`"",
        "cd /d `"$tree`"",
        $configure,
        "cmake --build --preset $preset || exit /b 1"
    )
    [IO.File]::WriteAllText($bat, (($lines | Where-Object { $_ }) -join "`r`n") + "`r`n", [Text.Encoding]::ASCII)
    Write-Host "building $variant (log $log)"
    $started = Get-Date
    $process = Start-Process -FilePath 'cmd.exe' -ArgumentList "/c `"$bat`"" -NoNewWindow -Wait -PassThru `
        -RedirectStandardOutput $log -RedirectStandardError "$log.err"
    if ($process.ExitCode -ne 0) { throw "$variant build failed with $($process.ExitCode); see $log" }
    if (-not (Test-Path -LiteralPath (Get-Executable $variant))) { throw "$variant built no ColourWars.exe." }
    Write-Host ("built {0} in {1:n0} s" -f $variant, ((Get-Date) - $started).TotalSeconds)
}

foreach ($variant in $Variants) {
    if (-not $SkipBuild) {
        Initialize-Worktree $variant
        Build-Variant $variant
    }
    elseif (-not (Test-Path -LiteralPath (Get-Executable $variant))) {
        throw "-SkipBuild, but $(Get-Executable $variant) is missing."
    }
}
if ($SkipCapture) { Write-Host 'Builds done; -SkipCapture.'; return }

function Get-PngSize([string]$path) {
    $bytes = [IO.File]::ReadAllBytes($path)
    $read = { param($at) ([int]$bytes[$at] -shl 24) -bor ([int]$bytes[$at + 1] -shl 16) -bor ([int]$bytes[$at + 2] -shl 8) -bor [int]$bytes[$at + 3] }
    return @((& $read 16), (& $read 20))
}

function Read-Shared([string]$path) {
    if (-not (Test-Path -LiteralPath $path)) { return '' }
    $stream = [IO.File]::Open($path, 'Open', 'Read', 'ReadWrite')
    try { return (New-Object IO.StreamReader($stream)).ReadToEnd() } finally { $stream.Dispose() }
}

# The palette check. The game keeps its default pair when the save's
# team_palette names no pair, so a wrong key would still capture. Each heavy
# frame is therefore checked for its team colours, read from the variant's own
# presentation.json: every second pixel in each direction is matched against
# every palette colour within 12 per channel, and each team colour the frame
# should show must be matched at least 50 times and more often than any colour
# of another pair. A 1P frame shows team A only.
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
public static class StudyBPixels {
    public static int[] Count(string path, int[] rgb, int tolerance) {
        using (var bitmap = new Bitmap(path)) {
            var data = bitmap.LockBits(new Rectangle(0, 0, bitmap.Width, bitmap.Height), ImageLockMode.ReadOnly, PixelFormat.Format24bppRgb);
            var bytes = new byte[data.Stride * bitmap.Height];
            Marshal.Copy(data.Scan0, bytes, 0, bytes.Length);
            bitmap.UnlockBits(data);
            var counts = new int[rgb.Length / 3];
            for (int y = 0; y < bitmap.Height; y += 2) {
                for (int x = 0; x < bitmap.Width; x += 2) {
                    int at = y * data.Stride + 3 * x;
                    int b = bytes[at], g = bytes[at + 1], r = bytes[at + 2];
                    for (int k = 0; k < counts.Length; ++k) {
                        if (Math.Abs(r - rgb[3 * k]) <= tolerance && Math.Abs(g - rgb[3 * k + 1]) <= tolerance && Math.Abs(b - rgb[3 * k + 2]) <= tolerance) { ++counts[k]; }
                    }
                }
            }
            return counts;
        }
    }
}
'@
$paletteCache = @{}
# name -> 'RRGGBB' and key -> @(team_a, team_b) from one variant's presentation.json.
function Get-PaletteDefinition([string]$variant) {
    if (-not $paletteCache.ContainsKey($variant)) {
        $path = Join-Path (Get-Worktree $variant) 'game\content\presentation.json'
        $json = [IO.File]::ReadAllText($path) | ConvertFrom-Json
        $colours = [ordered]@{}
        foreach ($entry in $json.team_palette.colours) { $colours[$entry.name] = $entry.colour.Substring(0, 6) }
        $pairs = @{}
        foreach ($entry in $json.team_palette.pairs) { $pairs[$entry.key] = @($entry.team_a, $entry.team_b) }
        $paletteCache[$variant] = @{ colours = $colours; pairs = $pairs }
    }
    return $paletteCache[$variant]
}
# Returns 'name=count ...' for the frame, or throws if the palette is not the one asked for.
function Test-Palette([string]$variant, [string]$palette, [int]$players, [string]$path, [string]$label) {
    $definition = Get-PaletteDefinition $variant
    if (-not $definition.pairs.ContainsKey($palette)) { throw "$label`: $variant has no palette pair '$palette'." }
    $names = @($definition.colours.Keys)
    $rgb = @()
    foreach ($name in $names) {
        $hex = $definition.colours[$name]
        $rgb += @([Convert]::ToInt32($hex.Substring(0, 2), 16), [Convert]::ToInt32($hex.Substring(2, 2), 16), [Convert]::ToInt32($hex.Substring(4, 2), 16))
    }
    $counts = [StudyBPixels]::Count($path, [int[]]$rgb, 12)
    $pair = $definition.pairs[$palette]
    $wanted = if ($players -ge 2) { @($pair[0], $pair[1]) } else { @($pair[0]) }
    $other = 0
    for ($k = 0; $k -lt $names.Count; ++$k) {
        if ($pair -notcontains $names[$k]) { $other = [Math]::Max($other, $counts[$k]) }
    }
    $text = @(for ($k = 0; $k -lt $names.Count; ++$k) { if ($counts[$k] -gt 0) { "$($names[$k])=$($counts[$k])" } }) -join ' '
    foreach ($name in $wanted) {
        $count = $counts[[array]::IndexOf($names, $name)]
        if ($count -lt 50 -or $count -le $other) {
            throw "$label does not show palette ${palette}: $name matched $count pixels, another pair's colour $other ($text)."
        }
    }
    return $text
}

# The launch script: the session settings, then per stage and player count a
# pass with the cues as each -IdentityCues entry asks.
$launch = @('+script.background', '1', '+intro', 'off', '+exec', 'study_b_begin', "$Seed")
$expected = @()
foreach ($stage in $Stages) {
    foreach ($count in $Players) {
        foreach ($cues in $IdentityCues) {
            if ($cues -eq 'off') { $launch += @('+exec', 'study_b_cues_off') }
            $launch += @('+exec', 'study_b_frames', $stage, "$count", "cues-$cues")
            if ($cues -eq 'off') { $launch += @('+exec', 'study_b_cues_on') }
            foreach ($frame in 'clean', 'heavy') {
                $expected += [pscustomobject]@{ stage = $stage; players = [int]$count; frame = $frame; cues = $cues
                    name = "$stage-${count}p-$frame-cues-$cues" }
            }
        }
    }
}
$launch += @('+echo', 'study-b:', 'done', '+quit')

$stimuli = Join-Path $output 'stimuli'
# The manifest lists this run's images only, so no older image may stay beside them.
Remove-Item -LiteralPath $stimuli -Recurse -Force -ErrorAction SilentlyContinue
$checksumPattern = 'checksum ([0-9a-f]{16}) at update (\d+), (paint [^\r\n,]*)'
$manifest = New-Object System.Collections.ArrayList
$simulation = @{}
$runs = @()

foreach ($variant in $Variants) {
    $executable = Get-Executable $variant
    foreach ($resolution in $Resolutions) {
        $size = $resolution.Split('x')
        foreach ($palette in $Palettes) {
            $run = "$variant-$resolution-$palette"
            $isolated = Join-Path $output "appdata\$run"
            Remove-Item -LiteralPath $isolated -Recurse -Force -ErrorAction SilentlyContinue
            $profileRoot = Join-Path $isolated 'ColourWars'
            $null = New-Item -ItemType Directory -Path (Join-Path $profileRoot 'debug') -Force
            # Only the display section is written; every other section reads
            # its defaults, as on a first run.
            $save = "{`"save_version`":5,`"resolution`":{`"x`":$($size[0]),`"y`":$($size[1])},`"fullscreen`":false,`"team_palette`":`"$palette`"}"
            [IO.File]::WriteAllText((Join-Path $profileRoot 'save_data.json'), $save, $utf8)
            Get-ChildItem -LiteralPath $PSScriptRoot -Filter 'study_b_*.cfg' |
                Copy-Item -Destination (Join-Path $profileRoot 'debug')

            $transcript = Join-Path $logs "capture-$run.log"
            Remove-Item -LiteralPath $transcript -ErrorAction SilentlyContinue
            $arguments = @('+logfile', ('"' + $transcript + '"')) + $launch
            Write-Host "capturing $run"
            $saved = $env:LOCALAPPDATA
            $env:LOCALAPPDATA = $isolated
            $previousQuietGame = Enter-QuietGame
            try {
                $process = Start-Process -FilePath $executable -ArgumentList $arguments -PassThru `
                    -WorkingDirectory $isolated
                $null = $process.Handle
            }
            finally { $env:LOCALAPPDATA = $saved; Exit-QuietGame $previousQuietGame }
            try {
                if (-not $process.WaitForExit($RunTimeoutSeconds * 1000)) {
                    throw "$run did not quit within $RunTimeoutSeconds s."
                }
                $process.WaitForExit()
                if ($process.ExitCode -ne 0) { throw "$run exited with $($process.ExitCode); see $transcript" }
            }
            finally {
                if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
            }

            $log = Read-Shared $transcript
            if ($log -notmatch '(?m)^study-b: done\r?$') { throw "$run never reached its end; see $transcript" }
            if ($log -match '(?m)^error: ' -or $log -match 'stopped: ') { throw "$run logged an error; see $transcript" }
            $markers = @([regex]::Matches($log, '(?m)^study-b: shot (\S+)'))
            $names = @($markers | ForEach-Object { $_.Groups[1].Value })
            if (($names -join ',') -ne (@($expected | ForEach-Object { $_.name }) -join ',')) {
                throw "$run shot [$($names -join ', ')], not the expected list."
            }
            $screens = Join-Path $profileRoot 'screens\studyb'
            for ($index = 0; $index -lt $markers.Count; ++$index) {
                $shot = $expected[$index]
                $from = $markers[$index].Index
                $sum = [regex]::Match($log.Substring($from), $checksumPattern)
                if (-not $sum.Success) { throw "$run $($shot.name): no checksum after its marker." }
                $source = Join-Path $screens "$($shot.name).png"
                if (-not (Test-Path -LiteralPath $source)) { throw "$run $($shot.name): no PNG at $source." }
                $relative = "$variant/$resolution/$($shot.players)p/$palette/$($shot.stage)-$($shot.frame)-cues-$($shot.cues).png"
                $target = Join-Path $stimuli ($relative -replace '/', '\')
                $null = New-Item -ItemType Directory -Path (Split-Path $target) -Force
                Copy-Item -LiteralPath $source -Destination $target -Force
                $pixels = Get-PngSize $target
                if ("$($pixels[0])x$($pixels[1])" -ne $resolution) {
                    throw "$run $($shot.name) is $($pixels[0])x$($pixels[1]), not $resolution."
                }
                $palettePixels = $null
                if ($shot.frame -eq 'heavy') {
                    $palettePixels = Test-Palette $variant $palette $shot.players $target "$run $($shot.name)"
                }
                $null = $manifest.Add([ordered]@{
                    variant = $variant; revision = $definitions[$variant].revision
                    resolution = $resolution; players = $shot.players; palette = $palette
                    stage = $shot.stage; shot = "$($shot.stage)-$($shot.frame)"; frame = $shot.frame
                    identity_cues = ($shot.cues -eq 'on'); seed = $Seed; path = $relative
                    sha256 = (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash.ToLowerInvariant()
                    checksum = $sum.Groups[1].Value; update = [int]$sum.Groups[2].Value; paint = $sum.Groups[3].Value
                    palette_pixels = $palettePixels
                })
                $key = "$resolution/$palette/$($shot.stage)/$($shot.players)p/$($shot.frame)/cues-$($shot.cues)"
                if (-not $simulation.ContainsKey($key)) { $simulation[$key] = [ordered]@{} }
                $simulation[$key][$variant] = "$($sum.Groups[1].Value) at update $($sum.Groups[2].Value), $($sum.Groups[3].Value)"
            }
            $runs += [ordered]@{ run = $run; transcript = "logs/capture-$run.log" }
        }
    }
}

# Every frame must be the same simulation in every variant, and the cue-off
# pass the same simulation as the cue-on pass.
$mismatches = @()
$report = [ordered]@{}
foreach ($key in ($simulation.Keys | Sort-Object)) {
    $values = @($simulation[$key].Values | Select-Object -Unique)
    $equal = ($values.Count -eq 1)
    if (-not $equal) { $mismatches += $key }
    $report[$key] = [ordered]@{ equal = $equal; variants = $simulation[$key] }
}
$cueMismatches = @()
foreach ($key in ($simulation.Keys | Where-Object { $_ -like '*/cues-on' } | Sort-Object)) {
    $other = $key -replace 'cues-on$', 'cues-off'
    if ($simulation.ContainsKey($other)) {
        foreach ($variant in $simulation[$key].Keys) {
            if ($simulation[$other].Contains($variant) -and $simulation[$other][$variant] -ne $simulation[$key][$variant]) {
                $cueMismatches += "$key $variant"
            }
        }
    }
}

$null = New-Item -ItemType Directory -Path $stimuli -Force
$sorted = @($manifest | Sort-Object { $_.path })
# Byte-identical images: a 1P frame shows team A only, so pairs that share
# team A's colour give the same 1P image, and V1b can match V1 where no
# slipped brick is in view. A test page drops or dedupes these.
$identical = @($sorted | Group-Object { $_.sha256 } | Where-Object { $_.Count -gt 1 } |
    ForEach-Object { [ordered]@{ sha256 = $_.Name; paths = @($_.Group | ForEach-Object { $_.path }) } } |
    Sort-Object { $_.paths[0] })
$document = [ordered]@{
    study = 'B'; seed = $Seed; preset = $preset
    variants = [ordered]@{}; identical_images = $identical; images = $sorted
}
foreach ($variant in $Variants) {
    $document.variants[$variant] = [ordered]@{
        revision = $definitions[$variant].revision; note = $definitions[$variant].note
    }
}
[IO.File]::WriteAllText((Join-Path $stimuli 'manifest.json'), ($document | ConvertTo-Json -Depth 6), $utf8)
$check = [ordered]@{
    compared = 'the Debug `checksum` line (state hash, update number, paint per team) logged just before each shot'
    variants_equal = ($mismatches.Count -eq 0); cue_passes_equal = ($cueMismatches.Count -eq 0)
    frames = $report; runs = $runs
}
[IO.File]::WriteAllText((Join-Path $stimuli 'simulation.json'), ($check | ConvertTo-Json -Depth 6), $utf8)

Write-Host "$($manifest.Count) images in $stimuli; $($identical.Count) groups of byte-identical images (manifest.json identical_images)"
if ($cueMismatches.Count -ne 0) {
    $cueMismatches | ForEach-Object { Write-Host "CUE MISMATCH $_" }
    throw "$($cueMismatches.Count) frame(s) differ between the cue-on and cue-off passes."
}
if ($Variants.Count -gt 1 -and $mismatches.Count -ne 0) {
    $mismatches | ForEach-Object { Write-Host "MISMATCH $_" }
    throw "$($mismatches.Count) frame(s) differ in simulation between variants: they are not art-only."
}
Write-Host "PASS Study B capture: every frame's checksum equal across $($Variants -join ', ')."
