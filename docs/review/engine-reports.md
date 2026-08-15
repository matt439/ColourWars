# Reports to file against Labrador

> Seven reports. Six were drafted 2026-08-14 against engine revision
> `8e43d72`; the seventh was written 2026-08-15 against `6b5c0bd`, the pin that
> closed five of the first six. Each one is a case where this client wanted
> something ordinary and the API forced a bad answer, and each carries an
> **exhibit**: the workaround that is in this repository now, written because
> the engine offered nothing better.
>
> **Five are closed and struck below; one is on hold; one is new and unfiled.**
> Every engine citation was checked against the pinned revision on the day of
> writing — and the pin is not the engine's master. That is how report 1 came
> to describe a bug the engine had fixed the day after the pin was taken. Check
> master before filing either of the two that are left.
>
> They are drafted here rather than in the submodule on purpose. CLAUDE.md puts
> engine *changes* in the Labrador repository; a report is a thing this client
> owes it, and this is the client.

## Why the exhibits matter more than the asks

`PHILOSOPHY.md` declines speculative frameworks, and the engine has been
consistent about it: `gamepad.h` says outright that there is no action map
because "the paint-shooter and the sample both spell their bindings in code,
and neither has a rebinding screen to fill such a table from". That is the
right call, and it means a report saying "an engine should have X" is worth
nothing. A report saying "here is the code I wrote because it did not, in a
file you can open" is worth something.

So each report below leads with the workaround. Four of the original six
exhibits were written in the three days before they were drafted and did not
exist when the gap audit was run; report 7's three were written the day the pin
moved, because the pin moving is what made them necessary.

| # | Report | Exhibit exists | Status |
|---|---|---|---|
| 1 | ~~`StateContext` never empties its stack~~ | n/a — unfixable client-side | **closed** — engine `881d3c7`, fixed before this was drafted |
| 2 | ~~No activation hook on `State`~~ | yes, two of them | **closed** — engine `8c104a7`, filed and landed |
| 3 | ~~`FocusGroup` has no disabled entry~~ | yes, two of them | **closed** — engine `9bc14b3`, filed and landed |
| 4 | ~~Every UI leaf is `final`~~ | yes | **closed** — engine `93821c7`, filed and landed |
| 5 | ~~No font default character~~ | yes | **closed** — engine `77704d1`, filed and landed |
| 6 | No audio bus or master gain | **not yet** | hold |
| 7 | Four math names deleted as uncalled, with callers here | yes, three | **to file** |

**The pin has since moved** — forty-three commits, to `6b5c0bd` — so all five
closures are live here now, and each heading below records what its arrival was
worth and links the commit that spent it. Report 1 was worth nothing to spend,
which was the prediction.

---

## 1. ~~`StateContext` never empties its stack~~ — CLOSED, and it was closed before this was written

**Never file this. The engine fixed it in `881d3c7`, "Drain the state stack
before the services it borrows", on 2026-08-10 — the day after the revision
this report was checked against, and four days before the report was drafted.**

`StateContext::clear()` destroys every live state, and `~Application` calls it
as its first statement, while every service is still alive. The three things
this report said a one-line ask hides are all in it, and all three are pinned
by tests in `tests/core/state_context_tests.cpp`: it pops from the back rather
than calling `frames_.clear()`, it drops `pending_` and clears `deferring_`
before it starts, and it fires no result callback, for the reason given
below — the engine's header cites this game's match frame reopening the menu
as the worked example.

So `~GameLevel` and `~Level` become safe at the next pin bump, with no change
on this side. **What this repository does about it: nothing.**

The lesson worth keeping is the one about aim rather than about the bug.
`8e43d72` is what `external/labrador` is pinned at, not what the engine's
master is, and a report checked against the pin describes a codebase the
engine may have left. Report 6 was drafted the same way and should be re-checked
against master before anyone files it.

The original analysis follows, unedited, because the reasoning is what the
engine's fix agreed with.

### What happens

`Application` derives from `StateContext` and declares every service as a
derived member:

```cpp
class Application : public DeviceNotify, public StateContext   // application.h:81
```

C++ destroys derived members first and the base subobject last. So
`StateContext::frames_` — and every `State` in it — is destroyed **after**
`audio_resources_`, `viewport_manager_`, `render_resources_` and the rest
(`application.h:154-170`). `~Application` touches only the audio engine and COM
(`application.cpp:50-60`), and `StateContext::~StateContext() = default`
(`state_context.cpp:8`).

This game's architecture is built on exactly those destructors, and says so in
its own documentation:

- `~GameLevel` calls `viewport_manager()->set_layout(previous_layout_)` —
  `game/states/game_states.cpp:115-118`
- `~Level` calls `sound_bank_->stop_effect(music_)` and two more —
  `game/objects/level.cpp:66-71`

Closing the window during a match — the X button or Alt+F4, so `WM_DESTROY` →
`PostQuitMessage` → `run()` returns → the `Application` is destroyed at
`game/main.cpp` — calls `set_layout` on a destroyed `ViewportManager` and
`stop_effect` on a `SoundBank` whose owner is gone.

### Why there is no exhibit

There is no honest client-side fix, and that is the report. Moving that cleanup
out of the destructors deletes precisely what `state_context.h` spends its
opening comment arguing for, and what CLAUDE.md advertises as the architectural
win of the state stack. A client cannot drain the stack: nothing in the public
API pops frames from outside, and there is no "the application is closing" hook
to do it from.

The engine's own round-2 review recorded this ordering and judged it *latent*,
on the explicit ground that every state destructor was then implicit or
defaulted. That premise expired when `~GameLevel` and `~Level` were written.

### The ask

`run()` clears the stack before returning. Preferable to a fix in
`~Application`, because it also gives states a real closing moment, in the
order the stack was built, while every service is still alive.

**Three things a one-line ask hides, and a client found all three:**

1. **`frames_.clear()` is not the fix.** MSVC destroys vector elements
   front-to-back, i.e. bottom-of-stack first, so `GameMenu` would die before the
   `GameLevel` sitting above it. It must pop from the back.
2. **It must set `deferring_` and drop `pending_`** (`state_context.h:169,173`)
   so that a destructor calling `pop()` cannot re-enter the drain it is inside.
3. **It must document whether `on_suspend` and pending `on_result` callbacks
   fire.** They must not: a result callback delivered during teardown runs
   against a state that is being destroyed, and this game's callbacks push new
   pages.

---

## 2. ~~No activation hook on `State`~~ — CLOSED, filed and landed

**Filed 2026-08-14 and implemented the same day: engine `8c104a7`, "Tell the
states when the player looks away".**

What shipped is the ask: `State::on_activated` / `on_deactivated`, empty
defaults, fanned out by `StateContext::notify_activation(bool)` across every
frame top-down, with all four of the shell's window handlers wired to it.

Two things came back larger than the ask, both from the questions this report
raised:

- **The engine makes the edge rather than documenting its absence.** This
  report asked only that the header say the callback is not edge-guarded. The
  engine's answer was that every client would then write the same remembered
  `bool` — the argument in this report's own point 2 — so `notify_activation`
  drops the repeat and the callback is an edge for everyone.
- **`StateContext::active()` came with it**, for the case an edge cannot
  reach: the stack keeps updating in the background, so a state can be
  constructed there and be younger than the message it needed. Its `init()`
  asks instead. True until told otherwise, since a window given the foreground
  on creation is never sent a message saying so.

`WindowNotify::on_activated`, `on_deactivated` and `on_suspending` lost their
`const` — they now carry news into a state stack where game code runs. Nothing
outside the engine implements `WindowNotify`, so it costs this repository
nothing.

**Spent, and all three exhibits are gone.** Every deletion this predicted
happened, and one of them differently than predicted:

- `window_has_foreground()` in `game/game_data.h` — gone, as its own comment
  asked. `GameData` is back to six borrowed pointers and their forwards, with
  no question of its own to answer.
- `GameLevel::match_has_the_foreground()` and `window_active_` — both gone.
  `on_activated`/`on_deactivated` call `Level::resume`/`suspend` straight
  through, because the engine makes the edge rather than leaving each client
  to remember one.
- `MainMenuTitle::update`'s poll — gone, but **as `context()->active()`, not
  as the callback pair this predicted.** The prediction was wrong in a way
  worth recording: that page gates a refresh it does every frame, so callbacks
  could only have set a bool for `update()` to read, which is the same
  hand-written edge just deleted from `GameLevel`. The engine shipped
  `StateContext::active()` for exactly the case an edge cannot serve, and it
  turns out this client had two of those, not one — `GameLevel::update`'s tick
  gate is the other, for the reason the engine's header gives: a match can be
  constructed while the window is already in the background, and never sees an
  edge at all.

The original report follows.

### What happens

The shell receives `WM_ACTIVATEAPP` and acts on it privately
(`application.cpp:526-538` → `on_activated`/`on_deactivated`), which suspends and
resumes the pad reader and forwards nothing to the state stack.
`State::on_suspend` and `on_resume` (`state.h:60,65`) mean "something was pushed
above me" and are called only from `apply_push`/`apply_pop`.

A suspended reader answers "disconnected" for every slot — which
`gamepad_reader.h` states as its contract — and `PlayerInput` turns that into a
neutral input for every player. Neutral is a legal input, so nothing downstream
notices. Alt-tab out of a match and it keeps running: four characters standing
still soaking paint, the clock counting down, the music and any looping weapon
voice at full volume over whatever the player switched to.

### The exhibits

**Two, and they want the hook for unrelated reasons**, which is the part worth
reading:

- `game/game_data.h` — `window_has_foreground()`, a `GetForegroundWindow()`
  comparison this game has to poll because nothing will tell it. Its comment
  says it exists to be deleted.
- `game/states/game_states.cpp` — `GameLevel::match_has_the_foreground()`, which
  polls it once a frame and suspends the match on the edge.
- `game/states/main_menu.cpp` — `MainMenuTitle::update` asks the *same question*
  so that its "Connect a controller" line does not appear every time the player
  clicks on another window. That page does not care about the simulation at
  all; it cares that the pad list is a lie while the reader is suspended.

One shell fact, two clients, neither of them able to be told.

### The ask

`virtual void State::on_activated()` / `on_deactivated()`, fanned out by
`StateContext::notify_activation(bool)` the way `draw()` already walks `frames_`.
Wire all four private handlers at `application.cpp` — alt-tab, minimise and
sleep/resume all want one answer.

**Two questions only a client discovers:**

1. **Top frame or all frames? All, top-down.** A suspended `GameLevel` under a
   pause menu still owns live audio, so a hook that only reaches the top frame
   leaves the thing that actually needs to know unreachable.
2. **Say in the header that the callback is not edge-guarded.** `WM_ACTIVATEAPP`
   is not, unlike the suspend path. `SoundBank::pause_effect` and
   `resume_effect` (`sound_bank.h`) carry no depth, so "suspended twice" and
   "suspended" are the same state and one resume answers both. A client that
   trusts the callback to be an edge writes a bug that only appears when the
   window is activated twice in a row. This game guards it with a remembered
   `bool`; every client will have to.

Bundle with report 1: same header, same test file, one pin bump.

---

## 3. ~~`FocusGroup` has no disabled or non-focusable entry~~ — CLOSED, filed and landed

**Filed 2026-08-14 and implemented the same day: engine `9bc14b3`, "Let a menu
say a row is there and cannot be picked".**

`FocusGroup::set_enabled(widget, bool)` and `enabled(widget)`, a third
`FocusStyle::disabled` colour that `refresh_style()` derives like the other
two, and `move()` taking its candidates from the enabled entries only — so a
disabled row between two live ones costs one press, not two. The setter rather
than an `add_disabled` because the Player Count case decided it: enablement is
a property that changes, and a permanently dead row is the same call made once.

**`activate()` now returns `Activation`, not `bool`, and that is a source
break here.** `none` / `ran` / `refused`. The old `bool` already meant two
things — the engine's own `button.h` says an unbound row is legal and common —
so a third meaning would have left it useless for exactly the thing this report
asked for. Five call sites in this repository are `if (this->focus_.activate(0))`
and become `== Activation::ran`; the two in `pause_menu.cpp` are statements and
are unaffected.

**Spent, and half of it was predicted wrongly.**

- **Mode Select registers the three unbuilt modes and switches them off** —
  `MainMenuModeSelect::init` — but **it does not get its beep back**, and that
  prediction was wrong on evidence this report had already written down.
  `move()` takes its candidates from the enabled entries, so the cursor still
  cannot land on Deathmatch, so there is still no press to refuse. `refused`
  is reachable only where a row switches off under a cursor already on it, and
  these three are decided once at init and never change.

  What it is worth is the paint. The colour used to be passed at construction
  and survive only because nothing ever registered the widget — the page's own
  comment said so, and said the moment anything did register it the next
  `add()` or `move()` would repaint it as an ordinary row. It is derived from
  enablement now, so it is the group's answer rather than this page's hope,
  and the rows are what they always were: there, and unpickable, said once
  rather than arranged for.
- **Player Count switches rows instead of refusing by hand**, as predicted,
  and it is the page that gets the half Mode Select does not.
  `follow_the_pads_on_the_table` runs once a frame against the live pad count;
  `start_team_select` no longer contains its own refusal and now runs only
  when it will succeed; `update()` turns `Activation::refused` into the error
  wave and the warning line. The row a player is sitting on when somebody
  pulls a pad out is exactly the case the engine's setter was designed for,
  and it is the case this client has.
- Nothing else changed: the pages that never call `set_enabled` behave exactly
  as they did.

**The gap worth recording for whoever files the next one.** The report asked
that activating a disabled entry be "reportable to the page so it can say
why", and the engine delivered that — but `refused` can only ever be reached
in the live case, because the walk skips disabled rows. A page whose rows are
statically unavailable can grey them and nothing more. That is probably the
right design and it is not what this report thought it was asking for.

The original report follows.

### What happens

`FocusGroup::refresh_style()` (`focus.cpp:118-130`) walks every registered
widget and sets its colour to one of exactly two values, and it is called
automatically from `add`, `set_focused`, `move` and `set_style`. There is no
third state and no way to register a widget the cursor should skip.

The header knows (`focus.h:89-93`):

> exposed because a page that recolours a widget for its own reasons — a
> disabled row, a team tint — has to put the focus paint back afterwards

That names the case and offers a repaint fighting a repaint.

### The exhibits — two, and only one of them could be worked around

**Mode Select** advertises four modes and three are not built. They used to be
registered with an action that played an error beep, painted in the same colour
as the working rows. They are now simply **not registered**
(`game/states/main_menu.cpp`, `MainMenuModeSelect::init`), so they keep their
construction colour for life and `move()` walks past them by bounds. That works,
and it costs the beep: there is no way to be on Deathmatch, so there is no press
to answer, and the screen can no longer say *why* it is unavailable.

**Player Count** cannot use that trick. Its rows become available and unavailable
*while the page is open*, because a pad can be plugged in between two frames. A
page that re-registered its focus group every frame to track that would reset
the cursor every frame with it. So that page keeps all four rows live and
refuses the press instead, with a warning line
(`MainMenuPlayerCount::start_team_select`).

One gap, approached from both sides: static unavailability is workaroundable by
declining to register, live unavailability is not workaroundable at all.

### The ask

A registered-but-not-focusable entry — `add_disabled(widget)`, or a
`set_enabled(widget, bool)` — with a third `FocusStyle` colour, skipped by
`move()` and left alone by `refresh_style()`. Activating one should be
reportable to the page so it can say why, rather than silently doing nothing.

---

## 4. ~~Every UI leaf is `final`, and `UiContainer` is not a `UiWidget`~~ — CLOSED, filed and landed

**Filed 2026-08-14 and implemented: engine `93821c7`, "Open the widget set,
and let a container be focused".**

Both halves, and the ask's second option was not needed. `UiTexture`, `UiText`
and `UiTextDropShadow` lose `final`; `UiContainer` becomes a `UiWidget` and
loses `final` too, so there is no `UiCompound` and nothing new to learn — a
compound widget is a class deriving from the container that was already there.

The header now gives the reason the old ones did not. Dropping `final` costs
nothing structural, because `~GameObject` is virtual and every leaf was
already safe to hold and delete as a `UiWidget*` — which is the answer to this
report's closing point that the silence read as unconsidered rather than
decided.

**How the container got a colour without narrowing anything.**
`UiObject::set_colour` is now a virtual no-op, and `UiWidget` re-declares it
`override = 0`. So a container forwards the paint to children it knows only as
`UiObject*` with no cast and no narrowing of `add_child`, and painting a thing
that has no colour is nothing rather than an error. Nesting works by
composition: a nested container forwards in turn.

**Spent, both halves, and both as predicted.**

- **`PromptBar` derives instead of composing.** `class PromptBar final :
  public labrador::UiTextDropShadow`, one constructor, and the four
  forwarders are gone. So is a subtlety they carried: there used to be two
  `hidden` flags, the bar's and the text object's, with a comment explaining
  that a page hides the bar and never the string. One object, one flag,
  nothing to explain.
- **The options page has rows.** `class OptionRow final : public
  labrador::UiContainer` in `game/states/option_row.{h,cpp}`, holding its
  label, its value and what left and right do to it, handed whole to
  `FocusGroup::add`. `MainMenuOptions::adjust_focused_value` is a
  `dynamic_cast` and a call where it was one arm per row, so a third setting
  now costs nothing there. Four loose widgets became two.

  **It overrides `set_colour` to paint the label only**, which is the one
  place the engine's default was not what this client wanted:
  `UiContainer::set_colour` reaches every child, and inheriting it would put
  the focus colour on the value and lose the colour that tells a value from a
  label. A three-line override, and the reason the base is documented as
  painting the whole row is that most callers do want that.

  This is also what report 6's volume setting needed somewhere to be drawn,
  and that place now exists.

**Not verified interactively.** There is no pad on the machine this was done
on, and this game's menus read pads only, so the Options page was reasoned
about rather than driven: the navigation walk was re-derived by hand against
`nearest_in_direction` to confirm that left and right still find nothing and
fall through to the page. The reasoning is recorded in `MainMenuOptions::update`
because it changed — the old comment said every row shared a centre x, and a
row that spans its label and its value does not. The answer survives for a
different reason, which is that the walk measures overlap.

The original report follows.

### What happens

```
UiContainer      final   widget.h:47    and is a UiObject, not a UiWidget
UiTexture        final   widget.h:80
UiText           final   widget.h:115
UiTextDropShadow final   widget.h:137
```

`UiWidget` itself is open, so a client can derive from it — but only by
reimplementing `update`, `draw`, `bounds` and `set_colour` over members it
composes. It cannot extend a text object with one extra behaviour, and it cannot
hand a container to `FocusGroup::add`, which takes `UiWidget*`.

### The exhibit

`game/states/prompt_bar.h` — `PromptBar` is "a line of text plus a rule for
building it", which is a `UiTextDropShadow` with one extra constructor. It
composes one and forwards four virtuals to it, because it cannot derive.

The larger case is a row-with-a-value: a label and a value side by side that the
focus group can hold. That is a `UiContainer` that is also a `UiWidget`, and it
cannot exist. It is why this game's options page recovers the label↔value
relationship by comparing the focused pointer against each label in turn, and
why there is no slider anywhere — which is, in turn, why the volume setting in
report 6 has nowhere to be drawn.

### The ask

Drop `final` from the three text and texture leaves, and either make
`UiContainer` a `UiWidget` or add a `UiCompound : UiWidget` that holds children.
If `final` is load-bearing for a reason a client cannot see, say so in the
header — the current headers give no reason, so it reads as unconsidered rather
than decided.

---

## 5. ~~No font default character~~ — CLOSED, filed and landed

**Filed 2026-08-14 and implemented: engine `77704d1`, "Give a font something to
draw when it has not got the glyph".**

Both parts of the ask, and the engine took the sharpest argument in this report
— that `text_encoding.h` promises U+FFFD and U+FFFD is in no atlas — as the
reason rather than as decoration. That header now says the promise leads to a
question mark.

- **The built-in `font` kind installs a stand-in character at load.** `?` if
  the font has it, `' '` if not, nothing if it has neither — and both
  candidates are deliberately inside the 95-character region, since a fallback
  outside it would reintroduce the bug inside the fix. `ContainsCharacter` is
  asked before `SetDefaultCharacter` rather than the throw being caught.
- **`RenderResources::can_render(font, text)` and `first_unrenderable(font,
  text)`**, per UTF-16 unit, `npos` when the whole string is drawable. They ask
  of the atlas, not of what would happen, because after the substitution
  nothing throws and the only question left is whether the player can read it.

**Spent, and the shape it took is not the one predicted, for a reason worth
keeping.** `drawable_text.{h,cpp}` stays, as expected, and the transliteration
table with it. But the prediction was that the hand-written ceiling *becomes*
`can_render`, and it cannot: **`widen_drawable` runs before any font exists.**
`presentation.json` and every level file are read before `load_manifest`
(`main.cpp`), which is the whole reason the ceiling was hand-written in the
first place — a fact this report had, in the sentence about the throw arriving
four screens later, and did not follow through.

So there are two checks now rather than one replaced:

- **The region constant stays where the text is read**, because that is the
  only place that can name the file and the key, and it has no atlas to ask.
- **`check_font_can_draw` runs once after `load_manifest`**, over the two
  authored strings — the weapon descriptions against
  `weapon_description_font`, the stage names against `item_font` — and asks the
  atlas the engine actually loaded. `first_unrenderable` gives it the index, so
  the message names the character, where it is and which font lacks it.

That makes the constant *verified* rather than *replaced*, which is arguably
the better outcome: the two disagree exactly when a font is regenerated with a
narrower region than the headers were decoded as having, and nothing in the
build prevents that.

### `first_unrenderable` and the layout controls — for the next report

**A caller cannot use it on a string with a newline in it without filtering
first, and every weapon description in this game has one.** The first run of
this check condemned all four of them at their `\n`.

It is not a defect: `render_resources.h` says the calls ask of the atlas rather
than of what would happen, and no atlas holds a glyph for U+000A. But the
question a client has is "will this string draw", and the gap between the two
is exactly the set of characters the glyph walk answers itself before it looks
anything up — `\n` and `\r`. `drawable_text::is_layout_control` already knew
that list, and `check_font_can_draw` now walks the runs between them.

Worth raising with report 6 rather than alone: either the engine's pair skips
what the walk handles, or the header names the two characters a caller has to
filter, rather than leaving each client to find them by condemning its own
content.

The original report follows.

### What happens

Every `.spritefont` in both trees carries exactly **95 glyphs, U+0020 to U+007E,
contiguous, with `defaultChar` = U+0000**. I decoded the header of all
thirty-three, the engine sample's included, and there is no exception. That is
MakeSpriteFont's default region, which nothing chose and nothing records.

With no default character, DirectXTK's `FindGlyph` throws. `SetDefaultCharacter`,
`ContainsCharacter` and `FindGlyph` are all public on `SpriteFont` and exposed by
none of `render_resources.h`; the only route to a `SpriteFont*` is `impl()`,
behind the backend header, whose own comment says a third client "would be a
mistake". So a client cannot ask "can this font draw this string?", cannot
nominate a substitute glyph, and cannot supply a fallback font. Both text entry
points walk glyphs and both throw, so probing costs the same as failing.

**The sharpest part is that the engine documents the opposite behaviour.**
`text_encoding.h` promises that invalid UTF-8 becomes U+FFFD so that "text about
to be drawn should show mojibake, not vanish" — and U+FFFD is outside 32..126,
so it is in no atlas in either tree. The documented graceful path leads directly
into the throw it was written to avoid.

The engine also already makes exactly this kind of substitution one module over:
a missing wave bank becomes a silent bank plus one line on stderr, which
`resource_loader` calls the one substitution it makes for a missing file.

### The exhibit

`game/drawable_text.h` and `.cpp` — every content string is now converted through
one place that knows the 95-character ceiling, transliterates the punctuation an
editor inserts on its own, and throws at load naming the file and the key.
Written because the alternative was measured: a curly apostrophe in a weapon
description let the game start, load and reach the menu, and would have thrown
at weapon select four screens later, in a modal box captioned "startup failure",
during play.

### The ask

Lowest value of the six, because a client can close it — but the part worth
having is small and belongs in the engine: **the built-in `font` kind should
install a default character at load**, so that out-of-the-box behaviour is
degradation rather than a throw, matching what the audio loader already does.
`font_can_render(FontHandle, wstring_view)` or `first_unrenderable(...)` would
let a client report rather than guess.

---

## 6. No audio bus, master gain, listener or voice cap — **hold until the workaround exists**

**Do not file this yet.** Everything below is true, and filing it before the
client-side mix is written throws away the exhibit that would make it
persuasive. The engine has been consistent about declining features that no
client has demonstrated wanting; a report with 74 hand-written multiplications
attached is a different document from a report without them.

### What happens

`SoundBank::play_wave` and `play_effect` take a volume **per call**
(`sound_bank.h:67,73`) and the class carries no gain. `AudioResources` is a bank
registry with no gain either. DirectXTK's `AudioEngine` — which has
`SetMasterVolume` — is a private member of `Application` with no accessor among
its services. So a client can only multiply a gain into every emitter, cannot
touch a fire-and-forget wave it holds no handle to, and cannot reach the one
object that could do it globally.

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

---

## 7. Four math names were deleted as uncalled, and this client called all four

**Written 2026-08-15 against `6b5c0bd`, the pin that closed reports 1–5. Check
master before filing: the engine moves faster than this file.**

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
report that asks for all five is a report that can be dismissed on the weakest
one.

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
were not. The ask below is therefore about the *procedure* first and the four
names second.

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
  Trivially replaceable, listed for completeness, and the least of the four.
- `game/objects/projectile.cpp` — `DiffusingProjectile::diffuse_to_size`, a
  protected helper doing what `inflate_to_size` did, on a rectangle passed by
  reference because mist and spray each own their own. **This one is arguably
  better where it is now** and the engine may reasonably decline it: the base
  already owns the size, and the two subclasses were writing the same line.
- `game/rotate.h` — one free function, `rotate_vector`, byte-for-byte the body
  that was deleted. **This is the one that should go back.** A 2D engine's
  vector type that can answer `angle()`, `unit_vec_from_angle()` and `normal()`
  and cannot rotate a vector by an angle has a hole in it, and the client's
  copy is a second definition of engine arithmetic sitting outside the engine's
  tests.

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
supposed to adopt and never did — is a real one and is not settled by this
client having found a use for one of the twenty-five.

`area` and `inflate_to_size` are listed for the record and this client does not
ask for either back.

---

## What is deliberately not here

- **`CameraTools` never sets a scale**, so field of view is whatever the pane's
  pixel size is and a 4K player sees nine times the world a 720p player does.
  Real, confirmed, and a genuine engine question — but it needs someone to
  decide first whether the engine's camera owns a resolution-independent world
  height. Filing it as a defect before that decision would be filing a design
  question as a bug.
- **`Renderer::window_size_changed` returning a signal `Application` ignores**,
  and there being no way to ask the desktop size. Same reason: it is one part of
  the larger "layout follows the real window" question, and the client's own
  workaround comment already records it.
- **The write side of `json.h`.** `ARCHITECTURE.md` names the read-only
  asymmetry and settles it, and T12 declines automatic serialisation. What
  survives is narrower — the ~50-line atomic-write protocol in `game/save.cpp`
  is entirely game-agnostic and no second client should re-derive it — and that
  is a small ask best made alongside something else.
- **`Scene::contacts()` having no reader**, `labrador::Label` having no client,
  and the anti-tunnelling bound never being evaluated. These are not engine
  defects. They are things this client has not got round to exercising, which is
  a debt this repository owes the engine rather than the other way round.
