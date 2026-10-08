[CmdletBinding()]
param(
    [string] $SourcePath = '',
    [string] $OutputDirectory = '',
    # Defaults to the checked-in kit for its canonical source. Custom sources
    # only compare against retained output when a reference is supplied.
    [string] $ReferenceDirectory = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Add-Type -AssemblyName System.Drawing
$checkRuntime = -not $SourcePath -and -not $ReferenceDirectory
if (-not $SourcePath) { $SourcePath = Join-Path $PSScriptRoot '../game/art/polish/kit/source.json' }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $PSScriptRoot '../out/validation/workshop-kit' }

function Assert-True {
    param([bool] $Condition, [string] $Message)
    if (-not $Condition) { throw $Message }
}

if (-not ('ColourWars.WorkshopKitCheck' -as [type])) {
    Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.IO;
using System.Text;
namespace ColourWars {
    public static class WorkshopKitCheck {
        private static uint U32(byte[] data, int offset) { return BitConverter.ToUInt32(data, offset); }
        public static void Verify(string pngPath, string ddsPath, byte[] expected, int width, int height) {
            using (var png = new Bitmap(pngPath)) {
                if (png.Width != width || png.Height != height) throw new Exception("Wrong PNG dimensions.");
                for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
                    int i = (y * width + x) * 4;
                    Color c = png.GetPixel(x, y);
                    if (c.R != expected[i] || c.G != expected[i + 1] ||
                        c.B != expected[i + 2] || c.A != expected[i + 3])
                        throw new Exception("PNG texel differs from source/layer/gutter contract at " + x + "," + y);
                }
            }
            byte[] dds = File.ReadAllBytes(ddsPath);
            if (dds.Length < 128 || Encoding.ASCII.GetString(dds, 0, 4) != "DDS " ||
                U32(dds, 4) != 124 || U32(dds, 12) != height || U32(dds, 16) != width ||
                U32(dds, 20) != width * 4 || U32(dds, 28) != 1 || U32(dds, 76) != 32)
                throw new Exception("DDS header does not declare the required dimensions, pitch and single mip.");
            int start = 128;
            string fourCc = Encoding.ASCII.GetString(dds, 84, 4);
            if (fourCc == "DX10") {
                if (dds.Length < 148 || U32(dds, 128) != 28 || U32(dds, 132) != 3 ||
                    U32(dds, 140) != 1) throw new Exception("DDS must be one R8G8B8A8_UNORM 2D texture.");
                start = 148;
            } else if (U32(dds, 80) != 65 || U32(dds, 88) != 32 ||
                U32(dds, 92) != 0xff || U32(dds, 96) != 0xff00 ||
                U32(dds, 100) != 0xff0000 || U32(dds, 104) != 0xff000000) {
                throw new Exception("DDS must contain uncompressed 8-bit RGBA channels.");
            }
            if (dds.Length != start + expected.Length) throw new Exception("Unexpected DDS mip, array or payload length.");
            for (int i = 0; i < expected.Length; i += 4) {
                int alpha = expected[i + 3];
                if (dds[start + i + 3] != alpha) throw new Exception("DDS changed source alpha.");
                for (int c = 0; c < 3; ++c) {
                    int premultiplied = (expected[i + c] * alpha + 127) / 255;
                    if (Math.Abs(dds[start + i + c] - premultiplied) > 1)
                        throw new Exception("DDS is not authored UNORM colour premultiplied by alpha at texel " + i / 4);
                }
            }
        }
    }
}
'@
}

function Test-Atlas {
    param([string] $Source, [string] $Directory)
    $inputData = Get-Content -LiteralPath $Source -Raw | ConvertFrom-Json
    $layout = Get-Content -LiteralPath (Join-Path $Directory 'workshop_kit.layout.json') -Raw | ConvertFrom-Json
    $definition = Get-Content -LiteralPath (Join-Path $Directory 'workshop_kit.json') -Raw | ConvertFrom-Json
    Assert-True ($layout.source_sha256 -eq (Get-FileHash -LiteralPath $Source -Algorithm SHA256).Hash.ToLowerInvariant()) 'Layout source SHA256 is stale.'
    Assert-True ($layout.schema_version -eq 1 -and $layout.world_units_per_texel -eq 4) 'Wrong layout schema or world scale.'
    Assert-True ($layout.atlas.width -eq 256 -and $layout.atlas.height -eq 256 -and $layout.atlas.padding -ge 2) 'Wrong atlas size or insufficient padding.'
    Assert-True ($layout.sprites.Count -eq $inputData.sprites.Count) 'Layout dropped or added sprites.'
    Assert-True ($definition.animation_strips.Count -eq 0) 'Frame-selected kit unexpectedly declares engine animation strips.'
    $colours = [Collections.Generic.Dictionary[string, byte[]]]::new([StringComparer]::Ordinal)
    $paletteIndex = 0
    foreach ($entry in $inputData.palette.PSObject.Properties) {
        $rgba = $entry.Value.TrimStart('#').ToUpperInvariant()
        if ($rgba.Length -eq 6) { $rgba += 'FF' }
        Assert-True ($layout.palette[$paletteIndex].symbol -ceq $entry.Name -and $layout.palette[$paletteIndex].rgba -ceq $rgba) 'Layout changed palette values or authored order.'
        ++$paletteIndex
        $colours.Add($entry.Name, [byte[]]@(
            [Convert]::ToByte($rgba.Substring(0, 2), 16),
            [Convert]::ToByte($rgba.Substring(2, 2), 16),
            [Convert]::ToByte($rgba.Substring(4, 2), 16),
            [Convert]::ToByte($rgba.Substring(6, 2), 16)))
    }
    Assert-True ($layout.palette.Count -eq $paletteIndex) 'Layout added palette entries.'
    $expected = [byte[]]::new(256 * 256 * 4)
    $rectangles = [Collections.Generic.List[object]]::new()
    $frameIndex = 0
    for ($spriteIndex = 0; $spriteIndex -lt $inputData.sprites.Count; ++$spriteIndex) {
        $sprite = $inputData.sprites[$spriteIndex]
        $metadata = $layout.sprites[$spriteIndex]
        $width = $sprite.rows[0].Length
        $height = $sprite.rows.Count
        Assert-True ($metadata.name -ceq $sprite.name -and $metadata.native_size.w -eq $width -and $metadata.native_size.h -eq $height -and $metadata.world_size.w -eq $width * 4 -and $metadata.world_size.h -eq $height * 4) 'Layout changed sprite identity or dimensions.'
        Assert-True ($metadata.pivot.x -eq $sprite.pivot[0] -and $metadata.pivot.y -eq $sprite.pivot[1] -and $metadata.world_pivot.x -eq $sprite.pivot[0] * 4 -and $metadata.world_pivot.y -eq $sprite.pivot[1] * 4) 'Layout changed a pivot.'
        $enginePivot = $sprite.PSObject.Properties['engine_pivot'] -and $sprite.engine_pivot
        Assert-True ($metadata.engine_pivot -eq $enginePivot) 'Layout changed the engine pivot policy.'
        $anchorCount = 0
        if ($sprite.PSObject.Properties['anchors']) {
            foreach ($anchor in $sprite.anchors.PSObject.Properties) {
                $native = $metadata.anchors.PSObject.Properties[$anchor.Name].Value
                $world = $metadata.world_anchors.PSObject.Properties[$anchor.Name].Value
                Assert-True ($native.x -eq $anchor.Value[0] -and $native.y -eq $anchor.Value[1] -and $world.x -eq $native.x * 4 -and $world.y -eq $native.y * 4) 'Layout changed an attachment anchor.'
                ++$anchorCount
            }
        }
        Assert-True (@($metadata.anchors.PSObject.Properties).Count -eq $anchorCount -and @($metadata.world_anchors.PSObject.Properties).Count -eq $anchorCount) 'Layout added or dropped anchors.'
        $team = @()
        if ($sprite.PSObject.Properties['team_symbols']) { $team = @($sprite.team_symbols) }
        $layers = @('base')
        if ($team.Count -gt 0) { $layers += 'team' }
        else { Assert-True ($null -eq $metadata.frames.team) 'Unexpected team layer.' }
        foreach ($layerName in $layers) {
            $frame = $metadata.frames.PSObject.Properties[$layerName].Value
            $name = if ($layerName -eq 'base') { $sprite.name } else { $sprite.name + '_team' }
            Assert-True ($frame.name -ceq $name -and $frame.w -eq $width -and $frame.h -eq $height) 'Layer changed the native sprite rectangle.'
            Assert-True ($frame.x -ge 2 -and $frame.y -ge 2 -and $frame.x + $width + 2 -le 256 -and $frame.y + $height + 2 -le 256) 'Frame crosses the atlas padding boundary.'
            foreach ($other in $rectangles) {
                $apart = $frame.x + $frame.w + 4 -le $other.x -or $other.x + $other.w + 4 -le $frame.x -or
                         $frame.y + $frame.h + 4 -le $other.y -or $other.y + $other.h + 4 -le $frame.y
                Assert-True $apart 'Frames overlap or share their two-texel gutters.'
            }
            $rectangles.Add($frame)
            $engineFrame = $definition.sprite_frames[$frameIndex]
            Assert-True ($engineFrame.name -ceq $name -and $engineFrame.position.x -eq $frame.x -and $engineFrame.position.y -eq $frame.y -and $engineFrame.size.w -eq $width -and $engineFrame.size.h -eq $height) 'Engine sprite frame and authoring layout disagree.'
            if ($enginePivot) {
                Assert-True ($engineFrame.origin.x -eq $sprite.pivot[0] -and $engineFrame.origin.y -eq $sprite.pivot[1]) 'Runtime frame origin differs from its authored pivot.'
            }
            else {
                Assert-True (-not $engineFrame.PSObject.Properties['origin']) 'Legacy study frame unexpectedly gained a runtime origin.'
            }
            ++$frameIndex
            for ($y = 0; $y -lt $height; ++$y) {
                for ($x = 0; $x -lt $width; ++$x) {
                    $symbol = [string]$sprite.rows[$y][$x]
                    $isTeam = $team -ccontains $symbol
                    if (($layerName -eq 'team') -ne $isTeam) { continue }
                    $pixel = $colours[$symbol]
                    $offset = (($frame.y + $y) * 256 + $frame.x + $x) * 4
                    [Array]::Copy($pixel, 0, $expected, $offset, 4)
                }
            }
        }
    }
    Assert-True ($definition.sprite_frames.Count -eq $frameIndex) 'Unexpected engine sprite frames.'
    # Comparing the entire atlas checks every authored pixel, complementary team
    # alpha, transparent gutters and all unused space, not just selected samples.
    [ColourWars.WorkshopKitCheck]::Verify((Join-Path $Directory 'workshop_kit.png'),
        (Join-Path $Directory 'workshop_kit.dds'), $expected, 256, 256)
}

function Get-ArtifactHashes {
    param([string] $Directory)
    $hashes = [ordered]@{}
    foreach ($name in 'workshop_kit.png', 'workshop_kit.dds', 'workshop_kit.json', 'workshop_kit.layout.json') {
        $hashes[$name] = (Get-FileHash -LiteralPath (Join-Path $Directory $name) -Algorithm SHA256).Hash
    }
    return $hashes
}

function Test-PresentationAnchors {
    param($Source, $Player)
    # Read the independent authoring source, not a render diagnostic computed
    # from the same presentation offsets it is meant to check.
    $byName = [Collections.Generic.Dictionary[string, object]]::new([StringComparer]::Ordinal)
    foreach ($sprite in $Source.sprites) { $byName.Add($sprite.name, $sprite) }
    $scale = [double]$Source.world_units_per_texel
    Assert-True ($Player.sprite_sheet -ceq 'workshop_kit' -and $Player.pixel_size -eq $scale) 'Presentation changed the workshop sheet or native scale.'
    function Assert-Close {
        param($Actual, [double] $Expected, [string] $Label)
        Assert-True ($Actual -is [int] -or $Actual -is [long] -or $Actual -is [double] -or $Actual -is [decimal]) "Presentation $Label must be numeric."
        $number = [double]$Actual
        Assert-True (-not [double]::IsNaN($number) -and -not [double]::IsInfinity($number) -and
            [Math]::Abs($number - $Expected) -le 0.00001) "Presentation $Label differs from the authored source."
    }
    function Read-PresentationSprite {
        param([string] $Frame, $Size)
        Assert-True ($byName.ContainsKey($Frame)) "Presentation frame '$Frame' is not an authored sprite."
        $sprite = $byName[$Frame]
        Assert-True ($sprite.PSObject.Properties['engine_pivot'] -and $sprite.engine_pivot) "Presentation frame '$Frame' needs its native engine pivot."
        Assert-True ($Size.x -eq $sprite.rows[0].Length * $scale -and $Size.y -eq $sprite.rows.Count * $scale) "Presentation dimensions for '$Frame' differ from the authored source."
        return $sprite
    }
    function Test-AnchorOffset {
        param($Sprite, [string] $AnchorName, $Offset)
        $anchor = $Sprite.anchors.PSObject.Properties[$AnchorName].Value
        Assert-Close $Offset.x (($anchor[0] - $Sprite.pivot[0]) * $scale) "$($Sprite.name) $AnchorName x"
        Assert-Close $Offset.y (($anchor[1] - $Sprite.pivot[1]) * $scale) "$($Sprite.name) $AnchorName y"
    }
    $bodyFrames = @($Player.idle_frames) + @($Player.run_frames) + @($Player.air_frame, $Player.landing_frame)
    Assert-True ($Player.idle_frames.Count -eq 2 -and $Player.run_frames.Count -eq 4 -and $Player.badge_frames.Count -eq 4) 'Presentation must include the authored body cycle and four identity badges.'
    foreach ($frame in $bodyFrames) {
        $sprite = Read-PresentationSprite $frame $Player.body_size
        Assert-True ($sprite.team_symbols.Count -gt 0) "Body '$frame' has no animated team mask."
        Test-AnchorOffset $sprite 'shoulder' $Player.shoulder_offset
        Test-AnchorOffset $sprite 'rear_shoulder' $Player.rear_shoulder_offset
        Assert-True ($sprite.pivot[0] * 2 -eq $sprite.rows[0].Length -and $sprite.pivot[1] * 2 -eq $sprite.rows.Count) "Body '$frame' must draw around its centre."
    }
    $null = Read-PresentationSprite $Player.glove_frame $Player.glove_size
    foreach ($frame in $Player.badge_frames) { $null = Read-PresentationSprite $frame $Player.badge_size }
    Assert-True ($Player.sprayer.frame -ceq 'sprayer_pose_0') 'Continuous sprayer must use the authored zero-degree pose.'
    Assert-True ($Player.sprayer_poses.Count -eq 5) 'Presentation must include all five authored sprayer directions.'
    $poseNames = @('sprayer_pose_90up', 'sprayer_pose_45up', 'sprayer_pose_0', 'sprayer_pose_45down', 'sprayer_pose_90down')
    for ($index = 0; $index -lt $poseNames.Count; ++$index) {
        Assert-True ($Player.sprayer_poses[$index].frame -ceq $poseNames[$index]) 'Presentation sprayer direction is missing or has the wrong authored frame.'
    }
    $previousAngle = [double]::NegativeInfinity
    foreach ($pose in $Player.sprayer_poses) {
        Assert-True ($pose.aim_angle -gt $previousAngle) 'Authored sprayer directions must be ordered and unique.'
        $previousAngle = $pose.aim_angle
    }
    foreach ($tool in @($Player.sprayer) + @($Player.sprayer_poses) + @($Player.roller)) {
        $sprite = Read-PresentationSprite $tool.frame $tool.size
        Assert-True ($tool.team_frame -ceq ($sprite.name + '_team') -and $sprite.team_symbols.Count -gt 0) "Presentation tool '$($sprite.name)' has a mismatched team mask."
        $anchorName = if ($tool.frame -ceq $Player.roller.frame) { 'contact' } else { 'muzzle' }
        Test-AnchorOffset $sprite $anchorName $tool.muzzle_offset
        Test-AnchorOffset $sprite 'support_grip' $tool.support_grip_offset
        $anchor = $sprite.anchors.PSObject.Properties[$anchorName].Value
        $angle = [Math]::Atan2($anchor[1] - $sprite.pivot[1], $anchor[0] - $sprite.pivot[0])
        Assert-True ($tool.aim_angle -ge -[Math]::PI / 2 -and $tool.aim_angle -le [Math]::PI / 2) "Presentation aim angle for '$($sprite.name)' leaves the authored right-facing half-plane."
        Assert-Close $tool.aim_angle $angle "$($sprite.name) aim angle"
    }
}

function Write-Source {
    param($Data, [string] $Path)
    [IO.File]::WriteAllText($Path, ($Data | ConvertTo-Json -Depth 12) + "`n", [Text.UTF8Encoding]::new($false))
}

$builder = Join-Path $PSScriptRoot 'build-workshop-kit.ps1'
$sourceFile = [IO.Path]::GetFullPath($SourcePath)
$run = Join-Path ([IO.Path]::GetFullPath($OutputDirectory)) ([Guid]::NewGuid().ToString('N'))
$null = [IO.Directory]::CreateDirectory($run)
$first = Join-Path $run 'first'
$second = Join-Path $run 'second'
& $builder -SourcePath $sourceFile -OutputDirectory $first
& $builder -SourcePath $sourceFile -OutputDirectory $second
Test-Atlas $sourceFile $first
Test-Atlas $sourceFile $second
$hashes = Get-ArtifactHashes $first
$repeat = Get-ArtifactHashes $second
foreach ($name in $hashes.Keys) { Assert-True ($hashes[$name] -eq $repeat[$name]) "Nondeterministic output: $name" }
if (-not $ReferenceDirectory -and $sourceFile -eq [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../game/art/polish/kit/source.json'))) {
    $ReferenceDirectory = Split-Path -Parent $sourceFile
}
if ($ReferenceDirectory) {
    $ReferenceDirectory = [IO.Path]::GetFullPath($ReferenceDirectory)
    Test-Atlas $sourceFile $ReferenceDirectory
    $reference = Get-ArtifactHashes $ReferenceDirectory
    foreach ($name in $hashes.Keys) {
        Assert-True ($hashes[$name] -eq $reference[$name]) "Retained atlas is stale: $name. Run build-workshop-kit.ps1."
    }
}
$runtimeVerified = ''
$presentationVerified = $null
if ($checkRuntime) {
    $runtimeVerified = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../game/content/textures'))
    foreach ($name in 'workshop_kit.dds', 'workshop_kit.json') {
        $runtimePath = Join-Path $runtimeVerified $name
        Assert-True (Test-Path -LiteralPath $runtimePath -PathType Leaf) "Missing runtime kit: $name. Run build-workshop-kit.ps1 -StageRuntime."
        Assert-True ((Get-FileHash -LiteralPath $runtimePath -Algorithm SHA256).Hash -eq $hashes[$name]) "Runtime kit is stale: $name. Run build-workshop-kit.ps1 -StageRuntime."
    }
    $presentationPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../game/content/presentation.json'))
    $authoredSource = Get-Content -LiteralPath $sourceFile -Raw | ConvertFrom-Json
    $playerPresentation = (Get-Content -LiteralPath $presentationPath -Raw | ConvertFrom-Json).workshop_player
    Test-PresentationAnchors $authoredSource $playerPresentation
    $staleCases = [ordered]@{
        'muzzle offset' = @{ mutate = { param($p) $p.sprayer_poses[1].muzzle_offset.x += 1 }; diagnostic = 'muzzle x' }
        'support grip offset' = @{ mutate = { param($p) $p.roller.support_grip_offset.y += 1 }; diagnostic = 'support_grip y' }
        'native dimensions' = @{ mutate = { param($p) $p.glove_size.x += 1 }; diagnostic = 'dimensions' }
        'pose aim angle' = @{ mutate = { param($p) $p.sprayer_poses[1].aim_angle += 0.01 }; diagnostic = 'aim angle' }
        'body shoulder' = @{ mutate = { param($p) $p.shoulder_offset.x += 1 }; diagnostic = 'shoulder x' }
    }
    foreach ($case in $staleCases.GetEnumerator()) {
        $stale = ($playerPresentation | ConvertTo-Json -Depth 12) | ConvertFrom-Json
        & $case.Value.mutate $stale
        $failure = $null
        try { Test-PresentationAnchors $authoredSource $stale }
        catch { $failure = $_.Exception.Message }
        Assert-True ($null -ne $failure -and $failure -match $case.Value.diagnostic) "Stale presentation '$($case.Key)' was accepted or failed for the wrong reason: $failure"
    }
    $presentationVerified = [ordered]@{
        path = $presentationPath
        sha256 = (Get-FileHash -LiteralPath $presentationPath -Algorithm SHA256).Hash.ToLowerInvariant()
        body_frames = 8
        tool_records = 7
        rejected_stale_cases = @($staleCases.Keys)
    }
}

# Include fractional alpha independently of the current art, so an opaque-only
# source cannot accidentally let a broken premultiplication pipeline pass.
$fixture = [ordered]@{
    schema_version = 1
    world_units_per_texel = 4
    palette = [ordered]@{ '.' = '00000000'; 'o' = '#80402080'; 't' = '#C0C0C080'; 'w' = '#FFFFFF' }
    sprites = @(
        [ordered]@{ name = 'probe'; rows = @('.otw', 'wto.'); pivot = @(0.5, 2); engine_pivot = $true; anchors = [ordered]@{ muzzle = @(4, 0.5) }; team_symbols = @('t') },
        [ordered]@{ name = 'plain'; rows = @('wo', 'o.'); pivot = @(1, 1) }
    )
}
$fixturePath = Join-Path $run 'fixture.json'
$fixtureOutput = Join-Path $run 'fixture'
Write-Source $fixture $fixturePath
& $builder -SourcePath $fixturePath -OutputDirectory $fixtureOutput
Test-Atlas $fixturePath $fixtureOutput
$protected = Get-ArtifactHashes $fixtureOutput
$invalidPath = Join-Path $run 'invalid.json'
$rejected = [Collections.Generic.List[string]]::new()
function Test-Rejection {
    param([string] $Name, [scriptblock] $Mutate, [string] $Diagnostic)
    $bad = ($fixture | ConvertTo-Json -Depth 12) | ConvertFrom-Json
    & $Mutate $bad
    Write-Source $bad $invalidPath
    $failure = $null
    try { & $builder -SourcePath $invalidPath -OutputDirectory $fixtureOutput | Out-Null }
    catch { $failure = $_.Exception.Message }
    Assert-True ($null -ne $failure -and $failure -match $Diagnostic) "Malformed case '$Name' was accepted or failed for the wrong reason: $failure"
    $after = Get-ArtifactHashes $fixtureOutput
    foreach ($name in $protected.Keys) { Assert-True ($protected[$name] -eq $after[$name]) "Rejected source overwrote $name." }
    $rejected.Add($Name)
}

Test-Rejection 'ragged rows' { param($d) $d.sprites[0].rows[1] = 'o' } 'every row'
Test-Rejection 'undefined symbol' { param($d) $d.sprites[0].rows[1] = 'zzz.' } 'undefined palette'
Test-Rejection 'duplicate identifier' { param($d) $d.sprites[1].name = 'probe' } 'Duplicate sprite/frame'
Test-Rejection 'generated layer collision' { param($d) $d.sprites[1].name = 'probe_team' } 'Duplicate sprite/frame'
Test-Rejection 'bad colour' { param($d) $d.palette.o = 'ultraviolet' } 'needs RRGGBB'
Test-Rejection 'coloured team mask' { param($d) $d.sprites[0].team_symbols = @('o') } 'visible grayscale'
Test-Rejection 'duplicate team symbol' { param($d) $d.sprites[0].team_symbols = @('t', 't') } 'unique, defined'
Test-Rejection 'out of bounds pivot' { param($d) $d.sprites[0].pivot = @(5, 1) } 'within the native'
Test-Rejection 'invalid engine pivot flag' { param($d) $d.sprites[0].engine_pivot = 'yes' } 'engine_pivot must be a boolean'
Test-Rejection 'nonnumeric anchor' { param($d) $d.sprites[0].anchors.muzzle = @('NaN', 1) } 'finite numbers'
Test-Rejection 'wrong texel density' { param($d) $d.world_units_per_texel = 1 } 'exactly 4 world'
Test-Rejection 'atlas overflow' {
    param($d)
    $d.sprites[0].rows = @(1..252 | ForEach-Object { 'w' * 252 })
} 'exceeds its 256x256 atlas'

$result = [ordered]@{
    outcome = 'passed'
    source = $sourceFile
    source_sha256 = (Get-FileHash -LiteralPath $sourceFile -Algorithm SHA256).Hash.ToLowerInvariant()
    artifact_sha256 = $hashes
    deterministic_builds = 2
    retained_output_verified = $ReferenceDirectory
    runtime_output_verified = $runtimeVerified
    presentation_source_contract = $presentationVerified
    checks = @('all native PNG texels', 'disjoint team layers', 'frame rectangles and transparent gutters',
        'palette order', 'world dimensions and attachment coordinates', 'opt-in native engine origins', 'DDS RGBA single mip',
        'premultiplied UNORM including fractional alpha', 'malformed input preserves prior output')
    rejected_cases = $rejected.ToArray()
}
$resultPath = Join-Path $run 'result.json'
[IO.File]::WriteAllText($resultPath, ($result | ConvertTo-Json -Depth 8) + "`n", [Text.UTF8Encoding]::new($false))
Write-Output "Workshop kit checks passed. Evidence: $resultPath"
