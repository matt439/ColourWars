[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Add-Type -AssemblyName System.Drawing

if (-not ('ColourWars.PaintFxArt' -as [type])) {
    Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Text;
namespace ColourWars {
    public static class PaintFxArt {
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
        // Outline the union, not each constituent ellipse: joined lobes stay liquid.
        public static Bitmap Finish(Bitmap mask, Bitmap tones, int keyline) {
            var result = new Bitmap(mask.Width, mask.Height, PixelFormat.Format32bppArgb);
            for (int y=0; y<mask.Height; ++y) for (int x=0; x<mask.Width; ++x) {
                if (mask.GetPixel(x,y).A == 0) continue;
                bool edge = false;
                for (int dy=-keyline; dy<=keyline && !edge; ++dy)
                    for (int dx=-keyline; dx<=keyline; ++dx) {
                        int sx=x+dx, sy=y+dy;
                        if (sx<0 || sy<0 || sx>=mask.Width || sy>=mask.Height ||
                            mask.GetPixel(sx,sy).A == 0) { edge=true; break; }
                    }
                Color c=tones.GetPixel(x,y);
                if (c.A == 0) c=Color.FromArgb(232,232,232);
                result.SetPixel(x,y,edge ? Color.Black : c);
            }
            return result;
        }
        public static void Copy(Bitmap source, Bitmap atlas, int x, int y, int scale) {
            for (int cy=0; cy<source.Height; ++cy) for (int cx=0; cx<source.Width; ++cx) {
                Color c=source.GetPixel(cx,cy);
                for (int dy=0; dy<scale; ++dy) for (int dx=0; dx<scale; ++dx)
                    atlas.SetPixel(x+cx*scale+dx,y+cy*scale+dy,c);
            }
        }
        public static Bitmap Tint(Bitmap source, Color team) {
            var result=new Bitmap(source.Width,source.Height,PixelFormat.Format32bppArgb);
            for(int y=0;y<source.Height;++y) for(int x=0;x<source.Width;++x) {
                Color c=source.GetPixel(x,y);
                result.SetPixel(x,y,Color.FromArgb(c.A,team.R*c.R/255,team.G*c.G/255,team.B*c.B/255));
            }
            return result;
        }
    }
}
'@
}

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$sourceDir = Join-Path $repo 'game/art/paint-fx'
$working = Join-Path $repo 'game/art/sources/textures'
$textures = Join-Path $repo 'game/content/textures'
$source = Get-Content -Raw (Join-Path $sourceDir 'paint-fx.json') | ConvertFrom-Json
if ($source.canvas_size -ne 32 -or $source.source_texels_per_cell -ne 4) {
    throw 'Paint FX keeps 32x32 vector cells expanded to 128x128 source frames.'
}
$expected = @('shot_spray','shot_jet','shot_rolling','shot_mist','shot_ball',
    'impact_spray','impact_jet','impact_rolling','impact_mist','impact_ball')
if (($source.strips.name -join ',') -ne ($expected -join ',')) { throw 'Paint FX strip contract changed.' }
$size = [int]$source.canvas_size
$scale = [int]$source.source_texels_per_cell
$frameSize = $size*$scale

function Draw-Shapes {
    param([Drawing.Graphics] $Graphics, [object[]] $Shapes, [bool] $Tones)
    foreach ($shape in $Shapes) {
        $start=if($Tones){2}else{1}
        $tone=if($Tones){[int]$shape[1]}else{255}
        if($tone -lt 0 -or $tone -gt 255){throw 'Paint FX tone must be grayscale 0..255.'}
        $brush=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb($tone,$tone,$tone))
        try {
            switch([string]$shape[0]) {
                'ellipse' { $Graphics.FillEllipse($brush,[int]$shape[$start],[int]$shape[($start+1)],
                    [int]$shape[($start+2)],[int]$shape[($start+3)]) }
                'poly' {
                    $points=[Collections.Generic.List[Drawing.Point]]::new()
                    for($i=$start;$i -lt $shape.Count;$i+=2){
                        $points.Add([Drawing.Point]::new([int]$shape[$i],[int]$shape[($i+1)]))
                    }
                    $Graphics.FillPolygon($brush,$points.ToArray())
                }
                default {throw "Unknown Paint FX shape '$($shape[0])'."}
            }
        } finally {$brush.Dispose()}
    }
}

$atlas=[Drawing.Bitmap]::new([int]$source.atlas_width,[int]$source.atlas_height,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
$strips=[Collections.Generic.List[object]]::new()
$drawings=[ordered]@{}
try {
    $row=0
    foreach($strip in $source.strips){
        $impact=$strip.name.StartsWith('impact_')
        $count=$strip.frames.Count
        if(($impact -and ($count -ne 5 -or $strip.looping)) -or
            (-not $impact -and ($count -ne 3 -or -not $strip.looping))){
            throw "$($strip.name) has an invalid transient/looping frame contract."
        }
        $y=4+$row*($frameSize+8)
        $stripDrawings=[Collections.Generic.List[Drawing.Bitmap]]::new()
        for($index=0;$index -lt $count;++$index){
            $frame=$strip.frames[$index]
            if($impact -and $index -eq $count-1 -and ($frame.lobes.Count -or $frame.tones.Count)){
                throw "$($strip.name) must finish on a fully transparent frame."
            }
            $mask=[Drawing.Bitmap]::new($size,$size)
            $tones=[Drawing.Bitmap]::new($size,$size)
            foreach($layer in @(@{bitmap=$mask;shapes=$frame.lobes;tones=$false},
                @{bitmap=$tones;shapes=$frame.tones;tones=$true})){
                $g=[Drawing.Graphics]::FromImage($layer.bitmap)
                try {
                    $g.SmoothingMode=[Drawing.Drawing2D.SmoothingMode]::None
                    $g.CompositingMode=[Drawing.Drawing2D.CompositingMode]::SourceCopy
                    Draw-Shapes $g $layer.shapes $layer.tones
                } finally {$g.Dispose()}
            }
            try {$drawing=[ColourWars.PaintFxArt]::Finish($mask,$tones,[int]$strip.keyline)}
            finally {$mask.Dispose();$tones.Dispose()}
            # Transparent edge cells prevent neighbouring contiguous strip frames bleeding.
            for($edge=0;$edge -lt $size;++$edge){
                if($drawing.GetPixel(0,$edge).A -or $drawing.GetPixel(($size-1),$edge).A -or
                    $drawing.GetPixel($edge,0).A -or $drawing.GetPixel($edge,($size-1)).A){
                    throw "$($strip.name) frame $index needs a transparent canvas border."
                }
            }
            if($strip.name -eq 'shot_jet'){
                for($cy=0;$cy -lt $size;++$cy){for($cx=0;$cx -lt $size;++$cx){
                    if(-not $drawing.GetPixel($cx,$cy).A){continue}
                    $dx=[Math]::Abs($cx-$size/2.0+0.5)+0.5
                    $dy=[Math]::Abs($cy-$size/2.0+0.5)+0.5
                    if($dx*$dx+$dy*$dy -gt $size*$size/4.0){
                        throw 'Jet artwork escapes its inscribed circle, and can escape existing bounds when aimed.'
                    }
                }}
            }
            [ColourWars.PaintFxArt]::Copy($drawing,$atlas,(4+$index*$frameSize),$y,$scale)
            $stripDrawings.Add($drawing)
        }
        $drawings.Add($strip.name,$stripDrawings)
        $strips.Add([ordered]@{name=$strip.name;first_frame=[ordered]@{x=4;y=$y;w=$frameSize;h=$frameSize};
            frame_count=$count;frame_time=$strip.frame_time;looping=$strip.looping})
        ++$row
    }
    [ColourWars.PaintFxArt]::SavePng($atlas,(Join-Path $working 'paint_fx.png'))
    $definition=[ordered]@{animation_strips=$strips.ToArray();sprite_frames=@()}
    [IO.File]::WriteAllText((Join-Path $textures 'paint_fx.json'),
        (($definition|ConvertTo-Json -Depth 6)+[Environment]::NewLine),[Text.UTF8Encoding]::new($false))

    $tuning=Get-Content -Raw (Join-Path $repo 'game/content/tuning.json')|ConvertFrom-Json
    $presentation=Get-Content -Raw (Join-Path $repo 'game/content/presentation.json')|ConvertFrom-Json
    $reviewScale=720.0/$presentation.camera.reference_height
    $preview=[Drawing.Bitmap]::new(1440,1472,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g=[Drawing.Graphics]::FromImage($preview)
    $font=[Drawing.Font]::new([Drawing.FontFamily]::GenericMonospace,10)
    $small=[Drawing.Font]::new([Drawing.FontFamily]::GenericMonospace,8)
    $ink=[Drawing.SolidBrush]::new([Drawing.Color]::Black)
    $ground=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(128,128,128))
    try {
        $g.Clear([Drawing.Color]::FromArgb(228,228,228))
        $g.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
        $g.PixelOffsetMode=[Drawing.Drawing2D.PixelOffsetMode]::Half
        $g.DrawString('Paint.exe liquid FX / merged lobes + detached droplets / grayscale team tint',$font,$ink,12,10)
        $g.DrawString('Left: authored cells at 3x. Right: actual 720p scale, unchanged shot sizes; impact = thickness x splash_scale.',$small,$ink,12,34)
        $palettes=[ordered]@{blue='0634CC';orange='FF7F00';pink='FF007F';green='00FF00';yellow='FFFF00'}
        $row=0
        foreach($strip in $source.strips){
            $top=66+$row*138
            $g.DrawString($strip.name,$font,$ink,12,$top)
            $frame=0
            foreach($drawing in $drawings[$strip.name]){
                $sample=[ColourWars.PaintFxArt]::Tint($drawing,[Drawing.ColorTranslator]::FromHtml('#0634CC'))
                try{
                    $g.FillRectangle($ground,(12+$frame*100),($top+24),96,96)
                    $g.DrawImage($sample,[Drawing.Rectangle]::new((12+$frame*100),($top+24),96,96))
                } finally {$sample.Dispose()}
                ++$frame
            }
            $kind=$strip.name.Substring($strip.name.IndexOf('_')+1)
            $worldSize=if($strip.name.StartsWith('shot_')){[double]$tuning.projectiles.$kind.size.x}
                else{[double]$tuning.paint.thickness*[double]$tuning.paint.visual.splash_scale}
            $pixelSize=[Math]::Max(1,[int][Math]::Round($worldSize*$reviewScale))
            $col=0
            foreach($entry in $palettes.GetEnumerator()){
                $x=532+$col*178
                $g.DrawString(($entry.Key+' '+$pixelSize+'px'),$small,$ink,$x,$top)
                $g.FillRectangle($ground,$x,($top+24),168,96)
                # All frames fit at real scale. Large bucket shots use one live
                # frame plus two staggered frames; impacts show the whole decay.
                $frame=0
                foreach($drawing in $drawings[$strip.name]){
                    $sample=[ColourWars.PaintFxArt]::Tint($drawing,[Drawing.ColorTranslator]::FromHtml('#'+$entry.Value))
                    try{
                        $sampleX=if($pixelSize -gt 48){$x+4+47*$frame}else{$x+7+32*$frame}
                        $sampleY=if($pixelSize -gt 48){$top+28+10*($frame%2)}else{$top+72-[int]($pixelSize/2)}
                        $g.DrawImage($sample,[Drawing.Rectangle]::new($sampleX,$sampleY,$pixelSize,$pixelSize))
                    } finally {$sample.Dispose()}
                    ++$frame
                }
                ++$col
            }
            ++$row
        }
        [ColourWars.PaintFxArt]::SavePng($preview,(Join-Path $sourceDir 'lineup.png'))
    } finally {$ground.Dispose();$ink.Dispose();$small.Dispose();$font.Dispose();$g.Dispose();$preview.Dispose()}
} finally {
    foreach($stripDrawings in $drawings.Values){foreach($drawing in $stripDrawings){$drawing.Dispose()}}
    $atlas.Dispose()
}

& (Join-Path $repo 'game/art/tools/texconv.exe') (Join-Path $working 'paint_fx.png') `
    -pmalpha -m 1 -f R8G8B8A8_UNORM -y -o $textures
if($LASTEXITCODE -ne 0){throw "texconv failed with exit code $LASTEXITCODE"}
Write-Output 'Built five looping shots, five liquid impact strips, DDS and 720p team-colour lineup.'
