# Developer console work record

The finished, deferred and unscoped `DBG` rows of the
[developer console](debug-console.md), and its `CI#1` row: what each changed,
why, and how it was checked. They were split out of the reference on
2026-10-30 (`HYG#31`) so that page describes the console as it is; the
commands these rows added are documented there. [TODO.md](../TODO.md) indexes
the open ones.

## Bind word round trips (`DBG#9`)

**Done (2026-10-28).** The author approved autonomous bounded follow-ups.
The third ranked authoring finding was a persistence defect: a valid
`bind f12 echo C:\temp\` stores its command, but serializing that spaced line
places a closing quote after the terminal backslash. The current tokenizer
reads that pair as a literal quote; a fresh session then rejects the generated
binds file and falls back to defaults.

The encoder now places a quoted word's terminal backslash run outside its
closing quote, using the existing adjacent quoted/bare word segments. Saved
binds use the same encoder for forced `//` quoting. Printable-ASCII admission,
interior backslashes, literal-quote escaping, dollar doubling, statement
splitting, comments and malformed-input refusals are unchanged.

The native Debug baseline reproduced a complete table falling back to defaults
and a generated `.bad` file for the accepted bind. After the fix, seven actual
private save/reload tables preserve sibling bindings, explicit removals and
saved bytes, with no `.bad` file. Forty decoded word/statement round trips
cover empty, spaced, quoted and semicolon words with trailing slash runs;
wide launch arguments and a following statement preserve exact echo output.
Dollar/comment/quoted/multiple-statement binds and the existing refusals pass.

Both game builds and the full console acceptance case pass. Debug runs the
new cases and all existing console/script/replay/tuning checks; Release
retains inert hooks, shared layout and absence of developer RTTI. Independent
review passes. No player's binds or save were read or changed.

Evidence under `out/validation/implementation-loop-20261028/`: failing baseline
`dbg9-baseline/d470bf52074041208722c6c8b5d20354`, final both-preset batch
`dbg9-candidate/3bfe705104f949cf8914753e5d1aa395`. The final receipt verifies
all six source/test files remained unchanged during qualification.

## Executable script discovery (`DBG#12`)

**Done (2026-10-28).** Script listing and Tab completion now recognize
mixed-case `.cfg` extensions and require regular files. Previously,
`exec upper` could run a physical `upper.CFG` on Windows while discovery
omitted it; Tab also advertised directories named `*.cfg` that execution
refused and listing omitted.

With quoted completion complete, the ranked next slice is:

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | The shared script-name callback disagrees with execution about case and regular files. | Selected as `DBG#12`: make discovery reflect executable files while preserving search order and name grammar. Bounded callback change with private files/directories and native execution/list/completion controls. |
| 2 | The editor font-retirement failure is intermittent even with identical executable bytes. | Continue the separately bounded lifecycle diagnosis; no new feature depends on clearing that gate. |
| 3 | Reserved screenshot basenames need a defined naming policy. | Keep separate from discovery; no device-path experiment. |

Discovery compares extensions using Windows' case-insensitive ordinal comparison
([API](https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-comparestringordinal)).
A separate per-entry regular-file error code preserves directory traversal.
Stem spelling, name validation, exact deduplication, sorting, directory
precedence, execution/list file resolution and explicit command-name suffix
grammar remain unchanged.

The native old-source baseline first executed a private uppercase-extension
script successfully by stem, then reproduced its missing completion. Candidate
checks retain eight literal discovered names and ordered summaries, uppercase/
mixed/lowercase files in both folders, duplicate first-folder precedence and
a later regular-file fallback behind a directory decoy. Wrong extensions,
invalid names, directories and unchanged explicit suffix refusals are checked.
All seventeen input files, the complete private tree and caller path/profile
state remain unchanged.

Both game builds and the complete console CPU case pass, including the existing
Debug script/replay/tuning checks and Release's inert/layout/RTTI boundary.
Independent review confirms only the discovery condition changed; both source
hashes remained stable. No real profile, window, device or foreground input was
used by the new checks.

Evidence under `out/validation/implementation-loop-20261028/`: baseline
`dbg12-baseline/61543302c85847c5bfa92f293b7b2dfd`, both-preset candidate
`dbg12-candidate/1adfef9a2f72413f99310aa144cdc843`.

## Screenshot folder names (`DBG#13`)

**Done (2026-10-29).** `screenshot.dir NUL` formerly reported success, although
the later capture could not create that Windows device-named directory. The
setter now rejects reserved components and preserves its previous setting, so the
command reports the problem while it can still be corrected.

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | Screenshot folder validation accepts reserved Windows components and defers failure until capture. | Selected as `DBG#13`: validate sanitized components with pure command tests, without opening those paths or requesting a capture. Small immediate-feedback fix. |
| 2 | The input fixture's process-wide GDI count criterion remains unresolved. | Continue `ED#20` independently; its failed candidate and bounded diagnostics are retained. |
| 3 | Screenshot file basenames have a separate sanitizer and naming policy. | Keep separate from the folder setter; scope a pure naming policy before changing filenames. |

After the existing placeholder expansion, segmentation and sanitization, it rejects
case-insensitive ASCII `CON`, `PRN`, `AUX`, `NUL`, `COM1` through `COM9` and
`LPT1` through `LPT9` in any component. Windows applies its
[reserved-name rules](https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file)
to directories as well as files. The existing ASCII sanitizer already replaces
extensions, non-ASCII bytes and punctuation with underscores; validate its
result rather than introducing another filename grammar.

It preserves `off`, query and empty-argument behavior, `{time}`, both separators,
dot/empty components, ordinary sanitization, the three-folder limit and existing
error precedence. Refusal leaves the previous folder and screenshot queue
unchanged. A local ASCII-only predicate avoids importing the stage-name
validator's unrelated length and leading-dash restrictions.

The unchanged-production baseline reproduced the old setter accepting `CON`.
The first test fixture had stopped earlier on its own success-message oracle;
that fixture and receipt are retained separately. Corrected query assertions
read one fresh console-log info row, matching execution's existing contract.
The corrected baseline is `dbg13-baseline/802a7609d93a4f07baa55677c9b90d5e`.

Both game builds and the full console CPU case pass in
`dbg13-candidate/7796eb6475564c7abd1d73ef5b5d779d`: Debug took 89 seconds and
Release 17 seconds. The new setter-only matrix checks 397 refusals across all
22 ASCII device stems, three spellings, both separators and every component
position, plus valid neighbors, long names, sanitization, `{time}`, `off`,
empty arguments and combined traversal/depth precedence. No proposed path is
opened and no screenshot is queued by these checks. Independent review and
both source hashes pass; the Release inert/layout/RTTI boundary remains intact.

## Screenshot file basenames (`DBG#14`)

**Done (2026-10-29).** `screenshot_file_name("CON", now)` formerly returned
`CON.png`. Windows reserves that stem even with an extension, so sanitizing
punctuation alone did not produce a usable capture filename. The
[Windows naming contract](https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file)
also applies to the final screenshot basename.

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | Reserved screenshot stems survive normalization and `.png` appending. | Selected as `DBG#14`: prefix the final sanitized reserved stem with `_`, using pure string regression checks and the existing folder rule. Small capture reliability fix without a device-path experiment. |
| 2 | Input-fixture cleanup needs an explicit resource measurement contract. | Continue the separate `ED#20` investigation; its failed count candidate remains retained. |

After existing extension stripping, ASCII replacement and timestamp fallback,
the sanitizer prefixes `_` only for an exact case-insensitive ASCII device stem. Thus `CON`
becomes `_CON.png` and `lPt9.PNG` becomes `_lPt9.png`. Preserve valid names,
stem spelling, default timestamps and existing `.png` handling, including the
literal request `.png` becoming `_png.png`. Names such as `CON.` already
normalize to `CON_.png` and stay unchanged.

The two Debug callers share the existing folder predicate through a small
standard-library-only header. Its body, the folder policy and the shipping
stage-name contract are unchanged.

The pure old-source baseline returned `CON.png` and failed before the existing
PNG-writing checks, with all five source/absence guards intact. Evidence is
`dbg14-baseline/1fd3035850a44e21a6b463997b15f69e` under
`out/validation/implementation-loop-20261028/`.

Both game builds and the full console CPU case pass in
`dbg14-candidate/2c3ebacbedb84a328e0580a790888319` (Debug 88 seconds, Release
18 seconds). Debug checks 199 reserved stem/case/suffix values, 24 valid
neighbors and four sanitizer controls through `screenshot_file_name` itself.
The earlier 397 folder refusals and original PNG/queue checks remain intact;
Release confirms the inert/layout/RTTI boundary. All five source hashes and
independent review pass. New checks use pure string values, with no capture
request or reserved-path access.

## Replay file basenames (`DBG#15`)

**Done (2026-10-29).** `replay_name("CON")` formerly returned `CON`, which
recording later appended `.json` to before opening an output stream. Play uses
the same normalizer. That result still named a
[reserved Windows stem](https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file)
instead of the ordinary replay file the commands promise. The exact device
I/O outcome has not been tested and must not be probed for this fix.

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | Replay normalization admits reserved device stems before both read and write paths. | Selected as `DBG#15`: reuse the Debug stem rule and prefix `_`, with pure helper checks before replay-file work. Small naming correction; no device-path access. |
| 2 | Repeated normalization strips another `_json` from some otherwise valid stems. | Examine the compatibility and raw-suffix policy separately before changing that existing behavior. |
| 3 | Script-name validation also accepts reserved stems, but execution first requires a regular file. | A weaker early-diagnostic follow-up; keep outside this replay slice. |

After the existing ASCII replacement and one case-sensitive `_json` stripping
step, the helper now prefixes `_` for an exact reserved stem before returning
the nonempty name. Empty timestamp fallback and every other normalization rule remain.
Thus `CON`, `CON.json` and `CON_json` become `_CON`, while `CON.JSON` remains
`CON_JSON`. Do not change the separate suffix-ordering behavior in this slice.

The existing shared predicate's body is unchanged. The old-source pure-helper
baseline returned `CON` and failed at the new assertion in
`dbg15-baseline/b81be67df1924737895429290533834b`, with all four source guards
intact. The new block performs no I/O; earlier existing bot-metadata checks
already perform their own private file checks.

Both game builds and the complete console CPU case pass in
`dbg15-candidate/c3adb92a3e8049a79faa692f1ed08137` under
`out/validation/implementation-loop-20261028/` (Debug 104 seconds, Release
18 seconds). Debug checks 199 reserved stem/case/suffix values, 24 valid
neighbors and eight suffix/sanitizer controls. Existing codec, private file,
real-Level record/play and screenshot checks pass; Release's inert/layout/RTTI
boundary remains intact. Independent review and all four source hashes pass.
New tests call `replay_name` only, with no reserved-name record/play command
or device-path access. File format, recorded simulation, playback policy and
existing files remain unchanged.

## Replay catalog filenames (`DBG#16`)

**Done (2026-10-29).** Replay listings and play completion now return full
`.json` filenames. Previously, `round_json.json` was offered as `round_json`,
which play normalized again to `round`; that could select `round.json` instead.
The full filename reaches the intended recording, and `stop.json` remains
distinct from the separate `play stop` control word.

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | Catalog stems ending in `_json` are normalized a second time by play. | Selected as `DBG#16`: list and complete full `.json` filenames. A small caller-side correction that preserves existing raw command aliases and stored files. |
| 2 | Changing suffix stripping could make the normalizer itself idempotent. | Broader compatibility change: existing `_json` and punctuation aliases would resolve differently. Keep separate. |
| 3 | Arbitrarily renamed files may not fit the command's normalization grammar. | Leave catalog admission and quoting policy unchanged; qualify canonical stems produced by the existing normalizer. |

The catalog helper is used only by play completion and the `replays` listing;
neither caller constructs a replay path from its returned strings. It preserves
regular-file checks and lowercase `.json` admission, and sorts the filenames.
`replay_name`, raw play/record aliases, codec, stored files and simulation are
unchanged. Arbitrarily renamed files retain the existing admission policy.

The retained Debug baseline fails at the first full-filename completion check.
Both game builds and complete console CPU cases now pass. Six distinguishable
private tapes cover `_json`, ordinary names, adjacent and repeated suffixes,
and `stop.json`; seven actual play lookups reach their expected tape before
the deliberate missing-fingerprint refusal. Literal listing/completion names,
the builtin stop control, raw aliases, every private file, both environment
views and the original CPU match/director state are preserved. These new
lookups do not arm playback or advance a match; the existing real-Level replay
checks also pass. Release retains its inert developer-tool and executable
inspection checks. Evidence: `dbg16-baseline/d42bc77fe95f4ffabe851aabb70f47e5`
and `dbg16-candidate/e16a6d4af2ce452aa9a160792d66a571` under
`out/validation/implementation-loop-20261028/`, with all five source hashes
unchanged during qualification.

## Quoted completion boundaries (`DBG#11`)

**Done (2026-10-28).** Tab completion now recognizes statement and word
boundaries outside double quotes, matching command execution's grammar.
Previously, completing `echo "text; stag` rewrote quoted argument data as a
new command name. That unfinished argument now stays unchanged.

The scoped independent shortlist was:

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | `DebugRegistry::complete` treats semicolons inside double quotes as statement separators. | Selected as `DBG#11`: preserve quoted text and complete the actual final statement/argument. A bounded boundary scan and literal registry checks; no editor dependency. |
| 2 | Script discovery accepts only exact lowercase `.cfg`, while Windows execution can open uppercase `.CFG`. | A later discovery-only slice with private execution/list/completion controls. |
| 3 | Screenshot naming preserves Windows reserved basenames. | Scope a naming policy and pure string checks separately before any implementation; no device-path experiment. |

The boundary scan follows the existing grammar:
inside quotes, backslash followed immediately by a double quote escapes that
quote; other backslashes and single quotes are literal. An unfinished
quoted word stays unchanged with no candidates. Existing raw partial-word
matching, completed-token parsing, command/variable/live choice pools, sorting,
deduplication, common prefixes and path suffix behavior remain intact.

The native old-source baseline reproduced the exact quoted-data corruption
before the production change. Literal CPU cases cover incomplete/closed and
escaped quotes, immediate backslash/quote pairs, single quotes, adjacent
segments, real separators and whitespace. A local command observes exact
previous arguments and the partial word; unfinished quoted text never invokes
its completion callback. Sorted unique candidates, common prefixes, path
suffixes and unchanged command/game/log state are also checked.

Both game builds and the complete console acceptance case pass. Debug retains
the existing scripts, replay, tuning and bind checks; Release retains inert
hooks, shared layout and absence of developer RTTI. Independent source/test
review passes, and both source hashes remain unchanged during qualification.
This does not qualify foreground keyboard delivery or human review.

Evidence under `out/validation/implementation-loop-20261028/`: baseline
`dbg11-baseline/87c6620c4e244d4a910617c4e23254ab`, both-preset candidate
`dbg11-candidate/7ca2e3f98999441b821056aef054525f`.

## Bind reset persistence (`DBG#10`)

**Done (2026-10-28).** This ranked authoring follow-up fixed an error-handling
gap: `save_binds` ignored removal errors when the table returned to defaults.
A blocked `binds.cfg` retained overrides for the next session while the reset
reported current-session success. Ordinary failed writes already warned that
a change holds only for this session.

The defaults branch now checks the removal error and uses the same warning
policy, naming the file. Current-session defaults and command success,
successful/absent-file removal, the loading guard and the existing atomic
writer are unchanged. This also covers reaching defaults through `unbind`,
not just `binds.reset`.

An owned read handle on a private saved override reproduced the missing
warning, with reads/writes allowed and deletion withheld. The new checks cover
reset and unbind, saved bytes, active defaults, warning severity/path and a
fresh session retaining the saved override while locked. After handle release,
retry removes the file without a warning and a fresh session loads defaults;
an absent file also produces no warning. No ACL, security attribute or real
player profile is changed.

Both game builds and full console acceptance pass, including prior Debug
script/replay/tuning/bind checks and the Release inert/layout/RTTI boundary.
Independent review and four-file source-immutability checks pass. Evidence
under `out/validation/implementation-loop-20261028/`: failing private baseline
`dbg10-baseline/1e76223c74134073af356fddd48ede3a`, final both-preset batch
`dbg10-candidate/53b37c8972294ef797f71362378d6d72`.

## Roster statements for bots, teams and Boost (`DBG#17`)

**Done 2026-10-29.** A developer start can now choose everything the shipping
setup chooses per seat, so an attended bot or Boost session (`ROAD#10`,
`ROAD#5`) starts from a launch line instead of the menus:

```
seat <1-4> team <a|b>
seat <2-4> bot <easy|normal|hard>
seat <2-4> human
seat <1-4> boost <0|1|2>
```

- **Spelling.** One `seat` command, with the field as its second word, so
  nothing clashes with `bot <seat> <behaviour>`, which keeps its meaning: a
  Debug behaviour drives a seat's input, a shipping bot is the seat's
  controller. `bot all` drives only idle dummies and never a shipping bot.
- **Planning.** `MatchRequest::seats` holds the statements;
  `MatchPlanner::resolve` applies them through the existing `MatchSetup`
  setters after `players`: controllers first, then devices are claimed (the
  claims skip bots, so with every pad connected the humans still take pads in
  slot order), then teams and Boost in typed order, so a later statement wins.
  The layout follows the human count, since bots have no pane; with no shipping
  bot it is `layout_for(players)` as before. On a running match, a controller
  change re-claims devices and keeps the teams. Any seat statement starts a new
  session.
- **Refusals**, when typed and again when planned: a bot P1, a seat outside
  1-4, a team other than a or b, an unknown difficulty, a Boost outside 0-2 and
  a value after `human`. A seat outside the roster is refused only when
  planned, since `players` may come after it: `+stage crossfeed +seat 2 bot easy`
  without `+players` is a one-player plan, and that match does not start
  (`cannot start ...` is logged). The plan is also checked for a device on a bot
  seat before it is handed out; the claims cannot produce one, so the check is
  a guard, not a reachable error.
- **Replays.** A planned roster records and plays back through format 4
  unchanged. A human's difficulty is stored as Normal, as the codec requires;
  `seat <n> human` leaves the seat's old difficulty in memory, which nothing
  reads for a human.
- `status` shows `boost <n>` beside a seat with Boost.
- `review_bots_boost.cfg` ships in the [scripts](debug-console.md#scripts) table. Run unattended
  with `+script.background 1 +exec review_bots_boost +quit`, it plans the roster
  shown by `status` and ended twice at the same checksum, `d3d2fc87ef1743d5` at
  update 2276 (paint A 50016, B 42432 of 124320).

**Acceptance (CPU, `debug_console`).** `scripts/acceptance/debug_console/roster_checks.cpp`:
plans with no seat statement keep the devices, teams, layout and human seats
they had before, for 1-4 players with no pad and with every pad; a plan setting every field with every pad connected; later
statements, human hand-back and a recount on a running match; each refusal,
typed and planned; every bot mask leaves bot seats deviceless with the human
layout; and a seeded four-seat match on Close Quarters planned by the director
from the typed statements (two shipping bots, Boost on three seats, a mixed
team and a Debug dummy) records 240 updates, reads back the same roster and
replays with matching checksums. The shipped-script walk checks every `seat`
line's field and value and that its seat is in the script's `players` count,
and Release still carries none of the console's RTTI names. No pad or attended
play is needed.

**Not decided here.** This is preparation only. Whether Easy/Normal/Hard are
useful (`ROAD#10`) and whether Boost +1/+2 are fair (`ROAD#5`) stay DECISION;
no difficulty, Boost or team default changed.

## Arena review screenshot pack (`DBG#18`)

**Done (2026-10-29).** `review_arena <stage>` and its helpers take seed-7
paint-bot frames at 4, 3, 2 and 1 players (3P added beside the scoped 1/2/4, as
the Turbulence owner asks for 1-4 panes), a clean one at GO with its banner and
a heavy one after 20 match seconds at x4, each paused and preceded by
`checksum`, into `screens\review-<stage>-<time>\`. Seat 1 is a paint bot, so
the one-player pass has a paint source; its heavy frame is one bot's 20 seconds
and is light on the larger stages. Ten wrappers cover Press Tower, Close
Quarters, Crossfeed (for `POL#24`'s 1-4P readability), King of the Hill,
Turbulence and its blockout and four trials. The three stages with stations
also photograph every station on the one-player heavy frame with
`overlay.stations` on, as do the five Turbulence variant wrappers since
`POL#40` gave them stations.

The shipped-script walk now checks a `goto` station against the stage the walk
last named (the Movement Course before any), so a station name cannot drift
from its stage. Qualification on x64-debug: the `debug_console` case passes
(27 files, 1027 statements followed), and fails as expected
(`review_turbulence.cfg:8: no turbulence station crown`) with a Press Tower
station planted in a Turbulence wrapper. Every wrapper run with `+quit` exits
0 and finishes (`review_arena: done`): 26 shots for Press Tower, 16 for
Crossfeed, 13 for Close Quarters, 8 for each other stage. Two Press Tower runs
logged the same eight checksums (first `7f5d58f38d8aae9e`, heavy 4P
`144cd67c8c7fb7e4`). Keep the window in front during a run, as for every
script.

The scope as filed follows.

The shipped scripts photographed the
Movement Course (`station_tour`, `review_feedback_ab`) and Close Quarters
(`bots_4p`); nothing in `scripts/debug/` covers Press Tower or Turbulence.
Every explicit-only stage is reachable with `+stage`, since
`include_in_random` (`game/objects/stage.h:16`) only filters Random, and the
Turbulence owner still asks for native 1/2/3/4-pane clean and heavy captures
([polish-turbulence.md](polish-turbulence.md), lines 1047-1050).

**Scope.** A parameterised `review_arena.cfg` and one thin wrapper per stage:
Press Tower, the shipping Turbulence, its blockout and four trials
(`turbulence_blockout`, `_mover_trial`, `_slab_trial`, `_ferry_trial`,
`_gate_trial`), and the original King of the Hill and Close Quarters. Each takes
seeded paint-bot frames at 1, 2 and 4 players, a clean frame at GO and a heavy
one after a fixed number of match seconds, with `checksum` before each shot,
into `screens\review-<stage>-<time>\`, then puts back the bots, seed, folder
and timescale as `bots_4p` does. One player has no idle seat, so the 1P heavy
frame needs its own paint source (`bot 1 paint` on the keyboard seat). Where a
stage has stations the pack uses `goto`; Turbulence gets them only once
`POL#40` lands, and until then its frames are spawn-framed. (`POL#40` has
landed; the five variant wrappers now photograph their stations.)

**First step.** Write `review_arena.cfg` against Press Tower, which already has
stations, from the `bots_4p` pattern, then add the wrappers.

**Acceptance.** The `debug_console` case's shipped-script walk accepts every
new script, stage and station. Each wrapper run with `+quit` writes its PNGs and
logs checksums; two runs with the same seed log the same checksums. No pad is
needed; the run notes go in each arena's owner doc.

**Not decided here.** The pictures are evidence for review, not acceptance:
readability, composition and arena approval stay with the arena owners and their
DECISION rows.

## Loaded-stage replay fingerprint (`DBG#8`)

**Done (2026-10-28).** Debug recordings now carry
`stage_hash`, the loaded semantic stage definition for both shipped and user
stages. `LevelLoadedInfo` reads one owned byte snapshot and supplies those same
bytes to Labrador's checked parser and a private client-side semantic hash.
The hash is cached on the definition; recording and playback never reopen its
path. The cache has the same class layout in Debug and Release, and does not
change the stage's game meaning or the shipping instant replay.

The versioned hash retains decoded values, authored member order and array/object
order, including fields the current loader does not use. Whitespace and equivalent
JSON escapes do not matter. Equal numeric spellings such as `1`, `1.0` and `1e0`
share a representation; exact integers above 2^53 are never rounded through a
double. A floating negative zero remains distinct from positive zero, as it does
in the loaded numeric value; integer `-0` is already positive zero after parsing.
This is a conservative identity for the loaded definition, not a stage snapshot
or a portable replay format.

`replay_header` captures the registry definition `LevelBuilder` used for the
recorded match. User-stage rescans are forbidden while a match holds a definition,
so that entry cannot be replaced underneath it. Before `play` can arm playback or
request a rebuild, it requires a fingerprint, an exact current stage-catalog
entry and a loaded registry definition, then compares their hashes. Catalog
membership matters because a refused or removed user stage's old definition can
remain registered. A disk edit without a rescan still compares with the unchanged
loaded definition.

A mismatch identifies the stage and asks for the recorded definition to be
restored or a new run to be recorded. It leaves the current match, pending request
and active or pending playback unchanged. Legacy files remain readable by the
version-3/4 codec, but `play` refuses a missing fingerprint with an explicit
legacy-format instruction to record again. The tuning `force` option cannot
bypass either stage guard. Existing tuning, presentation, checkpoint and final
state checks remain in place.

This is a Debug-only input check. The same-build, same-machine, same-step and
single-round restrictions of `DBG#2` and Release's recording-code exclusion are
unchanged. No reload, user-file mutation or Random-membership change is used to
make playback pass. Attended runtime and hardware qualification are separate.

**Local qualification (Debug and Release, 2026-10-28):** the existing
`debug_console` case passes, retaining its checkpoint/final-state and Release
executable exclusion checks. New isolated shipped and user-stage recordings
each reproduce 240 steps, four checkpoints and final MATCH. Semantic controls
cover formatting/escape/equal-number normalization, source/member order,
unknown fields, exact large integers, embedded string bytes and signed zero.
Actual rescans cover formatting-only replacement, geometry changes and removal;
a disk edit without rescan keeps the loaded snapshot. Legacy/force, malformed
hash, missing catalog/service and active/pending replay controls refuse before
match, queued request, playback or warning side effects. Loader `level_format`,
`user_stages` and hidden startup with one valid/one refused isolated stage and
the accepted audio bank also pass in both presets. No hardware or attended-play
gate is closed by these checks.

## Background script work (`DBG#6`)

**Implemented and qualified, 2026-10-26.** `script.background [0|1]` is a Debug-only,
session-only opt-in, default `0`. The pinned native loop already ticks without
focus; the client coordinates its own script, match and suspension gates.
The opt-in permits automation in an ordinarily inactive, shown window. With it
enabled, hidden, minimized and power-suspended windows remain held. It never marks the real
Application active or admits physical gameplay/menu controls while inactive.

Truthful connection checks, manual/covered pauses and existing resume countdowns
remain authoritative. Ordinary opt-in focus loss avoids gameplay-buffer and
replay-suspend mutations; disabling it while inactive restores the normal hold.
Music/SFX are muted before background simulation without changing saved
preferences, current preferences restore on actual activation, and rumble output
stays zero. New inactive matches and script-driven rebuild/menu/quit transitions
obey the same policy. Normal focus behavior and Release remain unchanged.

For an unattended developer match in a shown, ordinarily unfocused window:

```powershell
ColourWars.exe +script.background 1 +stage movement_course +bot all paint +wait 300 +quit
```

The command without an argument reports the current choice. It is not saved.
Physical inputs are cleared after mouse-aim resolution, before bots/replay
supply their effective inputs; connection facts are retained. Buffered physical
edges from a zero-run foreground tick are discarded without changing clock
pacing or pending manual steps. Disabling the mode during an inactive match
holds the next script statement and queued match action until activation.
Default-off menu-side script handling keeps its earlier behavior.

Both game presets build under `/W4 /WX`. The focused `debug_console
--background-only` case passes in Debug and Release, including all-field
post-mouse neutralization, driver ordering, held edges, both disable-batch
orders, command validation, existing clock/script/bot/replay regressions,
shared layouts and Release command/RTTI exclusion. Nine relevant existing
consumer cases also pass per preset.

The Debug `RumbleOnly` supplement passes twenty cases through actual GameLevel
and StateContext, with injected window facts and synthetic activation. It
records and replays 420 updates, checks the final fingerprint and neutral
non-bot tape channels, pre-update Music/SFX gains, every motor intent and
unchanged private Save bytes. It also checks native private-HWND visibility,
minimize and message forwarding/lifetime, including synthetic power messages.
The existing Rumble checks pass in both presets; no extra screenshots are added.

Receipts are under `out/validation/overnight-20261026/phase20/`:
`background-cpu-*-v2.log`, `regression-*` and `background-native-v4/` (Debug
`30d7b44643864a248f5fb8d9d48e252b`, Release `b014cdcd1d36467a81a2817885dc293a`).
Both published-bank SystemOnlyPath startups also pass with isolated saves,
per-monitor v2 DPI awareness and graceful shutdown: Debug
`99c565c8260f455e9ea69b4f1c664180`, Release `378b21cc7e2046398367b2f154a65ce2`.
Failed native receipts are retained: v1 had a diagnostic-string compile error;
v2 exposed post-neutralization mouse aim and prompted the production fix;
v3 drove the reconnect menu with the wrong pad, corrected to its actual owner.
These controlled results do not establish attended OS-focus/power-sleep,
physical-pad, listening or performance acceptance, and do not satisfy `CI#1`.

The refreshed local Release ZIP at `9d08adc5` also passes file/audio/console
inspection, all nine level-byte checks, normal extracted startup and a separate
missing-bank-copy startup. Receipts are under
`out/validation/overnight-20261026/phase21/`; the normal startup is
`3953dcbf708644dd974ce81a8f51a88b`, and the fallback is
`a5291edaec4d4ca783c37027ed030486`. Its 63-file ZIP SHA256 is
`191451EDD2A1AD69D883E0D997FBA98255082308C17FC666CA27929829E993EB`.
Clean-machine acceptance remains `ROAD#1`.

## Remaining live-tuning work (`DBG#1`)

**DEFERRED (readiness audit, 2026-10-26).** No additional field/caller is selected;
resume when an implementation task needs one. General string/font edits and
missing-key insertion must first get their own bounded scope. The existing
fifty-eight colour paths listed under [Live tuning](debug-console.md#live-tuning)
support read, validated apply, diff, reset and save: the initial seven, both
readiness colours on Team Select and Weapon Select, the weapon-feedback pair,
the opaque player-identity ink/background pair, the combat-feedback pair and
the blocked-shot highlight/keyline pair, the common idle-drip keyline,
respawn pen colour, cursor ink/fill/shade/shadow, silhouette body/ink/rim/shade and reveal
ink/paper/shade, plus desktop, monitor power-light, program, acting face/sweat, glass, scribble, crack, HUD health/track and shared label foreground/identity shadow, captured Stage Select preview backing, normal timer foreground/shadow, opaque closing text and panel ink/paper/accent, positive-alpha closing shade, shared HUD paper ink/paper/shade, and figure arm/hand and quiet blink face colours.
General string/font edits and insertion of absent keys remain separate scoped
implementations. No additional colour field is selected after this family.

A future selected colour increment must follow an actual field's consumer,
add that field explicitly and report when it is adopted: live, new objects,
rebuild or next screen. Numeric path wildcards do not establish a colour's
adoption scope. Keep the supported list tied to those audits rather than
exposing arbitrary strings by their spelling. General string/font edits and
missing-key insertion need their own scoped implementations. Further consumer
audits are optional if selected: choose an actual field and caller before
implementation. An inventory is not a mandatory backlog or a target count.

Every extension must validate the complete candidate through `Presentation`
and its existing resource checks before applying it; a rejected edit changes
neither live content nor the diff.

Acceptance: valid named/hex colours apply and round-trip through diff/reset/save;
invalid values leave state intact. Preserve surrounding source bytes, line
endings and unrelated external edits, existing conflict/force rules and atomic
replacement behavior. Check captured versus reopened/rebuilt consumers and replay
edit restrictions. Keep numeric/boolean behavior and Release developer-tool
exclusion intact. These checks use scratch content and isolated profiles.

The local RapidJSON span reader and in-memory candidate validation
(`labrador::parse_json`, engine report 12) already provide the required route.
The engine declined to keep source spans, so the span reader and splice stay
this tool's.

HUD painter fixtures re-pinned to the OFL fonts (2026-10-28): the HUD timer,
closing materials/shade/text, HUD paper and HUD label probes pinned Gill Sans
MT Bold metrics, and 95c1da3b renamed their fonts without updating them. They
now pin Lato Black (`lato_black_48` line spacing 77, `_36` 58, `_24` 38) and
the geometry it moves: clock text at (891.5,54), timer glyph ROI
[901,68,1032,120) (131x52), clock envelope [858,43,1067,146) (209x103),
identity/HEALTH/PAINT label ROIs 380x111/125x28/97x28, paper crops
487x142/327x181/209x103 and their patch sites, and timer witnesses (911,79)
and (970,117). Checks, controls and tolerances are unchanged; only the pinned
literals and the counts derived from them moved. The dated figures below were
measured against the Gill atlases.

Shared HUD paper material family (2026-10-24), **verified**:
paths 56-58 select `presentation.hud.paper.ink`, `.paper` and `.shade`
as live borrowed colours. Only three exact allowlist rows change production.
The existing reader requires distinct opaque ink/paper and positive-alpha
shade; no additional contrast or shade-equality restriction is introduced.
Authored `141414` / `FFF5DF` / `00000055`, drawing, full-RGB shadows,
alpha-scaled inset bands, layout and resources remain unchanged. The actual
callers are the identity panel, the combined HEALTH/PAINT panel and the
ordinary-timer panel; ink also reaches the two direct meter borders. The
separate closing-clock region and root identity badges keep their own materials.

Debug and Release game/editor builds and all four focused `debug_console`
and `story_audio` CPU cases passed on their first actual attempts.
The qualified private core is v2: its named-colour expected-value oracle and
stale count comment were corrected during source review before any runtime.
No failed build or CPU run preceded this qualification. Evidence is under
`out/validation/hud-paper-colours-20261024/` in `core-result-v1.json`.

Independent 58-path fixtures preserve the earlier contracts. Isolated ink/paper
LF/CRLF cases use valid distinct tuples, exact one-token and two-token saves,
external-byte adoption, conflict/force, full-reader partial/zero/equality
refusals, and rebase/reset. A selected reset which would equal its pending
sibling refuses with both edits intact. Separately valid disk/current pairs
which would become equal on Save or force Save also refuse, including an
unedited counterpart and external selected value after selected reset.
Shade retains AA01/partial/FF success and zero refusal after selected reset
while sibling/overlay ledgers remain. Retained HUD metadata covers all three
new subjects, complete 58-value guards, borrowed records, geometry and captured
sheet/frame/font measurements. Direct loaded-resource/missing-frame checks
remain separate from no-Application full-reader commands. Record/replay
restrictions and Release developer exclusion remain covered. CPU results
establish neither rendered pixels nor human visual acceptance.

Native Debug and Release passes succeeded on their first actual attempts,
reusing the qualified core producer binaries without repeating game builds.
Release records 64 readbacks / 29 rows / 18 crops; Debug totals 100 / 43 / 18.
The shared consumer checks 35 material sites (560 sampled pixels) across the
three paper panels and two extra ink borders. Twelve counterexample controls
produce 202 site discriminations and 3,232 differing pixels per preset.
Independent underlays, repeat/copied adoption, normal/reduced and
settled/reduced comparisons, ordered bands and detached-tick counterexamples
pass. Closing-branch checks isolate the separate closing-clock region while
positive full-HUD changes confirm that identity/meters still adopt shared
paper colours; no whole-HUD closing invariance is claimed.

Debug adds 36 readbacks and 14 rows: nine accepted rendered states and five
refusal rows. Its twenty statements include fifteen accepted commands
(three queries, seven tunes, four resets and one Save) and five refusals.
The genuine three-token Save/rebase and original/saved resets preserve
independent sibling edits. Equal ink, equal paper, partial ink, partial paper
and zero shade refuse atomically in typed state, each followed by one
held-valid-frame read. Refusal reads do not count as accepted states, new raw
references or copied adoption; zero is not an accepted shade palette.
The Debug state checks include 5,040 patch checks. Observation update/input
poll and game Save counters remain zero.

The eighteen ordinary-HUD crops show six palettes across identity, meters and
timer panels: 898,584 pixels and 2,696,022 P6 bytes per preset. A bounded montage
of those panels was reviewed for artifact QA; the closing region has no
exported crop or human acceptance claim. Reconciliation passes all 1,498
frozen source pins, eleven prior fixtures, qualified producer/result hashes
and the preserved 4795ca4 ZIP. Twenty-two shared raw files and eighteen PNG
pairs are byte-identical between presets. Independent PNG CRC/filter decoding
also confirms full RGB parity with each PPM payload.

Evidence is under `out/validation/hud-paper-colours-20261024/` in
`native-result-v1.json` and `validation-reconciliation-v1.json`.
There were no native runtime failures or runtime source corrections.

The preserved 4795ca4 ZIP predates these changes. Human contrast/readability,
comfort, C2 recognition, arena/weapon balance, physical controls, listening,
clean-PC and performance acceptance remain open. `DBG#1` remains open.
No further unattended field is selected. A new automated slice requires a
concrete observation or bounded owner requirement; further colour audits are
optional if selected. This does not close the remaining TODOs. The dated
checkpoints below preserve their original scope and next-step statements.

Positive-alpha closing-clock shade increment (2026-10-24), **verified**:
path 55, `presentation.hud.closing_clock.shade`, has an exact live row.
That allowlist row is the only production change. Its existing positive-alpha
reader accepts names/RGB and RGBA alpha01 through FF, refuses zero, and imposes
no opaque-only, contrast or equality rule. Authored `00000055`, both base
shadows, six RGB-preserving alpha-scaled bands, geometry, resources and
rendering remain unchanged.

Debug and Release game/editor builds and all four `debug_console` and
`story_audio` CPU cases passed on their first attempts. Independent 55-path
arrays and a separate positive-alpha classifier retain the 54 earlier
contracts. Nine retained-HUD metadata subjects guard borrowed records, colours,
geometry, fonts/resources and RNG without drawing or advancing a match.

Actual commands accept named/RGB/partial shade, alpha01/FE/FF boundaries and a
shade equal to ink. Black and nonblack zero refusals preserve complete live,
document, ledger and file state. Exact escaped spans, one-token and shade/paper
pair saves, selected/sibling resets, LF/CRLF conflict/force and rebase remain
covered. External selected zero after reset cannot be adopted while other
ledger entries are saved. Valid-positive recording/replay write attempts
prove policy refusal without relying on an invalid zero input. Separate direct
complete metadata resource validation and missing-frame refusal remain distinct
from no-Application full-reader commands. Release developer exclusion passes.

Both native presets passed on their first actual attempts, without a runtime
failure or runtime source correction. Release uses **68 readbacks, 30 rows and
12 native 202 x 100 crops**; combined Debug totals are **93/39/12**. Each preset
has 242,400 crop pixels and 727,380 P6 bytes. Complete decoded PNG RGB matches
the PPM payloads, with opaque alpha, for all twelve crops.

Eight material sites cover 128 pixels: both base shadows and one/two/three-band
depths in each direction. Two controlled underlays and four valid palettes,
including minimum-positive alpha01, retain full author RGB and repeated
source-over bands. Twelve independent patch observations compare 1,536 RGBA
pixels. Eight wrong-reference controls produce all 48 site discriminations
and 768 pixel inequalities per preset. The low positive input is valid without
a universal visible-contrast guarantee.

Twenty-two full-frame repeats and eight copied HUD comparisons establish
retained drawing/adoption separately from those material references. Five
normal-branch invariances, paired normal/reduced and settled/reduced relations,
and two beat counterexamples retain applicability. Band pixels match across
Canvases over opaque paper; exposed base-shadow relations retain the controlled
underlay distinction. The proof covers these material sites, not complete
glyph/panel reconstruction or natural Level sequencing.

Debug executes ten statements with eight accepted rendered states, one private
one-token Save/rebase and original/saved resets. Its single zero-alpha refusal
preserves typed live/document/ledger/file/resource state, followed by one draw
equal to the held valid partial-alpha frame. Zero is not an accepted visual
palette. These checks add twenty-five reads and nine rows, including
1,024 cached-reference patch checks; no extra crops or raw-reference reads
are needed.

Calibration totals are 2,176 pixel checks in Release and 2,976 in Debug.
Every observation update, input poll and game Save counter remains zero.
Evidence is under `out/validation/closing-shade-colour-20261024/`, with
`validation-reconciliation-v1.json`. All 1,496 frozen source pins, nine prior
fixtures, qualified producer/result hashes and the preserved 4795ca4 ZIP
reconcile. Existing evidence remains intact; qualified core producers were
reused for the native probes without repeating game builds.

The preserved ZIP predates these changes. Human contrast/readability, comfort,
C2 recognition, arena/weapon balance, physical controls, listening, clean-PC and
performance acceptance remain open. `DBG#1` remains open. The separate shared
HUD paper audit maps distinct opaque ink/paper, positive-alpha shade, three
helper callers and two extra meter-border ink uses. It selects no implementation
or mandatory backlog; reassess the actual report before choosing further work.

Opaque closing-clock material family (2026-10-24), **verified**:
paths 52-54 expose the existing `presentation.hud.closing_clock.ink`,
`.paper` and `.accent` as live borrowed colours. Production changes only
three exact allowlist rows. The opaque-only reader, equal-colour validity,
defaults, drawing, beat geometry, shade, layout and resources remain unchanged.
This does not expose the whole closing-clock palette.

Debug and Release game/editor builds and all four `debug_console` and
`story_audio` CPU cases passed on their first attempts. Independent 54-path
fixtures retain earlier RGBA and next-screen contracts. Span checks preserve
nested escaped tokens and exact source bytes; actual full-reader commands
enforce opaque named/RGB/RGBA-FF acceptance and atomic partial/zero refusal.
Equal opaque sibling values are explicitly valid.

Eight retained-HUD metadata subjects preserve borrowed records, fonts, resources,
geometry, other colours and RNG without drawing or advancing a match. The three
material subjects exercise cyclic opaque sibling pairs, saved and unsaved
selected resets, exact two-token saves/rebase, LF/CRLF conflict/force behavior
and replay restrictions. Invalid merged controls target an unedited opaque
neighbour, then invalid external selected alpha after selected reset, preserving
the remaining ledgers and files. Separate complete metadata resource validation
and missing-frame refusal are distinct from the no-Application command checks.
Release developer-tool exclusion passes.

Both selected native presets passed on their first attempts at hidden
1920x1080. Release uses **64 readbacks, 27 rows and 9 native 202 x 100 crops**:
181,800 crop pixels and 545,535 P6 bytes. Combined Debug totals are
**104/37/13**, 262,600 crop pixels and 787,995 P6 bytes. All nine/thirteen PNGs
have complete decoded RGB parity with their PPM payloads and opaque alpha.

Twelve flat patches cover two ink body sites, four ink tick under-strokes,
two paper body sites and four accent stripes. Eleven independent raw
observations compare 2,904 RGBA patch pixels. Six wrong-opaque or omitted
references produce all 24 per-site discriminations and 528 pixel inequalities.
Twenty-one full-frame repeats and five copied HUD comparisons establish
retained drawing/adoption separately. Five normal-branch invariances, two
isolated inactive-accent invariances, three settled/reduced equalities and one
active/settled beat counterexample retain applicability. Opaque straight/double
alpha cannot discriminate and is not claimed.

Debug executes seventeen statements: fourteen accepted operations, including
three queries and eleven mutations/Save operations, plus three atomic
partial-alpha refusals. Ten rendered states add forty reads, ten rows, four
crops and 2,640 patch checks, with ten repeat/copied/sibling checks. One exact
two-token ink/paper Save/rebase and selected resets retain saved siblings or
pending accent; equal opaque paper/accent is valid. The three structured JSON
refusal records say `raster_checked=false` and add no reads or CSV rows.
They establish atomic reader rejection, not refused-input pixels.

Calibration checks total 2,048 pixels in Release and 3,328 in Debug. Every
observation update, input poll and game Save counter remains zero. Independent
proof covers the twelve material patches; envelope crops include existing
glyph/shade art, while copied HUD frames establish adoption rather than an
independent reconstruction of that art.

Evidence is under `out/validation/closing-material-colours-20261024/`,
with `validation-reconciliation-v1.json`. All 1,494 source pins, fifteen prior
fixture pins, qualified producer/result hashes and the preserved ZIP reconcile.
The preview51 and timer46-48 bodies remain unchanged. Thirteen shared raw files
and nine PNG pairs match byte for byte between presets. Native probes reuse the
qualified core producers without repeating game builds.

The preserved 4795ca4 Release ZIP predates these changes. Human urgency,
contrast/readability, comfort, C2 recognition, arena/weapon balance, physical
controls, listening, clean-PC and performance acceptance remain open.
`DBG#1` stays open. Closing shade remains unsupported; reassess its bounded
source report before selecting any implementation. Its bands copy shade RGB
and scale only alpha, with zero alpha rejected by the existing reader.
No shade rendering correction or optional colour inventory is selected here.

Captured Stage Select preview backing increment (2026-10-24), **verified**:
path 51, `presentation.hud.player_identity.background_colour`, has an exact
`next_screen` classification. Stage Select captures one premultiplied tint at
preview-background construction. Authored `121827EE` is unchanged; correcting
its nonblack translucent RGB submission is an explicit behavior change. This
field does not colour gameplay HUD paper or tint the map. Other values, art,
layout, fonts, labels and stage data remain fixed.

Both game/editor builds and all four Debug/Release `debug_console` and
`story_audio` CPU cases pass on their first attempts, including Release
developer-code exclusion. They cover actual reader/commands, complete candidate
and resource validation, atomic refusal, exact save/reset/rebase/conflict/force
and replay restrictions. Derived UiTexture metadata checks the capture model;
the actual page evidence comes from the separate native leaf.

Both selected native presets pass on their first attempts: Release has
68 readbacks, 26 rows and 16 native 64 x 32 crops; Debug has 92/38/16. The crops
cover 32,768 pixels and 98,512 P6 bytes per preset, with complete decoded PNG RGB
and opaque-alpha checks. Four palettes cover Random/Crossfeed browsing and
ready states. Eight wrong-straight/double controls produce 16,384 aggregate
pixel inequalities. Explicit opaque-edge underlays are `445268` when browsing
and `75EDD7` when ready; comparisons match those states. Retained pages keep
their captured tint through updates, cycling and ready changes; actual
Back-to-Weapon-to-Proceed reconstruction adopts the new tint. The Weapon
transition row is a navigation observation without a raw colour oracle.

Debug adds seven actual query/edit/reset/Save commands and 12 paired rendered
observations, preserving retained-page capture and exact private one-token
Save/rebase. These navigation fixtures use 63 menu updates and 249 device polls
in Release, or 97/363 in combined Debug. Positive-time, Level/player/Scene and
gameplay-weapon updates and game Saves remain zero.

Targeted StagePaper and MenuText dependent checks pass in both presets. Two
legacy preview-background references now independently premultiply their tint.
An old malformed StagePaper case reached the stronger reader before its intended
page rejection; both boundaries are now checked separately. That failure and a
private wrapper's optional-metadata error after a native pass are preserved.
Three completed dependent probes were reused by exact hashes. Only Release
MenuText was retried after a deep-path capture-write failure; identical sources
pass under a shorter output path. The original writer exposed no OS error.
No production reader, geometry, values or equality tolerance was relaxed.

Evidence is under `out/validation/stage-preview-background-colour-20261024/`,
with final `validation-reconciliation-v4.json`. Nineteen shared raw files and
16 PNG pairs match byte for byte across presets; 1,492 source pins, qualified
producer hashes and the preserved 4795ca4 Release ZIP are reconciled. That ZIP
predates these increments. Automated checks do not establish human preview
contrast, C2 recognition/readability, comfort, arena balance, physical controls,
listening, clean-PC or performance acceptance. `DBG#1` remains open. The next
selected bounded group is opaque closing-clock ink/paper/accent; its preparation
remains private.

HUD label foreground and identity shadow increment (2026-10-24), **verified**:
paths 49/50, `presentation.hud.player_identity.colour` and
`presentation.hud.player_identity.shadow_colour`, are live on retained
InterfaceGameplay draws. Foreground supplies the identity label and both
HEALTH/PAINT labels; shadow is identity-only. Three existing C++ tint arguments
premultiply base RGB once, preserving authored opaque foreground `141414` and
BLACK-alpha34 shadow `00000022`. Root badge colours, HUD background, values,
text, captured fonts, geometry, clocks and resources stay outside this pair.

Both game/editor builds and all four Debug/Release `debug_console` and
`story_audio` CPU cases pass on their first attempts, including Release
developer-code exclusion. Independent 50-path fixtures preserve the badge and
closing-text opacity contracts and unrestricted RGBA on this pair. Named/RGB/
partial/zero input, exact escaped/nested spans, selected reset, pending sibling
preservation, paired two-token save/rebase, conflict/force/full-candidate
refusal and recording/replay restrictions are covered. Retained CPU HUD and
resource checks are metadata evidence, not rendered adoption.

`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -HudLabelTuneColoursOnly`
passes at hidden 1920x1080 in one supplied prepared HUD context, with normal
clock at 123.25 and fixed label/caller inputs. Shared Release evidence is 39
readbacks, eight rows and sixteen native crops; Debug totals are 99/23/16.
The three independent regions are identity 380x105, HEALTH 141x26 and PAINT 108x26.
Eight observations compare 46374 RGBA pixels each (370992 comparisons). Same-font
BC2 references use literal ordered materials and explicit base channels:
identity includes its two-row shade strip, then shadow and foreground; meter
labels use unshaded paper without shadows. Copied complete-HUD parity proves
adoption, not independent full-HUD art or engine font-rasterizer correctness.

Seven negative frames provide 13 separately required region discriminations
and 66983 observed aggregate pixel inequalities. Foreground controls must change
all three regions; shadow controls change identity and preserve both meter
regions. A reversed-order control uses source-derived overlap (139,91).
Repeated draws, unselected regions, captured resources and draw RNG remain
guarded. The sixteen crops total 345096 pixels and 1035520 P6 bytes: eight
identity crops plus four of each meter label. PNG decode verifies native
sizes, opaque alpha and complete RGB parity with each retained PPM.

Debug executes 15 real statements and renders 15 states (baseline plus 14
non-Save command states), adding 60 readbacks and fifteen rows. Actual commands
cover pending sibling adoption, one two-token private Save/rebase, original
resets and saved-sibling-preserving resets with exact joint saved pixels.
CPU checks own pending-sibling reset/conflict/force variants. There are no game
Save calls, observation updates or input polls; calibration comparisons total
3168 in Debug and 1248 in Release. Eighteen shared raw files and all sixteen
PNG pairs are byte-identical between presets. Native probes were compiled
fresh while qualified game/editor builds and CPU results were retained with
unchanged producer hashes.

The first native-v1 attempt failed at an exact expected-origin guard before
ROI readbacks. A derived summary had copied an adjacent wrong float even
though its detailed source audit already recorded the correct origin. Source
arithmetic and independent review corrected that one expected literal and its
comment, with line-ending normalization. Production formulas, fonts, ROIs,
materials, equalities, controls and counters did not change; fresh native-v2
runs passed. The failed receipt, original sources and summary erratum remain.
This establishes no compiler or renderer defect. Earlier invalid case-folded
letter analysis was separately retracted before this fixture was prepared;
it did not invalidate the prior numeric clock evidence.

Evidence is under `out/validation/hud-label-colours-20261024/`.
The preserved 4795ca4 Release ZIP predates the clock and HUD-label increments.
Hidden checks establish no natural Level sequence, arbitrary label/layout/
scale, small-pane readability, C2 identity/rear-threat recognition or comfort,
physical controls, listening, clean-PC or performance acceptance.
`DBG#1` remains open. The selected next consumer is the captured Stage Select
preview backing, with next-screen adoption and an explicit default-RGB
correction; that separate field is not implemented by this result.

Opaque closing-clock text increment (2026-10-24), **verified**:
path 48, `presentation.hud.closing_clock.text`, is live on applicable closing-
clock text draws. The sole production change is its exact colour
allowlist row. The existing reader's alpha1 constraint, draw/fade behavior,
normal timer pair, other closing materials, resources and content stay fixed.

Both game/editor builds and all four Debug/Release `debug_console` and
`story_audio` CPU cases pass on first attempts, including Release developer-code
exclusion. Independent 48-path fixtures preserve unrestricted RGBA on the
normal pair and scope closing text to opaque names/RGB/RGBA-FF. Actual command
partial/zero refusals retain live values, documents, ledgers and files. Exact
save/rebase/reset/conflict/force and replay checks remain; full-reader saves
also refuse invalid opaque-neighbour merges and invalid external selected text
after selected reset, preserving remaining sibling/unrelated edits.

The existing `HudTimerTuneColourOnly` leaf now qualifies all three clock
subjects at hidden 1920x1080. Release family totals are 161 readbacks, 54 rows
and twelve native 122x51 crops; Debug totals are 261/85/12. Closing-only shared
evidence is 50/18/4 and its combined Debug subtotal is 82/29/4. The twelve PPMs
total 224160 bytes. Both native presets passed on their first actual attempts.

Closing observations start with a running clock at 10 seconds and cover eight
clock/Reduce Effects contexts. Correct references use the audited opaque closing
paper and same-font opaque glyphs, with no normal timer shadow. Eight applicable
observations compare all 6222 RGBA ROI pixels. Wrong opaque tint and an added
normal-shadow control produce 3267 pixel inequalities; source witness (980,79)
must remain closing paper in the correct output. Straight/double-alpha
discriminators are inapplicable at alpha1 and are not claimed.

Closing Debug executes nine real statements: seven accepted and two partial/
zero refusals. Seven rendered accepted states, two refusal pixel checks and
two normal-branch observations add 32 reads and eleven rows. Refusals preserve
live data, both documents' baseline/current/revision/ledger, private source/
staging and resources. One-token save/rebase and exact original/saved resets,
RNG, normal clocks and the other closing materials stay guarded. No game Save,
observation update or input poll is performed.

Twenty-one shared CSV/JSON/PPM files and twelve PNG pairs are byte-identical
between presets; complete decoded PNG RGB matches the retained PPM payloads.
All fourteen prior normal-clock shared raw files, six Debug command files and
their PNGs retain the qualified path47 bytes. Native probes were compiled
fresh while qualified game/editor builds and CPU results were retained with
unchanged producer hashes. Copied whole-HUD comparisons establish adoption;
the same-font reference does not independently test engine font rasterization.

Evidence is under `out/validation/closing-clock-text-colour-20261024/`.
The preserved 4795ca4 Release ZIP predates these clock increments. Hidden HUD
checks establish no natural Level sequence, human clock/C2/arena readability
or comfort, physical-pad, listening, clean-PC or performance acceptance.
`DBG#1` remains open with further actual consumers selected by audit.

Normal HUD timer shadow increment (2026-10-24), **verified**:
path 47, `presentation.hud.timer.shadow_colour`, is live on the same retained
normal-clock consumer. One base premultiplication preserves authored BLACK
alpha34 and the independently supported foreground. The running closing branch,
font/resource capture, offsets, formatting, geometry and content stay fixed.

Both game/editor builds and all four Debug/Release `debug_console` and
`story_audio` CPU cases pass on their first attempts, including Release
developer-code exclusion. Independent 47-path fixtures and shared foreground/
shadow subjects cover named/RGB/RGBA parsing, escaped nested tokens, save/rebase,
complete candidate rejection and record/replay restrictions. Selected resets
preserve the sibling colour and unrelated pending edits; invalid merged saves
retain all three ledger entries and source/staging/current/baseline bytes.

The existing `HudTimerTuneColourOnly` leaf now qualifies both normal-clock
subjects through shared mechanisms at hidden 1920x1080. Release family totals
are 111 readbacks, 36 rows and eight native 122x51 crops; Debug totals are
179/56/8. Foreground retains its 55/18/4 shared and 89/28/4 Debug subtotals;
shadow contributes 56/18/4 shared and 90/28/4 Debug. The eight PPMs total
149440 bytes. These family runs passed on their first actual attempts.

The shadow reference composes explicit-channel shadow before the unchanged
foreground, using the same BC2 font and one effective opaque-paper backdrop.
Twelve shadow observations compare all 6222 RGBA ROI pixels. Four negative
tint/order controls produce 4552 pixel inequalities, including a reversed-order
check at source-verified glyph overlap (919,79). Defaults, nonblack partial/zero
alpha, repeated draws, unselected regions and normal/closing/Reduce Effects
boundaries remain exact. Copied whole-HUD comparisons establish adoption only.

Each Debug subject executes eight actual commands/states, with one-token private
save/rebase and exact authored/saved resets; both keep resources, input, RNG and
default preferences intact. No game Save, observation update or input poll is
performed. Fourteen shared CSV/JSON/PPM files and eight PNG pairs are byte-equal
between presets, with complete decoded PNG RGB matching each PPM. The seven
foreground shared raw files, three Debug command files and foreground PNGs
also retain their bytes from the qualified path46 native-v2 evidence.

Native qualification retained the qualified game/editor builds and CPU results,
with unchanged producer hashes, and compiled fresh native probes. Source review corrected the private wrapper's expected family-scope
label before launch; it required no producer, oracle or tolerance change.

Evidence is under `out/validation/hud-timer-shadow-colour-20261024/`.
The preserved 4795ca4 Release ZIP predates both normal-clock increments.
Automated HUD checks establish no natural Level sequence, human clock/arena
readability or comfort, physical-pad, listening, clean-PC or performance
acceptance. `DBG#1` remains open with further consumers selected by audit.

Normal HUD timer foreground increment (2026-10-24), **verified**:
path 46, `presentation.hud.timer.colour`, is live on existing InterfaceGameplay
normal-clock draws. Only its exact colour path and existing foreground tint
argument change. The shadow, closing-clock branch, font/resources, caller
inputs, formatting, geometry and authored content remain fixed.

Both game/editor builds and all four Debug/Release `debug_console` and
`story_audio` CPU cases pass, including Release developer-code exclusion.
The independent 46-path sweep, escaped nested tokens, exact LF/CRLF source
splices, one-token save/rebase, sibling-preserving selected resets and complete
invalid-merge ledger retention are covered. Recording/replay reject new edits;
retained CPU HUD/resource evidence is metadata only, with no pixel claim.

`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -HudTimerTuneColourOnly`
passes at hidden 1920x1080. Shared consumer evidence is 55 readbacks, 18 rows
and four native 122x51 crops (74720 PPM bytes). An independent same-font,
linear-filter reference uses literal placement/channels on one effective
opaque paper backdrop, retaining the separate shadow. Twelve observations
check all 6222 RGBA ROI pixels. Three straight/double-alpha negative controls
discriminate 7968 pixels. Copied full-HUD comparisons prove adoption only;
the reference does not independently test the engine's font rasterizer.

Eight source-derived contexts cover ordinary time, just above ten, exactly ten,
fractional closing time, zero, stopped short time and Reduce Effects on normal
and closing draws. Foreground edits retain the closing-clock signal exactly.
Debug adds eight actual private commands/states, 34 readbacks and ten rows,
including one-token save/rebase and exact original/saved resets; combined Debug
is 89/28/4. Fonts/resources, unselected regions, repeated draws and RNG stay
guarded, with zero observation updates, input polls or game Save calls. Seven
shared raw CSV/JSON/PPM files and all four PNGs are byte-identical between
presets; decoded PNG pixels match their complete PPM RGB payloads.

The first Debug native launch stopped at a missing probe argument registration,
before Application/window/renderer construction. Its failed receipt is retained.
Only the accepted-mode predicate and usage string were corrected and reviewed;
the unchanged oracle and exact tolerances passed in fresh native-v2 runs.
Qualified game/editor and CPU producers were reused by hashes.

Evidence is under `out/validation/hud-timer-colour-20261024/`. The preserved
clean 4795ca4 Release ZIP predates this increment. Automated checks establish
neither human clock readability/comfort nor attended Crossfeed, physical-pad,
listening, clean-PC or performance acceptance. `DBG#1` remains open.

Quiet blink face colour increment (2026-10-24), **verified**:
path 45, `presentation.figure.quiet.face_colour`, reads live through existing
FigureAttachments. The face fill is premultiplied once; the independently tinted
arm-colour lid stays later in the draw order. Authored opaque WHITE, quiet timing,
geometry, captured resources, input and simulation remain unchanged.

Both game/editor builds and the Debug/Release `debug_console` and `story_audio`
CPU cases pass, including Release developer-code exclusion. The 45-path sweep,
escaped nested spans, exact two-token save/rebase and sibling-preserving resets
are covered. An invalid merged eyelid/eye-height relationship is refused by
ordinary and forced saves without changing source, staging, live state or the
edit ledger; recording/replay restrictions remain enforced.

The existing Limb native family retains all 43/44 checks and adds a separately
counted direct quiet-face consumer. Shared face evidence is 38 readbacks,
10 rows and ten 176x24 crops: four literal mirrored eyes, 128 RGBA pixels per
case split into 64 face and 64 lid pixels, five face/arm palettes, two underlays,
and eight negative references with 512 role-pixel inequalities. Null/disabled
records, repeated draws, opaque-default parity, exterior pixels, resources and RNG stay
guarded. Debug adds nine actual commands/states, 54 readbacks and 18 rows for
the private face/arm Tune save and selected resets, with no game Save writes.

Combined readback/row/crop totals are Debug **150/41/14** and Release
**60/14/14**, including the unchanged legacy subtotals **58/13/4** and
**22/4/4**. Calibration checks are 4,800 and 1,920; all fourteen shared PPMs are
byte-identical between builds, and every PNG's decoded RGB matches its source.
The old preparation-only Weapon calls remain separate: 20 Debug and 10 Release
zero-dt calls, with no additional quiet-face preparation or observation updates.
Both native first attempts pass. Exact source/build/probe/content/image pins
are reconciled under `out/validation/quiet-face-colour-20261024/` in
`validation-reconciliation-v1.json`; native run IDs are
`bca67f7d000f4813a969ee1f6e799521` and `a3ace014888245378457d9d29d25af14`.
This proves the retained direct consumer, not natural Player blink timing,
full body artwork, 1-4P visibility, human comfort, hardware or performance.
`DBG#1` remains open for another bounded actual-consumer audit.

Figure arm/hand colour family (2026-10-21), **verified**:
paths 43/44, `presentation.figure.arm_colour` and `presentation.figure.hand_colour`,
read live with base RGBA premultiplied once; defaults, geometry, quiet face and
captured rest/sleeve layers remain unchanged.

Both game/editor builds and four [CPU cases](../out/validation/crossfeed-20261020/limb-colour-v2/build-consumer-result.json)
pass, reusing the unchanged Debug production build after hash checks.
The earlier [CPU freshness fixture](../out/validation/crossfeed-20261020/limb-colour-prep/first-CPU-failure-v1.json) and
[native WHITE-eyelid inequality](../out/validation/crossfeed-20261020/limb-colour-prep/first-native-failure-v2.json)
failures are preserved; production needed no further correction.

Corrected [native runs](../out/validation/crossfeed-20261020/limb-colour-v2/native-v2/runner-result.json)
pass: `-LimbTuneColoursOnly` gives Release 22 readbacks/4 rows, Debug 58/13,
four 1280x480 crops each. Twelve procedural regions independently compare
21142 native RGBA pixels/case; six controls retain 248 discriminating inequalities
and four exact-WHITE guards. Copied full-fixture parity proves adoption only;
rest/sleeves are invariance guards.

Ten zero-delta Weapon preparations in Release, twenty combined in Debug,
precede zero observation/game updates or input polls. Debug passes nine
commands/states, two-token Save/rebase/reset and two private PlaySettings/Jump J
seed Saves/default display.

[Cross-preset reconciliation](../out/validation/crossfeed-20261020/limb-colour-v2/native-v2/reconciliation.json)
confirms 12 shared files/four PPMs byte-identical. Four native RGB PNGs pass
13-field metadata, complete decoded RGB parity and opaque decoded-alpha checks;
[root inspection](../out/validation/crossfeed-20261020/limb-colour-prep/actual-native-evidence-check-v1.json)
covers all four palettes. Actual-Player/Level, natural blink timing,
integrated body artwork and human/hardware, contrast, comfort, listening,
balance and performance gates remain open.

Monitor crack colour increment (2026-10-21), **verified**:
path 42, `presentation.opening.crack`, reads live; one premultiplication preserves
opaque `3A3A3A`, broad-ink/thin-dab order, captured paths and resources.

Both game/editor builds and four focused [CPU cases](../out/validation/crossfeed-20261020/crack-colour-v1/build-consumer-result.json)
pass. Ordinary/forced merged saves reject the invalid document
(corner_step 0.002 below pen_width 0.004), preserving exact state and both
ledger keys/token pairs; selected reset retains unrelated edits.

`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -CrackTuneColourOnly`
passes first execution in [both presets](../out/validation/crossfeed-20261020/crack-colour-v1/native/runner-result.json):
Release 42 readbacks/8 rows, Debug 78/17; eight native contiguous 115x34 crops,
31280 pixels/93952 P6 bytes, left/right paths across four palettes.

Independent proof uses direct front-details, Canvas and calibration only.
All 3910 actual/raw RGBA pixels/case precede whole-ROI source checks; four
models are byte-equivalent, without GPU precision inference. Ten controls
check 1274 changed/37826 unchanged comparisons; alpha0 retains BLACK ink.
Calibration checks 32 WHITE/BLACK RGBA pixels/read without extra reads.
Copied complete front-details parity proves adoption only.

[Parity](../out/validation/crossfeed-20261020/crack-colour-v1/native/reconciliation.json)
retains 20 identical shared files, including eight PPMs. Eight PNGs pass post-contact
IHDR, dimensions and decoded RGB parity, with 13 flat metadata fields each.
[Inspection](../out/validation/crossfeed-20261020/crack-colour-v1/root-view-v1.json) and
[source reconciliation](../out/validation/crossfeed-20261020/crack-colour-v1/root-native-evidence-gate-v1.json) pass.

Debug: nine left-only commands/states, one `"2060C080"` crack-token Save/rebase/reset,
two private PlaySettings/Jump J seed Saves/default display; all eight update/poll
fields zero. Actual-Level, arbitrary paths/transforms, human contrast/readability/comfort,
hardware/input/listening, balance and performance gates remain open.

HUD meter-track colour increment (2026-10-21), **verified**:
path 41, `presentation.hud.meter_track_colour`, reads live on both borrowed
HUD meters. One track premultiplication preserves authored `B8B8B8`, geometry,
resources and existing health/team fill paths.

Both game/editor builds and four focused [CPU cases](../out/validation/crossfeed-20261020/hud-track-colour-v1/build-consumer-result.json)
pass. Ordinary/forced keyline_width 4.1 merged saves retain exact state and
both ledger keys/token pairs; selected reset retains unrelated edits.

`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -HudTrackTuneColourOnly`
passes [both presets](../out/validation/crossfeed-20261020/hud-track-colour-v1/native/runner-result.json):
Release 54 readbacks/12 rows, Debug 90/21; twelve lossless paired 192x32 ROI
stacks, 73728 native pixels/221352 P6 bytes. Paired health/ammo 0/0,.5/.5,1/1
covers three cases, not nine Cartesian combinations. Disjoint HEALTH/PAINT
192x16 regions omit the source gap; these are not contiguous crops.

All 6144 raw RGBA pixels/case precede coherent whole-stack source-model
admission. Independent ROI order is opaque `141414` ink, track, opaque
health/team prefixes. Six controls discriminate twelve tracks: 27648 changed/
9216 unchanged comparisons. Full-covered frames are invariant; alpha0 exposes
ink. Copied whole-HUD equality proves adoption only.

[Parity](../out/validation/crossfeed-20261020/hud-track-colour-v1/native/reconciliation.json)
retains 28 identical shared files, including twelve PPMs.
[Inspection](../out/validation/crossfeed-20261020/hud-track-colour-v1/native/contact-and-view-reconciliation.json)
and [source reconciliation](../out/validation/crossfeed-20261020/hud-track-colour-v1/final-source-reconciliation.json)
pass. Historical source-equivalence defects/correction failures remain preserved;
there is no pixelwise mixing or tolerance.

Debug half/half: nine commands/states, one Track-token Save/rebase/reset,
two private PlaySettings/Jump J seed Saves/default display; zero local/Player/
Level/Scene/State/input/time updates and input polls. Actual-Level, human,
small-view, balance, hardware/listening/performance gates remain open.

HUD health colour increment (2026-10-21), **verified**:
path 40, `presentation.hud.health_colour`, reads live on retained HUD draws.
One base premultiplication preserves opaque `76E8B5`, geometry, resources and
the separate ammunition/team tint.

Both game/editor builds and four focused [CPU cases](../out/validation/crossfeed-20261020/hud-health-colour-v1/build-consumer-result.json)
pass. External HUD paper keyline_width 4.1 rejects ordinary/forced merged saves,
retaining exact source/staged/live/current/baseline and both ledger keys/token
pairs; one-token Save/rebase/reset retains unrelated edits.

`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -HudHealthTuneColourOnly`
passes its first execution in [both presets](../out/validation/crossfeed-20261020/hud-health-colour-v1/native/runner-result.json):
Release 54 readbacks/12 rows/12 crops, Debug 90/21/12. Three supplied health
amounts 0/.5/1 across four palettes use independent track
`[1036,64,1228,80)`: one opaque B8 underlay, then a zero/96/192-pixel prefix.
All 3072 raw RGBA pixels/case and 36864 retained RGB pixels match
(twelve native 192x16 crops, 110760 P6 bytes). Six exact-mask controls
discriminate 13824 changed/4608 unchanged comparisons. Empty-health whole
frames are invariant; ammunition bytes stay fixed; alpha0 retains the track.
Copied full-HUD rasters prove adoption only.

[Parity](../out/validation/crossfeed-20261020/hud-health-colour-v1/native/reconciliation.json)
confirms all 28 shared files, including twelve PPMs, byte-identical.
All twelve crops were [inspected](../out/validation/crossfeed-20261020/hud-health-colour-v1/native/contact-and-view-reconciliation.json);
contacts match and native serialization has no warnings.
[Twelve source/default hashes and four fresh objects/executables reconcile](../out/validation/crossfeed-20261020/hud-health-colour-v1/final-source-reconciliation.json).

Debug's nine full-health commands/states prove one Health-token Save/rebase/
saved reset, two private PlaySettings/Jump J seed Saves/default display and
zero updates/input polls. No Player/Level simulation runs. Actual-Level
visibility and human contrast/readability/comfort, small-view, balance,
hardware/listening/performance acceptance remain open.

Scribble colour increment (2026-10-21), **native v2 verified**:
path 39, `presentation.opening.scribble`, reads live on applicable captured-note
draws. One premultiplication preserves opaque `3050C0`, art/order/resources;
no notes are created or rerolled.

Both game/editor builds and four focused [CPU cases](../out/validation/crossfeed-20261020/scribble-colour-v1/build-consumer-result.json)
pass. Ordinary/forced merged corner_step 0.002 below pen_width 0.004 is rejected,
retaining source/staged/live/current/baseline and both ledger keys/token pairs;
one-token save/reset retains unrelated edits.

[Native v2](../out/validation/crossfeed-20261020/scribble-colour-v2/native/runner-result.json)
passes both presets: shared 40 readbacks/eight rows/eight native 67x67 crops
(35912 pixels, 107840 P6 bytes), Debug 76/17/8. Two paired notes, colour0/tilt0
and colour2/tilt0.1, provide different opaque papers, not a complete matrix.
Five static rectangles plus 74 ordered literal dabs/context match all 4489
raw RGBA pixels; eight wrong controls discriminate 24 rows. All 35912 retained
RGB pixels match one complete representative source model per crop, with no
per-pixel mixing, blanket tolerance or universal GPU claim. Eight PPMs are
byte-identical across presets; all eight crops were inspected. Alpha0 retains
paper/static note. Copied whole front details prove adoption only.

The [first native failure](../out/validation/crossfeed-20261020/scribble-colour-v1/first-native-failure-investigation.json)
retains a supplementary precision-oracle mismatch (175 versus 174).
Four predictive source models represent precision choices;
[D3D11 spec 17.5](https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm)
allows blending at output precision or higher. Model constants were not fitted
to captures; the precision correction changes no production code.

Debug executes nine commands/states in Context A only: one-token Save/rebase/
saved reset, two private PlaySettings/Jump J seed Saves/default display;
all updates remain zero. [Metadata revalidation](../out/validation/crossfeed-20261020/scribble-colour-v2/native/metadata-revalidation-v3.json) passes on saved captures, without a GPU rerun.
Actual-Level visibility and human/readability/small-view/balance/hardware/
listening/performance acceptance remain open.

Silhouette body increment (2026-10-20), **verified**:
path 38, `presentation.opening.silhouette`, reads live on applicable story draws.
One mass-tint premultiplication preserves opaque `0E0E0E`, art/order/resources
and repeated-write multiplicity.

Both game/editor builds, four focused CPU cases and both native first
executions pass. Shade_width 0.004 below pen_width 0.008 rejects ordinary/forced
merged saves, preserving source/staged/live/current/baseline and every ledger
key/token pair; one-token save/rebase/reset retains unrelated edits.

Shared evidence has 61 readbacks, 12 rows and 12 crops (759808 pixels).
Nine literal 3x3 RGBA patches qualify ordered one/two/three body writes;
retained PPMs independently check 324 literal RGB pixels. Thirteen wrong
references yield 34 affected-patch refusals/eight unchanged guards. Alpha0
retains BLACK ink and later art. Copied full artwork proves adoption only;
broad exterior bounds do not independently prove the complete body mask.

Debug's nine commands/nine states across three contexts add 108 readbacks/27
rows: totals 169/39/12. Exact one-Body-token Save/rebase/saved reset and two
private PlaySettings/Jump J seed Saves retain default display, real content/
private preferences/resources/RNG. All updates are zero.

All 28 shared files, including 12 PPMs, and the supplemental contact PNG match
between presets. Root inspected all crops; twelve frozen hashes/default JSON
and fresh Silhouette source/object hashes/timestamps reconcile. Receipts:
`out/validation/crossfeed-20261020/silhouette-body-colour-v1/`.
Child half-size uses a simulated 640x360 screen inside the 1280x720 application.
Actual-Level body visibility, human story/comfort/readability, small-view,
balance, hardware/listening and performance remain open. More fields remain.

Glass colour increment (2026-10-20), **verified**:
path 37, `presentation.opening.glass_colour`, is live at all four Monitor
initial-glass/palette/picker/close writes. Four once-premultiplied tints preserve
opaque `161616`, API/geometry/resources and overlays.

Both game/editor builds, four focused CPU cases and both native first
executions pass. Merged glass width 0.95/x 0.1 rejects ordinary/forced saves,
retaining source/staged/live/current/baseline and both ledger keys/token pairs.
One-token save/rebase/reset retains unrelated edits.

Shared evidence has 100 readbacks, 20 rows, 20 crops (55272 pixels),
18 per-write controls (12 partial straight/squared, six alpha0 straight) and two
blanks. Four main 80x24 montages use one effective opaque-beige backdrop;
eight picker 99x53/eight close 27x26 crops use two genuine Canvas underlays.
Independent main proof covers five ordered ROIs only; standalone proof covers
guarded picker/close ROIs, retaining white frames/caller swatches/24 X dabs.
Copied full-Monitor rasters prove adoption only. Alpha0 underlayer/overlay
checks pass; this does not independently reproduce the full bezel.

Debug's nine commands/nine states across three methods add 108 readbacks/27
rows: totals 208/47/20, standalone black only. Exact one-Glass-token
Save/rebase/saved reset and two private PlaySettings/Jump J seed Saves preserve
default display, real content/private preferences. All updates are zero.

44 shared files, including 20 PPMs, and the supplemental contact PNG match
between presets. Root inspected all crops; twelve frozen hashes/default JSON
and fresh Monitor source/object hashes/timestamps reconcile. Receipts:
`out/validation/crossfeed-20261020/glass-colour-v1/`.
Actual-Level glass visibility remains **unmeasured**. Human story/readability,
small-view, balance, hardware/listening and performance remain open.

Acting face/sweat increment (2026-10-20), **verified**:
paths 35–36 are live on retained FigureActing draws. Two once-premultiplied
tints preserve opaque `WHITE`/`8FD3FF`, geometry and later BLACK pupils.

Both game/editor builds, four focused CPU cases and both native first
executions pass. CPU per-field one-token save/rebase/reset retains siblings;
external `opening.acting.hop=1.001` rejects ordinary/forced merged saves,
retaining source/staged/live/current/baseline and all three ledger keys with
both token strings.

Shared evidence has 90 readbacks, 16 rows, sixteen native 320x128 crops
(655360 pixels), two blank reads and 24 field-specific controls across both
facings/two Canvas colours. First backend comparisons qualify independent
ordered two-eye masks, BLACK pupils and eight sweat strips, including per-write
alpha/opaque parity. Alpha0 retains pupils. Copied-FigureActing full rasters
prove adoption only.

Debug's 14 commands/14 states add 56 readbacks/14 rows, right-facing/black only:
totals 146/30/16. Sweat reset precedes exact one-Face-token Save/rebase;
sibling-retaining resets pass. Two private PlaySettings/Jump J seed Saves retain
default display; real content/private preferences/resources/RNG stay unchanged.
All local/Player/Level/Scene/StateContext/input/time updates are zero.

All 36 shared files, including 16 PPMs, and the supplemental contact PNG match.
Root inspected all crops; twelve frozen hashes/default JSON and fresh actor
object source/hash/timestamps reconcile. Receipts:
`out/validation/crossfeed-20261020/acting-colour-v1/`.
Actual-Level acting visibility remains **unmeasured**, with source callers only.
Human story/readability, small-view, balance, hardware/listening and performance
remain open. More colour fields remain.

Program Level visibility witness (2026-10-20), **verified**:
both presets pass first execution of separate `ProgramLevelVisibilityOnly` /
`program-level-visibility`: one retained actual two-player Crossfeed full
opening, 1280x720, seed 9411.

Each builds one Level and makes one explicit positive 0.4-second setup Level
update, retaining state `opening`. Constructor Scene admission remains source
provenance, not runtime instrumentation. Observation makes zero
Level/Player/Scene/StateContext/input/time/layout updates. One RNG bracket spans
all six draws/assignments/restoration; public state/style/resources stay fixed.
Private story clocks/camera/persona are not public snapshots.

Each preset records six readbacks, three case plus nine sample rows (12 total),
two full baseline/changed PPMs, zero crops/Debug commands/Save calls. All 256
program-patch pixels at (640,100) change RGB (44,44,44) to `35BCE1`, then reset;
256 paper pixels at (400,250) retain `C9D3D9`, and glass (132,100) retains RGB
(22,22,22). Native full-RGBA repeat/reset checks pass. Independent saved-PPM
checks confirm literal patches and 160535 changed pixels, zero outside program
mask `[136,59,1143,539)`.

Nine shared files, including two PPMs, and the supplemental contact PNG match
between presets. Root inspected both compositions; 33 source/support and two
reused game-executable hashes reconcile, with production/defaults unchanged.
Receipts: `out/validation/crossfeed-20261020/program-level-visibility-v1/`.
This adds no colour paths; Program34's separate 20/56 raw-alpha/command evidence
stays unchanged. Other phases/rosters, small views, human story/readability,
balance, physical input, listening and performance remain open.

Program colour increment (2026-10-20), **verified**:
path 34, `presentation.opening.program_colour`, is live on retained Monitor
draws. One local premultiplication preserves opaque `2C2C2C` without event fade
or geometry/art/font changes.

Both game/editor builds, four focused CPU cases and both native first executions
pass. CPU rejects external `opening.palette_height=0` in ordinary/forced complete
merged saves, preserving exact source/staged/live/current/baseline and every
ledger token. One-token save/rebase/reset retains unrelated edits.

Shared evidence has 20 readbacks, four rows, four native 320x192 corner montages
and three negative controls, using one effective Canvas. Debug's nine
statements/nine states add 36 readbacks/nine rows: totals 56/13/4. Independent
ordered raw writes use opaque `161616` glass then visible mask
`[136,59,1143,539)` within ROI `[135,58,1144,540)`, with glass/palette guards;
alpha0 retains glass. Copied-Monitor full raster proves adoption only.
Two private PlaySettings/Jump J seed Saves retain default display;
resources/persona/RNG, real content and private preferences remain unchanged,
with zero
local/Player/Level/StateContext/input/time updates.

All 12 shared files, including four PPMs, and the separate supplemental contact
PNG match
between presets. Root contact inspection, 12 frozen source/support hashes and
unchanged authored presentation reconcile; both Monitor objects have fresh
compile timestamps and reconciled source/object hashes. Receipts: `out/validation/crossfeed-20261020/program-colour-v1/`.
At that direct-consumer checkpoint, actual-Level visibility was
**source-only/unexecuted**; the separate witness above now verifies that one checkpoint.
Human readability/story,
small-view, OS/global input, physical pads, listening and performance remain open.

Power-light colour increment (2026-10-20), **verified**:
path 33, `presentation.opening.monitor_art.power_light`, is live on visible
retained Monitor front details. One local premultiplication preserves default
`C5D6AA`; there is no event fade or geometry/art/font change.

Both game/editor builds, four focused CPU cases (console and story audio per
preset), independent review and both native first executions pass. CPU checks
reject external `opening.monitor_art.shadow_width=0.101` in ordinary/forced
merged saves, preserving exact source/staged/live/current/baseline/ledger state.
Source/profile/resources/persona retention passes across the fixtures.

Shared native evidence has 40 readbacks, eight rows, eight 32x32 ROI crops
(8192 pixels) and six controls. Debug's nine statements/nine states add
72 readbacks/eighteen rows: totals 112/26/8. Independent ordered raw writes
cover outer ink `[1246,652,1256,662)` then inner light
`[1249,655,1254,659)`; colour deltas stay within that inner 5x4 area.
Alpha0 retains opaque ink. The copied-Monitor full-front reference proves
adoption only. Two private PlaySettings/Jump J seed Saves retain default
display, with zero local/Player/Level/StateContext/input/time updates.

All 21 shared artifacts, including eight PPMs, match byte for byte. Root
contact-sheet inspection and twelve frozen source/support hashes reconcile.
Receipts live under `out/validation/crossfeed-20261020/power-light-colour-v1/`.
Attended story/art
comfort, small-view, OS/global input, physical pads, listening and performance
acceptance remain open.

Desktop colour increment (2026-10-20), **verified**:
path 32, `presentation.opening.desktop_colour`, is live on the retained closed
program. One local tint conversion preserves the opaque `008080` default;
there is no event fade or geometry/art/font change.

Both game/editor builds, four focused CPU cases (console and story audio per
preset), independent review and both native first executions pass. CPU coverage includes unrelated-edit retention,
one-token save/rebase/saved reset and invalid external `glass.width=0.95` with
`x=0.1`: ordinary and forced merged saves reject the candidate and preserve exact source/staged/live/
current/baseline/ledger state.

Shared native evidence has 40 readbacks, eight rows, eight 320x192 crops
and six controls. Debug's nine commands add 72 readbacks and eighteen rows:
totals 112/26/8. The independent full-frame literal raw rectangle
`[136,59,1143,588)` is 1007x529, including edge/exterior and alpha0 checks;
a second Monitor supplies adoption comparison rather than the independent
alpha oracle. Two private PlaySettings/Jump J seed Saves, default display and
zero local/Player/Level/StateContext/input/time updates pass.

All 21 shared artifacts, including eight PPMs, match byte for byte; root
contact-sheet inspection and twelve frozen source/support hashes reconcile.
Receipts live under `out/validation/crossfeed-20261020/desktop-colour-v1/`.
Actual story/art comfort, OS/global input, physical pads, listening and performance
acceptance remain open. This proof isolates public `draw_closed`, without a full story route.

Cursor shade/shadow increment (2026-10-20), **verified**:
exact paths 30-31 are `presentation.opening.cursor_art.shade` and `.shadow`,
live on retained story arrows. Production adds only their allowlist entries;
existing drawing, authored defaults and geometry remain unchanged. Both already
premultiply base alpha once, without an event fade. Click ticks keep ink/fill.

Both game/editor builds and four focused CPU cases (console and story audio
per preset) pass. CPU checks cover both sibling resets, unrelated edits, exact two-token
save/rebase and saved-baseline resets. Invalid external `shadow_offset=0.201`
rejects ordinary and forced merged saves, retaining exact source/staged/
live/current/baseline/ledger state and all unselected opening fields/resources.
Build and CPU receipts reconcile.

Both native first executions pass. Shared evidence is 68 readbacks, 24 rows (eight cases and sixteen
sample rows) and eight 72x80 crops. Independent raw samples account for one
shifted shadow write followed by one S-cell shade write at `(649,262)`, and
one shadow-only write at `(656,273)`, including per-write target quantization.
Sixteen targeted comparisons reject straight/squared alpha, including the
shadow contribution beneath translucent shade. Full-cursor copied-dependency raster comparisons
prove adoption; the two raw sites supply independent alpha/layer evidence.
Reduce Effects restores rest, and click ticks retain the fixed arrow.
The existing Cursor23 helper and historical receipts remain unchanged.

Debug's fifteen commands and twelve states add 144 readbacks and 72
rows, for totals of 212/96/8. Two private PlaySettings/Jump J seed Saves, default
display, content, captured resources, caller inputs and RNG remain intact.
There are zero local, Player, Level, StateContext, input or time updates.
All 22 shared artifacts, including eight PPMs, match byte for byte. Root
contact-sheet inspection and twelve frozen source/support hashes reconcile.
Receipts live under `out/validation/crossfeed-20261020/cursor-material-colour-v1/`.
No active-story interaction, physical/global input, human readability, listening
or performance acceptance is claimed. Further colour work remains open.

Reveal ink/paper/shade increment (2026-10-20), **verified**:
exact paths 27-29 are `presentation.opening.reveal_art.ink`, `.paper` and
`.shade`, live on retained visible percentage cards. Only the explicit colour
allowlist changes; drawing, defaults, captured font/sheet/fill bindings and
percentage-text policy stay unchanged. Existing ordered premultiplication,
0.12-alpha airbrush layers and quadratic click beat are retained.

Both game/editor builds and four focused CPU cases (console and story audio
per preset) pass. CPU checks cover all three sibling resets, source-preserving saves,
conflicts/force and exact three-token save/rebase. Invalid external padding
0.181 refuses both ordinary and forced merged saves without changing
source/staged/live/text/baseline/ledger state.

Both native first executions pass. Shared evidence is 228 readbacks and 264 rows (22 cases plus
242 sample rows), with ten crops: six 400x120 material panels and four 800x120
label pairs, 672000 pixels total. Seven material sites and four two-team
half-age tick sites use independent ordered raw-channel references and negative
alpha/overlap/beat controls; actual sample footprints match in both presets.
The material-only route measures actual captured-font `L" "` as exactly
`Vector2F::ZERO`; empty text is not a substitute. A second production consumer
compares complete-card/text adoption only, not independent geometry or glyph
coverage. Expired/Reduce Effects draws retain the settled card.

Debug's 21 commands and fourteen states add 196 readbacks and 336 rows
(28 cases plus 308 samples), for totals of 424/600/10. Three-token private
save/rebase/reset preserves siblings, real content, preferences, Jump J
and default display; two preference seed Saves and zero local/Level/StateContext/
input updates pass. All 26 shared artifacts, including ten PPMs, match byte
for byte; root contact-sheet inspection and twelve frozen source/support
hashes reconcile. Receipts live under
`out/validation/crossfeed-20261020/reveal-colour-v1/`. No active-story route,
physical/global input, human art/readability, listening or performance
acceptance is claimed. `DBG#1` remains open beyond these twenty-nine paths.

Silhouette ink/rim/shade increment (2026-10-20), **verified**:
exact paths 24-26 are `presentation.opening.silhouette_art.ink`, `.rim` and
`.shade`, live on retained visible intro/reveal silhouettes. The production
change is only the explicit allowlist; existing conversion, drawing, defaults
and artwork remain unchanged. Ink/rim use ordinary base premultiplication.
Each shade pass submits `(R*A/4, G*A/4, B*A/4, A/4)` in the existing four-pass
order, with per-write target quantization and no temporal fade. The separate
`opening.silhouette` body fill stays outside this colour scope.

Both game/editor builds and four focused CPU cases (console and story audio
per preset) pass. CPU checks retain both borrowed record addresses, body fill, unselected colours,
resources, geometry and caller-owned persona/pose inputs. The three fields
have independent sibling resets, exact LF/CRLF saves, conflicts/force and a
three-token save/rebase. An external `shade_width < pen_width` rejects the
complete merged candidate, even with force, preserving source/staged/live/text/
baseline/ledger state and allowing each colour to reset to its saved baseline.

Both native first executions pass. Shared evidence has 38 readbacks, ten rows, eight 440x240
montage crops and ten controls. Independent adult mug patches are three 3x3
interiors centred at ink `(963,690)`, rim `(932,627)` and shade `(959,679)`,
audited against per-edge truncation. Their ordered raw submissions represent
two ink writes, one rim write over the opaque body and four quarter-alpha
shade writes; target quantization occurs after each write. Controls
reject straight RGB, squared base alpha, a missing ink/shade pass and full
shade alpha. Uniform patch interiors, all-art zero alpha retaining adult/child
body, fixed caller inputs/resources/RNG and exact default restoration pass.
Child coverage is aggregate colour adoption/restoration,
not an independent full-geometry oracle.

Debug adds thirteen tune/reset/save commands, 78 readbacks and 26 rows:
totals of 116 readbacks, 36 rows and eight crops. Exact three-token
private save/rebase, sibling retention, saved-raster resets, actual content,
private preferences and no queued match work pass. Its two
seed Saves cover PlaySettings and Jump J while retaining default display.
All 21 shared artifacts, including eight PPMs, match byte for byte; the root
contact-sheet inspection and twelve frozen source/support hashes reconcile.
Receipts live under `out/validation/crossfeed-20261020/silhouette-colour-v1/`.
The native scope
has zero local, Player, Level, StateContext, input or time updates. No
active-match route, physical/global input, human readability, listening or
uncontended performance acceptance is claimed. Further colour consumers and
unscoped string/font/missing-key work remain open.

Cursor ink/fill increment (2026-10-20), **verified**:
the twenty-second/twenty-third exact paths expose
`presentation.opening.cursor_art.ink` and `.fill` on retained resting arrows
and reveal click ticks. Both already had correct base-alpha conversion; this
increment changes only the production allowlist. No drawing, authored value,
geometry or timing changes are made. Shade/shadow remain independent residual
materials when both selected colours have alpha0. Reduce Effects removes only
the detached click ticks, retaining the arrow; there is no event opacity fade.

Both game/editor builds and the focused `debug_console` and `story_audio` CPU
cases pass in both presets. Shared tokens/refusals, independent sibling resets,
exact LF/CRLF save/rebase, conflicts/force and structural aliases include both
fields. Borrowed opening/cursor records and unselected geometry/resources are
retained. External click_duration0.501 rejects ordinary and forced complete
merged saves without changing source/staged/live/text/baseline/ledger state.
The first Debug run caught a stale object: private promotion preserved a source
timestamp older than the previous `tune_scope.cpp.obj`, so Ninja skipped the
new allowlist. Source bytes and assertions stayed unchanged. Refreshing source
timestamps rebuilt the object with both paths; fresh Debug/Release checks pass.
The original failure, build log and `stale-object-diagnosis.json` are retained.

`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -CursorTuneColoursOnly`
passes on its first native execution in both presets. Shared evidence is
124 readbacks, 112 CSV rows and twelve 72x80 crops. Seven literal colour states
over black/nonblack underlays exercise independent ink/fill changes, partial
alpha and pair alpha0 on the same existing cursor. Immutable baseline-derived
second-consumer rasters prove full-shape adoption/default parity; seven raw
sample sites with targeted straight/squared-base-alpha controls provide the
independent alpha proof. They are not an independent full cursor geometry
oracle. Samples account for actual per-edge sprite truncation and ordered
click-ink/fill overlap. Unselected shade/shadow and Reduce Effects restoration
remain checked. The fixed synthetic draw input is screen_height720.

Debug adds fifteen actual commands, 168 readbacks and 192 rows: totals of
292 readbacks, 304 rows and twelve crops. Independent reset keeps the sibling
pending; exact two-token private save/rebase and saved-baseline reset pass.
Actual content, private preferences, current saved keyboard binding, queued
work and global RNG remain unchanged. Both private PlaySettings/display seed
Saves succeed; this scope does not seed a nondefault Jump key. Release remains
consumer-only. No local preparation, Level or StateContext update is performed.

All 30 shared artifacts, including twelve PPM crops and the raw-sample CSV,
match byte for byte between presets. The contact sheet was visually inspected;
twelve frozen source/support hashes still match, including the unchanged
ScreenCursor consumer. Evidence is under
`out/validation/crossfeed-20261020/cursor-colour-v1/`, including
`build-consumer-reconciliation.json`, `native/reconciliation.json` and
`final-source-reconciliation.json`. Native small-view/human readability,
physical/global input, listening, performance and active-match route acceptance
remain open. `DBG#1` remains open beyond twenty-three audited fields.

Respawn pen colour increment (2026-10-20), **verified**:
the twenty-first exact path, `presentation.respawn_redraw.pen_colour`, is live
for existing partial outlines. Its one-site base-alpha conversion preserves
the opaque BLACK default, captured resources and ordered stroke overlaps.
Progress changes path length rather than opacity; no event fade is introduced.

Both game/editor builds and the focused `debug_console`, `movement` and
`story_audio` CPU cases pass in both presets. Movement still measures the P3
95-unit tap and 207.375-unit held rise. Shared strict-token/reset/LF+CRLF
save/rebase/conflict/force checks include this field. A real dead Player is
prepared once through bounded Level updates, then timers/state/camera,
figure/cull bounds, match fingerprint, all stroke values and boundary queries
stay frozen across thirteen commands. Ordinary and forced merged saves reject
external `draw_from == fill_at`, preserving source/staged/live/text/baseline/
ledger state; reset still returns to the saved baseline.

`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -RespawnPenTuneColoursOnly`
passes on its first execution in both presets. Shared evidence uses 62
readbacks, eighteen CSV rows, eight 440x160 crops and three local Player
preparations. Four retained direct outlines cover both facings and two partial
path lengths alongside an actual frozen dead Player. An independent ordered
Manhattan-prefix/raw-nib reference, explicit overlap channel samples, opaque
parity and straight/squared-base-alpha controls pass. Nonblack zero alpha and
below/at-start boundaries are blank. Actual whole/filled figures remain
nonblank and colour-independent; Reduce Effects retains the same essential
outline. Snapshot and repeat-raster checks preserve the Player, geometry,
ordered strokes and global RNG between the counted preparations.

Debug adds nine actual tune/reset/save commands, 54 readbacks, eighteen rows
and two local Player preparations: totals of 116 readbacks, 36 rows, eight
crops and five preparations. Exact one-token private save/rebase, reset to
saved, unchanged actual content and preferences, and no queued match work pass.
Its two successful private seed Saves cover PlaySettings and Jump J; default
display settings remain unchanged. Release runs the consumer proof alone.
Neither native scope advances a Level, StateContext or input frame.

All 21 shared artifacts, including eight PPM crops, match byte for byte between
presets. The contact sheet was visually inspected; twelve frozen source hashes
still match. Evidence is under
`out/validation/crossfeed-20261020/respawn-pen-colour-v1/`, including
`build-consumer-result.json`, `native/reconciliation.json` and
`final-source-reconciliation.json`. No physical/global input, active-match
route, human readability, listening or performance acceptance is claimed.
`DBG#1` remains open beyond twenty-one audited fields.

Idle-drip keyline increment (2026-10-20), **verified**:
the twentieth exact path, `presentation.idle_drip.keyline`, is live for existing
attached beads and falling drops. The common colour reaches all five stable
profile records; only Sprayer, Sniper and Bucket remain enabled and receive
Level drips. Only the outline tint gains base-alpha premultiplication. There
is no event fade, and captured opaque paint, sheet/frame, geometry and timing
are unchanged.

Both game/editor builds and all six focused `debug_console`, `idle_drip` and
`menu_presentation` CPU cases pass across Debug/Release. Shared token/refusal,
reset, exact LF/CRLF save/rebase, conflict/force and unrelated external-edit
checks include the new field. Six preconstructed aged attached/falling effects
retain public state and all five borrowed profile records across commands;
an unrelated invalid Sniper interval rejects the complete merged candidate,
including with force, without changing source/staged/live/ledger state.
The initial Debug console test compilation found three shadowing locals under
`/WX`. Only those test names/references changed; the original Debug game/editor
build and idle-drip pass were retained, then failed/unfinished checks passed.
The original failure and both source freezes remain alongside the recovery.

`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -IdleDripTuneColoursOnly`
passes on its first execution in both presets. Shared evidence uses 46
readbacks, ten CSV rows, eight 380x260 crops and fifteen local cosmetic advance
calls. Independent raw rectangles/channels check six retained consumers across
all three tools, opaque-default parity, straight-RGB and squared-base-alpha
negative references, and nonblack zero alpha with all six opaque paint
interiors retained. Enabled/Reduce Effects draws are blank; restoration returns
the exact pixels, profile flags and public state. Frozen rendering preserves
public timers/drops/bounds and global RNG. Private cosmetic-stream invariance
is source-audited constness only, not an executed future-sequence proof.

Debug adds nine actual tune/reset/save commands, 60 readbacks, twenty rows
and fifteen local advances: totals of 106 readbacks, thirty rows, eight crops
and thirty local advances. Exact one-token private save/rebase, saved-baseline
reset and unchanged actual content/preferences/queued work pass; both private
preference seed Saves succeed. Release remains consumer-only. Neither native
scope updates a Level or StateContext.

All 22 shared artifacts, including eight PPM crops and the guard CSV, match
byte for byte between presets. The contact sheet was visually inspected and
twelve frozen source hashes still match. Evidence is under
`out/validation/crossfeed-20261020/idle-drip-colour-v1/`, especially
`build-consumer-reconciliation.json`, `native/reconciliation.json` and
`final-source-reconciliation.json`. No active-match route, physical/global
input, listening, human readability or performance acceptance is claimed.
`DBG#1` remains open beyond twenty audited fields.

Blocked-shot colour increment (2026-10-20), **verified**:
the eighteenth/nineteenth exact paths expose the existing highlight/keyline
pair. The draw helper premultiplies straight base RGBA once before the
unchanged quadratic fade. Both game/editor builds and the focused
`debug_console`, `blocked_shot` and `menu_presentation` CPU cases pass in both
presets. Retained aged cues cover every paint owner and the invalid-owner
bucket; exact LF/CRLF saves, conflicts/force, independent reset and invalid
complete merged-candidate rollback retain source/staged/live/ledger state.

`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -BlockedShotTuneColoursOnly`
passes on its first execution in both presets. Shared consumer evidence is
32 readbacks, eight CSV rows, eight 440x240 crops and eight controls. Eight
contacts span four cardinal normals at full/half life, with the existing
half-life fade of 0.75. Independent raw geometry/channels, opaque-default
legacy parity, straight-alpha/doubled-fade controls and stored opaque team-bead
interiors pass. Nonblack zero-alpha highlight/keyline removes their ticks and
outlines while retaining those beads. Snapshots cover stored contact tints,
position/normal/owner, age, all ten owner-repeat budget slots per instance,
per-cue/aggregate bounds and draw RNG.

Debug adds fifteen actual tune/reset/save commands, 60 readbacks and twenty
rows: totals of 92 readbacks, 28 rows and eight crops. Exact two-token private
source/staged save/rebase, reset to the saved baseline and unchanged actual
content/private preferences pass. Its two private preference seed Saves
succeed; no Save failure is expected or observed. Release runs the consumer
proof alone. One local half-age preparation per fixture means Debug totals
two local preparations and Release one; neither updates a Level or StateContext.

All 21 shared artifacts, including eight PPM crops, match byte for byte between
presets. The contact sheet was visually inspected, and all twelve frozen
production/CPU/native source hashes still match the validated files. Evidence
is under `out/validation/crossfeed-20261020/blocked-shot-colour-v1/`, including
`build-consumer-result.json`, `native/reconciliation.json` and
`final-source-reconciliation.json`. These synthetic contact checks establish
no new shot-launch classification, active-match route, physical/global input,
OS-focus delivery, listening, human readability or performance acceptance.
`DBG#1` remains open beyond nineteen audited fields.

Combat-colour increment (2026-10-20), **verified**:
the sixteenth/seventeenth exact paths expose the existing highlight/keyline
pair. `CombatFeedback` keeps its borrowed presentation record; its shared fill
helper now premultiplies the authored base alpha before applying the event
fade once. Opaque reviewed defaults and opaque team fills retain their pixels;
sheet/fill capture, cue admission, geometry and timing are unchanged.

Both game/editor builds pass. The focused `debug_console`, `combat_feedback`
and `menu_presentation` CPU cases pass in each preset. The seventeen-path
checks cover named/RGB/RGBA tokens and refusals, independent reset, exact
LF/CRLF saves, conflict/force and unrelated external edits. Preconstructed real
hurt/burst cues retain public timers, detached death rectangle and cull bounds
while colour commands adopt straight partial/nonblack zero-alpha channels.
An unrelated invalid combat timing constraint rejects the complete merged save
and retains source/staged/live/ledger state, even with force. Release
developer-tool exclusion remains intact.

Both focused native runs pass. The isolated matrix is
`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -CombatTuneColoursOnly`.
Four preconstructed full/half-life hurt and
elimination cues are compared over black/nonblack underlays with independent
raw-source rectangles and explicit RGBA channels. Opaque legacy parity,
straight-alpha/double-fade negative controls, opaque team interiors and frozen
public timers/death rectangles/bounds/draw RNG are checked. Each preset's shared
consumer scope uses 32 readbacks, eight CSV rows, eight 420x360 crops and eight
controls, plus two local effect-age preparations. It performs zero Level or
StateContext updates.

Debug adds fifteen actual validated tune/reset/save commands, 60 readbacks,
twenty rows and two local age preparations on borrowed consumers. Its exact
private two-token source/staged save and rebase, reset to the saved baseline,
actual content/tuning, private preferences (including the saved Jump binding)
and absence of queued match work are checked. Debug totals are 92 readbacks,
28 rows, eight crops and four local age preparations, with zero Level or
StateContext updates. Two private preference seed Saves succeed; no controlled
Save failure is exercised by this native scope.

All 21 shared capture files match byte for byte, including eight PPMs, their
converted crops and both contact sheets. The compact sheet was visually
inspected; this does not close human contrast/comfort acceptance. Evidence is
under `out/validation/crossfeed-20261020/combat-colour-v1/`, including
`build-consumer-result.json`, `native/runner-result.json` and
`native/reconciliation.json`. No active-match route, OS-focus delivery,
physical/global input, listening, human readability or uncontended performance
acceptance is claimed. `DBG#1` remains open beyond these seventeen fields.

Identity-background increment (2026-10-20), **verified**:
the fifteenth exact path uses the existing opaque/distinct badge validation and
the shared live world/HUD/menu renderer. Added CPU checks cover independent
ink/background reset, exact LF/CRLF save/rebase, conflicts/force and both semantic
merge directions: an independently valid external sibling colour must not make
the merged pair equal. A rejected merge preserves source bytes and the prior
live, staged and pending states, even with force. The native companion checks
paper pixels while retaining ink, geometry, visibility and draw state.

Both game/editor builds and focused Debug/Release console acceptance pass.
The first CPU compile caught shadowed test-local names under `/W4 /WX`; that
failed log is retained beside the corrected final receipts. Both native
Identity-only runs pass, with all 151 shared PPMs matching byte for byte.
The background companion adds 28 readbacks, five rows and four crops per
preset, with no simulation updates. Debug adds 40 readbacks, eight rows and
ten real command attempts (seven accepted, three semantic refusals), private
one-token save/rebase and reset to the saved baseline. Private play preferences
and saved Jump J remain unchanged. Team/Weapon/Results are source-audited users
of the shared renderer; their pages are not newly opened by this increment.
Evidence is under `out/validation/crossfeed-20261020/background-colour-v1/`,
including `native-reconciliation.json` and `consumer-final/`. Human
contrast/readability, physical-pad, listening and performance acceptance remain open.

Identity-ink increment (2026-10-20), **verified**: the fourteenth
exact path uses the existing opaque/distinct badge-colour validation. Added CPU
checks cover atomic semantic refusals, independent reset, exact LF/CRLF save and
rebase, token conflicts/force and unrelated external edits. A valid external
background edit that makes the merged ink invalid must restore the exact source
bytes and retain the previous live, staged and pending states, even with force.
Both builds pass and all 40 consumer cases have successful receipts per preset
in `out/validation/crossfeed-20261020/camera-final-v1/consumer-reconciliation.json`.
The retained Release DisplayTrial isolated-save seed failure passed with the
unchanged binary in a fresh private directory; later unfinished cases passed
separately. No save code changed and the transient failure is not diagnosed.

Both native Identity-only runs pass with 147 shared PPM frames/crops matching
byte for byte. The new ink scope uses 28 additional readbacks, five rows and
four crops per preset; Debug adds 40 readbacks, eight rows and ten real command
attempts (seven accepted, three semantic refusals), exact private one-token
save/rebase and saved-baseline reset. Preconstructed PlayerBadge/UiPlayerBadge
roster cells and actual frozen world/HUD badges pass literal opaque pixel
substitution, captured geometry/state, hidden/focus policy and RNG/fingerprint
checks. Team Select, Weapon Select and Results wiring is source-audited through
that shared renderer; those pages are not directly opened by this increment.
Run paths and counts are in `camera-final-v1/native-reconciliation.json`.
No physical-pad, listening, human readability or performance acceptance is claimed.

Validation of the seven-colour increment (2026-10-18): Debug and Release build
under `/W4 /WX` and pass all 36 consumer cases each. The Debug console case
checks raw string spans/escapes and first-occurrence keys, the strict allowlist,
safe named/RGB/RGBA values and refusals, fixed borrowed records, F9 preservation,
read/diff/reset, exact LF/CRLF save splices, conflicts/force and unrelated disk
edits, completion and inspect rows. Real Level checks retain recording/replay
edit restrictions, existing Label identity, premultiplied RGBA adoption, hidden
zero tint and unchanged match fingerprints/RNG. Release still excludes developer
tool RTTI.

`scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -TuneColoursOnly` passes
on the NVIDIA GeForce RTX 5080 at hidden 1280x720. Eleven records and nine
captures cover all seven real consumers, Home/Mode capture and reopening/reset,
full resource rejection, queued reload refusal and rebuild/rebase, staged
fallback and saving. Content, user files, profiles and candidate TEMP/TMP are
private to the run. Reload now honours the same content-directory overrides as
`tune`; its normal source/staged defaults and source precedence are unchanged.
The runner explicitly refuses this Debug-only scope for Release before compiling
or launching it. Both game executables also pass isolated system-PATH startup,
per-monitor-v2 DPI awareness and graceful shutdown with the accepted audio bank.

Evidence is under `out/validation/tune-colours/`; the native run is
`a2271b60ffb04b71860a8629e3a40d7a`. No tests were skipped. These muted hidden
checks establish no physical-pad, listening, human readability or uncontended
performance acceptance.

Validation of the Team-colour increment (2026-10-19): both builds and all 37 CPU
cases per preset pass. The nine-path checks cover the new fields' named/RGB/RGBA
commands, exact tokens and scopes, invalid values, independent reset, LF/CRLF
splices, conflicts/force, unrelated external changes and structural aliases.
Independent review strengthened the save loop so each Team case introduces a
fresh unrelated colour; the final Debug console case was recompiled and passed
after that assertion-only change. The original full-suite source and receipts
are retained, and Release compiles this test body out.

`scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -TeamTuneColoursOnly`
passes with actual tune/reset commands on one open two-player Team Select page.
Seven updates, three CSV records and two compact icon strips show a ready and an
unready seat retaining their colours before refresh, adopting both edits on the
next connected update and restoring the exact same-page baseline after reset.
Independent icon rasters, outside-strip pixels, captured artwork/fonts, sparse
roster, input snapshots, settings, save bytes and draw RNG are checked. The
dedicated Release guard refuses this scope before compilation or run creation;
Release developer-tool exclusion still passes CPU acceptance. Both isolated
system-path startups pass with the published audio bank and per-monitor-v2 DPI
awareness. Evidence is under `out/validation/team-tune-colours/`. These muted
hidden checks establish no physical-pad, listening, human readability or
uncontended performance acceptance.

Validation of the Weapon-colour increment (2026-10-19): both builds and the
complete Debug console CPU case in each preset pass. The eleven-path checks
cover independent Weapon reset, named/RGB/RGBA commands, exact LF/CRLF splices,
fresh unrelated external edits, conflicts/force and structural aliases. The
full 38-case suites from the preceding menu-label commit remain the baseline;
they were not rerun for these two Debug-only allowlist entries. Release
developer-tool exclusion still passes.

`scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -WeaponTuneColoursOnly`
passes on one open two-player Weapon Select page. Four actual tune/reset
commands, ten updates, six CSV records and two compact crops check frozen,
inactive and missing-pad retention, neutral reconnect adoption and exact reset.
Independent straight-alpha colours and premultiplied icon rasters check both
ready and unready tiles; artwork, labels, outside-card pixels, sparse roster,
input snapshots, settings, private source/staged content, save bytes and draw
RNG are preserved. The dedicated Release guard refuses this scope before
compilation or run creation. Both isolated system-path startups pass with the
published audio bank and per-monitor-v2 DPI awareness. Evidence is under
`out/validation/weapon-tune-colours/`. These muted hidden checks establish no
physical-pad, listening, human readability or uncontended performance acceptance.

Validation of the weapon-feedback increment (2026-10-19): both builds and all
38 CPU cases per preset pass. The thirteen-path checks cover named/RGB/RGBA
commands, independent reset, exact LF/CRLF save splices, conflicts/force, fresh
unrelated external edits and completion. The new keyline colour shares its
completion prefix with the existing numeric `keyline_width`; both remain
available. Release developer-tool exclusion still passes.

`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -FeedbackTuneColoursOnly`
passes in both presets at hidden 1280x720. Each consumer-only run uses 56
readbacks, eight CSV records and eight 360x280 crops (2,419,320 PPM bytes).
Independent raw-source rasters check opaque defaults, RGB, half-alpha RGBA and
nonblack zero-alpha colours over black and coloured backgrounds. Opaque-default
legacy parity, deliberate straight-alpha and doubled-fade negative controls,
Reduce Effects, team centres and frozen geometry/timers/RNG are checked. The
eleven shared CSV/PPM/summary files are byte-identical between presets.

Debug additionally executes fifteen actual tune/reset/save commands against
preconstructed consumers, using 66 more readbacks and twenty CSV records.
Private two-token save/rebase, reset to saved values, preferences and source/
staged bytes are checked. Release runs the consumer proof alone. Neither path
updates a state or enters a match. Both isolated system-PATH startups pass with
the published audio bank and per-monitor-v2 DPI awareness.

Evidence is under `out/validation/feedback-tune-colours/`. The initial CPU
completion assertion and two native fixture assertions were corrected; their
sources, failed logs and receipts remain alongside the passing runs. Native
corrections sample the readiness bead's interior and read successful command
replies from the console log. No checks were skipped. These hidden checks
establish no physical-pad, listening, human readability or uncontended
performance acceptance. `DBG#1` remains open beyond the thirteen audited paths.

Alpha-consumer audit (2026-10-19): six already-supported fields still submitted
straight tint RGB to the engine's premultiplied blend: the three movement inks,
debug foreground/shadow and the captured Home background. Twenty-one drawing
submissions across fourteen client files now convert the base tint once. All
six authored defaults are opaque, so their existing channels and movement fades
are preserved. The presentation records, parser, thirteen-path allowlist and
save format are unchanged. The earlier seven-field native probe checked opaque
movement/debug text; its partial/zero-alpha pixel proof covered station labels.

Both builds, all 38 CPU cases per preset and both published-bank/DPI2/system-PATH
startups pass. `scripts/check-painter.ps1 -Preset <preset> -SkipBuild
-ExposedColourAlphaOnly` passes in both presets at hidden 1280x720. The shared
consumer proof uses 285 readbacks, 142 CSV rows and twelve native crops
(3,485,556 PPM bytes). Seventeen shared CSV/PPM/summary files are byte-identical
between presets, including all fifteen raw CSV/PPM files.

All thirteen real Home-background capture sites are opened directly: 52 page
openings check opaque defaults, half-alpha, nonblack zero-alpha, retained pages,
reopening and exact default restoration over two underlays. Independent raw
references preserve the existing scrim. Display confirmation uses only the
probe's guarded hidden window and the same windowed 1280x720 candidate, with
destructor rollback and no Keep action. Private preferences and save bytes stay
unchanged after the two successful seed writes.

Twenty preconstructed movement cues cover takeoff, landing, step and skid,
full/half life, and retained opaque/partial/zero-alpha contacted paint. An
independent integer puff mask and literal contact geometry check base-alpha
conversion before fade, opaque parity and deliberate straight/doubled-fade
negative controls. Actual ordinary DebugText matches nineteen literal rows and
raw font tints. Ten local effect-age updates prepare half-life fixtures; no
Player, Level or StateContext update is performed.

Debug adds eight validated private tune/reset commands and independent frozen/
live console, paint/status-panel and perf text references. Its additional 104
readbacks, 48 rows and four crops bring the totals to 389/190/16 (4,929,456 PPM
bytes). Fixed severity/caret/status/perf foregrounds, captured fonts, log/input
state, synthetic perf samples, private content/preferences and draw RNG are
preserved. Release excludes this console scope.

Evidence is under `out/validation/exposed-colour-alpha/`. Source review caught
and corrected the initial probe's one-pixel landing-outline clipping before
execution; its original source and runner remain archived. Both native presets
passed on their first execution. These checks establish no active-match route,
physical-device, listening, human readability or uncontended performance
acceptance. `DBG#1` remains open; this audit adds no new tunable fields.

## Foreground checks (`CI#1`)

**UNSCOPED.** `scripts/check-debug-console.ps1` remains an opt-in local check:
its real key events and focus-sensitive scripts need a reliable foreground
desktop, which the current hosted CI workflow does not establish. First assess
an attended/dedicated runner model with explicit focus ownership and retained
failure evidence. Scope CI integration only after that model is demonstrated;
a hidden launch or synthetic CPU input check does not qualify the foreground
workflow. Existing CPU acceptance and startup/editor CI checks remain in place.
