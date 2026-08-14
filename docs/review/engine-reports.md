# Reports to file against Labrador

> Six reports, drafted 2026-08-14 against engine revision `8e43d72`. Each one is
> a case where this client wanted something ordinary and the API forced a bad
> answer, and each carries an **exhibit**: the workaround that is in this
> repository now, written because the engine offered nothing better.
>
> **These are drafts, not filings.** Nothing here has been raised with the
> engine. Every engine citation was checked against the pinned revision on the
> day of writing.
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

So each report below leads with the workaround. Four of the six exhibits were
written in the last three days and did not exist when the gap audit was run.

| # | Report | Exhibit exists | Priority |
|---|---|---|---|
| 1 | `StateContext` never empties its stack | n/a — unfixable client-side | **file first** |
| 2 | No activation hook on `State` | yes, two of them | **file first** |
| 3 | `FocusGroup` has no disabled entry | yes, two of them | file |
| 4 | Every UI leaf is `final` | yes | file |
| 5 | No font default character | yes | file |
| 6 | No audio bus or master gain | **not yet** | hold |

---

## 1. `StateContext` never empties its stack, so a state's destructor runs against freed services

**Severity: undefined behaviour on the most common exit path.**

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

## 2. No activation hook on `State`, and two clients needed one for opposite reasons

**Severity: a four-minute match plays itself out with nobody in it.**

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

## 3. `FocusGroup` has no disabled or non-focusable entry

**Severity: a menu cannot say "you cannot pick this".**

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

## 4. Every UI leaf is `final`, and `UiContainer` is not a `UiWidget`

**Severity: a client cannot write a compound widget.**

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

## 5. No font default character, so a missing glyph is an exception rather than a blank

**Severity: a content file kills the process several screens from where it was read.**

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
- **`Scene::contacts()` having no reader**, `artattack::Label` having no client,
  and the anti-tunnelling bound never being evaluated. These are not engine
  defects. They are things this client has not got round to exercising, which is
  a debt this repository owes the engine rather than the other way round.
