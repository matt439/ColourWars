# Reports to file against Labrador

> The running list of API complaints this client owes the engine. Each one is a
> case where the game wanted something ordinary and the API forced a bad
> answer, and each carries an **exhibit**: the workaround that is in this
> repository, in a file you can open, written because the engine offered
> nothing better.
>
> **One is live and on hold. Seven are closed** and kept below as one paragraph
> each — what was asked, what shipped, and what this repository spent the
> answer on.
>
> They are drafted here rather than in the submodule on purpose. CLAUDE.md puts
> engine *changes* in the Labrador repository; a report is a thing this client
> owes it, and this is the client.

**Audited 2026-08-15 against `6b5c0bd`.** What that removed and what it turned
up is at the bottom, under [What the audit changed](#what-the-audit-changed).
Reports 7 and 8 were filed off the audited file and answered the same day, in
`343433c` and `f94ac81`; the pin is at `343433c`.

## The rule that governs this file

**Check the engine's master before filing, not the pin.** `external/labrador`
is pinned to a revision; the engine is not. Report 1 was drafted against the
pin and described a bug the engine had fixed the day after that pin was taken —
four days before anyone wrote the report down. Every citation below was checked
against `6b5c0bd`, and `6b5c0bd` will not be master for long.

## Why the exhibits matter more than the asks

`PHILOSOPHY.md` declines speculative frameworks, and the engine has been
consistent about it: `gamepad.h` says outright that there is no action map
because "the paint-shooter and the sample both spell their bindings in code,
and neither has a rebinding screen to fill such a table from". That is the
right call, and it means a report saying "an engine should have X" is worth
nothing. A report saying "here is the code I wrote because it did not" is worth
something.

So each report leads with the workaround. The evidence for this being the right
shape is the strike rate: **of the seven reports filed with exhibits, six were
implemented the day they were filed.** The seventh needed no filing — the
engine had already fixed it.

The one report with no exhibit is the one that has never been filed.

---

# Live

| # | Report | Exhibit | Status |
|---|---|---|---|
| 6 | No audio bus, master gain, listener or voice cap | **not yet** | hold — the exhibit has to exist first |

Nothing else is outstanding against the engine. What *is* outstanding is on
this side: the fullscreen workaround under
[What is deliberately not here](#what-is-deliberately-not-here), which needs a
pad to check.

---

## 6. No audio bus, master gain, listener or voice cap — **hold until the workaround exists**

**Do not file this yet.** Everything below is true and re-verified against
`6b5c0bd`, but filing it before the client-side mix is written throws away the
exhibit that would make it persuasive. The engine has been consistent about
declining features no client has demonstrated wanting; a report with 74
hand-written multiplications attached is a different document from a report
without them.

### What happens

`SoundBank::play_wave` and `play_effect` take a volume **per call**
(`sound_bank.h:66,72`) and the class carries no gain. `AudioResources` is a bank
registry with no gain either. DirectXTK's `AudioEngine` — which has
`SetMasterVolume` — is a private member of `Application` (`application.h:236`)
with no accessor among its services. So a client can only multiply a gain into
every emitter, cannot touch a fire-and-forget wave it holds no handle to, and
cannot reach the one object that could do it globally.

Two further halves only this client can raise:

- **No listener concept.** Four split-screen views, and every call passes pan 0.
  The API models one global mix in a game that is four people on a sofa.
- **No voice budget.** The sprayer's `shoot_interval` is **0.0** and the
  roller's is 0.0001, so four players can trigger the same splash wave hundreds
  of times a second over four looping weapon voices. The only thing anywhere
  that rate-limits a *sound* is a hand-written `damage_sound_delay` of 0.25 on
  one wave.

### The ask, when it goes

`AudioResources::set_master_volume`, or better a `SoundCategory` bus table;
plus a listener concept and a per-bus voice cap. The bus half is already on the
engine's own review list; the multi-listener and voice-budget halves are not,
and are the two only a split-screen client would ever report.

Report 4's closure means the volume row now has somewhere to be drawn —
`OptionRow` exists — so the thing blocking this report is no longer blocked
itself.

---

# Closed

Seven reports, all answered, all spent here. Kept as outcomes rather than as
reports: what was asked, what shipped, what this repository did with it. The
original bodies are in git — see [What the audit changed](#what-the-audit-changed)
for reports 1–5, and `f81e937` for 7 and 8.

## 1. ~~`StateContext` never empties its stack~~

**Never filed. The engine had already fixed it** in `881d3c7`, "Drain the state
stack before the services it borrows", the day after the pin this report was
checked against and four days before it was written.

`StateContext::clear()` destroys every live state while every service is still
alive, and does the three things the report was going to say a one-line ask
hides: it pops from the back rather than calling `frames_.clear()`, it drops
`pending_` and clears `deferring_` first, and it fires no result callback.

**Spent: nothing.** `~GameLevel` and `~Level` became safe at the pin bump with
no change on this side, which was the prediction. The lasting value of this
report is [the rule at the top of this file](#the-rule-that-governs-this-file),
which it is the reason for.

## 2. ~~No activation hook on `State`~~ — `8c104a7`

Asked for `State::on_activated`/`on_deactivated` fanned out by
`StateContext::notify_activation`. Got that, plus two things larger than the
ask: the engine **makes the edge** rather than documenting its absence, because
every client would otherwise write the same remembered `bool`; and
`StateContext::active()` came with it, for the case an edge cannot reach.

**Spent** in `2857160`. All three exhibits deleted:
`GameData::window_has_foreground()`, `GameLevel::match_has_the_foreground()`
and its `window_active_` member, and `MainMenuTitle::update`'s poll.

**The prediction was wrong about the third.** It became `context()->active()`,
not the callback pair — that page gates a refresh it does *every frame*, so
callbacks could only have set a bool for `update()` to read, which is the same
hand-written edge being deleted one file over. This client turned out to have
two cases an edge cannot serve, not one: the other is `GameLevel::update`'s
tick gate, because a match can be constructed while the window is already in
the background and never sees an edge at all.

## 3. ~~`FocusGroup` has no disabled entry~~ — `9bc14b3`

Asked for a registered-but-unpickable entry with a third colour, skipped by
`move()`, and for activating one to be reportable so a page can say why. Got
`set_enabled`/`enabled`, `FocusStyle::disabled`, and `activate()` returning
`Activation` — `none`/`ran`/`refused` — which was **a source break here**, five
call sites.

**Spent** in `705ac11`. Mode Select registers its three unbuilt modes and
switches them off; Player Count greys rows against the live pad count and turns
`refused` into the warning line it used to raise by hand.

**The prediction was wrong about Mode Select getting its beep back**, on
evidence the report itself contained. `move()` takes its candidates from the
enabled entries, so the cursor cannot land on Deathmatch, so there is no press
to refuse. What that page gained is the paint: the disabled colour used to be
passed at construction and survive only because nothing ever registered the
widget.

**The gap that leaves, for whoever files the next UI report.** `refused` is
reachable *only* where a row switches off under a cursor already on it. A page
whose rows are statically unavailable can grey them and nothing more. That is
probably the right design, and it is not what the report thought it was asking
for.

## 4. ~~Every UI leaf is `final`, and `UiContainer` is not a `UiWidget`~~ — `93821c7`

Asked for `final` off the three leaves and for containers to be focusable. Got
both, and the ask's fallback (`UiCompound`) was not needed — `UiContainer`
became a `UiWidget`, so a compound widget is just a class deriving from it.
`UiObject::set_colour` became a virtual no-op so a container can forward paint
to children it knows only as `UiObject*`.

**Spent** in `f078ef5`. `PromptBar` derives instead of composing, losing four
forwarders and a second `hidden` flag; `OptionRow : UiContainer` gives the
options page rows, turning `adjust_focused_value` from one arm per row into a
`dynamic_cast` and a call.

**One override was needed and is worth knowing about.** `OptionRow` overrides
`set_colour` to paint the label only. `UiContainer::set_colour` reaches every
child by design, and inheriting it would put the focus colour on the value and
lose the colour that distinguishes a value from a label.

**Not verified interactively** — no pad on the machine, and these menus read
pads only. The navigation walk was re-derived by hand against
`nearest_in_direction` to confirm left and right still fall through to the page.

## 5. ~~No font default character~~ — `77704d1`

Asked that the built-in `font` kind install a stand-in character at load, and
for a way to ask whether a font can draw a string. Got both. The engine took
the report's sharpest argument — `text_encoding.h` promises U+FFFD and U+FFFD
is in no atlas, so the documented graceful path led into the throw it was
written to avoid — as the reason rather than as decoration.

**Spent** in `2dab258`, and `drawable_text` stayed, as predicted. Question
marks in a weapon description are a worse outcome for *this* game than refusing
to start: the text is authored content with one build, so a wrong character is
always a mistake and never a runtime condition to degrade through.

**The prediction was wrong about how.** It said the hand-written ceiling
*becomes* `can_render`. It cannot: `widen_drawable` runs while
`presentation.json` is being read, which is before `load_manifest`, so there is
no font in the process to ask — which is the reason the ceiling was
hand-written in the first place. So there are two checks now rather than one
replaced: the region constant stays where the text is read, and
`check_font_can_draw` runs after the manifest and asks the real atlas. The
constant is *verified* rather than replaced, which is the better outcome — the
two disagree exactly when a font is regenerated with a narrower region, and no
build step prevents that.

This report also produced report 8, below.

## 7. ~~Four math names deleted as uncalled, with callers here~~ — `343433c`

Asked, in order: that a deletion justified by a caller count say which
repositories were counted; that `Vector2F::rotate_vector` come back; and that
the `intersects.{h,cpp}` question be **decided** rather than re-measured. Got
all three, in that order, and the engine agreed the procedure was the ask that
mattered — `PHILOSOPHY.md` carries it now, under the public face, as a
discipline rather than a check, because the engine cannot reach a repository
that consumes it and should not learn how.

**Spent** in this commit. `game/rotate.h` is deleted after two weeks, and both
call sites are back on `Vector2F::rotate_vector`. The engine's header records
that it was deleted once and why it is back; it came with the two test cases it
should have had, including that it agrees with `normal()` at `PI_OVER_2`.

**The `intersects` decision is the part to read**, and it is not the one this
report asked for the reason it gave. The table is kept — but explicitly *not*
because the count changed, though it had. The engine's answer is that a 2D
engine whose client has to ask whether a segment crosses a rotated rectangle
and write the answer itself has pushed geometry across the boundary, so every
predicate belongs there at whatever call count. **A reason that does not move
when the count moves**, which is what makes it an answer and the count merely
the thing that prompted it. Filed in `intersects.h` where a reader meets it.

`area` and `inflate_to_size` were listed for the record and not asked for; they
stay worked around here, `diffuse_to_size` happily so.

## 8. ~~`first_unrenderable` cannot be used on text with a newline~~ — `f94ac81`

Offered two options: the query skips what the pen walk skips, or the header
names the two characters a caller has to filter. **Got the better one.** The
engine's reasoning was the argument this report had made in passing — two
functions in this one client had already written the same filter, months
apart, so naming the characters in a header leaves every client writing it.

`Font::is_layout_control` now names U+000A and U+000D once, and both
`for_each_glyph` and `first_unrenderable` test it, so the walk and the query
cannot disagree about what is looked up. It stays private, correctly: the
observable contract is that the query skips them, not that clients can ask.

**Spent** in this commit. `check_font_can_draw` is one call over the whole
string again, and the run-walking is gone. Verified both ways with a probe
reading `line one\nline é two`: the newline at index 8 is skipped and the bad
character is still caught at index 14.

`drawable_text::is_layout_control` **stays**, and not as a leftover.
`widen_drawable` needs it, and cannot use the engine's: it runs while
`presentation.json` is being read, before any font exists. Two definitions of
the same two characters, in two repositories, for the reason report 5 records —
and this one is exempting them from a region check rather than from a glyph
lookup.

---

# What is deliberately not here

Re-checked 2026-08-15 against `6b5c0bd`.

- **`CameraTools` never sets a scale**, so field of view is whatever the pane's
  pixel size is and a 4K player sees nine times the world a 720p player does.
  Still true, and still not a defect to file: it needs someone to decide first
  whether the engine's camera owns a resolution-independent world height.

  **What changed is that the engine has now written down the adjacent
  contract.** `camera_tools.h:13-27` says `viewport_size` is in *world* units,
  that `calculate_camera` never reads `prev_camera.scale` and must not, and
  that passing pixels "is correct only at scale 1, silently and everywhere".
  This game passes `player_viewport(i).size()` — pixels — at
  `level.cpp:84,385`, which is correct *because* it never zooms, and would go
  quietly wrong the day it does. Worth knowing before anyone adds a zoom.

- **`Renderer::window_size_changed` returns a signal `Application` ignores**
  (`application.cpp:384,411`, both `std::ignore =`), and **nothing reports the
  desktop size**. Both still true.

  **But the reason this was parked has expired.** It was filed under "one part
  of the larger *layout follows the real window* question" — and `0702776`
  answered that question: `on_window_size_changed` now re-points
  `ResolutionManager` at the real client size before the renderer resizes, so
  every route into a resize holds the invariant.

  **That means this repository has an unrevisited workaround.**
  `MainMenuOptions::apply_settings` coerces the fullscreen path by hand, and
  its comment gives as the reason "Nothing on ResolutionManager or Application
  reports the desktop size" and describes ViewportManager "sizing from a number
  the screen does not have". The second half of that is now fixed engine-side.
  The workaround has not been re-examined, and **cannot be on this machine** —
  reaching the Options page needs a pad. Someone with one should check whether
  it is now redundant, and whether the resolution row says the right thing
  after a fullscreen toggle.

- **The write side of `json.h`.** Still absent, still correct:
  `ARCHITECTURE.md` names the read-only asymmetry and settles it, and T12
  declines automatic serialisation. What survives is narrower — the ~50-line
  atomic-write protocol in `game/save.cpp` is entirely game-agnostic and no
  second client should re-derive it — and is a small ask best made alongside
  something else.

- **`Scene::contacts()` having no reader** (`scene.h:198`), **`labrador::Label`
  having no client** (`engine/render/label.h:28` — it moved out of `scene/`),
  and the anti-tunnelling bound never being evaluated. Still true, and still
  not engine defects. They are things this client has not got round to
  exercising, which is a debt this repository owes the engine rather than the
  other way round.

---

# What the audit changed

Run 2026-08-15, immediately after the pin moved to `6b5c0bd`, because a ledger
that is nine-tenths answered reports is a ledger nobody can point anybody at.

**Removed: the bodies of the five closed reports**, about four hundred lines.
They described engine code at `8e43d72` and cited it by line — `focus.cpp:118-130`,
`widget.h:47`, `state_context.h:169,173`, `application.cpp:526-538` — and
forty-three commits have moved every one of those. They were originally kept
"because the reasoning is what the engine's fix agreed with", and that value
was banked when the fixes landed and were consumed. They are in git at
`a728f42` and one `git show` away.

**Kept, in one paragraph each:** what was asked, what shipped, which commit
spent it, and — the part worth keeping longest — **what each prediction got
wrong**. Three of the five were wrong in some detail, and this file's standing
rule is to record that rather than quietly match itself to the outcome.

**Promoted to report 8:** the `first_unrenderable` layout-control finding,
which was a note under closed report 5 marked "worth raising with report 6".
Report 6 is on indefinite hold, so that was a live item filed behind a blocked
one. It is small and should not wait.

That promotion is the audit paying for itself: reports 7 and 8 were both filed
off the audited file and both answered the same day. Report 8 would still be a
paragraph under a closed heading otherwise.

**Re-verified every live claim** against `6b5c0bd`. Report 6 survived intact;
only its citations had drifted (`sound_bank.h:67,73` → `66,72`). Two of the
four "deliberately not here" items had moved, and one of them turned up
something this repository has to act on — the fullscreen workaround above,
whose premise the engine fixed and which nobody has looked at since.

**Not touched: `docs/review/gap-audit.md`.** It says on its face that it is the
audit as written and is not updated as gaps close, which is the same rule the
engine's own `docs/review/` follows. This file is the opposite kind of document
and always has been, which is why auditing it is maintenance rather than
vandalism.
