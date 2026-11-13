# Painter source

`painter.json` is the editable production drawing. Its `.`, `K`, `W` and `T`
cells mean transparent, black, white and team mask. A cell is 4x4 texels. The
drawing is the author's 2026-10-18 build from the
[box-head builder](https://claude.ai/artifact/47ANdGfpipyeFa1W527cJq) (a private Claude artifact): an
ear-stub head, a backwards team cap, tall dot eyes, a smirk, centred overalls
with team straps, short team sleeves with white hands and tube legs, with no
teammate mark ([art rules](../../../docs/polish-plan.md#1-establish-the-art-and-display-rules)).
No filtering or image generation is involved.

The canvas is 17x30 cells (68x120 texels), centred on the unchanged 52x120
hitbox. The middle 13 cells span the hitbox. The two cells either side are
visual overhang for the brim, which runs three cells behind the head, and for
the front hand. Runtime widens the hitbox rectangle by `figure.body_inset`
(8 texels) on each side before drawing (`FigurePresentation::canvas_rectangle`).
Collision, physics and combat feedback keep the hitbox; drawing and culling
use the whole canvas.
A builder cell is one canvas cell, one column and one row in: the builder's
15x29 composite plus a gutter column each side and a blank top row.

Run `powershell -ExecutionPolicy Bypass -File scripts/build-painter-art.ps1`
from the repository root. Windows System.Drawing builds
`../sources/textures/painter.png`, `../../content/textures/painter.json` and
the runtime copy `../../content/textures/painter.png`, plus the authoring
previews `lineup.png` and `slips.png`.
The PNG has straight alpha; the engine's PNG loader premultiplies it into RGBA8
with one mip as it loads. PNG colour-space metadata is omitted to preserve
authored UNORM channel values. Runtime stages only the PNG and JSON.
Before writing outputs, the build checks that `presentation.json`'s 4x4 respawn
pen paths reproduce every black texel of idle with the rest arms drawn over it.
A source edit that changes this outline must update the presentation recipe
together; mismatch errors report missing and extra black texel counts.

The 1024x2048 atlas has exact 68x120 frames. Neutral strips start at (4,4),
(4,132), (4,260), (4,388) for idle, eight-frame run, jump and fall. The paint
strips have identical rectangles offset by 512 in y. Neutral and paint alpha
are complementary: a `T` cell exists only in paint, all other opaque cells only
in neutral. `locomotion_frame_times` in the source owns these timings: Run is
0.2 seconds per frame by default; runtime can retain its existing 0.05-second override.
Other locomotion strips use 0.05 seconds. Idle is a one-frame strip, with no
duplicate standalone neutral `idle` frame; `idle_paint` remains the uniform
mask frame referenced by tuning.

Five dedicated actions follow the locomotion. Their neutral strips start at
x=4 and paint strips at x=148, each with two contiguous frames. The eight-texel
gap between neutral and paint keeps their visible art separate. The editable
`actions` object contains full 17x30 drawings and frame timing; the build emits
matching neutral and `_paint` strips with `looping: false`. Runtime expires an
action after its two frames and returns to locomotion.

| Strip | First y | Seconds per frame | Authored gesture |
|---|---:|---:|---|
| `takeoff` | 1028 | 0.05 | Eyes up, flared bib and bent push-off knees, then tucked feet above the baseline; open mouth. |
| `land` | 1156 | 0.05 | Wide contact squat and squeezed flat eyes, then lowered eyes and straighter recovery legs. |
| `fire` | 1284 | 0.04 | Squinted eyes and a wide clenched mouth, then the recovery face; idle's legs and hips in both frames. |
| `hurt` | 1412 | 0.06 | Sideways lower-bib shift, one squeezed eye and a frown with one lifted foot, then the opposite wince. |
| `elimination` | 1540 | 0.07 | Crossed eyes and a frown with a buckling leg, then both knees fold beneath the flared bib. |

These are full-body drawings, but `fire` changes only the face so repeated
shots never redraw the legs. Runtime shows it only on a standing body and holds
its second frame under sustained fire; physics and the muzzle continue to use
their existing positions. Action timing and moving-fire readability had no human
review: the finish review that was to look (`POL#24`) was waived on 2026-11-05.

Each frame reserves four transparent texels on both horizontal edges, giving
eight texels between visible art in contiguous animation frames. There are
eight texels between strip rows and four at atlas edges. The cap (rows 0-4),
the head's bottom edge, neck, body top and the straps under the sleeves
(rows 11-15) are identical in every action; the build checks them and all
horizontal gutters.
Idle/run feet reach the y=120 baseline; jump tucks its feet eight texels above
it. Mirroring uses the full 68-texel canvas, whose centre is the hitbox centre.

Arms are not in the body strips. Two arm layers sit beside idle, each with a
neutral frame and a `_paint` frame 512 lower: `rest_arms` (76,4), drawn over the
body while no tool is held, and `sleeves` (148,4), the same two team sleeves
without hands, drawn over the roots of the live aiming arms while one is. The
build checks that every sleeve cell matches the rest arms. Runtime tints the
paint layers with the team colour, or the story's grey. The aiming arms are thin
black lines from shoulders under the sleeves (`figure.rear_shoulder` and
`front_shoulder`) to white, outlined hands on the tool's grips. The white 4x4
pen frame is (220,4).

The neutral white head occupies cells 4..12, rows 4..11, with ears at cells 3
and 13 on rows 7-8. The brim runs from cell 1 to 12 on row 4, the cap crown
rows 1-3. The tall eyes are cells 6 and 10 on rows 6-7 (`figure.quiet.eyes`),
the smirk cells 7-9 on row 10 with cell 9 raised on row 9. The neck is row 12
and the overalls cells 4..12, rows 13-21, with the straps in cells 6 and 10
on rows 14-15 above the bib and the leg split at cell 8, row 20.

## Figure slips

[slips.json](slips.json) holds the 11 face and head-outline slips the author
picked for stage 2 of the drawing mistakes
([research, decision 8](../../../docs/drawing-mistakes-research.md#9-author-decisions-and-open-questions)).
Each match's person draws one or two of them, chosen from the match's hand.
`presentation.json` `figure.slips` lists the slips in the same order, with the
exclusive pairs and each slip's respawn stroke edits.

The build checks every person: each slip alone, and each pair that is not
exclusive. Every pose must keep the head outline closed to a 4-connected bucket
fill, with no transparent pocket. Idle must keep a white row between the eyes
and the brim. Each person's strokes must trace their black cells exactly. The
build then bakes each person's 44x24 face band (rows 5-10, cells 3-13) for idle
and the ten action frames. A band is named
`face.<person>.<strip>.<frame>`, for example `face.front_ear_low+eyes_close_set.hurt.1`;
every locomotion strip draws `face.<person>.idle.0`. Identical bands share a
rectangle. They are packed on a 52x32 pitch right of the action strips (from
x=288, y=1028) and in the empty block under them (from y=1668). `slips.png` is
the authoring preview: every person's idle in blue.

## Respawn pen recipe

These axis-aligned pen paths reproduce the black cells of idle with the rest
arms over it exactly, with a 4x4 nib. Points are nib top-left positions in the
68x120 canvas. The `eye` part is also where the story's eye acting finds the
eyes. The runtime presentation owns drawing timing, colour and these data paths;
the table follows `presentation.json.respawn_redraw.strokes` in index order.

| Stroke index | Part | Points in order |
|---:|---|---|
| 0 | cap | (20,4), (44,4) |
| 1 | cap | (4,16), (48,16), (48,8) |
| 2 | cap | (16,8), (16,16) |
| 3 | head | (12,28), (16,28) |
| 4 | head | (48,28), (52,28) |
| 5 | head | (12,32), (16,32) |
| 6 | head | (48,32), (52,32) |
| 7 | head | (16,44), (48,44), (48,20) |
| 8 | head | (16,20), (16,44) |
| 9 | eye | (24,24), (24,28) |
| 10 | eye | (40,24), (40,28) |
| 11 | mouth | (28,40), (36,40), (36,36) |
| 12 | neck | (28,48), (28,44) |
| 13 | neck | (36,48), (36,44) |
| 14 | body | (16,52), (48,52) |
| 15 | body | (16,84), (48,84), (48,64) |
| 16 | body | (16,76), (16,84) |
| 17 | body | (32,80), (32,84) |
| 18 | arm | (12,72), (16,72) |
| 19 | arm | (52,72), (56,72) |
| 20 | arm | (8,56), (8,68) |
| 21 | arm | (56,56), (56,60) |
| 22 | arm | (60,64), (60,68) |
| 23 | leg | (16,116), (28,116), (28,88) |
| 24 | leg | (36,116), (48,116) |
| 25 | leg | (20,88), (20,116) |
| 26 | leg | (36,88), (36,116) |
| 27 | leg | (44,88), (44,116) |

`lineup.png` is an authoring check for all 21 poses in all eleven supported team
hues from `presentation.json.team_palette.colours`, each with the rest arms
over it as the game draws them, plus 28-pixel samples of every action frame.
Runtime captures and human acceptance at playing size remain separate evidence.
