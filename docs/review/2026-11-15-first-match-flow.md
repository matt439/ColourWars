# Fresh profile to practice, results and rematch (2026-11-15)

## What was actually rehearsed

The hidden/silent Release D3D11 hardware probe runs production `GameMenu`,
setup pages and `GameLevel`, using the real manifest catalog and a fresh
isolated save for each route. Only owned keyboard/synthetic-controller snapshots
and silent/reduced-motion preferences are supplied. No setup injection,
developer finish command, duration override or physical-controller claim.

Both keyboard and synthetic-controller hosts completed:

- Single Match > 1 Player > Movement Course: real lesson/practice runtime,
  still running after 250 simulated seconds, Pause and Back/resume.
- Single Match > 2 Players > fill the spare seat with a normal bot > explicitly
  choose/ready both teams and both tools/subs > Movement Course: normal
  240-second timer to actual Results, End menu, Rematch and live next match.
  Exactly one completed versus record exists before the rematch ends.

Simulation time advances at the normal 1/60 step; this is not a four-minute
wall-clock or physical-play feel/performance test. This uses a native acceptance
executable linked to production game code, not OS-driven interaction with the
standalone game window. The optional #92 guide was tested separately; these
counts take Play directly and exclude reading it.

## Measured action counts

The pre-fix successful route traces, identical for keyboard and synthetic pad:

| Route | Confirmations from Title to launch | Directions | Optional narrative skip |
|---|---:|---:|---:|
| Solo Movement Course | 9 | 5 | 1 |
| One human + one bot local match | 13 | 11 | 1 |

Directions include five Right actions from Random to Movement Course in the
shipped manifest order. Counts include explicit bot team/tool/sub readiness.
From settled Results, one confirm opens End and one chooses Rematch. A further
optional confirm skips the rematch narrative. A fresh versus run therefore
reaches live rematch with 17 confirmations when both narratives are skipped.
The correction below does not remove or bypass any setup step.

## Reproduced defect and correction

Ready on a neutral/unselected Team Select row was a silent no-op. A fresh host
could repeatedly confirm with no explanation, including while configuring a
bot. It now says **Choose a team with Left/Right, then Ready.** and uses the
existing refusal sound. It never selects a team, marks a row ready or saves a
match. Repeated refused Ready keeps the same pixels/state. Selecting valid teams
clears the hint and the normal path proceeds. The separate both-teams-required
warning is restored for an invalid same-team roster; its gate is unchanged.

The final flow probe deliberately adds two refused confirms before recovering,
so its versus trace totals 19 rather than 17. Those are explicit failure/retry
coverage, not a newly required step. Authored copy has resource bounds validation.

## Validation

- Release build.
- `check-painter.ps1 -Preset x64-release -FirstMatchFlowOnly`: all four fresh
  routes above, captures and per-phase cumulative action CSVs; refusal/repeat
  feedback and valid recovery; normal Results/Rematch and record count.
- Existing `-TeamSetupOnly`: held-entry, conflicting input, controller-loss and
  focus gates, manual edits, shuffle, solo no-op, actual 2P Restart and two
  best-of-three rounds through Results/End.
- `run-acceptance.ps1 -Preset x64-release -Cases menu_text,team_selection`.
- Native screenshots inspected, including the new warning at 720p.

Raw evidence remains outside Git under the dedicated worktree's `out/validation`.
No release, deployment, physical-controller acceptance or #88/nightly change.

## Next meaningful UX bet

The measured bottleneck is now concrete: hosts configure and ready a bot's team,
tool and sub as well as their own, but progress is mainly communicated by row
highlighting. A per-row textual status (choose team / choose tool / choose sub /
ready) and a clear host-next-step line could make the existing choices legible
without introducing Quick Play or choosing gameplay options for the user.
