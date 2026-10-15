# Paint.exe pause and confirmation pages

Implemented on 2026-10-15, part of `POL#16`. The pause page and its Restart/Quit
questions now use the same pale stepped paper, dark keyline, soft edge shade
and ink lettering as the live HUD and end menu. A detached stepped arrow and
a separate card identify the enabled shared focus. The drawing stays still,
including under Reduce effects.

The page centers its heading, connection status and confirmation question from
the actual font measurements. Missing pads retain their roster/device labels;
Resume and Restart use distinct disabled ink while the status explains what
they are waiting for. The shared brand line is hidden above the paused match.
The existing keyboard/pad prompts remain below the page.

`MenuPaperArtwork` in `game/states/menu_panel.cpp` draws the paper from the
existing solid texel. It is shared with the end menu, which retains its
persistent primary continuation card. Pause passes no persistent primary;
its card and arrow follow only an enabled focus. The editable colours, pen,
shade, padding and placement live in `presentation.json.pause_menu`.
No atlas or authoring tool is required for normal builds.

Input ownership, fallback navigation, audio cues and typed actions retain
their existing behavior. Resume and Back/Escape keep the shared frozen-world
count-in. Missing participants still prevent Resume and Restart, returning a
pad still requires explicit Resume, and Restart/Quit still ask a question
with **No** focused first. Controls/audio opens its existing settings page
and returns to the same pause cursor when closed without applying.

## Validation

Debug and Release build under `/W4 /WX` and each pass all 22 consumer
acceptance cases. Content variants cover the required pause drawing fields,
positive bounded geometry, padding/keyline relationships, heading/status
ordering and opaque, distinct disabled ink. Shared paper validation retains
the end menu's existing acceptance coverage.

The D3D11 hardware probe passes in both configurations on the NVIDIA GeForce
RTX 5080. It drives the actual pause and confirmation pages through keyboard
input and `StateContext`, checking:

- 720p, 1080p, QHD, 4:3, 16:10 and ultrawide placement; measured text/card
  separation and paper bounds, including the four-missing-pad roster.
- Resume first, safe No for both questions, deliberate Yes returning the
  correct Restart/Quit action, Escape cancellation and Controls/audio return.
- Missing-participant Resume/Restart refusal, focus skipping disabled rows,
  reachable Controls/Quit, and keyboard rescue when all four pads are absent.
- Visible focus-card fill and a detached ink arrow, static Reduce effects,
  repeated frozen drawing without random-number consumption, and exact resize
  round trips.

The retained [ordinary pause](images/polish-pause-menu/pause-720p.png),
[Restart question](images/polish-pause-menu/restart-720p.png) and
[four missing pads](images/polish-pause-menu/missing-pads-720p.png) were inspected
at native 720p. The page matrix uses a plain backing to isolate the drawing;
the separate live-match regression drives real `GameLevel` pause/resume in
1/2/4-player layouts in both builds. It passes the existing frozen world/HUD,
input rearming, focus return, count-in, Restart and Quit checks. The end-menu
single/continuing/drawn/completed-series hardware matrix also passes in both
builds after the drawing was shared.

The 53-file Release ZIP contains all 26 reviewed runtime assets byte for byte,
including the unchanged accepted sound bank and provenance. Isolated Debug
and extracted Release startup pass with a system-only PATH and graceful
shutdown. A separate extracted copy passes without its optional bank, with
the expected single silent-fallback diagnostic. No player's save is opened.

Evidence is under `out/acceptance/` and `out/validation/pause-menu/`, with
build/acceptance/visual logs in `out/validation/pause-menu-*.log`. Successful
page runs are Debug `1d5272c8fa9047ce9806dffbaa00e981` and Release
`92dfa455df38432d84360c0e4d9414d5`. Live-match regression evidence is in
`regression-x64-debug-0c1b244d426d4addaed2b364781fd81f` and
`regression-x64-release-e91372776e1a437296e55747a433664a` below that evidence
root. Package/startup evidence is in
`package-0da5df0ac32c4eaab0a14a1c2420a8b5/`.

Physical reconnection, movement feel, human readability and listening acceptance
remain with their existing TODO owners.

Rerun from an x64 Visual Studio Developer shell:

```powershell
scripts/run-acceptance.ps1 -Preset x64-debug
scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -PauseMenuOnly
scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -ResumeOnly
scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -EndMenuOnly
```

Use `x64-release` for the Release checks. Remaining menus, player cards, stage
preview and results stay open under `POL#16`; this increment does not accept
the reference arena or close the physical-pad and clean-PC checks.
