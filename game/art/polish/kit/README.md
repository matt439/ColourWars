# Editable workshop pixel study

`source.json` is the authoring source: one character in a row is one native
texel, with an explicit small material palette. Edit the rows and palette, then
run `scripts/build-workshop-kit.ps1 -StageRuntime` from the repository root. The packer emits
the PNG, premultiplied RGBA8 DDS, Labrador frame JSON and attachment/layout JSON.
No image service, resampling or private art file is needed to rebuild this kit.

These are newly authored pixel shapes informed by the retained workshop concept,
not crops or a reduction of that image. The source and complete generated atlas
stay here. `-StageRuntime` also copies only the DDS and engine frame JSON into
`game/content/textures/` for the playable comparison. It accepts only this
canonical source. Without that switch the builder leaves runtime content alone.
The comparison is an inspectable candidate, not the accepted replacement
character, complete animation set or arena.

The candidate density is **four world units per texel**. The 13 by 30 painter
therefore matches the current 52 by 120 collision rectangle. One-texel outlines,
small material clusters, a cap, overalls and work boots establish the silhouette.
The cap and overalls share the team layer; hardware, face and gloves stay neutral.
Sprayer paint and roller sleeve use the same separate grayscale team layer.
The base and team pixels are disjoint, so no hue extraction is involved.

The first nine sprites retain the original static composition. The playable
candidate adds eight body frames: `painter_idle_0`, `painter_idle_1`,
`painter_run_0` through `painter_run_3`, `painter_air` and `painter_land`.
Each has its own complementary `_team` frame. The brighter cap band, shoulder
trim and belt, asymmetric cap fleck and side-profile face are the personality
and dark-team readability changes under review. The run frames change the
overalls as well as the boots; a static uniform mask cannot follow them.

All candidate body frames keep the 13 by 30 canvas and pivot (6.5, 15).
Shoulder anchors are (8.5, 11.5) and (4.5, 11.5), mirrored with body facing.
The body contains the shoulder caps but leaves forearms to the runtime's
two-segment arm bridges. `glove` is a neutral 3 by 3 grip overlay. Neutral
7 by 8 `badge_1` through `badge_4` use cream outlines and numbers so identity
does not depend on team hue. Runtime uses player/view identity for the number.

All coordinates in `source.json` are measured from the sprite's top-left texel
edge. Pivots and semantic anchors may be fractional even though the artwork is
grid-exact. The sprayer is 22 by 12 texels, with grip at (2, 6) and muzzle at
(21.375, 6). This preserves the current 77.5-world-unit muzzle distance. The
roller is 28 by 22 texels with grip at (2, 7) and drum centre at (20.75, 7),
matching its existing 75-unit contact-centre distance. Its sleeve is authored
diagonally to be horizontal at the game's fixed 45-degree right-facing pose.
These anchors preserve geometric relationships; they do not prove that every
rotated pixel or the whole sleeve matches the gameplay contact volume.

The playable tool comparison adds five independently authored sprayer directions:
`sprayer_pose_0`, `sprayer_pose_45up`, `sprayer_pose_90up`,
`sprayer_pose_45down` and `sprayer_pose_90down`. These are native-grid drawings,
not rotated or resampled copies of the horizontal image. Each records `grip`,
`support_grip` and `muzzle`; the muzzle is always 19.375 native texels from the
grip. The zero-degree pose also supplies the continuous-rotation comparison.
Quantization belongs only to the visual pose: runtime anchors the selected
pose's muzzle at the actual simulated muzzle, then bridges both body shoulders
to its two grips. Between bins the grip moves to preserve projectile origin;
review that movement and the arm stepping at playing size.

`roller_grounded` is drawn directly at the game's fixed right-facing angle,
with a horizontal sleeve and stepped handle. Its 32 by 22 canvas has grip
(3.5, 3.5), contact (16.758252147248, 16.758252147248), support grip (6.5, 6.5)
and sleeve bottom at y = 21. With the current standing body and weapon offset,
that bottom lands on the player's foot plane. Mirroring the frame handles left
facing. The original diagonally authored roller remains available in the static
study as a record of the previous edge treatment.

All new frames set `engine_pivot: true`. The builder emits their native pivot
as the engine frame origin, identically on base and team layers. The original
nine sprites omit this flag and keep the old zero-origin frame contract because
the static study passes its pivots explicitly. Source/layout coordinates remain
native texel coordinates; playable presentation attachment offsets are world
units relative to that pivot.

Draw tool artwork at its own native size times four. Do not stretch it into the
old weapon sizes: those sizes also determine the gameplay muzzle and are not
integer multiples of this grid. Left-facing UV flips need the mirrored sprite
pivot to keep the grip attached. The study measures those relationships using
the actual weapon calculations without changing them.

The platform is a repeatable 128 by 48-world-unit timber/concrete/steel module.
Wall, shelving and tank use a darker, quieter palette. Platform upper edges
remain straight; decorative depth does not author collisions or scoring faces.
The puff is a single static shape study, not a replacement for the movement
feedback prototype. `pixel` is the probe's solid-colour utility frame.

Run `scripts/check-workshop-kit.ps1` to check deterministic rebuilding, source
validation, disjoint team masks, gutters, native origins and the emitted texture
contract. Its default canonical-source check also compares the staged runtime
DDS/JSON byte-for-byte with a fresh build. Custom-source checks do not inspect
runtime content. The canonical check also compares presentation body/glove/badge
sizes, shoulders, tool muzzle/support offsets and aim angles against the source
texels and anchors, independently of runtime attachment calculations. It rejects
five deliberately stale presentation variants. The fractional-alpha fixture exercises both origin policies;
malformed source must fail without replacing previous artifacts.
From an x64 VS Developer shell, `scripts/check-workshop-style.ps1` renders the
kit through the real camera and renderer. See the evidence and remaining
decisions in [the style study](../../../../docs/polish-style-kit.md).
