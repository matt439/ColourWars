[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Add-Type -AssemblyName System.Drawing

if (-not ('ColourWars.TerrainArt' -as [type])) {
    Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Text;
namespace ColourWars {
    public static class TerrainArt {
        public static void SavePng(Bitmap bitmap, string path) {
            using (var encoded = new MemoryStream()) {
                bitmap.Save(encoded, ImageFormat.Png);
                byte[] png = encoded.ToArray();
                using (var output = File.Create(path)) {
                    output.Write(png, 0, 8);
                    for (int offset = 8; offset < png.Length;) {
                        int length = (png[offset] << 24) | (png[offset+1] << 16) |
                            (png[offset+2] << 8) | png[offset+3];
                        string kind = Encoding.ASCII.GetString(png, offset+4, 4);
                        if (kind != "sRGB" && kind != "gAMA" && kind != "iCCP" && kind != "cHRM")
                            output.Write(png, offset, length+12);
                        offset += length+12;
                    }
                }
            }
        }
        // Inclusive Bresenham, so both end cells are always drawn and a line is one cell wide.
        public static void Line(Bitmap cells, Color c, int x0, int y0, int x1, int y1) {
            int dx = Math.Abs(x1-x0), sx = x0 < x1 ? 1 : -1;
            int dy = -Math.Abs(y1-y0), sy = y0 < y1 ? 1 : -1;
            int err = dx+dy;
            for (;;) {
                if (x0 >= 0 && y0 >= 0 && x0 < cells.Width && y0 < cells.Height) cells.SetPixel(x0, y0, c);
                if (x0 == x1 && y0 == y1) break;
                int e2 = 2*err;
                if (e2 >= dy) { err += dy; x0 += sx; }
                if (e2 <= dx) { err += dx; y0 += sy; }
            }
        }
        // A drawn cell 4-adjacent to transparency becomes outline, giving a thin staircase line.
        // Top-only edges treat the canvas sides and bottom as solid, and ignore cells below.
        public static void Outline(Bitmap cells, Color c, bool topOnly) {
            int w = cells.Width, h = cells.Height;
            var solid = new bool[w, h];
            for (int y=0; y<h; ++y) for (int x=0; x<w; ++x) solid[x,y] = cells.GetPixel(x,y).A != 0;
            for (int y=0; y<h; ++y) for (int x=0; x<w; ++x) {
                if (!solid[x,y]) continue;
                bool up = y == 0 || !solid[x,y-1];
                bool left = x == 0 ? !topOnly : !solid[x-1,y];
                bool right = x == w-1 ? !topOnly : !solid[x+1,y];
                bool down = !topOnly && (y == h-1 || !solid[x,y+1]);
                if (up || left || right || down) cells.SetPixel(x, y, c);
            }
        }
        public static void Copy(Bitmap source, Bitmap atlas, int x, int y, int scale) {
            for (int cy=0; cy<source.Height; ++cy) for (int cx=0; cx<source.Width; ++cx) {
                Color c=source.GetPixel(cx,cy);
                for (int dy=0; dy<scale; ++dy) for (int dx=0; dx<scale; ++dx)
                    atlas.SetPixel(x+cx*scale+dx,y+cy*scale+dy,c);
            }
        }
        // Counts drawn texels whose colour is outside the allowed ARGB set, or not fully opaque.
        public static int Foreign(Bitmap bitmap, int[] allowed) {
            var set = new HashSet<int>(allowed);
            int count = 0;
            for (int y=0; y<bitmap.Height; ++y) for (int x=0; x<bitmap.Width; ++x) {
                Color c = bitmap.GetPixel(x,y);
                if (c.A != 0 && (c.A != 255 || !set.Contains(c.ToArgb()))) ++count;
            }
            return count;
        }
        // Rectangles are x,y,w,h quadruples. Null when every frame keeps a transparent gutter
        // and nothing is drawn outside the frames; otherwise the first problem found.
        public static string Gutters(Bitmap atlas, int[] r, int gutter) {
            int n = r.Length/4;
            for (int i=0; i<n; ++i) {
                if (r[4*i] < gutter || r[4*i+1] < gutter ||
                    r[4*i]+r[4*i+2]+gutter > atlas.Width || r[4*i+1]+r[4*i+3]+gutter > atlas.Height)
                    return "frame " + i + " has no gutter at the atlas edge";
                for (int j=i+1; j<n; ++j)
                    if (r[4*i] < r[4*j]+r[4*j+2]+gutter && r[4*j] < r[4*i]+r[4*i+2]+gutter &&
                        r[4*i+1] < r[4*j+1]+r[4*j+3]+gutter && r[4*j+1] < r[4*i+1]+r[4*i+3]+gutter)
                        return "frames " + i + " and " + j + " touch without a gutter";
            }
            var owned = new bool[atlas.Width, atlas.Height];
            for (int i=0; i<n; ++i)
                for (int y=r[4*i+1]; y<r[4*i+1]+r[4*i+3]; ++y)
                    for (int x=r[4*i]; x<r[4*i]+r[4*i+2]; ++x) owned[x,y] = true;
            for (int y=0; y<atlas.Height; ++y) for (int x=0; x<atlas.Width; ++x)
                if (!owned[x,y] && atlas.GetPixel(x,y).A != 0)
                    return "gutter texel " + x + "," + y + " is drawn";
            return null;
        }
    }
}
'@
}

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$sourceDir = Join-Path $repo 'game/art/terrain'
$working = Join-Path $repo 'game/art/sources/textures'
$textures = Join-Path $repo 'game/content/textures'
$source = Get-Content -Raw (Join-Path $sourceDir 'terrain.json') | ConvertFrom-Json
$scale = [int]$source.cell_size
$gutter = [int]$source.gutter
if ($scale -ne 4 -or $gutter -lt 1) { throw 'Terrain keeps 4x4-texel cells and a transparent gutter.' }

# The 26 Muted Day roles, in order, are the contract every opaque texel is held to.
$roles = @('keyline','paper','bg_far','bg_mid','bg_line','bg_detail','ground_fill','ground_top',
    'ground_shade','pencil_body','pencil_shade','pencil_wood','graphite','ruler_fill','ruler_marks',
    'eraser_fill','eraser_shade','metal_light','metal_dark','shadow','shadow_deep','face','hud_paper',
    'hud_ink','highlight','prop_accent')
if ((@($source.palette.PSObject.Properties.Name) -join ',') -ne ($roles -join ',')) { throw 'Terrain palette roles changed.' }
$colours = [ordered]@{}
foreach ($role in $roles) {
    $hex = [string]$source.palette.$role
    if ($hex -notmatch '^[0-9A-F]{6}$') { throw "Palette role $role is not RRGGBB." }
    $colours[$role] = [Drawing.ColorTranslator]::FromHtml('#'+$hex)
}
function Get-Argb { param([string[]] $Names) [int[]]@($Names | ForEach-Object { $colours[$_].ToArgb() }) }
$paletteArgb = Get-Argb $roles
$backdropRoles = @('bg_far','bg_mid','bg_line','bg_detail','paper')

# Cell sizes are the contract: levels place these frames by name and size.
$propSizes = [ordered]@{hill_wide=@(384,80);hill_small=@(192,48);tree_round=@(32,56);tree_tall=@(24,64)
    house=@(96,88);cloud_big=@(64,24);cloud_small=@(40,16);sun=@(56,56);birds=@(24,8)
    brush_pot=@(64,80);tape_roll=@(64,40)}
if ((@($source.props.name) -join ',') -ne (@($propSizes.Keys) -join ',')) { throw 'Terrain prop list changed.' }
$tileNames = @('shadow_0','shadow_1','shadow_2','shadow_3')
if ((@($source.speckle.tiles.PSObject.Properties.Name) -join ',') -ne ($tileNames -join ',')) { throw 'Shadow tile list changed.' }

function Draw-Shapes {
    param([Drawing.Bitmap] $Cells, [object[]] $Shapes, [string[]] $Allowed, [string] $Name)
    $g=[Drawing.Graphics]::FromImage($Cells)
    try {
        $g.SmoothingMode=[Drawing.Drawing2D.SmoothingMode]::None
        $g.CompositingMode=[Drawing.Drawing2D.CompositingMode]::SourceCopy
        foreach ($shape in $Shapes) {
            $role=[string]$shape[1]
            if ($Allowed -notcontains $role) { throw "$Name may not draw with '$role'." }
            $n=[int[]]@($shape[2..($shape.Count-1)])
            $brush=[Drawing.SolidBrush]::new($colours[$role])
            try {
                switch([string]$shape[0]) {
                    'rect' { $g.FillRectangle($brush,$n[0],$n[1],$n[2],$n[3]) }
                    'ellipse' { $g.FillEllipse($brush,$n[0],$n[1],$n[2],$n[3]) }
                    'poly' {
                        $points=[Collections.Generic.List[Drawing.Point]]::new()
                        for($i=0;$i -lt $n.Count;$i+=2){$points.Add([Drawing.Point]::new($n[$i],$n[($i+1)]))}
                        $g.FillPolygon($brush,$points.ToArray())
                    }
                    { $_ -eq 'line' -or $_ -eq 'path' } {
                        $g.Flush()
                        for($i=0;$i+3 -lt $n.Count;$i+=2){
                            [ColourWars.TerrainArt]::Line($Cells,$colours[$role],$n[$i],$n[($i+1)],$n[($i+2)],$n[($i+3)])
                        }
                    }
                    default {throw "Unknown terrain shape '$($shape[0])' in $Name."}
                }
            } finally {$brush.Dispose()}
        }
    } finally {$g.Dispose()}
}

$drawings=[ordered]@{}
$atlas=$null
try {
    foreach ($prop in $source.props) {
        $w=[int]$prop.cells[0]; $h=[int]$prop.cells[1]
        if ($w -ne $propSizes[$prop.name][0] -or $h -ne $propSizes[$prop.name][1]) { throw "$($prop.name) changed size." }
        if ($prop.outline.role -ne 'bg_line') { throw "$($prop.name) must outline in bg_line, never black." }
        $cells=[Drawing.Bitmap]::new($w,$h,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $drawings[$prop.name]=$cells
        Draw-Shapes $cells @($prop.fill) @('bg_far','bg_mid','bg_detail','paper') $prop.name
        switch ([string]$prop.outline.edges) {
            'all' { [ColourWars.TerrainArt]::Outline($cells,$colours['bg_line'],$false) }
            'top' { [ColourWars.TerrainArt]::Outline($cells,$colours['bg_line'],$true) }
            default { throw "$($prop.name) outline edges must be all or top." }
        }
        Draw-Shapes $cells @($prop.detail) $backdropRoles $prop.name
        if ([ColourWars.TerrainArt]::Foreign($cells,(Get-Argb $backdropRoles))) {
            throw "$($prop.name) draws outside the backdrop roles."
        }
        if ($prop.name.StartsWith('hill_')) {
            for ($x=0;$x -lt $w;++$x) { if ($cells.GetPixel($x,($h-1)).A -ne 255) { throw "$($prop.name) must sit flat on the horizon." } }
        }
    }
    $tw=[int]$source.speckle.cells[0]; $th=[int]$source.speckle.cells[1]
    if ($tw -ne 16 -or $th -ne 8) { throw 'Shadow tiles are 16x8 cells.' }
    foreach ($name in $tileNames) {
        $rows=@($source.speckle.tiles.$name)
        if ($rows.Count -ne $th) { throw "$name needs $th rows." }
        $cells=[Drawing.Bitmap]::new($tw,$th,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $drawings[$name]=$cells
        for ($y=0;$y -lt $th;++$y) {
            $row=[string]$rows[$y]
            if ($row.Length -ne $tw) { throw "$name row $y is not $tw cells." }
            for ($x=0;$x -lt $tw;++$x) {
                $ch=[string]$row[$x]
                if ($ch -eq '.') { continue }
                # Empty end columns let any tile abut any other; the last row fades to nothing.
                if ($x -eq 0 -or $x -eq $tw-1 -or $y -eq $th-1) { throw "$name has a speck on its seam or last row." }
                $role=$source.speckle.legend.$ch
                if (-not $role) { throw "$name uses unknown speck '$ch'." }
                if (@('shadow','shadow_deep') -notcontains $role) {
                    throw "$name speck '$ch' must be shadow or shadow_deep, not '$role'."
                }
                $cells.SetPixel($x,$y,$colours[$role])
            }
        }
    }

    # Keep the original thirteen drawings and all swatches in their fixed
    # positions. New landmarks start on a fresh shelf after the old atlas.
    $order=@('house','hill_wide','tree_tall','sun','tree_round','hill_small','cloud_big','cloud_small','birds')+$tileNames
    $landmarks=@('brush_pot','tape_roll')
    $width=[int]$source.atlas_width
    $frames=[Collections.Generic.List[object]]::new()
    $x=$gutter; $y=$gutter; $shelf=0
    $place={
        param($name,$w,$h)
        if ($x+$w+$gutter -gt $width) { $script:x=$gutter; $script:y+=$shelf+$gutter; $script:shelf=0 }
        $frames.Add([ordered]@{name=$name;position=[ordered]@{x=$x;y=$y};size=[ordered]@{w=$w;h=$h}})
        $script:x+=$w+$gutter
        if ($h -gt $shelf) { $script:shelf=$h }
    }
    foreach ($name in $order) { & $place $name ($drawings[$name].Width*$scale) ($drawings[$name].Height*$scale) }
    foreach ($role in $roles) { & $place ('swatch_'+$role) $scale $scale }
    $x=$gutter; $y+=$shelf+$gutter; $shelf=0
    foreach ($name in $landmarks) { & $place $name ($drawings[$name].Width*$scale) ($drawings[$name].Height*$scale) }
    if ($frames.Count -ne 41) { throw 'Terrain atlas needs eleven props, four shadow tiles and 26 swatches.' }
    $height=$y+$shelf+$gutter

    $atlas=[Drawing.Bitmap]::new($width,$height,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $rects=[Collections.Generic.List[int]]::new()
    foreach ($frame in $frames) {
        $fx=$frame.position.x; $fy=$frame.position.y
        if ($frame.name.StartsWith('swatch_')) {
            $c=$colours[$frame.name.Substring(7)]
            for($dy=0;$dy -lt $scale;++$dy){for($dx=0;$dx -lt $scale;++$dx){$atlas.SetPixel($fx+$dx,$fy+$dy,$c)}}
        } else {
            [ColourWars.TerrainArt]::Copy($drawings[$frame.name],$atlas,$fx,$fy,$scale)
        }
        $rects.AddRange([int[]]@($fx,$fy,$frame.size.w,$frame.size.h))
    }
    $foreign=[ColourWars.TerrainArt]::Foreign($atlas,$paletteArgb)
    if ($foreign) { throw "$foreign opaque terrain texels are not Muted Day palette colours." }
    $gap=[ColourWars.TerrainArt]::Gutters($atlas,$rects.ToArray(),1)
    if ($gap) { throw "Terrain atlas: $gap." }

    [ColourWars.TerrainArt]::SavePng($atlas,(Join-Path $working 'terrain.png'))
    $definition=[ordered]@{animation_strips=@();sprite_frames=$frames.ToArray()}
    [IO.File]::WriteAllText((Join-Path $textures 'terrain.json'),
        (($definition|ConvertTo-Json -Depth 6)+[Environment]::NewLine),[Text.UTF8Encoding]::new($false))

    # Review sheet on paper: authored cells at 3 px, every frame at 1 px, tile seams, swatches.
    $preview=[Drawing.Bitmap]::new(1600,1400,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $lineupMargin=12; $lineupRight=$preview.Width-$lineupMargin; $lineupBottom=$preview.Height-$lineupMargin
    $g=[Drawing.Graphics]::FromImage($preview)
    # Review labels must not inherit desktop ClearType/font-smoothing state.
    $g.TextRenderingHint=[Drawing.Text.TextRenderingHint]::SingleBitPerPixelGridFit
    $g.TextContrast=4
    $font=[Drawing.Font]::new([Drawing.FontFamily]::GenericMonospace,10)
    $small=[Drawing.Font]::new([Drawing.FontFamily]::GenericMonospace,8)
    $ink=[Drawing.SolidBrush]::new($colours['hud_ink'])
    try {
        $g.Clear($colours['paper'])
        $g.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
        $g.PixelOffsetMode=[Drawing.Drawing2D.PixelOffsetMode]::Half
        $g.SmoothingMode=[Drawing.Drawing2D.SmoothingMode]::None
        $title="Terrain backdrop kit / Muted Day / atlas ${width}x$height"
        $titleSize=$g.MeasureString($title,$font)
        if (12+$titleSize.Width -gt $lineupRight -or 10+$titleSize.Height -gt 40) { throw 'Terrain lineup title overflows its header.' }
        $g.DrawString($title,$font,$ink,12,10)
        $script:lx=12; $script:ly=40; $script:lrow=0
        $script:lineupUsedBottom=40
        $show={
            param($label,$bitmap,$px)
            $w=$bitmap.Width*$px; $h=$bitmap.Height*$px
            $labelSize=$g.MeasureString($label,$small)
            $itemWidth=[Math]::Max($w,[Math]::Max($label.Length*7,[Math]::Ceiling($labelSize.Width)))
            if ($itemWidth -gt $lineupRight-$lineupMargin -or $labelSize.Height -gt 16) { throw "Terrain lineup item '$label' exceeds its label/image span." }
            if ($script:lx+$itemWidth -gt $lineupRight) { $script:lx=12; $script:ly+=$script:lrow+28; $script:lrow=0 }
            if ($script:ly+16+$h -gt $lineupBottom) { throw "Terrain lineup item '$label' overflows the review sheet." }
            $script:lineupUsedBottom=[Math]::Max($script:lineupUsedBottom,$script:ly+16+$h)
            $g.DrawString($label,$small,$ink,$script:lx,$script:ly)
            $g.DrawImage($bitmap,[Drawing.Rectangle]::new($script:lx,($script:ly+16),$w,$h))
            $script:lx+=$itemWidth+16
            if ($h+16 -gt $script:lrow) { $script:lrow=$h+16 }
        }
        foreach ($name in @($propSizes.Keys)+$tileNames) { & $show "$name 3x" $drawings[$name] 3 }
        $seam=[Drawing.Bitmap]::new(8*$tw,$th,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            $i=0
            foreach ($name in @($tileNames)+@('shadow_3','shadow_1','shadow_0','shadow_2')) {
                [ColourWars.TerrainArt]::Copy($drawings[$name],$seam,$i*$tw,0,1); ++$i
            }
            & $show 'shadow tiles abutted 0123 3102, 3x' $seam 3
        } finally {$seam.Dispose()}
        $script:lx=12; $script:ly+=$script:lrow+28; $script:lrow=0
        foreach ($name in @($propSizes.Keys)+$tileNames) { & $show "$name 1x" $drawings[$name] 1 }
        $script:lx=12; $script:ly+=$script:lrow+28; $script:lrow=0
        foreach ($role in $roles) {
            $labelSize=$g.MeasureString($role,$small)
            $swatchWidth=[Math]::Max(120,28+[Math]::Ceiling($labelSize.Width))
            if ($swatchWidth -gt $lineupRight-$lineupMargin) { throw "Terrain lineup swatch '$role' is too wide." }
            if ($script:lx+$swatchWidth -gt $lineupRight) { $script:lx=12; $script:ly+=40 }
            if ($script:ly+[Math]::Max(26,6+$labelSize.Height) -gt $lineupBottom) { throw "Terrain lineup swatch '$role' overflows the review sheet." }
            $script:lineupUsedBottom=[Math]::Max($script:lineupUsedBottom,$script:ly+[Math]::Max(26,6+$labelSize.Height))
            $brush=[Drawing.SolidBrush]::new($colours[$role])
            $pen=[Drawing.Pen]::new($colours['hud_ink'])
            try {
                $g.FillRectangle($brush,$script:lx,$script:ly,24,24)
                $g.DrawRectangle($pen,($script:lx-1),($script:ly-1),25,25)
            } finally {$pen.Dispose();$brush.Dispose()}
            $g.DrawString($role,$small,$ink,($script:lx+28),($script:ly+6))
            $script:lx+=150
        }
        $g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)
        [ColourWars.TerrainArt]::SavePng($preview,(Join-Path $sourceDir 'lineup.png'))
        Write-Output "Lineup $($preview.Width)x$($preview.Height): content bottom $lineupUsedBottom, limit $lineupBottom."
    } finally {$ink.Dispose();$small.Dispose();$font.Dispose();$g.Dispose();$preview.Dispose()}

    foreach ($frame in $frames) {
        if (-not $frame.name.StartsWith('swatch_')) {
            Write-Output ('{0,-12} {1,5},{2,4} {3,5}x{4}' -f $frame.name,$frame.position.x,$frame.position.y,$frame.size.w,$frame.size.h)
        }
    }
} finally {
    foreach($cells in $drawings.Values){$cells.Dispose()}
    if ($atlas) {$atlas.Dispose()}
}

& (Join-Path $repo 'game/art/tools/texconv.exe') (Join-Path $working 'terrain.png') `
    -pmalpha -m 1 -f R8G8B8A8_UNORM -y -o $textures
if($LASTEXITCODE -ne 0){throw "texconv failed with exit code $LASTEXITCODE"}
Write-Output "Built eleven backdrop props, four shadow tiles and 26 swatches into a ${width}x$height terrain DDS and lineup."
