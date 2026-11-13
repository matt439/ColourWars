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
4x4 texels, producing the 2048x2536 working atlas
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
| `press_roof_bay` | 600x96 | Press Tower: one sawtooth roof tooth in section, a paper-glazed upright with a cross mullion, a bg_mid roof slab falling to the right, a bottom chord and two truss webs; tiles edge to edge |
| `press_chimneys` | 176x72 | Press Tower: a distant bg_far gable and two chimneys, one capped, meant to show only above a roof slope |
| `press_frame` | 640x680 | Press Tower: the screw press, two riveted bg_mid columns under an arched crosshead, a nut, a bg_far threaded screw, a tilted capstan bar and four knee braces |
| `press_trestle` | 144x320 | Press Tower: a two-legged bg_mid trestle, cross-braced in bg_line, hung under a stair tread |
| `press_sign` | 160x256 | Press Tower: a blank bg_mid board a little out of true on two bg_line rods; mid-tone so it never reads as a HUD panel, and no lettering |
| `press_hook` | 96x288 | Press Tower: a crane trolley, a bg_line chain and a hook block |
| `press_rolls` | 160x120 | Press Tower: three bg_far paper rolls stacked end-on with paper cores |
| `yard_skyline` | 1024x224 | Slab Yard's distant works in bg_far: a sawtooth hall with one tooth dragged a cell low, two chimneys, a parapet block, a gable shed, two silos and a curved-roof warehouse with a thin stack; bg_mid windows and a shuttered door, no lettering |
| `yard_tank` | 192x384 | bg_far water tank with a leaning cone roof, porthole, ladder and band on three stilts with one crooked cross brace |
| `yard_crane` | 512x864 | bg_far gantry crane: open lattice girder (two thin chords, zigzag between, sky through), A-frame legs with X braces and bogies, a cab by the right leg and the trolley parked left with a short hook, well above the play band |
| `yard_stack` | 160x448 | bg_far exhaust stack: coolie hat, flanged duct with a branch elbowing down, plant box with a bg_mid louvre panel; nothing blows |
| `yard_crates` | 288x224 | three bg_far crates on a pallet, one X-braced, one planked with a bg_mid sack slumped on it, one dropped on top a little skew |
| `yard_fence` | 512x112 | chain-link panel: three bg_mid posts, sagging bg_line top wires and a quiet bg_far diamond mesh; posts sit on the ends so panels overlap one post |
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
