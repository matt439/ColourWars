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

## Readiness review and Join-preserving revision (local only)

The original 5-confirm bypass is superseded, not proposed for shipping. Exact
source requirement: `docs/review/2026-11-10-project-review.md`, REV#32 says
"Join cannot be skipped, because devices must claim seats and profiles are worn
per run and not saved". ROAD#21 in `docs/feature-roadmap.md` explains that a pad
slot is not a person and the next session starts with every pad bare. Join also
applies a chosen profile's Boost (`RosterSetupPage::take_profile_boost`).

The revised prototype goes review -> **existing Join** -> arena. Join's real
profile chooser, host connection gate and roster controls remain available. Only
an unchanged two-seat, one-human/Normal-bot, opposing-A/B setup takes the shorter
continuation. Other roster choices fall back to the full Team/Weapon route.
No new multiplayer join implementation was added.

Measured again with the actual Release runtime: **6 confirmations / 7 directions**
to launch versus **13 / 11** on full Play. Through natural Results and live
Rematch, including both optional story skips: **10 versus 17 confirmations**.
The synthetic-pad case also presses X once at Join to select an isolated saved
profile; this is an **additional profile-selection action**, not a confirm or a
direction. The probe checks that it is worn and its Boost=2 reaches the match.
Keyboard uses the disclosed no-Boost preset. Both retain ordinary preferences.

### Source of defaults / authority

`game/match/seat_setup.h` already defaults to Sprayer, Grenade, Normal difficulty
and no Boost. `game/match/match_setup.h` defaults to Single Match, Standard rules,
no duration override and one-human viewport. Authored duration is displayed.
**Two seats, bot P2 and opposing A/B are new preset choices**, not existing
MatchSetup defaults: roster starts empty and SeatSetup team starts `none`.

FTR#14 says "Candidates: a Quick Play item ... None is designed." TODO defines
UNSCOPED as undesigned, not forbidden. No explicit owner-only reservation for the
preset's two-seat/opposing-team choices was found in the current relevant docs.
The Join/profile requirement is an actual documented constraint and is retained.
Do not describe all preset choices as merely composing existing defaults.

Recommended reversible choice: optional disclosed one-human/Normal-bot preset,
with existing Join and full Customise, unchanged default Play and free arena
choice. This is a scoped product bet, not a change to core match mechanics.
Alternative: editable one-page setup, unmeasured and larger; repeat-last-setup
does not solve fresh-profile onboarding. This remains a local prototype pending
root's product-boundary review, not shipping permission.

### Bounded checks and remaining release qualifications

Release build and native FirstMatchFlowOnly passed all six routes. Existing full
practice and versus routes, review cancel, Customise into full Mode, natural
Results and Rematch still pass. Review rasters were captured with keyboard/pad at
1024x768, 1280x720, 1920x1080 and 2560x1080, effects reduced and ordinary. Inspected
4:3 original and ultrawide revised captures: no visible clipping; body/action gap
is tight. Captures are not an independent bounds assertion or motion acceptance.
Control bindings survive cancel/Customise; launched shared/per-pad comfort,
sticks, rumble, aim and CRT preferences are asserted unchanged. This is not a
physical-pad, performance or full accessibility acceptance claim.

Minimum before a release-separable PR:
- Move fixed prototype geometry into authored presentation/schema; add independent
  bounds/overlap/readability and Home focus/raster fixtures, all supported aspects,
  pseudo-locale and CRT on/off. No CRT rendering acceptance was performed here.
- Test Back at Join/arena, change bot difficulty/type, disconnect/reconnect,
  repeated confirms and saved/remembered setup recovery; recheck full route.
- Address and test already-worn profiles: the proposed setup starts Boost at zero;
  Join currently takes Boost when changing a profile, not automatically on entry.
  The new-profile-selection proof does not establish already-worn inheritance.
- Verify effective remapped controls, profile comfort overrides and colour palette,
  not only unchanged stored settings. Test profile changes through rematch.
- Run Debug, menu_presentation/locales, lint and required exact-head CI. Do not
  update independent fixtures merely to make an unexplained raster difference pass.

Evidence: `out/validation/quick-join-profile/x64-release/dfc8a5f35794400f94ef21296c7926aa`.
The intermediate `quick-join-final` failed because a broad test edit accidentally
added a confirm to the original practice path; corrected that test edit without
changing expectations, then reran the complete six-route matrix successfully.
No hosted CI, push, PR, deployment, release or #88 change.
`menu_text` and `team_selection` CPU acceptance also passed in Release; evidence in `out/validation/quick-readiness-cpu/x64-release`.
