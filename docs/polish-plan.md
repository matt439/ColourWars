# ColourWars polish and level-design plan

Status: planned; implementation has not started under this plan.
Direction agreed on 2026-10-08. Source baseline: game revision `13e9960`.

## Goal and direction

Make ColourWars look and play like a finished local multiplayer paint game,
with detailed, distinctive pixel art and levels designed around movement,
painting and encounters. The first deliverable is one complete, polished match
that establishes the standard for the rest of the game.

The user's direction is explicit:

- Use a consistent pixel-art style throughout the game.
- Replace the generic character of the newer weapon designs with distinctive
  paint tools, designed together with their wielders and firing animations.
- Add substantial environmental and background detail, depth and identity.
- Design better levels as well as improving their artwork.
- Improve both visual presentation and game feel.

Aim for expressive characters, satisfying liquid paint and places that feel
constructed and inhabited. Detail should communicate materials, function and
history while keeping players, projectiles and painted surfaces easy to read.

## Starting point

This plan comes from current source and the retained 2026-10-08 validation
captures. It does not establish how the game feels on physical controllers;
fresh two- and four-player playtests are part of the work.

Useful work already exists: explicit joining, reconnect pause, palette options,
labelled HUDs, controls cards, stage previews, saved settings, best-of sessions
and per-player paint contribution. Build on those systems.

The visible gap is consistency and depth: the textured menu, shaded weapon art,
simple characters and largely flat arena geometry speak different visual
languages. The gameplay code also offers concrete polish opportunities:

- Camera framing currently follows pane pixels at scale 1, so changing
  resolution changes the amount of world visible.
- Jumping requires a press while grounded; there is no jump buffer or short
  grace period after walking off a ledge.
- Damage reduces health and plays a sound, with little visual response. Death
  removes the character, and respawn restores it at its spawn.
- All five projectile types use the same animation, despite different behaviour.
- Surface splashes and the animated results reveal already exist, but need the
  same art and feedback treatment as the rest of the game.

Relevant implementation references:
[player](../game/objects/player.cpp), [camera and match flow](../game/objects/level.cpp),
[tuning](../game/content/tuning.json), [presentation](../game/content/presentation.json),
[weapon artwork](../game/art/README.md), [level editor](level-editor.md) and
[completed feature increments](feature-roadmap.md).

## 1. Establish the art and display rules

Produce a small style reference containing a character, two contrasting weapons,
a painted platform, background materials, effects and a HUD example. Inspect it
at actual gameplay sizes, especially four-player split-screen.

Decide and record:

- Native sprite grid, consistent world texel density, outline weight and shading.
- Character proportions, weapon scale, team-colour regions and player markers.
- Palette roles: quiet background, readable terrain, distinct characters and
  bright paint. Check every supported team palette.
- Authored world framing for each layout and behaviour at other aspect ratios.
- Display scaling, camera alignment and weapon rotation policy. Evaluate aimed
  poses or pixel-art rotation treatment while keeping simulation aim continuous.
- Typography and UI scaling that remain readable at the smallest supported size.

Reauthor the newer weapons and menu artwork to this standard. Reducing an
illustration's resolution alone does not supply deliberate pixel shapes or a
distinctive design. Keep editable sources and reproducible atlas generation.

The engine already exposes point filtering and a camera helper accepting world
units. Inspect and use those seams first. Pixel stability also depends on sprite
scale, fractional placement and rotation; filtering alone is not acceptance.

**Exit condition:** one gameplay composition looks coherent in single-player and
four-player views. Slow camera movement and aim sweeps remain stable and readable
at 720p, 1080p and QHD. Record the supported scaling behaviour and any letterboxing.

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
and responsive. Players can recognise shots, hits, empty paint, elimination and
respawn with sound muted. Art does not alter collisions, coverage or attribution.

## 5. Finish the match presentation and extend the treatment

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

On local builds with the private audio bank, review firing, impacts, movement,
UI and results timing and mix by ear. Retain the optional silent-bank behaviour
and intentionally silent distributable. Redistributable audio would be separate work.

**Exit condition:** a new player can join, identify their character and team,
understand the painting objective and controls, finish a match and rematch with
minimal explanation. All three arenas and all five weapons meet the same standard.

## 6. Acceptance and delivery

The reference milestone comprises Close Quarters' redesigned layout and detailed
environment, one coherent animated character, sprayer and roller artwork, stable
camera framing, movement polish and complete paint/damage/respawn feedback, taken
from setup through results. Review it in motion before expanding asset production.

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
| Level play | Two- and four-player matches, side swaps, all three arenas and all five weapons; route, spawn, paint-distribution and pacing notes. Exercise three-player sessions too. |
| Combat readability | Clean and heavily painted arenas, all palettes, crowded effects and muted audio. |
| Session flow | Join, disconnect/reconnect, focus change, pause/settings, restart, draw, series completion, rematch and return to menu. |
| Correctness | Relevant retained consumer acceptance checks, Debug/Release builds and startup checks; release asset/package inspection for changed runtime content. |
| Editor | For editor changes, run its document/GUI checks. For level changes, reopen saved files and verify them in the game; editor validation alone does not prove playability. |

Add focused regression coverage where gameplay mechanisms change, such as jump
buffering, camera mapping or paint ownership. Automated checks establish correctness;
physical playtests and visual review establish readability and feel. Keep the existing
outstanding clean-Windows deployment check in the release acceptance work.

Work stays in the game and its content, preserving its OOP design. Use the pinned
engine's existing interfaces; if an actual missing capability blocks an accepted
slice, record the client exhibit in [engine reports](review/engine-reports.md) and
handle the engine change in Labrador. Balance numbers belong in tuning, presentation
choices in presentation data, and arena layout and decoration in level data.

The next implementation task is to establish the pixel-art/display reference and
movement baseline, then build the first Close Quarters blockout against those rules.
Progress is gated by the exit conditions above, with each completed increment small
enough to review, validate and commit independently.
