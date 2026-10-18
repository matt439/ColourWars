# Paint.exe painter: production artwork and action poses

Implemented 2026-10-15, beginning `POL#7`, and redrawn to the author's own
build on 2026-10-18 ([below](#the-authors-build-2026-10-18)). This carries the accepted
[figure E and Zoom 400% rules](polish-plan.md#1-establish-the-art-and-display-rules)
into the game. The follow-up adds dedicated action poses to that foundation;
the reference arena and the composition's human review remain open. The
[five-tool production pass](polish-tools.md) subsequently replaces the legacy
weapons and adds their mechanisms while retaining these grip anchors.

## Figure and attachments

The figure keeps the existing 52 by 120 world-unit collision rectangle. Its
editable source uses a 13 by 30 cell canvas, expanded four times with nearest
sampling: black pencil outlines, a white box head, square eyes, a flat mouth,
a flat cap and team-coloured overalls. Neutral artwork and paint masks are
separate, aligned layers. Each gait frame has its own matching mask, so the
trouser tint follows the legs. Idle, eight-frame running, jumping and falling
clips retain the existing animation timing.

Runtime uses the engine's point sampler and the existing resolution-independent
camera. The four-times expansion describes the source grid; the camera supplies
the display scale at each resolution and pane size. Continuous aim and fractional
world placement retain their existing behaviour.

Black-and-white bobble, antenna, fork and flag accents distinguish up to four
teammates without introducing another team hue. Their choice uses each team's
roster order, independent of keyboard and pad input slots and of team interleaving.
Accents appear above the cap, mirror with the figure and are included in visual
culling.

Hands follow the existing tools' continuous aim and authored grip offsets.
Arms draw behind the tool and white, outlined hands draw over it. Hidden tools
and whole figures in the story reveal use resting arms. These attachments do
not move the simulation muzzle, change shot cadence or alter collisions.

The respawn pen traces the new idle artwork before the new fill appears.
The story's eye acting uses the new white face and eye positions. Respawn,
camera and audio timing are unchanged.

## Reproduction and review

Run `powershell -ExecutionPolicy Bypass -File scripts/build-painter-art.ps1`
from the repository root. The retained source, recipe and working PNG live
under `game/art/`; only `painter.dds` and its frame JSON enter runtime content.
Normal builds stage the reviewed outputs without invoking the art recipe.

Run `scripts/check-painter.ps1 -Preset x64-debug` from an x64 Visual Studio
Developer shell for real renderer captures and frozen-draw/culling checks.
The runner isolates its save and writes evidence under `out/validation/painter/`.

Foundation validation on 2026-10-15:

- Debug and Release build and pass all 11 consumer acceptance cases each.
  The movement case also rejects a mismatched body/paint strip and a zero
  animation frame time with an authored-key diagnostic. Existing course,
  ownership, audio, solo-story and replay checks pass.
- Both builds pass the real D3D11 painter probe on the NVIDIA GeForce RTX 5080:
  one/four players, all aim quadrants and both facings at 720p, representative
  diagonals at 1080p/QHD, all three team palettes, all five tools, pause, four
  same-team accents and sparse input slots. Repeated frozen draws produce
  identical pixels and preserve simulation state; Debug also compares replay
  fingerprints. Isolated raster output stays within projected culling bounds.
- Death, partial outline, fill, respawn, tool-free squash and dead whole-figure
  story poses are captured. The art recipe checks all 2,336 black idle/rest-arm
  texels against the authored pen paths before writing outputs. PNG, JSON, DDS
  and the source lineup regenerate byte for byte; the DDS matches premultiplied
  source pixels and has one mip level.
- Runtime staging regression passes. The 51-file Release ZIP contains the
  byte-matching painter DDS/JSON and excludes authoring PNGs and tools.
  Isolated Debug and extracted Release startup pass with the accepted bank;
  extracted Release also passes separately without it, with the single expected
  silent-bank diagnostic. Startup uses a system-only PATH and graceful close.

Full evidence is under `out/validation/painter/` and `out/acceptance/`.
Retained gameplay captures show [one player at 1080p](images/polish-painter/one-player-1080p.png),
[pink/green at four-player 720p](images/polish-painter/four-player-pink-green-720p.png),
[blue/yellow](images/polish-painter/four-player-blue-yellow-720p.png) and
[all four teammate accents](images/polish-painter/four-teammates-720p.png).
These captures retain the current arena and weapon artwork; composition/style,
movement feel, physical-pad and listening acceptance remain with their TODO owners.

## Action poses

The second production increment (2026-10-15, `POL#7`) adds takeoff, landing,
firing, hurt and elimination clips, each with a matching team-paint mask.
The source retains the four-texel cell grid, black outlines, neutral head and
fixed head-accent and shoulder anchors. The poses change the drawing inside
the existing canvas; collision rectangles and continuous tool aim stay exact.

Takeoff responds to a successful jump, including a buffered landing jump.
Landing uses the same genuine airborne-to-supported transition as the contact
puff: spawn settling and ground seams stay quiet. A new takeoff interrupts
landing or firing recovery. Every action leaves movement and firing responsive;
no control waits for an animation to finish.
Firing follows a paid shot before launch-obstruction retirement; an empty tank
or a held button during cooldown cannot invent another shot. Automatic fire
lets each short brace reach its recovery frame. The shared body phrase covers
all five tools; their individual mechanisms and secondary motion are implemented
in the [tool pass](polish-tools.md).
Hurt takes visual priority over the other live actions and follows accepted
damage, independently of the rate-limited hurt sound.

Elimination briefly draws the defeated figure where it fell, with its tool
hidden, before the existing respawn pen takes over. The redraw and respawn
schedule, camera hold, sounds and scoring are unchanged. The pen fill and a
figure drawn whole in the story use the idle artwork the pen traces.
Actions advance during simulation updates; drawing and additional player views
cannot restart or age them. Pause freezes them with the match.

Action validation on 2026-10-15:

- Debug and Release build and pass all 11 consumer acceptance cases each.
  The movement case covers both action frames, buffered takeoff, a rapid
  low-ceiling landing, spawn/seam suppression, all five paid-shot paths,
  sustained automatic fire, cooldown and empty tanks, accepted/ignored damage,
  elimination and respawn. Changing only action durations leaves movement,
  health, ammunition and projectile launch results identical through an
  interrupted jump/fire/hurt/death/respawn sequence.
- Both builds pass the real D3D11 painter probe on the NVIDIA GeForce RTX 5080.
  The added action matrix captures both frames of all five actions at 720p,
  with both single-player facings and all three four-player palettes. Pinned
  tool-free body rasters distinguish the frames. Repeated frozen draws are
  byte-identical, preserve observed simulation/action state and, in Debug,
  preserve replay fingerprints. Rendered pixels stay inside culling bounds.
  Early elimination and later idle produce identical whole-dead story figures.
  The existing 720p/1080p/QHD aim, teammate, tool and redraw matrix also passes.
- The art recipe reproduces PNG, JSON, DDS and lineup byte for byte. All
  original atlas pixels and rectangles remain unchanged; the added strips
  extend the atlas to 512x2048. The DDS remains premultiplied RGBA8 with one mip,
  and the idle pen still matches all 2,336 authored black texels.
- Isolated Debug and Release startup pass with the accepted bank, a system-only
  PATH and graceful shutdown. These checks establish no listening acceptance.

Evidence is under `out/validation/painter-actions/` and `out/acceptance/`.
The retained [action contact sheet](images/polish-painter/actions-720p.png)
contains native-size renderer crops; its labels identify pane count, palette,
facing, action and frame. The source [lineup](../game/art/painter/lineup.png)
also includes every action in all five hues and 28-pixel authoring samples.

## The author's build (2026-10-18)

The author rebuilt the figure from parts in the
[box-head builder](https://claude.ai/artifact/47ANdGfpipyeFa1W527cJq) and chose
the Muted Day level palette ([art rules](polish-plan.md#1-establish-the-art-and-display-rules)):
an ear-stub head, a backwards team cap, tall dot eyes, a smirk, centred overalls
with team straps, short team sleeves with white hands, and the existing tube
legs. Teammates have no head accent, because they already share the team
colour. This supersedes the figure, accents and canvas described above; the
records above are kept as written. The [source README](../game/art/painter/README.md)
has the current layout and pen recipe.

The author settled four fit questions the same day:

- **Canvas.** The build is 15 cells wide, so the canvas grows from 13x30 to
  17x30 cells (68x120 texels) with a gutter either side. It stays centred on the
  unchanged 52x120 hitbox, and the two outer cells each side are visual only.
  `figure.body_inset` gives the overhang, and `Player::figure_canvas` widens the
  hitbox before the body, arms, hands, respawn pen and story eyes draw.
  Collision, combat feedback and the quiet character's breathing keep the
  hitbox, and its lean still scales by hitbox texels.
- **Aiming arms.** While a tool is held, the team sleeves stay at the shoulders
  (a `sleeves` frame and its paint layer) over thin black forearms
  (`arm_width` 4) to 2x2 white, outlined hands on the grips (`hand_size` 16).
  At rest the `rest_arms` frame, now with a team paint layer, is drawn over the
  body with the front hand one cell forward.
- **Smirk.** The raised cell sits directly above the line's right end, so the
  respawn pen can draw it as one L stroke.
- **Ears.** They sit as built, on the eyes' lower row and the row below.

The action poses keep their gestures, legs and timing, redrawn on the new face
and body: raised, squeezed, lowered and crossed eyes; open, clenched and
frowning mouths; and the same flared or leaning lower bib. The accents, their
three `presentation.json` keys, the roster-ordinal choice in `PlayerBuilder` and
the `Player` constructor's `teammate_index` are removed. The respawn pen now
traces 2,112 black texels in 28 strokes.

Review follow-ups in the same change: the story's sweat drop sits just clear of
the front ear on the wider canvas, and the bucket and mister rear grips move
back four texels so the larger hands stay separate. The hurt ticks still
anchor to the hitbox, so on the rear side they briefly cover the brim tip; that
is left for the `POL#7` review.

Validation on 2026-10-18:

- Debug and Release build, and pass every consumer acceptance case and the
  isolated startup check.
- The Release painter probe passes its full real-D3D11 matrix on the NVIDIA
  GeForce RTX 5080. A Debug run passed 673 checks before reaching the script's
  600-second limit, which the full Debug matrix already exceeded before this change.
- The art recipe validates the pen against all 2,112 composite black texels,
  checks gutters, baselines, the fixed action rows and that the sleeves match
  the rest arms, and regenerates the lineup.

Captures: [four same-team painters at 720p](images/polish-painter/rebuilt-four-teammates-720p.png)
and [native crops, doubled](images/polish-painter/rebuilt-figure-crops.png)
(aimed sprayer and roller, the respawned figure, the tool-free story pose, fire,
hurt and landing).

## Remaining character work

`POL#7` still owns the human review of character/teammate readability in single- and
four-player gameplay. It runs in the current arenas now; the reference arena
(`POL#17`) is a re-check when it lands, not a precondition (author, 2026-10-17). The [five-tool artwork and motion](polish-tools.md)
now use these hand attachments. The [quiet-detail increment](polish-plan.md#tool-personality-and-quiet-detail)
adds independently phased breathing, blinks, weight shifts and acceleration/turn settling.
Terrain, paint, effects and UI keep their
existing owners and order.
