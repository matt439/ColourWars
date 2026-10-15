# Reduced gameplay effects

Implemented 2026-10-15, completing `POL#13`. **Reduce effects** is a shared,
saved On/Off preference in **Controls / audio**, reachable from Options and
Pause. It defaults to Off, preserving the authored presentation. Changes take
effect on Apply; Back discards pending changes. Older saves and missing or
invalid values use the default independently of other preferences.

## Presentation policy

With Reduce effects On:

- The world camera stays still. The separate View shake preference is retained
  and applies again when Reduce effects is Off.
- Paid shots keep their projectile, tool mechanism and firing pose, while the
  extra muzzle burst is omitted. Paint recovery keeps its team-colour bead
  without the two white rays; the dry cross and aim/readiness shapes remain.
- Quiet breathing, blinking, weight shifts and acceleration lean are suppressed.
  Action poses, hands, team masks and teammate head accents remain.
- Idle sprayer beads/drops and detached mister release haze are hidden. Active
  reduced updates retire old drips and reset quiet posing; restoring detail
  waits for fresh settling rather than replaying an interrupted bead.
- Countdown pulse and detached countdown, cursor-click, closing-clock and
  reveal-coverage ticks are suppressed. Their settled signals and readable
  coverage cards remain visible at the existing match/story boundaries.

Movement contacts, hurt ticks, elimination bursts, respawn redraw, blocked-shot
contacts, actual paint splashes, wet paint, ownership/keylines and the HUD remain
visible. These describe an action, contact or state even with sound muted.
Tool pressure recovery, roller contact and bucket motion retain their physical
explanation. There are no full-screen flashes in this treatment.

Draw paths read the applied preference immediately, including while paused,
without advancing timers. Suppressed firing and mist events still expire on
ordinary active ticks. Physics, aiming, ammunition, health, scoring, input and
sound are unchanged. The option is separate from the developer feedback switch,
which is an authoring comparison rather than a saved player preference.

## Repeated sounds

The other half of `POL#13` was already implemented by the
[replacement-audio integration](audio-inventory.md#9-runtime-migration-and-remaining-integration).
`MovementAudio` cycles six dry footsteps, four landings and four wet-contact
variants with independent seat phases, distance thresholds and repetition limits.
`PaintAudio` cycles six splashes per owner under shared impact/capture budgets.
Reduce effects leaves these informative sounds and their existing volume controls
available. No bank regeneration is required.

## Validation

Debug and Release build and pass all 20 consumer cases. Isolated Debug and
extracted Release startup pass with the accepted bank, a system-only PATH and
graceful shutdown. The 53-file Release ZIP contains all 26 byte-matching runtime
assets, the accepted bank and provenance. The extracted copy also starts without
the bank and reports its expected single silent-fallback diagnostic; its bank
is then restored. The player's real save is never opened by these checks.

The retained consumer cases cover default/type recovery, boolean persistence and
failed-save recovery; effective shake policy and live damage/death transitions;
immediate quiet-pose suppression and fresh settling; drip removal and fresh wait;
and five-tool firing/status timing and projectile equivalence. Four-player Level
checks retain identical movement, objects, health and paint ownership/score with
the preference On and Off.

The D3D11 painter probe compares normal, reduced and restored frames at native
720p in one and four panes, with real paid shots and mature idle detail. Frozen
redraws remain identical and preference changes preserve the match observations
and Debug replay fingerprint. Isolated dry crosses remain byte-identical;
refill beads retain visible ink with fewer pixels after their rays are removed.
Actual ready/cooldown markers stay unchanged, and a released mister renders
exactly its existing tool mechanism when the cosmetic tail is hidden.
Controls / audio is also checked through actual
Options and Pause navigation, Apply, Back and discard with an isolated save.

The retained [native gameplay crops](images/polish-reduced-effects/gameplay-720p.png)
compare the same frozen frame with the preference Off and On. The
[720p controls screen](images/polish-reduced-effects/controls-720p.png) records
the sixth setting, Apply/Back and status fitting above the footer. These use
the Movement Course, keyboard/mouse roster, Blue/Orange palette, unchanged
gameplay tuning and the pinned D3D11 engine; the crops show four mixed aimed
tools and a quiet sprayer. The broader painter matrix retains every palette,
one through four views, both facings and 720p/1080p/QHD checks.

Evidence is under `out/validation/reduced-effects/` and `out/acceptance/`.
The final painter runs are Debug `9f2fa97fd3664285803359b03cf919ab` and Release
`908887e03b284de9a51456b520e619ac`.
The reference-arena composition review, repeated-match comfort observations,
physical controllers and audible mix acceptance keep their existing owners.
