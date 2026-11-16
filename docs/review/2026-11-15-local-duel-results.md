# Two-human first match and clear score bases

Base: merged Play a bot (`432d5d17`). Native production menu/game rehearsal on
MattPC, D3D11 hardware, with isolated profiles and synthetic input snapshots.
This is not physical-controller, listening, performance or OS-driven-window
acceptance. Four-minute matches advance through ordinary 1/60 simulation steps.

## What actually happened

Fresh full Play on Crossfeed was rehearsed in both device orders:

- P1 keyboard/mouse, P2 Pad 4.
- P1 Pad 3, P2 keyboard/mouse.

Each needed **14 confirmations and 4 directions across both people** to launch.
The host contributed ten confirmations; the guest contributed four. The path
includes actual Join, separate team/tool/sub choices, arena lock and Start.
With both optional narrative skips, natural Results, End and Rematch, the total
was **18 confirmations**. Choosing Movement Course instead added four arena
navigation directions in the earlier exploratory run; it is not a setup defect.

Join and HUD correctly labelled each roster position and its sparse device.
The guest could advance Results/End and request Rematch. Disconnecting the pad
in the rematch paused it; the already-joined keyboard could not bypass the missing
human. Reconnecting the original pad resumed it. Both human input channels then
independently opened and closed Pause. The completed match was recorded once,
not again merely by entering Rematch. No seat/device/rematch defect was reproduced.
The existing seven practice/bot/profile routes also ran in the same probe.

## Reproduced result ambiguity

A zero-painted draw showed A 50.0% / B 50.0% above two 0.0% player rows.
A later painted Crossfeed match showed B 100.0% above P2's 0.1% contribution.
These are not scoring bugs: team plates divide *painted surface*, while player
rows divide the *whole arena*. `LevelEndInfo::team_a_ratio/team_b_ratio` explicitly
return equal shares when both areas are zero. The contribution heading already
names its denominator; the team plates did not.

## Narrow change

Only competitive **single-match** Results gain an authored caption:

- `Team share of painted surface`
- With exactly zero paint from both teams: `No surface painted. Equal shares mean a draw.`

Scoring, percentages, rounding, records, awards, continuation timing and input
ownership are unchanged. Solo and series have no new caption. The accepted
post-reveal hidden bar and omitted repeated winner stay hidden/omitted.

The first proposed universal caption did not fit: existing series text ends at
reference y463 and the padded plates begin at 482, leaving 19px for a 38px caption.
That design was rejected rather than shrinking type or moving existing rows.
Single matches have no series ledger, so their caption uses its reserved band:
authored y405, unscaled existing contribution font, bottom 443 and 39px clearance
to the plates. No existing position changed.

Schema/resource tests bound the new caption to the reserved single-match ledger
band and paper. Historical winner full-paper ink bounds are unchanged, including
its existing 715px positive fixture; this change does not impose a new winner-font
contract. Independent runtime fixtures additionally assert nonoverlap for the
actual shipped English and pseudo-locale winner/caption copy.

## Qualification

- Release build and CPU `menu_text`, `menu_presentation`, `locales`, `paint`,
  `results_awards`, `match_records` passed.
- Actual Results matrix covers empty, painted tie, nonzero victory, solo and
  series; revealed/plain bars, delayed continuation, sparse input-owner credit,
  redraw/resize, reduced effects and 1024x768 through ultrawide/QHD.
- A separate literal text/paper raster oracle verifies exact caption copy,
  unscaled font, centering and visibility. Solo must match the empty reference.
- Empty and equal-painted results differ only inside the caption footprint;
  the zero-paint 0.5/0.5 convention is separately asserted unchanged.
- Pseudo-locale uses the same matrix. Its existing independent contribution-row
  reference was extended with literal bracketed device/team text, not widened
  to ignore arbitrary pixel differences.

RTL is not supported by this product: `docs/localisation.md` limits shipped
atlases/authored text to printable ASCII, with English and `qps-ploc`. No RTL
readability claim or unrelated font/shaping work is included.

Evidence is retained locally under `out/validation/local-duel*`,
`score-basis-*` and the final `score-final-*` runs. No release, deployment,
nightly dispatch, issue #88 closure or physical/performance acceptance.

Final-code verification completed:

- Debug and Release builds passed; all six CPU cases above passed in both.
- Native strict-hardware Results raster matrix passed Debug English, Release
  English and Release `qps-ploc`, including the new narrow empty-draw case.
- Release CRT-enabled first-match rehearsal passed all seven prior routes plus
  both human-vs-human routes, natural outcomes, rematch and reconnect checks.
- 62 CI-script unit tests and local Markdown link validation passed.
- Visual inspection confirmed readable English and pseudo captions at 1024x768,
  and the painted live-match caption with CRT enabled.

These are bounded local qualifications, not a claim that all CI or every
physical device, graphics backend, locale or accessibility technology was tested.
