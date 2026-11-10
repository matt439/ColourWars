# Paint.exe closing and reveal accents

Implemented 2026-10-15 as part of `POL#16`. During the final ten positive
displayed seconds of live play, the existing clock gets stepped pale paper, a black keyline,
quiet lower/right shade and four brief detached pen ticks. The clock retains
its font size, position and ceiling-based `m:ss` value. Ordinary clock drawing
is unchanged. The response follows `ceil(remaining) - remaining`, so it shares
the existing last-ten audio boundary without adding sounds or state. A long
tick shows only the current second; pause and nonpositive updates cannot age it.
The clock is hidden while the start countdown shows the stage title, whose card
would otherwise cover it, and appears at GO; short matches begin the closing
treatment then.

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
Of those, on 2026-11-05 the author accepted the arenas and movement at their current
values and waived the attended human-readability, physical-pad and listening
checks (`POL#24`, the pad sessions, `ROAD#3`), which were not performed
([polish plan](polish-plan.md)). The clock highlight closes that half
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

## Paint strip weight in the reveal (`REV#38`)

Captured and measured 2026-11-10. The project review estimated, by arithmetic,
that paint strips come to 2-3 px in the pulled-back reveal, and noted that
[the retained 720p reveal](images/polish-match-accents/reveal-720p.png) has
almost no paint in it. These are the first heavily painted four-player reveals:
a full 240-second round, Debug build, D3D11 on MattPC's RTX 5080, on Crossfeed
and on Close Quarters (the widest Random arena), each with seed 7
(`purple_gold`) and seed 0 (`blue_yellow`), at 1280x720 and 1920x1080.

There is no window-size launch argument; the size came from the `resolution`
of a save in an isolated profile (`LOCALAPPDATA` set for the game process
only), with the palette left on Random so `seed` picks the pair. Each run was:

```
ColourWars.exe +script.background 1 +screenshot.dir <dir> +screenshot.strip off +overlay none
  +seed <7|0> +bot 1 paint +players 4 <roster> +stage <stage> +timescale 4
  +wait match +wait 239s +checksum
  +wait 4.9s +screenshot hover-a +wait 1s +screenshot hover-b +wait 0.85s +screenshot hesitate
  +wait 1.35s +screenshot cards +wait 2.8s +screenshot alone +quit
```

The waits are match seconds, which keep counting through the two-second
pullback and the reveal, so the five shots land on the first hover, the second
hover, the hesitation (no tint, no cards), the cards, and the cards after the
person has left. Nothing was tuned and `ammo` stayed normal. On Crossfeed
`<roster>` was `paint` (four paint bots). On Close Quarters four paint bots
covered only 55% of the paintable surface, so seats 2-4 were Hard shipping bots
instead (`+seat 2 bot hard +seat 3 bot hard +seat 4 bot hard`). A seed gives the
same checksum at both resolutions, so each 720p and 1080p pair is the same match.

The table measures the hesitation frame, the one where the paint has no help.
The picture is drawn unfiltered, so paint pixels match the team colours exactly;
the weight is the run length across each strip, counted on strips at least
12 px long. Scale is the arena's drawn interior width over its world width.
Painted is both teams' share of the paintable surface at 239 s (`checksum`);
the result is what the cards showed.

| Stage | Pair | Size | Scale (px per unit) | Floor and ceiling strips | Wall strips | Painted | Cards | Frame |
|---|---|---|---|---|---|---|---|---|
| Crossfeed | purple / gold | 720p | 0.265 | 2-3 px (mostly 3) | 2-3 px | 95% | A 45.8%, B 54.1% | [image](images/polish-match-accents/reveal-4p-crossfeed-purple-gold-720p.png) |
| Crossfeed | purple / gold | 1080p | 0.398 | 4 px | 4 px | 95% | same match | [image](images/polish-match-accents/reveal-4p-crossfeed-purple-gold-1080p.png) |
| Crossfeed | blue / yellow | 720p | 0.265 | 2-3 px (mostly 3) | 2-3 px | 97% | A 27.9%, B 72.0% | [image](images/polish-match-accents/reveal-4p-crossfeed-blue-yellow-720p.png) |
| Crossfeed | blue / yellow | 1080p | 0.398 | 4 px | 4 px | 97% | same match | [image](images/polish-match-accents/reveal-4p-crossfeed-blue-yellow-1080p.png) |
| Close Quarters | purple / gold | 720p | 0.162 | 1-2 px (about half each) | none 12 px long | 88% | A 34.6%, B 65.3% | [image](images/polish-match-accents/reveal-4p-close-quarters-purple-gold-720p.png) |
| Close Quarters | purple / gold | 1080p | 0.244 | 2-3 px (about half each) | 2-3 px | 88% | same match | [image](images/polish-match-accents/reveal-4p-close-quarters-purple-gold-1080p.png) |
| Close Quarters | blue / yellow | 720p | 0.162 | 1-2 px (about half each) | none 12 px long | 95% | A 17.0%, B 82.9% | [image](images/polish-match-accents/reveal-4p-close-quarters-blue-yellow-720p.png) |
| Close Quarters | blue / yellow | 1080p | 0.244 | 2-3 px (about half each) | 2-3 px | 95% | same match | [image](images/polish-match-accents/reveal-4p-close-quarters-blue-yellow-1080p.png) |

The coloured band is a little thinner than the review's arithmetic. A strip is
`paint.thickness` 12 units deep, but `PaintTile::draw_surface`
(`game/objects/paint_tile.cpp`) draws a black inward edge `edge_width` deep
over the tint, which leaves 10 units of colour. That is
2.65, 3.98, 1.62 and 2.44 px at the four scales above and is what the pixels
show. [Enlarged crops](images/polish-match-accents/reveal-4p-strip-crops.png)
of one block and one slab at each size confirm the counts.

What the frames show, as judged by the session that captured them:

- **Crossfeed.** Strips read as coloured outlines on every block and floor at
  both sizes. That paint is nearly everywhere, and which colour holds a given
  ledge, can be seen. Which team holds more cannot: 45.8 against 54.1 looks
  like an even mix, and even 27.9 against 72.0 reads as "more yellow" only on
  inspection. Yellow on the pale sky and floor is weaker than blue, purple or
  gold at 720p, but present.
- **Close Quarters.** At 720p the strips are one or two pixels: hairlines, as
  the review said. Only the centre floor and three slabs are paintable, about
  half the arena's width, so the picture is mostly unpainted grey however the
  round went. At 1080p the lines are easier to find but still lines.
- **The person.** The silhouette covers the middle of the picture through both
  hovers, the hesitation and the first 2.8 seconds of the cards: about a sixth
  of Crossfeed's width, and about a third of Close Quarters' paintable span.
  The paint under it is seen only once the person leaves
  ([Close Quarters, cards, person gone](images/polish-match-accents/reveal-4p-close-quarters-purple-gold-cards-720p.png)).
- **Hovers and cards.** A hover tints the whole picture one colour
  ([yellow hover on Crossfeed](images/polish-match-accents/reveal-4p-crossfeed-blue-yellow-hover-720p.png));
  it carries the story beat and says nothing about who painted what. The cards
  are large, keylined and unmistakable in all four matches.

So the strips are not clearly legible as a result. They show that paint is
there and whose it is ledge by ledge; the verdict rests on the cards, as the
review supposed, and on Close Quarters at 720p it rests on them alone.

This does not establish human readability: no person reviewed these frames at a
normal viewing distance. Bot paint is not human paint (a 95% painted Crossfeed
is more than a played round is likely to leave), there is one seed per pair,
and only two of the four window sizes were captured.

A reveal-only minimum weight or body tint was not built. It is the author's
call, and the objection in
[the record](review/2026-11-10-project-review.md#paint-strips-are-hairline-thin-in-the-reveal-rev38)
stands: the reveal would then show a picture that differs from the one fought
over.
