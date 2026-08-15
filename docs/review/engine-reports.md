# Reports to file against Labrador

> The running list of API complaints this client owes the engine. Each one is a
> case where the game wanted something ordinary and the API forced a bad
> answer, and each carries an **exhibit**: the workaround that is in this
> repository, in a file you can open, written because the engine offered
> nothing better.
>
> **Three are live and unfiled. Five are closed** and kept below as one
> paragraph each — what was asked, what shipped, and what this repository spent
> the answer on.
>
> They are drafted here rather than in the submodule on purpose. CLAUDE.md puts
> engine *changes* in the Labrador repository; a report is a thing this client
> owes it, and this is the client.

**Audited 2026-08-15 against `6b5c0bd`.** What that removed and what it turned
up is at the bottom, under [What the audit changed](#what-the-audit-changed).

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
shape is the strike rate: of the five reports filed with exhibits, four were
implemented the day they were filed.

---

# Live

| # | Report | Exhibit | Status |
|---|---|---|---|
| 6 | No audio bus, master gain, listener or voice cap | **not yet** | hold — the exhibit has to exist first |
| 7 | Four math names deleted as uncalled, with callers here | yes, three | **to file** |
| 8 | `first_unrenderable` cannot be used on text with a newline | yes | **to file** — small, bundle it |

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

## 7. Four math names were deleted as uncalled, and this client called all four

The engine's math review deleted around twenty-five zero-caller names across
two commits. **Four of them had callers here**, and the game stopped compiling
at the pin bump:

| Name | Deleted by | Callers here |
|---|---|---|
| `RectangleF::contains(const Point2F&)` | `42f1f47` | 1 |
| `RectangleF::area` | `42f1f47` | 1 |
| `RectangleF::inflate_to_size` | `42f1f47` | 2 |
| `Vector2F::rotate_vector` | `42f1f47` | 2 |

A fifth, `Vector2F::direction()` and the nine-value `mattmath::Direction` with
it, is **not** part of this report. Its only use here tested three of the nine
values, all of which mean the sign of `y`, and the engine's reasoning — that it
was shadowed in practice by `engine/ui/navigation.h`'s `Direction` — is
correct. That call site is now `velocity().y > 0.0f` and is better for it. A
report that asks for all five can be dismissed on the weakest one.

### This is not a new argument, and that is the point

`bb5356d`, the commit before the deletions, makes this report's case in its own
words:

> `TriangleRightAxisAligned` was on this step's list with "one caller, a test"
> against it. ColourWars uses it twice, and the second use constructs one into
> a `unique_ptr` held by base pointer. It is live client API. **That is the
> second item this step's list got wrong, and both were wrong the same way: a
> symbol's callers cannot be counted from one of the two repositories.**

The method was named as unsound, and the very next commit deleted twenty-odd
symbols by it. Two were caught by hand because somebody thought to look; four
were not. The ask leads with the *procedure* and the names come second.

### The exhibits

**One of the four needed no workaround at all**, and it is the most interesting
of them. `out_of_bounds.contains(spawn)` in `level_loaded_info.cpp` is now
`rectangle_point_intersect(out_of_bounds, spawn)` — the free predicate that
survived. So this client is now the first caller of the predicate table that
`bb5356d` measured as having **no caller outside its own file and the tests**,
and recorded in `docs/file-length-audit.md` as a deletion candidate on that
evidence. **Do not delete `intersects.{h,cpp}` on the strength of that
measurement.** It is the same count, taken the same way, from the same one of
the two repositories.

The other three:

- `game/objects/paint_tile.cpp` — `area()` is `width * height` written out.
  Trivially replaceable, listed for completeness, the least of the four.
- `game/objects/projectile.cpp` — `DiffusingProjectile::diffuse_to_size`, a
  protected helper doing what `inflate_to_size` did, on a rectangle passed by
  reference because mist and spray each own their own. **This one is arguably
  better where it is now** and the engine may reasonably decline it: the base
  already owns the size, and the two subclasses were writing the same line.
- `game/rotate.h` — one free function, `rotate_vector`, byte-for-byte the body
  that was deleted. **This is the one that should go back.** A 2D engine's
  vector type that can answer `angle()`, `unit_vec_from_angle()` and `normal()`
  and cannot rotate a vector by an angle has a hole in it, and the client's copy
  is a second definition of engine arithmetic sitting outside the engine's tests.

### The ask

**First, the procedure.** A deletion justified by a caller count should say
which repositories were counted. `git grep` in the engine tree answers a
different question from the one the commit message claims to answer, and
`bb5356d` says so about itself. The client is a submodule of nothing — it
consumes the engine, not the reverse — so the engine cannot grep it as part of
its own build, which makes this a discipline rather than a check, and worth
writing into `CONVENTIONS.md` or the review process rather than remembering.

**Second, `Vector2F::rotate_vector` back**, with the test it should have had.
Twelve lines including the header declaration.

**Third, a decision recorded about `intersects.{h,cpp}`** that does not rest on
the caller count, now that the count is wrong. The library-shape question
`bb5356d` raised — is this a table of predicates the collision system was
supposed to adopt and never did — is real, and is not settled by this client
having found a use for one of the twenty-five.

`area` and `inflate_to_size` are listed for the record and are not asked for.

---

## 8. `first_unrenderable` cannot be used on text with a newline

Small, and it costs the engine either two lines of code or two lines of header.
Bundle it with whatever goes next.

### What happens

`RenderResources::can_render` and `first_unrenderable` (`77704d1`) ask of the
**atlas**, which `render_resources.h` says plainly. No atlas holds a glyph for
U+000A, because the glyph walk answers a line feed itself before it ever looks
a character up.

So the two calls answer "this font has no glyph for that" about a character
that draws perfectly well. **Every weapon description in this game has a
literal `\n` in it**, putting its second sentence on a second line, and the
first run of the client-side check condemned all four of them.

### The exhibit

`game/drawable_text.cpp`, `check_font_can_draw` — it walks the runs *between*
layout controls and asks about each, rather than asking about the string.
`drawable_text::is_layout_control` already existed and already named the two
characters, because `widen_drawable` needed exactly the same exemption for
exactly the same reason.

That is the tell: two functions in this client, written months apart, both had
to know that `\n` and `\r` are not glyph lookups. The engine's glyph walk knows
it too. Only the query does not.

### The ask

Either the pair skips what the walk handles, or the header names the two
characters a caller has to filter. The first is better — the question a client
asks is "will this string draw", and a call that answers "no" about a newline
is answering a question nobody has — but the second costs nothing and would
have saved this client from finding out by condemning its own content.

---

# Closed

Five reports, all answered, all spent here. Kept as outcomes rather than as
reports: what was asked, what shipped, what this repository did with it. The
original bodies are in git — see [What the audit changed](#what-the-audit-changed).

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

This report also produced [report 8](#8-first_unrenderable-cannot-be-used-on-text-with-a-newline).

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
