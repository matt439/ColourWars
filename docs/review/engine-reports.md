# Reports to file against Labrador

> The running list of API complaints this client owes the engine. Each one is a
> case where the game wanted something ordinary and the API forced a bad
> answer, and each carries an **exhibit**: the workaround that is in this
> repository, in a file you can open, written because the engine offered
> nothing better.
>
> **Eight are live as unfiled drafts with exhibits (6, 9, 10, 11, 12, 13, 14, 15). Seven are closed** and kept below as one paragraph
> each — what was asked, what shipped, and what this repository spent the
> answer on.
>
> They are drafted here rather than in the submodule on purpose. CLAUDE.md puts
> engine *changes* in the Labrador repository; a report is a thing this client
> owes it, and this is the client.

**Audited 2026-08-15 against `6b5c0bd`.** What that removed and what it turned
up is at the bottom, under [What the audit changed](#what-the-audit-changed).
Reports 7 and 8 were filed off the audited file and answered the same day, in
`343433c` and `f94ac81`.

**Re-checked 2026-08-27 against `c8176ef`.** Sixty-five commits, and they
closed nothing here — see
[What the 2026-08-27 bump changed](#what-the-2026-08-27-bump-changed) at the
bottom. Report 6's citations moved again, because the audio seam landed
underneath them, and are corrected below.

**Re-checked 2026-09-17 against the then-current pin `984cf13`.** Twenty-nine
commits, and again nothing closed — but one of them reached this repository's
loading order and had to be finished here. See
[What the 2026-09-17 bump changed](#what-the-2026-09-17-bump-changed).

**Re-checked 2026-09-26 against `d356841`.** This pin adds only the UTF-8
file-loading fix to the prior pin. Executable/content paths, JSON, binary
assets and XAudio2 wave banks now preserve Unicode at filesystem boundaries;
ColourWars applies the same convention to profile saves. No audio-bus report
closed. Report 6 now records the existing client mixing exhibit instead of
claiming it has not been written.

**Moved 2026-09-26 to `4cefb68`**, upstream master that day, and re-read under
[What the 2026-09-26 bump changed](#what-the-2026-09-26-bump-changed). Nothing
closed.

**Re-checked 2026-10-14 against `52e809a`** (`v0.1.0` and three documentation
commits), seventy-five commits on. Every live report was read against master and
every exhibit against this tree: nothing closed, a dozen citations had drifted
and are corrected below, and the camera entry under
[What is deliberately not here](#what-is-deliberately-not-here) turned out to have
been answered by this repository. See
[What the 2026-10-14 bump changed](#what-the-2026-10-14-bump-changed).

**Packaging draft added 2026-10-15 against published master `52e809a`.**
`git ls-remote origin refs/heads/master` in the engine confirmed that revision;
its CMake files still have no install/export rules or content-staging helper.
[Report 15](#15-runtime-packaging-and-content-staging-are-repeated-in-the-client--draft-with-release-exhibit)
updates gap-audit F8 against the release machinery this game has since built.
The engine's PI-09/PI-10 already reserve filing and a decision; PI-11 is a
conditional implementation story, not an implemented answer.

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

Report 6 now has its client-side mixing exhibit. It remains an unfiled draft.

---

# Live

| # | Report | Exhibit | Status |
|---|---|---|---|
| 6 | No audio bus, master gain, listener or voice cap | `PlaySettings` gains applied at each emitter | draft — exhibit present; recheck upstream before filing |
| 9 | No application-level per-frame hook or input capture | `MenuPage::update`/`update_page`, `DebugDirector::before_page_update`/`before_match_update`, the frozen-world console; phase 2: `filter_match_inputs` and `DebugPageTarget::debug_reset_input` for the pad chord; phase 4: the live console's keyboard-seat and closing-frame blanks and its own draw path | draft — exhibit present; recheck upstream before filing |
| 10 | No way to read back the frame that was presented | `capture_stack` in `game/debug/debug_screenshot.cpp` | draft — exhibit present; recheck upstream before filing |
| 11 | F10 and a lone Alt belong to the system menu, with no opt-out | the `WH_GETMESSAGE` hook in `game/debug/debug_director_impl.cpp`; `alt-probe.txt` from `scripts/check-debug-console.ps1` | draft — low priority; exhibit present |
| 12 | JSON is read-only, path-only and opaque | the `%TEMP%` candidate file in `tune_commands.cpp`, `json_spans.cpp`, `check_tune_consumed` | draft — exhibit present; recheck upstream before filing |
| 13 | `Key` has no names | the key table in `game/debug/debug_keys.cpp` | draft — low priority; exhibit present |
| 14 | No frame statistics or GPU timing | `DebugDirector::begin_match_draw` and the QPC timing in `update_match`; `debug_perf.*` | draft — exhibit present; recheck upstream before filing |
| 15 | Runtime packaging and content staging are repeated in the client | `game/CMakeLists.txt`, `cmake/release_package.cmake`, `cmake/release_assets.cmake`, `cmake/stage_wave_bank.cmake` | draft — exhibit present; checked against published master 2026-10-15 |

Nothing else is outstanding, on either side, beyond reports 6 and 9-15 below. The fullscreen workaround the
audit flagged under [What is deliberately not here](#what-is-deliberately-not-here)
has since been driven and fixed — keyboard support made the Options page
reachable on a machine with no pad.

---

## 6. No audio bus, master gain, listener or voice cap — **draft with mixing exhibit**

**Exhibit added by the controls/audio increment.** `game/play_settings.h`
holds the saved music and SFX gains; `game/states/menu_page.cpp`,
`game/objects/level.cpp`, `player.cpp`, `weapon.cpp`, `weapon_bucket.cpp`,
`weapon_sniper.cpp`, `structure_paintable.cpp`, `level_intro.cpp` (the
reveal's hover and click) and `controls_settings_menu.cpp` (the music preview)
apply them at individual emitters. Resuming a match or weapon also reapplies the gain. The client now
demonstrates the duplicated mixing work that was previously hypothetical.

On 2026-10-14, Labrador master at `52e809a` still had no bus/master-gain
interface: the audio headers changed only in comments, and the new `webaudio`
backend declares nothing above the seam. The engine's own product documents now
take a provisional position (its `STORIES.md` OCA-51: "no volume bus without a
demand argument"), so the filing has to make that argument: every client
otherwise multiplies a gain into every emitter, as this one does. The report remains unfiled; recheck upstream before
filing. The exhibit supports shared music/SFX gain. Listener and voice-budget
requests below remain separate proposals, not implemented requirements.

### What happens

`SoundBank::play_wave` and `play_effect` take a volume **per call**
(`sound_bank.h:125,131`, and `set_effect_volume` at `:136`) and the class carries no gain. `AudioResources` is a
bank registry with no gain either. The one object that could do it globally is
a private member of `Application` (`application.h:282`) with no accessor among
its services. So a client can only multiply a gain into every emitter, cannot
touch a fire-and-forget wave it holds no handle to, and cannot reach the object
that could do it globally.

**And that object is the engine's own now, which changes what the ask is.**
Until `94076f1` it was DirectXTK's `AudioEngine`, and `SetMasterVolume` was a
method the engine happened to be sitting on — so the report could be read as
"expose the library you already have". The audio seam replaced it with
`labrador::AudioDevice` (`engine/audio/audio_device.h`), whose whole surface is
`open_wave_bank`, `wave_index`, `create_voice`, `play_wave`, nine per-voice
verbs and the shell's `update`/`suspend`/`resume`. No master gain among them,
and nothing left to expose. A master gain is now something the engine would
have to *decide*, on both backends, which is a stronger reason to bring an
exhibit and not a weaker one.

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

The shared Controls / audio page now supplies the volume rows, and the
per-emitter gain calls above supply the exhibit. No missing client UI or
unwritten mix remains as a reason to hold this draft.

---

## 9. No application-level per-frame hook or input capture — **draft with console exhibit**

**Exhibit added by the developer debug system (phase 1).** A client-wide
developer console (Debug only; `game/debug/`, `docs/debug-console.md`) needs
two things the engine does not offer: somewhere to look at the keyboard every
frame whatever state is on top, and a way to say "this key press is mine".

### What happens

`Application::update` polls the devices and then updates the top state only
(`application.cpp`, `StateContext::update`). There is no hook before or after
the stack, and `Keyboard` has no notion of a consumed key. So the console:

- turned `MenuPage::update` into a non-virtual template method (`update` calls
  `DebugDirector::before_page_update`, then the page's `update_page`, across 15
  overrides, the Debug menu's among them) and added a hook at the top of `GameLevel::update`, purely so the
  toggle key is seen on every screen;
- is a pushed, non-covering state — the only way to keep `PlayerInput` and
  `MenuInput` off the keyboard while someone types. The cost is the real one:
  **the world must freeze while the console is open.** A see-through console
  that tunes a live match while it runs was built by gating by hand (phase 4,
  below), and this is the bill;
- implements its own repeat for Backspace, Delete and the arrows
  (`DebugKeyRepeat`), because only typed text repeats.

### The ask, when it goes

A pre-stack (or overlay-layer) update hook that runs every frame, and a
`Keyboard` capture or consume scope, so an overlay can own the keyboard while
the states under it keep updating. Answering it would let this repository
delete the `update_page` template method, the two `before_*_update` hooks and
the frozen-world limitation. Control-key repeat is a smaller, separate ask.

**Exhibit added by phase 2 (the debug menu's pad chord).** Holding pad
Back+Start opens the debug menu, and the two releases that follow must reach
neither the per-player F1 overlay (Back) nor the pause menu (Start). `Gamepads`
has no consume scope any more than `Keyboard` does, so the game removes the
edges by hand in two places: `DebugDirector::filter_match_inputs` clears
`toggle_debug` and `toggle_pause_menu` from the `PlayerInputData` that
`GameLevel::update` just read, and every page's `MenuInput` arming is reset
(`DebugPageTarget::debug_reset_input`) on every frame of the chord, which also
costs any other player's A press held across those frames. A consume scope on
`Gamepads`, as asked above for `Keyboard`, would delete both.

**Exhibit added by phase 4 (the live console).** `console.live 1` opens the
console over a match that keeps running. With no consume scope, the game gates
the readers itself, and pays for it:

- **The whole keyboard seat stands still while it is open**, not only the keys
  being typed: `filter_match_inputs` replaces the keyboard/mouse slot's
  `PlayerInputData` with a connected, idle one, because the game cannot tell a
  typed `d` from a held move-right. A keyboard player cannot play and type.
- **Mouse aim still follows the cursor**, because `Level::resolve_mouse_aim`
  reads the `Mouse` directly; only the shot (a `PlayerInputData` field) is
  blanked.
- **Every in-match keyboard reader is gated by hand.** The director skips its
  own binds while the live console is open; F1 and Escape are covered only
  because they arrive through the blanked slot; F10 is not (it is read by the
  director's hook before the console), so it still opens the debug menu, which
  closes the live console.
- **The closing frame needs its own blank.** The console closes on the Escape
  release so the edge is over before anyone else reads it - but the match
  is not frozen any more, and that same release is the keyboard player's
  `toggle_pause_menu`. A one-shot flag blanks the seat on the frame it closes.
- **It has its own draw path**: the director draws the extracted
  `DebugConsoleView` from `draw_status`, the match's last draw, because there is
  no overlay layer above the stack.
- **It exists only over a match.** Over a menu page the console is still the
  frozen pushed state.

On the plus side it causes no `Level::suspend` (no state is pushed), so typing
during a recording no longer desynchronises the replay. The ask is unchanged;
answering it would also let this repository delete the keyboard-slot blank, the
closing-frame blank and the live view's separate draw path.

---

## 10. No way to read back the frame that was presented — **draft with screenshot exhibit**

**Exhibit added by the developer debug system (phase 2).** The Debug
`screenshot` command saves the window as a PNG.

### What happens

`Renderer::read_back_buffer` is legal only between `submit()` and
`end_frame()` (`renderer.h`), and `Application::render` runs those two back to
back with no client hook between them (`application.cpp`). So the game cannot
read the frame it is about to present. `capture_stack` instead renders the
whole stack a second time, out of band, from the top state's update:
`begin_frame`, `Application::draw`, `submit`, `read_back_buffer`, and never
`end_frame`, which is the sequence the render probes under `scripts/acceptance/`
already rely on. The next `Application::render` begins a fresh frame.

It works on the one backend this repository builds (`d3d11`, checked by
`scripts/check-debug-console.ps1`), at the cost of a second full draw, and it
depends on `begin_frame` being callable outside `render()` with a frame left
unpresented, which the seam does say a frame may be ("contributes nothing")
but which only the probes' backends exercise.

### The ask, when it goes

`Application::request_capture(callback)`: read back the next presented frame
between its own `submit` and `end_frame` and hand the pixels to the callback.
Answering it would delete `capture_stack` and its out-of-band frame.

---

## 11. F10 and a lone Alt belong to the system menu, with no opt-out — **draft, low priority**

**Exhibit added by the developer debug system (phase 2).** The debug menu
wanted F10.

### What happens

The window procedure forwards `WM_SYSKEYDOWN`/`WM_SYSKEYUP` to the keyboard
and then, deliberately, to `DefWindowProc` ("Alt+F4 and F10 are the system's",
`engine/app/win32/window.cpp`). On a `WS_OVERLAPPEDWINDOW`, `DefWindowProc` turns an F10 or a
lone Alt release into `SC_KEYMENU` and enters the system menu's modal loop:
`Application`'s pump stops ticking until another key or a click ends menu mode,
and that key is eaten. `scripts/check-debug-console.ps1` records it for Alt
(`alt-probe.txt`: "A lone Alt tap entered the system menu loop"). A game cannot
bind F10, and a player who taps Alt freezes the game for a key press.

The game's way round it is a thread message hook (`SetWindowsHookExW`,
`WH_GETMESSAGE`), installed by the Debug director only, that rewrites F10
key messages to `WM_NULL` before the window procedure sees them and counts the
press itself. It works, and it is the kind of thing a client should not have
to write: a hook over the engine's own message pump, to undo a policy the
engine chose for it.

### The ask, when it goes

An `ApplicationOptions` flag that swallows keyboard-initiated `SC_KEYMENU`
(`WM_SYSCOMMAND` with `lParam` 0) while leaving Alt+F4 alone, so F10 and Alt
reach the game as ordinary keys. Answering it would delete the hook, and with
it F11 as the debug menu's fallback key.

---

## 12. JSON is read-only, path-only and opaque — **draft with live-tuning exhibit**

**Exhibit added by the developer debug system (phase 3).** The Debug `tune`
command edits any number or boolean in `tuning.json` and `presentation.json`
while the game runs, and `tune.save` writes the edits back into the source
files.

### What happens

`read_json_file(const char* path)` is the engine's only way in
(`engine/assets/json.h`). There is no parse of text already in memory, a
`JsonValue` cannot list an object's keys, and nothing says where in the file a
value came from. Live tuning needs all three, so it does without:

- **Every candidate edit goes through a temporary file.** To judge an edit by
  the game's own rules (ranges, cross-field checks), `tune` splices the new
  token into the text, writes it to `%TEMP%\ColourWars-tune-<pid>-<file>.json`
  (until phase 4 a folder, `ColourWars-tune-<pid>\`, whose removal raced the
  reader's handle and left empty folders behind; the first edit now sweeps
  those) and constructs `GameTuning` or `Presentation` from that path
  (`game/debug/tune_commands.cpp`, `tune_validate_and_apply`). The reader's
  error names the temporary file, so the message is rewritten before the
  console shows it, and the copied-over live object keeps the temporary path
  as its source.
- **The game carries its own scanner to find the values at all.**
  `game/debug/json_spans.cpp` runs rapidjson's SAX reader with the engine's
  flags, keeps the key path, and recovers each number's byte span by scanning
  around the stream position, because the reader writes its position back
  only after the number callback returns. That is what lets the console list
  `tuning.player.gravity`, complete it, and replace exactly its bytes.
- **A dead key cannot be told from a live one** except by experiment: the
  acceptance case replaces every number and boolean, one at a time, with a
  string and requires the reader to throw naming it
  (`check_tune_consumed` in `scripts/acceptance/debug_console/tune_checks.cpp`;
  some five hundred values today).

### The ask, when it goes

`parse_json(std::string_view text, std::string name)` returning a
`JsonDocument`, `JsonValue::keys()`, and optionally `JsonValue::source_span()`
(byte offsets into the text). Answering the first would delete the temporary
file and the message rewrite; the second and third would delete
`json_spans.cpp`, and with `keys()` the consumed-leaf check could become a
reader-side "unknown key" error at startup instead of a test.

**None of this is a write.** The engine's product documents file this report as
"JSON write" against the read-only asymmetry `ARCHITECTURE.md` settles. The ask
is reading: parse from memory, list keys, say where a value came from. Say so
in the first line when it goes.

---

## 13. `Key` has no names — **draft, low priority**

**Exhibit added by the developer debug system (phase 4).** `bind <key> <line>`
and `binds.cfg` name keys in text.

### What happens

`labrador::Key` is an enum and nothing more: no `key_name(Key)`, no
`key_from_name`. So `game/debug/debug_keys.cpp` carries its own table of about
eighty names (`a`-`z`, `0`-`9`, `f1`-`f12`, `numpad0`-`9`, `pageup`,
`lbracket`, ...) built against the enum's order. Every console, bind file or
rebinding screen a client writes needs the same table, and each will spell the
keys its own way; a key added to the enum is silently unnamed in all of them.

### The ask, when it goes

`std::string_view key_name(Key)` and `std::optional<Key> key_from_name(std::string_view)`
next to the enum, with one spelling per key. Answering it would delete the
table in `debug_keys.cpp` (the `ctrl+`/`shift+` chord grammar stays the game's).

---

## 14. No frame statistics or GPU timing — **draft with perf exhibit**

**Exhibit added by the developer debug system (phase 4).** The Debug perf
overlay (`overlay perf`) shows frame, update and draw time.

### What happens

`Application` exposes no per-frame statistics (frame interval, time in update,
render and present) and `Renderer` no timestamp queries. So the overlay times
only what the game itself calls, with `QueryPerformanceCounter`: each
`Level::update` inside the debug clock's run loop, and the match draw from a
new hook at the top of `GameLevel::draw` to the director's `draw_status`; the
frame interval is the time between two match draws. The render submit, present
(and so vsync), the GPU and everything the engine does between states are
invisible, and the panel has to say "CPU". A screenshot's out-of-band redraw
(report 10) re-enters the same hooks, so the game suppresses its samples by
hand.

### The ask, when it goes

`Application::frame_stats()` (the last frame's update, render-record, submit
and present times) and, later, GPU timestamp queries on `Renderer`. Answering
the first would delete the draw hook and the screenshot suppression, and let the
overlay show the whole frame.

The engine defers a frame-time overlay of its own (its `TODO.md` REV#10, "waits
for a client that needs it"). This exhibit is that client; cite REV#10 when
filing, and ask for the accessor rather than an engine-drawn overlay.

---

## 15. Runtime packaging and content staging are repeated in the client — **draft with release exhibit**

**Exhibit: a working release, not a proposed packaging system.**
[game/CMakeLists.txt](../../game/CMakeLists.txt) defines `colourwars_content`,
an always-run target that mirrors assets beside the executable even when only
content changes. It enumerates entries to exclude source waves and tools,
and calls [stage_wave_bank.cmake](../../cmake/stage_wave_bank.cmake) separately
so a newly generated bank is copied and a removed bank disappears from the
runtime mirror. [release_package.cmake](../../cmake/release_package.cmake)
then owns install rules and CPack ZIP generation;
[release_assets.cmake](../../cmake/release_assets.cmake) owns the reviewed
redistributable file list and checks it against the manifest.

### What happens

The game's development mirror and release tree have different contents. The
mirror can contain the local legacy wave bank; the release list includes its
JSON definition for the silent fallback but excludes the bank, source audio
and tools. The release script follows `TARGET_RUNTIME_DLLS:ColourWars`, adds
the MSVC runtime with `InstallRequiredSystemLibraries`, copies third-party
notices, and refuses configurations outside this game's supported Release
x64 D3D11/XAudio2 build. CI's **Extract and inspect Release package** step in
[ci.yml](../../.github/workflows/ci.yml) checks required DLLs and rejects audio
and development-file leaks. A clean-PC run remains `ROAD#1`; a package that
passes this inspection is not evidence of that run.

This already works. The remaining cost is that a game building on Labrador
must discover and encode the engine's runtime dependencies and content-root
layout itself, alongside the game's own shipping choices. A dependency or
loader-layout change at a pin bump makes the client recheck those rules. The
minimal sample independently spells its development copies in
`samples/minimal/CMakeLists.txt`; it explicitly says it has no install rules.

Checked against published engine master `52e809a` on 2026-10-15: the tracked
`CMakeLists.txt` and `*.cmake` files have no `install(...)`, `export(...)`,
CPack setup, package-config generation, or shared content-staging helper.
`engine/app/content_root.h` owns executable-relative resolution; the game
owns the matching build layout. Engine product stories
[PI-09](../../external/labrador/docs/product/STORIES.md#pi-09),
[PI-10](../../external/labrador/docs/product/STORIES.md#pi-10) and
[PI-11](../../external/labrador/docs/product/STORIES.md#pi-11) already describe
filing this demand, deciding it, and conditionally supplying runtime install
support. Those stories do not close the implementation gap.

The frozen gap audit's F8 is broader than this evidence now supports. This
game has a supported shipping mechanism: its own install/CPack rules. Missing
engine install/export/package-config support also means it supplies no CMake
package for a `find_package(Labrador)` consumer, but ColourWars uses
`add_subdirectory` and has no prebuilt-package workaround to delete. That is
context, not a second demand from this caller.

### The ask, when it goes

Give a consuming target a documented CMake helper for the runtime files the
engine makes it need and for placing a **caller-supplied** content list where
the loader expects it. On the current Windows build that means following the
runtime DLL targets, adding the redistributable MSVC runtime and refusing a
debug CRT. A development-staging helper should run on asset-only builds and
handle explicitly named generated optional files, including removing their
stale staged copies. It must work when Labrador is a subdirectory without
adding sample/test targets or unrelated install entries to the consumer.
The spelling and whether development and install use one function are open;
PI-11's narrower runtime-install scope can be answered separately.

An answer would let this game replace the runtime dependency/CRT plumbing in
`release_package.cmake` and, if staging is supplied, the generic copy mechanics
in `colourwars_content` and `stage_wave_bank.cmake`. The game keeps its asset
allowlist and manifest check, optional-bank recipe and shipping exclusion,
Release/backend restrictions, ZIP name, build stamp, notices selection and
CI assertions. The helper must preserve those choices; copying the content
or development output directory wholesale would not answer this report.

**Status:** unfiled draft. Recheck engine master before filing; record an
implemented answer or a decline with its trade-off here, as for the other
live reports.

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
and its `window_active_` member, and `MainMenuTitle::update`'s poll (the page's
hook is `update_page` now).

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

Re-checked 2026-08-15 against `6b5c0bd`; citations re-pointed 2026-09-17 at
`984cf13`, where the comment sweeps moved every one of them by a few lines and
changed none of the facts.

- **Who owns a resolution-independent world height.** This entry used to say
  `CameraTools` never sets a scale, so a 4K player saw nine times the world a
  720p player did. That stopped being true here, not in the engine: `fbd45dc`
  added `PlayerCamera::follow` (`game/objects/player_camera.cpp`), which sets
  `scale` from the screen height over `presentation.json`'s
  `camera.reference_height` of 1080 and passes the visible rectangle in world
  units, as `camera_tools.h:13-24` asks. What is left is a question, not a
  defect: whether the engine's camera should own that reference height, which
  `PlayerCamera` now re-derives along with centring an axis wider than the
  arena. `PlayerCamera` is the exhibit if it is ever asked.

- **`Renderer::window_size_changed` returns a signal `Application` ignores**
  (`application.cpp:436,463`, both `std::ignore =`), and **nothing reports the
  desktop size**. Both still true.

  **But the reason this was parked has expired.** It was filed under "one part
  of the larger *layout follows the real window* question" — and `0702776`
  answered that question: `on_window_size_changed` now re-points
  `ResolutionManager` at the real client size before the renderer resizes, so
  every route into a resize holds the invariant.

  **The workaround this left behind is gone**, and driving the page proved it
  was hiding two bugs rather than one. The coercion in
  `MainMenuOptions::apply_settings` is deleted: the layout follows the window
  on its own now, and `Application` remembers the windowed size to return to.
  What the page kept is a display rule — while fullscreen owns the size the
  Resolution row shows `resolution_ivec()` and is disabled, because
  `resolution()` is a label and on a 1440p monitor it sat there reading
  1920x1080 while the game ran at 2560x1440.

  **One thing seen once and not reproduced, worth knowing before someone
  writes the same sequence again.** `Application::set_resolution` immediately
  followed by `set_fullscreen(true)` resizes the window twice in one call, each
  a synchronous WM_SIZE into the renderer's swap chain, from inside a window
  procedure. That killed the process once with
  STATUS_FATAL_USER_CALLBACK_EXCEPTION (`0xc000041d`) — an exception escaping
  the window proc, so `wWinMain`'s handler never saw it and nothing was printed.
  It did not recur in the next four runs, and the page no longer makes that
  call pair at all, so there is no exhibit and no report. If it is ever seen
  again with a reproduction, the ask writes itself: an engine that throws out
  of its own window procedure dies silently, which is the one failure mode T6
  is meant to rule out.

- **The write side of `json.h`.** Still absent, still correct (the engine's
  OCA-53 plans a guide, not an API, and declines a save facility under T1):
  `ARCHITECTURE.md` names the read-only asymmetry and settles it, and T12
  declines automatic serialisation. What survives is narrower — the ~50-line
  atomic-write protocol in `game/save.cpp` is entirely game-agnostic and no
  second client should re-derive it — and is a small ask best made alongside
  something else.

- **`Scene::contacts()` having no reader** (`scene.h:200`), **`labrador::Label`
  having no client** (`engine/render/label.h:26` — it moved out of `scene/`),
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

---

# What the 2026-08-27 bump changed

Sixty-five commits, `343433c` → `c8176ef`. Read straight after the pin moved,
because this file's whole point is that a pin bump is not consumed until
somebody has asked it what it closed.

**It closed nothing here.** Report 6 is the only live one and the audio work in
this bump went past it rather than through it — see the report itself, where the
seam has made the ask larger rather than smaller. There is nothing to delete
from this repository on the strength of these sixty-five commits, which is worth
writing down: the 2026-08-15 bump closed five reports and produced four commits
of deletions, and "no deletions this time" is a finding rather than an omission.

**One thing broke, and it was an include.** `game/pch.h` named
`engine/core/throw_if_failed.h`, which `b01bc22` ("Take Windows out of core/")
moved to `engine/render/`. Nothing in `game/` has ever called `ThrowIfFailed` or
caught a `com_exception`, and the engine header now says in its own comment that
its only includers are the two Direct3D backends — so the line went rather than
followed the file. Both presets configure and build clean under `/W4 /WX` after
that one deletion.

**Four things that read like breaks and are not**, each checked rather than
assumed:

- **`Direction` moved** out of `engine/ui/navigation.h` into
  `engine/input/direction.h`. `navigation.h` includes it, so every
  `labrador::Direction` site in the menu states compiles untouched.
- **`DirectX::SoundState` became `labrador::SoundState`**, and the enumerators
  lost their shouting (`STOPPED` → `stopped`). Nothing in this repository names
  the type or reads a state.
- **`SoundBank::silent()` now refuses an unresolved handle** where it used to
  accept one, so a build with no wave bank and a build with one no longer
  disagree about whether a handle nobody resolved is a mistake. The only handle
  here that can be unresolved is `Weapon::loop_sound_` — the sniper and the
  bucket have no looping voice — and all five of its uses were already behind a
  `.valid()` guard, so the stricter engine reaches none of them.
- **`SoundBankObject`'s default constructor now throws** `std::logic_error`
  instead of dereferencing null in somebody else's frame. `MenuPage` is the only
  thing here that derives from it and always passes the `AudioResources`.

**And one where the engine read this repository before deciding.** `169a3c0`
refuses a sheet frame with `"rotated": true` by name and starts honouring
`"origin"`, and its message says it went and looked at
`game/content/textures/sprite_sheet_1.json` rather than guessing: 41 frames, 9
strips, one frame mentioning either key, and both values the identity. Verified
here — the refusal rejects nothing this game has, and the origin composition
moves nothing it draws.

## What was offered and not taken

`engine/input/direction.h` ships `stick_direction`, `pad_direction` and
`DirectionRepeat`, described in its own header as "the three things every client
wiring a menu writes by hand", and this client wrote none of them. That is not
an oversight to correct: `MenuInput::menu_input` fires a direction on the
**release** edge, against a presence/absence hysteresis band, so a held stick is
one step and never a stream — and the d-pad half is the same release rule rather
than the engine's "d-pad wins a disagreement". Hold-to-repeat is a different
model, not the model this client hand-rolled, so there is no workaround here for
`DirectionRepeat` to replace.

Worth recording because the direction of the finding runs the other way for
once. The engine built for a menu client, named the menu client's three
problems, and the menu client has a fourth answer it arrived at on purpose. If
that is worth a report it is a report about the release edge, and it needs the
same thing report 6 needs: an exhibit, which in this case would be a page that
wants a repeat and cannot have one.

---

# What the 2026-09-17 bump changed

Twenty-nine commits, `c8176ef` → `984cf13`: the engine's GPT-6 review series
applied, on top of a comment sweep across the whole of `engine/`. Read straight
after the pin moved, as this file asks.

**It closed nothing here.** Report 6 is still the only live one. The seven
audio files in this bump are the comment sweep and nothing else — across every
engine header outside `render/` and `math/`, the only non-comment line that
changed is `DirectionRepeat::start_held`, and
[What was offered and not taken](#what-was-offered-and-not-taken) already
records why this client has no `DirectionRepeat` for it to matter to.

**Nothing broke.** Both presets built clean under `/W4 /WX` with no change on
this side, where the 2026-08-27 bump cost an include. The first build step is
new, though: `check_engine_includes` is now a dependency of `labrador_settings`
(`3a70b82`), so it runs when a consumer builds only its game target and not
just in the default build. CLAUDE.md's "the only thing enforcing that
direction from this tree" is now enforced on every build here rather than on
most of them.

**One commit reached this repository's loading order and stopped short, and
this bump was spent finishing it.** `92966b0` makes `Application::load_manifest`
resolve a relative manifest path beside the executable and every relative
directory inside the manifest beside the manifest, so a game runs from
wherever it is started — the engine's samples had said they did and did not
(its G6-09). This game reads `tuning.json` and `presentation.json` *before* the
manifest, by design, so the rule did not reach them: the pin bump alone turned
a game that failed on `./manifest.json` from the wrong start directory into one
that failed on `./tuning.json` from the same place, with the manifest sitting
beside it and now resolvable. Reproduced before fixing — launched from the
repository root, `startup failure: read_json_file - cannot open
'./tuning.json'`. `content_root.h` anticipated exactly this: its pieces are
public "so that a game with its own loading order can apply them itself", and
this is that game.

**Spent in this commit.** `main.cpp` resolves both files under
`executable_directory()` before reading them, with the manifest left to the
engine. `legacy_save_file_path()` resolves the same way, and that is not a
courtesy: the old game could only run with its content in the working
directory, so beside the exe is where every save it ever wrote is, and the
carry-over now finds it from any start directory. CLAUDE.md and the CMake
comment said "relative to its working directory" and say the opposite now. And
CI's start-up check launches from the workspace root instead of from beside
the exe, because started from the exe's own folder the two roots coincide and
the check proved nothing about which one was used. Verified from the
repository root and from `C:\Windows\Temp`: the window comes up, stderr empty.

Worth writing down what kind of finding this is, because it is not a report.
This client had no workaround here; it had the bug, and README's "runs from
anywhere" was untrue until today, unproven because CI launched from the one
directory where it could not fail. The engine found it in its own sample, fixed
it, and the fix arrived with one seam left for a client with a different
loading order to close. That is the boundary working in the direction this
file does not usually record.

**Four fixes that read as though they reach here, and do not**, each checked
rather than assumed:

- **`Visual::bounds()` reports the drawn box now**, origin and rotation
  included, so the cull agrees with the draw (`8dcab46`). Both `Visual`s this
  game builds — the level objects of `"type": "Visual"` and the viewport
  dividers, `level_object_builder.cpp` — pass no origin and no turn, and the
  one sheet frame naming an origin is the identity (recorded in the 2026-08-27
  section). The box is the destination rectangle it always was.
- **The math constants are `constinit`** (`e03606f`). The review's cross-TU
  bug needs a namespace-scope object in a client copying one of them before
  the engine's own initialisers have run. Every `Vector2F::ZERO`,
  `RectangleF::ZERO` and `Camera::DEFAULT_CAMERA` in `game/` is a member
  default or a default argument, evaluated at construction. Two things sit at
  namespace scope: `Colour::black` copied into a divider colour, and `Colour`
  was already `constexpr`; and `menu_consts::DESIGN_RESOLUTION`
  (`menu_page.h:24`), built from two literals through a constructor that was
  out of line at `c8176ef` and is `constexpr` now — so it *was* dynamically
  initialised, and is not any more, and it never mattered because its fifteen
  readers are all inside functions. Nothing here was reading zeros.
- **A segment through a polygon's two corners is a hit now** (`d373f71`). The
  three routines it fixed are not on this game's path: its one `intersects.h`
  call is `rectangle_point_intersect` (`level_loaded_info.cpp:74`), and the
  narrow phase is a separating-axis test over polygon edges, which the fix
  does not touch. Report 7's argument for keeping `intersects` — that a client
  should not have to write segment-against-rectangle itself — is still an
  argument about a call this client does not yet make.
- **`Window` destroys the native window on every exit** (`fb542b1`), and
  `~StateContext` drains its own stack (`eb7336f`). `wWinMain`'s catch prints
  and opens a `MessageBoxA`; before this bump, a throw after
  `app.initialize()` — a manifest that does not open — unwound `Application`
  with the native window alive and its user data pointing at a destroyed
  object, and the message box's modal loop was a way back into that window
  procedure. Fixed on the engine's side, no change here: the catch was always
  the shape the engine's samples have, and the engine made that shape safe
  rather than asking clients to change it. The drain is report 1's fix
  reaching a destructor this game never runs directly.

**Citations moved, facts did not.** Report 6: `sound_bank.h:130,136` →
`129,135`, `application.h:244` → `252`. Under "deliberately not here":
`camera_tools.h:13-27` → `13-28`, `application.cpp:384,411` → `389,416`.
`scene.h:198` and `label.h:28` did not move. Every engine-side one is a
comment paragraph growing or shrinking above the line. And one citation had
drifted on *this* side without a pin bump to blame — `level.cpp:84,385` for
the pixel-sized `calculate_camera` calls is `91,472` now, and has been since
this repository's own edits to that file. The line numbers here are checked
against the engine at every bump and against the game only when someone
notices, which is the honest description of the rule.

---

# What the 2026-09-26 bump changed

Moved from `d356841` to upstream master at `4cefb68`, consuming the September
26 review fixes and the intervening presentation work. The earlier UTF-8 path
fix remains available in upstream's implementation, including the helpers
used by this game's save handling.

**No game source changes were required.** Report 6 remains open: there is
still no shared music/SFX bus or master gain to replace the client's volume
handling. The Scene composition changes support the existing fullscreen
menu/countdown draws after the gameplay views. This client does not retain
Scene contact snapshots, and its Visual builders use axis-aligned rectangles
with identity origins, so those fixes require no migration here.

**Validation:** both `x64-debug` and `x64-release` configured and built under
`/W4 /WX`, including the engine architecture checks. Release packaging passed
the CI checks for required DLLs and excluded development/commercial-audio files.
Both executables rendered the title screen when launched from
`C:\Windows\Temp`. The extracted Release also used Unicode installation and
isolated save paths, produced its save successfully, and rendered correctly
after maximizing and restoring the window. Debug stderr was empty; Release
reported only the expected optional-wave-bank fallback. Local build and startup
logs are under `out/validation/labrador-20260926/`.

**Gameplay remains unverified by this check.** The UI automation's Enter/Space
presses did not advance the title screen. The keyboard snapshots live state at
each poll and the menu requires a press followed by a later release, so a short
synthetic pair can be missed; this does not establish an input regression.
No controller, match, or audio-playback validation is claimed.

# What the 2026-10-14 bump changed

Moved from `4cefb68` to upstream master at `52e809a`: seventy-five commits, the
`v0.1.0` tag among them. Most are the web port, the website and documentation.
The engine now keeps `CHANGES.md`, a list of what a game can see change, with a
backfill from `4cefb68` written for exactly this move; it was followed item by
item.

**Source changes, all from `4175640`.** `app.initialize(instance, show_command)`
became `app.initialize()` in `game/main.cpp`, whose `wWinMain` stops naming the
two parameters. The three window-driven probes
(`scripts/acceptance/*_visual.cpp`) passed `SW_HIDE` and now set
`ApplicationOptions::visible = false`. Nothing else in the backfill reached this
tree: no file got a Win32 name through `application.h`, nothing calls
`Application::window()`, makes a `Window` or implements `WindowNotify`, and the
one direct `GamepadReader` (`bot_checks.cpp`) never reads it.

**Two behaviour changes with no compile error.** Both are Windows-only and
neither needs code here.

- **Touch is no longer a mouse click** (`eaa9b3f`). A tap in a match used to
  paint and aim the keyboard/mouse seat; now a finger does nothing, because
  nothing reads `Application::touch()`. Menus never took the mouse. Playing by
  touch would be a new feature. Not checked: this machine has no touchscreen.
- **A window restored without activation stays inactive** (`ea90387`). A match
  restored behind another window stays frozen and silent until it is clicked,
  as after an alt-tab; every page already gates on `context()->active()`, so no
  missing-pad pause can open meanwhile. Menu music still resumes with the
  device. Not driven.

**Closed: nothing.** Each live report and each parked ask was re-read against
master with its exhibits. The engine now tracks the seven drafts as one item
(its `TODO.md` PI-09, waiting on what ColourWars detail may be published, and
`STORIES.md` PI-10, deciding each one), and asks that a decline cite the
passage it rests on. The corrections this pass made are in place above: report
6's exhibit list and line numbers, report 9's override count, report 11's moved
window file, report 12's check location and its "not a write" framing, report
14's REV#10, report 2's `update_page`, and the parked engine citations.

**What this repository had already answered.** The camera entry under
[What is deliberately not here](#what-is-deliberately-not-here) described a
4K player seeing nine times the world. `PlayerCamera` has fixed a 1080-unit
world height since `fbd45dc`; the entry is now the narrower ownership question.

**Validation:** `x64-debug` and `x64-release` reconfigured and built under
`/W4 /WX`; consumer acceptance passed on both, `check-startup` passed on both,
and the movement-feedback, workshop-playable and workshop-style probes passed
with their windows hidden through `visible`. No pad, touchscreen or audio
playback was exercised.
