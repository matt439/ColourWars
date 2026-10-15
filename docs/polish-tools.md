# Paint.exe tools: production artwork and secondary motion

Implemented 2026-10-15, completing `POL#8`'s five-tool artwork and mechanism pass.
The tools join the [E painter](polish-painter.md) in the accepted
[Paint.exe drawing language](polish-plan.md#1-establish-the-art-and-display-rules).
Human review of the reference composition remains `POL#7`, after the arena
blockout (`POL#17`). These captures retain the existing arena, projectiles and HUD.

## Drawing and attachments

The editable [vector source and recipe](../game/art/weapons/README.md) draw black
outlines, neutral worn hardware and explicit team-paint materials. Each tool has
four same-canvas poses, each split into complementary neutral and grayscale paint
layers. The runtime atlas contains 44 frames: 20 pose pairs and four preserved
utility frames. Its premultiplied RGBA8 DDS has one mip and four-texel frame gutters.
Only the DDS and frame JSON enter staging and Release; sources, PNGs, preview and
tools remain under `game/art/`.

| Tool | Design and mechanism |
|---|---|
| Sprayer | Compact pressure reservoir, valve, hose and team-wet nozzle. Paid fire cycles a pressure kick and recovery; supported travel bows the hose, which settles after stopping. |
| Sniper | Long precision tube, piston and pressure dial. Each paid shot releases pressure and recovers deliberately, for at least the current firing interval. The dial depicts the mechanism; it does not report ammunition. |
| Roller | Battered circular sleeve seen end-on, axle, bracket and taped grip. Wear/index poses follow signed supported travel; reversing reverses the drum. Ground contact compresses and settles it; airborne support loss exposes the lifted bracket. |
| Mister | Bulbous garden atomiser with pump and outlet valve. Paid flow cycles the valve; release leaves a short expanding, fading haze at its last nozzle position and direction. |
| Bucket | Dented tin, wet rim, visible fill and wire bail above a fixed hand bar. Paid throws have follow-through; supported travel swings the handle and sloshes the fill, then settles. |

The artwork retains every existing tool canvas, pivot, muzzle offset and grip.
Hands stay attached while mechanisms move within the canvas. The roller's axle
stays at local `(75,0)`, preserving its paint contact. Continuous aim, ammunition,
cadence, projectile launch/ownership, movement, collisions and scored paint keep
their existing simulation paths. Tanks and fill are ungraduated materials.

## Motion and lifecycle

Numeric visual timing and travel periods live in `presentation.json.tool_motion`.
The concrete `Weapon` workflow observes the actual paid-shot result, before any
launch-obstruction retirement. Holding fire through cooldown or an empty tank
cannot create another pressure event. Automatic sprayer/mister fire lets a
phrase recover before another paid shot starts it.

`Player::end_contacts` supplies bounded, signed resolved travel after all
separation and buffered jumps. Requested velocity into a wall, airborne travel
and spawn settling cannot turn the roller. The mister haze is cosmetic: no
projectile, paint, damage or ammunition is created. Its detached envelope is
included in culling even if the player moves or re-aims.

Motion advances only during simulation updates. Drawing is const and does not
age or restart a phrase; pause and nonpositive deltas freeze it. Elimination,
fatal contacts, placement, respawn and time-up clear motion and release history.
Startup rejects missing pose frames or a pose/mask with a different canvas or
origin, identifying the tool and frame.

## Reproduction and validation

Run `powershell -ExecutionPolicy Bypass -File scripts/build-weapon-art.ps1`
to regenerate the working PNG, JSON, DDS and [authoring lineup](../game/art/weapons/lineup.png).
Normal builds consume the reviewed outputs. The lineup includes all four poses
in neutral and all five hues at the real 720p gameplay scale: 2/3 world scale,
an 80-pixel body, for both single- and four-player panes.

Validation on 2026-10-15:

- Debug and Release build and pass all 12 consumer acceptance cases each.
  The new `tool_motion` case covers paid/cooldown/dry-fire edges, both sustained
  stream poses, frozen updates, bounded haze and lifecycle resets. A 1,200-tick
  sequence per tool compares motion enabled/disabled: ammunition, cadence,
  continuous aim, muzzle, projectile ownership and subsequent travel match.
  Real Player/Scene floor, wall, jump, death and respawn contacts check the
  roller's resolved-distance path.
- Both builds pass the hardware D3D11 painter/tool matrix on the NVIDIA GeForce
  RTX 5080. At single-player 720p it samples all five tools in both facings at
  paid fire/hold/release/recovery, hose/bucket travel and roller travel/lift.
  Four-player 720p samples the four aimed tools facing right and the roller
  facing left in all three palettes, with additional 1080p/QHD extent captures.
  The existing aim/action/redraw matrix also passes. Isolated sampled non-idle
  tool rasters differ from idle at gameplay zoom, including the released
  roller's travel/lift mechanisms. Repeated
  draws are byte-identical, preserve observed motion/simulation state and the
  Debug replay fingerprint, and stay within projected culling bounds.
- Consecutive art regenerations are byte-identical. Pixel checks cover all 20
  distinct poses, complementary grayscale paint masks, opaque grip regions,
  firearm terminals, the fixed axle, disjoint gutters, unchanged utility pixels
  and every DDS pixel's premultiplication against the PNG.
- The 51-file Release ZIP contains all 24 reviewed runtime assets byte for byte
  and excludes authoring assets. Isolated Debug and extracted Release startup
  pass with the accepted bank, a system-only PATH and graceful shutdown.
  Extracted Release also passes separately without the bank, with the single
  expected silent-bank diagnostic.

Evidence is under `out/validation/paint-exe-tools/` and `out/acceptance/`.
Retained captures show [native 720p tool crops](images/polish-tools/tools-720p.png),
[four-player firing](images/polish-tools/four-player-720p.png) and
[one-player 1080p aiming](images/polish-tools/one-player-1080p.png).

Idle droplets and quiet character details remain `POL#12`; projectile/impact
shapes, liquid paint and emitted roller flecks remain `POL#11`. Readiness cues
remain `POL#14`. These automated checks and image inspection do not establish
human movement-feel, controller, listening or final composition acceptance.
