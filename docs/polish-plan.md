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

## Development order (2026-10-17)

The author set the order of the remaining work and clarified the map / tuning
sequence on 2026-10-20. The completed art-stage approval remains in force; the
research, new-map implementation and tuning can proceed without another
approval gate. A style lock is a
working baseline for the next iteration, not a permanent freeze on art or tuning.

1. **Establish the gameplay art style (approved for this stage, 2026-10-20):**
   the painter and tools (`POL#7`), the player and shot cues (`POL#14`) and the
   material, prop and background kit (`POL#9`), judged in the current arenas.
   The author has seen and approved the implemented treatment for this early
   development stage, with the explicit qualification that nothing is final.
2. **Research, create a new map, then tune movement and weapons together.**
   Record primary-source comparisons, concrete level examples and transfer
   limits before choosing values ([research](level-design-research.md)). Create
   an original new map (`POL#17`) with the current movement and weapon baseline;
   refining the preserved CQ draft does not replace this deliverable. Measure
   intended routes, score geography, spawn approaches, firing breaks and panes.
   Once it is playable, tune movement (`POL#3`) and all five tools (`POL#27`)
   together against that map and Movement Course: scale, exposure, projectile
   speeds/ranges/arcs, cadence, painting and recovery. Iterate geometry when
   observations identify a layout issue. Fit the approved kit and inspect
   1-4P/heavy-paint composition. Reference-arena acceptance follows actual play;
   then apply the process to the other arenas (`POL#21`). Values remain provisional.
   **The author accepted Crossfeed as the reference arena on 2026-10-25**
   ([record](polish-crossfeed.md#author-arena-acceptance-2026-10-25)); its
   unreviewed spawn-pressure and readability checks move to `POL#24`.

UI artwork follows the gameplay style, as before. Audio, release and engine
work are outside this sequence and keep their own rows.

**Stage approval (2026-10-20).** The author approved the current painter and five
tools (`POL#7`), player/shot cues (`POL#14`) and environment kit (`POL#9`) for this
stage. Their current-arena review rows are complete. The small-view treatment,
static landmarks and retained parallax comparison are implemented; shipped
backdrops remain static. The latest sequence opens the researched new-map pass
(`POL#17`), followed by coupled movement / weapon tuning (`POL#3`, `POL#27`). Re-check the
treatment in the new composition during that work and the reference-match
finish review (`POL#24`); future iteration remains expected. No new test matrix,
device session or detailed observations are implied by this approval.

`POL#16` stays deferred behind the blockouts. Listening/mix choices (`POL#23`),
physical-pad sessions and attended performance/hardware acceptance (`POL#25`)
retain their own gates. Supporting implementation choices in TODO's NOW can
proceed when they do not compete with the blockout work.

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
| Figures and teammate identity | Variant E, rebuilt from parts on 2026-10-18 with no teammate accent (below) |

These names refer to the [round-2 visual study](../game/art/references/style-board/index.html)
([original artifact](https://claude.ai/artifact/9J5kQgRMvVtXPe6r6cJ7vP)),
supplied by the author on 2026-10-15 and retained with the
[production sources](../game/art/README.md). The names do not specify numeric colours, sprite dimensions or
finished assets. The workshop kit and its pixel-grid choices remain historical
comparison evidence, rather than the production art standard.

The [classic Paint research](classic-paint-research.md) (2026-10-17) records what
Windows 3.1 Paintbrush and Paint 95/98 looked like, their tools and their pixel
facts, and lists the open decisions on making the style achievable in a real
paint program.
The author's choices from it: assets are achievable in **Windows 98 Paint**, in
24-bit colour, with at most **28 colours per level**, characters, team paint and
HUD included, and no dithering by default. The match's random team pair takes 2
slots, leaving 26 for everything else, and those 26 must work with every pair.
The author chose the greys-plus-muted-hues family, **Muted Day**, on 2026-10-18;
the palette below settles the earlier mockup question.

**Figure and level palette chosen by the author on 2026-10-18.** In the
[box-head builder](https://claude.ai/artifact/47ANdGfpipyeFa1W527cJq) the author
picked this build, each part a few strokes in Windows 98 Paint:

| Slot | Part | Drawing |
|---|---|---|
| Head | `h_ears` | 9x8 white rectangle with a 1x2 ear either side, level with the eyes |
| Hat | `t_cap_back` | 9x4 team-colour cap, top corners erased, brim line running 3 px past the back |
| Eyes | `tall` | two 1x2 vertical strokes, columns 5 and 9 |
| Mouth | `smirk` | 3 px line, one pixel up at its right end |
| Body | `b_overalls` | 9x9 team box, white shirt in the top 3 rows leaving two team straps, 1 px leg split in the bottom 2 rows |
| Arms | `a_stub` | 2x2 team sleeves with a black edge, 2x2 white hands, front hand one pixel forward |
| Legs | `e_tubes` | the current outlined 3 px team tubes with black boot rows |
| Teammate mark | none | |

There is no teammate mark: teammates already share the team colour, so the
bobble/antenna/fork/flag head accents are withdrawn. Characters use black,
white, the team colour and the level's dark grey only.

The level palette is **Muted Day**, the greys-plus-muted-hues family, which
settles the question above. Its 26 non-team roles:

| Role | Hex | Role | Hex | Role | Hex |
|---|---|---|---|---|---|
| keyline | `000000` | pencil_body | `9A9186` | metal_dark | `6E7275` |
| paper | `C9D3D9` | pencil_shade | `756E65` | shadow | `9CA2A2` |
| bg_far | `C3C8C0` | pencil_wood | `C9AE8C` | shadow_deep | `7E8382` |
| bg_mid | `A7B09E` | graphite | `3E3E40` | face | `FFFFFF` |
| bg_line | `8E958C` | ruler_fill | `B5AFA3` | hud_paper | `F4F2EC` |
| bg_detail | `F2F1EA` | ruler_marks | `45423E` | hud_ink | `2A2A2A` |
| ground_fill | `8C8A86` | eraser_fill | `B8958C` | highlight | `FFFFFF` |
| ground_top | `B4B1AA` | eraser_shade | `8F716A` | prop_accent | `945E52` |
| ground_shade | `6A6865` | metal_light | `B8BCBE` | | |

The team pairs stay blue/orange, pink/green and blue/yellow. The production
painter is drawn to this build ([painter](polish-painter.md#the-authors-build-2026-10-18));
the Muted Day palette is applied by the [terrain kit](polish-terrain.md) (`POL#9`).

Build the gameplay style first: a small reference containing a character, two
contrasting weapons, a painted platform, background materials and effects.
Inspect it at actual gameplay sizes, especially four-player split-screen. Carry
the accepted treatment into the HUD and menus after the gameplay composition
works.

Record the production details as the reference is built:

- Character proportions, weapon scale and team-colour regions.
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

**Current sequence (author, 2026-10-20):** improve the Close Quarters layout with
the existing camera framing and measured movement baseline first. Once the initial level pass is
playable, tune movement on that layout and the course, then make small level and
movement adjustments from play observations. The course supplies measurements
and comparisons for that process.

Measure the resulting jump envelope and travel speed. Use those measurements
when placing platforms; the old movement note is not a level-design specification.
Tune values through keyboard/mouse and controller playtests. They stay
provisional, retuned as level design, art and audio develop, and nothing waits on
fixing them (author, 2026-10-17).

**Exit condition:** required jumps are reproducible, input feels responsive, and
buffered input cannot cause extra jumps after pause, death, respawn or dropping
through a platform. Capture comparable before/after clips and record chosen values.

## 3. Redesign the levels in playable blockouts

Level design is a full workstream. Revise geometry, routes, scale, spawn areas and
the distribution of paintable surface before producing finished arena artwork.
Use simple geometry in the [existing editor](level-editor.md), save experimental
copies, rebuild, and judge them in the game. The editor does not simulate play.

The [Close Quarters first playable increment](polish-close-quarters.md) is a
preserved earlier experiment with driven-route and exposed-paint evidence.
The author's clarified deliverable is an original **new map** informed by
[research](level-design-research.md), followed by coupled movement / weapon
tuning. Passing CQ routes do not establish suitable spacing, density, inputs
or weapon scale. Crossfeed, not CQ, is the accepted reference arena (2026-10-25).

### Design requirements

| Area | Requirement and how to judge it |
|---|---|
| Routes | Provide a clear route into the contest and a meaningful alternative. Flanks should reconnect to useful space; avoid long empty corridors, accidental dead ends and a single mandatory choke. |
| Movement | Base gaps, steps and clearances on the measured movement course. Ordinary routes should tolerate normal input variation; optional shortcuts may demand more skill. Test both directions. |
| Verticality | Use height for route choice and tradeoffs. Important high ground needs a practical counter-route and escape options; it should not safely cover every route and spawn. |
| Spawns | Keep four valid spawn positions per team with full character clearance. Give players room to orient and routes into play that an opponent cannot lock down from one firing position. Test repeated respawns under pressure. |
| Painting | Ordinary floors and platform tops accept paint by default, including home areas; exceptions need a clear material or gameplay reason (author, 2026-10-24). Budget reachable paint by region, including walls and ceilings; assess travel cost and repaint opportunity rather than excluding home floor. |
| Weapon roles | Give short-range and roller play useful connected surfaces, and ranged weapons useful but contestable sightlines. Test all five tools at current values during the first layout pass and record role issues; tune their numbers (`POL#27`) as layout and movement settle in iteration. |
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

These briefs predate the author's research-first / new-map clarification.
Preserve the existing maps and CQ draft. With Crossfeed accepted as the reference
arena (2026-10-25), `POL#21` takes King of the Hill and Turbulence through the
same process: write a short brief per layout against the research criteria,
build the blockout in the editor, then measure routes, paint by region, spawn
approaches and side swaps as Crossfeed did. Use supported structures, platforms, ramps and
visuals; evaluate any new gameplay object as a separate, evidenced requirement.

**King of the Hill (2026-10-25):** the blockout is [Press Tower](polish-press-tower.md),
a new explicit-only stage (`press_tower`) beside the untouched `king_of_the_hill.json`,
which stays selectable and in Random. Its baseline acceptance case passes; attended play,
native four-pane captures and the brief's support art remain open there. Turbulence has
not started.

The first new-map pass uses current movement and weapon values. Define required,
optional and recovery routes independently from their input families. Reconcile
paint by region and face direction, and measure real firing / escape windows.
Then tune movement and all five weapons together as play exposes specific issues.

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

The kit is part of the current approved art baseline
([development order](#development-order-2026-10-17)): materials, props and
background layers follow the art rules, and are fitted to Close Quarters as
its layout develops.

**First increment implemented 2026-10-18 (part of `POL#9`).** The
[Tool Props terrain kit](polish-terrain.md) draws every collision object in the
four arenas as drawn stationery in the Muted Day palette: tiled ground, pencils,
erasers, a paint tin, rulers for jump-throughs and set squares for ramps, each
inside its collision shape and outlined only where exposed. Airbrush speckle
hangs under floating platforms, and a crude drawn backdrop (paper sky, hills,
trees, a house, clouds, a sun, birds) replaces the dark void. The warehouse and
industrial directions in the briefs above predate the Paint.exe rules: Tool Props
governs the terrain everywhere, and each arena's subject (Close Quarters' paint
workshop and supply warehouse) becomes its own backdrop and landmarks when its
blockout is fitted. Reusable static landmarks and the parallax comparison are
also implemented; the author approved the current kit for this stage on
2026-10-20. Further room detail remains optional, and CQ composition is
rechecked with `POL#17`/`POL#24`. The monitor palette now borrows the same kit swatches
([palette coherence](polish-terrain.md#monitor-palette-coherence-2026-10-18)),
replacing its old grey ramp while keeping the final empty story slot.

**Shared paintability finish (2026-10-24).** [Smooth white primer and dark sealed
edges](polish-terrain.md#paintable-and-sealed-faces-2026-10-24) distinguish scoring
faces across all five levels, including paintable one-way rulers and individually
enabled sides or undersides. This drawing pass preserves each level's authored
paintability; recognition at speed remains part of the human arena review.

**Quiet cloud drift implemented 2026-10-18 (part of `POL#9`).** The
[existing backdrop clouds](polish-terrain.md#quiet-cloud-drift-2026-10-18) now
move slowly in bounded, independently phased cycles on the four-unit grid.
They freeze with the world, stay stationary under Reduce effects and keep
their full excursion inside their cull bounds. The current kit is approved for
this stage; signage wording, further room life and any runtime parallax remain
separate choices. The retained comparison recommends keeping backdrops static.

### Characters and distinctive weapons

The [production painter increments](polish-painter.md) implement `POL#7`'s artwork:
the painter (redrawn to the author's 2026-10-18 build), animated team masks,
team sleeves and continuous tool grips, with
idle/run/jump/fall and dedicated takeoff/landing/firing/hurt/elimination clips,
plus a matching respawn redraw. The author approved the current treatment for
this development stage on 2026-10-20.
The [five-tool production pass](polish-tools.md) implements `POL#8`: distinct
Paint.exe designs, aligned paint masks and grips, paid-shot pressure recovery,
hose motion, supported roller rotation/lift/settle, bucket slosh/handle motion
and a detached cosmetic mist tail. The current-arena style review is complete;
re-check Close Quarters' composition as `POL#17` develops and during `POL#24`.
The projectile, contact and quiet-detail increments below extend
that treatment without changing the match simulation.

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

**Quiet detail implemented (2026-10-15, `POL#12`).** A settled, alive sprayer
waits a locally varied 2.8–6.2 seconds, grows a team-paint bead for 0.42 seconds
and stretches it for another 0.18 seconds at the rendered nozzle's underside.
The detached drop falls vertically under cosmetic gravity for at most 0.75
seconds. Sweeping its footprint against actual terrain shapes ends it on
contact, including thin platforms and ramps. There is no contact splash or
sound, ammunition use, damage, paint capture or player credit.

Movement, jumping, held fire (including an empty tool) and elimination cancel
the attached bead and restart the quiet wait. Detached drops finish independently
of the player and aim. Each sprayer has a scene-owned, bounded cosmetic object;
Level advances it once after resolved contacts and the final muzzle repair.
Scene culling uses the drops' own bounds, so elimination or leaving the player's
view does not prematurely erase them. Pause freezes the state; time-up clears
the drips before the reveal hides the tools, and teardown owns their cleanup.
The private cosmetic sequence never draws from gameplay randomness.

Every painter also has individually phased breathing and weight shifts after
settling, occasional short blinks, and a restrained acceleration/turn lean that
settles after stopping. These reuse the coarse drawing and neutral fill rather
than replacing its action clips. Compression keeps the feet baseline, and the
same posed canvas supplies the shoulders while hands retain
the actual tool grips. Actions, airborne motion, elimination and story/redraw
poses suppress inappropriate idle details. All sizes and timings are in
`presentation.json` (`idle_drip` and `figure.quiet`). Their current treatment is
approved for this stage; re-check the reference composition in `POL#17`/`POL#24`.

Validation on 2026-10-15:

- Debug and Release build under `/W4 /WX` and pass all 17 consumer cases.
  The new drip case covers private timing, attachment, gravity, true ramp/thin
  platform contacts, interruption, death/respawn, pause and time-up cleanup.
  Enabled/disabled comparisons preserve actual Level paint scores and gameplay;
  character comparisons preserve all five tools' shots, movement, health,
  ammunition, aim and muzzle. Invalid character geometry/timings are rejected.
- Both builds pass the D3D11 painter probe on the NVIDIA GeForce RTX 5080.
  Each adds 46 quiet-detail world captures: one/four players in all three palettes
  at 720p, plus representative stretch poses at 1080p and QHD. The mature bead's
  raster touches the real tool lip; detached drops separate. Growth, stretching,
  falling, breathing, blinking and both movement directions are captured.
  Repeated frozen draws preserve pixels, cosmetic and simulation state, and Debug
  replay fingerprints; isolated drip pixels stay within their culling bounds.
- Isolated Debug and Release startup pass with the accepted bank, a system-only
  PATH and graceful shutdown. No audio or atlas changes are part of this increment.

CPU evidence is under `out/acceptance/`; final visual/startup evidence is under
`out/validation/quiet-detail-final/`. Visual run IDs are
`x64-debug/e7ea6854f2d24c5a959ed277486c98ef` and
`x64-release/098a1cd87d2e480daa989b7579d0f2c0`; their metadata records the baseline
revision, worktree changes and pinned engine. The retained
[quiet-detail contact sheet](images/polish-quiet/quiet-contact.png) contains
native-size renderer crops. These checks and image inspection leave human
composition, movement-feel, controller and listening acceptance in their existing rows.

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

**Projectile and surface-paint production pass implemented (2026-10-15,
`POL#11`).** The [implementation and evidence](polish-paint-effects.md) record
distinct liquid shots and impacts for all five tools, outward splash droplets,
continuous black paint edges, wet settling and immediate repaint tint with
unchanged coverage and credit. The roller remains a contact effect. Source
artwork is reproducible; Debug/Release consumer and hardware renderer checks
cover all palettes and 720p/1080p/QHD four-player views. The current treatment is
approved for this stage; reference-arena composition and human readability are
re-checked in `POL#17` and `POL#24`.

**Player contact feedback implemented (2026-10-15, part of `POL#10`).** The
[implementation and evidence](polish-combat-feedback.md) add a short, local
white/keylined hurt accent and a detached, team-coloured elimination burst to
the existing action poses and accepted sounds. Repeated hits rate-limit the
accent, and the existing spawn redraw remains the arrival treatment.

**Movement contact particles implemented (2026-10-15, part of `POL#10`).** The
[implementation and evidence](polish-movement-contact.md) add restrained,
black-keylined step and reversal flecks on the existing audio stride. Painted
support supplies its actual colour and contact point, including adjacent-floor
seams and enemy paint. The effects remain cosmetic and bounded.

**Firing and paint-supply accents implemented (2026-10-15, part of `POL#10`).**
The [implementation and evidence](polish-weapon-feedback.md) add distinct paid-shot
muzzle phrases to the four ranged tools, a rate-limited empty-paint response and
a brief acknowledgement when a complete shot's paint regenerates. The roller
keeps its contact swash. Aim, firing cadence and paint ownership are unchanged.

**Optional local view shake implemented (2026-10-15, part of `POL#10`).** The
[implementation and evidence](polish-view-shake.md) add a restrained, bounded
world-camera accent on accepted damage, elimination and heavier landings.
Each player's pane reacts only to that player; the base aiming camera, HUD,
collision and scoring remain stable. Controls / audio exposes a saved View
shake switch from the main menu and pause. The combined
[reduced-effects preference](polish-reduced-effects.md) is implemented under `POL#13`.

**Shared paper focus and press motion implemented (2026-10-16, part of
`POL#10`).** Title/Home, Options/Credits, Controls / audio, Pad sticks,
Pause/confirmation and End use bounded ornament motion in `MenuFocusMotion`
and `MenuPaperArtwork`. Moving to another visible, enabled focused widget
briefly offsets the detached arrow inward before it settles; initial focus
starts settled. On retained pages, a successful activation such as Apply or a
permitted value adjustment depresses the focused paper face into its existing
shadow, then releases. Repeated accepted presses restart one response rather
than stacking effects.

Labels, the complete card envelope and widget/navigation bounds retain their
coordinates. Actions, transitions and the existing navigation, confirm and cancel
sounds remain immediate. Title proceed, transitions and pops take effect before
an outgoing-card press can be shown. [Shared presentation controls](menu-presentation.md#shared-paper-focus-and-press-motion)
set the durations, arrow travel and press fraction when a page is constructed.
Applied Reduce effects retains the settled drawing immediately. Suspension,
deactivation, resuming and developer input reset clear interrupted motion;
drawing does not advance it. Results has no shared cursor and stays static.

Verification uses the `menu_focus_motion` consumer case and
`scripts/check-painter.ps1 -Preset x64-debug -MenuMotionOnly` (or `x64-release`),
with evidence under `out/validation/2026-10-16-menu-motion/verification.json`.
Both builds, all 31 consumer cases per preset, ten relevant native menu/resume
scopes per preset and both published-bank startup checks pass. The 60 native
motion PPM captures match between Debug and Release.
Human review of the shared paper motion was accepted with `POL#10` (2026-10-17).

**Setup choice motion implemented (2026-10-16, part of `POL#10`).** Mode and
Player count settle their focus accent inward within the existing padding and
acknowledge accepted value changes with the same bounded press clock. Team
selection contracts the chosen team icon inside its existing canvas; weapon
selection contracts only the tile and border within existing icon padding.
Each player row owns one response for actual choice changes and successful
ready/unready changes. A successful team shuffle acknowledges every row,
including an unchanged team. Initial choices are settled, repeated input
restarts one response, and rejected actions and endpoints do not fake success.

The [shared presentation controls](menu-presentation.md#setup-selection-motion)
capture durations and the new `selection_inset_fraction` (`0.04`, accepted
range `0` to `0.08`) when the page opens. Tool art, paint masks, supply art,
lettering, badges and input geometry retain their positions. Applied Reduce
effects immediately preserves the settled drawing. Active-time updates advance
the clocks; missing-player and inactive gates, suspension/resume, deactivation,
developer interception and input reset clear interrupted responses. Input
callbacks, readiness, shuffle/random resolution and transitions retain their
existing timing, including the final confirmation's immediate advance.

Verification extends the existing `menu_focus_motion` consumer case and adds
`scripts/check-painter.ps1 -Preset x64-debug -SetupMotionOnly` (or
`x64-release`); the batch evidence is
`out/validation/2026-10-16-setup-motion/verification.json`.
Both builds, all 31 consumer cases per preset, seven relevant native menu/resume
scopes per preset and both published-bank startup checks pass. The 96 native
setup-motion PPM captures match between Debug and Release.
Human review of the shared paper and setup/per-player motion was accepted with
`POL#10` (2026-10-17).
Reference-arena, movement-feel, controller and listening acceptance retain their
existing gates.

**Join and arena-stage motion implemented (2026-10-16, part of `POL#10`).**
Join retains one clock and gutter accent per stable player row. An accepted
claim or release briefly slides and shrinks a thin stripe within the row's
existing left focus padding. Responses start after the final roster refresh;
a release and replacement in the same row produces one bounded response.
Empty rows retain their positions. Connection and input-slot changes clear
interrupted responses quietly, without acknowledging a join that did not occur.
The stripe is absent at rest and with applied Reduce effects.

Arena browsing settles only the existing briefing stripe inward; successful
lock and unlock briefly press that stripe. Captured focus/press clocks and the
existing accent width set the response, clamped within the padding above the
briefing text. Repeated events replace one bounded response. The final Random
confirmation retains its outgoing Random view and music exit; resolving the
stage adds no motion response or visual refresh and preserves the existing
single Random selection draw. Accepted final Start settles any unfinished lock
or browse response before the outgoing fade, without refreshing the displayed
Random text/preview or adding a new pulse.

The [Join and arena-stage presentation boundary](menu-presentation.md#join-and-arena-stage-motion)
uses no new JSON fields or artwork assets. Lettering, status, heading, roster,
preview and widget/input rectangles retain their positions. The existing
readiness snapshot, final-claim barrier, Player 1 Back behavior, confirmation
and transition timing remain unchanged. Both pages use the shared registered
interruption policy for inactive/missing-player gates, suspension/resume,
deactivation, developer interception and input reset. Drawing never advances
the clocks; applied Reduce effects immediately retains the settled result.

Verification adds
`scripts/check-painter.ps1 -Preset x64-debug -JoinStageMotionOnly` (or
`x64-release`), with batch evidence at
`out/validation/2026-10-16-join-stage-motion/verification.json`.
Both builds, all 31 consumer cases per preset, seven relevant native menu/resume
scopes per preset and both published-bank startup checks pass. The 78 native
Join/Stage PPM captures match between Debug and Release; 45 moving-envelope
comparisons per preset keep every changed pixel inside the authored padding.
The unchanged resume check also verifies rapid final confirmation followed by
focus loss during the outgoing music fade.
All planned menu-focus and selection-motion surfaces are implemented. **The
author accepted them on 2026-10-17** ("menus feel good"), across the shared
paper, setup/per-player choices, Join and arena-stage selection, closing `POL#10`.
Reference-arena, movement-feel, physical-controller and listening acceptance
retain their existing gates.

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

**Reduced effects implemented (2026-10-15, `POL#13`).** The saved
[Reduce effects preference](polish-reduced-effects.md) overrides optional view
shake, omits muzzle bursts/refill rays and quiet figure/drip/released-mist detail,
and retains gameplay contacts, damage, paint, identity and readiness cues. Options
and Pause share its Apply/Back behavior. The replacement audio already supplies
independently cycled, bounded footstep, landing, wet-contact and splash variants.

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
lost/back cues, and a completed-series cue. The shared resume count-in now uses
the accepted `ui.count_in` beat. The accepted results coda and each arena's
ending sting are integrated. Arena music now fades in, fades under its ending
sting and ducks through pause/reconnect/settings and the resume count-in.
Menu music now has one owner across setup and nested settings; menu/results
entry and exit fades preserve their outgoing page until the handoff. Optional
detail assessment and listening remain open.

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

Carry the accepted Paint.exe language through menu artwork, player/weapon cards, stage
preview, HUD, countdown, pause and results. Review typography at actual display
sizes. Preserve the existing clear focus indicators, labels and controls reference.

Improve stage entry, match start, closing countdown and the animated winner reveal.
Show player contributions clearly and make the rematch route easy to follow.
Measure loading pauses before choosing transition behaviour. Preserve the deliberate
withholding of live coverage percentages so the ending remains a reveal.

<a id="level-build-pause-measured-2026-10-17"></a>
**Level build pause measured (2026-10-17, `GAP#5.1` step 0).** The audit's freeze
(no frame between stage select's last draw and the match's first,
[gap-audit 5.1](review/gap-audit.md#51-every-screen-change-is-a-hard-cut-and-the-level-build-freezes-on-the-last-menu-frame))
is now timed. Debug's [`perf`](debug-console.md#perf) prints `build <ms>  <stage>  <n>P`:
wall time from the end of the director's `before_level_build` to the top of
`after_level_build`, which is the layout plus `LevelBuilder::build_level`. Release
has no console, so its figures come from a one-off harness (not kept). It links the
Release game objects and times `build_level` on the device-free content the
acceptance cases build with: 7 builds per stage and count, 3 runs. The same harness
in Debug reads about 10% under the in-game Debug figures, so building without a
device leaves nothing material out. Milliseconds, Ryzen 9 9950X3D:

| stage | Debug in game, 1P | Debug in game, 4P | Release, 1P | Release, 4P |
|---|---|---|---|---|
| `movement_course` | 6.6-7.2 | 6.7-6.8 | 1.5-1.8 | 1.4-1.8 |
| `close_quarters` | 3.6-3.7 | 3.6-4.0 | 0.8-0.9 | 0.7-1.0 |
| `king_of_the_hill` | 7.7-7.8 | 8.1-15.0 | 1.5-2.0 | 1.5-1.9 |
| `turbulence` | 8.6-8.8 | 8.7 | 1.7-2.4 | 1.7-2.2 |

Debug is the first build and two `restart`s
(`+stage <s> +players <n> +wait frames 3 +perf +restart ...`); the 15.0 is a first
build. Release is min-max over 21 builds. Every Release build fits well inside one
60 Hz frame, even on a machine several times slower, and Debug drops at most one
frame. The "many frames" freeze is not there, so no loading card is needed. Not
in these numbers is the match's first draw after the build, which `perf` treats as
an interval start. What remains of `GAP#5.1` is the hard cut itself: a transition
look in the Paint.exe language (a fade, a wipe, a redraw or a deliberate cut) is UI
art for a later pass using the approved gameplay style. Separately, the startup
window is thought to show a blank client area during the asset walk (about 53 ms in
Release, an unretained timing). Labrador answered
[report 19](review/engine-reports.md#19-no-public-later-show-convenience-for-a-hidden-application--needs-more-evidence-cacf112)
"needs more evidence": it calls the white flash inferred, not observed, and wants two
captures, recorded with commit and date: a default-visible `x64-release` launch from
`ShowWindow` to the first present, and a hidden launch shown natively after the first
state is constructed. If the first shows a flash and the second cures it, the engine's
preferred fix is its own (show after the first `end_frame`, no class-brush paint) and
needs no client call. Those captures are this repository's to make (`GAP#5.1`).
Re-measure with `perf` when the blockouts (`POL#17`, `POL#21`) add geometry: the
build makes paint tiles for every paintable face.

**Takeoff/landing production puffs implemented (2026-10-15, part of `POL#16`).**
The [drawing and evidence](polish-feedback.md#paintexe-production-puffs-2026-10-15)
replace the prototype squares with neutral, stepped, black-keylined silhouettes:
a rising takeoff crown and a lower, wider landing cloud. Existing timing,
impact strength, movement and paint ownership are unchanged. These shared cues
apply to all five tools; the reference-arena composition and match/UI treatment
remain open.

**Sniper/bucket quiet paint implemented (2026-10-15, part of `POL#16`).** The
[tool detail and evidence](polish-tools.md#sniper-residue-and-bucket-runoff)
extend the sprayer's cosmetic residual paint to a restrained precision-nozzle
bead and runoff from the bucket's wet rim stripe. Each has its own authored
attachment, size and quiet interval. This completes those two quiet-tool
details from §4; stage-entry, opening/closing accents and the remaining match/UI
treatment still belong here.

**Monitor scene artwork implemented (2026-10-15, part of `POL#16`).** The
[drawing and evidence](polish-monitor.md) give the physical computer around
the opening and reveal a stepped black keyline, a pale upper edge, airbrush
shade and small casing details. Notes and other bezel decorations draw over
the picture, including previews, the countdown push, pullback and the
closed-program ending. The story and picture layout keep their existing
behaviour. The remaining opening/closing accents and match/UI treatment stay
open; this does not establish reference-arena composition acceptance.

**Person and cursor artwork implemented (2026-10-15, part of `POL#16`).** The
[foreground drawing and evidence](polish-monitor.md#person-and-cursor-foreground)
complete the same monitor composition with a coarse dark silhouette contour,
restrained neutral rim and edge shade. Adult, child and mug personas keep their
existing poses. The shaded coarse-cell pointer keeps its tip on the selected
swatch or close button; brief detached click ticks follow the existing reveal
click and are suppressed by Reduce effects.

**Countdown artwork implemented (2026-10-15, part of `POL#16`).** The
[start-signal drawing and evidence](polish-countdown.md) replace the font-based
number/GO signal with coarse, black-keylined strokes, a stepped paper card and
quiet offset shade. A brief settling pulse and detached pen ticks follow the
existing countdown clock; Reduce effects keeps the settled signal. The shared
device reminders, story push, sound cues, control-release boundary and one-second
GO beat retain their behaviour. Closing/reveal accents and the remaining
interface treatment stay open.

**Closing and reveal accents implemented (2026-10-15, part of `POL#16`).** The
[drawing and evidence](polish-match-accents.md) give the final ten displayed
seconds a stepped pale clock card with black ink and brief detached pen ticks.
The reveal's exact team-labelled percentages appear on matching paper cards
with team-coloured, ink-outlined lettering only after the existing choice click.
Their local ticks follow that click; Reduce effects retains both settled cards.
Match clocks, sound cues, coverage, story timing and the solo overview keep their
existing behaviour. Menus, cards, stage preview, the remaining HUD, pause and
results treatment remain open, as does reference-arena composition acceptance.

**Fresh wall runoff implemented (2026-10-15, part of `POL#16`).** The
[surface detail](polish-paint-effects.md#wall-runoff) adds a short, downward
bead and wet trail within contiguous, already-owned wall paint. Each exposed
vertical strip has one bounded slot; opposing/neutral paint and occluded edges
stop it. Reduce effects hides it. Paint coverage, credit and collision remain
exact. Remaining arena and interface work keeps its existing order.

**Reservoir paint supply implemented (2026-10-15, part of `POL#16`).** The
[sprayer and mister windows](polish-tools.md#reservoir-paint-supply) now lose
and regain liquid rows from actual ammunition, retaining opaque neutral empty
interiors and permanent wet nozzles. Full-canvas layers preserve registration
through continuous aim, both facings and camera scaling. Reduce effects keeps
this information; selection previews show full reservoirs. Gameplay and the
remaining arena/interface work retain their existing behaviour and order.

**Stage-entry header implemented (2026-10-15, part of `POL#16`).** The
[entry drawing and evidence](polish-countdown.md#stage-entry) add the loaded
arena's name on a stepped paper header during the existing start countdown.
It fits above the signal in every pane layout, clears at GO and remains static
under Reduce effects. Solo practice and story skipping retain their existing
timing. Menus, cards, stage preview, the remaining HUD, pause and results stay
open, with arena production and review retaining their existing gates.

**Live HUD artwork implemented (2026-10-15, part of `POL#16`).** The
[per-pane paper drawing and evidence](polish-hud.md) carry the countdown's stepped
paper, dark keyline and quiet shade through player labels, health/paint meters
and the ordinary match clock. Labels, roster/device identity, meter quantities
and clock placement retain their existing meaning. The drawing is static under
Reduce effects; the final-ten clock keeps its existing brief ticks. Menus,
cards, stage preview, pause and results remain open, with reference-arena
production and human review retaining their existing gates.

**End-menu artwork and primary continuation implemented (2026-10-15, part of
`POL#16`/`POL#18`).** The [end-of-round page](#make-couch-play-comfortable-between-actions-and-rounds)
uses stepped pale paper, dark ink, offset shade and a persistent continuation
card. A detached ink arrow identifies the shared focus. Single matches offer
Rematch first; series keep their round-specific continuation. Remaining menus,
cards, stage preview, pause and results retain their existing owners, along
with the arena and human-review gates.

**Pause and confirmation artwork implemented (2026-10-15, part of `POL#16`).**
The [pause-page drawing and evidence](polish-pause-menu.md) carry the stepped
paper, dark ink, soft shade and detached focus arrow through ordinary pause,
controller loss and the Restart/Quit questions. Disabled Resume/Restart rows
have separate readable ink; only enabled focus gets a selection card.
The opening device, fallback navigation, safe No default and shared resume
count-in retain their existing rules. Remaining menus, selection cards, stage
preview and results stay open, along with arena production and human review.

**Results artwork implemented (2026-10-15, part of `POL#16`).** The
[results-page drawing and evidence](polish-results-menu.md) carry the same
stepped paper, dark ink and soft shade through the score panel and labelled
percentage cards. Centered headings and series lines use actual font widths;
the score bar has a neutral unpainted track and a dark keyline. Competitive
shares, solo arena coverage, device-owner contributions, reveal/continue timing
and series flow retain their existing rules. Point sampling keeps neighbouring
atlas colours out of the solid score track. Remaining menus, selection cards
and stage preview stay open, with arena production and human review retaining
their existing gates.

**Title and home artwork implemented (2026-10-15, part of `POL#16`).** The
[entry pages](polish-main-menu.md) carry the stepped paper, dark ink and quiet
shade onto a neutral surround. The title centers from actual font widths;
Home retains a persistent Play card and detached focus arrow. Existing copy,
actions, prompts and music retain their behavior. Options/Credits, match setup,
selection cards and stage preview remain open, with arena production and human
review retaining their existing gates.

**Options and Credits artwork implemented (2026-10-15, part of `POL#16`).**
The [settings pages](polish-main-menu.md#options-and-credits) extend the same
stepped paper, dark lettering, neutral surround and detached focus arrow.
Options cards include both label and value; disabled resolution and save
failure have readable separate inks. The complete authored Credits text,
staged changes, Apply, discard, fullscreen preset and Controls/audio return
retain their behavior. Shipped text fits at its natural font size, including
all palette names and the long save-failure status. Controls/audio, match setup, selection
cards and stage preview remain open, with arena and human-review gates unchanged.

**Controls/audio artwork implemented (2026-10-15, part of `POL#16`).** The
[shared settings page](polish-main-menu.md#controlsaudio) extends the stepped
paper, dark lettering and detached focus arrow to both Options and Pause.
Compact setting cards enclose each label and value; the complete controls
legend and Apply/save status fit beside and below the settings. Pending edits,
SFX audition, Apply, Back discard and the paused match retain their existing
rules. Match setup, selection cards and stage preview remain open, alongside
the existing arena and human-review gates.

**Mode and Player Count artwork implemented (2026-10-16, part of `POL#16`).**
The [setup-page drawing](polish-main-menu.md#mode-and-player-count) extends the
shared stepped paper, dark ink, neutral surround and detached focus arrow to
the two shared-choice pages. `main_menu.setup_box_size` supplies their common
footer-centered paper. A copy of `main_menu.art` supplies its colours and keyline;
the required `main_menu.setup_focus_padding` (`28, 7`) and
`main_menu.setup_shadow_offset` (`2`) override the card padding and shadow offset.
They preserve pressed-face/keyline clearance while the card with extra shade
clearance fits the existing 100-unit row spacing. The enabled focused row gets the shared card
and existing focus/press motion, with no persistent primary card. Reduce effects
retains the settled artwork. The legacy brand and stripe are suppressed on these two
pages while the controls footer remains visible.

Every existing text anchor, font, scale and logical widget bound is retained.
Text-shadow alpha is zero, preserving the offsets and measured extents while
drawing plain dark lettering. The duration row retains its label/value
positions and fitting path. Choice order, duration cycling, series/solo rules,
Back, device claiming, transitions, RNG and sound/music bindings are unchanged.
The drawing uses the existing solid texel and needs no new artwork asset.

Verification uses the focused `SetupPaperOnly` native scope, with evidence at
`out/validation/2026-10-16-setup-paper/verification.json`.
Both builds and all 31 consumer cases per preset pass, along with sixteen
native runs across eight relevant menu/resume scopes per preset. Each preset
passes 117 independent full-frame checks and 24 motion-envelope checks;
all 174 native PPM captures match between Debug and Release. Four startup
checks pass: the normal published-bank Debug/Release pair and extracted Release
with the published bank and optional-bank fallback under a system-only PATH.
The 53-file Release ZIP contains all 26 reviewed runtime assets byte for byte,
excluding source, debug and private audio; package evidence is in
`out/validation/2026-10-16-setup-paper/package-verification.json`.

Accepted arenas and human
review remain `POL#16`. Reference-arena, movement-feel, physical-controller
and listening acceptance retain their existing gates.

**Join artwork implemented (2026-10-16, part of `POL#16`).** The
[Join page](polish-main-menu.md#join) reuses the shared setup paper and captured
art overrides on the neutral Home surround, without new data fields or assets.
Its connected rows and bounded gutter acknowledgements use focus ink; reserved
missing and unclaimed rows use disabled ink. Heading and status use ordinary
dark ink. Join has no shared focus cursor, card or arrow. The controls footer
remains visible and the legacy brand is suppressed.

Transparent text shadows preserve fonts, offsets, anchors and logical bounds.
Actual generated row, heading, status and captured-gutter bounds must fit the
paper interior and retain their vertical order. Stable roster holes, claim and
release behavior, readiness barrier, Back/rescue, destinations, RNG and audio
retain their existing rules. Reduce effects and interrupted responses keep the
existing gutter-motion policy.

Verification uses the focused `JoinPaperOnly` scripted-reader scope, with evidence
at `out/validation/2026-10-16-join-paper/verification.json`.
Both builds and all 31 consumer cases per preset pass. The focused native pair
passes across six display sizes, with 216 independent full-frame comparisons
per preset and 162 PPM captures matching byte for byte between Debug and Release.
Seven related menu/resume scopes also pass in both presets, bringing the batch
to sixteen passing native runs; the [owner](polish-main-menu.md#join) lists them.
The four startup checks pass, including extracted Release with its published
bank and optional-bank fallback under a system-only PATH. The 53-file Release
ZIP contains all 26 reviewed runtime assets byte for byte and excludes source,
debug and private audio; package evidence is in
`out/validation/2026-10-16-join-paper/package-verification.json`.

Accepted arenas and human review
remain `POL#16`; reference-arena, movement-feel (`POL#3`), physical-controller and
listening acceptance retain their existing gates.

**Team Select paper and cards implemented (2026-10-16, part of `POL#16`).**
The [Team Select page](polish-main-menu.md#team-select) uses a wider shared-style
paper panel and static `focus_fill` cards enclosing each existing player label,
badge and three choice icons. The required `main_menu.team_box_size`
(`1840, 920`) preserves badge/card geometry, while
`main_menu.team_select.warning_position` (`150, 860`) places the existing
both-teams warning below the complete roster. Captured dark ink and transparent
text shadows retain the other fonts, anchors and logical extents. The controls
footer stays visible; there is no shared cursor or arrow.

The page reuses the setup art overrides and existing fill, with no new runtime
asset. Actual card bounds, row order, label/icon separation and warning placement
are checked against the usable paper interior. Atlas/frame choices, live
readiness colours, independent icon motion, Reduce effects, team rules,
shuffle/RNG, recovery and transitions retain their existing behavior.

Batch evidence is at
`out/validation/2026-10-16-team-paper/verification.json`.
Both builds, all 31 consumer cases per preset and the focused `TeamPaperOnly`
native pair pass. Each preset
passes 273 independent full-frame comparisons across six display sizes and
one-to-four-player rosters; fifteen moving cases cover thirty moving row
instances. The 132 PPM captures and CSV evidence match byte for byte between
Debug and Release. Eight related menu/resume scopes also pass in both presets,
bringing the batch to eighteen passing native runs; the
[owner](polish-main-menu.md#team-select) lists them. Four startup checks pass, including extracted Release with
the published bank and optional-bank fallback under a system-only PATH. Its
53-file ZIP contains all 26 reviewed runtime assets byte for byte and excludes
source, debug and private audio.

Accepted arenas and human review remain
`POL#16`; movement-feel (`POL#3`), reference-arena, physical-controller and
listening acceptance retain their existing gates.

**Weapon Select paper and tiles implemented (2026-10-16, part of `POL#16`).**
The [Weapon Select page](polish-main-menu.md#weapon-select) uses a wider paper
panel and stepped paper within the existing icon-border envelopes. The required
`main_menu.weapon_box_size` (`1840, 920`) preserves player-badge positions.
Captured setup art supplies the ink and shade; the tile retains its catalog's
sheet/fill, existing rule-width pen and live white/mint readiness face tint.
The original tile/border contraction remains cosmetic; tool, paint and supply
layers, lettering and input geometry stay fixed. No enclosing row card is added.
A fixed cream canvas preserves the preview's protected pixels beneath the
animated readiness surround, with bounded keyline bleed inside existing padding.

Dark lettering has zero-alpha shadows with its existing offsets and logical
extents. The authored description colour now ships as dark ink. Catalog/name,
description/font and artwork snapshots, neutral Random palette preview, final
Random RNG, readiness, recovery and transitions retain their existing rules.
Actual full-row bounds and maximum tile envelopes must fit the paper with
ordered rows, separate columns and positive moving faces. The drawing needs no
new runtime asset.

The focused `WeaponPaperOnly` scripted-reader scope has batch evidence at
`out/validation/2026-10-16-weapon-paper/verification.json`.
Both builds, all 31 consumer cases per preset and twenty native runs pass.
The focused pair passes 482 independent full-frame references per preset;
motion checks cover fifteen moving cases and twenty-one moving row instances. All 148
PPM captures and CSV evidence match byte for byte between Debug and Release,
with the outer profile preserved. Four isolated startup checks pass, including
extracted Release with the published bank and optional-bank fallback under a
system-only PATH. The 53-file Release ZIP contains all 26 reviewed runtime
assets byte for byte and excludes source, debug and private audio.

Accepted arenas and human review remain `POL#16`; movement-feel (`POL#3`),
physical-controller and listening acceptance retain their existing gates.

**Stage Select paper and briefing implemented (2026-10-16, part of `POL#16`).**
The [Stage Select page](polish-main-menu.md#stage-select) uses a shared-style
outer panel and a `focus_fill` briefing card, with the required
`main_menu.stage_box_size` (`1840, 920`). The briefing artwork encloses the
authored rectangle and actual text/status/hint bounds with pen/shadow clearance,
and follows actual refreshes without changing the authored layout or fit budget.
The new required `stage_select.selected_ink` supplies dark confirmed name/status
ink; the preview edge retains its live mint selection tint. The briefing label
and goal use captured focus ink; other static labels use captured ordinary ink.
Paper text has transparent retained shadows. Random's light question mark
and shadow remain on the existing dark preview.

StagePreview rendering, text/font snapshots, fitting, eligibility, browsing,
lock/unlock, recovery, final Random draw/outgoing view, stripe motion, input and
music handoff retain their existing behavior. Actual card containment and
preview separation are checked, without new arena content or runtime assets.

Batch evidence is at
`out/validation/2026-10-16-stage-paper/verification.json`.
Both builds and all 31 consumer cases per preset pass. The focused
`StagePaperOnly` scripted-reader pair passes 711 full-frame and 711 exact
preview references per preset, with fifteen moving cases, 123 pending-fade rows
and the immutable outer profile retained. All 246 PPM captures and CSV evidence
match byte for byte between Debug and Release. Ten related menu/resume scopes
also pass in both presets, bringing the batch to twenty-two passing native runs;
the [owner](polish-main-menu.md#stage-select) lists them. Four isolated startup checks
pass, including extracted Release with its published bank and optional-bank
fallback under a system-only PATH. The 53-file ZIP contains all 26 reviewed
runtime assets byte for byte and excludes source, debug and private audio.

Accepted-arena artwork and human presentation/readability review remain
`POL#16`. The current material/prop kit is approved for this development stage.
The [development order](#development-order-2026-10-17) starts with `POL#17`'s
level improvement, then movement tuning (`POL#3`), then small iterative changes
to both, with weapon tuning (`POL#27`) as the layout and movement settle.
Physical-controller and listening acceptance retain their existing gates.

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
  claiming performance acceptance. The [capture workflow](polish-frame-pacing.md)
  records bounded rendered intervals and machine/load metadata; hidden or shared-load
  diagnostics do not qualify the baseline.

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

**Aim and readiness prototype implemented (2026-10-15, part of `POL#14`).** The
[implementation and evidence](polish-aim-marker.md) add an optional local marker
to the four aimed tools. A chevron, recovery bars and a dry cross distinguish
ready, cooldown and insufficient paint using the same gates as firing, even
when fire is released. The marker follows nozzle direction; it predicts no hit
point and the fixed-aim roller has none. Controls / audio saves its On/Off
preference. The shared player badges are implemented below; human review in
the reference composition remains open.

**Bare-cover contact implemented (2026-10-15, part of `POL#14`).** The
[implementation and evidence](polish-blocked-shots.md) add a bounded, detached
stop tick and rebounding beads at the actual struck face when flying paint
hits cover without splashing a paintable tile. Launch contacts survive immediate
projectile deletion; flight contacts use the same cue. Existing wet splashes
remain the response on painted hits. The queue ages once per active tick,
freezes under pause and clears at time-up, without changing paint or firing.

**Shared player identity implemented (2026-10-15, part of `POL#14`).** A
monochrome pencil-cell `P1`–`P4` box uses the same
[badge drawing](../game/objects/player_badge.cpp) beside team/weapon selection
labels, in each pane's HUD and on the results contribution rows. The existing
device labels remain; teammates share the team colour and have no head
accent (author, 2026-10-18). Identity follows
roster/view order throughout; keyboard and sparse or reordered pad slots do
not choose the number, and results retain their input-slot paint attribution.

Each pane draws only its own figure's badge above the head. It stays
quiet during ordinary play, enlarges briefly at the countdown/live start,
actual respawn and return from a covering menu or focus loss, then settles.
The cue follows the same world camera as the character, including optional
view shake. Dead/redrawing figures, the story opening and the ending overview
have no local badge. Positive live ticks age the emphasis; pause and repeated
const draws cannot advance it. Reduced effects retain a steady, readable badge
without enlargement. Match timers, controls, shots and scoring are unchanged.

`presentation.json.player_identity` owns world/UI dimensions and spacing,
opaque contrasting ink, border, duration and emphasis scale. The shared box
needs no new atlas. The existing menu layouts carry the identity drawing;
the full UI artwork pass remains `POL#16`. The author approved the current
identification/readiness treatment for this stage on 2026-10-20; re-check it
once `POL#17` supplies the reference composition, with the finish review in `POL#24`.

Validation on 2026-10-15: Debug and Release build under `/W4 /WX` and each pass
all 21 consumer acceptance cases. The new case checks 1/2/4-player roster
mapping, countdown/live/death/redraw/natural and developer respawns, resume,
nonpositive updates, reduced effects and malformed presentation. Repeated
emphasis and disabled/enabled comparisons preserve actual movement, aim,
shots, ammunition, health, paint ownership and scores.

Both builds pass the focused hardware D3D11 identity probe on the NVIDIA
GeForce RTX 5080. All three four-player palettes cover identical-tool teammates
overlapping and crossing, quiet/enlarged/reduced badges, sparse/reordered
device slots, pause and death/respawn/story gates. Representative 1080p/QHD
captures check projected bounds. Same-state world-badge on/off differences
stay inside each owning badge rectangle; native `P1`–`P4` rasters differ, and
frozen draws preserve pixels, cosmetic state and Debug replay fingerprints.
The [retained four-player overlap capture](images/polish-player-identity/overlap-720p.png)
shows the quiet badges at 720p. Evidence is under
`out/validation/player-identity/` and `out/acceptance/`; focused run IDs are
Debug `ed17d825a8d24b179681195bf66b6660` and Release
`4977bb0d0fe74ac19d5fa04cc72c0f44`. Rerun with
`scripts/check-painter.ps1 -Preset x64-debug -IdentityOnly` (or `x64-release`).
These checks and image inspection do not establish human readability acceptance.

Isolated Debug, Release and extracted Release startup pass with the accepted
bank and a system-only PATH; the separate extracted missing-bank copy also
starts and closes gracefully with its single expected silent-bank diagnostic.
The 53-file Release ZIP contains all 26 reviewed runtime assets byte for byte
and no authoring files. The full Debug painter regression matrix also passes.

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

### Decide which Splatoon mechanics to borrow (`POL#30`)

**DECISION, answered with or after the coupled `POL#27` pass.** ColourWars
follows Splatoon on purpose in places (no live score, random pairs). The
[level research](level-design-research.md) lists the mechanics it does not
borrow as constraints on maps, but nothing records whether that absence is a
decision. A 2026-10-24 review named three. Each changes balance, so none should
be added ahead of `POL#27`.

- **Paint underfoot.** Standing on paint does nothing: no speed or ammunition
  recovery on your own colour, no slowdown or damage on the enemy's, and
  ammunition regenerates on a timer alone. This is the largest unexplored lever
  for making territory matter during the round instead of only at the count.
  The hook exists: the player already tracks the paintable structures
  supporting it.
- **A special charged by painting.** There are five primaries and nothing else.
  Splatoon uses a special to answer "why keep painting when nobody is here".
  The arena briefs currently answer that with geometry alone.
- **A splat tally on results.** Results show surface held only. A per-owner
  count on the contribution rows is small, since the damage path knows the
  projectile's owner. It also nudges the game toward the kill emphasis it has
  avoided ("eliminations make space to paint", below).

The decision may be "no" for any of them. Record it here either way, so a later
gap list reads the absence as intended.

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

**Shared resume count-in implemented (2026-10-15, part of `POL#18`).**
[Closing pause](../game/states/game_states.cpp) with Resume, including after a
reconnect, shows a shared **RESUMING 2–1** paper signal over the frozen player
panes. Returning focus to live play starts the same count. The existing opening
countdown and ending retain their own timing; Restart builds a fresh round and
Quit leaves immediately. Returning a pad still requires explicit Resume.

`presentation.json.pause_menu.resume_countdown` owns the duration (two seconds
by default, greater than zero and at most three). `ResumeCountdown` gates the
whole `Level::update` call and the developer clock/seat filter, so physics,
match time, ammunition and health regeneration, shot cooldowns, respawn and
cosmetic timers receive no count-in time. A long final tick never spills its
excess into play. Resize and repeated draws refresh presentation without
advancing the world. The signal reuses the accepted countdown drawing;
Reduce effects keeps its settled, readable form. Player badges emphasise the
characters at entry and again when play resumes.

A fresh pause can interrupt the count. A missing participant reopens reconnect
pause; focus loss freezes the world and starts a full count after return,
including focus changes under a covering menu. Movement and aim remain held
intentions. Jump presses, fire and drop-through held across the last beat need
release before rearming. A jump already in flight retains its held extension;
a neutral jump observed during the count permits a fresh press on the first
live tick. The Start/Escape press used to leave a menu cannot reopen it on
release. Old weapon loops stop before audio resumes, while a suspended redraw
keeps its position.

The accepted `ui.count_in` sounds once per displayed positive second, using
current SFX gain/mute. Its owned voice stops on completion, re-pause, focus loss,
rebuild and teardown; missed beats never queue. Match music and player voices
stay suspended until the count completes. The bank's wave bytes are unchanged.
Physical-pad play still needs to establish whether the brief wait prevents
surprise starts without making frequent pauses tiresome (`ROAD#2`).

Validation: Debug and Release build under `/W4 /WX` and pass all 22 consumer
cases. Focused final movement and recording-audio reruns cover count-in
release/rearming, unchanged airborne arcs, content rejection, beat ownership,
gain/mute, cancellation, silent-bank playback and stale firing-loop cleanup.
The actual `GameLevel` hardware probe passes in both builds on the NVIDIA
GeForce RTX 5080. Keyboard/mouse drives real pause pages in 1/2/4-player layouts;
other seats are idle fixtures. It checks re-pause, live and covered focus return,
Restart/Quit, held versus fresh actions, nonpositive time and oversized final
ticks. Pending health/ammo recovery, shot readiness, flying paint, dead-player
respawn/redraw, cosmetics and underlying world/HUD pixels remain frozen through
the gate. Shared paper bounds and frozen redraw pass at 720p, 1080p and QHD,
including a 4P720 second beat with Reduce effects. Retained captures show the
[shared 2](images/polish-resume/resume-4p-720p.png) and
[settled reduced-effects 1](images/polish-resume/resume-4p-reduced-720p.png).

The 53-file Release ZIP byte-matches all 26 runtime assets. Isolated Debug and
extracted Release startup pass with the bank and a system-only PATH; a separate
extracted copy passes the missing-bank fallback with one expected diagnostic.
No player's real save is opened. Evidence is under `out/acceptance/`,
`out/validation/resume-*.log` and `out/validation/resume-visual-final/`; final
visual run IDs are Debug `343c9b1190d24d15a0197e182d423756` and Release
`2d0a9c2039234fe08a63b1b1b9be912f`. Rerun the hardware check with
`scripts/check-painter.ps1 -Preset x64-debug -ResumeOnly` (or `x64-release`).
These checks do not establish physical-pad, listening or human comfort acceptance.

**Primary Rematch and end-menu paper implemented (2026-10-15, part of
`POL#18`/`POL#16`).** The [end menu](../game/states/end_menu.cpp) initially focuses
its first, persistent paper card: **Rematch** for a single match, **Next Round**
or **Replay Draw** for an unfinished series, and **Play Series Again** after a
series completes. The existing restart result still rebuilds with the same
settings and advances or resets the series through `GameLevel`; no extra ready
step is introduced. Change Teams, Change Weapons, Change Level and Exit to Main
Menu remain directly accessible below it. An unfinished series keeps only
continuation and Exit, preserving its fixed roster and setup.

The page carries the countdown/HUD drawing language into a static, pale stepped
panel with dark text, a black keyline, offset shade and restrained lower/right
edge shading. A detached ink arrow follows the engine focus group; the primary
card remains visible while another row is focused. Reduce effects retains all
information. The shared footer and Select prompt keep their existing keyboard
and pad labels. Other menu pages retain their artwork.

`presentation.json.end_menu` owns this page's panel size, heading position,
first-action layout and `art` colours, keyline, shadow and focus padding.
The row spacing leaves a gap between complete cards using the actual font
metrics, including when a secondary action is focused beside the persistent
primary card. Startup rejects malformed colours, invisible or identical
text/fill choices, out-of-range dimensions and invalid paper/padding geometry.
The drawing uses the existing solid texel and fonts; there is no new
atlas, animation timer or random state.

Validation: Debug and Release build under `/W4 /WX` and pass all 22 consumer
cases. Final focused `story_audio` reruns cover the added heading placement and
paper/focus content checks. Both D3D11 hardware end-menu matrices pass on the
NVIDIA GeForce RTX 5080: actual keyboard-driven pages return every continuation,
setup and exit result across single matches and continuing, drawn and completed
series. Ongoing-series setup labels are absent from both pixels and navigation.

The four session states fit at 720p, 1080p, QHD, 4:3, 16:10 and ultrawide.
Actual font metrics establish separated action cards, a clear heading and a
contained last row. Repeated drawing preserves pixels and random state; neutral
time and Reduce effects retain the same static page. The same live page and
cursor survive an exact 720p/1080p resize round-trip. Native captures retain
[initial Rematch focus](images/polish-end-menu/rematch-720p.png),
[secondary focus beside Rematch](images/polish-end-menu/secondary-focus-720p.png)
and [completed-series continuation](images/polish-end-menu/series-720p.png).

Isolated Debug and extracted Release startup pass with the accepted bank, a
system-only PATH and graceful shutdown. The 53-file Release ZIP byte-matches
all 26 reviewed runtime assets and excludes authoring assets. Evidence is under
`out/acceptance/` and `out/validation/end-menu*`; final renderer run IDs are
Debug `64e06ffaae7c42e4bcca94a413d9c562` and Release
`ed454a07b52d4fea9795c0148531f4ad`. Rerun with
`scripts/check-painter.ps1 -Preset x64-debug -EndMenuOnly` (or `x64-release`).
Physical-pad comfort, listening and human readability acceptance remain in
their existing TODO rows; this increment does not close `POL#16` or `POL#18`.

A visible per-player ready check before the next round is
worth testing if one person's confirmation repeatedly starts play before the
others are ready; it also adds friction, so it should not become mandatory by
assumption. Give disconnected groups a clear way back to the menu.

**Per-pad deadzones and live preview implemented (2026-10-15, part of `POL#18`).**
Options and Pause > Controls / audio > **Pad sticks** selects Pad 1-4 and sets
separate **Move deadzone** and **Aim deadzone** percentages (0-90%, in five-point
steps). The preview shows raw horizontal movement, rescaled movement response,
the right stick's radial cutoff, raw position and active direction. A hollow
square marks raw aiming and a solid square marks a new aim direction; inside
the cutoff the page says **No new aim**, matching the character's retained aim.
Use the D-pad or keyboard to navigate this page; both sticks stay available for
testing without moving focus or changing a pending setting.
The pending settings affect the preview immediately; Apply activates and saves
all four profiles. Back discards only unapplied edits, and returns to the
covering page without resuming the match. The parent keeps pending shared
preferences and preserves profiles applied in the child when it later saves.

Profiles in [`PlaySettings`](../game/play_settings.h) and the save's `pad_sticks`
array belong to **XInput device slots**, labelled **Pad 1-4** on the page; they
are independent of the roster/view's player number. Opening from a pad's pause
selects that slot; otherwise the first connected pad, or Pad 1 when none is
connected. Disconnected slots stay editable and show **disconnected**, with
neutral input. A reconnect to a different slot uses that slot's profile, so the
page explicitly asks players to check their slot after reconnecting. This is
slot persistence, with no claim of physical device identity or automatic
mid-match reassignment (`ROAD#4`).

Absent or individually malformed profile fields inherit the authored movement
threshold and the old save's shared aim sensitivity. Editing a threshold makes
that field an explicit percentage. Shared gameplay/preview math preserves
continuous movement from zero to full speed after the horizontal cutoff and
uses a radial aim cutoff before normalizing direction. Live matches read
applied profiles on every input sample. Drop-through, menu navigation, mouse
aim, fixed roller aim and A-or-left-trigger jump keep their existing rules.
`presentation.json.controls_settings.stick_preview` owns chart geometry;
the page reuses the existing paper, fonts and solid texel.

Validation: Debug and Release build under `/W4 /WX` and pass all 23 consumer
cases. Save fixtures cover four distinct profiles, independent malformed-field
recovery, legacy inheritance, safe bounds and round trips. Shared input checks
cover cutoff continuity, full travel, radial direction and live changes in
individual slots. Navigation checks keep fully deflected sticks out of menu
actions while retaining D-pad repeats and neutral rearming.

Both D3D11 hardware preview matrices pass on the NVIDIA GeForce RTX 5080 at
720p, 1080p and QHD: native copy/contrast, five focus cards, initial/pausing slot
selection, pending cutoff previews, clamps, Apply/reload/write refusal,
Back/Escape discard and parent Apply preserving child profiles. Repeated drawing,
Reduce effects and resize preserve the page. Both parent Controls/audio matrices
also pass six display sizes/aspects, including
Pause > Controls/audio > Pad sticks and back with the match and save frozen.
The retained
[720p capture](images/polish-stick-settings/pad-sticks-720p.png) shows the
disconnected-pad preview. Rerun with
`scripts/check-painter.ps1 -Preset x64-debug -StickSettingsOnly` (or `x64-release`).
Debug and extracted Release startup pass
with isolated saves and a system-only PATH; the 53-file ZIP retains the accepted
bank and byte-identical presentation data. Evidence is under `out/acceptance/`
and `out/validation/stick-*`.

Physical review with two controllers, including drift, partial movement, aiming
and reconnect-slot identification, remains in `POL#18`/`ROAD#2`. Full remapping
remains separate (`GAP#4.2`).

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
