# Combat-map research and the next playable experiment

Research checked **2026-10-20**. This precedes the new-map design and movement /
weapon tuning, as requested by the author. The deliverable is an original new
combat map, then tuning against that map; refining the earlier Close Quarters
draft does not replace it. Existing maps and the preserved CQ experiment remain
available for comparison.

The numeric baseline below records the state before tuning. The resulting
[Crossfeed comparison](polish-crossfeed.md#coupled-movement-and-five-tool-comparison-2026-10-20)
owns the selected provisional values and their measured limits.

## What the comparison must account for

ColourWars is a local, one-to-four-player platform shooter with individual
split-screen cameras and two colour teams. The final held painted surface wins;
eliminations deny presence but award no score. Painting over an opponent changes
ownership. Exposed floors, walls and undersides score, while buried faces do not.
Paint ownership changes neither movement nor ammunition recovery.

Each player chooses one primary. Ammunition recovers anywhere after 0.5 seconds
without a paid shot, at 0.4 tank/second: an empty tank needs about three seconds
without firing. Health recovers after five damage-free seconds; respawn takes
three seconds. There is no ammunition pickup, ink swimming, wall climb, dodge,
projectile parry, Super Jump or active secondary-weapon system to assume in a map.

The baseline player body is 52 by 120 world units, with an 800-unit/s horizontal
cap, acceleration and braking. The measured tap / held apex is approximately
95 / 207.472 units. The same-height full-speed held jump travels about 813 units;
that does not establish reach at another height or from rest. Full-speed release
already needs about 121 units to stop. One-way platforms admit upward movement
and can be dropped through; ramps support players while paint passes through.
The 0.12-second jump buffer and 0.10-second ledge grace are separate from jump
height and airtime. See [the course measurements](polish-movement-course.md).

Baseline data, before this experiment's tuning:

| Primary | Launch speed | Interval | Projectile / lifetime | Important qualification |
|---|---:|---:|---|---|
| Sprayer | 1,100 | 0 s | Spray / 5 s | Gravity, drag and horizontal player-velocity inheritance. |
| Sniper | 2,000 | 1 s | Straight jet / 5 s | No gravity or drag; ideal unobstructed travel is 10,000 units. |
| Roller | 0 | 0.0001 s | Contact volume / 0.000001 s | Fixed forward/down aim; lifetime is not useful reach. |
| Mister | 300 | 0.1 s | Spreading, growing mist / 5 s | Horizontal player-velocity inheritance changes moving reach. |
| Bucket | 800 | 0.5 s | Broad ballistic ball / 5 s | Gravity, drag and horizontal player-velocity inheritance. |

These are the pre-tuning authored inputs from [tuning.json](../game/content/tuning.json), not
measured effective ranges. The game's fixed-step cadence, muzzle offset,
projectile extent, diffusion, terrain contact and retirement all matter.
Changing movement can change three tools' flight envelopes as well as exposure.

## Primary-source findings

The labels below distinguish **developer statements**, **visible observations**
and **our inferences**. A publisher screenshot establishes visible geometry,
not a traversable route, a spawn algorithm or competitive balance. No foreign
movement constants or map geometry are adopted.

### TowerFall: connected combat spaces and reasons to move

**Developer statements.** The [official site](https://www.towerfall-game.com/)
describes limited arrows recovered from walls and bodies, risky chest pickups,
and a dodge that can catch arrows but leaves a vulnerable recovery. In the
[2014 developer interview](https://blog.playstation.com/2014/03/14/towerfall-ascension-on-ps4-behind-the-arena/),
Thorson identifies spatial positioning as a design goal; drill arrows can change
cover's function by piercing it. The [28 August 2018 Switch announcement](https://maddythorson.medium.com/towerfall-is-coming-to-nintendo-switch-on-september-27th-97e1a1ee99b1)
relates wider arenas to player count, recommends tighter classic arenas to keep
newcomers in the action, and distinguishes wider team games with revival.

**Visible observations.** These images were inspected from the developer /
publisher's [Steam gallery](https://store.steampowered.com/app/251470/TowerFall_Ascension/),
with provenance checked through [Steam's app metadata](https://store.steampowered.com/api/appdetails?appids=251470).
The IDs identify screenshots, not verified map names:

| Official image | What is actually visible |
|---|---|
| [Gallery 0](https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/251470/ss_5f5782e7f91f99dbeb7f9a561771378f850ae872.1920x1080.jpg?t=1715114308) | Four side chambers around a central shaft; thick blocks interrupt horizontal lines, while short ledges divide the sides vertically. All four characters share one arena view. |
| [Gallery 5](https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/251470/ss_8a230efea8416904a3d7e6a3aa145e9fa40a356b.1920x1080.jpg?t=1715114308) | A broad middle platform separates upper and lower spaces; side gaps and small upper platforms provide visible connections and intermediate positions. |
| [Gallery 6](https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/251470/ss_15d95c4c3c19032a44985faabe531282698196c8.1920x1080.jpg?t=1715114308) | Staggered platform clusters flank a central gap. Projecting blocks interrupt several horizontal lines while upper and lower spaces remain visually connected. |

**Inference.** Give alternate approaches different attack angles and reconnect
them near useful painting. ColourWars must supply its own circulation incentives:
it has neither arrow retrieval nor chests, and repeated respawns differ from
round elimination. Its separate cameras also remove the shared view's immediate
information about every approach. The screenshots do not establish wrapping,
default spawn selection or jumps achievable with ColourWars' movement.

### Splatoon: geography and weapons are designed together

**Developer statements.** [Iwata Asks, chapter 5 (2015)](https://www.nintendo.com/en-za/Iwata-Asks/Iwata-Asks-Splatoon/Splatoon/5-This-Depth-Has-No-End/5-This-Depth-Has-No-End-1021373.html)
describes deliberately pairing geographical features with weapons and retaining
strengths and weaknesses. An imbalanced stage could amplify a small team
advantage until comeback opportunities disappeared; adding a weapon meant
testing stages again. The developers also describe careful painting contributing
without strong combat skills and revisiting surfaces missed while descending.

**Documented changes.** [Nintendo's update history](https://en-americas-support.nintendo.com/app/answers/detail/a_id/61257/)
provides concrete interventions, rather than a universal map recipe. Dates are
Nintendo US release dates:

| Version / date | Explicitly documented geometry purpose |
|---|---|
| 3.0.0 / 27 February 2023 | Changed terrain on eight listed stages, including Eeltail Alley and Hammerhead Bridge, to reduce distant weapons' influence. The notes do not identify individual new cover pieces. |
| 8.0.0 / 30 May 2024 | Barnacle & Dime and Hammerhead Bridge gained central approaches and tactical-position choices, in all modes. |
| 9.3.0 / 12 March 2025 | **Tricolor**, not ordinary two-team Turf War: Eeltail Alley gained spawn shielding and defender launch choices; Hammerhead Bridge gained shielding for attacker advancement. Bluefin Depot expanded a grate / spawn terrain and added an inkrail for central entry. Nintendo relates Tricolor revisions to uneven attacker / defender results. |

**Transfer boundary.** [Nintendo's Splatoon guidance](https://www.nintendo.com/en-gb/Games/Wii-U-games/Splatoon-892510.html)
distinguishes scoring ground from walls used for climbing and describes Super
Jump travel. ColourWars' walls and ceilings directly score, and it has none of
that traversal or ink-refill machinery. Adding a blocker here changes both
firing geometry and the available scoring faces. The patch notes support
measuring approach choices alongside weapon reach; they do not prove our map is
balanced or tell us what a particular new wall should look like.

**Author clarification, 2026-10-24:** take broad floor paintability from the
Splatoon reference: ordinary floors, including home areas and raised platforms,
should normally accept paint. Nonpaintable surfaces need a clear material or
gameplay reason, such as grates; proximity to spawn is not that reason.
[Nintendo's Turf War introduction](https://www.nintendo.com/au/news-and-articles/get-splatting-with-the-basics-in-our-splatoon-2-101/)
describes ground coverage as the objective and distinguishes nonscoring walls.
Our choice to score exposed walls and undersides still differs. The old H4
wording was applied too restrictively in Crossfeed; assess regional area and
travel/repaint cost without banning ordinary home floor from scoring.

### INVERSUS: give a map a tactical question, preserve its counterplay

**Developer statements.** Ryan Juckett's [11 February 2016 map discussion](https://blog.playstation.com/2016/02/11/brain-bending-shooter-inversus-coming-to-ps4/)
gives named examples: **Alley** centres on dividing routes and hyper-shot
pickups; **Pipes** removes dodge room and makes regenerating ammunition and
blocking important; **Bomber** uses wrapping and awareness across the screen.
Its shots change which tiles each team can traverse.

**Inference.** A ColourWars map should pose an identifiable choice, such as a
short exposed repaint route versus a longer change of attack angle. Copying a
Pipes-like compulsory tunnel would discard its projectile-blocking defense.
Our paint changes score, not passability. No INVERSUS respawn timing or comeback
probability was established by this source.

**Weapon experiment evidence.** Juckett's [26 October 2015 prototype account](https://blog.hypersect.com/reducing-perceived-attack-delay/)
describes charging without directional commitment encouraging constant charging;
early detonation around cover proved too powerful, while steering bullets drew
attention away from movement. These are historical prototype results, not rules
against indirect attacks: [Deluxe's 9 August 2017 announcement](https://blog.hypersect.com/inversus-newsletter-august-9-2017/)
explicitly introduces around-corner split shots. Our test is whether cover,
arcs, cadence and escape time leave each other meaningful work.

### N++: movement rhythm, visibility and iteration

**Developer statements.** [Metanet's 4 December 2014 account](https://blog.playstation.com/archive/2014/12/04/ins-outs-ups-downs-lefts-rights-designing-n-levels/)
describes routes shaped around momentum, varied skill and rigorous revision.
A single-screen view lets players plan and judge landings; replays reveal
different approaches. Substantial changes are followed by reassessment.

**Visible observation.** The article's [embedded authoring GIF](https://i.imgur.com/W6dIbqG.gif)
ends with repeated L-shaped ledges in one open chamber, a central vertical
hazard chain ending above the floor, and rewards in separated pockets and under
a ledge. Sampled frames revise geometry and rewards: this is authoring, not a
recorded traversal. No map name was verified.

**Inference.** Repeat a readable landing motif, distinguish optional excursions,
and provide recovery after misses. Do not import its lethal challenges or
acrobatic movement. ColourWars must preserve paint access and space for an
opponent, and judge visibility in an individual pane rather than a whole level.

### Celeste: command forgiveness is a separate variable

**Developer statements.** [Maddy Thorson's forgiveness article](https://www.mattmakesgames.com/articles/celeste_and_forgiveness/index.html)
describes expanded command windows through coyote time and buffering; apex
gravity treatment adds adjustment time. Other techniques rely on dash or wall
jump mechanics absent here.

**Inference.** Keep ColourWars' buffer and grace distinct from apex / airtime
comparisons. The author's high / floaty-jump feedback does not establish that
input forgiveness should be removed, nor supply suitable numerical constants.

## Criteria for the original new map

These are **our hypotheses**, based on ColourWars' mechanics and the comparisons.
They are not claims that another developer has prescribed these numbers.

| ID | Design contract | Measurable test before tuning |
|---|---|---|
| H1: circulation | Useful alternate approaches reconnect at contested paint, with retreat / recovery choices. | A directed route graph names every required, optional and recovery connection. Drive each intended connection and return; record first-contest and uninterrupted painting time from each spawn. A drawn opening alone is insufficient. |
| H2: intended inputs | Route importance and input family are separate: walk, tap, held jump and drop. | Test rises, gaps, whole-body ceiling / corner clearance and landing widths with the real Player at relevant starting speeds and hold durations, including +/-2 input ticks. Name whether a landing must continue, stop or reverse; reaching it alone does not prove braking room. A required 140-unit climb may intentionally need hold; a failed tap alone is not a tuning defect. |
| H3: protected re-entry | Four full-body spawns per team lead to two physically distinct departures and useful decision space. | Check separation, support, repeated respawn, escape / return and sampled attacker firing positions for all five tools. Record which exits a position can threaten; do not claim universal spawn safety. |
| H4: score geography | Ordinary floor and platform tops accept paint by default, including home areas; nonpaintable exceptions need a clear reason. Roller access includes connected walking paint, alongside walls / undersides. | Reconcile exposed strips and runtime tiles by object, region and face. Check ordinary-floor coverage; measure travel cost, paint per traversal / tank and repaint opportunity. Diagnose safe loops through geography and pacing rather than arbitrary paint bans. |
| H5: contested height | A perch trades a useful lane for exposure elsewhere; it cannot simultaneously solve both home departures and all valuable paint. | Enumerate approaches, covered / uncovered targets and exits. Test actual arcs / sweeps through the authored solids. Ramps are traversal, not bullet-proof cover. |
| H6: camera and warning | A compulsory landing and first escape choice are visible before commitment. | Capture 1/2/3/4-player native panes, moving approaches, clean and heavy paint. Measure time from visible threat / projectile to contact. A shared-screen screenshot cannot establish this. |
| H7: weapon / scale fit | Straight fire, ballistic arcs, diffusion and contact painting have useful, different jobs. | Measure stationary and moving flight, first-contact distance / time, paint per burst / tank, damage opportunities, cadence and ammo recovery on the same map stations, mirrored by team. Add moving-target and cover-to-cover trials: compare exposure time, projectile arrival, repeat shots and retreat rather than treating an unobstructed range as counterplay. |
| H8: recovery pacing | Repositioning creates useful partial recovery without making one safe camping loop decisive. | Compare actual time between cover, shot-free intervals, regenerated fraction and return-to-contest time. From empty, recovering fraction f needs approximately 0.5 + f/0.4 seconds without another paid shot. |

Begin with current values. Choose representative edges and tool stations rather
than an unbounded cross-product. Measure movement at rest / intermediate / full
speed; account for projectile velocity inheritance. Include opposite directions,
miss recovery, 1v1 and 2v2 separately, the uneven three-player case, and side /
loadout swaps. Automated checks establish feasibility and repeatability; fresh
players, physical controllers, perception, pressure and competitive balance
still require play observations.

## Order of work and the preserved CQ experiment

1. Record this research and its criteria before deciding movement or weapon values.
2. Author a **new** original combat map and its directed route / scoring brief,
   register it through the supported stage pipeline, then measure the baseline.
3. Tune movement and all five weapons against that map, revisiting geometry when
   a failure comes from an unintended route or firing opportunity. Re-run the
   Movement Course and consumer / native checks; retain candidate measurements.
4. Continue the remaining actionable TODOs. Human acceptance gates stay open.

The preserved CQ draft is an earlier experiment, not research evidence or the
new-map deliverable. Its 140-unit main / upper access rises, long lower lane,
exposed-area budget and incomplete pressure / camera qualification make useful
comparison questions under H1-H8. Its passing scripted routes do not establish
that its spacing, density, compulsory input choices or weapon scale are right.
No further CQ redesign is required to create the new map.
