# Gameplay tuning

The game reads `game/content/tuning.json` before creating the application. The
build stages that file beside the executable. `GameTuning` owns the records that
weapons and projectiles borrow; gameplay mechanics stay in C++, while authored
balance values select their amounts.

## Keys

Every object in the file takes only the keys its reader reads: the root, each
block, each vector and sound, and each array entry. An unknown key fails at
startup with the file, the object and the key, and so does a name written
twice in one object. A misspelt optional block such as `paint_underfoot` is no
longer silently skipped. A key starting with `_` is a comment, with any value.
Each weapon and projectile has its own list, taken from what its class acts on.
`aim_tilt` is the roller's alone, `loop_sounds` and the `stop` cue belong to
the held-fire tools, `ready` to the sniper and bucket, the player-velocity pair
to the sprayer, mister and bucket, `spread_step` to the mist, and `diffusion`
to the spray and mist. The lists are code, beside the reads in
[tuning.cpp](../game/config/tuning.cpp). `presentation.json` and the level files
do not check their keys this way yet.

## Tool angles and projectile wind threshold

These required fields replace the gameplay constants from gap-audit 8.5. The
shipped values preserve the previous binary32 angles and wind threshold exactly.

| JSON path | Shipped value | Accepted range | Meaning |
|---|---|---|---|
| `weapons.roller.aim_tilt` | `0.7853982` | `0 <= tilt < pi/2` radians | Downward angle from horizontal when facing right; left facing uses `pi - tilt` |
| `projectiles.mist.spread_step` | `0.2617994` | `0 <= step <= pi/4` radians | Angular separation between adjacent mist pellets |
| `projectiles.wind_stop_factor` | `4.0` | Finite and nonnegative | Shared multiplier used to derive the horizontal snap-to-zero speed from each projectile's wind resistance |

The roller's shipped tilt is the same float as `pi/4` (45 degrees); its mirrored
rotation is the same float as the previous `3*pi/4`. A vertical roller tilt is
excluded because it makes left and right facing ambiguous. Zero gives horizontal
aim. Input still cannot aim the roller independently of its facing direction.

The mist's shipped step is the same float as `pi/12` (15 degrees). Its five
pellets remain in the order `0, +1, -1, +2, -2` times that step around the supplied
firing velocity. Zero sends five parallel pellets; the upper bound places the
outer pellets perpendicular to that velocity. Pellet count and order remain
mechanics rather than editable data.

The wind threshold remains:

```text
stop_speed = wind_resistance * wind_stop_factor
if abs(horizontal_velocity) > stop_speed:
    apply this step's existing wind deceleration
else:
    snap horizontal_velocity to zero
```

`wind_resistance` is acceleration, so the multiplier's dimensional equivalent is
seconds. It does not replace the update timestep. Equality still snaps to zero;
both directions use the same threshold. Zero removes the positive early-stop
threshold without disabling wind deceleration. Gravity, velocity caps, movement
integration, collision handling and projectile lifetime are unchanged.

Every projectile's `wind_resistance` must also be finite and nonnegative, and its
float product with the shared factor must be finite. Invalid values fail at
startup with the source filename and key. The two angle fields are required only
on their respective roller and mist records, not on the other tool types.

In Debug, all three fields work through the existing `tune` commands and use the
same validation. Roller tilt applies on the next weapon update, and wind factor
on the next projectile update, including existing objects. Mist spread applies
to newly constructed shots; a flying pellet keeps its launch direction. See
[the console's tuning scopes](debug-console.md#live-tuning).

## Accepted ranges

Every number in `tuning.json` is read through the shared checks in
`game/config/content_value.h`, which `presentation.json` and the level definitions use
too. A value outside its range fails at startup, and a `tune` edit is refused,
with the file, the object path and the key, for example
`'respawn_delay' must be finite and not negative, and is -1.` or
`'jump_buffer_time' is 2, which is outside [0, 1].`

The bounds say what the code can use or what a field means. They do not fence
in today's numbers, so a balance pass never needs a rebuild to leave them.

| Kind | Rule | Fields |
|---|---|---|
| Signed rate | finite | player `gravity`, `jump_launch_velocity`, `jump_acceleration` and `drop_down_velocity` (y is down, so the sign is the direction); projectile `gravity`; `player_velocity_amount`; offsets and other plain vectors |
| Delay or amount | finite, `>= 0` | `match.countdown` and `overview`; player `x_acceleration` (zero is a constant run), `ramp_gravity_multiplier`, `jump_max_time`, health regeneration, `respawn_delay`, `damage_sound_delay`; `weapons.ammo_regen_delay` and `ammo_regen_rate`; weapon `shoot_interval` (zero fires every update), `starting_vel_length`, `ammo_usage`; projectile `player_damage` and diffusion `start_time`/`end_time` |
| Extent, cap or divisor | finite, `> 0` | player `x_initial_velocity` (zero never leaves rest) and `x_deacceleration` (the braking divisor, and what stops a released stick) and `animations.run_speed` (the ground speed above which the running clip plays); projectile `delete_timer`, `size`, `collision_size` and diffusion `end_scale`; `projectiles.max_velocity`; and the fields that were already positive |
| Gain, fraction or threshold | `0..1` | every sound `volume`, the `input` thresholds, `player.drop_down_analog_threshold` |

Rules that compare two fields sit at the end of the reader that reads both.
Besides the paint-tile geometry and the input thresholds
(`0 < menu_release_threshold < menu_engage_threshold`):

- a projectile's diffusion `start_time` is at most its `end_time` (equal is a
  step straight to `end_scale`);
- `match.opening.push` happens inside the countdown, so it is at most
  `match.countdown`. The countdown is deliberately not compared with the match
  duration: they are sequential timers.

A weapon's `ammo_usage` is deliberately not compared with
`weapons.starting_ammo`. A tank too small for one shot is a tool that reads as
empty, a visible state rather than a fault, and the empty-paint acceptance
fixtures are built exactly that way.

## Verification and menu presentation

The `gameplay_tuning` consumer case uses the actual builders, roller and
projectile integrator without creating a graphics or audio device. It checks
the shipped values against the previous formulas, custom values, boundary and
invalid content, and updates through borrowed records. The existing tool,
projectile, movement and paint checks continue to cover their wider behavior.
Debug-console acceptance checks the new tuning scopes and rejected-edit rollback.

Run `scripts/run-acceptance.ps1 -Preset x64-debug` and repeat with `x64-release`
from an x64 Visual Studio Developer shell.

The remaining menu geometry and sheet choices are documented in
[menu presentation](menu-presentation.md). The historical
[gap audit](review/gap-audit.md#85-the-cjson-line-is-drawn-inconsistently) is unchanged;
[TODO.md](../TODO.md) records open work.

## Headless balance runner (`TOOL#1`, 2026-10-25)

**First report complete (2026-10-26).** The runner, Skirmish policy, paired final
paint metrics and first fixed 250-match report are implemented and recorded below.
They provide descriptive evidence for the five-tool balance pass (`POL#27`) and
arena reviews (`POL#21`, `POL#31`); the author accepted all three on 2026-11-05
without the attended sessions, at current values. Levels
build without a device, the Debug [bots](debug-console.md#bots) are seeded and
deterministic, and paint credit is per seat. Final paint can expose a question
about an arena or tool; it does not measure whether a route was used.

The runner is a Debug-only console executable beside the acceptance cases. It
builds a stage with no graphics or audio device, puts bots on every seat, steps
the fixed 60 Hz simulation as fast as the CPU allows, and writes one CSV row per
match: stage, seed, roster, tools, final paint share per team and seat,
eliminations, time starved of ammunition, and paint by region and face direction
(the measures the blockout docs already use). A sweep is stage x tool pairing x
seed, with a summary per cell and its spread across seeds.

Slices, in order:

1. **Runner.** Device-free build, bots on all seats, CSV out, deterministic per
   seed. Acceptance: the same seed reproduces the same row bit for bit, and a
   short sweep on `crossfeed` completes inside the acceptance budget.
2. **A `skirmish` bot (implemented).** `paint` and `chase` each play half the game. A bot that
   paints, engages an opponent in range, retreats to refill and returns is the
   minimum for the numbers to say anything about tools rather than bots. It
   stays a developer tool; the separately requested shipping AI opponents use
   their own driver and [ROAD#10 qualification](feature-roadmap.md#ai-bot-players-road10).
3. **First report (complete 2026-10-26).** All 25 ordered tool pairs on
   `crossfeed` and `press_tower`, five fixed seeds each: 250 matches, with the
   findings in [Crossfeed](polish-crossfeed.md#first-headless-five-tool-report-2026-10-26)
   and [Press Tower](polish-press-tower.md#first-headless-five-tool-report-2026-10-26)
   as descriptive evidence for `POL#27` and `POL#31`.

Bot results are evidence, not acceptance. They find outliers for an attended
session to check; they never replace one, and a tuning change still goes
through the author.

### Device-free runner core (2026-10-25)

The first runner slice is implemented. `scripts/run-balance.ps1` compiles a
Debug-only console beside the acceptance cases, using the actual game objects.
It loads manifest resources as CPU metadata with null texture handles and silent
banks, builds real `Level` instances and supplies `BotDriver` input to every
seat. It creates no `Application`, graphics/audio device or physical input reader.

From an x64 VS Developer shell:

```powershell
scripts/run-balance.ps1 -Stage crossfeed,press_tower -Seed 0,17 -Players 2 -Tools sniper,sniper -Duration 20
```

Stages and seeds form the sweep dimensions; tool order and the A/B/A/B roster
are fixed for each invocation. One to four players are supported. Tools default
to Sprayer per seat and the bot defaults to `paint`; the existing `idle`, `walk`,
`jump`, `chase` and `skirmish` behaviours are also available. `-OutputDirectory` chooses a
fresh evidence directory and `-Csv` chooses a new CSV path. Existing CSVs and
run directories are refused. `-SkipBuild` is for an already current Debug build;
the tool itself is always freshly compiled. Run metadata pins source/data,
executable, arguments and CSV bytes separately from the deterministic rows.

The original 67-column prefix records stage, seed, roster/tools, requested duration,
actual active ticks and the fixed-step bits, raw team/seat paint areas, final
health/ammunition and a gameplay fingerprint. Team/seat `share_of_painted`
uses the painted-area denominator; `coverage_of_arena` uses all scoring area,
including neutral tiles. Zero denominators yield zero. Seat paint credit uses
`paint_owners::index(player_num, view)`, so device-less bots retain distinct owners.
Unused seats retain `none` labels, paint owner `-1` and zero-valued measures.

Ammunition starvation is sampled at the real active-input seam, after the bot
chooses input and before that world step: an alive, positive-health player's
actual full-shot readiness is `empty`. It is not an `ammo == 0` test.
`fire_starved_ticks` separately requires attempted fire; a refilling bot may have
zero such ticks. `observed_death_edges` counts sampled alive-to-dead state edges,
not credited eliminations. It can miss a terminal lethal health crossing whose
death-state transition would occur after time-up, so it is a lower bound.

Each match resets the seeded RNG and builds a fresh Level and bot driver. The
step is the existing replay convention, `TicksPerSecond / 60` (166666 ticks),
converted to float; the runner stops at the first real time-up/zoom-out transition.
Rows use classic locale, stable numeric precision and UTF-8/LF bytes, with no
wall-clock timing. Reproduction is qualified on the same build/data/machine and
step, rather than promised across toolchains. A bounded step ceiling rejects a
configuration that cannot finish instead of spinning indefinitely.

Validation: four nontrivial CPU self-check matches cover same-seed whole-row byte
identity, two/four-player owner mapping, seed zero/uint32 maximum and a short
Crossfeed sweep, plus CLI refusals and Unicode CSV/no-overwrite publication.
The public launcher completed a four-row Crossfeed/Press Tower, seed 0/17,
20-second Sniper control: each row had at least 1201 active ticks, the eight
seat samples totalled 2394 empty-readiness ticks, and all final ammunition values
were positive. Same-order replay produced identical complete CSV bytes; reversing
stage and seed order preserved all four complete rows by key. Existing
`debug_console` and `press_tower` CPU cases passed in Debug and Release.
Receipts are in `out/validation/todo-loop-20261025/tool1-core-v1/`,
`tool1-qualification-v1/` and `tool1-order-v1/`.

Native UTF-8 log decoding is scoped to this runner and restores the caller's
console encoding. A separate Unicode-path launcher control preserved the exact
prior CSV row and printed the real path correctly; both the helper and launcher
restored code page 437 after execution. This corrected only diagnostic decoding;
the earlier CSV files and match results remain valid. The supplemental receipt
is in `out/validation/todo-loop-20261025/tool1-encoding-v1/`.

`scripts/run-acceptance.ps1 -Preset x64-debug -Cases balance_runner` runs the
bounded self-checks. The default Debug suite includes them; Release rejects this
developer-only case. Case selection is validated before building, and
`-CompileOnly` lets the launcher reuse the compiler/object linkage without
pretending the self-checks ran during compilation.

### Credited projectile eliminations (schema 2)

`Player::lethal_projectiles()` records the first projectile contact that takes
an alive player's positive health through zero, before the later dead-state
transition. Match-lifetime counters survive timed and forced respawn; a fresh
match starts them at zero. Repeated contacts cannot replace the fatal source.
The existing sampled `observed_death_edges` remains a separate measure and can
miss that terminal contact at time-up.

A valid projectile owner is only provisional provenance. The runner validates
unique actual roster owners/teams and credits a different opposing participant;
unknown, friendly, self or absent owners remain uncredited. Credit snapshots
recompute rather than accumulate twice. Each seat records
`credited_enemy_eliminations`, `credited_enemy_projectile_deaths` and
`uncredited_projectile_lethals`; match totals reconcile the credited and
uncredited counts. This records projectile lethals, not a new scoring rule.

Schema 2 retains the original 67-column prefix/order and appends 14 fields:
two match totals and those three measures for each of four seats, giving
81 columns. The declared version changes from 1 to 2; unused-seat counters
remain zero. Earlier schema-1 files remain historical controls. Damage, tuning and
gameplay decisions are unchanged.

The ordinary `lethal_provenance` consumer case exercises real Player/projectile
callbacks, immunity and repeated/competing hits, respawn retention and
uncredited sources. The added collision controls cover a fast Jet
crossing with both endpoints clear and an actual Scene batch where one
shared Roller volume hits two victims. The Debug runner adds actual-roster validation,
idempotent attribution and deterministic match checks. `lethal_provenance` runs in both presets; the
`balance_runner` case and public launcher remain Debug-only.

Qualification: Debug/Release game/editor builds and include-boundary checks
passed, along with `lethal_provenance`, `combat_feedback`, `view_shake` and
`debug_console` in both presets plus Debug's `balance_runner` (nine CPU cases).
The public two-Sniper, 20-second Crossfeed/Press Tower control at seeds 0/17
retained all 66 non-schema legacy values, including fingerprints, exactly;
only schema 1 became 2. Complete 81-column repeats were byte-identical and
reversed-order rows matched by key. Press Tower seed 17 recorded one credited
elimination/death; the other rows recorded zero, with no uncredited lethals.
These short paint-bot controls qualify measurement, not tool balance. Actual
source baseline `e5256a5` and engine `1e60fb2`, source/producer/CSV pins,
case logs and the separate CSV comparison are retained under
`out/validation/todo-loop-20261025/tool1-lethal-native-v1/`.

### Skirmish measurement bot

`scripts/run-balance.ps1 -Bot skirmish` selects the new Debug-only policy.
It paints, engages, withdraws to refill and returns, using actual full-shot
affordability rather than positive ammunition alone. The
[bot policy and limits](debug-console.md#skirmish-measurement-bot) describe
the saved-X return/Y jump hint and tuning-derived stationary-launch heuristic.
There is no line-of-sight, path or ballistic-intercept solver. Default `paint`,
the earlier bots, schema-2 metrics and gameplay tuning remain unchanged.

The `skirmish_bot` consumer case exercises policy transitions, death/reset,
private RNG and deterministic input, actual five-tool observations, a real
positive-but-unaffordable Sniper tank and unchanged old-bot observations.
Release checks developer-code exclusion instead of running the policy.

Qualification: Debug/Release game/editor builds and include-boundary checks
passed. `skirmish_bot` and `debug_console` passed in both presets, plus Debug's
`balance_runner` (five CPU cases). Four 4P, 60-second controls used
Sprayer/Sniper/Mister/Bucket on Crossfeed and Press Tower at seeds 0/17;
two 2P, 60-second controls used Roller/Sprayer on those stages at seed 0.
Every new match ran 3601 active ticks. The primary four-row Skirmish CSV
repeated byte-for-byte, and its reversed-order rows matched all 81 fields by
key. The previous four-row, 20-second
two-Sniper `paint` control retained its entire 81-column CSV bytes.
These six new controls cover the tools across different rosters. The full
comparative report was still open at this qualification; the
[first five-tool report](#first-five-tool-report-2026-10-26) below completes it.
Attended balance decisions remain open.

The first Debug qualification stopped because an old CLI negative still
expected registered `skirmish` to be rejected. That failed run remains under
`tool1-skirmish-native-v1/`. Its passed Debug producers, dedicated Skirmish
and console checks were retained with a frozen input/evidence record.
The narrow fixture repair adds positive Skirmish parsing and uses a genuine
unknown bot for the negative; no bot policy or gameplay changed. Fresh
`tool1-skirmish-native-v2/` runs passed the affected Debug runner and remaining
Release/public checks. Source baseline `95ac1e9`, engine `1e60fb2`, exact
source/producer/CSV hashes, both comparison reviews and the preserved failure
are under `out/validation/todo-loop-20261025/` in those native roots and
`tool1-skirmish-prep-v1/`.

### Paired final paint CSV contract

The runner writes a separate region/face companion while retaining the main
schema **2**, its **81 columns** and existing serializer. Controlled native Paint
and Skirmish golden runs retained their entire prior 81-column CSV bytes; the
compatibility claim is scoped to those same-build/data controls.
The companion has independent schema **1**, **29 columns**. Its one-based
`match_row` joins each group to a main data row, excluding the header; stage,
seed, player count, bot, requested duration, final fingerprint and the actual
view-ordered roster provide additional identity checks. Roster text records
seat/team/tool/paint-owner, rather than assuming a device slot is a seat.

At the real time-up boundary the integration takes `Level::paint_surfaces()`:
an owned vector of each actual scoring tile's world-space convex quad, authored
`top`/`bottom`/`left`/`right` face, team, raw paint owner, authoritative scored-area
float and snapshot-local source ordinal. It includes neutral tiles and discovers
the paintable role independently of collision tags, including paintable one-way
supports. The retained report owns its values; it keeps no Level, Player or tile
pointers. Snapshot collection is read-only. The runner checks the old final
fingerprint again after measurement and rejects a changed fingerprint.
The current producer is static and axis aligned; the quad interface does not
establish moving/rotated scoring support or acceptance.

This is **final held paint**, not cumulative paint events, time spent holding an
area, player visits or route occupancy. A low final painted area can identify a
question for an attended route review; it cannot establish that nobody used the
route. Valid seat credit requires a unique actual roster owner with a matching
team. Neutral and uncredited painted rows remain explicit; invalid, absent or
team-mismatched owners are not silently assigned to a seat.

The helper clips each convex quad against the stage's vertical region rectangles,
using geometric fractions to allocate the tile's authoritative scored area. It
does not replace the quad with its axis-aligned bounding box or replace its area
with corner-derived area. `outside_partition` is the complement, including area
above/below the finite bands; its rectangle-bound fields are blank. Crossfeed
uses its five named X bands and Press Tower its seven; other stages use a whole-
arena band. Faces remain authored/local identities, not directions recomputed
from a world normal. Cells have canonical region/face/team/owner/seat/status order.

Two area authorities are deliberately recorded. `arena_runtime_*` reproduces the
existing per-source float score additions, retaining source groups and tile
order, and verifies those totals and raw-owner areas bit-for-bit against
`LevelEndInfo`. `arena_runtime_painted_area` is the double sum of the two already
grouped team floats. The main row continues using its existing runtime values. `arena_sample_*`
and region/face denominators use compensated double sums of the authoritative
individual tile floats. They can differ from the grouped runtime floats through
rounding; arbitrary tile permutations are not a replay contract. Allocation
conserves the sample totals within the emitted `allocation_tolerance`.
`held_sample_area` is raw held area, not a percentage.
`region_face_sample_paintable_area` includes neutral tiles, while
`region_face_sample_painted_area` includes only A/B-painted area. Analysts must
choose the intended denominator and handle a zero denominator explicitly. For
painted-row coverage use the paintable denominator; share of painted area uses
the painted denominator and excludes neutral rows.

**Output paths and publication.** The launcher accepts `-PaintCsv`; the native
console accepts `--paint-csv`. Omission creates `<mainstem>.paint.csv` beside the
selected main CSV, even when `-Csv` is outside `-OutputDirectory`; the launcher's
default main is `<new evidence directory>/matches.csv`. There is no companion-off
switch. Wrapper relative paths resolve from the repository root; direct native
relative paths use its working directory. Both destinations must be new and
distinct. Unicode paths are supported; ambiguous/aliased or conflicting paths,
one output used as the other's ancestor, and overlaps with the launcher's
reserved evidence paths are refused before output creation.

The native publisher exclusively reserves two sibling staging files before any
match. Both full CSVs are serialized, written, flushed and closed before
publication. It publishes the companion first and the main last, without replacing
an existing destination. Main-last is a completion-marker convention for this
writer, **not a jointly atomic two-file transaction**. A successful wrapper also
requires native exit 0, both final files and unchanged input/source pins, then
records both hashes in `run-info.json`. File existence alone does not qualify a
pair or prove that a raced destination belongs to this run.

On failure, preserve the run log, receipt and every named owned staging file or
published orphan companion. In particular, a failed main move can leave the
complete companion published and the complete main stage retained; this runner
has not published its own main completion marker. Another writer may own a
raced main path. The tool performs no automatic rollback, deletion, append,
finalization or resume. Inspect the recorded ownership/paths and hashes after the
owned process exits, and retry with a fresh output pair and a fresh wrapper
evidence directory. Do not treat an orphan companion as a complete result or
manually overwrite a destination to make the pair appear complete.

**Evidence boundary.** The owned snapshot API and shared region/CSV helper have
qualified foundations: Debug/Release game/editor builds and include checks,
`paint_surfaces` and `balance_regions` in both presets, existing paint/console
checks and the earlier Debug runner checks (nine CPU cases across the retained
foundation runs). The original `near` fixture-identifier compile failure remains
preserved; the narrow identifier rename changed no production or metric math.
The foundation result is
`out/validation/todo-loop-20261025/tool1-region-foundation-native-v2/result-v1.json`
(SHA256 `906B9081DBA3AD34A575654A1BD7DF77F4D78EFC8C553271275B998C246254FF`),
with independent foundation review `3C94419C6CE6275BBC9B5AEC41E99FDD364171576A09C1F157B7FDACA1EE53B6`.
The seven-file integration source bundle is `tool1-pair-prep-v1/ready-v1.json`
under that validation root (SHA256
`07B086A56C84239362C37D01A518E9EE8577E6A44EAD8E8980056E176EF0C94D`).
The source-only full-bundle review is
`tool1-pair-prep-v1/independent-full-source-review-v1.json` under that root
(SHA256 `515202F2944F3A84FBDF7E50A25BB4946A8BF273E2F2726EC8270C6B7A0C1F03`).
The integrated qualification below is separate from the retained foundation and
source-review results.

**Integrated qualification (2026-10-26).** The actual scope is `b3ad8743` plus
the pinned API/helper and seven-file runner integration, rather than clean HEAD.
The integrated Debug console compiled and ran its four real self-check matches,
including paired seeded-byte identity and the controlled partial-publication
failure that retained an orphan companion and main stage. The native existing-
directory-handle alias refusal also passed. The initial integrated executable
SHA256 is `226DC4F0AD84731F132736EF5CA8FB0663490A19616FC4654F1AE937E47FE9B5`.
All eight existing Debug/Release game/editor/engine/build-info producer hashes
matched the qualified foundation. Those producers were reused; the positive
public-wrapper controls compiled only their two console executables.

The public preflight matrix refused 29 ordinary malformed/preexisting/conflicting
output or evidence cases. Each retained identical tree contents, root presence
and sentinels and restored console encoding, with no C++ console compilation or native matches.
Three optional alternate-`out`-junction wrapper alias cases **did not run**:
`out` resolved to the same ordinary path, so the harness recorded a LIMIT and
changed no junction. The separately passed native resolved-parent alias control
does not stand in for those three wrapper cases.

Eight positive invocations produced four matches each: **32 match observations**,
including legacy controls and repeats, not 32 independent seed configurations.
The 4P/60-second Skirmish and 2P/20-second Paint controls on both stages at seeds
0/17 retained their exact previous complete main CSVs. New 3-second controls
covered 2P/4P, both stages, all five tools and seeds 0/uint32-max. Paired repeat
files were whole-byte identical. Reversed-order companions were rejoined through
their local main-row ordinals before all other 28 fields were compared by match
and cell key. Direct Unicode explicit/default destinations and both public
wrapper choices agreed where configured identically. Receipt checks verified
both output hashes, source/input pins, exact roster/fingerprint metadata,
runtime-float authority, sample-double allocation and bounded tolerance, UTF-8
paths/logs and restored caller encoding.

The compile/self-check receipt is
`tool1-pair-compile-native-v1/result-v1.json`, SHA256
`1DEFF06625FD106696F6150BCEBB9757CD487B6FB857312BB6BCBB1C4D7BEDDC`.
The public refusals are `tool1-pair-native-v1/negative/result-v1.json`, SHA256
`13FED846A295AAC1460692ADA40B74603EA7192FA6A3AC624ECCB1357CDD4E06`;
the positives are `tool1-pair-native-v1/positive/result-v1.json`, SHA256
`A2BAF13208446A9C594F5391E76DC218391EA5B0951FC5BDEF7B47602056EFB7`.
Their separate CSV/receipt analysis is `tool1-pair-native-v1/positive/csv-checks.json`,
SHA256 `170F27B62D52F2CCA831C6F8791AA995D22E0191533CF56B1E0BBB7F541E1193`.
All paths are under `out/validation/todo-loop-20261025/`. The old complete Paint
main hash is `79DAF9F654CB315BFC56F363EB7C09FE17D3C233F36613367621F2F6F49AC722`;
Skirmish is `D4E311355FD13B6C1F46AD898D1CC3A424A0BD0498BD1B8A055AE6729950FB68`.

Private positive qualification harness v1/v2 drafts and their source-only issues
remain preserved. Their array/oracle corrections were frozen in v3 before actual
positive execution; they were not production repairs or native integration
failures. The actual integrated controls above passed. These are measurement and
publication checks, not a statistical five-tool report, cross-machine replay
promise, physical-device/listening pass or author balance/arena acceptance.

### First five-tool report (2026-10-26)

The fixed first `TOOL#1` report is complete on clean `817a3686`, with no gameplay
or tuning changes: **250 distinct native matches**, two arenas times all 25
ordered pairs of Sprayer/Sniper/Roller/Mister/Bucket times seeds **0, 1, 17, 42,
2026**. Every match is 2P A/B, 60 seconds, using the existing Skirmish policy.
One serial native job per ordered pair produced ten matches (both arenas and all
five seeds). The first driver invocation deliberately paused after one successful job/
ten matches; resume verified and retained it, then ran the remaining 24 jobs.
There was no duplicated first job or failed native match. All 25 jobs exited 0,
with no builds and no task-owned native processes left in the completion receipt.

The arena owners contain the descriptive findings and questions:
[Crossfeed](polish-crossfeed.md#first-headless-five-tool-report-2026-10-26) and
[Press Tower](polish-press-tower.md#first-headless-five-tool-report-2026-10-26).
They show all five equal-tool controls with raw five values, medians and literal
ranges, plus mixed-tool and region/face examples. The complete 50 ordered stage/
pair cells remain available, not just those examples. Five chosen deterministic
seeds are not independent population samples; ranges are observed minima/maxima,
not confidence intervals. No significance, side-fairness, optimal-tool or human
balance claim follows. Swaps combine side, seat RNG, tool and conditional bot
behavior. Skirmish still has no line-of-sight, route or ballistic-intercept solver.

The retained native analysis contains all 250 `match-observations.csv` rows,
300 measure rows in `pair-summary.csv`, 5,600 `region-face-summary.csv` rows and
30 `side-swap-summary.csv` rows, with full raw five values and denominators.
There are 12,278 source companion rows. `report.md` gives the complete 50-cell
paint-share/elimination/empty-readiness view; `analysis.json` and the copied raw
inputs retain the exact machine-readable result. Region/face paint is final held
area. In particular, centre/crown are full-height fixed X bands (Y=-100-1800),
not route, summit, time-held or hill-capture measures. Zero final paint cannot
establish non-use, camping or spawn pressure.

Unknown/uncredited paint-owner sample area and outside-partition sample paintable
area were each zero in this report. Separately, every match recorded zero
uncredited projectile lethals; that is a death-provenance counter, not paint
ownership. Sample-double and runtime-float authorities remain separate. The
recorded sample-minus-runtime paintable areas were 0 or 0.0000457763671875,
reflecting source/tile addition rounding; sampled painted-area differences were
zero. The main score is not corrected to match the sample denominator.
`sample-runtime-authority.csv` retains both authorities and allocation tolerances;
those tolerances are numeric reconciliation bounds, not balance thresholds.
Null fractions for zero denominators remain null rather than invented coverage.

Evidence root: `out/validation/todo-loop-20261025/tool1-first-report-native-v1/`.
The executable SHA256 remains
`226DC4F0AD84731F132736EF5CA8FB0663490A19616FC4654F1AE937E47FE9B5`.
`driver-completion.json` is SHA256
`C4E9E20EB6974C2D244394964D711353857F44362B2C20FD02E57FD14C4C4406`;
`inputs-native-v1.json` is
`BD574C87DEE148E12FD6B33BFD992844F72E64DA5355BC8850DAB3066C040647`;
`analysis-result.json` is
`15FB49D74E63F49427BCDBA10C488A73C116DC9A294588550F1D8F17D640E488`.
The input manifest binds each actual command/exit receipt and paired CSV hashes;
the analysis receipt pins all 61 analysis artifacts. The analyzer validates
supplied bytes/declarations; the driver/job receipts establish actual execution.
The deliberate pause and completed job receipts remain preserved.
The independent actual-data/protocol review is `independent-actual-review-v1.json`,
SHA256 `5C89F9A5C460EAAB0E03E04C14925520F675DF4ED04B8D14EC67F6B66FFD61E1`.

This completes the finite `TOOL#1` scope. `POL#27` and `POL#31` were attended
decisions, accepted by the author on 2026-11-05 at current values; the report neither authorizes retuning nor accepts a new bot opponent.
`POL#36`, paint underfoot, was off in this baseline. It has shipped on since the
author accepted `POL#30` on 2026-11-05, without the switch-off comparison planned here.

## Shipping bots in the headless runner (`TOOL#2`, 2026-10-29)

**Done 2026-10-29.** The runner and `scripts/run-balance.ps1` take a `match`
policy: the shipping `MatchBotDriver` on every seat, with one
`-Difficulty easy|normal|hard` per seat (all Normal by default). A roster cannot
make P1 a bot, and a one-on-one bot match needs one, so the driver gained a
second constructor taking one optional difficulty per participant; the roster
constructor now delegates to it with the same per-seat seeds, so shipping
matches are unchanged (`bot_runtime`, `bot_driver`, `bot_roster`,
`bot_gate_routes` and `debug_console` pass in both presets). `MatchBot` gained a
read-only `difficulty()`. The main CSV stays schema 2: the `bot` field names the
policy, `match:<p1>:<p2>`, so no column was added, and the Debug policies' rows,
including the Paint and Skirmish controls, keep their bytes (`skirmish_bot`
passes). The `balance_runner` self-check runs three Crossfeed matches under the
driver: one seeded Easy-v-Hard row repeats byte for byte, each seat reads back
its requested difficulty from `MatchBotDriver::bot(participant)`, and swapping
the difficulties changes the row.

**Ladder, 2026-10-29 (Debug).** Easy v Normal, Normal v Hard and Easy v Hard,
each in both seat orders, on Crossfeed and Press Tower at seeds 0, 1, 17, 42 and
2026, 2P, 60 seconds, Sprayer v Sprayer: 60 matches. Each matchup pools both seat
orders (ten matches per stage). "Share of painted" is the higher difficulty's
held paint over both players' held paint at time-up; splats are credited enemy
eliminations.

| Matchup | Stage | Matches | Higher paints more | Higher share of painted, median (range) | Splats, median higher / lower |
|---|---|---|---|---|---|
| normal v easy | crossfeed | 10 | 7/10 | 0.527 (0.378-0.613) | 0 / 0 |
| normal v easy | press_tower | 10 | 5/10 | 0.493 (0.352-0.574) | 0 / 0 |
| hard v normal | crossfeed | 10 | 3/10 | 0.476 (0.362-0.541) | 0 / 0 |
| hard v normal | press_tower | 10 | 4/10 | 0.482 (0.368-0.567) | 0 / 1 |
| hard v easy | crossfeed | 10 | 2/10 | 0.421 (0.315-0.577) | 0 / 0 |
| hard v easy | press_tower | 10 | 3/10 | 0.442 (0.360-0.543) | 0.5 / 0 |

At this tool pair the shipping difficulties do not order by held paint: Normal
edges Easy on Crossfeed and ties it on Press Tower, and Hard holds less than
both Normal and Easy on both stages. Splats are nearly absent, so these matches
measure painting, not fighting. One tool pair and five seeds are descriptive,
not a strength ranking with confidence. Receipts, with each run's arguments,
pins and both CSVs: `out/validation/tool2-ladder-20261029/<first>-v-<second>/`.

Next concrete gap: why Hard paints less (its reaction, aim and territory values
are in `tuning.json` `bots`), and whether other tool pairs agree, is evidence
for the attended `ROAD#10` comparison. `TOOL#3` (below) ran the five-tool
matrix with Hard on both sides; it does not compare difficulties.

Not decided here: whether the difficulties are spaced well, whether any bot is
fit to ship, and any tuning change. Those were `ROAD#10` and `POL#27`, accepted
by the author on 2026-11-05 at current values without the attended comparison.

## Turbulence bands and Hard five-tool rerun (`TOOL#3`, 2026-10-29)

**Done 2026-10-30.** Before this, region bands existed only for `crossfeed`,
`press_tower` and `turbulence_blockout`; every other stage fell back to one
`whole_arena` band, and the first five-tool report covered the two arenas only
([slice 3](#headless-balance-runner-tool1-2026-10-25)) under Skirmish, which has
no line-of-sight, route or ballistic-intercept solver (see
[Skirmish measurement bot](#skirmish-measurement-bot)).

Scope: give the four Turbulence trials (`turbulence_mover_trial`,
`turbulence_gate_trial`, `turbulence_ferry_trial`, `turbulence_slab_trial`) the
blockout's five X bands, after a check that each stage's `level_bounds` matches
the blockout's (all five currently share the 4200 x 1900 out-of-bounds rectangle).
The full `turbulence` candidate is 6200 wide and keeps `whole_arena` until its
own bands are scoped. Then rerun the 25 ordered pairs x five seeds matrix with
`TOOL#2`'s `match` policy at Hard on both sides, on Crossfeed, Press Tower, the
blockout and the four trials, and record per-band final held paint in the
arena owners. A stage the match policy cannot route is reported with that limit,
not dropped.

Acceptance: `balance_regions` and `balance_runner` pass in Debug; the existing
three partitions are unchanged; the matrix completes with receipts under
`out/validation/`, and its summaries are filed as descriptive evidence in
[Crossfeed](polish-crossfeed.md), [Press Tower](polish-press-tower.md) and
[Turbulence](polish-turbulence.md), each stating that the bands are final held
area, not route use, and naming the policy's limits.

**Bands.** `stage_partition` resolves the four trials to the blockout's five
`x_band` bands (`home_a` 0-600, `approach_a` 600-1100, `contest` 1100-2900,
`approach_b` 2900-3400, `home_b` 3400-4000, full height). The `balance_regions`
self-check now pins, per trial, that the out-of-bounds rectangle equals the
blockout's and its camera X extent is the same 0-4000, that its bands equal the
blockout's band for band, that the bands span that camera extent exactly (the
100-unit out-of-bounds margins stay `outside_partition`, which held no paintable
area in any match), and that the 6200-wide `turbulence` candidate still falls
back to `whole_arena`. Dropping one trial from `stage_partition` fails the case.
`balance_regions` passes in Debug and Release and `balance_runner` in Debug
(it is Debug-only); the Crossfeed, Press Tower and blockout cut checks are unchanged.

**Matrix, 2026-10-30 (Debug, clean `c2630ef1`).** All 25 ordered
Sprayer/Sniper/Roller/Mister/Bucket pairs x seeds 0, 1, 17, 42, 2026 on seven
stages: Crossfeed, Press Tower, the blockout and the four trials. Every match is
2P A/B, 60 seconds, the shipping `MatchBotDriver` at Hard on both seats
(`match:hard:hard`): **875 matches**, one `run-balance.ps1` job per ordered pair
(35 matches each), all 25 exiting 0 with no failed or repeated job. Each
`run-info.json` records a clean worktree, `ColourWars.exe` SHA256
`B030E4A17F0F6973217576CC0C66B39E67D86107C07521BA431ECC194FD5E024` and
`tuning.json` SHA256
`691ED30550AC42A912F1948E50F7987471DDB88BA784D00C51079D1075893034`.

| Stage | Team A share of painted, median [min, max] % | Painted / paintable, median % | Matches with a credited elimination | Credited eliminations |
|---|---|---|---|---|
| crossfeed | 50.9 [6.5, 92.8] | 57.3 | 90/125 | 156 |
| press_tower | 51.4 [6.5, 94.5] | 54.8 | 99/125 | 214 |
| turbulence_blockout | 49.2 [4.9, 94.2] | 53.1 | 115/125 | 243 |
| turbulence_mover_trial | 51.2 [6.2, 90.3] | 51.1 | 98/125 | 160 |
| turbulence_gate_trial | 49.9 [5.5, 93.6] | 51.4 | 81/125 | 126 |
| turbulence_ferry_trial | 47.9 [4.0, 94.0] | 52.0 | 103/125 | 185 |
| turbulence_slab_trial | 52.3 [8.6, 94.4] | 47.8 | 63/125 | 93 |

Every seat held paint in every match, every band of every stage held paint in
at least 124 of its 125 matches, and no match recorded an uncredited projectile
lethal; observed death edges equal credited eliminations on every stage. So no
stage was unroutable in the sense of a seat stuck at home with nothing painted;
whether the bots *travel* each trial's route is `POL#42`'s measure, not this one.
Unlike `TOOL#2`'s Sprayer-only ladder, Hard bots fight here: the blockout had a
credited elimination in 115 of 125 matches.

Each tool's own share of painted against every opponent, pooled over both seats
(50 matches per cell, the mirror counted from both sides), median %:

| Stage | Sprayer | Sniper | Roller | Mister | Bucket |
|---|---|---|---|---|---|
| crossfeed | 53.7 | 13.0 | 55.7 | 56.0 | 44.3 |
| press_tower | 52.3 | 13.1 | 57.0 | 53.9 | 42.8 |
| turbulence_blockout | 56.6 | 12.0 | 47.2 | 62.5 | 47.0 |
| turbulence_mover_trial | 54.0 | 13.6 | 49.7 | 62.0 | 47.9 |
| turbulence_gate_trial | 59.2 | 14.9 | 46.0 | 63.1 | 47.8 |
| turbulence_ferry_trial | 55.7 | 13.0 | 51.5 | 59.7 | 45.5 |
| turbulence_slab_trial | 52.2 | 13.6 | 51.8 | 59.2 | 48.7 |

The Hard Sniper bot holds about an eighth of the painted area against any other
tool on every stage, and two Hard Snipers together cover a median 9.8-14.3% of the arena;
on the Turbulence family Mister holds the most and Roller less than on the two
arenas. These describe this bot with these tools; whether a human Sniper's
held-paint/elimination trade is acceptable is the existing `POL#27` question,
and nothing here says a bot plays a tool the way a person does.

Per-band final held paint is filed in the arena owners:
[Crossfeed](polish-crossfeed.md#hard-five-tool-matrix-tool3-2026-10-30),
[Press Tower](polish-press-tower.md#hard-five-tool-matrix-tool3-2026-10-30) and
[Turbulence](polish-turbulence.md#hard-five-tool-matrix-tool3-2026-10-30).

**Limits.** Bands are final held area at time-up: not route use, visits, time
held, spawn pressure or hill control. A moving scored tile (mover, ferry, slab)
is credited to the band its final position falls in, so the ferry trial's band
denominators vary by match. Five chosen seeds are not population samples; ranges
are observed minima/maxima. The `match` policy is the shipping Hard bot, one
difficulty, 2P only; ordered swaps combine side, seat RNG, tool and conditional
bot behaviour, so they do not isolate map-side fairness. No 1v1/2v2 human,
uneven three-player or attended result follows.

Receipts: `out/validation/tool3-hard-matrix-20261030/` - one `<p1>-v-<p2>/`
folder per ordered pair (`run-info.json` with arguments, pins and CSV hashes,
`matches.csv`, `matches.paint.csv`, logs), `driver.log`, the driver
`tool3-matrix.ps1`, the analyzer `tool3_analyze.py` and its output `summary.md`
(SHA256 `C804069F646B296AA297A4D2CE512C7F6DE57AFB86F5A6BE19FF1DCF1165C06C`), which has
every table here plus per-band coverage for every equal-tool control.

Not decided here: arena acceptance, route or geometry changes, tool tuning and
bot difficulty. Those were `POL#21`, `POL#27`, `POL#31` and `ROAD#10`, accepted
by the author on 2026-11-05 (the slab trial and Press Tower joined Random). The 6200-wide `turbulence` candidate still has no bands; scoping them
is not open work until that candidate is chosen for review.

## Two-bot teams and teammate overlap (`REV#22`, 2026-11-10)

**Measured 2026-11-10: no waste found, so no ally cost term was added and
`REV#22` is closed.** The review asked whether two bots on one team duplicate
each other's work, because `MatchBotObservation` lists enemies only and
`MatchBot::choose_territory` costs a candidate by distance from the bot alone.
Two things already limit overlap: the driver skips every surface the bot's team
holds when it samples candidates, and each seat draws from its own seeded
sequence. What was unmeasured is whether both bots still head for the same
unpainted ground between replans.

**Method.** `scripts/run-balance.ps1 -Bot match`, unchanged, on the four Random
arenas (Crossfeed, Close Quarters, Press Tower, the Turbulence slab trial) at
seeds 0, 1, 17, 42 and 2026, Sprayer on every seat, one difficulty across all
seats, for Easy, Normal and Hard. The runner seats teams A/B/A/B, so
`-Players 2` is 1v1, `-Players 3` is two bots against one (2v1) and
`-Players 4` is 2v2. First pass: the three rosters at 20, 60 and 240 seconds
(20 is before the arenas fill, 60 is the `TOOL#2`/`TOOL#3` horizon, 240 the
default match). Second pass, added after the first was read: 1v1 at 40 and 120
seconds and 2v2 at 120 seconds. **720 matches** in 36 jobs of 20, all exiting 0,
plus one repeated job (Normal 2v2, 60 seconds) whose two CSVs have the same
SHA256. Every `run-info.json` records Debug, a clean worktree at `e5bc7fae`,
`ColourWars.exe` SHA256
`185AA945F905BAD2C96B0E6969688254F06E98D36D00A9A1235F43C2FE74C5CF` and
`tuning.json` SHA256
`D0B0D926547C781A535E423A1C07F4098676FAF924DFAACD361ACBA4729DD3B3`.

Measures, all from held paint at time-up:

- **Team gain**: a team's held area with two bots over the same team's held
  area in the 1v1 match of the same stage and seed. 2v1 keeps the opposition at
  one bot; 2v2 doubles it too.
- **Lesser teammate's share**: the smaller of the two teammates' held areas over
  their sum. A friendly repaint keeps the first painter's credit
  (`game/objects/paint_tile.cpp`), so a bot that trails its teammate over the same ground
  holds almost nothing: near 0 is shadowing, 0.5 an even split.
- **Equal bot-seconds**: painted area in 2v2 after T seconds over painted area
  in 1v1 after 2T, same stage and seed. Both spend the same bot time on both
  teams, so 1 means a second bot is worth as much as the first bot's next T
  seconds.
- **Band overlap**: over the cells of (region band, face), the sum of the
  smaller of two seats' shares of their own held paint; 1 is the same cells in
  the same proportions, 0 is disjoint. For scale, the same number between the
  lone bots of one side in two 1v1 matches of different seeds, which is what
  two bots that cannot affect each other produce.

The criterion written before the run was a team gain of about 1.6 or more at 20
seconds and a lesser share not near zero for "no waste", and a gain of 1.3 or
less on most cells or a lesser share routinely under 0.15 for "clear waste".
The gain half of it did not survive the first pass: one bot's own rate falls
as the ground near it fills (Crossfeed, Normal, 1v1: 37.8% of the arena painted
at 20 seconds, 50.2% at 40, 59.4% at 60, 75.8% at 120, 84.7% at 240), so a gain
under 2 does not separate teammates getting in each other's way from an arena
running out. The equal bot-seconds pass was added to separate them.

**60 seconds.** Median [min, max]; painted / paintable and the 2v1 gain are
over five matches, the 2v2 gain and lesser share over ten teams.

| Stage | Difficulty | 1v1 painted / paintable % | 2v2 painted / paintable % | Team gain, 2v1 | Team gain, 2v2 | Lesser teammate's share, 2v2 | Credited eliminations 1v1 / 2v1 / 2v2 |
|---|---|---|---|---|---|---|---|
| crossfeed | easy | 66.6 [65.6, 75.7] | 73.6 [69.7, 79.2] | 1.62 [1.29, 1.93] | 1.07 [0.90, 1.31] | 0.46 [0.35, 0.50] | 3 / 17 / 30 |
| crossfeed | normal | 59.4 [54.9, 70.9] | 78.7 [75.3, 82.2] | 1.76 [1.49, 1.90] | 1.33 [0.87, 1.81] | 0.43 [0.36, 0.49] | 9 / 11 / 19 |
| crossfeed | hard | 64.7 [62.0, 72.6] | 85.5 [78.8, 86.6] | 1.28 [1.20, 1.58] | 1.27 [1.00, 1.58] | 0.45 [0.19, 0.50] | 11 / 16 / 23 |
| close_quarters | easy | 31.3 [26.2, 45.3] | 47.6 [42.7, 54.4] | 2.78 [1.23, 5.69] | 1.23 [0.63, 5.56] | 0.41 [0.04, 0.49] | 11 / 27 / 31 |
| close_quarters | normal | 32.2 [28.8, 38.6] | 40.8 [39.5, 52.3] | 1.60 [0.54, 4.66] | 1.38 [0.63, 2.78] | 0.37 [0.22, 0.47] | 17 / 17 / 30 |
| close_quarters | hard | 32.8 [31.0, 48.5] | 52.8 [41.1, 65.9] | 2.79 [2.04, 3.56] | 1.55 [0.33, 2.96] | 0.39 [0.24, 0.49] | 13 / 15 / 28 |
| press_tower | easy | 69.1 [64.8, 70.5] | 85.4 [77.6, 90.5] | 1.80 [1.09, 1.89] | 1.19 [0.87, 2.54] | 0.46 [0.41, 0.50] | 10 / 12 / 18 |
| press_tower | normal | 61.3 [56.6, 66.8] | 82.7 [75.9, 86.2] | 1.71 [1.50, 1.78] | 1.34 [1.11, 1.84] | 0.46 [0.40, 0.49] | 10 / 20 / 26 |
| press_tower | hard | 62.8 [61.7, 67.5] | 78.7 [74.0, 81.0] | 1.40 [1.00, 1.67] | 1.23 [0.95, 1.83] | 0.45 [0.34, 0.50] | 8 / 19 / 36 |
| turbulence_slab_trial | easy | 56.4 [53.1, 67.0] | 71.2 [68.3, 75.0] | 1.55 [1.23, 2.26] | 1.27 [0.73, 2.09] | 0.41 [0.24, 0.49] | 2 / 11 / 23 |
| turbulence_slab_trial | normal | 51.0 [43.8, 58.7] | 74.6 [68.5, 78.8] | 1.95 [1.73, 5.03] | 1.29 [0.92, 3.19] | 0.45 [0.21, 0.50] | 8 / 10 / 15 |
| turbulence_slab_trial | hard | 57.0 [51.3, 61.2] | 77.0 [74.6, 80.4] | 1.46 [1.26, 2.10] | 1.31 [0.95, 2.00] | 0.41 [0.37, 0.47] | 2 / 11 / 13 |

**The four arenas pooled**, by duration: 20 matches for the 2v1 gain, 40 teams
for the 2v2 columns.

| Duration | Difficulty | Team gain, 2v1 | Team gain, 2v2 | Lesser teammate's share, 2v2 | 2v2 teams with lesser share under 0.15 |
|---|---|---|---|---|---|
| 20 s | easy | 1.56 [1.05, 2.40] | 1.40 [0.68, 2.88] | 0.45 [0.08, 0.50] | 2/40 |
| 20 s | normal | 2.02 [1.02, 19.22] | 1.44 [0.62, 21.26] | 0.42 [0.23, 0.50] | 0/40 |
| 20 s | hard | 1.76 [1.15, 3.03] | 1.52 [1.07, 2.32] | 0.44 [0.32, 0.50] | 0/40 |
| 60 s | easy | 1.71 [1.09, 5.69] | 1.17 [0.63, 5.56] | 0.45 [0.04, 0.50] | 3/40 |
| 60 s | normal | 1.77 [0.54, 5.03] | 1.33 [0.63, 3.19] | 0.44 [0.21, 0.50] | 0/40 |
| 60 s | hard | 1.55 [1.00, 3.56] | 1.31 [0.33, 2.96] | 0.43 [0.19, 0.50] | 0/40 |
| 240 s | easy | 1.64 [1.19, 3.31] | 1.07 [0.66, 3.57] | 0.43 [0.31, 0.50] | 0/40 |
| 240 s | normal | 1.58 [1.21, 3.43] | 1.16 [0.66, 5.53] | 0.43 [0.17, 0.50] | 0/40 |
| 240 s | hard | 1.88 [1.13, 3.18] | 1.07 [0.67, 1.93] | 0.43 [0.15, 0.50] | 1/40 |

At 240 seconds 2v2 has painted a median 90.2-96.6% of Crossfeed, Press Tower
and the slab trial at every difficulty and 71.3-90.2% of Close Quarters, so the
2v2 gain there is bounded by the arena, not by the bots. The extreme gains are
Close Quarters matches where the 1v1 bot held almost nothing at time-up.

**Equal bot-seconds**, pooled over the four arenas (20 matches per row):

| T | Difficulty | Painted, 2v2 for T over 1v1 for 2T | Matches where 2v2 painted at least as much |
|---|---|---|---|
| 20 s | easy | 0.99 [0.79, 1.25] | 9/20 |
| 20 s | normal | 1.07 [0.82, 1.29] | 14/20 |
| 20 s | hard | 1.06 [0.75, 1.38] | 13/20 |
| 60 s | easy | 0.99 [0.85, 1.74] | 9/20 |
| 60 s | normal | 1.06 [0.87, 1.42] | 15/20 |
| 60 s | hard | 0.99 [0.62, 1.60] | 9/20 |
| 120 s | easy | 0.99 [0.75, 1.52] | 9/20 |
| 120 s | normal | 1.03 [0.98, 1.63] | 18/20 |
| 120 s | hard | 1.00 [0.58, 1.43] | 10/20 |

By stage and difficulty the 36 medians run from 0.85 to 1.33. The lowest are
Easy on Crossfeed (0.85 at 20 seconds, 0.92 at 60, 1.00 at 120) and Hard on
Close Quarters at 120 seconds (0.91); the highest is Normal on Close Quarters
at 120 seconds (1.33).

**Band overlap at 60 seconds.** Ten teams per teammates cell, 20 seed pairs
per solo cell.

| Stage | Difficulty | Teammates in 2v2 | Solo bots, different seeds |
|---|---|---|---|
| crossfeed | easy | 0.51 [0.41, 0.75] | 0.67 [0.57, 0.82] |
| crossfeed | normal | 0.54 [0.04, 0.71] | 0.79 [0.65, 0.88] |
| crossfeed | hard | 0.60 [0.54, 0.77] | 0.77 [0.55, 0.89] |
| close_quarters | easy | 0.90 [0.27, 0.98] | 0.85 [0.56, 1.00] |
| close_quarters | normal | 0.90 [0.68, 0.98] | 0.91 [0.68, 0.98] |
| close_quarters | hard | 0.72 [0.65, 0.95] | 0.93 [0.86, 0.99] |
| press_tower | easy | 0.63 [0.31, 0.76] | 0.71 [0.52, 0.85] |
| press_tower | normal | 0.55 [0.37, 0.76] | 0.70 [0.44, 0.87] |
| press_tower | hard | 0.70 [0.49, 0.81] | 0.75 [0.51, 0.86] |
| turbulence_slab_trial | easy | 0.53 [0.44, 0.72] | 0.70 [0.50, 0.88] |
| turbulence_slab_trial | normal | 0.65 [0.36, 0.72] | 0.59 [0.31, 0.85] |
| turbulence_slab_trial | hard | 0.68 [0.57, 0.90] | 0.75 [0.52, 0.90] |

Over all three durations the teammates' median is below the solo scale in 31
of 36 cells. Close Quarters has no bands (`stage_partition` gives it one
`whole_arena` region), so its cells are faces only and read high for both.

**Conclusion.** Nothing here shows two same-team bots wasting effort.

- A second bot per team paints as much as doubling the match for one bot per
  team: the pooled ratio is 0.99-1.07 at every difficulty and horizon. The
  shortfall of the team gain from 2 is the arena filling, which one bot meets
  as well.
- With the opposition held at one bot, the second bot adds a median 0.55-1.02
  of a solo bot's held paint (pooled 2v1 gain 1.55-2.02).
- Neither teammate shadows the other: the lesser teammate holds a median
  0.42-0.45 of the pair's paint, and under 0.15 in 6 of 360 teams.
- Teammates' held paint is spread over the bands no more alike than two
  independent bots' is, and usually less.

An ally term in the territory cost would move every recorded outcome for
rosters with two same-team bots, on a roster that is a minority in a four-seat
couch game. What it could recover is the shortfall against bots that cannot
get in each other's way: at most 1% in the pooled medians, 6-9% in four of the
36 cells and 15% in the weakest. These numbers do not say what coordination
beyond that would add. It was not added. The weakest cell, Easy on Crossfeed in the first minute, is where to
look first if two-bot teams are ever seen crowding in play.

**Limits.** Five chosen seeds and a Sprayer mirror: descriptive, with observed
ranges, not confidence intervals. Debug build. Held paint at time-up is not
paint laid: a team's repaint of its own ground and ground lost to the enemy are
both invisible, and the runner does not export laid area. No direct sample of
the two bots' chosen goals exists, because `MatchBot` exposes none; the credit
split and band overlap stand in for it. The opt-in `bot_team_trials` case does
count `ally` transitions (`ally_repaint_events`, `ally_transition_area`), but
they are not an overlap measure either, since a friendly repaint changes no
owner. 2v2 also
doubles the fighting (eliminations rise with every added bot), and the equal
bot-seconds comparison gives the 1v1 bots twice as long to travel and respawn.
A human teammate, mixed difficulties and mixed tools were not run.

Receipts: `out/validation/rev22-two-bot-teams-20261110/` - one
`<difficulty>-<roster>-<duration>s/` folder per job (`run-info.json` with
arguments, pins and CSV hashes, `matches.csv`, `matches.paint.csv`, logs),
`normal-2v2-60s-repeat/`, `driver.log`, the driver `rev22-matrix.ps1`, the
analyzer `rev22_analyze.py` and its output `summary.md` (SHA256
`D837B9B9098971E172B00304A2D1C2D643DF1ABB09C7619EEDDEA5AB8D26764F`), which has
every stage, difficulty and duration for each table here.

Not decided here: bot difficulty, tool tuning and whether a bot is a good
teammate for a person. The first two were `ROAD#10` and `POL#27`, accepted by
the author on 2026-11-05; the last is a play question.

## Weapon profiles (2026-11-10)

**Decided by the author, 2026-11-10.** A tool is judged on two measures, not
one: **eliminating opponents** and **painting the arena**. Each tool has an
intended profile on the two, and a balance finding is a tool missing its own
profile, not a tool scoring below the others on paint:

| Tool | Profile |
|---|---|
| Sprayer | The all-rounder: paints and fights about evenly. |
| Sniper | The killer: the most eliminations, the least paint. |
| Roller | A painter that dies often. |
| Mister | A painter that dies often. |
| Bucket | Between the all-rounder and the painters. |

The Sniper's low held-paint share (about an eighth of the painted area in
[`TOOL#3`](#turbulence-bands-and-hard-five-tool-rerun-tool3-2026-10-29)) is therefore
its intended trade, not a fault by itself.

The profiles are data, in [`scripts/balance/profiles.json`](../scripts/balance/profiles.json),
which the `TOOL#4` analyser checks every run against. Bucket's "between" is
written there as between Sprayer, Roller and Mister.

**What the existing evidence says.** `TOOL#3`'s 875 Hard matrix CSVs, read by
that analyser ([below](#analyser-on-the-tool3-matrix-2026-11-10)): the Sniper
and Mister sit on their profiles, and so do every tool's eliminations and
deaths. Paint departs twice. The all-rounder out-paints the Roller (Sprayer
0.614, Roller 0.573), so the Roller is not shown to be a painter beside it.
Bucket paints less than the all-rounder and both painters (0.520), above only
the Sniper. These are Hard bots on the
2026-10-30 build, before paint underfoot shipped (`POL#30`) and before sub
weapons (`ROAD#11`), over five chosen seeds: they show direction, not today's
balance, and nothing here says a person plays a tool as the bot does.

**What follows for measurement.** Score is still painted area, so a 1v1 Sniper
loses on purpose; its eliminations are worth something only when they let a
teammate paint. A role tool is therefore checked by its **team result in 2v2**
(a Sniper and a painter against two painters), which no sweep has measured yet.
The balance matrix tool reports both measures for every tool and flags a tool
against its profile ([`TOOL#4`](#balance-matrix-tool-tool4)).

## Balance matrix tool (`TOOL#4`)

**Plan, 2026-11-10.** The headless runner plays real, deterministic matches,
but each past sweep (`TOOL#1`-`TOOL#3`, `REV#22`) used a driver and analyser
kept only in a gitignored `out/validation/` folder. Their seeds were chosen by
hand and they report medians and min/max, not confidence intervals. The runner
seats every bot with the default grenade (`scripts/acceptance/balance_runner/runner.cpp`
sets no sub) and can try a tuning value only by editing `tuning.json`, which
`run-balance.ps1` refuses. No matrix has measured the game as it ships, with
paint underfoot on and subs carried.

`TOOL#4` makes the runner a repeatable report and an A/B comparison. It reports
evidence and never proposes or applies a tuning value: the five-tool values
were accepted under `POL#27`, and the arena owners say not to retune from bot
observations. Slices, in order:

1. **Analyser (done 2026-11-10,
   [below](#analyser-on-the-tool3-matrix-2026-11-10)).**
2. **Matrix driver (implemented 2026-11-10).** `scripts/run-balance-matrix.ps1` with one design file,
   `cases.psd1`-style. It compiles once, runs jobs in parallel, resumes after a
   kill, and repeats one job as a byte-identity check.
3. **Loadout and tuning patch (implemented 2026-11-10).** Runner flags for each seat's sub and an
   in-memory tuning patch, limited to `weapons`, `projectiles`, `subs` and
   `bots`. An empty patch reproduces the rows byte for byte. The runner stays
   Debug-only.
4. **The shipped game.** Every ordered tool pair on the four Random stages,
   over a consecutive seed block, at Hard and Normal 1v1, plus 2v2 rosters
   pairing each tool with each painter. Filed here as evidence.
5. **A/B (implemented 2026-11-10).** `analyze.py --compare` between two runs of one design: per-cell
   and per-tool change with intervals, and a check that nothing moved beyond a
   margin set in the design file. This serves future changes (new tools,
   pickups, an elimination burst) rather than reopening the accepted values.

Bot results stay evidence, not acceptance (see [above](#headless-balance-runner-tool1-2026-10-25)).

### Running and resuming a matrix (2026-11-10)

From an x64 Developer shell:

```powershell
scripts/run-balance-matrix.ps1 -Design scripts/balance/shipped.psd1 -OutputDirectory out/validation/shipped-matrix -PlanOnly
scripts/run-balance-matrix.ps1 -Design scripts/balance/shipped.psd1 -OutputDirectory out/validation/shipped-matrix -Parallel 6
```

The data file names stages, difficulties, one consecutive `SeedStart`/`SeedCount`
block (1-64 seeds), duration, roster generators and variants. The shipped design
has 344 jobs / 5,504 matches: all 25 ordered 1v1 tool pairs, and each tool with
Roller or Mister against two of that painter, side-swapped, on the four Random
arenas at Normal and Hard. Sixteen seeds (1000-1015) improve the sparse
elimination evidence over TOOL#3's five; the reported intervals, not that count,
say what a cell resolves. The diagnostic ladder uses the same stages, tools and
seed block at Hard, changing only aim error or reaction delay in each variant.
`-PlanOnly` validates and prints counts without compiling or creating evidence.
The driver compiles the Debug runner once; `-RunnerExecutable` reuses an explicitly
chosen compiled runner. `-SkipBuild` skips the game build, not that one compile.

Outputs must be in a dedicated directory below this checkout's `out/`. Run the
same command there to resume. A success is reused only when its signature and
both CSV hashes match. Changed executable, game library, source, reviewed
runtime assets, design or driver refuses resume and needs a new directory.
The staging allowlist in `cmake/runtime_assets.cmake` defines asset pins; an absent
optional bank is recorded as absent, and private/source-only files are excluded.
Generated variant patches are hashed before and after each job and at the end.
Failed and interrupted attempts stay in their own folders. A dead driver lock
is retained as a stale-lock receipt and reclaimed after its PID is verified gone;
a live owner is refused. Each Windows native job is assigned to a private
kill-on-close Job Object at process creation; cancellation or a killed driver
cannot leave its runner processes behind. `-RunTimeoutSeconds` bounds each job,
and cancellation skips queued jobs and terminates only owned runners.

`matrix-info.json` records the design, input pins and final outcome. Every job
has arguments, logs, timing, hashes and its attempt path in `jobs/<id>/job.json`.
The first job is repeated byte for byte, main and companion, outside the report's
observations. Each variant's aggregate is `variants/<name>/matches.csv` and
`matches.paint.csv`; companion `match_row` joins are renumbered to the aggregate.
Analyse one variant at a time:

```powershell
python scripts/balance/analyze.py out/validation/shipped-matrix/variants/baseline --out out/validation/shipped-summary
python scripts/balance/analyze.py out/validation/changed/variants/baseline --compare out/validation/shipped-matrix/variants/baseline --design-file out/validation/shipped-matrix/matrix-info.json --out out/validation/ab-summary
```

A/B refuses different observed stage/seed/team/tool/sub cells. It reports current
minus baseline with independent Welch intervals for each cell and tool, including
whole-team 2v2 rosters. `ComparisonMargins` in the design gives a symmetric margin
per measure. **Within margin** requires the complete interval inside it;
**beyond margin** requires the interval entirely outside it; **inconclusive**
certifies neither. A seed with no spread or only one observation still gets no
interval. Unadjusted checks across many cells are descriptive evidence.

### Runner loadouts, patches and schema 3 (2026-11-10)

`run-balance.ps1 -Subs grenade,wall -TuningPatch <file>` adds one concrete sub
per seat and a read-only JSON patch, for example:

```json
{"bots.hard.reaction_steps": 1, "weapons.sniper.ammo_usage": 0.10}
```

A patch is a flat object of existing dotted numeric/boolean leaves under
`weapons`, `projectiles`, `subs` or `bots`. Repeated, missing, string, container,
unknown and type-changing entries are refused. The result is parsed and
validated as an in-memory `GameTuning`; neither staged nor authored tuning is
written. `{}` reproduces unpatched row and companion bytes. Omitted subs retain
the game's grenade default. The native flags are `--subs` and `--tuning-patch`.

The main CSV is now schema **3**, **101 columns**: the original 81 fields retain
their order, followed by `sub`, `engaged_ticks`, `refilling_ticks`,
`ready_fire_intents` and `observed_sub_cooldown_resets` for each of four seats.
Companion schema 1 remains 29 columns and names main schema 3. Earlier schema-2
records above remain records of that version. The analyser reads both versions,
separates per-seat sub configurations and reports whole-team paint and
elimination rates for the side-swapped 2v2 rosters as well as individual tools.


### Analyser on the `TOOL#3` matrix (2026-11-10)

[`scripts/balance/analyze.py`](../scripts/balance/analyze.py) is standard-library
Python, so it needs no build; its test runs in CI's lint job. Give it CSVs or
folders:

```
python scripts/balance/analyze.py out/validation/<run> --out out/validation/<run>-summary
```

It writes `summary.md` and `summary.json`, and refuses an output folder inside
the repository but outside `out/`. Runs are split by roster, bot and duration.
For 1v1, one seed's two matches of a tool pair on a stage, X on team A and X on
team B, form one unit. A tool's figure is the equal-weight mean of its cells
against other tools (stage x opponent), mirrors left out. Every estimate,
whether a tool's figure, the gap between two tools or a tool against the field,
is a weighted sum of cells. Its interval comes from the spread over seeds of
each unit's contribution, with Welch-Satterthwaite degrees of freedom, so two
tools' complementary shares of the same matches count with their covariance.
Where a stratum has one seed, or no spread at all, it reports no interval
rather than a false certainty. A pair missing its swapped match is counted
and left out of the estimates, since one side carries the whole side advantage.

The profile levels are an order, `least` < `low` < `mid` < `high` < `most`, and
a tool is judged against the other tools' stated levels on the same measure: a
tool stated higher than another should be significantly above it. A reversed
order makes both tools `off`, and the note names the other. When every stated
order holds significantly, the tool's level is `confirmed`; otherwise it is
`consistent`. `most` and `least` are also judged against every tool, and `mid`
is also off as the outright most or least. A tool with no differently stated
tool to compare with is judged against the median of the others. `between`
lists tools that have their own entries: confirmed when above one and below
another, off when beyond all of them. A level is `no interval` when the data
cannot give one, and `not in data` when the tool did not play. The comparisons
are unadjusted at 95%, so among many checks one can flip by chance.

**Method check on `TOOL#3`.** The input is the 875 retained Hard 1v1 CSVs:
seven stages, five seeds, 350 side-swapped pairs and 175 mirrors. Output is in
`out/validation/tool4-analyzer-tool3/`.

| Tool | Share of painted | Eliminations / min | Deaths / min | Profile |
|---|---|---|---|---|
| Sprayer | 0.614 ± 0.008 | 0.668 ± 0.091 | 0.425 ± 0.069 | kills mid confirmed; **paint mid off** (above Roller) |
| Sniper | 0.124 ± 0.004 | 1.143 ± 0.112 | 0.104 ± 0.034 | kills most, paint least: both confirmed |
| Roller | 0.573 ± 0.009 | 0.439 ± 0.066 | 1.060 ± 0.105 | deaths high confirmed; **paint high off** (below Sprayer) |
| Mister | 0.669 ± 0.008 | 0.446 ± 0.073 | 0.993 ± 0.104 | paint high, deaths high: both confirmed |
| Bucket | 0.520 ± 0.009 | 0.593 ± 0.085 | 0.707 ± 0.098 | deaths between confirmed; **paint between off** (below all three) |

What the method check shows about the method:

- **Paint is precise; eliminations are not.** One unit's paint share varies by
  a median 0.038 within a cell, so five seeds resolve a tool's paint to ±0.008,
  and an A/B change of 0.017 is detectable. Eliminations vary by 0.45 a minute
  within a cell, and a tool's rate is only resolved to about ±0.09; a change of
  0.17 a minute needs this much data to be seen. Elimination claims therefore
  need more seeds than paint claims, which slice 4's design has to size for.
- **The side swap balances sides but does not pair.** Team A's advantage is
  0.001 ± 0.005 of the painted area. Within a cell, a tool's two orientations
  correlate at 0.05, so the two matches of one seed are nearly independent. An
  A/B comparison on the same seeds is therefore analysed unpaired (slice 5).

These describe the 2026-10-30 build and its bots, not today's game.

## Bot diagnostics (`TOOL#5`)

**Plan, 2026-11-10.** The shipping bot plays every tool by one policy
(`REV#20`), so each number is bot and tool together. The Sniper's profile is
intended, but its bot paints only when no enemy is in view, and its reach
exceeds `bots.sight_distance`, so any visible enemy holds its aim. How much of
its low paint is the tool and how much the bot is unmeasured. `TOOL#5` adds
per-seat counters to the runner, with no change to game code: time engaged
(derived from `bot_world()` by the bot's own sight, line-of-sight and reach
rule), time refilling (`MatchBot::refilling()`), shots and sub uses. With
`TOOL#4`'s tuning patch it also runs an aim-error and reaction ladder on the
`bots.hard` record. Its output feeds `REV#20` and decides nothing.

**Implemented diagnostic slice, 2026-11-10.** Schema 3 exports per-seat engaged
and refilling ticks, ready primary-fire intents and observed sub cooldown resets.
Engagement re-evaluates the bot driver's living-enemy, sight, line-of-sight and
ballistic reach rule against the current `bot_world()`; it describes opportunity,
not the policy's held decision between reactions. Refilling reads the driver's
post-think `MatchBot::refilling()`. The analyser pools engaged/refilling over alive
ticks and the two action proxies over active minutes. The separate
[`bot-ladder.psd1`](../scripts/balance/bot-ladder.psd1) measures baseline, zero and
0.20-radian aim error, and 1-step and 24-step reaction delay, changing one factor
at a time; it is evidence and never applies its values to the game.

**Remaining decision: exact paid-event counters.** A ready intent can be killed
by a contact later in the tick, before its weapon runs. An observed cooldown
rise identifies a sub use at shipped cooldowns but can miss a patched cooldown
at or below one tick, including zero. The public weapon APIs expose no exact
paid-event observer, and their visual pulses intentionally coalesce stream
shots. These columns are named as proxies and are not shots or exact sub-use
counts. Completing that part requires the author to allow a passive game-code
observer, which the original no-game-code-change scope forbids. The diagnostic
slice therefore does not close TOOL#5.
