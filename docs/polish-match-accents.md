# Paint.exe closing and reveal accents

Implemented 2026-10-15 as part of `POL#16`. During the final ten positive
displayed seconds of live play, the existing clock gets stepped pale paper, a black keyline,
quiet lower/right shade and four brief detached pen ticks. The clock retains
its font size, position and ceiling-based `m:ss` value. Ordinary clock drawing
is unchanged. The response follows `ceil(remaining) - remaining`, so it shares
the existing last-ten audio boundary without adding sounds or state. A long
tick shows only the current second; pause and nonpositive updates cannot age it.
Short matches retain the ordinary clock during the initial countdown and begin
the closing treatment when GO releases play.

After the person's reveal click, the existing `A x.x%` and `B x.x%` values
appear on two matching paper cards. Ink outlines keep the actual team colours
legible, including yellow. Each card fits its text to its picture half, reserves
its shade and ticks inside that half, and leaves the whole paint job visible
around it. The exact percentage strings and coverage calculation are unchanged.
The reveal response derives from its existing click age and settles after
0.18 seconds. Coverage remains hidden during the pullback, person arrival,
both hovers and hesitation. A draw still closes the program; solo practice
retains its plain overview and full results. There is no extra inspection step.

Reduce effects suppresses the detached clock and coverage ticks while keeping
the settled, readable cards. Drawing never advances timers or consumes random
numbers. Existing countdown, time-up, click, closing and celebration sounds,
control release, story acting and result-page policy retain their behavior.

`game/objects/interface_gameplay.cpp` owns the closing clock drawing;
`game/objects/reveal_art.cpp` owns the coverage artwork. Both reuse the solid
texel and existing fonts. The editable values are
`presentation.json.hud.closing_clock` (HUD reference pixels and seconds) and
`opening.reveal_art` (fractions of the supplied card height and seconds).
Startup checks required fields, types, colours and bounded measurements. Text
padding also clears the widest accepted stepped corner and its ink outline.
The replaced `opening.percent_shadow` setting has been removed.

## Validation

Rerun the focused hardware renderer checks with:

```powershell
scripts/check-painter.ps1 -Preset x64-debug -ClosingOnly
scripts/check-painter.ps1 -Preset x64-release -ClosingOnly
```

Debug and Release build under `/W4 /WX` and each pass all 21 consumer acceptance
cases. New content checks reject missing/wrong-type sections and fields, invalid
colours, out-of-range dimensions/timing and invisible clock ink, and retain the
accepted limits. Existing story/audio checks continue to cover final-ten/time-up
cues, suspension, skipping, winner/draw/solo policy and frozen gameplay before GO.

The hardware D3D11 matrix on the NVIDIA GeForce RTX 5080 covers:

- Closing clocks in one, two and four panes at 720p, plus four panes at 1080p
  and QHD. Initial/early beats, settled values, pre-closing values and zero
  verify both card visibility and local tick bounds. Short-match countdown
  fixtures keep the ordinary clock until live play.
- Native A/B coverage cards in all three palettes at 720p and representative
  1080p/QHD sizes. Animated pixels stay inside the supplied card bounds;
  reduced, negative and expired beat ages retain identical settled cards.
  The widest accepted pen with minimum padding also fits and was inspected.
- Real two-player A/B winners, four-player no-paint draw and solo finish.
  Same-state artwork comparisons bracket the actual reveal click. Repeated
  frozen draws preserve pixels, public state, draw RNG and Debug replay
  fingerprints. The existing full monitor matrix also passes in Debug.

The retained [four-player 720p clock crop](images/polish-match-accents/clock-4p-720p.png)
and [720p reveal](images/polish-match-accents/reveal-720p.png) are native-size
review examples. This drawing and raster verification does not establish human
readability, movement, reference-arena composition, pad or listening acceptance.
Those remain in their existing TODO rows. The clock highlight closes that half
of `GAP#5.4`; its fixed-step FPS display remains open.

The 53-file Release ZIP byte-matches all 26 reviewed runtime assets, including
the revised presentation and unchanged accepted sound bank, and contains no
authoring/probe files. Isolated Debug and extracted Release startup pass with
the bank and a system-only PATH. A separate extracted copy passes the absent-bank
fallback with its single expected diagnostic. All startup runs close gracefully.

Evidence is under `out/validation/closing/`, with final build/consumer logs at
`out/validation/closing-{build,acceptance}-x64-{debug,release}.log` and consumer
case results in `out/acceptance/<preset>/`. Renderer receipts identify baseline
`6452c98` plus working changes, the pinned engine, device and staged content.
Menus, cards, stage preview, the remaining HUD, pause, results and accepted-arena
treatment remain open under `POL#16`.

| Preset | Focused renderer run |
|---|---|
| Debug | `62cb56de0e3e4e248bf76d5c0e7c7e8d` |
| Release | `6e2c16038bac48ac8dc1b72a8f72e343` |
