# Paint.exe tools: production artwork and secondary motion

Implemented 2026-10-15, completing `POL#8`'s five-tool artwork and mechanism pass.
The tools join the [E painter](polish-painter.md) in the accepted
[Paint.exe drawing language](polish-plan.md#1-establish-the-art-and-display-rules).
The author approved all five tools with the current painter for this development
stage on 2026-10-20. This is a working baseline, open to further iteration;
Close Quarters composition and readability are rechecked in `POL#17`/`POL#24`.
These captures retain the existing arena, projectiles and HUD.

## Drawing and attachments

The editable [vector source and recipe](../game/art/weapons/README.md) draw black
outlines, neutral worn hardware and explicit team-paint materials. Each tool has
four same-canvas poses, each split into complementary neutral and grayscale paint
layers. The runtime atlas contains 44 named frames: 20 pose pairs and four preserved
utility frames, plus the two reservoir supply strips described below. Its
premultiplied RGBA8 DDS has one mip and four-texel packed-rectangle gutters.
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
their existing simulation paths. The [reservoir supply pass](#reservoir-paint-supply)
now makes the sprayer and mister liquid follow ammunition; the bucket opening
remains a wet material, and the sniper dial still depicts pressure.

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

The [quiet-detail increment](polish-plan.md#tool-personality-and-quiet-detail)
adds the sprayer's residual bead and falling drip, plus quiet character motion; the
[projectile and paint pass](polish-paint-effects.md) supplies liquid shot shapes,
matching impacts and roller contact flecks. The later
[aim/readiness cue](polish-aim-marker.md) displays actual shot eligibility;
the [bare-cover contact cue](polish-blocked-shots.md) explains an obstructed
shot. The [shared player badges](polish-plan.md#help-players-read-the-tool-and-the-fight)
now carry roster identity across selection, figure, pane and results. The author
approved the current identity, readiness and blocked-shot cues for this stage
on 2026-10-20, closing the current-arena review held by `POL#14`.
These automated checks and image inspection do not establish
human movement-feel, controller, listening or final composition acceptance.

## Sniper residue and bucket runoff

Implemented on 2026-10-15 as a gameplay-style increment of `POL#16`. The quiet
tool details in the polish plan now include a smaller, less frequent bead on
the precision nozzle and occasional runoff from the bucket's existing wet rim
stripe. Both reuse the sprayer's scene-owned cosmetic drip, its gravity and
terrain sweep, and its black keyline. No new atlas or sound is needed.

| Tool | Quiet interval | Bead width / stretch | Attachment from the tool's left-centre draw pivot |
|---|---|---|---|
| Sprayer | 2.8–6.2 s | 6 / 5 world units | `(69, 3)`, unchanged nozzle lip |
| Sniper | 4.2–8.4 s | 4 / 3 world units | `(99, 3)`, precision nozzle lip |
| Bucket | 4.8–9.6 s | 5 / 4 world units | `(46, 8)`, end of the rim's painted runoff stripe |

The bucket bead runs down the outside of the tin before falling free; it does
not appear at the bucket's distant projectile launch point. Attachments follow
continuous aim and the actual facing mirror, then detached drops fall vertically
without following later aim. The three tools share the 0.42 s growth, 0.18 s
stretch and at most 0.75 s fall. Each roster member retains a private, varied
cosmetic sequence. Sizes, offsets and intervals live in
`presentation.json.idle_drip.tools`; startup validates each profile independently.

Only a supported, alive, motionless painter in its idle pose can grow a bead.
Movement, jumping, held fire (including an empty tool) and elimination cancel
the attached bead. Falling drops finish independently, disappear at actual
terrain contact and never change ammunition, projectiles, damage or scored
paint. Pause freezes them; time-up clears them before the reveal. Reduce effects
suppresses all three tools' residue and starts a fresh wait when restored.

Validation on 2026-10-15:

- Debug and Release build under `/W4 /WX` and pass all 21 consumer cases.
  The extended `idle_drip` case checks the three tools' independent profiles,
  contextual content errors, actual aimed/mirrored art attachments, held empty
  fire, interruption, pause, death/respawn, live reduced effects and time-up.
  Enabled/disabled real matches preserve movement, shots, ammunition, health,
  projectile counts, paint ownership and scores.
- Both D3D11 hardware quiet-detail probes pass on the NVIDIA GeForce RTX 5080,
  with 182 world captures each. They retain the sprayer/quiet-character matrix
  and add sniper/bucket growth, stretching, falling, continuous aim in four
  quadrants and both facings. All three palettes run in one/four views at 720p,
  with representative attached beads at 1080p and QHD. Per-tool on/off and
  reduced-effects rasters, culling bounds, repeated frozen draws, observed state
  and Debug replay fingerprints pass. Native-size image inspection confirms
  contact with the lip or runoff stripe through aim and facing changes.
- Isolated Debug and Release startup pass with the accepted sound bank, a
  system-only PATH and graceful shutdown.

The retained [four-player 720p contact sheet](images/polish-tool-residue/tool-residue-720p.png)
contains native-size renderer crops. Local evidence is under `out/acceptance/`
and `out/validation/tool-residue/`; visual run IDs are
`x64-debug/373ef08549bf440d95e2a83f7fd4c6eb` and
`x64-release/1040f9f3283e413b8ba3d72eb3c6ec1a`. The focused probe runs only the
affected quiet-detail matrices; the full painter matrix remains its default:

```powershell
scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -QuietOnly
```

The author's 2026-10-20 stage approval covers the current tool presentation.
Reference-arena composition and readability are rechecked in `POL#17`/`POL#24`;
remaining match and UI work stays in `POL#16`.

## Reservoir paint supply

Implemented 2026-10-15 as a gameplay-art increment of `POL#16`, following the
visible-supply requirement in polish-plan §4. The sprayer and mister reservoirs
now show actual remaining paint. The sprayer has six liquid rows, the mister
seven visible rows after hardware occlusion. Empty space is opaque neutral
grey; the nozzle remains wet and team-coloured. Other tools retain their
existing material and mechanism drawings.

Drawing selects `ceil(clamp(ammo / starting_ammo, 0, 1) * rows)` directly from
the current ammunition. Positive reserves retain at least the bottom row;
zero is empty. The coarse window is supplementary information: the existing
readiness cue still reports whether the tool can afford a complete shot.
Regeneration, infinite-ammo refill, respawn and live maximum-ammo edits need
no separate visual timer. Pause freezes the underlying ammunition, and Reduce
effects preserves the information. The opening's grey override also applies
to the liquid. Selection previews use the full reservoir and clear that layer
when switching to another tool or Random.

The [editable source and recipe](../game/art/weapons/README.md) separate reservoir
liquid from permanent wet paint. Each supply level occupies the full original
tool canvas and shares its destination, origin, rotation and flip. This keeps
the liquid registered with the base under camera-space edge rounding at 720p
and through continuous aim. The recipe checks identical supply masks after
every pose's hardware occlusion before sharing a strip. Startup rejects
missing strips, fewer than two levels, different canvases and nonzero body
origins. Drawing remains const and introduces no simulation state or randomness.

The atlas grows from 2048x1280 to 2560x1280 (2.5 MiB additional uncompressed
RGBA8 texture memory), retaining all original tool silhouettes, grips, muzzles
and utility pixels. Only the reviewed DDS and JSON enter staging and Release.

Run the focused renderer check from an x64 VS Developer shell:

```powershell
scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -SupplyOnly
scripts/check-painter.ps1 -Preset x64-release -SkipBuild -SupplyOnly
```

Validation on 2026-10-15:

- Debug and Release build under `/W4 /WX` and each pass all 21 consumer cases,
  including ammunition/cadence, movement, paint ownership, tool motion and
  Debug replay determinism.
- Both focused D3D11 hardware matrices pass on the NVIDIA GeForce RTX 5080:
  one/four-player 720p in every palette, four aim quadrants and all four poses,
  with representative 4P1080p/QHD checks. Independently composed authored
  layers match the full/half weapon raster; quantity changes stay inside the
  existing opaque tool silhouette and leave hardware/permanent paint exact.
  Checks include clamped quantities, paid depletion, natural regeneration,
  reset, motion disabled, Reduce effects, full menu previews and switching
  tools, frozen pixels/state/Debug fingerprint and unchanged draw RNG.
- Two fresh-process regenerations are byte-identical for source PNG, atlas
  JSON, DDS and lineup. Full assembled artwork matches the previous revision
  pixel for pixel in every pose and all five hues; every supply frame contains
  exactly its bottom row count. Other tools, utility pixels and silhouettes
  are unchanged. DDS pixels match premultiplied RGBA8 with one mip.
- The 53-file Release ZIP contains all 26 reviewed runtime assets byte for
  byte and excludes authoring/debug files. Isolated Debug and extracted
  Release startup pass with the bank and a system-only PATH. A separate
  extracted no-bank copy starts with exactly the expected single fallback
  diagnostic; every run closes gracefully without touching the player's save.

Evidence is under `out/validation/tool-supply/` and
`out/validation/weapon-supply-art/`. Successful visual runs are Debug
`13f3e25ba6934188b64a3b43987f6cde` and Release
`852709454c504f5ab607fef273706f98`. The retained
[native four-player comparison](images/polish-tools/supply-720p.png) shows full,
half and empty windows for both tools in all five hues without enlargement.
These checks complete this supply increment, while `POL#16` remains open for
the remaining presentation work. The current art is the author's approved
working baseline as of 2026-10-20; reference-match composition, movement, pad,
listening and frame-pacing acceptance keep their separate TODO owners.
