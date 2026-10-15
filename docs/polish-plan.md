# ColourWars polish and level-design plan

Status: implementation started on 2026-10-08. Camera/movement foundations and a
takeoff/landing feedback prototype are implemented. The superseded workshop
kit and its runtime comparison were retired on 2026-10-15; the historical
style studies and authored Movement Course route trials remain. The Paint.exe art
rules are accepted as of 2026-10-15. Initial braking, released-aim and route
policies (`POL#4`) and stronger takeoff/landing puffs (`POL#5`) are implemented;
their values remain tunable. Production artwork, movement-feel and arena
acceptance remain open.
Direction agreed on 2026-10-08. Source baseline: game revision `13e9960`.

First increment: resolution-independent camera framing, configurable jump buffering
and ledge grace, and an explicitly selectable Movement Course (excluded from Random).
See [implementation and evidence](polish-foundation.md) and
[course routes and remaining trials](polish-movement-course.md).

Second increment: [movement feedback and evidence](polish-feedback.md), plus
[the workshop style candidate and grid/framing study](polish-style-reference.md).
The generated reference is concept art, not a runtime atlas or accepted arena.

Third increment: [editable workshop kit and framing evidence](polish-style-kit.md).
That increment retained the kit as a separate art study.

Fourth increment: [playable workshop comparison and evidence](polish-playable-kit.md).
That historical increment offered original, rotating-tool and authored-tool
presentation, animated team masks, hands, badges and native tiled terrain.
Its runtime comparison was retired on 2026-10-15.

Fifth increment: [workshop refinement and course trials](polish-refine.md).
It changed the arms to fixed-length bones, added hysteresis to posed-tool
direction changes, and moves larger light badges behind the head. It lifts
dark-team masks to a luminance floor. It shows that standing contact is
pixel-flush, retains the Movement Course route trials as CI acceptance, and
captured feedback on and off while the camera moved. Its workshop candidates
have since been retired; movement-feel review remains open.

Direction changed on 2026-10-09: the author chose **Paint.exe Maximalism**
(crude, mouse-drawn box-head figures, embraced deliberately) over the workshop
pixel-art kit, which the second to fifth increments above built and which is
now superseded. On 2026-10-15 the author confirmed all five round-2 picks:
Zoom 400% values, Tool Props terrain, airbrush cast shadows, a black keyline
under paint, and figure variant E with a head accent for teammates. These are
the accepted rules in §1. Gameplay style comes first, UI after; where this
plan says pixel art below, read Paint.exe Maximalism. The author also chose to
retire the workshop kit on 2026-10-15 (`POL#2`): its comparison modes, F8 binding,
runtime/source atlas, build/check scripts and dedicated cases are removed.
Historical images, clips and study records remain, with sources in Git history.

The theme, picked on the same style board, is a grey, unsaved picture with one
palette slot left and two team colours fighting for it. [The backstory](backstory.md)
records it, and its build on 2026-10-12 and 13 delivered the respawn redraw,
grey scenery with each team on its own half, the opening (monitor, person,
preview flicker, undo, leaving, push through the glass; A, B or Start skips; a
quick rematch), the reveal (back into the monitor, the person hovers each
colour, clicks the winner, the coverage shows and the person leaves, or closes
the program when nobody painted) and the figures acting. Solo practice skips
both ends, and the results page keeps only the percentages after the reveal.
Its accepted pop, sigh and chair-creak sounds are integrated as of 2026-10-15.
Open work and its order are in [TODO.md](../TODO.md).

## Goal and direction

Make ColourWars look and play like a finished local multiplayer paint game,
with a distinctive, consistent art style and levels designed around movement,
painting and encounters. The first deliverable is one complete, polished match
that establishes the standard for the rest of the game.

The quality target is a good, deliberately crafted indie game: cohesive art,
responsive controls, distinctive tools, thoughtful levels and convincing detail
throughout an ordinary play session. Functional completeness is one part of that
standard; finish also requires iteration on timing, transitions and personality.

The user's direction is explicit:

- Use a consistent art style throughout the game (pixel art as first given;
  Paint.exe Maximalism since 2026-10-09).
- Replace the generic character of the newer weapon designs with distinctive
  paint tools, designed together with their wielders and firing animations.
- Add substantial environmental and background detail, depth and identity.
- Design better levels as well as improving their artwork.
- Improve both visual presentation and game feel.
- Replace every existing sound effect and music track. The user has identified
  copyright concerns and considers the current audio unsuitable for the game.
- Add small, expressive feedback effects to ordinary actions, especially jumping
  and landing, so movement and interaction feel tactile as well as responsive.
- Give quiet moments life too: for example, a paint droplet should occasionally
  form at the muzzle and fall while the player stands still. Treat these small
  details as part of the intended quality level throughout the game.

Aim for expressive characters, satisfying liquid paint and places that feel
constructed and inhabited. Detail should communicate materials, function and
history while keeping players, projectiles and painted surfaces easy to read.

### What the indie-game standard means in practice

- Moving, jumping and painting in an otherwise empty room should be enjoyable.
  Review control response, poses, contact, sound and recovery together.
- Tools should communicate their materials and working mechanism. Liquid sloshes,
  paint beads, a hose follows movement and a roller turns against the ground.
- Ordinary actions need a complete visual phrase: an immediate response, a clear
  action/contact and a small recovery or settling motion. Let new input interrupt
  decorative poses so animation never makes the controls wait.
- Quiet scenes should feel inhabited through occasional, purposeful motion.
  Reserve the strongest contrast and movement for information needed during play.
- The finish must hold through transitions: start and stop firing, land and move
  again, empty and refill, die and respawn, pause and resume, finish and rematch.
- Visual and audio style, timing and level readability need human review at playing
  size. A checklist of implemented effects does not establish the quality target.

Small details should reinforce ColourWars' identity as a messy contest with physical
paint tools. Choose them for material, weight, state or personality, and evaluate
how they work together in the reference match.

## Starting point

This plan comes from current source and the retained 2026-10-08 validation
captures. It does not establish how the game feels on physical controllers;
fresh two- and four-player playtests are part of the work.

Useful work already exists: explicit joining, reconnect pause, palette options,
labelled HUDs, controls cards, stage previews, saved settings, best-of sessions
and per-player paint contribution. Build on those systems.

At the source baseline, the visible gap was consistency and depth: the textured menu, shaded weapon art,
simple characters and largely flat arena geometry speak different visual
languages. The gameplay code also offers concrete polish opportunities:

- Camera framing followed pane pixels at scale 1, so changing
  resolution changes the amount of world visible.
- Jumping required a press while grounded; there was no jump buffer or short
  grace period after walking off a ledge.
- Damage reduces health and plays a sound, with little visual response. Death
  removes the character, and respawn restores it at its spawn.
- All five projectile types use the same animation, despite different behaviour.
- Surface splashes and the animated results reveal already exist, but need the
  same art and feedback treatment as the rest of the game.

The private legacy audio bank has been retired from live content and packaging.
The accepted distributable replacement set now supplies the game's existing
audio bindings; [NOTICE](../NOTICE) records its sources. Further cue hooks and
the in-game mix remain open, with a silent fallback when the bank is missing.

Relevant implementation references:
[player](../game/objects/player.cpp), [camera and match flow](../game/objects/level.cpp),
[tuning](../game/content/tuning.json), [presentation](../game/content/presentation.json),
[weapon artwork](../game/art/README.md), [level editor](level-editor.md) and
[completed feature increments](feature-roadmap.md).

## 1. Establish the art and display rules

**Art rules accepted by the author on 2026-10-15 (`POL#1`).** Use Paint.exe
Maximalism: crude, mouse-drawn box-head figures, embraced deliberately. The
confirmed round-2 choices are:

| Element | Accepted treatment |
|---|---|
| Values | Zoom 400% |
| Terrain | Tool Props |
| Cast shadows | Airbrush |
| Paint edge | Black keyline under paint |
| Figures and teammate identity | Variant E with a head accent for teammates |

These names refer to the [round-2 visual study](../game/art/references/style-board/index.html)
([original artifact](https://claude.ai/artifact/9J5kQgRMvVtXPe6r6cJ7vP)),
supplied by the author on 2026-10-15 and retained with the
[production sources](../game/art/README.md). The names do not specify numeric colours, sprite dimensions or
finished assets. The workshop kit and its pixel-grid choices remain historical
comparison evidence, rather than the production art standard.

Build the gameplay style first: a small reference containing a character, two
contrasting weapons, a painted platform, background materials and effects.
Inspect it at actual gameplay sizes, especially four-player split-screen. Carry
the accepted treatment into the HUD and menus after the gameplay composition
works.

Record the production details as the reference is built:

- Character proportions, weapon scale, team-colour regions and the head accent.
  The in-world team cue must remain readable at four-player size.
- Palette roles: quiet background, readable terrain, distinct characters and
  bright paint. Check every supported team palette.
- Consistent world art scale, keyline weight, airbrush shading and truthful
  collision silhouettes beneath the Tool Props terrain.
- Authored world framing for each layout and behaviour at other aspect ratios.
- Display scaling, camera alignment and weapon rotation treatment while keeping
  simulation aim continuous.
- Typography and UI scaling that remain readable at the smallest supported size,
  once the gameplay style is established.

Reauthor the newer weapons and, afterwards, menu artwork to this standard. Keep
editable sources and reproducible atlas generation. The engine already exposes
point filtering and a camera helper accepting world units; use those seams and
review scale, fractional placement and rotation in motion.

**Exit condition:** one gameplay composition looks coherent in single-player and
four-player views. Slow camera movement and aim sweeps remain stable and readable
at 720p, 1080p and QHD. Record the supported scaling behaviour and any letterboxing.

**Workshop retirement validation (2026-10-15):** Debug and Release build and
pass all 11 remaining consumer cases. Movement Course routes, movement feedback,
story/redraw audio and replay determinism remain covered. The registry no longer
offers `visual`, F8 has no default action, and replay v3 explicitly rejects the
old v2 schema. Acceptance runners ignore object files whose retired source no
longer exists, so an incremental build cannot link the old visual into a probe.

The hidden D3D11 movement-feedback probe passes in one/four views at 720p,
1080p and QHD, including camera pan, aim sweeps and frozen redraw. Both staged
builds and the 49-file Release ZIP exclude the workshop atlas and player visual;
the ZIP retains the unchanged accepted sound bank. Isolated Debug and extracted
Release startup pass with the real bank and separately without it, using a
system-only PATH and graceful close. The driven console phase-4 check passes
the six-PNG feedback review, station tour, bots, binds/history across relaunch,
live-console input and perf overlay. Evidence is under
`out/validation/workshop-retirement/` and `out/acceptance/`. These checks retain
the current presentation; they do not accept new Paint.exe production art or
replace human movement/audio/pad review.

## 2. Establish movement and camera feel

Set camera framing before judging movement speed, weapon range or arena size.
At the same layout and aspect ratio, resolution should preserve the designed
world extent. Keep camera bounds and mouse aiming correct after scaling or resize.

Use a small movement course to compare the baseline with candidate changes:

- Short and long jumps, running jumps, landings, ceilings and platform drops.
- Acceleration, braking, reversing and controller movement deadzones.
- A configurable jump buffer and a short ledge grace period.
- Aim response, retained direction, muzzle placement and moving while firing.
- Camera follow, restrained look-ahead and visibility of upcoming landings.
- Prototype takeoff and landing effects on the actual movement events, so their
  timing can be judged alongside input response before final artwork and audio.

Measure the resulting jump envelope and travel speed. Use those measurements
when placing platforms; the old movement note is not a level-design specification.
Tune values through keyboard/mouse and controller playtests before freezing them.

**Exit condition:** required jumps are reproducible, input feels responsive, and
buffered input cannot cause extra jumps after pause, death, respawn or dropping
through a platform. Capture comparable before/after clips and record chosen values.

## 3. Redesign the levels in playable blockouts

Level design is a full workstream. Revise geometry, routes, scale, spawn areas and
the distribution of paintable surface before producing finished arena artwork.
Use simple geometry in the [existing editor](level-editor.md), save experimental
copies, rebuild, and judge them in the game. The editor does not simulate play.

### Design requirements

| Area | Requirement and how to judge it |
|---|---|
| Routes | Provide a clear route into the contest and a meaningful alternative. Flanks should reconnect to useful space; avoid long empty corridors, accidental dead ends and a single mandatory choke. |
| Movement | Base gaps, steps and clearances on the measured movement course. Ordinary routes should tolerate normal input variation; optional shortcuts may demand more skill. Test both directions. |
| Verticality | Use height for route choice and tradeoffs. Important high ground needs a practical counter-route and escape options; it should not safely cover every route and spawn. |
| Spawns | Keep four valid spawn positions per team with full character clearance. Give players room to orient and routes into play that an opponent cannot lock down from one firing position. Test repeated respawns under pressure. |
| Painting | Budget exposed, reachable paintable surface by region, including walls and ceilings. Large safe home areas or awkward perimeter strips must not decide the result without meaningful contest. |
| Weapon roles | Give short-range and roller play useful connected surfaces, and ranged weapons useful but contestable sightlines. Test all five existing weapons during blockout, even before their art is replaced. |
| Pacing | Measure time to first contest, time back from spawn, unopposed painting stretches and recurring choke points. Adjust routes and dimensions using observed matches. |
| Fairness | Swap teams and spawn sides with comparable loadouts. Investigate repeatable side or weapon advantages; a small set of wins alone does not establish balance. |
| Readability | Players should recognise solid cover, jump-through platforms, ramps, paintable surfaces and decoration at speed. Camera framing must expose enough information to choose the next move. |
| Player count | Qualify 1v1 and 2v2 separately, and exercise three-player layouts. Identify whether a layout needs revision or an explicitly narrower recommended player count. |

Record surface area and route observations before and after geometry revisions.
Scoring is based on exposed paintable surface, not visual floor area or distance
travelled. Preserve deliberate ramp paint behaviour and buried-face exclusion.

### Initial arena briefs

These are proposed starting designs to test, not declarations that the existing
maps already provide them. All three retain the painted-surface match rules.

| Arena | Play identity to develop | Environment direction |
|---|---|---|
| Close Quarters | A compact loop of connected rooms, a central contest and a useful upper or lower bypass. Frequent encounters with space to disengage; sheltered spawn approaches. First reference arena. | Paint workshop and supply warehouse: shelving, tanks, workbenches, timber, pipes, loading doors and distant work areas. |
| King of the Hill | A central elevated structure approached from both sides and more than one height. High-ground control trades off against lower paint routes and flank exposure. The name does not introduce a hill-scoring mode. | A printworks or workshop tower: supports, gantries, stairs represented by supported geometry, hanging signs, skylights and distant roof structures. |
| Turbulence | Offset platforms and crossing routes that reward movement and repositioning. Mix short engagements with exposed crossings and recoverable lower routes. | An industrial loading yard: ducts, vents, stacked supplies, overhead structures, distant machinery and layered sky. Decorative machinery does not imply new wind or hazard mechanics. |

Begin with one Close Quarters blockout. Iterate it until actual two- and
four-player matches support its route and pacing goals, then establish blockouts
for the other two identities. Use supported structures, platforms, ramps and
visuals; evaluate any new gameplay object as a separate, evidenced requirement.

**Exit condition:** the reference blockout has reachable intended routes, usable
spawns, contestable paint distribution and useful roles for all five weapons.
Both sides have been played, failed routes and spawn traps have been addressed,
and remaining balance questions are recorded before art production proceeds.
Retain a short route, spawn and paint-distribution brief for each accepted layout.

## 4. Finish one arena, character and action chain

Take the accepted Close Quarters blockout to finished presentation with one
character design and the sprayer and roller. Other weapons remain available for
gameplay checks while their visual treatment follows later.

### Environment and background

Build a reusable material and prop kit around the arena's purpose:

- Platforms with readable edges, undersides, supports, joints and surface wear.
- Distinct metal, wood, concrete and other materials using controlled pixel clusters.
- Background layers: distant architecture, middle-distance machinery and shelving,
  and restrained near details. Prototype modest parallax after camera framing works.
- Landmarks, signage, lights, brackets, pipes and evidence of use to identify spaces.
- Small ambient animations where they add life without competing with combat.
- Background contrast and detail density that keep characters and fresh paint clear.

Keep collision silhouettes truthful. Decorative depth, foreground objects and
surface effects must not conceal landing edges or suggest a route that cannot be
used. Check the arena both clean and heavily painted with every team palette.

### Characters and distinctive weapons

The [production painter increments](polish-painter.md) implement `POL#7`'s artwork:
figure E, animated team masks, head accents and continuous tool grips, with
idle/run/jump/fall and dedicated takeoff/landing/firing/hurt/elimination clips,
plus a matching respawn redraw. The composition's human review remains open.

Author the character, hands, weapon grips and aim poses together. Cover idle,
running, takeoff, airborne motion, landing, firing, hurt, elimination and respawn.
Keep silhouette and team identity readable in small views; give same-team players
a clear identity cue. Preserve hitboxes and muzzle behaviour during visual changes.

Explore these weapon briefs, refining their silhouettes through actual-size reviews:

| Weapon | Design and motion direction |
|---|---|
| Sprayer | A compact pressure-fed paint tool with recognisable reservoir, hose/nozzle construction and a sputtering stream. |
| Sniper | An exaggerated precision paint nozzle with a pressure gauge, long silhouette and a sharp pressure-release firing beat. |
| Roller | A battered decorator's roller with a taped grip, visible drum rotation and a substantial painted contact trail. |
| Mister | A garden-style atomiser with a distinctive tank and outlet; communicate its mist through shape and motion. |
| Bucket | A dented paint tin with a recognisable handle, sloshing fill and a committed pouring or throwing gesture. |

Use material details, purposeful wear and motion to give each tool personality.
Retain the useful separation between neutral hardware and team-coloured paint.
Check left/right facing, all supported aim angles and motion while firing.

### Tool personality and quiet detail

Build secondary motion into the character and tool rather than relying entirely
on emitted particles. Use a few readable pixel poses and modest changes in shape;
inspect them at the actual split-screen scale.

The idle muzzle drip is a required signature detail for the reference sprayer:

1. Once an alive player has settled into a stationary, non-firing pose, occasionally
   form a small bead of the player's team paint at the underside of the nozzle.
   Vary the quiet interval so several players do not drip in synchrony.
2. Let the bead grow and stretch briefly, then detach and fall with gravity.
   Attach it to the actual rendered muzzle through aiming and left/right facing;
   after detachment, its fall is independent of the weapon's aim direction.
3. End the droplet on a valid surface contact or its lifetime limit. Use at most
   a tiny, brief contact effect. Keep idle dripping quiet by default.
4. Cancel an attached idle bead when movement, firing or elimination interrupts it;
   already falling droplets may finish their short life. Pause freezes them and
   scene teardown clears them. Author the timing and size after observing the
   effect in play; the intended impression is occasional residual liquid.
5. Keep it cosmetic: no ammunition use, damage, scored paint or ownership change.
   Its brief contact cue must not resemble a newly captured surface. Use cosmetic
   randomness separate from gameplay choices and update once per world event,
   regardless of how many player views can see the tool.

Give each tool appropriate related details as its production pass arrives:

| Tool | Small details to develop |
|---|---|
| Sprayer | A slight pressure kick during firing, a hose that follows motion, occasional idle muzzle drip and a short reservoir/valve settle when firing stops. |
| Sniper | A crisp pressure-release motion, deliberate recovery, a gauge that settles and a restrained residual bead. The recovery animation must agree with the existing shot cadence. |
| Roller | Drum rotation tied to supported travel, a small contact compression, flecks while painting and a visible lift/settle when leaving or meeting the ground. |
| Mister | Valve movement, a visibly fine spray and a brief dissipating mist tail on release. The tail is decoration after firing stops. |
| Bucket | A handle that swings and settles, paint that sloshes as movement changes, a wet rim and occasional rim drip, plus follow-through around the existing throw timing. |

Character detail should support these objects: subtle breathing and occasional
blinking/weight shifts at rest, a grip adjustment after settling, modest body lean
on acceleration and turns, and carried parts that catch up after a stop. Keep hands
connected to grips and feet grounded. Preserve aim accuracy and muzzle behaviour.
Avoid making every character perform the same idle loop at the same phase.

### Movement and interaction feedback

Small effects are part of the movement design, including ordinary actions that
do not involve combat. Coordinate the character pose, a brief pixel-art effect
and a new sound cue around the same event.

| Event | Visual and audio treatment |
|---|---|
| Jump takeoff | A small puff or burst at the feet, an immediate push-off pose and a short takeoff sound. Keep the input-to-jump response immediate. |
| Ground landing | A brief contact puff or ring, a small compression/recovery pose and a landing sound. Scale intensity with impact speed, keeping small hops subtle. |
| Running steps | Restrained footstep cues and occasional small contact particles, timed to gait and supported movement. Use surface character where it is readable and affordable. |
| Abrupt stop or reversal | A short skid puff and lean/skid pose when the movement warrants it; avoid constant particles during small corrections. |
| Painted-surface contact | A small paint-coloured fleck or wet contact variation when the contacted surface is actually painted. This decoration must not place paint or change ownership. |
| Firing and paint impact | Weapon-specific motion, muzzle response, projectile shape and splash, with matching new sounds. |
| Damage, elimination and respawn | A readable hurt flash/reaction, elimination burst and brief respawn arrival effect, each with its own restrained cue. |
| Empty paint and recovery | A clear visual response and short sound when firing cannot proceed, plus a recovery cue. Rate-limit repeated empty-fire cues while the button is held. |
| Menu focus and confirmation | Small focus/selection motion paired with distinct navigation, confirm and cancel sounds. |

Trigger takeoff once on a successful jump, and landing once on a genuine transition
from airborne motion to supported ground. Walking across platform seams, remaining
grounded, respawning or drawing the same player in several views must not repeatedly
emit landing effects or sounds. Use contact position and surface information for
placement; suppress unsupported footstep effects while airborne.

Keep squash/stretch, particles and weapon recoil visual unless a gameplay change
is separately intended. Effects must not delay controls, change collision bounds,
obscure footholds or suggest new scoring. Bound their lifetime, count and cue rate,
and check the result with four players moving and firing at once.

### Paint and feedback

Give each weapon a distinct projectile or stream profile and matching impact.
Complete the action chain from firing animation through surface/player contact,
damage, elimination and respawn. Add clear empty-paint and recovery feedback.
Build on the existing surface splash rather than treating it as absent.

Paint should look liquid through authored splat shapes, droplets, edge variation
and brief impact motion. Keep decorative marks consistent with ownership and the
exact scoring geometry. Bound effect lifetimes and counts for four-player play.
Any shake should be restrained and local to the relevant view; preserve the
responsiveness of the shared multiplayer simulation.

**Exit condition:** a complete match in the reference arena feels visually coherent
and responsive. Jumping, landing and movement have visible, correctly timed feedback.
Players can recognise shots, hits, empty paint, elimination and respawn with sound
muted. Art does not alter collisions, coverage or attribution, and repeated rendering
or sustained ground contact does not duplicate effects. The idle sprayer drip and
roller contact/settling motion are present, and quiet play has deliberate character.

### Detail across the world, interface and match

Develop a coordinated set of small details beyond the character. These are concrete
directions for the relevant passes; choose variants by testing them in the reference
arena, then carry successful treatments through the rest of the game.

| Moment or object | Treatment and purpose |
|---|---|
| Fresh paint | Give a new splash a brief wet highlight and settling edge before it becomes quieter. Tiny decorative drips can follow gravity along the already-owned face; keep team coverage boundaries exact. |
| Repainting | Let the impact clearly replace the previous team tint within the actual changed area. Friendly impacts can still splash without falsely suggesting a new capture or additional score. |
| Empty and recovering tool | Coordinate a brief tool response with the paint meter. Any visible tank level or gauge must agree with actual ammunition; avoid showing an apparently full tool that cannot fire. |
| Rooms at rest | Select a few details appropriate to the workshop: a slowly turning background fan, a gently swaying sign or cable, an occasional pipe droplet, or distant machinery. Give each room a purpose and some still space. |
| Nearby contact | Consider small local secondary motion, such as a hanging prop rocking near a landing. It must read as decoration and preserve the clear collision silhouette. |
| Repeated sounds | Use a small authored set of footstep/splash variants and restrained variation so repetition stays natural. Retain recognisable event identity and control overlapping cues. |
| Menu focus | Give focus a small, prompt movement or emphasis with a clear pressed state. Transitions should preserve selection context and remain responsive to deliberate navigation. |
| Player ready | A brief character acknowledgement and a clear ready marker make joining and confirmation satisfying. The animation must not hide who is still choosing. |
| Match opening | Let players settle into a ready pose and make the countdown's final beat visually and audibly decisive. Release controls at the established gameplay start. |
| Results | Coordinate the existing fill reveal with restrained motion, replacement sound and a brief winner pose/celebration. Keep contributions and the route to rematch clear, with ties treated deliberately. |

First complete movement/contact and firing/start-stop details, then the signature
idle behaviour, room life and interface/match accents. Each detail needs a named
trigger, purpose, duration, interruption behaviour and visual/audio intensity.
Prototype inexpensive versions in the game before committing to all final frames.

Separate critical feedback, physical secondary motion and ambient life when setting
their intensity. Combat and movement should remain easy to read while decorative
activity can stay quiet. Provide a way to reduce optional shake, flashes and ambient
effects while retaining useful cues. Test the combined experience for repeated
matches, as a charming detail can become tiring when repeated constantly.

## 5. Replace the entire audio set

This is a full replacement of music and sound effects, with a coherent sound
direction suited to ColourWars. No existing track, sample, loop or sound cue is
accepted as final content. Address the user's copyright concerns and the poor fit
of the current audio together; new audio must also improve the experience.

Start the inventory and sound direction alongside the visual reference. Integrate
replacement movement, sprayer, roller, UI and match audio into the first polished
match, then complete the remaining weapons and arenas before final acceptance.

### Inventory and production

The inventory, the new cue list and a proposed sound direction are in
[audio-inventory.md](audio-inventory.md) (2026-10-14); the direction is accepted.
The [complete production set](audio-inventory.md#7-pol22-production-candidates)
now has retained sources, per-wave provenance, reproducible exports and an
isolated bank. The author accepted the set on 2026-10-14, closing POL#22.
The 2026-10-15 [runtime migration](audio-inventory.md#9-runtime-migration-and-remaining-integration)
replaces all existing audio bindings and includes the story cues and separate
countdown beats/GO. POL#23 retains the new hooks listed there and the gameplay
mix; ROAD#3 owns in-game audio acceptance.
The movement increment adds impact-scaled landing variants, distance-driven
footsteps and reversal skids with per-player repetition limits. Respawn redraws
now have separate pause-aware pen loops per player, and exposed painted support
selects wet step/landing variants within the same movement budgets. Remaining
hooks and the listening mix stay in POL#23. The menu-feedback increment adds
distinct unready and pending SFX-volume previews, shared match/pause pad
lost/back cues, and a completed-series cue. Results music, ending stings,
fades/ducking and the resume count-in remain open.

- Inventory every source wave, bank entry, loop instance and runtime reference,
  including tuning, presentation, level music and any code-selected cues. Give
  each old item an explicit replacement or retirement entry; cover unused legacy
  entries too so they cannot accidentally return in the finished bank.
- Cover title/menu music, each arena's music, countdowns and results; menu and
  confirmation sounds; jump, land, footsteps, skid, hurt, elimination and respawn;
  every weapon's firing/loop/stop sounds; paint impacts and paint supply feedback.
  Assess quiet tool/room details individually; a visual idle accent need not make
  a sound, and frequent decorative cues must not compete with useful feedback.
- Develop a sound palette with tactile liquid paint, distinct tool mechanisms,
  readable movement cues and music that supports the match's energy. Audition
  candidate music and effects together. Pixel art does not by itself require
  every sound to be a chiptune or synthetic beep.
- Produce original recordings/compositions or obtain assets with documented
  permission for the intended use and distribution. Record creator/source,
  licence or permission, attribution requirements and modifications for each
  asset. Include permission to redistribute source files if those are committed.
- Retain editable production sources and reproducible export/bank steps. Set
  practical file-size and format targets, trim silence, create clean loop points
  and author consistent gains without flattening each weapon's character.

### Integration, mix and distribution

The replacement bank definition, verified bank build, staging, runtime references
and release asset list are integrated. Only the accepted set can supply the live
bank or package; private legacy files are excluded regardless of timestamps.
README, NOTICE and repository instructions describe this workflow. All 103
accepted masters are in the bank; inclusion does not mean every new cue has a
gameplay trigger yet. Complete the remaining hooks in
[audio inventory §9](audio-inventory.md#9-runtime-migration-and-remaining-integration),
keeping looping/stoppable voices in the existing audio lifecycle.

Retain the optional-bank fallback for missing audio and keep gameplay understandable
when muted. The finished release should include the new distributable audio;
silence is a fallback rather than completion of this workstream.

Mix and audition at one, two and four players. Limit overlapping repetitive
impacts and footsteps, distinguish weapon voices, and keep music below useful
action cues. Verify smooth loops/stops, music/SFX controls, muting, focus loss,
pause/resume, reconnect and scene transitions without stuck loops or sudden peaks.

**Exit condition:** every legacy audio item is replaced or retired, every required
new cue has a documented distributable source, and the extracted release plays the
new music and effects without depending on private files. The absent-bank fallback
still works. Complete a listening pass on the reference match and full multiplayer
mix; copyright provenance alone does not establish sound quality.

## 6. Finish the match presentation and extend the treatment

Carry the pixel-art language through menu artwork, player/weapon cards, stage
preview, HUD, countdown, pause and results. Review typography at actual display
sizes. Preserve the existing clear focus indicators, labels and controls reference.

Improve stage entry, match start, closing countdown and the animated winner reveal.
Show player contributions clearly and make the rematch route easy to follow.
Measure loading pauses before choosing transition behaviour. Preserve the deliberate
withholding of live coverage percentages so the ending remains a reveal.

Once the reference match meets its exit conditions, extend the approved character
poses and feedback to every weapon and finish the other accepted arena blockouts.
Give each arena its own material palette, landmarks and background composition.
Recheck gameplay after decoration, as visual emphasis can change route choices.

Carry the replacement audio through every arena, weapon and menu. Synchronise
movement, firing, UI and results cues with their visual events and review the
complete match's pacing and mix by ear using the new set.

**Exit condition:** a new player can join, identify their character and team,
understand the painting objective and controls, finish a match and rematch with
minimal explanation. All three arenas and all five weapons meet the same standard.

## 7. Acceptance and delivery

The reference milestone comprises Close Quarters' redesigned layout and detailed
environment, one coherent animated character, sprayer and roller artwork, stable
camera framing, movement polish and complete paint/damage/respawn feedback, taken
from setup through results. Include jump/landing/contact effects and replacement
audio for that complete experience, the idle muzzle drip, tool settling, character
idle motion and a few purposeful signs of life in the room. Review it in motion and
by ear before expanding asset production.

Use a repeatable finish review for each major pass:

1. Watch an idle scene long enough to see occasional details, then move, aim,
   jump, land, fire, stop, exhaust paint and recover. Review the transitions too.
2. Repeat the same sequence with individual details disabled. Retain and refine
   details that contribute clear material, weight, state or personality; adjust
   those that draw attention without helping the experience.
3. Review at normal speed and actual display size, with audio on and muted. Use
   slow motion only to diagnose contact, attachment and timing problems.
4. Repeat in four-player combat and through pause, death, respawn and rematch.
   Quiet details must not multiply with view count, pile up or obscure action.
5. Get fresh-player observations about responsiveness, readability, tool identity
   and what still feels unfinished. Record concrete changes and repeat the review.

The finish gate combines technical acceptance with this play-and-observe review.
The reference match must look coherent, feel satisfying in repeated ordinary play,
and remain readable under pressure before its treatment becomes the production
standard. Count of assets, particles or completed tasks is not a quality metric.

For each implementation increment, retain a short evidence record:

- Game/engine revisions, configuration, stage revision, display, player count,
  input devices, team assignments, weapons and changed tuning.
- Matched screenshots and short clips before/after, including four-player combat.
- Movement/route failures, spawn pressure, painting distribution, readability and
  pacing observations. Name unresolved questions and the next experiment.
- Measured frame pacing and effect behaviour against the intended 60 fps target
  on a recorded reference machine. Set the supported hardware baseline before
  claiming performance acceptance.

Qualify these cases across the finished content:

| Check | Required evidence |
|---|---|
| Display | 720p, 1080p and QHD; one through four views; resize/fullscreen; stable framing, readable UI and pixel treatment. |
| Controls and movement | Keyboard/mouse and physical controllers; aim sweeps, required jumps, drops, collisions and buffered-input edge cases. |
| Action feedback | One takeoff effect per successful jump and one landing effect per actual landing; small hops, long falls, platform seams, slopes, pause and respawn; correct contact placement and no duplicate cues across views. |
| Quiet detail | Occasional muzzle drip, grounded idle motion and room life; correct attachments at aim angles/facing, no synchronised crowd of identical loops, pause/teardown behaviour and no effect on ammunition, damage or paint scoring. |
| Finish and restraint | Repeated action sequences and matches, comparisons with details disabled, actual four-player size, and reduced-effects settings that preserve information. Record human feedback and remaining rough transitions. |
| Level play | Two- and four-player matches, side swaps, all three arenas and all five weapons; route, spawn, paint-distribution and pacing notes. Exercise three-player sessions too. |
| Combat readability | Clean and heavily painted arenas, all palettes, crowded effects and muted audio. |
| Audio replacement | Complete old-to-new/retired inventory, source/permission records, no legacy audio in the finished bank or ZIP, and playback from the extracted release without private files. Check missing-bank fallback separately. |
| Audio quality | Listening passes for one, two and four players; movement/weapon identity, music balance, event timing, cue overlap, loops, volume settings, pause/focus/reconnect and transitions. |
| Session flow | Join, disconnect/reconnect, focus change, pause/settings, restart, draw, series completion, rematch and return to menu. |
| Correctness | Relevant retained consumer acceptance checks, Debug/Release builds and startup checks; release asset/package inspection for changed runtime content. |
| Editor | For editor changes, run its document/GUI checks. For level changes, reopen saved files and verify them in the game; editor validation alone does not prove playability. |

Add focused regression coverage where gameplay mechanisms change, such as jump
buffering, camera mapping, event emission or paint ownership. Automated checks
establish correctness; physical playtests and visual review establish readability
and feel. Keep the existing
outstanding clean-Windows deployment check in the release acceptance work.

Work stays in the game and its content, preserving its OOP design. Use the pinned
engine's existing interfaces; if an actual missing capability blocks an accepted
slice, record the client exhibit in [engine reports](review/engine-reports.md) and
handle the engine change in Labrador. Balance numbers belong in tuning, presentation
choices in presentation data, and arena layout and decoration in level data.

Continue the gameplay artwork under the accepted Paint.exe rules. The workshop
comparison is historical; its retired presentation choices do not gate production.
On 2026-10-15 the author requested initial movement and puff decisions now,
with further tuning as the game develops. The [movement decisions](
polish-movement-course.md#initial-movement-decisions-2026-10-15) and
[feedback values](polish-feedback.md#authored-prototype-values) record them.

Human movement review remains `POL#3`: use keyboard/mouse and physical
controllers with feedback enabled and disabled, including camera motion and aim
sweeps. Use that evidence for later tuning alongside the new character poses
and audio; it does not block implementing the remaining game.
Progress is gated by the exit conditions above, with each completed increment small
enough to review, validate and commit independently.

## 8. Further improvements to prototype

The following candidates extend the plan after another source review on
2026-10-08. They are design hypotheses, not implemented features or additional
requirements for every milestone. Fold the small experiments into the relevant
passes above; promote them to production work when observation supports them.
Keep the first polished match as the next deliverable.

### Help players read the tool and the fight

The current [weapon update](../game/objects/weapon.cpp) distinguishes ammunition
cost from shot interval, while the HUD shows remaining paint. Make those states
legible together: a precision tool can have paint remaining but still be recovering
between shots. Prototype a small ready/not-ready accent near the tool or aim marker,
and distinguish recovery from insufficient paint by shape or motion as well as
colour. Drive readiness from the equipped tool's cooldown and ammunition eligibility
while the player can act, independently of whether fire is held. A decorative gauge
must not promise a shot the weapon cannot yet produce. Avoid repeating a warning
every update.

Try a modest, optional aim-direction marker for the aimed tools, especially at
four-player size. Start with direction and readiness rather than a predicted hit
point: the bucket arcs and inherits movement, mist expands, and a roller has fixed
aim. The marker must describe the equipped tool honestly. Separately, make a shot
stopped by nearby cover produce a readable contact at the actual obstruction;
[launch obstruction handling](../game/objects/projectile.cpp) already prevents
shooting through that cover. Compare against the unmarked version and retain only
cues that help players understand an action without hiding its target.

Extend the planned player markers into a consistent identity system: the same
player number or small badge on the selection card, character, pane and results.
Briefly emphasise the local character on match start, respawn and resume, then
settle to a quiet marker. Test two teammates crossing and overlapping while using
the same weapon. Optional team symbols or sparse surface markings could supplement
the palettes; inspect them on heavily painted terrain before committing to a
pattern that might clutter the pixel art. Neither identity nor shot readiness
should depend on colour alone.

**Try first:** one precision-tool readiness cue and one local-player marker in
the reference match. Ask players to explain a failed shot and find their character
after a crowded encounter, with sound muted and at actual four-player size.

### Give the arena decisions that use the existing rules

Make the reference blockout test three specific situations:

- **A short recovery pocket:** place cover near the main contest with a second
  exit, so a player can stop firing, recover paint and re-enter from another angle.
  Current [tuning](../game/content/tuning.json) starts paint regeneration after
  0.5 seconds and health regeneration after five seconds. Use that existing
  difference to compare a brief pause with a full retreat. Reject a pocket that
  becomes a safe position from which to paint every useful approach.
- **A visible painting choice:** put a broad exposed scoring face in competition
  with a smaller face reachable while repositioning. Some cover can be unpaintable,
  giving safety a surface-area tradeoff. Observe whether players choose between
  fresh territory, enemy repaint and an encounter; do not add live score values to
  direct them. A route should have a reason to be used even when nobody is there
  to shoot.
- **A place teammates can name:** give each useful region one clear landmark,
  such as the mixing tank, drying racks or loading door. Carry those silhouettes
  into the stage preview and final overview. Test whether two people can call a
  location to one another without needing an added navigation overlay. Keep the
  landmark recognisable when nearby scoring surfaces are covered in paint.

Compare a compact Close Quarters copy in 1v1 if the main blockout produces long
searches or returns from spawn. Move approaches and scoring regions deliberately;
uniformly shrinking a map also changes jump and weapon relationships. Record time
to encounters and useful painting choices. Keep a separate duel variant only if
one layout cannot satisfactorily serve both 1v1 and 2v2.

Implemented 2026-10-15 (`ED#1`): the editor's spawn rectangles follow player
tuning, its tap/held guide uses the recorded movement envelope (hidden when its
tuning snapshot changes), and its cyan edges and area-by-selection diagnostics
use exposed scoring faces. The editor shares the runtime buried-face interval
calculation, including its flat-structure/jump-through blockers and exclusion of
ramps and decorations. See [Blockout aids](level-editor.md#blockout-aids) for the
controls, measurement source, acceptance and limits. These aids explain a
blockout; they do not replace playing it.

### Let players learn by making a mess

The game already has weapon descriptions, controls cards and timed solo practice
in [the main menu](../game/states/main_menu.cpp). Build on that with an optional
small workshop using the movement course and reference arena's material kit.
Provide immediate tool changes and a paint reset, with no competitive timer.
Include a low obstacle, a jump-through platform, a paintable wall and ceiling, and
cover around which the different shot shapes can be tried.

Teach through short, optional tasks: vary jump height, drop through the platform,
paint a high face, empty a tool and recover, then try reaching the same face with
a different tool. Use the real movement, ammunition and painting rules. Explain
that painted surface determines the winner; eliminations make space to paint.
Players should be able to leave immediately or stay and experiment, and repeat
only the help they need. Begin with solo use; a shared waiting-room version is a
later possibility if group playtests show a reason for it.

**Keep it if:** a fresh player can join an ordinary match knowing how to move,
recover paint and choose a useful tool without another person explaining the game.

### Make couch play comfortable between actions and rounds

Prototype a short shared count-in after pause or reconnect. Currently
[closing pause](../game/states/game_states.cpp) resumes the match immediately.
During the proposed count-in, show each character clearly and freeze physics and
all gameplay timers, including match time, ammunition and health regeneration,
shot cooldowns and respawn. Allow re-pausing and handle disconnect/focus loss
normally. Test whether it prevents surprise starts without making frequent short
pauses tiresome. Consume the menu action and suppress stale jump/pause edges across
the transition. Define and test how held fire resumes while preserving deliberate
movement and aim input.

Compare a prominent **Rematch** action with the current single-match **Restart**
wording and first focus on **Change Teams** in the
[end menu](../game/states/end_menu.cpp). Keep the existing direct routes to change
teams, tools and stage. A visible per-player ready check before the next round is
worth testing if one person's confirmation repeatedly starts play before the
others are ready; it also adds friction, so it should not become mandatory by
assumption. Give disconnected groups a clear way back to the menu.

The [aim preference](../game/play_settings.h) currently sets one radial deadzone
for everyone, although players may bring different controllers. Try individual
stick deadzones with a small live response preview, labelled for what they change.
Test two controllers together and retain settings by the documented player/device
association. Full remapping is a separate usability candidate. Preserve the
existing A-or-left-trigger jump choice, and introduce no new requirement to take
the aiming thumb off the stick.

### Make the finished paint job worth looking at

Decided with the author on 2026-10-15 (`POL#15`): keep the existing
[backstory reveal](backstory.md#the-reveal-full-screen) as the view of the finished
picture. It already shows the whole paint job. Do not add a separate **View arena**
action or unpainted-face highlight; repeated rounds should reach the next-match
choices without another inspection step. Solo practice keeps its plain overview
and full results, as recorded in the backstory's [building rules](backstory.md#building-it).

Carry the accepted Paint.exe presentation through the existing reveal and results
under §6. Label personal contribution as surface still held at the finish, which
is what the game measures. It is not total effort, shots fired or a judgement of
the teammate who made space for others to paint.

### Order the experiments by what they can teach

First try readiness/identity cues with movement polish, and recovery pockets and
painting choices in the Close Quarters blockout. Next try resume/rematch handling
during the complete-match pass. Build the optional
workshop and individual input settings once fresh-player observations identify
the most useful lessons and comfort changes. Use before/after sessions to decide
what earns its place; these candidates do not displace the agreed art, audio,
movement or level-design work.

## Design references

These developer accounts inform the approach; the specific ColourWars details
above are proposals for this game. Borrow principles and evaluate them against its
local multiplayer rules rather than copying another game's assets or mechanics.

- Maddy Thorson's [Celeste & Forgiveness](https://maddymakesgames.com/articles/celeste_and_forgiveness/index.html)
  describes small allowances such as jump buffering and ledge grace that help
  intended input succeed. The lesson here is to combine invisible control polish
  with visible feedback; an effect cannot substitute for a responsive action.
- Motion Twin's developers, in [Designing each of the 50 weapons in Dead Cells to feel distinctive](https://www.gamedeveloper.com/design/designing-each-of-the-50-weapons-in-i-dead-cells-i-to-feel-distinctive),
  describe iterating animation timing and combining motion, effects and sound to
  establish weapon weight and identity. Apply that coordinated iteration to the
  paint tools. Their single-player time-stop techniques are not a requirement for
  ColourWars' shared multiplayer simulation.
- [Game Accessibility Guidelines](https://gameaccessibilityguidelines.com/full-list/)
  recommends interactive learning, practice without failure and additional cues
  beyond colour. The workshop and identity experiments above apply those principles
  to this game's existing rules; their specific form still needs playtesting.
- Microsoft's [Xbox Accessibility Guideline 107: Input](https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/107)
  discusses configurable input and alternatives to demanding control combinations.
  Use it to inform controller-comfort and remapping work while keeping prompts
  accurate and ordinary movement/aiming comfortable.
