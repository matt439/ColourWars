# Tool Props terrain, shadows and backdrop (`POL#9`)

Implemented 2026-10-18. This page owns the terrain kit's rules, its runtime shape and
what of `POL#9` remains. The art rules it follows are the accepted
[Paint.exe rules](polish-plan.md#1-establish-the-art-and-display-rules) (Tool Props
terrain, airbrush shadows, Zoom 400%) and the author's Muted Day palette. The picture
it was built to is the world the author chose that palette in, in the
[box-head builder](https://claude.ai/artifact/47ANdGfpipyeFa1W527cJq): drawn
stationery on tiled ground, airbrush speckle under floating platforms, and a crude
drawing of hills, trees, a house, clouds, a sun and birds behind.

## What it looks like

Every collision object in the four shipped arenas wears a **material**, a drawing of
a piece of stationery that fills its collision shape exactly:

| Material | Worn by | Drawing |
|---|---|---|
| `ground` | solid structures; the big paintable floors | Halved-bond tile courses with black joints in the top four courses, `ground_shade` below. Paintable ground has a light `ground_top` lip under its top keyline; solid ground does not. |
| `pencil` | long paintable platforms and walls | `pencil_body` with a shaded lower facet; an eraser and ferrule at the end away from the stage's mirror line, a cut wood end with its graphite core toward it. A pencil across the mirror line is cut at both ends; one standing up has its eraser at the top; one shorter than four times its thickness (or 60 units) has no end caps. |
| `eraser` | tall paintable blocks (King of the Hill's tiers) | `eraser_fill`, a keylined `eraser_shade` lower third and one shine dash below the paint band. |
| `paint_tin` | King of the Hill's summit | Metal tin side on: rim, `prop_accent` label band, shaded side. |
| `ruler` | jump-through platforms and fences | Light `ruler_fill` with tick marks rising from the lower edge (short every 16 units, long every 80). |
| `set_square` | ramps | A `metal_light` triangle stepped in whole cells inside the collision triangle, its engraved inner triangle drawn as a line, not a hole. |

Behind the terrain, each arena's decorations are a `paper` sky over the whole
out-of-bounds rectangle, rolling `bg_far` hills on the floor line, two kinds of tree,
a house, clouds, a sun with a flat little face and two flights of birds, all from the
`terrain` sheet. Under every exposed underside of a flat platform an airbrush shadow
hangs: baked speckle tiles, dense `shadow_deep` near the platform thinning to
`shadow`.

## Rules

- **One cell is four world units**, as the painter's. Keylines are one cell. Interior
  detail (ticks, tile joints) sits on the world grid so neighbours line up; outlines
  sit on the exact edges, so sizes that are not whole cells lose part of an interior
  cell, never an outline. Miniature overview, monitor and stage-preview views use
  the pixel treatment below when a cell projects below one pixel; gameplay keeps
  the authored drawing.
- **The drawing never leaves the collision shape.** No overhang, no hole in a
  walkable, no tip beyond the rectangle (the pencil's cut end is the honest version of
  its sharpened one). If it is black-lined you can stand on it; backdrop props are
  lined in `bg_line`, never black, and never look solid.
- **Outlines, lips and end caps go only on exposed faces.** The same exposed-interval
  test that decides where paint can land (`paint_face_geometry.h`) decides where
  terrain is outlined, so abutting and stacked pieces (King of the Hill's castle, the
  Turbulence divider's ledge) read as one surface rather than a seam.
- **Thin rulers keep one keyline.** Under five cells thick the lower (or right) edge
  stays open, so a 10-unit spawn ceiling is a black line over a light board, not a
  black bar.
- **Material fits type.** `ground` on solid or paintable structures, `pencil`,
  `eraser` and `paint_tin` on paintable ones, `ruler` on jump-throughs, `set_square`
  on ramps only. The game, the editor and the schema refuse anything else
  ([level format](level-format.md)). An object without a material keeps the legacy
  stretched frame.
- **Hues live in texels; tints stay grey.** Every JSON `colour` is still a grey (the
  terrain's and backdrop's are `WHITE`); Muted Day's muted hues reach the screen only
  through the `terrain` sheet's swatch frames and backdrop drawings, which the art
  build refuses to contain any colour outside the palette. No team hue appears in the
  scenery ([backstory](backstory.md)).
- **Shadows land on the backdrop only.** They are plain scene objects drawn before
  every structure, so terrain and paint always cover them. One object per exposed
  underside, each culled by its own extent.

## Runtime

- [`TerrainMaterial`](../game/objects/terrain_material.h) is a small hierarchy, in
  the game's OOP grain: the base resolves the swatch frames and replays a quad list;
  `OrientedTerrainMaterial` lays its drawing along the long axis; `GroundMaterial`,
  `PencilMaterial`, `RulerMaterial`, `EraserMaterial`, `PaintTinMaterial` and
  `SetSquareMaterial` each work their drawing out once, at construction, into an
  immutable list of swatch quads. Ordinary drawing only replays it. The explicit
  miniature path projects that same drawing for each view without mutating it.
  A `Structure` owns its material; `Structure::draw` paints it instead of the
  stretched frame.
- [`AirbrushShadow`](../game/objects/airbrush_shadow.h) lays the four baked speckle
  tiles along a span on the world grid, choosing a variant by grid index and cropping
  the last tile rather than stretching it.
- `LevelObjectBuilder` reads `material`, computes each object's exposed faces and the
  stage's mirror line (pencils point their erasers away from it, so mirrored halves
  stay mirrored), and builds the shadows. Level and the stage-select preview both
  take the shadows from it, so the preview shows the same arena.
- The [`terrain` sheet](../game/art/terrain/README.md) is built by
  `scripts/build-terrain-art.ps1` from `game/art/terrain/terrain.json`.
- The opening and reveal's monitor palette uses those same swatch frames with a
  WHITE tint; its colours have no second RGB list in presentation data.

## Checks

`scripts/acceptance/terrain.cpp` builds every shipped stage and requires that every
collision object has a material fitting its type, that every quad lies inside the
collision rectangle (and, for ramps, inside the triangle), that two builds draw the
same quads, that each object stays within a quad budget, that every shadow hangs
under a real underside inside its cull bounds, that every backdrop is untinted on
paper, and that the material table refuses what it does not draw. Level-format and
editor acceptance cover the field ([level editor](level-editor.md)).

The terrain case also checks miniature projection at native and subpixel scales,
negative/fractional camera origins, exposed seams, light thin rulers, surviving
major marks and unchanged ramp pixel occupancy. These are geometry/raster rules,
not human style acceptance.

## Decisions made while building it

These are calls the author has not yet seen; each is easy to reverse.

- **Tool Props over the warehouse brief.** The plan's warehouse direction predates the
  2026-10-09 switch to Paint.exe. The kit follows the accepted Tool Props rule and the
  mockup the author chose Muted Day in. The paint workshop and supply warehouse remain
  Close Quarters' *subject*: its own backdrop and landmarks are fitted with its
  blockout (`POL#17`).
- **Which material each existing piece wears**: solids are ground, jump-throughs are
  rulers, long paintables pencils, tall ones erasers, the King of the Hill summit a
  paint tin.
- **The one-cell keyline**, matching the figure, with thin rulers keeping only the top
  one.
- **Paint covers the outline.** The paint band (12 units, keyline at its inner edge)
  sits over the top of the material, so a painted top's outer edge is the team colour
  against the sky. In the 2026-10-18 shots that reads clearly for every pair checked;
  if yellow or green on paper proves weak in play, the alternative is to redraw the
  outer keyline over the paint.

## Monitor palette coherence (2026-10-18)

The monitor's old row of 15 greys is replaced by the kit's 26 role swatches,
followed by the one empty slot the story is about. `opening.palette_sheet` and
`opening.palette_frames` in `presentation.json` name the atlas and the ordered
frames. The face and highlight roles deliberately remain separate white cells,
as they are in the kit. A later palette edit changes the shared atlas texels.

The row keeps square cells and visible gutters, shrinking together to fit the
program's width. The picker pair, including its panel and outline, stays inside
the picture above the row. The cursor follows these fitted targets through the
opening and reveal; saving fills only the final slot. The existing picture,
glass, opening/reveal timing and gameplay cameras retain their layout.

Startup checks the sheet and each indexed frame before a match can use it,
requiring positive square swatches with zero authored origins. Missing, empty or
wrongly typed fields and missing resources report their presentation keys.

Validation on 2026-10-18:

- Debug and Release build under `/W4 /WX` and pass all 36 consumer cases. The
  story/audio case also checks the shipped role order, malformed fields, missing
  resources, displaced/nonsquare frames and valid custom palettes.
- Both focused D3D11 monitor matrices pass on the NVIDIA GeForce RTX 5080 at
  640x480, 1024x768, 720p, 1080p and QHD, plus pushed framing. Actual pixels match
  all 26 Muted Day cells in order, with square cells and visible gutters. Preview
  and saving retain every existing swatch; the picker and panel remain contained.
  Existing 2P/4P opening/reveal/winner/draw, solo bypass, cursor targeting,
  reduced-effects and frozen raster/state/RNG checks pass; Debug also preserves
  replay fingerprints.
- Isolated Debug and Release startup pass with the accepted bank, a system-only
  PATH and graceful shutdown. `git diff --check` passes.

The retained [story preview at 720p](images/polish-terrain/monitor-palette-story-720p.png)
shows the row with the arena; native-size palette/picker crops are retained for
[1280x720](images/polish-terrain/monitor-palette-1280.png) and
[640x480](images/polish-terrain/monitor-palette-640.png).
Local evidence is under `out/validation/monitor-palette/` and `out/acceptance/`.
Renderer run IDs are Debug `cab6344d19ea4df6a077b7f879b996e6` and Release
`40276642ba4c4c4e865b8bbce23d5c95`. Human gameplay/style review remains below.

## Quiet cloud drift (2026-10-18)

The existing 23 clouds now drift slowly behind the four arenas. Each follows a
bounded horizontal cycle, with a 32- or 48-unit excursion and a 48–72-second
period. Different starting phases and periods keep neighbouring clouds from
moving in lockstep. Position changes in four-world-unit cells, preserving the
drawn grid. The atlas, palette and collision silhouettes are unchanged.

`Visual` entries opt in through the level's optional `drift` object
([format](level-format.md#decorative-drift)). The authored rectangle is the
stationary anchor. [`DriftingVisual`](../game/objects/drifting_visual.h) owns a
bounded cosmetic clock; Scene advances it once per world update, independently
of how many views draw it. Draws only read the chosen position. Pause, count-in,
the opening and the ending freeze it with the world. Rebuilding the stage starts
from its authored phase, and stage previews retain that initial pose.

Reduce effects immediately draws each cloud at its stationary anchor, including
when changed while paused. Its clock stays frozen while reduced and resumes
from the held phase when detail returns. Culling includes both drift extremes,
the anchor, authored frame origins and view-pixel quantization. Drift has no
collision, paint, audio or gameplay-randomness role.

The runtime, editor and schema check the optional fields. The editor's existing
numeric inspector can edit them; undo/redo and save/reopen retain them and
unknown metadata. A plain `Visual` without `drift` remains stationary.

Validation on 2026-10-18:

- Debug and Release build under `/W4 /WX` and pass all 36 consumer cases. The
  terrain case covers every shipped cloud, deterministic restart and phases,
  full-cycle bounds, four-unit snapping, large/nonpositive/nonfinite ticks,
  RNG independence, live reduction and named invalid-content diagnostics.
- Both editor document suites and hidden launch/selection/close checks pass.
  Drift checks cover schema agreement, numeric/type limits, float conversion,
  transactional inspector edits, history and metadata-preserving save/reopen.
- `scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -DriftOnly` and its
  Release counterpart pass on the NVIDIA GeForce RTX 5080. Real D3D11 cloud
  rasters match the old static art at zero phase, move to distinct extremes
  and return exactly after a cycle at 1P/4P 720p, 1080p and QHD. Direct draws
  agree with Scene when only a drifting edge enters the view. Projected
  bounds, immediate reduction, held phases, frozen redraw/RNG and resize
  round trips pass. Actual arena captures preserve simulation observations
  and Debug replay fingerprints during drawing.
- Isolated Debug and Release startup pass with the accepted bank and a
  system-only PATH. The 55-file Release ZIP builds; all four packaged levels
  byte-match their sources, required runtimes and accepted audio are present,
  and no development files are packaged. Its extracted copy also passes
  isolated startup and graceful shutdown. `git diff --check` passes.

Retained native-size evidence includes the
[cloud/culling crops at 720p](images/polish-terrain/cloud-drift-culling-720p.png),
Close Quarters [early](images/polish-terrain/cloud-drift-cq-early-720p.png) and
[12 seconds later](images/polish-terrain/cloud-drift-cq-later-720p.png), and a
[4P 1080p sky composition](images/polish-terrain/cloud-drift-cq-4p-sky-1080p.png). The last
fixture explicitly places the real players above the platforms so the normal
cameras show clouds; it establishes no route or jump reachability. These are
art/renderer checks; human motion/readability review remains open.

Local evidence is under `out/validation/cloud-drift/` and `out/acceptance/`.
Final renderer run IDs are Debug `d47fffce9a4342f3ade57904128a1741` and Release
`3b239ecb30a345f3bce5c803adfe3f1d`.

## Small-view terrain readability (2026-10-18)

The overview, monitor picture and Stage Select preview keep useful exposed
terrain strokes when the four-unit drawing cell projects below one pixel.
The old drawing lost some keylines and ruler marks because both projected
edges of their quads truncated to the same pixel. Changing the sampler would
not recover a quad with no area.

Materials now record which quads are exposed strokes or ruler marks while
constructing their unchanged authored drawing. The miniature path grows an
exposed stroke inward to one view pixel, clipped to its own filled extent.
Buried faces remain unlined. Ramp strokes stay within their original stepped
row bands, retaining the projected silhouette rather than inventing a ledge.
Incidental brick joints and engraved interior lines keep their ordinary
projection; the treatment does not widen everything drawn in black.

A one-pixel ruler stays light. Two pixels can carry one leading keyline and
a light body; larger rulers can retain their opposite edge and marks. Marks
need three pixels of spacing to remain separate, so major marks survive after
minor marks become unresolvable. No palette, atlas, collision shape, paint
budget, gameplay camera or world-grid value changes.

`TerrainMaterial::paint_miniature` takes the view's camera explicitly. At one
pixel per cell or above it calls the ordinary painting path. Smaller drawings
use viewport-local integer rectangles and restore the caller's camera; their
point sampler remains unchanged. Each paintable structure still draws its
material, splashes, owned paint and runoff in that order. Normal active and
countdown gameplay continues through `Scene::draw`.

The pinned engine supplies neither view context to object drawing nor a
replacement draw callback inside Scene's ordered world walk. Level therefore
uses a small sequential traversal for overview/monitor views, retaining Scene's
viewport, append offset, culling and scenery-before-collision order. Scene still
owns every object. A borrowed scenery list includes shadows, idle drips and
later debug station labels; live collision objects come from Scene. Its public
collision span does not promise render order, so parity with the pin is a
tested dependency. Private [engine draft 20](review/engine-reports.md#20-scene-drawing-gives-client-objects-no-view-context---draft-with-miniature-terrain-exhibit)
records what an engine answer would let the client delete. It is unfiled and
still needs publication approval and an upstream-master recheck.

StagePreview uses the same explicit-camera structure methods in its existing
pane, with its existing camera and decorative object order. Drawing remains
const and consumes no gameplay randomness. Human readability and style review
remain part of `POL#9`.

Retained native-size crops from a 1280x720 window compare the Close Quarters
Stage Select preview [before](images/polish-terrain/small-view-cq-legacy-720p.png)
and [with the miniature treatment](images/polish-terrain/small-view-cq-adapted-720p.png).
The [four-player monitor picture](images/polish-terrain/small-view-monitor-720p.png)
shows the same treatment in the opening. These captures support renderer review;
they do not replace readability review in play.

Validation on 2026-10-18:

- Debug and Release build under `/W4 /WX` and pass all 36 consumer cases each.
  The terrain case independently checks known exposed/masked edges, touching
  ground, horizontal/vertical ruler marks and light bodies, ramp occupancy,
  every shipped material, fractional/negative projection and invalid cameras.
- `scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -MiniatureTerrainOnly`
  and its Release counterpart pass on the NVIDIA GeForce RTX 5080. Each records
  379 comparisons; both CSVs and all 46 retained capture pairs are byte-identical.
  Checks include independent pixel oracles, owned-paint dominance, ordinary
  material/Scene parity, culling and append offsets, actual StagePreview panes,
  all four arenas at 1P/4P framing and 720p/1080p/QHD, real story/overview and
  solo-bypass paths, frozen observations/RNG/Debug fingerprints and resize returns.
- Both `-StagePaperOnly` regressions pass, retaining the independent complete
  paper/text oracle, original physical preview pane/camera, all shipped stages,
  Random latching, ordinary destinations, live recovery and reduction checks.
- Both `-MonitorOnly` regressions pass. The original shipped full-drift story
  captures remain. A second identical 2P/4P story uses a private stage copy with
  only five decorative cloud `drift` fields removed; reinserting them must
  reproduce the original JSON. This isolates click accents from the legitimate
  cloud-to-anchor change under Reduce effects. Six before/at/after comparisons
  retain timing, containment, state, draw-RNG and Debug fingerprint assertions;
  41 story records and compact cursor/card crops accompany the original captures.
- Four hidden isolated startups pass with a system-only PATH and per-monitor-v2
  DPI awareness: Debug and Release beside their builds, the extracted Release
  ZIP, and a separate package copy with only its bank removed. All shut down
  gracefully. The 55-file ZIP retains all 28 reviewed runtime assets, accepted
  audio/provenance, runtimes and licences; Release developer RTTI is absent.

Local evidence is under `out/validation/miniature-terrain/`; the consumer runner's
new optional `-OutputDirectory` keeps this batch's CPU evidence separate from
earlier runs. Initial native fixture failures and their diagnoses remain there:
explicit float arguments, capacity for the prefix-view check, the stale monitor
comparison and its newly declared third-party header path were repaired without
changing production after the builds. Miniature/Stage paper results retain exact
source-hash proof that subsequent edits affect only the monitor fixture/runner.
Rendering was muted and may have shared the machine with Unreal validation;
these checks establish no listening, physical-pad or performance acceptance.

| Native scope | Debug run | Release run |
|---|---|---|
| Miniature terrain | `04f6840b7e344f31996eba4d8d8a0363` | `215ddb09e4c243c4b3da7d81eb360c79` |
| Monitor | `384c0f05495744998586d217693486da` | `06ca1dc8100f40a99a0e7dd25a067800` |
| Stage paper | `576301748e6c438db96b4d15368e4c67` | `52e2ed07aebc47748e7cc8a4163e15f0` |

## Reusable static landmarks (2026-10-19)

The kit adds a brush pot and tape roll, each used once in every current arena.
They are quiet background silhouettes in the existing backdrop roles, with
`bg_line` outlines rather than the black edges that identify walkable terrain.
There are no captions, team colours or new runtime types. Both use ordinary
WHITE-tinted `Visual` entries at their authored atlas size: 256x320 and 256x160.
They remain stationary under pause and Reduce effects.

The placements are provisional composition choices for review with the painter
and tools. Each sits behind an existing floor or platform, whose collision and
paint cover the decoration. Close Quarters uses the two lower side regions;
King of the Hill uses the lower castle interior; Turbulence uses the side
chambers; the Movement Course uses a short-gap landing and the right runway.
This adds reusable room details to the existing layouts. Close Quarters' own
composition and fitting to a redesigned blockout remain with `POL#17`.

The atlas appends a new shelf after the original 39 frames. Every old frame's
coordinates and the full original 2048x596 texel region are retained; the
expanded atlas is 2048x920. The recipe and generator remain the editable source,
and the [lineup](../game/art/terrain/lineup.png) includes both silhouettes. The
game, StagePreview and overview/monitor paths use the existing scenery walk.

Validation on 2026-10-19: both builds and all 38 consumer CPU cases per preset
pass under `/W4 /WX`. The terrain case checks actual DDS/frame resolution,
each arena's exact two native-size placements and drawing order, ordinary
static Visuals, conservative fractional-camera culling, restart/time/reduction
invariance and gameplay RNG. Existing material, cloud and miniature checks pass.
Two complete art regenerations produce byte-identical PNG, JSON, DDS and lineup.
The first reproduction exposed desktop-dependent ClearType fringes in lineup
labels; explicit bitmap text rendering fixes those labels without changing the
atlas. Both initial outputs and the original generator are retained as evidence.
An independent byte/pixel proof also passes: removing only the two inserted
decorations restores each original level byte-for-byte, including collision,
spawns, bounds, sound and stations. The previous recipe data, all 39 frame
definitions and every texel in the original atlas region are unchanged. Source
PNG and runtime DDS pixels agree; the two additions use only backdrop roles,
constant four-by-four cells and transparent gutters.

`scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -LandmarkOnly` and its
Release counterpart pass. Each makes 48 clean/heavy paired comparisons, or 96
primary coverage states: all four arenas, three team pairs, 1P/4P pane layouts
and 720p/1080p. These static Scene compositions use each arena's unchanged
authored overview bounds. Actual projectile contacts establish more than 99%
owned paint area; every prop retains visible pixels. Independent raw-atlas draws
replace only the two landmark calls before actual terrain and paint, including
the King of the Hill ruler that visibly crosses the brush handles.

Supplementary checks cover 64 native clipped-edge cases, eight actual StagePreview
size cases and 24 actual Level monitor/overview size cases. Scratch twin stages
omit only the two decorations, with reconstruction checked against the original.
Frozen state, paint attribution, RNG, Debug fingerprints, solo monitor bypass,
static Reduce effects behavior and resize returns pass. Each preset records all
1,100 reference/repeat render readbacks separately from its primary case count,
and retains exactly eight compact clean/heavy crops. All 12 CSV/PPM files are
byte-identical between presets; the contact sheet was inspected. These are
scripted composition and renderer checks. Human style/route readability,
listening, physical pads and uncontended performance remain unaccepted.

Both editor document suites and their three hidden launch/selection/close
checks per preset pass. A fresh 55-file Release ZIP retains all 28 reviewed
runtime assets, accepted audio/provenance, runtimes and licences, with developer
RTTI absent. Four isolated system-only-PATH startups pass with per-monitor-v2
DPI awareness and graceful shutdown: both builds, the extracted ZIP, and a
separate bank-free package copy. No existing artifacts or source bank were
deleted. Local receipts, source snapshots and initial failure evidence are
under `out/validation/terrain-landmarks/`.
The evidence-only helper's initial PowerShell compatibility failures are retained
alongside its final passed proof; they changed no game content.

## Not done (what `POL#9` still holds)

- Human review in play, clean and heavily painted, with all three team pairs, at 1P
  and 4P (part of locking the style with `POL#7` and `POL#14`).
- Author review of the reusable brush pot and tape roll in the current arenas;
  signage and captioned Paint-artefact jokes need the author's wording. Further
  room life and modest parallax remain choices after camera framing.
- Close Quarters' own backdrop and fitting the kit to its blockout (`POL#17`); a
  palette and composition per arena (`POL#16`).

## Ready implementation slices (2026-10-18)

The small-view treatment and reusable static landmarks above are implemented.
The current-arena parallax comparison remains below; retain checks and evidence
for it. Final human style
acceptance remains separate; no slice unlocks the phase-2 blockouts by itself.

| Slice | Scope | Acceptance | Size |
|---|---|---|---|
| Modest parallax prototype | Compare distant/middle backdrop layers with the static version in the current arenas. Establish the per-view draw approach before broadening the art. | Independent split-screen views, conservative culling, resize and overview/monitor transitions remain correct; define static preview/editor behaviour. Keep drawing free of simulation/RNG mutation and retain a static Reduce effects presentation. | L |

The small-view treatment and static landmarks are ready for author review. The
parallax prototype is a further current-phase choice, not a request to finish
all scenery before reviewing it.
Retain parallax only if the comparison improves the composition and readability;
the static backdrop remains a valid outcome of the prototype.
Captioned jokes need the author's wording and do not gate non-text landmarks.
Close Quarters' own composition and fitting the kit to revised geometry stay
with `POL#17`; final per-arena production stays with `POL#16`/`POL#21`.
