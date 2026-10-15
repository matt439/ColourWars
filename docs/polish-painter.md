# Paint.exe painter: production artwork and action poses

Implemented 2026-10-15, beginning `POL#7`. This carries the accepted
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

## Remaining character work

`POL#7` still owns the final human review of character/teammate readability in a coherent
single- and four-player gameplay composition, after the reference-arena
(`POL#17`) pass provides that composition. The [five-tool artwork and motion](polish-tools.md)
now use these hand attachments. The [quiet-detail increment](polish-plan.md#tool-personality-and-quiet-detail)
adds independently phased breathing, blinks, weight shifts and acceleration/turn settling.
Terrain, paint, effects and UI keep their
existing owners and order.
