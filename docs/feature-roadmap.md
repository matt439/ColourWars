# Feature roadmap

## First increment: explicit joining and reconnect pause

Implemented:

- Player count leads to Join Players. The selecting device claims P1; other
  devices explicitly claim remaining seats. P1 confirms the completed roster.
- A device cannot claim two players, and joining stops at the selected count
  (at most four). Leaving frees a seat without renumbering other players.
- Setup and gameplay show player number and device. Gameplay identity has a
  dark backing to stay readable over the level art.
- A participating controller disconnect pauses the simulation and match audio.
  The pause menu names missing players and disables Resume and Restart until
  all return. Reconnection requires an explicit resume; spare devices cannot
  take over a player. If all participants disconnect, any available device can
  navigate Quit and its confirmation.

Identity is bound to an XInput slot for the match. A controller returning in a
different slot requires quitting and joining again. Physical-device identity
and reassignment during a match are follow-up work if this proves inconvenient.

Validation: both normal Debug and Release builds; a temporary native roster
probe for duplicate claims, capacity, stable seats, reset and labels; and a
temporary build using scripted gamepad states at the existing reader seam for
join/leave, reserved disconnected seats, two- and four-player setup, HUD identity,
disconnect pause, explicit resume and the all-disconnected quit path. The
engine pin and engine source are unchanged. Physical controller unplug/replug
and audible suspension still need a hardware check.

## Next increments

1. Accessibility: retain randomized palettes by default; add a saved fixed
   palette option and a non-colour team identifier in gameplay and results.
   Validate both teams across all palettes, HUDs and split-screen layouts.
2. Pause controls/settings: persistent controls reference, aim sensitivity and
   separate music/SFX volume, shared with main-menu settings and saved between
   launches. Verify settings affect current play and survive a restart.
3. Sessions: best-of rounds, cumulative wins and per-player paint contribution
   on results. Define contribution ownership and repaint accounting before
   changing scoring; preserve a standalone match and solo practice.
4. Release package: an explicit redistributable asset list, required runtime
   DLLs and a visible build identifier. Validate an extracted package outside
   the checkout, without the local commercial wave bank.
