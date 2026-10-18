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
4x4 texels, producing the 2048x596 working atlas
`../sources/textures/terrain.png`, runtime
`../../content/textures/terrain.json` and `.dds`, and the review
[`lineup.png`](lineup.png). Ordinary builds need neither System.Drawing nor
Python; they stage only the DDS and JSON.

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
| `hill_wide` | 1536x320 | two rolling bg_far humps, outline on top only, flat opaque bottom row |
| `hill_small` | 768x192 | one bg_far hump with a low shoulder, same horizon rules |
| `tree_round` | 128x224 | round bg_mid canopy on a one-cell bg_line stick trunk |
| `tree_tall` | 96x256 | tall faceted bg_mid canopy on a stick trunk |
| `house` | 384x352 | bg_mid gable roof and walls, two bg_detail windows with a bar, a door with a knob |
| `cloud_big` | 256x96 | two bg_detail bumps and a small one on a long flat base |
| `cloud_small` | 160x64 | two bumps on a flat base |
| `sun` | 224x224 | bg_detail disc, twelve bg_line rays, a flat little face; not yellow |
| `birds` | 96x32 | two bg_line gull strokes |
| `shadow_0`..`shadow_3` | 64x32 | shadow_deep specks dense in the top two rows, shadow specks thinning to none in the last; columns 0 and 15 empty so tiles abut in any order |
| `swatch_<role>` | 4x4 | one opaque block per palette role |

The palette is Muted Day, the 26 roles in
[the art rules](../../../docs/polish-plan.md#1-establish-the-art-and-display-rules).
The build checks the role list and order against that table.

`lineup.png` shows every frame at three pixels per cell, the four shadow tiles
abutted in two orders to check the seams, every frame again at one pixel per
cell, and the swatches, all on the paper colour. The DDS is premultiplied RGBA8
UNORM with one mip. Frame order and coordinates are fixed, and rebuilding the
same source produces byte-identical PNG, JSON, DDS and lineup.
