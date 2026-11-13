# Terrain backdrop kit source

[`terrain.json`](terrain.json) is the editable recipe for the `terrain` sheet:
the Muted Day palette, the quiet backdrop props a level hangs behind play, the
airbrush shadow tiles hung under platforms, and one swatch per palette role.
The props are deliberately crude flat drawings: one bg_line outline cell (never
black), flat bg_far/bg_mid/bg_detail fills and staircase curves on the cell
grid. Every hue lives in these texels; runtime draws the sheet with a WHITE tint.

Run `powershell -ExecutionPolicy Bypass -File scripts/build-terrain-art.ps1`
from the repository root. Windows System.Drawing fills the integer-cell shapes
without antialiasing and draws lines cell by cell. Each cell expands to exactly
4x4 texels, producing the 2048x920 working atlas
`../sources/textures/terrain.png`, runtime
`../../content/textures/terrain.json` and `.png` (a copy of the working PNG),
and the review [`lineup.png`](lineup.png). Ordinary builds need neither
System.Drawing nor Python; they stage only the PNG and JSON.

A prop draws `fill`, outlines the union one cell thick (`edges: top` for the
hills, whose sides and bottom run under their neighbours), then draws `detail`
on top. Shapes are `[op, role, ...]` in cells: `rect x y w h`,
`ellipse x y w h`, `poly x0 y0 x1 y1 ...`, `line x0 y0 x1 y1` and
`path x0 y0 x1 y1 ...`. The shadow tiles are character grids, one speck per
cell. The build fails if any opaque texel is not a palette colour, if a prop
draws outside the backdrop roles, if a hill's bottom row is not solid, if a
speck sits on a tile seam, or if any two frames touch without a transparent
gutter.

| Frame | Texels | Drawing |
|---|---|---|
| `hill_wide` | 1536x320 | two rolling bg_far humps with one flat step where the drag paused, outline on top only, flat opaque bottom row |
| `hill_small` | 768x192 | one bg_far hump with a low shoulder dragged a cell too deep, same horizon rules |
| `tree_round` | 128x224 | round bg_mid canopy on a one-cell bg_line stick trunk that leans |
| `tree_tall` | 96x256 | tall faceted bg_mid canopy on a stick trunk that leans the other way |
| `house` | 384x352 | bg_mid gable roof, its peak a little left of centre, and walls, two bg_detail windows with a bar, a door with a knob |
| `cloud_big` | 256x96 | two bg_detail bumps and a last one drawn too big, on a long flat base |
| `cloud_small` | 160x64 | two bumps, the second dragged wide, on a flat base |
| `sun` | 224x224 | bg_detail disc, twelve bg_line rays (one off level, one short), a flat little face; not yellow |
| `birds` | 96x32 | two bg_line gull strokes |
| `brush_pot` | 256x320 | quiet stationery pot with three differently shaped brushes, flat fills and a plain rim with no highlights; bg_line outlines distinguish it from walkable terrain |
| `tape_roll` | 256x160 | flat roll with a paper core and short peeled end, no shading inside the core or on the end; the same backdrop roles, without team colours or text |
| `cf_paint_tube` | 128x256 | Crossfeed: squeezed-out paint tube standing on its ribbed cap, crimped end up, two crumple lines and a blank colour band |
| `cf_pencil_cup` | 160x288 | Crossfeed: plain banded cup with two sharpened pencils, one upside down on its eraser and a leaning stub |
| `cf_palette` | 384x256 | Crossfeed: kidney palette propped on a stick, thumb hole and notch, grey wells and one empty ring |
| `shadow_0`..`shadow_3` | 64x32 | shadow_deep specks dense in the top two rows, shadow specks thinning to none in the last; columns 0 and 15 empty so tiles abut in any order |
| `swatch_<role>` | 4x4 | one opaque block per palette role |

The palette is Muted Day, the 26 roles in
[the art rules](../../../docs/polish-plan.md#1-establish-the-art-and-display-rules).
The build checks the role list and order against that table.

`lineup.png` shows every frame at three pixels per cell, the four shadow tiles
abutted in two orders to check the seams, every frame again at one pixel per
cell, and the swatches, all on the paper colour. The PNG has straight alpha; the
engine's PNG loader premultiplies it into RGBA8 UNORM with one mip as it loads.
Frame order and coordinates are fixed, and rebuilding the same source produces
byte-identical PNG, JSON and lineup. The two landmarks
occupy a new shelf after the original frames and swatches; all original frame
coordinates and the original 2048x596 texel region remain unchanged.

Arena props (`POL#16`) follow the original eleven in `props`. The build takes
each one's name and cell size from the recipe (levels place frames by that
size), packs them after the two landmarks on the landmark shelves, and grows
the atlas and the lineup downward to fit; the original frames keep their
coordinates. An arena prop obeys every rule above.
