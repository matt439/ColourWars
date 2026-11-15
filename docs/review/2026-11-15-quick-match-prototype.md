# LOCAL ONLY: reviewed one-bot shortcut prototype

Base: master after #93 (`0fd18572`). Not an approved shipping design. No push,
PR, release, deployment, nightly or issue change is authorized by this prototype.

## Measured result, not a projection

The same native D3D11 Release rehearsal ran on fresh isolated profiles with
keyboard and synthetic-controller input, the actual manifest and production
GameMenu/GameLevel. Both routes chose Movement Course and reached natural
240-second Results, End, Rematch and the next live match.

| From Title to match launch | Confirmations | Directions |
|---|---:|---:|
| Existing full Play, one human + Normal bot | 13 | 11 |
| Optional reviewed Play a bot preset | 5 | 7 |

This saves **8 confirmations (62%) and 4 directions (36%)** for the measured
route. Both optional narrative skips included, the complete first match to live
rematch comparison is 17 versus 9 confirmations. Results-to-End-to-Rematch still
requires two; it was already short and is unchanged. The five directional
steps to Movement Course are unchanged, as is the arena briefing/lock/start.

Separate preflight checks exercised review Back without a saved match or control
change and Customise into the existing full Mode Select page, compared with an
independently constructed Mode Select raster. Counts reset at fresh Title after
that separate recovery exercise. The existing full solo practice route also
passed for both inputs, including untimed practice and pause/resume.

## What is actually removed

The 13 full-route confirmations are Title, Play, mode, player count, roster
continue, two team readiness actions, four tool/sub readiness actions, arena lock
and Start. Most are **real informed choices**, not accidental duplicate presses.
The prototype batches mode/count/roster/team/tool/sub decisions into one explicit
review. It does not label those choices redundant or silently erase them.
Arena lock followed by Start intentionally makes READY visible before launch;
that existing safety/input boundary is retained, not optimised away.

## Disclosed provisional preset

An optional Home action (full Play remains selected by default and unchanged)
opens a review, not a match. It states:

- One human on the confirming device, Team A; one Normal bot, Team B.
- Both stock Sprayer + Grenade, no Boost/handicap.
- Single Match, Standard rules; current authored default duration shown as a
  numeric value (4 min in this build).
- Existing controls, colours and accessibility settings unchanged.
- Player chooses the arena on the existing full arena screen, with its briefing.

Choose arena accepts the review. Customise uses the full existing setup path.
Back from review changes nothing. Saving still occurs through the existing
normal match-start path. Runtime assertions verify launched teams, bot difficulty,
tools, subs, handicaps, mode and duration against the disclosed preset.

## Product decision before any shipping work

FTR#14 remains UNSCOPED. An opt-in prefilled preset is a new product choice,
particularly fixed opposing teams and waiving separate bot/tool readiness. This
prototype does not establish authorization to ship those defaults.

Recommended option: approve the disclosed one-human/Normal-bot starter preset,
keeping an explicit review, Customise, free arena choice and unchanged full Play.
Alternative: use a one-page editable setup with no preselected teams/tools; this
preserves individual decisions but cannot claim this measured 5-confirm result.
A repeat-last-setup shortcut would avoid new defaults but does not help a fresh
profile. These alternatives have not been prototyped or measured here.

## Limits / retained evidence

`-FirstMatchFlowOnly` passed all six routes locally; Release build and
`git diff --check` passed. This is a native acceptance executable linked to game
code, not OS-driven standalone-window or physical-controller acceptance. Time
advances in ordinary 1/60 simulation steps, not four minutes of wall-clock play.
No listening, physical input, performance, localization, full visual regression,
production layout/schema qualification or hosted CI claim.

Prototype uses fixed review geometry and has not updated the independent Home
raster fixtures to include its new action. It is intentionally not PR-ready.
Captures and action CSVs: `out/validation/quick-prototype/x64-release/` run
`d5df9d2839a34f4084ca20a79058937f`. Review screenshot visually inspected at720p.
