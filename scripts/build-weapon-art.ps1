[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Add-Type -AssemblyName System.Drawing

# Derive material masks from the authored accent colours without regenerating
# or moving the artwork. Runtime multiplies these grayscale texels by a team
# colour. Material regions keep similarly coloured hardware out of the masks.
if (-not ('ColourWars.WeaponPaintLayers' -as [type])) {
    Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Text;
namespace ColourWars {
    public static class WeaponPaintLayers {
        public static void SaveAtlasPng(Bitmap atlas, string path) {
            // System.Drawing tags PNGs as sRGB. The bundled texconv applies
            // that transfer function before premultiplying, but Labrador uses
            // UNORM channel values throughout. Drop colour-space metadata from
            // this runtime atlas so conversion preserves its authored bytes.
            using (var encoded = new MemoryStream()) {
                atlas.Save(encoded, ImageFormat.Png);
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

        public static void Split(Bitmap atlas, string name, Rectangle box, int maskOffset) {
            for (int y = 0; y < box.Height; ++y) {
                for (int x = 0; x < box.Width; ++x) {
                    Color pixel = atlas.GetPixel(box.X + x, box.Y + y);
                    if (pixel.A == 0) continue;
                    int maximum = Math.Max(pixel.R, Math.Max(pixel.G, pixel.B));
                    int minimum = Math.Min(pixel.R, Math.Min(pixel.G, pixel.B));
                    float hue = pixel.GetHue();
                    double amount = 0;
                    if (name == "sprayer") {
                        // Reservoir only. The mint nozzle and pressure gauge
                        // are hardware accents and must keep their own colours.
                        bool reservoir = x >= 52 && x <= 222 && y <= 54;
                        if (reservoir && hue >= 115 && hue <= 185)
                            amount = Math.Min(1.0, (maximum - minimum) / 18.0);
                    } else if (name == "bucket") {
                        if (hue >= 115 && hue <= 185)
                            amount = Math.Min(1.0, (maximum - minimum) / 18.0);
                    } else if (name == "sniper") {
                        // The coral trigger, sight lens and muzzle are not
                        // paint. Only the upper reservoir changes with teams.
                        bool reservoir = x >= 95 && x <= 183 && y <= 58;
                        if (reservoir && (hue <= 26 || hue >= 350))
                            amount = Math.Min(1.0, (maximum - minimum) / 18.0);
                    } else if (name == "mister") {
                        // The canister begins below the shroud and its mount.
                        // Keep the amber pressure gauge and ivory body intact.
                        bool canister = x >= 80 && x <= 158 && y >= 80;
                        if (canister && hue >= 20 && hue <= 65)
                            amount = Math.Min(1.0, (maximum - minimum) / 30.0);
                    } else if (name == "roller") {
                        // Only the pale sleeve; not the bent arm, end caps or
                        // handle. Preserve their original material colours.
                        if (x >= 250 && x <= 345 && pixel.R >= pixel.B && maximum > 80)
                            amount = Math.Min(1.0, (maximum - 80) / 35.0);
                    }
                    if (amount <= 0) continue;
                    int maskAlpha = (int)Math.Round(pixel.A * amount);
                    if (maskAlpha == 0) continue;
                    double alpha = pixel.A / 255.0;
                    double paintAlpha = maskAlpha / 255.0;
                    // The two layers recombine to exactly the original alpha:
                    // paint + body * (1 - paint). Avoid thickened sprite edges.
                    int bodyAlpha = paintAlpha >= 1.0 ? 0 :
                        (int)Math.Round(255.0 * (alpha - paintAlpha) / (1.0 - paintAlpha));
                    bodyAlpha = Math.Max(0, Math.Min(255, bodyAlpha));
                    atlas.SetPixel(box.X + x, box.Y + y,
                        Color.FromArgb(bodyAlpha, maximum, maximum, maximum));
                    atlas.SetPixel(box.X + x, box.Y + y + maskOffset,
                        Color.FromArgb(maskAlpha, maximum, maximum, maximum));
                }
            }
        }
    }
}
'@
}

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$textures = Join-Path $repo 'game/content/textures'
$working = Join-Path $repo 'game/art/sources/textures'
$source = [Drawing.Bitmap]::FromFile((Join-Path $repo 'game/art/weapon_paint_source.png'))
$legacy = [Drawing.Bitmap]::FromFile((Join-Path $working 'sprite_sheet_1.png'))
$atlas = [Drawing.Bitmap]::new(1024, 1024, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
$graphics = [Drawing.Graphics]::FromImage($atlas)
$frames = [Collections.Generic.List[object]]::new()

function Add-Frame {
    param([string] $Name, [int[]] $Box)
    $frames.Add([ordered]@{
        name = $Name
        position = [ordered]@{ x = $Box[0]; y = $Box[1] }
        size = [ordered]@{ w = $Box[2]; h = $Box[3] }
    })
}

function Copy-Sprite {
    param([Drawing.Image] $Image, [int[]] $SourceBox, [int[]] $DestinationBox)
    $destination = [Drawing.Rectangle]::new($DestinationBox[0], $DestinationBox[1],
        $DestinationBox[2], $DestinationBox[3])
    $graphics.DrawImage($Image, $destination, $SourceBox[0], $SourceBox[1],
        $SourceBox[2], $SourceBox[3], [Drawing.GraphicsUnit]::Pixel)
}

try {
    $graphics.Clear([Drawing.Color]::Transparent)
    $graphics.CompositingMode = [Drawing.Drawing2D.CompositingMode]::SourceCopy
    $graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality

    # Four source texels per world unit. Empty gutters isolate bilinear samples.
    # The crop canvases centre each muzzle on the left-centre aim pivot. The
    # roller drum centre stays at x=75 world units, matching its paint contact.
    Copy-Sprite $source @(291, 20, 438, 272) @(8, 8, 280, 120)
    Add-Frame 'sprayer' @(8, 8, 280, 120)
    Copy-Sprite $source @(71, 299, 918, 286) @(304, 8, 400, 120)
    Add-Frame 'sniper' @(304, 8, 400, 120)
    Copy-Sprite $source @(252, 580, 554, 325) @(8, 144, 400, 200)
    Add-Frame 'roller' @(8, 144, 400, 200)
    # The mister's spray head sits above its canister; reserve space above it
    # in the frame so the barrel still meets the unchanged projectile origin.
    Copy-Sprite $source @(293, 920, 406, 242) @(424, 175, 240, 89)
    Add-Frame 'mister' @(424, 144, 240, 120)
    Copy-Sprite $source @(356, 1181, 310, 331) @(720, 8, 240, 240)
    Add-Frame 'bucket' @(720, 8, 240, 240)

    # Existing utility frames let Weapon and the random-choice menu stay on
    # their own sheet. Do not alter the player/projectile atlas.
    $graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
    Copy-Sprite $legacy @(0, 350, 64, 64) @(424, 288, 64, 64)
    Add-Frame 'random' @(424, 288, 64, 64)
    Copy-Sprite $legacy @(36, 0, 32, 32) @(504, 288, 32, 32)
    Add-Frame 'error' @(504, 288, 32, 32)
    Copy-Sprite $legacy @(32, 0, 4, 4) @(552, 288, 4, 4)
    Add-Frame 'pixel' @(552, 288, 4, 4)
    Copy-Sprite $legacy @(100, 150, 4, 4) @(568, 288, 4, 4)
    Add-Frame 'nozzle' @(568, 288, 4, 4)

    # All painting is complete before accessing the bitmap's pixels directly.
    $graphics.Flush()
    foreach ($frame in @($frames.ToArray() | Select-Object -First 5)) {
        $box = [Drawing.Rectangle]::new($frame.position.x, $frame.position.y,
            $frame.size.w, $frame.size.h)
        [ColourWars.WeaponPaintLayers]::Split($atlas, $frame.name, $box, 512)
        Add-Frame ($frame.name + '_paint') @($box.X, ($box.Y + 512), $box.Width, $box.Height)
    }

    [ColourWars.WeaponPaintLayers]::SaveAtlasPng($atlas, (Join-Path $working 'weapon_paint.png'))
    $definition = [ordered]@{ animation_strips = @(); sprite_frames = $frames.ToArray() }
    $json = ($definition | ConvertTo-Json -Depth 6) + [Environment]::NewLine
    [IO.File]::WriteAllText((Join-Path $textures 'weapon_paint.json'), $json,
        [Text.UTF8Encoding]::new($false))
}
finally {
    $graphics.Dispose()
    $atlas.Dispose()
    $legacy.Dispose()
    $source.Dispose()
}

# Labrador blends premultiplied texels. Keep the small atlas uncompressed to
# preserve the fine alpha edges and colour blocks at native gameplay size.
& (Join-Path $repo 'game/art/tools/texconv.exe') (Join-Path $working 'weapon_paint.png') `
    -pmalpha -m 1 -f R8G8B8A8_UNORM -y -o $textures
if ($LASTEXITCODE -ne 0) { throw "texconv failed with exit code $LASTEXITCODE" }
Write-Output 'Built weapon_paint.png, .json and premultiplied .dds.'
