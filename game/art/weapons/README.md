# Paint.exe tool source

[`weapons.json`](weapons.json) is the editable production drawing for all five
tools. It follows the accepted
[`p2_figures_sheet.webp`](../references/style-board/img/p2_figures_sheet.webp):
black outlines, neutral white/grey hardware, simple worn materials and a round
end-on roller. The painter's coarse, code-native drawing is the companion source.
The original [`weapon_paint_source.png`](../weapon_paint_source.png) is retained
as historical generated artwork; the production recipe no longer reads it.

Run `powershell -ExecutionPolicy Bypass -File scripts/build-weapon-art.ps1`
from the repository root. Windows System.Drawing rasterises integer world-unit
vectors without antialiasing, then expands each pixel to an exact 4x4 source
block. The script writes the working `../sources/textures/weapon_paint.png`,
runtime `../../content/textures/weapon_paint.json` and `.dds`, and the authoring
[`lineup.png`](lineup.png). Ordinary builds stage only the runtime DDS/JSON.
Python and image generation are not needed to edit or reproduce the artwork.

The source draws each pose's `underlays`, then `body`, then `overlays` in order.
Operations are `["rect", material, x, y, width, height]`, the same arguments for
`"ellipse"`, `["poly", material, x0, y0, x1, y1, ...]`, and
`["line", material, width, x0, y0, x1, y1, ...]`. Coordinates are inside the
unchanged world canvas, measured from its top-left; the aim line is half its
height. `K`, `W`, `G`, `D` and `B` are neutral black, white, grey, dark grey and
wood. `T` and `H` are exact permanent-paint material IDs, converted to white and
darker grayscale texels. `U` and `V` identify reservoir liquid and become the
same grayscale values. Source magenta and cyan never enter runtime colour.

Every tool has `<tool>`, `<tool>_motion_1`, `<tool>_motion_2` and `<tool>_lift`,
and every frame has a matching `<frame>_paint`. The base and paint layers have
disjoint opaque texels: each material pixel belongs to exactly one layer.
Runtime team tint therefore does not thicken edges or colour hardware. The
sprayer and mister additionally have `<tool>_supply` strips. Their empty
reservoir interiors are opaque neutral `D` in the base; liquid overlays the
bottom rows according to actual ammunition. Permanent wet nozzles remain in
`<frame>_paint`. The bucket's wet opening and roller coating remain materials;
the sniper dial represents pressure release and recovery.

The sprayer strip has seven frames (zero to six filled world rows), the mister
eight (zero to seven visible rows after hardware occlusion). Frame zero is
transparent; the last is full. Each frame retains the complete tool canvas,
with no authored origin. Using the identical destination, grip origin, rotation
and flip for all layers avoids independent camera-space edge rounding. These
are manually selected quantity levels, not a timed animation. The recipe
rejects any pose whose visible supply mask differs from idle, so shared liquid
cannot cover moving hardware. The [runtime record](../../../docs/polish-tools.md#reservoir-paint-supply)
describes selection and validation.

| Tool | Source canvas | World canvas | Grip points, relative to left-centre pivot | Authored secondary motion |
|---|---|---|---|---|
| Sprayer | 280x120 | 70x30 | (4,8), (22,4) | Sagging hose bows under pressure, upper valve depresses then settles. |
| Sniper | 400x120 | 100x30 | (8,8), (32,4) | Pressure needle snaps across its dial while a small piston retracts and recocks. |
| Roller | 400x200 | 100x50 | (0,0), (16,0) | Sleeve wear/index rotates in three phases; contact compresses around the fixed axle. Lift exposes the raised bracket. |
| Mister | 240x120 | 60x30 | (4,8), (18,4) | Pump and outlet valve depress, release and return above a bulbous tank. |
| Bucket | 240x240 | 60x60 | (4,-8), (16,-8) | Wire bail swings above a fixed hand bar; wet fill tilts and recovers around the dented tin. |

All poses keep the same source dimensions and left-centre origin. The three
firearms end on the existing local aim line at `x = world width`. Existing
projectile nozzle offsets remain beyond those terminals. The roller drum axle
is exactly local `(75,0)` in every pose, agreeing with its existing contact
origin; compression never translates it. The grip regions remain opaque at
the `presentation.figure.grips` points. No tuning, collisions, launch positions,
coverage or ammunition values are part of the art recipe.

The 2560x1280 atlas packs all 40 material frames and both supply strips with
four transparent texels around each packed rectangle. Supply frames retain
the body's transparent canvas borders. The `random`, `error`, `pixel` and `nozzle` utility frames
are exact copies from the historical player/projectile source. The DDS is
premultiplied RGBA8, one mip; PNG colour-space metadata is omitted so texconv
preserves authored UNORM bytes. Atlas packing and JSON frame ordering are stable.

`lineup.png` shows all four poses enlarged at two pixels per world unit, followed
by neutral and all five supported team hues on grey. Its smaller samples use
the actual 720p gameplay scale, `720 / presentation.camera.reference_height`
(currently 2/3), for all pane counts including four players. The E body alongside
each set is therefore 80 pixels tall, rather than the painter source sheet's
28-pixel stress samples. Runtime perspective, aiming, hand overlap, arena
composition and human readability acceptance remain separate checks.
The final rows also compare full, half and empty reservoirs in all five hues
at the same native gameplay scale.
