[CmdletBinding()]
param()

. (Join-Path $PSScriptRoot 'art-png.ps1')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Add-Type -AssemblyName System.Drawing

if (-not ('ColourWars.PainterArt' -as [type])) {
    Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition (@'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Text;
namespace ColourWars {
    public static class PainterArt {
        public static void SavePng(Bitmap bitmap, string path) {
            PainterPng.SavePng(bitmap, path);
        }

        // The face rules a slip must keep (docs/drawing-mistakes-research.md,
        // stage 2 and section 4): the head outline closed to a 4-connected
        // bucket fill, no transparent pocket inside it, and on idle a white
        // row between the eyes and the brim. Returns null when the face is fine.
        public static string FaceProblem(string[] rows, bool idle) {
            int height = rows.Length, width = rows[0].Length;
            var outside = new bool[height, width];
            var stack = new System.Collections.Generic.Stack<int[]>();
            for (int row = 0; row < height; ++row)
                for (int column = 0; column < width; ++column)
                    if ((row == 0 || row == height - 1 || column == 0 || column == width - 1) &&
                        rows[row][column] != 'K' && !outside[row, column]) {
                        outside[row, column] = true;
                        stack.Push(new[] { row, column });
                    }
            int[][] steps = { new[] { 1, 0 }, new[] { -1, 0 }, new[] { 0, 1 }, new[] { 0, -1 } };
            while (stack.Count > 0) {
                int[] at = stack.Pop();
                foreach (int[] step in steps) {
                    int row = at[0] + step[0], column = at[1] + step[1];
                    if (row < 0 || row >= height || column < 0 || column >= width ||
                        outside[row, column] || rows[row][column] == 'K') continue;
                    outside[row, column] = true;
                    stack.Push(new[] { row, column });
                }
            }
            for (int row = 5; row <= 10; ++row)
                for (int column = 0; column < width; ++column) {
                    if (rows[row][column] == 'W' && outside[row, column])
                        return "face cell (" + row + "," + column + ") is open to a bucket fill; the outline must close.";
                    if (rows[row][column] == '.' && !outside[row, column])
                        return "leaves a transparent pocket at (" + row + "," + column + ") inside the head.";
                }
            if (!idle) return null;
            // A face feature inside the head box that reaches row 5 and below it.
            var seen = new bool[height, width];
            for (int row = 5; row <= 10; ++row)
                for (int column = 5; column <= 11; ++column) {
                    if (rows[row][column] != 'K' || seen[row, column]) continue;
                    int top = row, bottom = row;
                    seen[row, column] = true;
                    stack.Push(new[] { row, column });
                    while (stack.Count > 0) {
                        int[] at = stack.Pop();
                        top = Math.Min(top, at[0]);
                        bottom = Math.Max(bottom, at[0]);
                        foreach (int[] step in steps) {
                            int r = at[0] + step[0], c = at[1] + step[1];
                            if (r < 5 || r > 10 || c < 5 || c > 11 || seen[r, c] || rows[r][c] != 'K') continue;
                            seen[r, c] = true;
                            stack.Push(new[] { r, c });
                        }
                    }
                    if (top == 5 && bottom >= 6)
                        return "puts a face feature in row 5; keep a white row between the eyes and the brim.";
                }
            return null;
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
'@ + (Get-ArtPngSource 'PainterPng'))
}

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$sourceDir = Join-Path $repo 'game/art/painter'
$working = Join-Path $repo 'game/art/sources/textures'
$textures = Join-Path $repo 'game/content/textures'
$source = Get-Content -Raw (Join-Path $sourceDir 'painter.json') | ConvertFrom-Json
if ($source.cell_size -ne 4 -or $source.width -ne 17 -or $source.height -ne 30) {
    throw 'Painter source must be a 17x30 grid with four source texels per cell.'
}
foreach ($name in @('idle', 'run', 'jump', 'fall')) {
    $frameTime = [double] $source.locomotion_frame_times.$name
    if ($frameTime -le 0 -or [double]::IsNaN($frameTime) -or [double]::IsInfinity($frameTime)) {
        throw "$name must author a finite positive locomotion frame time."
    }
}
# The 13-cell hitbox sits in the middle; two cells either side are overhang.
$cells = 17
$canvasWidth = $cells * 4

function Assert-Grid {
    param([string[]] $Rows, [int] $Width, [int] $Height, [string] $Name)
    if ($Rows.Count -ne $Height) { throw "$Name has $($Rows.Count) rows; expected $Height." }
    foreach ($row in $Rows) {
        if ($row.Length -ne $Width -or $row -match '[^.KWT]') {
            throw "Invalid $Name row '$row'; expected $Width cells using . K W T."
        }
    }
}

Assert-Grid $source.torso $cells 22 'torso'
$poses = [ordered]@{}
foreach ($name in @('idle', 'run_0', 'run_1', 'run_2', 'run_3', 'run_4',
    'run_5', 'run_6', 'run_7', 'jump', 'fall')) {
    [string[]] $legs = $source.legs.$name
    Assert-Grid $legs $cells 8 $name
    [string[]] $rows = @($source.torso) + @($legs)
    # Source rectangles are contiguous in Labrador strips; each authored pose
    # reserves one empty cell on each side, giving eight visible-art gutter texels.
    if (@($rows | Where-Object { $_[0] -ne '.' -or $_[$cells - 1] -ne '.' }).Count) {
        throw "$name must keep four transparent texels at both horizontal edges."
    }
    if ($name -ne 'jump' -and $rows[29] -notmatch 'K') {
        throw "$name must plant at least one foot on the y=120 baseline."
    }
    $poses.Add($name, $rows)
}
Assert-Grid $source.rest_arms $cells 30 'rest_arms'
Assert-Grid $source.sleeves $cells 30 'sleeves'
for ($row = 0; $row -lt 30; ++$row) {
    for ($column = 0; $column -lt $cells; ++$column) {
        $sleeve = $source.sleeves[$row][$column]
        if ($sleeve -ne '.' -and $sleeve -ne $source.rest_arms[$row][$column]) {
            throw 'sleeves must be the rest arms without their hands, so the shoulders match.'
        }
    }
}
$actions = @('takeoff', 'land', 'fire', 'hurt', 'elimination')
foreach ($name in $actions) {
    $action = $source.actions.$name
    if ($action.frames.Count -ne 2 -or $action.frame_time -le 0) {
        throw "$name must author two full frames and a positive frame_time."
    }
    for ($frame = 0; $frame -lt $action.frames.Count; ++$frame) {
        [string[]] $rows = $action.frames[$frame]
        $pose = "${name}_$frame"
        Assert-Grid $rows $cells 30 $pose
        if (@($rows | Where-Object { $_[0] -ne '.' -or $_[$cells - 1] -ne '.' }).Count) {
            throw "$pose must keep four transparent texels at both horizontal edges."
        }
        # The cap, head's bottom edge, neck, body top and shoulder roots stay
        # fixed. Expressions, ears and the lower bib can move within the band.
        foreach ($row in @(0, 1, 2, 3, 4, 11, 12, 13, 14, 15)) {
            if ($rows[$row] -ne $source.torso[$row]) {
                throw "$pose must preserve the cap, head outline and shoulder roots."
            }
        }
        $poses.Add($pose, $rows)
    }
}

# The fill must finish exactly where the live respawn pencil left its outline.
# Validate the authored presentation before changing any generated output.
$presentation = Get-Content -Raw (Join-Path $repo 'game/content/presentation.json') | ConvertFrom-Json
$look = $presentation.respawn_redraw
if ($look.canvas_size.x -ne $canvasWidth -or $look.canvas_size.y -ne 120 -or $look.pen_width -ne 4) {
    throw "Painter redraw requires respawn_redraw canvas_size ${canvasWidth}x120 and pen_width 4 in presentation.json."
}

# The rest arms are drawn over the body, so the finished figure is the pose
# with every opaque rest-arm cell on top.
function Get-Dressed {
    param([string[]] $Rows)
    $dressed = [string[]]::new(30)
    for ($row = 0; $row -lt 30; ++$row) {
        $line = [char[]] $Rows[$row]
        for ($column = 0; $column -lt $cells; ++$column) {
            if ($source.rest_arms[$row][$column] -ne '.') { $line[$column] = $source.rest_arms[$row][$column] }
        }
        $dressed[$row] = [string]::new($line)
    }
    return , $dressed
}

function Test-Pen {
    param([string[]] $Rows, [object[]] $Strokes, [string] $Who)
    $dressed = Get-Dressed $Rows
    $expected = [Collections.Generic.HashSet[int]]::new()
    for ($row = 0; $row -lt 30; ++$row) {
        for ($column = 0; $column -lt $cells; ++$column) {
            if ($dressed[$row][$column] -ne 'K') { continue }
            for ($dy = 0; $dy -lt 4; ++$dy) {
                for ($dx = 0; $dx -lt 4; ++$dx) {
                    [void] $expected.Add((($row * 4 + $dy) * $canvasWidth + $column * 4 + $dx))
                }
            }
        }
    }
    $drawn = [Collections.Generic.HashSet[int]]::new()
    foreach ($stroke in $Strokes) {
        foreach ($point in $stroke.points) {
            if ($point.x -lt 0 -or $point.x -gt $canvasWidth - 4 -or $point.y -lt 0 -or $point.y -gt 116 -or
                $point.x % 4 -ne 0 -or $point.y % 4 -ne 0) {
                throw "Painter redraw '$($stroke.part)' ($Who) points must stay on the 4px grid inside the ${canvasWidth}x120 canvas."
            }
        }
        for ($at = 1; $at -lt $stroke.points.Count; ++$at) {
            $from = $stroke.points[$at - 1]
            $to = $stroke.points[$at]
            if (($from.x -eq $to.x) -eq ($from.y -eq $to.y)) {
                throw "Painter redraw '$($stroke.part)' ($Who) needs nonzero horizontal or vertical segments."
            }
            for ($y = [Math]::Min($from.y, $to.y); $y -lt [Math]::Max($from.y, $to.y) + 4; ++$y) {
                for ($x = [Math]::Min($from.x, $to.x); $x -lt [Math]::Max($from.x, $to.x) + 4; ++$x) {
                    [void] $drawn.Add(($y * $canvasWidth + $x))
                }
            }
        }
    }
    if (-not $expected.SetEquals($drawn)) {
        $missing = @($expected | Where-Object { -not $drawn.Contains($_) }).Count
        $extra = @($drawn | Where-Object { -not $expected.Contains($_) }).Count
        throw "Painter redraw ($Who) differs from idle + rest_arms: $missing missing black texels, $extra extra. Update presentation.json respawn_redraw.strokes (or that person's figure.slips stroke edits) to match game/art/painter; see its README pen recipe."
    }
    return $expected.Count
}
$penTexels = Test-Pen $poses['idle'] @($look.strokes) 'the house hand'
Write-Output "Validated respawn pencil against $penTexels authored black texels."

# Figure slips (POL#46 stage 2): each match's person drew one slip, or two that
# are not exclusive, over rows 5-10. Every person's face is checked like the
# authored figure and baked into the atlas as a band; see presentation.json
# figure.slips and docs/drawing-mistakes-research.md section 9, decision 8.
$slipSource = Get-Content -Raw (Join-Path $sourceDir 'slips.json') | ConvertFrom-Json
$slipLook = $presentation.figure.slips
$band = $slipLook.band
if ($band.x -ne 12 -or $band.y -ne 20 -or $band.width -ne 44 -or $band.height -ne 24) {
    throw 'figure.slips.band must be canvas texels (12,20,44,24): rows 5-10, cells 3-13, symmetric under the facing flip.'
}
$bandRows = 5..10
$bandFirst = 3
$bandCells = 11
$slipIds = @($slipSource.slips | ForEach-Object { $_.id })
$lookIds = @($slipLook.list | ForEach-Object { $_.id })
if (($slipIds -join ',') -ne ($lookIds -join ',')) {
    throw "presentation.json figure.slips.list ($($lookIds -join ', ')) must name game/art/painter/slips.json's slips in the same order ($($slipIds -join ', '))."
}
$poseNames = @('idle') + @($actions | ForEach-Object { "${_}_0"; "${_}_1" })
foreach ($name in $poseNames) {
    foreach ($row in $bandRows) {
        $line = $poses[$name][$row]
        if ($line.Substring(0, $bandFirst) -ne '...' -or $line.Substring($bandFirst + $bandCells) -ne '...' -or $line -match 'T') {
            throw "$name row $row must keep its face inside cells 3-13 with no team cells, so a band can replace it."
        }
    }
}

# A slip's rows for one pose: its own rows for an action it authored, else
# idle's changed cells wherever that pose still draws idle's original cell.
function Get-SlipPose {
    param([object] $Slip, [string] $Name)
    $rows = [string[]] @($poses[$Name])
    $own = if ($Slip.PSObject.Properties['actions'] -and $Slip.actions.PSObject.Properties[$Name]) {
        @($Slip.actions.$Name) } else { $null }
    for ($index = 0; $index -lt 6; ++$index) {
        $row = $bandRows[$index]
        $authored = [string] $Slip.rows_5_10[$index]
        if ($authored.Length -ne $cells -or $authored -match '[^.KW]') {
            throw "Slip '$($Slip.id)' row $row must be $cells cells of . K W."
        }
        if ($null -ne $own) {
            $rows[$row] = [string] $own[$index]
            if ($rows[$row].Length -ne $cells -or $rows[$row] -match '[^.KW]') {
                throw "Slip '$($Slip.id)' $Name row $row must be $cells cells of . K W."
            }
            continue
        }
        $line = [char[]] $rows[$row]
        for ($column = 0; $column -lt $cells; ++$column) {
            $was = $poses['idle'][$row][$column]
            if ($authored[$column] -ne $was -and $line[$column] -eq $was) { $line[$column] = $authored[$column] }
        }
        $rows[$row] = [string]::new($line)
    }
    return , $rows
}

$exclusive = [Collections.Generic.HashSet[string]]::new()
foreach ($pair in $slipLook.exclusive) {
    if (@($pair).Count -ne 2 -or $slipIds -notcontains $pair[0] -or $slipIds -notcontains $pair[1] -or $pair[0] -eq $pair[1]) {
        throw "figure.slips.exclusive pair '$($pair -join '+')' must name two different slips."
    }
    [void] $exclusive.Add("$($pair[0])+$($pair[1])")
    [void] $exclusive.Add("$($pair[1])+$($pair[0])")
}
# People: every slip alone, then every pair that is not exclusive, in list order.
$people = [Collections.Generic.List[object]]::new()
for ($first = 0; $first -lt $slipIds.Count; ++$first) { $people.Add([int[]] @($first)) }
for ($first = 0; $first -lt $slipIds.Count; ++$first) {
    for ($second = $first + 1; $second -lt $slipIds.Count; ++$second) {
        if (-not $exclusive.Contains("$($slipIds[$first])+$($slipIds[$second])")) {
            $people.Add([int[]] @($first, $second))
        }
    }
}

# A person's strokes: the house strokes with each slip's edits, by index.
function Get-PersonStrokes {
    param([int[]] $Person, [string] $Who)
    $replace = @{}
    $insert = @{}
    foreach ($index in $Person) {
        foreach ($edit in $slipLook.list[$index].strokes) {
            if ($edit.PSObject.Properties['after']) {
                $at = [int] $edit.after
                if ($at -lt 0 -or $at -ge $look.strokes.Count) { throw "$Who inserts after stroke $at, which does not exist." }
                if (-not $insert.ContainsKey($at)) { $insert[$at] = [Collections.Generic.List[object]]::new() }
                $insert[$at].Add([pscustomobject]@{ part = $edit.part; points = @($edit.points) })
                continue
            }
            $at = [int] $edit.stroke
            if ($at -lt 0 -or $at -ge $look.strokes.Count) { throw "$Who edits stroke $at, which does not exist." }
            if ($replace.ContainsKey($at)) { throw "$Who edits stroke $at twice; make the two slips exclusive." }
            $replace[$at] = if ($edit.PSObject.Properties['remove'] -and $edit.remove) { $null } else {
                [pscustomobject]@{ part = $look.strokes[$at].part; points = @($edit.points) } }
        }
    }
    $strokes = [Collections.Generic.List[object]]::new()
    for ($at = 0; $at -lt $look.strokes.Count; ++$at) {
        if (-not $replace.ContainsKey($at)) { $strokes.Add($look.strokes[$at]) }
        elseif ($null -ne $replace[$at]) { $strokes.Add($replace[$at]) }
        if ($insert.ContainsKey($at)) { $strokes.AddRange($insert[$at]) }
    }
    return , $strokes.ToArray()
}

$slipPoses = @{}
foreach ($slip in $slipSource.slips) {
    $slipPoses[$slip.id] = @{}
    foreach ($name in $poseNames) { $slipPoses[$slip.id][$name] = Get-SlipPose $slip $name }
}
$personNames = [Collections.Generic.List[string]]::new()
$personPoses = @{}
foreach ($person in $people) {
    $who = (@($person | ForEach-Object { $slipIds[$_] }) -join '+')
    $personNames.Add($who)
    $personPoses[$who] = @{}
    foreach ($name in $poseNames) {
        $rows = [string[]] @($poses[$name])
        $changed = @{}
        foreach ($index in $person) {
            $slipped = $slipPoses[$slipIds[$index]][$name]
            foreach ($row in $bandRows) {
                $line = [char[]] $rows[$row]
                for ($column = 0; $column -lt $cells; ++$column) {
                    if ($slipped[$row][$column] -eq $poses[$name][$row][$column]) { continue }
                    if ($changed.ContainsKey("$row,$column")) {
                        throw "$who`: two slips change $name cell ($row,$column); make them exclusive."
                    }
                    $changed["$row,$column"] = $true
                    $line[$column] = $slipped[$row][$column]
                }
                $rows[$row] = [string]::new($line)
            }
        }
        foreach ($row in $bandRows) {
            if ($rows[$row].Substring(0, $bandFirst) -ne '...' -or $rows[$row].Substring($bandFirst + $bandCells) -ne '...') {
                throw "$who`: $name row $row reaches outside the band, cells 3-13."
            }
        }
        $problem = [ColourWars.PainterArt]::FaceProblem((Get-Dressed $rows), $name -eq 'idle')
        if ($problem) { throw "$who`: $name $problem" }
        $personPoses[$who][$name] = $rows
    }
    [void] (Test-Pen $personPoses[$who]['idle'] (Get-PersonStrokes $person $who) $who)
}
Write-Output "Validated $($people.Count) people's faces and respawn pencils ($($slipIds.Count) slips, $($exclusive.Count / 2) exclusive pairs)."

$atlas = [Drawing.Bitmap]::new(1024, 2048, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
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
                [ColourWars.PainterArt]::Grid($atlas, $poses[$pose], 4 + $canvasWidth * $frame,
                    $y, 4, $paint, [Drawing.Color]::White, $false)
            }
            $strips.Add([ordered]@{ name = $name + $suffix;
                first_frame = [ordered]@{ x = 4; y = $y; w = $canvasWidth; h = 120 };
                frame_count = $count; frame_time = $source.locomotion_frame_times.$name;
                looping = $true })
            # The uniform animation reads idle_paint as a standalone frame.
            if ($name -eq 'idle' -and $paint) { Add-Frame ($name + $suffix) 4 $y $canvasWidth 120 }
            ++$row
        }
        # Arm layers share the idle row, their paint copies 512 lower too.
        foreach ($arms in @(@{ name = 'rest_arms'; x = 8 + $canvasWidth },
            @{ name = 'sleeves'; x = 12 + 2 * $canvasWidth })) {
            [ColourWars.PainterArt]::Grid($atlas, $source.($arms.name), $arms.x, 4 + $layerY, 4,
                $paint, [Drawing.Color]::White, $false)
            Add-Frame ($arms.name + $suffix) $arms.x (4 + $layerY) $canvasWidth 120
        }
    }

    # Action rows pair neutral and paint strips below the locomotion layers.
    for ($row = 0; $row -lt $actions.Count; ++$row) {
        $name = $actions[$row]
        $action = $source.actions.$name
        $y = 1028 + 128 * $row
        foreach ($paint in @($false, $true)) {
            $suffix = if ($paint) { '_paint' } else { '' }
            $x = if ($paint) { 12 + 2 * $canvasWidth } else { 4 }
            for ($frame = 0; $frame -lt $action.frames.Count; ++$frame) {
                [ColourWars.PainterArt]::Grid($atlas, $poses["${name}_$frame"],
                    ($x + $canvasWidth * $frame), $y, 4, $paint, [Drawing.Color]::White, $false)
            }
            $strips.Add([ordered]@{ name = $name + $suffix;
                first_frame = [ordered]@{ x = $x; y = $y; w = $canvasWidth; h = 120 };
                frame_count = $action.frames.Count; frame_time = $action.frame_time;
                looping = $false })
        }
    }

    # Face bands: one 44x24 frame per distinct face, named for every person and
    # pose that draws it, packed on a 52x32 pitch into the two free blocks right
    # of the action strips and under them.
    $slots = [Collections.Generic.List[object]]::new()
    for ($y = 1024; $y + 32 -le 1664; $y += 32) {
        for ($x = 12 + 4 * $canvasWidth; $x + 52 -le 1024; $x += 52) { $slots.Add(@($x, $y)) }
    }
    for ($y = 1664; $y + 32 -le 2048; $y += 32) {
        for ($x = 0; $x + 52 -le 1024; $x += 52) { $slots.Add(@($x, $y)) }
    }
    $placed = @{}
    foreach ($who in $personNames) {
        foreach ($name in $poseNames) {
            $face = [string[]] @($bandRows | ForEach-Object { $personPoses[$who][$name][$_].Substring($bandFirst, $bandCells) })
            $key = $face -join '|'
            if (-not $placed.ContainsKey($key)) {
                if ($placed.Count -ge $slots.Count) { throw "The painter atlas has room for $($slots.Count) face bands." }
                $slot = $slots[$placed.Count]
                [ColourWars.PainterArt]::Grid($atlas, $face, $slot[0] + 4, $slot[1] + 4, 4,
                    $false, [Drawing.Color]::White, $false)
                $placed[$key] = $slot
            }
            $at = $placed[$key]
            # Locomotion shares idle's torso rows, so every locomotion strip draws idle's band.
            $pose = if ($name -eq 'idle') { 'idle.0' } else { $name -replace '_(\d)$', '.$1' }
            Add-Frame "$($slipLook.band_frame).$who.$pose" ($at[0] + 4) ($at[1] + 4) ($bandCells * 4) 24
        }
    }
    Write-Output "Baked $($placed.Count) distinct face bands for $($personNames.Count) people."

    $pixelX = 16 + 3 * $canvasWidth
    [ColourWars.PainterArt]::Grid($atlas, @('W'), $pixelX, 4, 4,
        $false, [Drawing.Color]::White, $false)
    Add-Frame 'pixel' $pixelX 4 4 4

    [ColourWars.PainterArt]::SavePng($atlas, (Join-Path $working 'painter.png'))
    $definition = [ordered]@{ animation_strips = $strips.ToArray(); sprite_frames = $frames.ToArray() }
    [IO.File]::WriteAllText((Join-Path $textures 'painter.json'),
        ($definition | ConvertTo-Json -Depth 6) + [Environment]::NewLine,
        [Text.UTF8Encoding]::new($false))
}
finally { $atlas.Dispose() }

# Production source preview: every authored pose in each supported hue, with
# the rest arms over it as the game draws them, and 28 px action silhouettes.
$spacing = $canvasWidth + 16
$palettes = @($presentation.team_palette.colours)
$silhouettesY = 64 + 312 * $palettes.Count
$preview = [Drawing.Bitmap]::new((108 + $spacing * 11), (152 + 312 * $palettes.Count), [Drawing.Imaging.PixelFormat]::Format32bppArgb)
$graphics = [Drawing.Graphics]::FromImage($preview)
$font = [Drawing.Font]::new([Drawing.FontFamily]::GenericMonospace, 9)
$brush = [Drawing.SolidBrush]::new([Drawing.Color]::Black)
$ground = [Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(128, 128, 128))
try {
    $graphics.Clear([Drawing.Color]::FromArgb(228, 228, 228))
    $graphics.DrawString("Painter / editable ${cells}x30 cells / native ${canvasWidth}x120 over a 52x120 hitbox", $font, $brush, 12, 8)
    $row = 0
    foreach ($entry in $palettes) {
        $colour = [Drawing.ColorTranslator]::FromHtml('#' + $entry.colour)
        $column = 0
        foreach ($name in $poses.Keys) {
            # Eleven locomotion drawings followed by a row of ten action frames.
            $poseRow = if ($column -lt 11) { 0 } else { 1 }
            $poseColumn = if ($column -lt 11) { $column } else { $column - 11 }
            $y = 52 + 312 * $row + 156 * $poseRow
            $x = 92 + $spacing * $poseColumn
            if ($poseColumn -eq 0) {
                $graphics.DrawString($entry.name, $font, $brush, 8, $y + 50)
            }
            $label = $name.Replace('elimination', 'elim')
            $graphics.DrawString($label, $font, $brush, $x - 2, $y - 20)
            $graphics.FillRectangle($ground, $x - 4, $y + 120, $canvasWidth + 8, 4)
            $graphics.Flush()
            [ColourWars.PainterArt]::Grid($preview, $poses[$name], $x, $y, 4,
                $false, $colour, $true)
            [ColourWars.PainterArt]::Grid($preview, $source.rest_arms, $x, $y, 4,
                $false, $colour, $true)
            ++$column
        }
        ++$row
    }
    $graphics.Flush()
    # DrawImage uses the same source sprite as the native preview; no resampling
    # is involved in authoring the runtime atlas itself.
    $graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
    $graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::Half
    $graphics.DrawString('Action silhouettes / 28 px bodies (nearest-neighbour preview)', $font, $brush, 12, $silhouettesY)
    for ($column = 0; $column -lt 10; ++$column) {
        $x = 92 + $spacing * $column
        $graphics.DrawImage($preview, [Drawing.Rectangle]::new($x, ($silhouettesY + 24), 16, 28),
            $x, 208, $canvasWidth, 120, [Drawing.GraphicsUnit]::Pixel)
        $name = @($poses.Keys)[$column + 11]
        $graphics.DrawString($name.Replace('elimination', 'elim'), $font, $brush, $x - 2, ($silhouettesY + 60))
    }
    [ColourWars.PainterArt]::SavePng($preview, (Join-Path $sourceDir 'lineup.png'))
}
finally {
    $ground.Dispose(); $brush.Dispose(); $font.Dispose(); $graphics.Dispose(); $preview.Dispose()
}

# Authoring preview of every person the figure slips allow: idle, as blue's
# figure, with the rest arms over it.
$columns = 9
$preview = [Drawing.Bitmap]::new(12 + $columns * ($canvasWidth + 48),
    40 + [Math]::Ceiling($personNames.Count / $columns) * 156, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
$graphics = [Drawing.Graphics]::FromImage($preview)
$font = [Drawing.Font]::new([Drawing.FontFamily]::GenericMonospace, 7)
$brush = [Drawing.SolidBrush]::new([Drawing.Color]::Black)
try {
    $graphics.Clear([Drawing.Color]::FromArgb(228, 228, 228))
    $graphics.DrawString("Figure slips / $($personNames.Count) people from game/art/painter/slips.json", $font, $brush, 12, 8)
    $blue = [Drawing.ColorTranslator]::FromHtml('#' + ($palettes | Where-Object name -eq 'blue').colour)
    for ($index = 0; $index -lt $personNames.Count; ++$index) {
        $who = $personNames[$index]
        $x = 12 + ($index % $columns) * ($canvasWidth + 48)
        $y = 52 + [Math]::Floor($index / $columns) * 156
        $graphics.DrawString($who.Replace('+', "+`n"), $font, $brush, $x - 6, $y - 26)
        $graphics.Flush()
        [ColourWars.PainterArt]::Grid($preview, $personPoses[$who]['idle'], $x, $y, 4, $false, $blue, $true)
        [ColourWars.PainterArt]::Grid($preview, $source.rest_arms, $x, $y, 4, $false, $blue, $true)
    }
    $graphics.Flush()
    [ColourWars.PainterArt]::SavePng($preview, (Join-Path $sourceDir 'slips.png'))
}
finally {
    $brush.Dispose(); $font.Dispose(); $graphics.Dispose(); $preview.Dispose()
}

# Labrador uses premultiplied alpha. RGBA8 and one mip retain the authored grid.
& (Join-Path $repo 'game/art/tools/texconv.exe') (Join-Path $working 'painter.png') `
    -pmalpha -m 1 -f R8G8B8A8_UNORM -y -o $textures
if ($LASTEXITCODE -ne 0) { throw "texconv failed with exit code $LASTEXITCODE" }
Write-Output 'Built painter.png, painter.json, premultiplied painter.dds and source lineup.png and slips.png.'
