[CmdletBinding()]
param(
    # Write each regenerated atlas over the tracked one. Without it nothing
    # under game/content/ is touched.
    [switch] $Update,
    # Regenerate only these recipe entries (file names), for example
    # -Only courier_prime_bold_16.spritefont.
    [string[]] $Only = @(),
    # The most BC2 blocks (4x4 texels, 16 bytes each) a regenerated texture
    # may differ in and still be 'equivalent'. The noise seen so far is two
    # blocks; one redrawn glyph at the smallest size is about a dozen.
    [int] $MaxTextureBlocks = 8
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Add-Type -AssemblyName System.Drawing

# Regenerates the runtime sprite fonts from game/art/sources/fonts/fonts.json
# with the tracked MakeSpriteFont, and compares each result with the atlas in
# game/content/fonts/. The faces are the OFL files beside the recipe, but
# MakeSpriteFont reads only installed fonts, so any face that is missing is
# installed for the current user (no elevation) before the run. Neither builds
# nor CI run this; builds check only the atlas headers
# (cmake/check_sprite_fonts.cmake).
#
# Each atlas reports one of:
#   identical   - byte for byte the tracked file;
#   equivalent  - the same glyph table, region, texture size and format, with
#                 a line spacing within 1e-5 and at most -MaxTextureBlocks BC2
#                 blocks of texture different, which is font-version or
#                 rasteriser noise (see the README);
#   DIFFERENT   - anything structural differs, or more texture blocks than
#                 that. The run fails.
$repo = Split-Path -Parent $PSScriptRoot
$recipePath = Join-Path $repo 'game/art/sources/fonts/fonts.json'
$recipe = Get-Content -LiteralPath $recipePath -Raw | ConvertFrom-Json
$tool = Join-Path $repo $recipe.tool
$contentFonts = Join-Path $repo 'game/content/fonts'
if (-not (Test-Path -LiteralPath $tool -PathType Leaf)) { throw "MakeSpriteFont is missing: $tool" }

function Get-InstalledFaces {
    return @((New-Object System.Drawing.Text.InstalledFontCollection).Families |
        ForEach-Object { $_.Name })
}

# A per-user install is a copy into %LOCALAPPDATA%\Microsoft\Windows\Fonts and a
# value under HKCU's Fonts key, which GDI+ reads like a machine-wide font.
$installed = Get-InstalledFaces
$faceDir = Split-Path -Parent $recipePath
$userFonts = Join-Path $env:LOCALAPPDATA 'Microsoft\Windows\Fonts'
$userFontKey = 'HKCU:\Software\Microsoft\Windows NT\CurrentVersion\Fonts'
foreach ($font in $recipe.fonts) {
    if ($installed -contains $font.face) { continue }
    $source = Join-Path $faceDir $font.face_file
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { throw "Face file is missing: $source" }
    [System.IO.Directory]::CreateDirectory($userFonts) | Out-Null
    $target = Join-Path $userFonts $font.face_file
    Copy-Item -LiteralPath $source -Destination $target -Force
    New-ItemProperty -Path $userFontKey -Name "$($font.face_file) (TrueType)" -Value $target `
        -PropertyType String -Force | Out-Null
    Write-Host "installed $($font.face_file) for the current user"
    $installed = Get-InstalledFaces
}
$region = '0x{0:X2}-0x{1:X2}' -f [int]$recipe.character_region.first, [int]$recipe.character_region.last

function Read-SpriteFont([string] $Path) {
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 40 -or [System.Text.Encoding]::ASCII.GetString($bytes, 0, 8) -ne 'DXTKfont') {
        throw "$Path is not a DXTKfont atlas"
    }
    $count = [BitConverter]::ToUInt32($bytes, 8)
    $tableEnd = 12 + 32 * $count
    return [pscustomobject]@{
        Bytes = $bytes
        Glyphs = $count
        Table = [byte[]]$bytes[12..($tableEnd - 1)]
        LineSpacing = [BitConverter]::ToSingle($bytes, $tableEnd)
        Layout = [byte[]]$bytes[($tableEnd + 4)..($tableEnd + 27)]
        Format = [BitConverter]::ToUInt32($bytes, $tableEnd + 16)
        Texture = $tableEnd + 28
    }
}

function Test-SameBytes([byte[]] $First, [byte[]] $Second) {
    return [System.Linq.Enumerable]::SequenceEqual($First, $Second)
}

$work = Join-Path ([System.IO.Path]::GetTempPath()) ('colourwars-fonts-' + [guid]::NewGuid().ToString('N'))
[System.IO.Directory]::CreateDirectory($work) | Out-Null
$failed = @()
try {
    foreach ($font in $recipe.fonts) {
        if ($Only.Count -gt 0 -and $Only -notcontains $font.file) { continue }
        if ($installed -notcontains $font.face) {
            throw "Face '$($font.face)' ($($font.face_file)) is not installed, so $($font.file) cannot be regenerated: installing it for the current user did not register the family name. GDI+ would otherwise substitute a different face silently."
        }
        $output = Join-Path $work $font.file
        $arguments = @($font.face, $output, "/FontSize:$($font.size)", "/FontStyle:$($font.style)",
            "/CharacterRegion:$region", "/TextureFormat:$($recipe.texture_format)")
        if ($font.PSObject.Properties['line_spacing_adjust']) {
            $arguments += "/LineSpacing:$($font.line_spacing_adjust)"
        }
        $log = & $tool @arguments
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $output -PathType Leaf)) {
            throw "MakeSpriteFont failed for $($font.file):`n$($log -join "`n")"
        }

        $built = Read-SpriteFont $output
        $trackedPath = Join-Path $contentFonts $font.file
        $notes = @()
        $status = 'identical'
        if (-not (Test-Path -LiteralPath $trackedPath -PathType Leaf)) {
            $status = 'DIFFERENT'; $notes += 'no tracked atlas'
        } else {
            $tracked = Read-SpriteFont $trackedPath
            if (-not (Test-SameBytes $built.Bytes $tracked.Bytes)) {
                $status = 'equivalent'
                if ($built.Glyphs -ne $tracked.Glyphs -or -not (Test-SameBytes $built.Table $tracked.Table)) {
                    $status = 'DIFFERENT'; $notes += 'glyph table differs'
                }
                if (-not (Test-SameBytes $built.Layout $tracked.Layout)) {
                    $status = 'DIFFERENT'; $notes += 'default character, texture size or format differs'
                }
                $spacing = [math]::Abs([double]$built.LineSpacing - [double]$tracked.LineSpacing)
                if ($spacing -gt 1e-5 * [math]::Abs([double]$tracked.LineSpacing)) {
                    $status = 'DIFFERENT'; $notes += ('line spacing {0} vs tracked {1}' -f $built.LineSpacing, $tracked.LineSpacing)
                } elseif ($spacing -ne 0) {
                    $notes += ('line spacing {0:R} vs tracked {1:R}' -f $built.LineSpacing, $tracked.LineSpacing)
                }
                if ($status -ne 'DIFFERENT' -and $built.Bytes.Length -ne $tracked.Bytes.Length) {
                    $status = 'DIFFERENT'; $notes += 'file size differs'
                }
                if ($status -ne 'DIFFERENT' -and $built.Format -ne 74) {
                    # Blocks are counted as BC2 (DXGI_FORMAT_BC2_UNORM), the
                    # recipe's format; anything else cannot be judged as noise.
                    $status = 'DIFFERENT'; $notes += "texture format DXGI $($built.Format) is not BC2"
                }
                if ($status -ne 'DIFFERENT') {
                    $differing = 0
                    $blocks = @{}
                    for ($i = $built.Texture; $i -lt $built.Bytes.Length; ++$i) {
                        if ($built.Bytes[$i] -ne $tracked.Bytes[$i]) {
                            ++$differing
                            $blocks[[math]::Floor(($i - $built.Texture) / 16)] = $true
                        }
                    }
                    if ($differing -gt 0) {
                        $notes += ('{0} of {1} texture bytes differ, in {2} BC2 blocks' -f
                            $differing, ($built.Bytes.Length - $built.Texture), $blocks.Count)
                    }
                    if ($blocks.Count -gt $MaxTextureBlocks) {
                        $status = 'DIFFERENT'; $notes += "more than $MaxTextureBlocks texture blocks differ"
                    }
                }
            }
        }
        if ([double]$built.LineSpacing -ne 0 -and
            [math]::Abs([double]$built.LineSpacing - [double]$font.line_spacing) -gt 1e-5 * [double]$font.line_spacing) {
            $status = 'DIFFERENT'; $notes += "line spacing $($built.LineSpacing) is not the recipe's $($font.line_spacing)"
        }
        $line = '{0,-36} {1}' -f $font.file, $status
        if ($notes.Count -gt 0) { $line += ' (' + ($notes -join '; ') + ')' }
        Write-Host $line
        if ($status -eq 'DIFFERENT') { $failed += $font.file }
        if ($Update -and $status -ne 'identical') {
            Copy-Item -LiteralPath $output -Destination $trackedPath -Force
            Write-Host "  updated $trackedPath"
        }
    }
}
finally {
    Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue
}
if ($failed.Count -gt 0 -and -not $Update) {
    throw "Regenerated atlases differ from the tracked ones beyond noise: $($failed -join ', '). Check the recipe and installed face versions; -Update writes them over if the change is intended."
}
