[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Add-Type -AssemblyName System.Drawing

# The editable integer vector recipe replaces the retained historical generated
# sheet. Rasterise first, then expand each world pixel to four source texels.
if (-not ('ColourWars.ToolArt' -as [type])) {
    Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Text;
namespace ColourWars {
    public static class ToolArt {
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
        public static bool Paint(Color c) { return c.A != 0 && c.G == 0 && c.R == c.B && c.R != 0; }
        public static void Layer(Bitmap source, Bitmap atlas, Rectangle box, bool paint, int scale) {
            for (int y=0; y<source.Height; ++y) for (int x=0; x<source.Width; ++x) {
                Color c = source.GetPixel(x,y);
                if (c.A == 0 || Paint(c) != paint) continue;
                if (paint) c = Color.FromArgb(c.A,c.R,c.R,c.R);
                for (int dy=0;dy<scale;++dy) for (int dx=0;dx<scale;++dx)
                    atlas.SetPixel(box.X+x*scale+dx,box.Y+y*scale+dy,c);
            }
        }
        public static Bitmap Combined(Bitmap source, Color team) {
            var result = new Bitmap(source.Width,source.Height,PixelFormat.Format32bppArgb);
            for (int y=0;y<source.Height;++y) for (int x=0;x<source.Width;++x) {
                Color c=source.GetPixel(x,y);
                if (Paint(c)) c=Color.FromArgb(c.A,team.R*c.R/255,team.G*c.R/255,team.B*c.R/255);
                result.SetPixel(x,y,c);
            }
            return result;
        }
    }
    // Best-short-side packing, with stable input order and no rotation.
    public class ToolPacker {
        private List<Rectangle> free = new List<Rectangle>();
        public ToolPacker(int width,int height) { free.Add(new Rectangle(0,0,width,height)); }
        public Rectangle Add(int width,int height) {
            int best=-1,score=int.MaxValue,longScore=int.MaxValue;
            for(int i=0;i<free.Count;++i) {
                Rectangle f=free[i];
                if(width>f.Width || height>f.Height) continue;
                int s=Math.Min(f.Width-width,f.Height-height),ls=Math.Max(f.Width-width,f.Height-height);
                if(s<score || (s==score && ls<longScore)) { best=i;score=s;longScore=ls; }
            }
            if(best<0) throw new InvalidOperationException("Tool atlas is full.");
            Rectangle used=new Rectangle(free[best].X,free[best].Y,width,height);
            var next=new List<Rectangle>();
            foreach(Rectangle f in free) {
                if(!f.IntersectsWith(used)) { next.Add(f); continue; }
                if(used.Left>f.Left) next.Add(new Rectangle(f.Left,f.Top,used.Left-f.Left,f.Height));
                if(used.Right<f.Right) next.Add(new Rectangle(used.Right,f.Top,f.Right-used.Right,f.Height));
                if(used.Top>f.Top) next.Add(new Rectangle(f.Left,f.Top,f.Width,used.Top-f.Top));
                if(used.Bottom<f.Bottom) next.Add(new Rectangle(f.Left,used.Bottom,f.Width,f.Bottom-used.Bottom));
            }
            for(int i=0;i<next.Count;++i) for(int j=next.Count-1;j>=0;--j)
                if(i!=j && next[j].Contains(next[i])) { next.RemoveAt(i);--i;break; }
            free=next; return used;
        }
    }
}
'@
}

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$sourceDir = Join-Path $repo 'game/art/weapons'
$working = Join-Path $repo 'game/art/sources/textures'
$textures = Join-Path $repo 'game/content/textures'
$source = Get-Content -Raw (Join-Path $sourceDir 'weapons.json') | ConvertFrom-Json
if ($source.source_texels_per_world_unit -ne 4) { throw 'Tools must retain four source texels per world unit.' }
$scale = [int]$source.source_texels_per_world_unit
$presentation = Get-Content -Raw (Join-Path $repo 'game/content/presentation.json') | ConvertFrom-Json
$reviewScale = 720.0 / $presentation.camera.reference_height
if (($source.pose_order -join ',') -ne 'idle,motion_1,motion_2,lift') {
    throw 'Tool source must provide idle, motion_1, motion_2 and lift in that order.'
}
$colours = @{}
foreach ($entry in $source.palette.PSObject.Properties) {
    $colours[$entry.Name] = [Drawing.ColorTranslator]::FromHtml('#'+$entry.Value)
}
$contracts = [ordered]@{ sprayer=@(70,30); sniper=@(100,30); roller=@(100,50); mister=@(60,30); bucket=@(60,60) }

function Draw-Shape {
    param([Drawing.Graphics] $Graphics, [object[]] $Shape)
    $op = [string]$Shape[0]
    if (-not $colours.ContainsKey([string]$Shape[1])) { throw "Unknown tool material '$($Shape[1])'." }
    $colour = $colours[[string]$Shape[1]]
    $brush = [Drawing.SolidBrush]::new($colour)
    try {
        switch ($op) {
            'rect' { $Graphics.FillRectangle($brush,[int]$Shape[2],[int]$Shape[3],[int]$Shape[4],[int]$Shape[5]) }
            'ellipse' { $Graphics.FillEllipse($brush,[int]$Shape[2],[int]$Shape[3],[int]$Shape[4],[int]$Shape[5]) }
            { $_ -in @('line','poly') } {
                $first = if ($op -eq 'line') { 3 } else { 2 }
                $points = [Collections.Generic.List[Drawing.Point]]::new()
                for ($i=$first;$i -lt $Shape.Count;$i+=2) {
                    $points.Add([Drawing.Point]::new([int]$Shape[$i],[int]$Shape[$i+1]))
                }
                if ($op -eq 'line') {
                    $pen = [Drawing.Pen]::new($colour,[single]$Shape[2])
                    try { $Graphics.DrawLines($pen,$points.ToArray()) } finally { $pen.Dispose() }
                } else { $Graphics.FillPolygon($brush,$points.ToArray()) }
            }
            default { throw "Unknown tool operation '$op'." }
        }
    } finally { $brush.Dispose() }
}

$drawings = [ordered]@{}
$items = [Collections.Generic.List[object]]::new()
$frames = [Collections.Generic.List[object]]::new()
$atlas = [Drawing.Bitmap]::new([int]$source.atlas_width,[int]$source.atlas_height,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
$legacy = [Drawing.Bitmap]::FromFile((Join-Path $working 'sprite_sheet_1.png'))
try {
    foreach ($name in $contracts.Keys) {
        $tool = $source.tools.$name
        if ($tool.width -ne $contracts[$name][0] -or $tool.height -ne $contracts[$name][1]) {
            throw "Tool '$name' changed the existing world canvas contract."
        }
        foreach ($pose in $source.pose_order) {
            $frame = if ($pose -eq 'idle') { $name } else { "${name}_$pose" }
            $drawing = [Drawing.Bitmap]::new([int]$tool.width,[int]$tool.height,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
            $g = [Drawing.Graphics]::FromImage($drawing)
            try {
                $g.CompositingMode = [Drawing.Drawing2D.CompositingMode]::SourceCopy
                $g.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::None
                foreach ($shape in $tool.underlays.$pose) { Draw-Shape $g $shape }
                foreach ($shape in $tool.body) { Draw-Shape $g $shape }
                foreach ($shape in $tool.overlays.$pose) { Draw-Shape $g $shape }
            } finally { $g.Dispose() }
            foreach ($hand in @('rear','front')) {
                $grip = $presentation.figure.grips.$name.$hand
                $gx = [int]$grip.x; $gy = [int]($grip.y+$tool.height/2)
                if ($gx -lt 0 -or $gx -ge $drawing.Width -or $gy -lt 0 -or $gy -ge $drawing.Height -or
                    $drawing.GetPixel($gx,$gy).A -eq 0) {
                    throw "$frame has no $hand grip material at presentation.figure.grips.$name.$hand."
                }
            }
            if ($name -in @('sprayer','sniper','mister') -and
                $drawing.GetPixel(($drawing.Width-1),([int]($drawing.Height/2))).A -eq 0) {
                throw "$frame must end on its unchanged aim-line muzzle."
            }
            if ($name -eq 'roller' -and $drawing.GetPixel(75,25).ToArgb() -ne [Drawing.Color]::Black.ToArgb()) {
                throw "$frame must retain the roller axle at local (75,0)."
            }
            $drawings.Add($frame,$drawing)
            foreach ($paint in @($false,$true)) {
                $items.Add(@{ name=$frame+$(if($paint){'_paint'}else{''}); drawing=$drawing;
                    paint=$paint; width=[int]$tool.width*$scale; height=[int]$tool.height*$scale })
            }
        }
    }
    $packer = [ColourWars.ToolPacker]::new([int]$source.atlas_width,[int]$source.atlas_height)
    foreach ($item in ($items | Sort-Object @{Expression={$_.width*$_.height};Descending=$true},name)) {
        $allocated = $packer.Add(($item.width+8),($item.height+8))
        $box = [Drawing.Rectangle]::new(($allocated.X+4),($allocated.Y+4),$item.width,$item.height)
        [ColourWars.ToolArt]::Layer($item.drawing,$atlas,$box,$item.paint,$scale)
        $frames.Add([ordered]@{ name=$item.name;position=[ordered]@{x=$box.X;y=$box.Y};size=[ordered]@{w=$box.Width;h=$box.Height} })
    }
    # Utility sprites remain exact copies of the same legacy rectangles.
    $utilities = [ordered]@{ random=@(0,350,64,64);error=@(36,0,32,32);pixel=@(32,0,4,4);nozzle=@(100,150,4,4) }
    foreach ($name in $utilities.Keys) {
        $src=$utilities[$name];$allocated=$packer.Add(($src[2]+8),($src[3]+8))
        for($y=0;$y -lt $src[3];++$y){for($x=0;$x -lt $src[2];++$x){
            $atlas.SetPixel(($allocated.X+4+$x),($allocated.Y+4+$y),$legacy.GetPixel(($src[0]+$x),($src[1]+$y)))
        }}
        $frames.Add([ordered]@{name=$name;position=[ordered]@{x=$allocated.X+4;y=$allocated.Y+4};size=[ordered]@{w=$src[2];h=$src[3]}})
    }
    [ColourWars.ToolArt]::SavePng($atlas,(Join-Path $working 'weapon_paint.png'))
    $definition=[ordered]@{animation_strips=@();sprite_frames=@($frames.ToArray()|Sort-Object name)}
    [IO.File]::WriteAllText((Join-Path $textures 'weapon_paint.json'),(($definition|ConvertTo-Json -Depth 6)+[Environment]::NewLine),[Text.UTF8Encoding]::new($false))

    $preview=[Drawing.Bitmap]::new(1420,2280,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g=[Drawing.Graphics]::FromImage($preview)
    $font=[Drawing.Font]::new([Drawing.FontFamily]::GenericMonospace,10)
    $small=[Drawing.Font]::new([Drawing.FontFamily]::GenericMonospace,8)
    $ink=[Drawing.SolidBrush]::new([Drawing.Color]::Black)
    $ground=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(128,128,128))
    $painter=Get-Content -Raw (Join-Path $repo 'game/art/painter/painter.json')|ConvertFrom-Json
    $palettes=[ordered]@{neutral='FFFFFF';blue='0634CC';orange='FF7F00';pink='FF007F';green='00FF00';yellow='FFFF00'}
    try {
        $g.Clear([Drawing.Color]::FromArgb(228,228,228))
        $g.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
        $g.PixelOffsetMode=[Drawing.Drawing2D.PixelOffsetMode]::Half
        $g.DrawString('Paint.exe tools / integer vector source / fixed pivots and grips / four authored poses',$font,$ink,12,10)
        $g.DrawString('Enlarged: 2 px per world unit. Actual 4P 720p scale: 720 / camera.reference_height; neutral + five team hues.',$small,$ink,12,34)
        $row=0
        foreach($name in $contracts.Keys){
            $top=68+$row*435
            $g.DrawString($name,$font,$ink,12,$top)
            $col=0
            foreach($pose in $source.pose_order){
                $frame=if($pose -eq 'idle'){$name}else{"${name}_$pose"}
                $combined=[ColourWars.ToolArt]::Combined($drawings[$frame],[Drawing.ColorTranslator]::FromHtml('#0634CC'))
                try{
                    $x=145+310*$col;$g.DrawString($pose,$small,$ink,$x,$top)
                    $g.DrawImage($combined,[Drawing.Rectangle]::new($x,($top+24),($combined.Width*2),($combined.Height*2)))
                }finally{$combined.Dispose()}
                ++$col
            }
            $col=0
            foreach($entry in $palettes.GetEnumerator()){
                $x=12+474*($col%3);$y=$top+155+135*[Math]::Floor($col/3);$team=[Drawing.ColorTranslator]::FromHtml('#'+$entry.Value)
                $g.DrawString($entry.Key,$small,$ink,$x,$y);$g.FillRectangle($ground,$x,($y+18),440,106)
                # A body-sized E figure anchors the small tool samples.
                $body=[Drawing.Bitmap]::new(52,120);$bg=[Drawing.Graphics]::FromImage($body)
                try{
                    $bodyRows=@($painter.torso)+@($painter.legs.idle)
                    foreach($rows in @(@{rows=$painter.rest_arms},@{rows=$bodyRows})){
                        for($cy=0;$cy -lt 30;++$cy){for($cx=0;$cx -lt 13;++$cx){
                            $v=$rows.rows[$cy][$cx];if($v -eq '.'){continue}
                            $c=if($v -eq 'K'){[Drawing.Color]::Black}elseif($v -eq 'T'){$team}else{[Drawing.Color]::White}
                            $b=[Drawing.SolidBrush]::new($c)
                            try{$bg.FillRectangle($b,($cx*4),($cy*4),4,4)}finally{$b.Dispose()}
                        }}
                    }
                }finally{$bg.Dispose()}
                try{$g.DrawImage($body,[Drawing.Rectangle]::new(($x+5),($y+28),[int][Math]::Round(52*$reviewScale),[int][Math]::Round(120*$reviewScale)))}finally{$body.Dispose()}
                $sample=0
                foreach($pose in $source.pose_order){
                    $frame=if($pose -eq 'idle'){$name}else{"${name}_$pose"};$combined=[ColourWars.ToolArt]::Combined($drawings[$frame],$team)
                    try{
                        $w=[Math]::Max(1,[int][Math]::Round($combined.Width*$reviewScale));$h=[Math]::Max(1,[int][Math]::Round($combined.Height*$reviewScale))
                        $g.DrawImage($combined,[Drawing.Rectangle]::new(($x+65+$sample*90),($y+68-[int]($h/2)),$w,$h))
                    }finally{$combined.Dispose()}
                    ++$sample
                }
                ++$col
            }
            ++$row
        }
        [ColourWars.ToolArt]::SavePng($preview,(Join-Path $sourceDir 'lineup.png'))
    }finally{$ground.Dispose();$ink.Dispose();$small.Dispose();$font.Dispose();$g.Dispose();$preview.Dispose()}
}finally{
    foreach($drawing in $drawings.Values){$drawing.Dispose()}
    $legacy.Dispose();$atlas.Dispose()
}

# Premultiplied UNORM RGBA8, one mip; metadata-free PNG preserves authored bytes.
& (Join-Path $repo 'game/art/tools/texconv.exe') (Join-Path $working 'weapon_paint.png') `
    -pmalpha -m 1 -f R8G8B8A8_UNORM -y -o $textures
if($LASTEXITCODE -ne 0){throw "texconv failed with exit code $LASTEXITCODE"}
Write-Output 'Built 20 tool poses, complementary masks, retained utility frames, DDS and source lineup.'
