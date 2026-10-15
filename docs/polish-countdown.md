# Paint.exe countdown artwork

Implemented 2026-10-15 as part of `POL#16`, following the monitor/person/cursor
scene. The shared start signal now uses editable coarse-cell number and GO!
strokes with a dark keyline, pale number fill, warm yellow GO and an offset grey
shade. The countdown card has stepped corners, a paper face and restrained
lower/right airbrush shade. GET READY and the device-specific controls reminders
remain readable on their existing fonts.

The drawing is in `game/objects/countdown_art.cpp`, using the existing solid
texel. `presentation.json.match.countdown_art` owns its colours, card keyline
and shadow measurements in match reference pixels, beat duration in seconds and
pulse fraction. Startup bounds these values and identifies invalid content.
The obsolete `tuning.json.match.countdown_text` font/colour/scale settings have
been removed. Countdown duration and audio remain in tuning.

Each number and GO briefly settles from a slightly larger drawing while four
detached pen ticks contract. The response lasts 0.18 seconds by default and is
derived entirely from the existing countdown timer. Reduce effects suppresses
the pulse and ticks while retaining the settled signal and contrast. Drawing
does not advance state or consume random numbers. Number, keyline, shade and
ticks fit inside the supplied number box, including during the pulse.

This remains one fullscreen signal shared by every pane. Full and quick story
openings use it during the existing monitor push; skipped openings and solo
practice use the split-screen countdown. The controls cards clear when GO
releases movement. GO stays for its established one-second beat. There are no
new sound triggers, gameplay delays or solo story elements. Multi-digit
countdowns fit the same box; the initial label now uses the same ceiling rule
as subsequent ticks for a fractional authored duration.

## Validation

The focused renderer check is:

```powershell
scripts/check-painter.ps1 -Preset x64-debug -CountdownOnly
scripts/check-painter.ps1 -Preset x64-release -CountdownOnly
```

Debug and Release build under `/W4 /WX` and each pass all 21 consumer acceptance
cases. The countdown-content checks cover numeric limits, missing fields,
wrong types, invalid colours and accepted boundary values. Existing story/audio
acceptance continues to check countdown/GO events, sound suspension, skipping,
solo policy and the frozen world before GO.

The focused hardware D3D11 matrix covers:

- Distinct 0–9 and GO glyph rasters at native 720p, with representative 1080p
  and QHD checks. Settled and reduced/negative/expired beat ages agree; animated
  and settled raster differences stay inside the number box.
- Real solo countdowns at all three sizes and mixed keyboard/sparse-pad two-
  and four-player matches across off, quick and full openings. Reminder cards
  fit the display; resizing away and back restores the exact frozen image.
- Paused and zero-time updates, the before/after-GO control-release boundary,
  GO tail and expiry. Same-state art suppression confirms GO is visible through
  its tail and absent after one second. Repeated frozen draws preserve pixels,
  public simulation and cosmetic state, draw RNG and the Debug replay fingerprint.

Native review examples are the [720p countdown](images/polish-countdown/countdown-720p.png)
and [two-player GO](images/polish-countdown/go-720p.png). These establish the
drawing and event placement; movement, reference-arena composition, physical-pad
and human readability acceptance remain in their existing TODO rows.

The 53-file Release ZIP byte-matches the revised presentation and tuning data
and the unchanged accepted sound bank, and contains no authoring/probe files.
Isolated Debug and extracted Release startup pass with the bank and a
system-only PATH. Extracted Release separately passes the absent-bank fallback
with its single expected diagnostic. All startup runs close gracefully.

Evidence is under `out/validation/countdown/`, with build/consumer logs at
`out/validation/countdown-{build,acceptance}-{debug,release}.log` and consumer
case results under `out/acceptance/<preset>/`. The renderer receipts identify
baseline `33c058d` plus working changes, the pinned engine, device and exact
staged content. Closing/reveal accents and the remaining interface treatment
stay open under `POL#16`.

| Preset | Focused renderer run |
|---|---|
| Debug | `0101e70df1374ac498dae730810fb25f` |
| Release | `de70aae8397d4d568b5abc0829481b13` |
