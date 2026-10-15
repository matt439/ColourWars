# E painter source

`painter.json` is the editable production drawing. Its `.`, `K`, `W` and `T`
cells mean transparent, black, white and team mask. A cell is 4x4 texels;
the real 13x30 drawing expands directly to the existing 52x120 player canvas.
No filtering or image generation is involved. The accepted references are
[`p2_figures_scene.webp`](../references/style-board/img/p2_figures_scene.webp)
and [`p2_figures_sheet.webp`](../references/style-board/img/p2_figures_sheet.webp).

Run `powershell -ExecutionPolicy Bypass -File scripts/build-painter-art.ps1`
from the repository root. Windows System.Drawing and the retained texconv build
`../sources/textures/painter.png`, `../../content/textures/painter.json` and
`../../content/textures/painter.dds`, plus the authoring preview `lineup.png`.
The DDS is premultiplied RGBA8 with one mip. PNG colour-space metadata is omitted
to preserve authored UNORM channel values. Runtime stages only DDS and JSON.
Before writing outputs, the build checks that `presentation.json`'s 4x4 respawn
pen paths reproduce every black texel of idle plus rest arms exactly. A source
edit that changes this outline must update the presentation recipe together;
mismatch errors report missing and extra black texel counts.

The 512x2048 atlas has exact 52x120 frames. Neutral strips start at (4,4),
(4,132), (4,260), (4,388) for idle, eight-frame run, jump and fall. The paint
strips have identical rectangles offset by 512 in y. Neutral and paint alpha
are complementary: a `T` cell exists only in paint, all other opaque cells only
in neutral. Run remains 0.2 seconds per frame by default; runtime can retain its
existing 0.05-second override. Other locomotion strips use 0.05 seconds. All
original locomotion and accessory rectangles and texels are unchanged.

Five dedicated actions follow the original atlas area. Their neutral strips
start at x=4 and paint strips at x=116, each with two contiguous frames. The
eight-texel gap between neutral and paint keeps their visible art separate.
The editable `actions` object contains full 13x30 drawings and frame timing;
the build emits matching neutral and `_paint` strips with `looping: false`.
Runtime expires an action after its two frames and returns to locomotion.

| Strip | First y | Seconds per frame | Authored gesture |
|---|---:|---:|---|
| `takeoff` | 1028 | 0.05 | Flared bib and bent push-off knees, then tucked feet above the baseline; open mouth. |
| `land` | 1156 | 0.05 | Wide contact squat and squeezed eyes, then straighter recovery legs. |
| `fire` | 1284 | 0.04 | Leaning bib and asymmetric braced legs, then recoil recovery; clenched mouth. |
| `hurt` | 1412 | 0.06 | Sideways lower-bib shift and one lifted foot, then an opposite wince/stumble. |
| `elimination` | 1540 | 0.07 | Crossed eyes with a buckling leg, then both knees fold beneath the flared bib. |

These are deliberately full-body phrases. Repeated accepted shots repeat the
braced firing gesture while physics and the muzzle continue to use their existing
positions. Action timing and moving-fire readability still need human review.

Each body frame reserves four transparent texels on both horizontal edges,
providing eight texels between visible art in contiguous animation frames.
There are eight texels between strip rows and four at atlas edges. Rest arms
are separate at (68,4), 52x120. Bobble and antenna frames are (128,4) and
(148,4), fork and flag are (168,4) and (188,4), all 12x12. The white 4x4
pen frame is (208,4). Accents use native size 12x12 and offset (16,-12) above
the body; their bottoms touch the top of its cap. All four teammate ordinals
can therefore have different neutral head silhouettes.

The neutral white box head occupies x=8..44, y=16..48 with its face interior
starting y=20. The flat black brim is y=12..20, cap crown y=0..12. The bib
outline occupies x=8..40, y=52..88, with white shirt above the team fill and
one white 4x4 fill miss. Its shoulders stay at (12,60) and (32,60) across all
poses. The cap and head outline stay fixed across actions so above-head accents
keep their root; expressions, lower bib and legs carry the gesture. The build
checks those fixed rows and all horizontal transparent gutters. Idle/run feet
reach the y=120 baseline; jump tucks its feet eight pixels above it. Mirroring
uses the full 52-pixel canvas. Arms are absent from body
strips so runtime connected aiming arms replace the separate rest arms cleanly.

## Respawn pen recipe

These axis-aligned pen paths reproduce the black cells of idle plus rest arms
exactly with a 4x4 nib. Points are nib top-left positions. Closed 8x8 eye paths
have nonzero segments and fill the square eyes without a special dot primitive.
The cap has two horizontal rows of brim, deliberately drawn as separate paths.
The runtime presentation owns drawing timing, colour and these data paths.

| Part | Points in order |
|---|---|
| cap | (8,12), (8,0), (32,0), (32,12), (44,12) |
| cap | (8,12), (44,12) |
| head | (8,16), (40,16), (40,44), (8,44), (8,16) |
| eye | (16,24), (20,24), (20,28), (16,28), (16,24) |
| eye | (28,24), (32,24), (32,28), (28,28), (28,24) |
| mouth | (20,36), (28,36) |
| neck | (16,44), (16,48) |
| neck | (28,44), (28,48) |
| body | (8,52), (36,52), (36,84), (8,84), (8,52) |
| arm | (8,56), (4,56), (4,84), (8,84) |
| arm | (36,56), (40,56), (40,84), (36,84) |
| leg | (12,84), (12,116), (8,116), (20,116), (20,88) |
| leg | (28,88), (28,116), (40,116), (36,116), (36,88) |

`lineup.png` is an authoring check for all 21 poses in all five supported team
hues, plus native and 28-pixel samples of all four accents and 28-pixel samples
of every action frame. Runtime captures and human
acceptance at playing size remain separate evidence.
