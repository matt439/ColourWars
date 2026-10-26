# Gameplay tuning

The game reads `game/content/tuning.json` before creating the application. The
build stages that file beside the executable. `GameTuning` owns the records that
weapons and projectiles borrow; gameplay mechanics stay in C++, while authored
balance values select their amounts.

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
`game/content_value.h`, which `presentation.json` and the level definitions use
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
| Extent, cap or divisor | finite, `> 0` | player `x_initial_velocity` (zero never leaves rest) and `x_deacceleration` (the braking divisor, and what stops a released stick); projectile `delete_timer`, `size`, `collision_size` and diffusion `end_scale`; `projectiles.max_velocity`; and the fields that were already positive |
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
arena reviews (`POL#21`, `POL#31`), which still require attended decisions. Levels
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
   stays a developer tool: shipping AI opponents is outside the chosen scope
   ([match scope](feature-roadmap.md#match-scope-decisions-2026-10-15)).
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

This completes the finite `TOOL#1` scope. `POL#27` and `POL#31` remain attended
decisions; the report neither authorizes retuning nor accepts a new bot opponent.
`POL#36` remains its separately selected paint-underfoot prototype, using a future
explicit switch-off comparison; no such mechanic was enabled in this baseline.
