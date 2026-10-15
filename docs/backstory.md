# ColourWars backstory

Status: decided on 2026-10-12. The art direction is Paint.exe Maximalism (crude,
mouse-drawn box-heads), and the theme is the one picked on the style board on
2026-10-09: a paint program, one empty palette slot, and the person at the
computer. This page is the backstory built on those three things.

Like Splatoon's, the fiction exists to explain the game. Every rule below
answers a question a player might ask about a mechanic.

## The premise

A picture is open in a paint program. Everything in it is grey, and the palette
has one empty slot.

While choosing a colour for that slot, the person at the computer tried two
colours, like tester patches on a wall: one previewed over the left half of the
picture, the other over the right half. The figures on one side flickered blue
and the figures on the other side flickered orange. Then the person undid both
and walked away.

Each side has now seen itself in colour, once, and can't forget it. Nobody is
wrong, because each side saw something different. They all know only one colour
can be saved. They believe that whichever colour covers more of the picture will
persuade the person to save it.

## How it explains the game

| Mechanic | In the story |
|---|---|
| The grey world, with no team hues in the scenery | Nothing has been coloured yet; the slot is still empty |
| The random colour pairs | The two swatches this person happened to hover over |
| Two teams | The two halves of the picture, each previewed in one colour |
| Covering ground | Persuading the person: more coverage, a stronger case |
| The hidden score | Nobody inside the picture can see all of it |
| The reveal | The person comes back and chooses |
| The rematch | A different person, somewhere else, with the same dilemma |

## A match, start to finish

### Opening (full screen, before the view splits)

A computer monitor's bezel fills the screen, probably a beige CRT. It is picked
at random each match from many variations: different shades, cracks, sticky
notes, stickers and coffee rings. Each one is a different place and a different
person, which is why every match starts with a fresh grey picture.

The intro plays the premise live, not as a memory. Every match is a different
person, so the person who previews and then leaves is this match's person,
happening now. The intro and the reveal mirror each other, with the same
silhouette at both ends.

1. **The bezel, full screen.** The person's silhouette sits in front. The
   figures are frozen.
2. **The person tries both colours.** The cursor previews one colour over the
   left half of the picture, then the other over the right half. This is also
   how players learn which colour is theirs, so the beat does a real job as well
   as telling the story. Each half lines up with that team's spawn.
3. **They undo it and leave.** The picture goes back to grey. The silhouette
   sighs, stands and walks out, and a chair creaks.
4. **The figures come alive.** They unfreeze, look at their hands, then at the
   other side. The existing 3-2-1 countdown runs over this.
5. **The camera pushes through the glass,** and split screen begins.

Keep it to about 6-8 seconds, because matches restart fast. It can shorten on
quick rematches by starting from the preview, and a button can skip it.

The bezel can hint at who the person is, so it should match the silhouette in
the reveal. A child's sticky notes go with a small silhouette; a coffee ring goes
with someone holding a mug. Assemble both from one kit of parts.

### Respawn

The source is the program's undo history. A splatted figure has been painted
over in the other side's colour, so it is gone from the picture, but the history
still holds the strokes the person used to draw it.

1. A **pop** as the paint comes off.
2. A fast-forward **redraw**, line by line (box, eyes, mouth, legs), at the spot
   where the person first drew that figure.

That spot is the team's spawn. The redraw's length is the respawn timer. Nobody
presses Ctrl+Z, because the person is away; the pop is a sound, not a keypress.

### The reveal (full screen)

1. When the clock runs out, split screen ends and the camera pulls back to the
   same bezel. Everyone watches one view of the whole picture, and for the first
   time they can see the coverage.
2. A silhouette of the person moves into the centre of the screen.
3. The figures freeze, as toys do when a person is watching, but they cheat:
   darting eyes, sweat drops, shaking knees.
4. The cursor hovers over each swatch in suspense. Each hover tints the whole
   picture in that colour, a callback to the opening preview. The intro tinted
   one half each because the person was trying; the reveal tints the whole
   picture because the person is deciding.
5. The cursor suddenly clicks the winner, and the coverage percentages appear.
6. The silhouette leans back and leaves. Only then do the winners celebrate and
   the losers slump.

### Rematch

The next match opens on a new random bezel: a different person somewhere else,
with their own grey picture and their own two swatches. Nothing anyone won is
undone.

## Why everyone draws the same box-heads

Every person in every match draws the same box-headed figures. It's how everyone
draws a person when they have a mouse and no talent. That is a joke to lean into,
not a hole to explain.

## Rules for anything new

- Keep the three fixed pieces: the paint program, the one empty slot, and the
  person shown as a cursor or silhouette.
- The drawings own the contest. A human never starts it, drives it or plays it
  through them; the human only chooses afterwards.
- The person is never seen clearly, only as a bezel, a cursor and a silhouette.
- Invent the program. Never name or trace a real one, or a real operating system.

## Building it

Decided with the author on 2026-10-12, while the first slice was built:

- **Figures are grey until they come alive.** They are grey in the opening,
  flash their colour during their half's preview, go grey again at the undo,
  and fill with the team colour when they come alive. They keep that colour
  for the rest of the match, because players tell the teams apart by it.
- **The redraw is the respawn timer.** No number is drawn while a player
  waits; the figure being redrawn shows how long is left.
- **Two-player split screen stays top/bottom.** The stages are wide bands, so
  left/right panes would be slivers. The preview shows each team its side
  before the split.
- **The picture is scaled down to fit the monitor's glass,** whatever its
  shape; nothing is cropped.
- **The silhouette is large, in the centre of the screen,** in front of the
  monitor, at both ends of the match.
- **Solo practice skips all of it.** No monitor, no person, no preview and no
  reveal: a practice match goes straight to the split-screen countdown and
  ends on the plain overview and the full results page.
- **The reveal has no tie branch of its own.** A dead heat (in practice, nobody painting) gives the person nothing
  to choose: they come back, stare at the empty picture, and close the program
  without saving. Nobody celebrates, and the results page says "Draw!".
- **After the reveal, the results screen keeps only the percentages.**
- **A, B or Start skips the intro** (Space, Escape or Enter on the keyboard).
- **A rematch may repeat the last colour pair.** The draw stays random.

| Beat | State |
|---|---|
| Respawn: the redraw at the spawn | Built. `RespawnRedraw`; strokes and timings in `presentation.json` `respawn_redraw` |
| Respawn: the pop | Built. The accepted `player.pop` plays when the paint comes off, through `player.sounds.death` in the replacement bank (`STORY#1`) |
| Grey scenery, each team on its own half | Built. Enforced by the game and the level editor |
| Opening: monitor, person, preview, undo, leaving, countdown in the picture, push through the glass | Built. `level_intro.cpp`; `Monitor`, `Silhouette`, `ScreenCursor`; timings in `tuning.json` `match.opening`, look in `presentation.json` `opening` |
| Opening: skip, quick rematch | Built. A, B or Start skips; a Start (Escape) pressed while the intro plays is a skip, never a pause, however late after GO it is released (`latch_intro_pause`); a rematch starts at the preview |
| Rematch: a new random monitor | Built. Rolled per match from `opening.bezel_shades` and the decorations |
| Opening: figures looking at their hands, then across at the other side | Built. `FigureActing` repaints the eyes the respawn strokes locate |
| Opening: sigh and chair creak | Built. Accepted `room.sigh` and `room.chair` cues, `match.sounds` slots and phase-edge play calls in `level_intro.cpp` (`STORY#2`) |
| Reveal: pull back into the monitor, the person sits, hovers each colour over the whole picture, wavers, clicks the winner, the slot fills, the coverage shows, the person leaves | Built. `level_intro.cpp`; timings in `tuning.json` `match.reveal`, sounds `match.sounds.hover/click`; the results page then keeps only the percentages |
| Reveal, nobody painted: the person stares, then closes the program instead of choosing | Built. The glass goes to `opening.desktop_colour` |
| Reveal: figures freezing but cheating (darting eyes, sweat, shaking knees), then celebrating or slumping once the person has gone | Built. `tuning.json` `match.reveal.react`, `presentation.json` `opening.acting` |

All story beats are built as of 2026-10-15. The replacement bank and data bindings
are recorded in [audio inventory §9](audio-inventory.md#9-runtime-migration-and-remaining-integration).
Actual listening, pause/resume and multiplayer mix acceptance remain `ROAD#3`;
the construction checks do not establish their audible timing or balance.

## Rejected on the way, and why

| Idea | Why it was dropped |
|---|---|
| Colour gods, an election, a union | Too much invented structure around a simple fight |
| Two siblings whose drawings fight for them | The contest belongs to the kids, so the drawings are only stand-ins, and players map onto "half a kid" |
| A drawn paint fight, or paintball | Paint fights aren't something people do; paintball means guns |
| Sides that disagree about what the picture is of (sea against sunset) | Ties colours to stages, which clashes with random pairs |
| Plain favourite colours | Explains the disagreement without making it interesting |
| Undo to grey, or Save As copies, for the rematch | Erases the win, or needs a reason to replay the same picture; the random bezel does neither |
