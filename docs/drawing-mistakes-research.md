# Deliberate drawing mistakes: research

Investigation, 2026-11-01. Nothing is decided and nothing in the repository was changed. This page asks whether ColourWars should add deliberate drawing mistakes to its art, because the art can look "too perfect" for a world drawn by a person with a mouse ([backstory](backstory.md)). It ranks what reads as too perfect, lists the mistakes people really make, compares ways to add them and plans the human tests. **[E]** marks evidence found in the repo or in a cited source. **[I]** marks inference. External papers were read as abstracts or summaries unless stated otherwise.

## 1. Facts that set the scale

- **A cell is bigger on screen than first assumed.** One art cell is 4 world units. The display scaling record ([polish-plan.md](polish-plan.md#display-scaling-record)) gives these sizes [E]:
  - 2 px per cell at 4P 720p, so the 30-cell figure is about 60 px tall, not about 28 px.
  - 3 px at 4P 1080p.
  - 4 px at 1P and 2P 1080p and at 4P QHD.
  - **8/3 px** at 1P and 2P 720p, and **16/3 px** at 1P and 2P QHD.
- **At the fractional scales, a one-cell mistake is not stable.** One cell draws 2 px wide in some places and 3 px in others. It can change width as the camera moves, which reads as flicker. Display policy is still open as `POL#34` [E]. Any mistake therefore has to be judged at an 8/3 layout as well as at the integer layouts [I].
- **The figure canvas is 17x30 cells.** The middle 13 columns are the hitbox. The two columns on each side are overhang, and columns 0 and 16 must stay empty ([painter README](../game/art/painter/README.md), `build-painter-art.ps1:86,114`) [E].
- **The figure is not perfectly symmetric now** [E, from `painter.json`]:
  - the cap brim sticks out on one side only (torso row 4);
  - row 9 has one off-centre cell at column 9, where the centre is column 8;
  - the front hand sits one pixel forward ([polish-plan.md](polish-plan.md) §1).
- **The figure is mirrored when it faces left** (`SpriteFlip` in `player.cpp`). A mistake on one side therefore jumps to the other side whenever the player turns [E].

## 2. Verdict

**Yes, but narrowly, and mostly by removing skill rather than adding noise.** Confidence is medium-high on the direction and low on which mistakes the figure should get. Only the human tests can settle the second.

Most of what reads as "too perfect" is not too geometric. Paint's Rectangle, Ellipse, Line and Curve tools, Ctrl+drag stamps, Flip and colour replace all make exact shapes. Identical repeats are a real Paint mark ([classic-paint-research.md](classic-paint-research.md), lines 182 and 202) [E]. The real tells are these [I, from a visual audit of the lineups and `docs/images/`]:

1. Rendering that is more skilled than the person who drew the box-heads.
2. Repetition that shows no sign of having been placed by hand.
3. Output Paint could not make, such as anti-aliased type and alpha.

The approach follows from that:

- **Use:** frozen mistakes in whole cells, authored once, that a person using Paint 98 at 400% could plausibly have made and kept.
- **Start with:** the backdrop props and the brick pattern. Leave the figure for later.
- **Never:** runtime boil, or procedural noise on outlines, paint edges or collision.

The fiction already covers this choice. "Every person in every match draws the same box-headed figures. It's how everyone draws a person when they have a mouse and no talent" ([backstory.md](backstory.md#why-everyone-draws-the-same-box-heads)) [E]. Each match is a different person, but they all draw alike. So the only figure mistakes that fit the canon are **common** ones, made by many people. That is exactly what a study of common errors measures [I].

## 3. What reads as too perfect now (ranked)

| # | Tell | Where | Visible at 4P 720p | How strongly it contradicts the story |
|---|---|---|---|---|
| 1 | Volume rendering on backdrop props: `bg_far` highlight lines and an inner rim ellipse inside `bg_mid` bodies | `brush_pot`, `tape_roll` in [terrain.json](../game/art/terrain/terrain.json) | Medium: large (the pot fills about 1/5 of a pane behind the centre plinth) but low-contrast | High: more skilled than the box-heads standing in front of it |
| 2 | Bricks: exact half bond, equal courses, every joint one exact spacing apart | [terrain_material_drawings.cpp](../game/objects/terrain_material_drawings.cpp) `GroundMaterial` | High as an area, but a 1-cell change is only 2 px of a 64 px brick | Medium-high: rectangle bricks are believable; perfect placement is less so |
| 3 | Anti-aliased type, and alpha tints, in the HUD and menus | HUD cards, wordmark, Results, menu labels; `CrudeDrawing` translucent tints (`crude_drawing.h:14-16`) | High in menus | Medium. The HUD is already inside the 28-colour budget (`classic-paint-research.md:220`), and an anti-aliased ramp adds colours that are not in it. Whether Paint 98 text was anti-aliased is recorded as unknown (`:159`, `:286`) |
| 4 | Rendered tool detail (sprayer scope, sniper dial, roller highlight) | [weapons lineup](../game/art/weapons/lineup.png) | Low at 4P, visible at 1P | Medium. The bucket is already wonky and already right. |
| 5 | Clean hill curves, a round `tree_round`, a symmetric house | `terrain.json` backdrop | Medium, low contrast | Low-medium |
| 6 | Identical figures across frames, seats and teams | `painter.json`: 22 shared torso rows, one mask tinted per team | High as a subject | Low. Same figure for everyone is canon (`backstory.md`, "Why everyone draws the same box-heads"), and so is the same figure on both teams (one person previews both halves, `:13-19`) |
| 7 | Mirrored stage | `docs/images/polish-crossfeed/overview-clean-720p.png` | Medium | Low. The geometry must stay mirrored for fairness. |
| 8 | Four airbrush shadow tiles repeating | [airbrush_shadow.cpp](../game/objects/airbrush_shadow.cpp) | Low | Low |
| 9 | Square droplets and flat puddles in impact frames | [paint-fx](../game/art/paint-fx/) | Low | Low |

**Already hand-made, so keep them:**

- the scribbly action legs, the hurt and elim faces and the one-sided brim;
- the bucket;
- the lumpy paint lobes;
- the sun's face and the stepped clouds;
- the airbrush fringe;
- the countdown glyphs (`countdown_art.cpp:12-13`: "The uneven shoulders and feet are intentional").

**Use current screenshots only.** `docs/images/polish-tools/four-player-720p.png` and `polish-paint-effects/four-player-720p.png` show an older figure without the brim, so do not judge the current art from them.

**One fix is due whatever is decided.** `crude_drawing.h:8-10` promises "a shaky hand", but `crude_drawing.cpp` has no jitter, so the comment should say what the code does.

## 4. Which mistakes are real

No study counts mouse-in-Paint errors directly [E, absence]. The evidence comes in three kinds:

- **Motor:**
  - Dragging is less accurate than pointing, on every device ([MacKenzie et al., CHI'91](https://www.yorku.ca/mack/CHI91.html)).
  - Cursor paths are a ballistic movement followed by corrections ([HKUST](https://researchportal.hkust.edu.hk/en/publications/structure-of-handmouse-movements/)).
  - For tracing, the mouse is slower and more error-prone than a stylus ([Zabramski](https://vvise.iat.sfu.ca/pubs/zabramski2013easy)).
- **Perceptual:**
  - Copying errors are mostly misperception rather than hand control ([Chamberlain & Wagemans review](https://eprints-gro.gold.ac.uk/20010/1/Chamberlain%20%26%20Wagemans%20-%20Errors%20in%20drawing%20pre%20formatting%20.pdf)).
  - The [oblique effect](https://en.wikipedia.org/wiki/Oblique_effect): diagonals are copied less accurately than horizontals and verticals.
- **Paint itself:**
  - 3 undos before Vista;
  - Ctrl+drag stamps and Shift+drag trails ([Wikipedia](https://en.wikipedia.org/wiki/Microsoft_Paint));
  - fast pencil strokes become chains of straight segments.

Frequencies are not known. Study A exists to find them.

### Candidates (all whole cells, so all possible in Paint 98 at 400%)

| Code | Mistake | Evidence | Where it may go | Notes |
|---|---|---|---|---|
| SKILL | **Skill ceiling:** remove highlight rendering | [I], visual audit | `brush_pot`, `tape_roll` | Takes perfection out rather than adding error |
| F4 | **Stamp drift:** a whole brick or stamp placed 1 cell off, which leaves a doubled joint or a 1-cell seam | Mode [E]; placement error [I] | Brick interiors, repeated backdrop props | Keeps copies pixel-identical (`classic-paint-research.md:202`). Moving a single joint is different: it makes 31- and 33-cell variants, which are not copies. Say which one is meant. |
| D1 | **Shape misplaced:** a box or ellipse 1 cell too wide, tall or off-centre | Dragging accuracy [E] | Backdrop props | The tool shape is perfect; where it lands is not |
| B1 | **Overshoot or undershoot:** a 1-2 cell tail at a corner | Submovement model [E], size [I] | Prop interiors and backdrop, as `detail` shapes | Never on keylines that define collision |
| B2 | **Near-miss straight line:** a 1-cell kink where Shift was not held | [I] | Backdrop (house edges, hills) | Not on rulers or pencils: they are collision terrain ([polish-plan.md](polish-plan.md) §1) |
| A3/A4 | **Uneven stair-steps and flat facets on freehand curves** | Pixel-art sources [E], oblique effect [E] | Hills, clouds, tree, sun rays | Backdrop only |
| C2 | **Outline pinch:** a pocket that the bucket fill does not reach, made with single-cell keylines | [I] | Inside one backdrop prop only | Ruled out on the figure (§4 table below) |
| G1 | **Overdraw:** an earlier stroke showing under a later fill | 3-undo limit [E], persistence [I] | Inside one backdrop prop only | Fits the story well. Where a prop overlaps terrain it looks like a layering bug. |
| E2/E3 | **Uneven eye height** (vertical) | [I] | Figure face, later | Survives the facing flip. A side-to-side lump does not. |

"Eyes too high, head too short" ([Carbon & Wirth 2014](https://www.experimental-psychology.de/ccc/docs/pubs/CarbonWirth2014.pdf)) is about realistic faces. **It is dropped.** A box head has no anatomical eye line, the eyes already sit high (rows 6-7, under the row-4 brim), and the chin row 11 is protected by the action invariant (`build-painter-art.ps1:117-123`) [E]. Pseudoneglect (a small left bias) is also dropped as a reason for figure asymmetry, because the flip turns any left-side mistake into a facing feature [I].

### Ruled out, or allowed only with a named exception

| Mistake | Why |
|---|---|
| Sub-cell wobble, anti-aliased wobble | "Every edge stays a hard cell" (`classic-paint-research.md:205`). It also disappears when reduced to cells. |
| Line boil, per-frame variation | Drawn things may move as wholes: figures breathe, blink and lean, and clouds drift. A stroke's pixels change only when something paints them. Boil breaks that rule [I]. It also fights flicker sensitivity ([Eurographics](https://diglib.eg.org/handle/10.1111/v30i8pp2367-2386)), the Reduce effects mode and frozen-draw identity. |
| Any edit to collision outlines, landing edges, or the outline of rulers and pencils | `polish-plan.md:579-581`: collision silhouettes stay truthful. |
| Mistakes on scored paint edges, primer or owned paint | Score is painted area, and paint stays inside each exact scored rectangle (`polish-paint-effects.md:24-30`). Supporting research: sketchiness worsens area judgement ([Wood et al. 2012](https://openaccess.city.ac.uk/id/eprint/1274/)) and reads as uncertainty ([Boukhelifa et al. 2012](https://www.aviz.fr/Research/UncertaintySketchy)). Abstracts only. |
| Any mistake in or next to team-tint `T` cells, or any that differs by team | Team fairness and team-cue readability |
| **All figure body mistakes** (pockets, side lumps, leg or strap asymmetry) | Body rows 13-21, cap rows 2-3 and every leg column are `T` or border `T`. A white pocket in the body looks like the `FFFFFF` hit highlight (`presentation.json`, `combat_feedback`). It nearly disappears on yellow or azure and shows as a hole on blue or forest, which is unfair between teams. |
| Mistakes that pull the drawn silhouette in from the hitbox | Hits would land on empty air at the head top, feet or sides. |
| Eyes moved up against the brim | At 2 px a cell, the eye merges into the brim and the blink lid (`eyelid_height` 4) has nothing separate to close. |
| Open keyline with a kept fill leak, in gameplay layers | The keyline must be closed (`classic-paint-research.md:204`). In the backdrop, at most a few cells inside one prop, never across a pane split, where it would look like a clear-colour bug. |
| Wrong-colour fill | Costs no palette slot if it reuses a colour, but breaks "no team hues in the scenery" (`backstory.md:31`) and risks reading as team paint. |
| Doubled keyline weight | Keylines are 1 cell |
| Stray dots in or near the play space | They read as droplets or projectiles |
| Shift-drag trail | Looks exactly like an uncleared framebuffer. Banned. |
| Squeezed brick at a block edge | Joints keep 3 cells clear of the outline, "or they read as a doubled edge" (`terrain_material_drawings.cpp:113`). End bricks are already partial, because blocks have arbitrary widths. |

## 5. How each asset family could take mistakes

- **Backdrop props** ([terrain.json](../game/art/terrain/terrain.json)):
  - A prop is a list of `fill` shapes (`rect`, `ellipse`, `poly`, `line`, `path`) plus `detail` shapes. The build draws the outline as a one-cell union of the fill, so the outline is closed by construction.
  - A lopsided cloud or a kinked hill is a change to fill vertices. Overshoot, overdraw and pocket mistakes have to be `detail` shapes.
  - `cells` cannot grow: frame positions are fixed (`build-terrain-art.ps1:220-224`).
  - Variant frames are cheap. New frames go on a fresh shelf after the old atlas, as `brush_pot` and `tape_roll` did, plus a count change at `:238`.
- **Bricks** (`GroundMaterial`):
  - The drawing is computed once, in the constructor, so a mistake costs nothing per frame.
  - Courses count from each block's own `rectangle.y`, so there is no world course index.
  - `GroundMaterial` takes no `mirror_x`; only `PencilMaterial` does.
  - The joints are not mirror-symmetric today.
  - A seeded mistake should therefore hash the world y of the course top and the joint's world x, using splitmix with a constant seed, as `AirbrushShadow` does. Apply the shift before the 3-cell clearance test.
  - Mirrored mistakes would need new plumbing, and mirrored drift is itself a sign that the pattern was generated. Joints sit inside a collision outline that stays true [I]. The author decided on 2026-11-01 that decoration need not be equal: drift is neither mirrored nor counted per half.
- **Figure** ([painter.json](../game/art/painter/painter.json)):
  - Editable area: face rows 5-10 and the head-box keyline, away from `T`, torso rows 0-4 and 11-15 (the action invariant), the shoulder anchors (`presentation.json:951-952`) and the grips.
  - Neutral and paint masks are split by the `T` legend, so they stay complementary by construction.
  - A changed outline must also change the respawn pen paths (`presentation.json:989-1040`). The check at `build-painter-art.ps1:128-176` requires an exact black-texel match, with axis-aligned segments on the 4 px grid, so every kink becomes a stair step of extra pen points.
  - An eye move also changes `quiet.eyes` (`:973-976`), the action face rows and the eye pen strokes that `figure_acting.cpp` uses to find the eyes.
  - Nothing checks that a torso mistake is copied into the 10 action frames. Add a check, or state that actions may differ.
  - Probes affected: `quiet_character` and a MattPC `check-painter.ps1` run.
- **Weapons:** vertex edits in `weapons.json`. The author has chosen shade and highlight roles there (`metal_light`/`metal_dark`), so changing them is a separate author question.
- **Monitor and cursor:** `CrudeDrawing` code. A per-match hand would be derived from the existing `MatchRandom::persona()` draw (`match_random.h`), so it stays consistent with the bezel and silhouette.

## 6. Options compared

| Option | Cost | Risk | Fits the story | Where it fits |
|---|---|---|---|---|
| **(a) Authored one-off edits** to shapes and the figure grid | Low for props. Medium for the figure, because pen paths, eyes and action frames change too. | Low. Deterministic, and shared by every seat. | Good: common mistakes made by everyone (`backstory.md`, "Why everyone draws the same box-heads") | Props, backdrop, figure face |
| **(b) Hand-drawn variants chosen per instance** | Low for props (new frames on a fresh shelf, or a `flip` field on `Visual`). High for the figure (variant atlas regions, pen paths, eyes, `RespawnRedrawPresentation`). | Medium. Per-seat figure differences would work as teammate marks, which the author has declined. | Weak for figures: the canon is that everyone draws alike | Clouds, hills |
| **(c) Seeded variation in code or the build** | Low-medium for bricks | Medium. Too much becomes noise (tile-variation practice, [wayline](https://www.wayline.io/learn/tilemaps/3)). Global `CrudeDrawing` jitter would reach 42 files, including HUD, feedback and respawn. | Fine for stamp drift | Brick interiors only |
| **(d) Runtime boil** | High | High | Breaks the rule that a stroke's pixels change only when painted | Nothing in play |
| **(e) Mistakes shown by the story** | Low in the backdrop. Medium in the respawn redraw. | Low-medium | Strongest | A kept mistake drawn identically on every respawn |

**Respawn redraw.** Respawn replays "the strokes the person used to draw it" (`backstory.md:74-76`), and "nobody presses Ctrl+Z" (`:82`). An undone stroke is gone from a linear history, so it cannot be replayed. **The undone wrong stroke is dropped.** Two versions remain:

- **A kept mistake the pen draws every time** (an overshoot tail, say). It needs no new stroke kind, so it goes with the stage 2 figure work.
- **A stroke drawn wrong and then erased with the Eraser.** The eraser stroke is part of the history, so the story allows it. The author declined it on 2026-11-01: the redraw shows no erased strokes.

## 7. Recommendation and stages

**Stage 0: decisions, no art.**

- Answer open decision 4 in [classic-paint-research.md](classic-paint-research.md): is the "real program" test literal or stylistic? It decides whether each mistake must be reproducible in jspaint.
- Consider a **three-layer split** (a proposal; nothing records it):
  - **the picture**, which may take mistakes;
  - **the invented program** (palette row, close button, cursor), which a program would draw perfectly;
  - **the room** (bezel, notes, silhouette).

  The monitor and cursor are drawn in Paint style today ([polish-monitor.md](polish-monitor.md)). If the program chrome looks drawn by a machine, the picture reads as drawn by a hand [I].
- Write the guardrails below into the art rules.

**Stage 1: on a scratch branch.** This doubles as V1 for Study B, and ships only after Study B passes.

1. **Skill ceiling on `brush_pot` and `tape_roll`.**
   - Remove these shapes:
     - from `brush_pot`, the `bg_far` detail lines `12,6…`, `32,5…` and `51,14…`, the inner ellipse `bg_far 13,43,38,10` and the poly `bg_far 17,54…`;
     - from `tape_roll`, the `bg_far` ellipse `4,3,48,28` and the poly `42,23…`.
   - Keep only backdrop roles and no near-black lines, so the props stay lower-contrast than collision stationery and do not read as platforms you can stand on.
   - Leave the stationery shade roles and the eraser shine alone ([polish-terrain.md](polish-terrain.md)).
2. **One authored mistake per backdrop prop:** a lopsided cloud bump, a 1-cell hill kink or an uneven sun ray. These are fill-vertex edits within the existing `cells`, so all 41 frames, coordinates and palette checks stay as they are.
3. **Brick stamp drift.**
   - Hashed whole-brick offsets, or joint shifts, on a minority of interior joints.
   - Keep the 3-cell clearance.
   - Make 1-cell and 2-cell sizes separate Study B variants, because 1 cell may not be visible at 4P 720p.
   - Visibility at 4P 720p is a pass condition, not an assumption.
   - Run the `terrain` acceptance case (`miniature_checks.h` samples touching ground pieces) and `check-painter.ps1 -MiniatureTerrainOnly` on MattPC. CI only compiles the painter checks.

**Stage 1 as built (2026-11-01, branch `drawing-mistakes-research`).** Not
shipped: it waits on Study B.

- **Skill ceiling.** `brush_pot` lost its three brush-head glints, the body
  highlight and the shaded inner rim. Its mouth is now a plain `bg_line` ring
  around a `bg_mid` inside, because removing the inner ellipse alone left a solid
  dark disc. `tape_roll` lost the shade inside its core and the highlight on its
  peeled end. **It keeps its lighter top face**, unlike the list above: that face
  is a flat fill that gives the roll its shape, not a highlight, and removing it
  changes the silhouette.
- **One mistake per prop** ([terrain.json](../game/art/terrain/terrain.json)):
  a flat step on `hill_wide`'s first rise; `hill_small`'s dip a cell too deep;
  both tree trunks leaning, in opposite directions; the house's roof peak three
  cells left of centre; `cloud_big`'s last bump too big; `cloud_small`'s second
  bump dragged wide; one sun ray off level and one short. `birds` already looked
  drawn by hand and is unchanged.
- **Brick slips.** `brick_slip` in
  [terrain_material_drawings.cpp](../game/objects/terrain_material_drawings.cpp)
  moves about one joint in five by one cell, left or right, keyed on the joint's
  world position with splitmix64 and a constant seed. A slip that would break the
  3-cell clearance keeps the joint in place. The 2-cell variant for Study B is
  `BRICK_SLIP_CELLS = 2`, a rebuild.
  **Not yet per person:** the slips are the same every match. Seeding them from
  `persona()` needs the match seed passed through the level builder to the
  materials; that waits for stage 2, which needs the same plumbing for the
  figure.
- **Mortar strokes** (added 2026-11-01, from the author's sketch below). Each
  course line is drawn as several strokes in 40-cell world bands, and about one
  band in three lands a row off, in a direction fixed for that line. Where two
  strokes meet at different rows they overlap by up to two cells or touch only
  at a corner, so a 4-connected bucket fill could not leak through: the
  author's rule for gaps is **diagonal only**. Joints meet the lines where the
  lines were drawn, and about one joint in four runs one or two cells past an
  interior line. A drift or overshoot that would come within 3 cells of the
  outline is not drawn. Same constant-seed keying as the slips, so this is not
  per person yet either.
- **Doubled edges** (added 2026-11-01, the author's second example). Each brick
  is its own rectangle, and a few missed the edge they should have shared, so it
  came out twice: about one joint in eight gets a twin one or two cells to one
  side, and about one brick in ten between two joints gets its top edge again a
  row under the line or two rows under it. A one-cell gap leaves a sliver of
  brick between the two lines. This is F4's "doubled joint". The "doubled
  keyline weight" exclusion in section 4 still holds for outlines.
- **Simpler tools** ([polish-tools.md](polish-tools.md#drawing-and-attachments)):
  highlight strips, shade marks, glints and the roller's drum shine are gone;
  every moving mechanism stays.

**Stage 2: figure, after Study A, and an author decision.** The figure was chosen part by part by the author on 2026-10-18 ([polish-plan.md](polish-plan.md) §1), so any change reopens that choice and should be shown with before and after images.

- At most 1-2 shared face or head-keyline mistakes, the same for every seat and team, for example uneven eye height.
- Each must survive the flip and keep a white row between the eyes and the brim.
- The face must read the same in the hurt and elim states.
- Make the respawn pen draw any kept outline mistake, and update `quiet.eyes`, the action face rows and the eye strokes in the same change.

**Stage 3: after Study B and an author call.**

- Story beats: one small patched leak or overdraw inside a single backdrop prop.
- Per-match hands are decided (section 9); their pool comes from Study A.

**Guardrails for every stage:**

- **Fairness:** the same geometry for both teams; only team colour differs. Any variation is keyed per seat or per match, never per team.
- **Readability:** check at 4P 720p (2 px), at 1P or 2P 720p (8/3, including a camera pan), and at 1P 1080p (4 px).
- **Truth:**
  - collision, landing edges, keyline closure and scored paint rectangles stay unchanged;
  - the drawn silhouette is not pulled in from the hitbox;
  - figure edits stay off `T` and away from cells next to it.
- **Determinism:**
  - byte-identical rebuilds and frozen draws;
  - splitmix with constant seeds, and no gameplay RNG, `rand()`, `System.Random` or time;
  - `review_arena` checksums equal across art variants.
- **Paint rules:** whole cells, no anti-aliasing, within the 28 colours.

## 8. Human tests

No playtest with anyone other than the author is on record ([design-process.md](design-process.md)) [E]. Study B fits inside `POL#24`. Study A needs a new TODO row.

**Kits:** [scripts/studies/](../scripts/studies/README.md) holds both studies ready to run: Study A's reference images, participant and run sheets, consent sheet, codebook and analysis, and Study B's stimulus capture, test page and analysis. Two thresholds were fixed there before any data, beyond this section: `ASYM_MIN_PAIRS = 2` and `PROP_MIN_ERROR = 0.05` (a relative error).

**Pilot (half a day):** the author runs Study A on themselves first, to debug the materials and the timing.

**The author's first sample (2026-11-01).** Before any session, the author drew
a square by hand to show the mistakes they make on lines along the axes:

- a straight edge drawn as **two strokes a row apart**, which WOB codes when it
  survives reduction;
- a line **overshooting** the corner it should stop at (OVER);
- a corner where the two lines **do not meet** (GAP).
- (a second example) a rectangle drawn beside another to **share an edge**,
  which instead comes out as a **double line** (WEIGHT where the lines touch,
  ADD where a sliver separates them).

These are the first entries in the pool. They went into the brick mortar at once
(stage 1, "Mortar strokes" and "Doubled edges"), with corner gaps limited to diagonal touches. Terrain
outlines stay closed because they are collision, and the head box waits for
Study A.

### Study A: what mistakes do people make?

- **Who:** 6-10 adults with mixed mouse skill, at least 2 of whom have never used a pixel editor. Record which hand each person mouses with.
- **Tool:** [jspaint.app](https://jspaint.app), Classic theme. Only the last 3 actions may be undone, on the honour system; code any breach from the recording.
- **Items:**
  - the idle figure (always);
  - one of a 6x3 brick course or the brush pot (Latin square);
  - optionally, two run frames. Watch whether people **copy-paste or redraw**: if most copy-paste, identical frames are authentic.
- **Tasks, in this order:**
  - **D:** draw from a text description only;
  - **C:** copy, with the reference open beside the canvas;
  - **T:** trace over a grey paste of the reference.
- **Conditions:**
  - **Z:** a 17x30 canvas at 400% with the grid;
  - **F:** a 136x240 canvas at 100%, reduced to cells afterwards. Use **any-ink** for black (or skeletonise, then snap). Use majority vote only for fills, because majority vote erases one-pixel lines.
- **Limits and records:** 5 minutes per item, about 35 minutes per session. Keep the final PNG, a screen recording, the undo count, tool choices and any fill leak.
- **Codes:** ASYM, GAP, OVER, UNDER, WOB, WEIGHT, PROP, PLACE, STRAY, FILL, OMIT/ADD and GRID, scored automatically after alignment by overlap (IoU), with Python and Pillow kept outside the repo.
- **Second coder:** codes **all** drawings, because 25% of about 15 is too few for a stable kappa. Target kappa ≥ 0.6.
- **How a result becomes a rule:**
  - A code becomes a **candidate** if it appears in at least 30% of C or T drawings, from at least 3 people, and survives the reduction to cells.
  - Its size is the median, in cells.
  - It still has to pass the section 4 exclusions.
  - At N of about 8, treat the result as a ranking, not as exact rates.
- **Cheap supplement:** Google's [Quick, Draw!](https://research.google/blog/introducing-the-kaggle-quick-draw-doodle-recognition-challenge/) stroke data (CC BY 4.0) gives closure-gap and overshoot statistics, but it does not label mouse versus touch input.

### Study B: is it charming, and is it readable?

This is a screen for gross failures, not a measurement.

- **Who:** 8-12 people who did not take part in Study A.
- **Images:**
  - V0 (current) against V1 (stage 1), V1b (2-cell brick drift), V2 (plus figure candidates) and V3 (heavy).
  - Capture with `+exec review_crossfeed +quit` at seed 7, with `screenshot.strip off`. Identity cues go off for the find tests (`tune player_identity off`, `tune hud.teammate_marker off`).
  - Each window size is its own run (1280x720, 1920x1080), because the script shoots 4/3/2/1P at the current window size. Use its 2P shots for the 8/3 check.
  - Include reveal and overview frames, because the whole picture is visible at once there.
  - Simulation checksums must be equal across variants.
  - Clips: idle, run, **turning around**, a respawn redraw, and a camera pan at 8/3.
  - Fairness: all six palette pairs (`team_colour.h`), both team positions, protan, deutan and tritan simulations, and team-ID crops of figures standing in heavy paint of their own team.
- **Tests, from a local HTML page:**
  1. Paired choice, V0 against each variant, sides randomised: "Which looks drawn by a person?" Free text before any explanation.
  2. **Find the bug.** Participants are told the frame may contain a rendering bug and click where it is.
  3. "Which of these can you stand on?"
  4. Find your player at 4P, and "which team?" on crops.
  5. In-play notes during `POL#24`.
- **Pass rules, fixed before the study runs.** A variant passes only if all of these hold:
  - "Drawn by a person" chosen over V0 in at least 65% of trials, with the **lower confidence bound** above 50%. Read borderline results per participant, because trials are not independent.
  - Find the bug: no click on a gameplay-layer mistake from anyone. A backdrop mistake fails if more than 10% of participants click it.
  - No new standable-object confusions.
  - Team ID shows no drop in any palette or team cell. State the trial count, because detecting a 2-point drop needs about 200+ trials per palette.
  - Find time, median and 90th percentile, no worse than V0 at any seat.
  - Jitter or glitch comments no more than V0's rate plus one participant, with each one traced to a cause.
  - Pick one primary comparison in advance. If a heavier variant only squeaks past, ship the next lighter one.
- **Ethics:** consent sheet, screen-only recording, anonymous IDs, data kept outside the repo. **No participant pixels enter the repo**: use the mistake patterns, not the drawings.
- **Effort:** about 7-9 days in full. The minimum is about 4 days: Study A on the figure only, plus Study B's paired choice, find-the-bug and team-ID tests. Stage 1 needs Study B but not Study A.

## 9. Author decisions and open questions

**Decided by the author on 2026-11-01:**

1. **The three-layer split is adopted:** the picture, the program and the room.
   Menus take no mistakes, because the person did not draw them. Only the picture
   takes mistakes. The HUD and menus belong to the program layer [I: the author
   named menus; the HUD is placed with them by the same reasoning].
2. **Literal.** Every mistake must be one a person could really make in Paint 98
   at 400%, reproducible in jspaint.
3. **Each person has their own mistakes.** The figure design stays shared, as
   [backstory.md](backstory.md#why-everyone-draws-the-same-box-heads) says;
   what varies is the slips. They are chosen per match from
   `MatchRandom::persona()` ([match_random.h](../game/objects/match_random.h)), so
   they agree with that match's bezel and silhouette. One person drew both teams,
   so within a match both teams get the same mistakes, and fairness holds. This
   moves per-match hands from stage 3 into the main plan. Study A now supplies a
   pool of real mistakes to draw from, not one shared set.
4. **Weapons are simplified:** the rendered detail (sprayer scope, sniper dial,
   roller highlight) comes out, to the bucket's level.

5. **The figure is reopened.** Each match's person may add 1-2 face or
   head-outline slips, picked from the Study A pool, under the stage 2 rules.
   The respawn redraw shows no erased strokes: kept mistakes only.
6. **Decoration need not be equal.** Brick drift and other backdrop mistakes are
   neither mirrored nor balanced per half. Collision, landing edges and scored
   paint stay mirrored and true.

Nothing in this research is open now. Stage 1 waits only on being built and on
Study B; stage 2 waits on Study A.

---

Key files: [painter.json](../game/art/painter/painter.json), [build-painter-art.ps1](../scripts/build-painter-art.ps1), [presentation.json](../game/content/presentation.json), [terrain.json](../game/art/terrain/terrain.json), [terrain_material_drawings.cpp](../game/objects/terrain_material_drawings.cpp), [airbrush_shadow.cpp](../game/objects/airbrush_shadow.cpp), [crude_drawing.h](../game/objects/crude_drawing.h), [match_random.h](../game/objects/match_random.h), [review_arena.cfg](../scripts/debug/review_arena.cfg).
