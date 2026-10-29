# Design process: personas, user stories and journeys

Opened **2026-10-29** at the author's request. ColourWars has grown by
features, polish slices and attended reviews. Its intended players and what
each needs from a session are spread across the polish plan, the feature
roadmap, the gap audit and the arena records. Nothing states them once and
traces the work to them. This doc owns that formal pass and the `DES#` rows.

## What exists to draw on

Persona and story evidence is already recorded, but none of it is written as
personas or stories:

- Couch play and its constraints: local split screen for 1-4 players, one
  mouse player at most, pads on the other seats, and bots on idle seats
  ([feature-roadmap.md](feature-roadmap.md), `ROAD#10`, `ROAD#24`).
- Fresh-player observation is already a gate: the finish review (`POL#24`)
  and the Movement Course practice workshop (`POL#19`).
- Comfort and accessibility: per-seat toggles (`ROAD#7`), handicaps
  (`ROAD#5`) and reduced effects ([polish-reduced-effects.md](polish-reduced-effects.md)).
- Stage authors: the editor and My Stages ([level-editor.md](level-editor.md),
  `ROAD#16`).
- Deliberate Splatoon borrowings, which a story must not report as gaps
  (gap audit 1.1 and 2.1).
- The game's second audience: Labrador developers reading it as the engine's
  full-OOP client (AGENTS.md, [oop-guide.md](oop-guide.md)).

## Deliverables

1. **Personas** (3-6). Each one gives the player's context (who is on the
   couch, which device, experience with shooters), goals, frustrations and
   what a good session looks like. Every claim cites the doc or code it rests
   on. A persona with no evidence is marked as a hypothesis.
2. **User stories** in "As a ..., I want ..., so that ..." form, grouped by
   journey stage. Each story carries acceptance criteria and the feature, row
   or acceptance case that serves it today, or "none".
3. **Journeys** for first launch to first match, a returning group's evening
   (series, rematch, settings), solo practice, and authoring and playing a
   user stage. Each step points to the screen or state that serves it.
4. **Traceability and gaps.** A story with no serving feature becomes a
   candidate row in its owner doc. A feature with no story is listed, not
   removed.

## What this does not decide

The personas and priorities are the author's to accept. The draft records
evidence and options, not a direction. The pass does not reorder NOW, change
tuning, reopen closed Splatoon borrowings or close any attended gate.

## Draft personas, stories and journeys (`DES#1`)

**Open, not started.** Draft the deliverables 1-3 above in this doc. First
step: inventory the existing evidence listed above into a source table, then
write the personas from it. Acceptance: every persona and story cites a source
or is marked as a hypothesis, and every story has acceptance criteria and a
mapping (or "none"). The author's review follows as a separate `DECISION`
once the draft exists.

## Traceability and gap pass (`DES#2`)

**Blocked on `DES#1`.** Map the accepted stories to features, TODO rows and
`scripts/acceptance/` cases. File each unserved story as a candidate in the
owner doc it belongs to, with its own row. List features that no story
explains. Acceptance: one table covers every story, and each new candidate
follows TODO.md rule 4.
