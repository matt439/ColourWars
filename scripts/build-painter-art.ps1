[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Add-Type -AssemblyName System.Drawing

if (-not ('ColourWars.PainterArt' -as [type])) {
    Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Text;
namespace ColourWars {
    public static class PainterArt {
        public static void SavePng(Bitmap bitmap, string path) {
            // texconv must preserve UNORM bytes, as in build-weapon-art.ps1.
            using (var encoded = new MemoryStream()) {
                bitmap.Save(encoded, ImageFormat.Png);
                byte[] png = encoded.ToArray();
                using (var output = File.Create(path)) {
                    output.Write(png, 0, 8);
                    for (int offset = 8; offset < png.Length;) {
                        int length = (png[offset] << 24) | (png[offset + 1] << 16) |
                            (png[offset + 2] << 8) | png[offset + 3];
                        string kind = Encoding.ASCII.GetString(png, offset + 4, 4);
                        if (kind != "sRGB" && kind != "gAMA" && kind != "iCCP" && kind != "cHRM")
                            output.Write(png, offset, length + 12);
                        offset += length + 12;
                    }
                }
            }
        }

        public static void Grid(Bitmap bitmap, string[] rows, int x, int y,
            int cell, bool paint, Color team, bool combined) {
            for (int row = 0; row < rows.Length; ++row) {
                for (int column = 0; column < rows[row].Length; ++column) {
                    char value = rows[row][column];
                    if (value == '.' || (!combined && (value == 'T') != paint)) continue;
                    Color colour = value == 'K' ? Color.Black :
                        (value == 'T' ? team : Color.White);
                    for (int dy = 0; dy < cell; ++dy)
                        for (int dx = 0; dx < cell; ++dx)
                            bitmap.SetPixel(x + column * cell + dx, y + row * cell + dy, colour);
                }
            }
        }
    }
}
'@
}

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$sourceDir = Join-Path $repo 'game/art/painter'
$working = Join-Path $repo 'game/art/sources/textures'
$textures = Join-Path $repo 'game/content/textures'
$source = Get-Content -Raw (Join-Path $sourceDir 'painter.json') | ConvertFrom-Json
if ($source.cell_size -ne 4 -or $source.width -ne 13 -or $source.height -ne 30) {
    throw 'Painter source must be a 13x30 grid with four source texels per cell.'
}

function Assert-Grid {
    param([string[]] $Rows, [int] $Width, [int] $Height, [string] $Name)
    if ($Rows.Count -ne $Height) { throw "$Name has $($Rows.Count) rows; expected $Height." }
    foreach ($row in $Rows) {
        if ($row.Length -ne $Width -or $row -match '[^.KWT]') {
            throw "Invalid $Name row '$row'; expected $Width cells using . K W T."
        }
    }
}

Assert-Grid $source.torso 13 22 'torso'
$poses = [ordered]@{}
foreach ($name in @('idle', 'run_0', 'run_1', 'run_2', 'run_3', 'run_4',
    'run_5', 'run_6', 'run_7', 'jump', 'fall')) {
    [string[]] $legs = $source.legs.$name
    Assert-Grid $legs 13 8 $name
    [string[]] $rows = @($source.torso) + @($legs)
    # Source rectangles are contiguous in Labrador strips; each authored pose
    # reserves one empty cell on each side, giving eight visible-art gutter texels.
    if (@($rows | Where-Object { $_[0] -ne '.' -or $_[12] -ne '.' }).Count) {
        throw "$name must keep four transparent texels at both horizontal edges."
    }
    if ($name -ne 'jump' -and $rows[29] -notmatch 'K') {
        throw "$name must plant at least one foot on the y=120 baseline."
    }
    $poses.Add($name, $rows)
}
Assert-Grid $source.rest_arms 13 30 'rest_arms'
$actions = @('takeoff', 'land', 'fire', 'hurt', 'elimination')
foreach ($name in $actions) {
    $action = $source.actions.$name
    if ($action.frames.Count -ne 2 -or $action.frame_time -le 0) {
        throw "$name must author two full frames and a positive frame_time."
    }
    for ($frame = 0; $frame -lt $action.frames.Count; ++$frame) {
        [string[]] $rows = $action.frames[$frame]
        $pose = "${name}_$frame"
        Assert-Grid $rows 13 30 $pose
        if (@($rows | Where-Object { $_[0] -ne '.' -or $_[12] -ne '.' }).Count) {
            throw "$pose must keep four transparent texels at both horizontal edges."
        }
        # The above-head accent and live grip arms use the same roots in every
        # action. Expressions and the lower bib can move without moving either.
        foreach ($row in @(0, 1, 2, 3, 4, 11, 12, 13, 14, 15)) {
            if ($rows[$row] -ne $source.torso[$row]) {
                throw "$pose must preserve the cap, head outline and shoulder roots."
            }
        }
        $poses.Add($pose, $rows)
    }
}
$accents = @('accent_bobble', 'accent_antenna', 'accent_fork', 'accent_flag')
foreach ($accent in $accents) { Assert-Grid $source.$accent 3 3 $accent }

# The fill must finish exactly where the live respawn pencil left its outline.
# Validate the authored presentation before changing any generated output.
$look = (Get-Content -Raw (Join-Path $repo 'game/content/presentation.json') |
    ConvertFrom-Json).respawn_redraw
if ($look.canvas_size.x -ne 52 -or $look.canvas_size.y -ne 120 -or $look.pen_width -ne 4) {
    throw 'Painter redraw requires respawn_redraw canvas_size 52x120 and pen_width 4 in presentation.json.'
}
$expected = [Collections.Generic.HashSet[int]]::new()
foreach ($grid in @(@{ rows = $poses['idle'] }, @{ rows = $source.rest_arms })) {
    for ($row = 0; $row -lt 30; ++$row) {
        for ($column = 0; $column -lt 13; ++$column) {
            if ($grid.rows[$row][$column] -ne 'K') { continue }
            for ($dy = 0; $dy -lt 4; ++$dy) {
                for ($dx = 0; $dx -lt 4; ++$dx) {
                    [void] $expected.Add((($row * 4 + $dy) * 52 + $column * 4 + $dx))
                }
            }
        }
    }
}
$drawn = [Collections.Generic.HashSet[int]]::new()
foreach ($stroke in $look.strokes) {
    foreach ($point in $stroke.points) {
        if ($point.x -lt 0 -or $point.x -gt 48 -or $point.y -lt 0 -or $point.y -gt 116 -or
            $point.x % 4 -ne 0 -or $point.y % 4 -ne 0) {
            throw "Painter redraw '$($stroke.part)' points must stay on the 4px grid inside the 52x120 canvas."
        }
    }
    for ($at = 1; $at -lt $stroke.points.Count; ++$at) {
        $from = $stroke.points[$at - 1]
        $to = $stroke.points[$at]
        if (($from.x -eq $to.x) -eq ($from.y -eq $to.y)) {
            throw "Painter redraw '$($stroke.part)' needs nonzero horizontal or vertical segments."
        }
        for ($y = [Math]::Min($from.y, $to.y); $y -lt [Math]::Max($from.y, $to.y) + 4; ++$y) {
            for ($x = [Math]::Min($from.x, $to.x); $x -lt [Math]::Max($from.x, $to.x) + 4; ++$x) {
                [void] $drawn.Add(($y * 52 + $x))
            }
        }
    }
}
if (-not $expected.SetEquals($drawn)) {
    $missing = @($expected | Where-Object { -not $drawn.Contains($_) }).Count
    $extra = @($drawn | Where-Object { -not $expected.Contains($_) }).Count
    throw "Painter redraw differs from idle + rest_arms: $missing missing black texels, $extra extra. Update presentation.json respawn_redraw.strokes to match game/art/painter/painter.json; see its README pen recipe."
}
Write-Output "Validated respawn pencil against $($expected.Count) authored black texels."

$atlas = [Drawing.Bitmap]::new(512, 2048, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
$strips = [Collections.Generic.List[object]]::new()
$frames = [Collections.Generic.List[object]]::new()

function Add-Frame {
    param([string] $Name, [int] $X, [int] $Y, [int] $W, [int] $H)
    $frames.Add([ordered]@{ name = $Name; position = [ordered]@{ x = $X; y = $Y };
        size = [ordered]@{ w = $W; h = $H } })
}

try {
    foreach ($paint in @($false, $true)) {
        $suffix = if ($paint) { '_paint' } else { '' }
        $layerY = if ($paint) { 512 } else { 0 }
        $row = 0
        foreach ($name in @('idle', 'run', 'jump', 'fall')) {
            $count = if ($name -eq 'run') { 8 } else { 1 }
            $y = 4 + $layerY + 128 * $row
            for ($frame = 0; $frame -lt $count; ++$frame) {
                $pose = if ($count -eq 1) { $name } else { "run_$frame" }
                [ColourWars.PainterArt]::Grid($atlas, $poses[$pose], 4 + 52 * $frame,
                    $y, 4, $paint, [Drawing.Color]::White, $false)
            }
            $strips.Add([ordered]@{ name = $name + $suffix;
                first_frame = [ordered]@{ x = 4; y = $y; w = 52; h = 120 };
                frame_count = $count; frame_time = $(if ($name -eq 'run') { 0.2 } else { 0.05 });
                looping = $true })
            if ($name -eq 'idle') { Add-Frame ($name + $suffix) 4 $y 52 120 }
            ++$row
        }
    }

    # Keep every existing locomotion/accessory rectangle byte-for-byte. New
    # action rows pair neutral and paint strips below the original 1024px atlas.
    for ($row = 0; $row -lt $actions.Count; ++$row) {
        $name = $actions[$row]
        $action = $source.actions.$name
        $y = 1028 + 128 * $row
        foreach ($paint in @($false, $true)) {
            $suffix = if ($paint) { '_paint' } else { '' }
            $x = if ($paint) { 116 } else { 4 }
            for ($frame = 0; $frame -lt $action.frames.Count; ++$frame) {
                [ColourWars.PainterArt]::Grid($atlas, $poses["${name}_$frame"],
                    ($x + 52 * $frame), $y, 4, $paint, [Drawing.Color]::White, $false)
            }
            $strips.Add([ordered]@{ name = $name + $suffix;
                first_frame = [ordered]@{ x = $x; y = $y; w = 52; h = 120 };
                frame_count = $action.frames.Count; frame_time = $action.frame_time;
                looping = $false })
        }
    }

    [ColourWars.PainterArt]::Grid($atlas, $source.rest_arms, 68, 4, 4,
        $false, [Drawing.Color]::White, $false)
    Add-Frame 'rest_arms' 68 4 52 120
    for ($index = 0; $index -lt $accents.Count; ++$index) {
        $accent = $accents[$index]
        $x = 128 + 20 * $index
        [ColourWars.PainterArt]::Grid($atlas, $source.$accent, $x, 4, 4,
            $false, [Drawing.Color]::White, $false)
        Add-Frame $accent $x 4 12 12
    }
    [ColourWars.PainterArt]::Grid($atlas, @('W'), 208, 4, 4,
        $false, [Drawing.Color]::White, $false)
    Add-Frame 'pixel' 208 4 4 4

    [ColourWars.PainterArt]::SavePng($atlas, (Join-Path $working 'painter.png'))
    $definition = [ordered]@{ animation_strips = $strips.ToArray(); sprite_frames = $frames.ToArray() }
    [IO.File]::WriteAllText((Join-Path $textures 'painter.json'),
        ($definition | ConvertTo-Json -Depth 6) + [Environment]::NewLine,
        [Text.UTF8Encoding]::new($false))
}
finally { $atlas.Dispose() }

# Production source preview: every authored pose in each supported hue. The
# footer shows all four above-head accents at native size and at 28 px tall.
$preview = [Drawing.Bitmap]::new(1032, 1852, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
$graphics = [Drawing.Graphics]::FromImage($preview)
$font = [Drawing.Font]::new([Drawing.FontFamily]::GenericMonospace, 9)
$brush = [Drawing.SolidBrush]::new([Drawing.Color]::Black)
$ground = [Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(128, 128, 128))
try {
    $graphics.Clear([Drawing.Color]::FromArgb(228, 228, 228))
    $graphics.DrawString('E painter / editable 13x30 cells / native 52x120', $font, $brush, 12, 8)
    $palettes = [ordered]@{ blue = '0634CC'; orange = 'FF7F00'; pink = 'FF007F'; green = '00FF00'; yellow = 'FFFF00' }
    $row = 0
    foreach ($entry in $palettes.GetEnumerator()) {
        $colour = [Drawing.ColorTranslator]::FromHtml('#' + $entry.Value)
        $column = 0
        foreach ($name in $poses.Keys) {
            # Eleven locomotion drawings followed by a row of ten action frames.
            $poseRow = if ($column -lt 11) { 0 } else { 1 }
            $poseColumn = if ($column -lt 11) { $column } else { $column - 11 }
            $y = 52 + 312 * $row + 156 * $poseRow
            $x = 92 + 84 * $poseColumn
            if ($poseColumn -eq 0) {
                $graphics.DrawString($entry.Key, $font, $brush, 8, $y + 50)
            }
            $label = $name.Replace('elimination', 'elim')
            $graphics.DrawString($label, $font, $brush, $x - 2, $y - 20)
            $graphics.FillRectangle($ground, $x - 4, $y + 120, 60, 4)
            $graphics.Flush()
            [ColourWars.PainterArt]::Grid($preview, $source.rest_arms, $x, $y, 4,
                $false, $colour, $true)
            [ColourWars.PainterArt]::Grid($preview, $poses[$name], $x, $y, 4,
                $false, $colour, $true)
            ++$column
        }
        ++$row
    }
    $graphics.DrawString('Teammate accents / 120 px and 28 px bodies (same source)', $font, $brush, 12, 1604)
    for ($index = 0; $index -lt $accents.Count; ++$index) {
        $accent = $accents[$index]
        $x = 40 + 240 * $index
        $graphics.Flush()
        [ColourWars.PainterArt]::Grid($preview, $poses['idle'], $x, 1640, 4,
            $false, [Drawing.ColorTranslator]::FromHtml('#0634CC'), $true)
        [ColourWars.PainterArt]::Grid($preview, $source.$accent, $x + 16, 1628, 4,
            $false, [Drawing.Color]::White, $true)
        $graphics.DrawString($accent, $font, $brush, $x + 56, 1664)
    }
    $graphics.Flush()
    # DrawImage uses the same source sprite as the native preview; no resampling
    # is involved in authoring the runtime atlas itself.
    $graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
    $graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::Half
    for ($index = 0; $index -lt $accents.Count; ++$index) {
        $x = 40 + 240 * $index
        $graphics.DrawImage($preview, [Drawing.Rectangle]::new(($x + 96), 1708, 12, 31),
            $x, 1628, 52, 132, [Drawing.GraphicsUnit]::Pixel)
    }
    $graphics.DrawString('Action silhouettes / 28 px bodies (nearest-neighbour preview)', $font, $brush, 12, 1768)
    for ($column = 0; $column -lt 10; ++$column) {
        $x = 92 + 84 * $column
        $graphics.DrawImage($preview, [Drawing.Rectangle]::new($x, 1792, 12, 28),
            $x, 208, 52, 120, [Drawing.GraphicsUnit]::Pixel)
        $name = @($poses.Keys)[$column + 11]
        $graphics.DrawString($name.Replace('elimination', 'elim'), $font, $brush, $x - 2, 1828)
    }
    [ColourWars.PainterArt]::SavePng($preview, (Join-Path $sourceDir 'lineup.png'))
}
finally {
    $ground.Dispose(); $brush.Dispose(); $font.Dispose(); $graphics.Dispose(); $preview.Dispose()
}

# Labrador uses premultiplied alpha. RGBA8 and one mip retain the authored grid.
& (Join-Path $repo 'game/art/tools/texconv.exe') (Join-Path $working 'painter.png') `
    -pmalpha -m 1 -f R8G8B8A8_UNORM -y -o $textures
if ($LASTEXITCODE -ne 0) { throw "texconv failed with exit code $LASTEXITCODE" }
Write-Output 'Built painter.png, painter.json, premultiplied painter.dds and source lineup.png.'
