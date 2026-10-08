[CmdletBinding()]
param(
    [string] $SourcePath = '',
    [string] $OutputDirectory = '',
    [switch] $StageRuntime
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Add-Type -AssemblyName System.Drawing
if (-not $SourcePath) { $SourcePath = Join-Path $PSScriptRoot '../game/art/polish/kit/source.json' }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $PSScriptRoot '../game/art/polish/kit' }

function Assert-Object {
    param($Value, [string] $Label)
    if ($null -eq $Value -or $Value -isnot [pscustomobject]) {
        throw "$Label must be a JSON object."
    }
}

function Read-Point {
    param($Value, [string] $Label, [int] $Width, [int] $Height)
    if ($Value -isnot [array] -or $Value.Count -ne 2) {
        throw "$Label must be a two-number array."
    }
    for ($axis = 0; $axis -lt 2; ++$axis) {
        $number = $Value[$axis]
        if (($number -isnot [int] -and $number -isnot [long] -and
             $number -isnot [double] -and $number -isnot [decimal]) -or
            [double]::IsNaN([double]$number) -or [double]::IsInfinity([double]$number)) {
            throw "$Label must contain finite numbers."
        }
        $limit = if ($axis -eq 0) { $Width } else { $Height }
        if ($number -lt 0 -or $number -gt $limit) {
            throw "$Label must lie within the native sprite bounds (including edges)."
        }
    }
    return [ordered]@{ x = $Value[0]; y = $Value[1] }
}

# Validate the complete document and packing before creating any output files.
$sourceFile = [IO.Path]::GetFullPath($SourcePath)
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
if ($StageRuntime -and $sourceFile -ne [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../game/art/polish/kit/source.json'))) {
    throw 'Only the canonical workshop source may be staged into runtime content.'
}
$source = [IO.File]::ReadAllText($sourceFile) | ConvertFrom-Json
Assert-Object $source 'Source'
if ($source.schema_version -isnot [int] -or $source.schema_version -ne 1) {
    throw 'schema_version must be the integer 1.'
}
if (($source.world_units_per_texel -isnot [int] -and
     $source.world_units_per_texel -isnot [double]) -or $source.world_units_per_texel -ne 4) {
    throw 'This candidate uses exactly 4 world units per native texel.'
}
Assert-Object $source.palette 'palette'
$palette = [Collections.Generic.Dictionary[string, Drawing.Color]]::new([StringComparer]::Ordinal)
$paletteLayout = [Collections.Generic.List[object]]::new()
foreach ($entry in $source.palette.PSObject.Properties) {
    if ($entry.Name.Length -ne 1 -or [int][char]$entry.Name -lt 33 -or
        [int][char]$entry.Name -gt 126) { throw 'Palette symbols must be single printable ASCII characters.' }
    if ($entry.Value -isnot [string] -or $entry.Value -cnotmatch '^#?(?:[0-9a-fA-F]{6}|[0-9a-fA-F]{8})$') {
        throw "Palette symbol '$($entry.Name)' needs RRGGBB or RRGGBBAA, with an optional #."
    }
    $hex = $entry.Value.TrimStart('#').ToUpperInvariant()
    if ($hex.Length -eq 6) { $hex += 'FF' }
    $r = [Convert]::ToInt32($hex.Substring(0, 2), 16)
    $g = [Convert]::ToInt32($hex.Substring(2, 2), 16)
    $b = [Convert]::ToInt32($hex.Substring(4, 2), 16)
    $a = [Convert]::ToInt32($hex.Substring(6, 2), 16)
    $palette.Add($entry.Name, [Drawing.Color]::FromArgb($a, $r, $g, $b))
    $paletteLayout.Add([ordered]@{ symbol = $entry.Name; rgba = $hex })
}
if (-not $palette.ContainsKey('.') -or $palette['.'].ToArgb() -ne 0) {
    throw "The '.' palette symbol must be transparent black (00000000)."
}
if ($source.sprites -isnot [array] -or $source.sprites.Count -eq 0) {
    throw 'sprites must be a nonempty array.'
}
$names = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
$sprites = [Collections.Generic.List[object]]::new()
$frames = [Collections.Generic.List[object]]::new()
$atlasSize = 256
$padding = 2
$cursorX = $padding
$cursorY = $padding
$shelfHeight = 0
foreach ($sprite in $source.sprites) {
    Assert-Object $sprite 'Sprite'
    if ($sprite.name -isnot [string] -or $sprite.name -cnotmatch '^[a-z][a-z0-9_]*$') {
        throw 'Sprite names must be lower-case snake_case identifiers beginning with a letter.'
    }
    if ($sprite.rows -isnot [array] -or $sprite.rows.Count -eq 0 -or
        $sprite.rows[0] -isnot [string] -or $sprite.rows[0].Length -eq 0) {
        throw "$($sprite.name): rows must be a nonempty array of nonempty strings."
    }
    $width = $sprite.rows[0].Length
    $height = $sprite.rows.Count
    if ($width + 2 * $padding -gt $atlasSize -or $height + 2 * $padding -gt $atlasSize) {
        throw "$($sprite.name): sprite is too large for the 256-texel atlas."
    }
    foreach ($row in $sprite.rows) {
        if ($row -isnot [string] -or $row.Length -ne $width) {
            throw "$($sprite.name): every row must have exactly $width symbols."
        }
        foreach ($symbol in $row.ToCharArray()) {
            if (-not $palette.ContainsKey([string]$symbol)) {
                throw "$($sprite.name): undefined palette symbol '$symbol'."
            }
        }
    }
    $pivot = Read-Point $sprite.pivot "$($sprite.name) pivot" $width $height
    $enginePivot = $false
    if ($sprite.PSObject.Properties['engine_pivot']) {
        if ($sprite.engine_pivot -isnot [bool]) { throw "$($sprite.name): engine_pivot must be a boolean." }
        $enginePivot = $sprite.engine_pivot
    }
    $anchors = [ordered]@{}
    $worldAnchors = [ordered]@{}
    if ($sprite.PSObject.Properties['anchors']) {
        Assert-Object $sprite.anchors "$($sprite.name) anchors"
        foreach ($entry in $sprite.anchors.PSObject.Properties) {
            if ($entry.Name -cnotmatch '^[a-z][a-z0-9_]*$') { throw 'Anchor names must be snake_case identifiers.' }
            $point = Read-Point $entry.Value "$($sprite.name) anchor $($entry.Name)" $width $height
            $anchors[$entry.Name] = $point
            $worldAnchors[$entry.Name] = [ordered]@{ x = $point.x * 4; y = $point.y * 4 }
        }
    }
    $teamSymbols = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    if ($sprite.PSObject.Properties['team_symbols']) {
        if ($sprite.team_symbols -isnot [array]) { throw "$($sprite.name): team_symbols must be an array." }
        foreach ($symbol in $sprite.team_symbols) {
            if ($symbol -isnot [string] -or -not $palette.ContainsKey($symbol) -or
                $symbol -eq '.' -or -not $teamSymbols.Add($symbol)) {
                throw "$($sprite.name): team_symbols must contain unique, defined, nontransparent symbols."
            }
            $colour = $palette[$symbol]
            if ($colour.A -eq 0 -or $colour.R -ne $colour.G -or $colour.G -ne $colour.B) {
                throw "$($sprite.name): team symbol '$symbol' must be visible grayscale for tinting."
            }
        }
    }
    $layerFrames = [ordered]@{ base = $null; team = $null }
    $layers = @('base')
    if ($teamSymbols.Count -gt 0) { $layers += 'team' }
    foreach ($layer in $layers) {
        $frameName = if ($layer -eq 'base') { $sprite.name } else { $sprite.name + '_team' }
        if (-not $names.Add($frameName)) { throw "Duplicate sprite/frame identifier '$frameName'." }
        if ($cursorX + $width + $padding -gt $atlasSize) {
            $cursorX = $padding
            $cursorY += $shelfHeight + 2 * $padding
            $shelfHeight = 0
        }
        if ($cursorY + $height + $padding -gt $atlasSize) {
            throw 'The kit exceeds its 256x256 atlas; revise packing or the explicitly declared atlas size.'
        }
        $engineFrame = [ordered]@{
            name = $frameName
            position = [ordered]@{ x = $cursorX; y = $cursorY }
            size = [ordered]@{ w = $width; h = $height }
        }
        # Earlier study frames pass their origin explicitly. New runtime frames
        # opt into the authored native pivot, including their matching team mask.
        if ($enginePivot) { $engineFrame['origin'] = $pivot }
        $frames.Add($engineFrame)
        $layerFrames[$layer] = [ordered]@{ name = $frameName; x = $cursorX; y = $cursorY; w = $width; h = $height }
        $cursorX += $width + 2 * $padding
        $shelfHeight = [Math]::Max($shelfHeight, $height)
    }
    $sprites.Add([ordered]@{
        name = $sprite.name
        native_size = [ordered]@{ w = $width; h = $height }
        world_size = [ordered]@{ w = $width * 4; h = $height * 4 }
        pivot = $pivot
        world_pivot = [ordered]@{ x = $pivot.x * 4; y = $pivot.y * 4 }
        engine_pivot = $enginePivot
        anchors = $anchors
        world_anchors = $worldAnchors
        frames = $layerFrames
        rows = $sprite.rows
        team_symbols = $teamSymbols
    })
}

if (-not ('ColourWars.WorkshopKitPng' -as [type])) {
    Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Text;
namespace ColourWars {
    public static class WorkshopKitPng {
        public static void Save(Bitmap atlas, string path) {
            // Preserve authored UNORM bytes through texconv, matching the game
            // blend contract. System.Drawing otherwise supplies an sRGB tag.
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
    }
}
'@
}
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$converter = Join-Path $repo 'game/content/textures/texconv.exe'
if (-not (Test-Path -LiteralPath $converter -PathType Leaf)) { throw "Missing texconv: $converter" }
$null = [IO.Directory]::CreateDirectory($outputRoot)
$stage = Join-Path $outputRoot ('.workshop-stage-' + [Guid]::NewGuid().ToString('N'))
$null = [IO.Directory]::CreateDirectory($stage)
$atlas = [Drawing.Bitmap]::new($atlasSize, $atlasSize, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
try {
    foreach ($sprite in $sprites) {
        for ($y = 0; $y -lt $sprite.native_size.h; ++$y) {
            for ($x = 0; $x -lt $sprite.native_size.w; ++$x) {
                $symbol = [string]$sprite.rows[$y][$x]
                $layer = if ($sprite.team_symbols.Contains($symbol)) { $sprite.frames.team } else { $sprite.frames.base }
                $atlas.SetPixel($layer.x + $x, $layer.y + $y, $palette[$symbol])
            }
        }
    }
    [ColourWars.WorkshopKitPng]::Save($atlas, (Join-Path $stage 'workshop_kit.png'))
    $definition = [ordered]@{ animation_strips = @(); sprite_frames = $frames.ToArray() }
    $layout = [ordered]@{
        schema_version = 1
        source_sha256 = (Get-FileHash -LiteralPath $sourceFile -Algorithm SHA256).Hash.ToLowerInvariant()
        world_units_per_texel = 4
        atlas = [ordered]@{ width = $atlasSize; height = $atlasSize; padding = $padding }
        palette = $paletteLayout.ToArray()
        sprites = @($sprites | ForEach-Object {
            # Authoring rows and lookup sets belong in the source, not metadata.
            $entry = [ordered]@{}
            foreach ($key in $_.Keys) {
                if ($key -ne 'rows' -and $key -ne 'team_symbols') { $entry[$key] = $_[$key] }
            }
            $entry
        })
    }
    # ConvertTo-Json uses CRLF on Windows PowerShell. Keep retained JSON at LF,
    # matching the kit's Git attributes so a fresh checkout has identical bytes.
    $definitionJson = ($definition | ConvertTo-Json -Depth 12).Replace("`r`n", "`n") + "`n"
    $layoutJson = ($layout | ConvertTo-Json -Depth 12).Replace("`r`n", "`n") + "`n"
    [IO.File]::WriteAllText((Join-Path $stage 'workshop_kit.json'),
        $definitionJson, [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText((Join-Path $stage 'workshop_kit.layout.json'),
        $layoutJson, [Text.UTF8Encoding]::new($false))
    & $converter (Join-Path $stage 'workshop_kit.png') -pmalpha -m 1 -f R8G8B8A8_UNORM -y -o $stage
    if ($LASTEXITCODE -ne 0) { throw "texconv failed with exit code $LASTEXITCODE" }
    # A parse, packing or conversion failure leaves all previous artifacts intact.
    foreach ($name in 'workshop_kit.png', 'workshop_kit.dds', 'workshop_kit.json', 'workshop_kit.layout.json') {
        Copy-Item -LiteralPath (Join-Path $stage $name) -Destination (Join-Path $outputRoot $name) -Force
    }
    if ($StageRuntime) {
        $runtimeRoot = Join-Path $repo 'game/content/textures'
        foreach ($name in 'workshop_kit.dds', 'workshop_kit.json') {
            Copy-Item -LiteralPath (Join-Path $stage $name) -Destination (Join-Path $runtimeRoot $name) -Force
        }
    }
}
finally {
    $atlas.Dispose()
    # Only this invocation's known files can be removed. Delete the empty staging
    # directory non-recursively, so an unexpected file is never swept away.
    foreach ($name in 'workshop_kit.png', 'workshop_kit.dds', 'workshop_kit.json', 'workshop_kit.layout.json') {
        $path = Join-Path $stage $name
        if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Force }
    }
    [IO.Directory]::Delete($stage, $false)
}
Write-Output "Built grid-exact workshop kit: $($sprites.Count) sprites, $($frames.Count) frames in $outputRoot"
