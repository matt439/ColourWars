# Reports to file against Labrador

> The running list of API complaints this client owes the engine. Each one is a
> case where the game wanted something ordinary and the API forced a bad
> answer, and each carries an **exhibit**: the workaround that is in this
> repository, in a file you can open, written because the engine offered
> nothing better.
>
> **Seven are live as unfiled drafts with exhibits (6, 12, 15, 18, 20, 22, 23). Sixteen are closed** and kept below as one paragraph
> each — what was asked, what the engine answered (shipped or declined), and
> what this repository spent the answer on, if anything.
>
> They are drafted here rather than in the submodule on purpose. AGENTS.md puts
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

**Moved 2026-10-22 to upstream master `0c15f34`.** Two commits: the engine's
agent brief became `AGENTS.md` imported by `CLAUDE.md`, plus website
explorations. Under `engine/` only a comment in `engine/CMakeLists.txt`
changed, so nothing closed and no citation here moved; both presets, consumer
acceptance and the startup check pass on the new pin.

## The rule that governs this file

**Check the engine's master before filing, not the pin.** `external/labrador`
is pinned to a revision; the engine is not. Report 1 was drafted against the
pin and described a bug the engine had fixed the day after that pin was taken —
four days before anyone wrote the report down. Every citation below was checked
against `6b5c0bd`, and `6b5c0bd` will not be master for long.

## Publication gate (`ENG#PUBLISH`)

**Disclosure scope approved (2026-10-20):** the author approved minimal
engine-focused reports with reproducible examples. This resolves `ENG#PUBLISH`.
Use Labrador's established [issue-tracker route](../../external/labrador/CONTRIBUTING.md#asking-a-question-or-requesting-a-change),
choosing the bug or engine-request form appropriate to the finding. Labrador's
`PI-09` tracks filing; its older seven-draft count does not supersede the
live ledger below.

Each report should contain only the relevant engine API/behaviour, the checked
upstream-master commit and necessary build conditions, minimal reproduction
steps or a small standalone example, actual and expected results, and a short
description of the client workaround an engine answer would remove. Use
synthetic content where an example needs assets or data. Keep the full private
drafts, unrelated game source, game assets and private captures/data out of the
public report. Requests beyond what the example demonstrates remain proposals
to scope separately.

Thirteen drafts were present at approval; report 21 later added a fourteenth
and report 22 (2026-10-22) a fifteenth. Labrador answered 9, 10, 11, 13, 14,
16, 17, 19 and 21 before any was filed, and all nine are now [closed](#closed),
so six of those fifteen drafts remain unfiled (6, 12, 15, 18, 20 and 22);
report 22 is checked only against `0c15f34` so far. Report 23 (2026-10-22), found while spending 16, is a sixteenth, checked
against `19cf650`. Recording this approval posts no reports and leaves
the engine pin unchanged. The next work is a fresh upstream-master
check and preparation of the minimal report/reproducer for each surviving gap;
filing uses the approved scope and established route above. This supporting
work does not change TODO's level-first priority.

**Provenance re-checked 2026-10-21:** a [fresh live-master read](../../out/validation/crossfeed-20261020/engine-reports-refresh-20261021/live-master-check.json)
at 02:33:22 UTC still reported upstream `52e809a5a0229d37db4373faa7853df4c861a310`.
The [source recheck](../../out/validation/crossfeed-20261020/engine-reports-refresh-20261021/source-reconciliation-v1/freeze-v1.json)
matches all fourteen already-tested exhibit sources and 55 retained engine-file
pins; two known build-recipe edits change comments only. No builds or exhibits
were rerun, and no new runtime failure is recorded; the 2026-10-20 execution
records remain historical. All fourteen drafts remain unfiled under the approved
minimal-report scope and established issue route above. No public filing or
engine pin change occurred.

Before authorized filing, refresh engine master from upstream and recheck the
concrete client exhibit against that revision. Record the checked commit with
the report. A cached `origin/master` ref or local branch is not evidence of
live upstream master. An answered claim is withdrawn with a reason; an
unanswered one is filed only with the details and destination the author has
authorized.

## Why the exhibits matter more than the asks

`PHILOSOPHY.md` declines speculative frameworks, and the engine has been
consistent about it: `gamepad.h` used to say there was no action map because
no client had "a rebinding screen to fill such a table from". When this client
grew one, the engine did not take the exhibit as a reason to build the map; it
re-took the absence on firmer ground, the boundary table that gives bindings to
the game (report 21, [Closed](#21-digital-action-evaluation-remains-client-code--declined-197aff1)).
That is the right call, and it means a report saying "an engine should have X"
is worth nothing. A report saying "here is the code I wrote because it did not" is worth
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
| 6 | Shared Music/SFX attenuation remains emitter work | per-emitter saved gains and standalone `audio_gain_exhibit.cpp` | local unfiled draft - master checked 2026-10-20; synthetic Debug/Release runs pass |
| 12 | JSON is read-only, path-only and opaque | the `%TEMP%` candidate file in `tune_commands.cpp`, `json_spans.cpp`, `check_tune_consumed` | draft — exhibit present; recheck upstream before filing |
| 15 | Runtime packaging and content staging are repeated in the client | existing client release/staging rules and the synthetic `runtime-staging/` fixture | local unfiled draft - staging slice checked against master 2026-10-20; Debug/Release phases pass; runtime install remains separate |
| 23 | Resizing a hidden window activates it | `SettingsProbeActivationGuard`, the `WH_CBT` veto five painter fixtures hold around `set_resolution` | local unfiled draft - checked against pin `19cf650` 2026-10-22; no sanitized request or synthetic example yet |

Nothing else is outstanding, on either side, beyond the live reports below and the
evidence [report 19](#19-no-public-later-show-convenience-for-a-hidden-application--needs-more-evidence-cacf112)
leaves this repository to supply. The fullscreen workaround the
audit flagged under [What is deliberately not here](#what-is-deliberately-not-here)
has since been driven and fixed — keyboard support made the Options page
reachable on a machine with no pad.

---

## 6. No audio bus, master gain, listener or voice cap — **draft with mixing exhibit**

**Fileable slice prepared 2026-10-20: shared Music/SFX attenuation.** The
client's saved gains in `game/play_settings.h` are multiplied into per-emitter
calls in `game/states/menu_page.cpp`, `game/objects/weapon.cpp`,
`game/objects/level_audio.cpp` and `game/states/screen_music.cpp`. Music also
keeps authored level, fade and pause ducking; resume paths reapply the user
gain to individual retained effects. This is the concrete repeated work.

The [sanitized request](../engine-reports/audio-attenuation.md) and
[standalone synthetic exhibit](../../scripts/engine-reports/audio_gain_exhibit.cpp)
reduce it to two loops and two independent one-shot emitters in one bank.
The ordinary per-call API works as documented: each emitter supplies its own
volume, and an emitter that omits user-gain multiplication retains its local
level. The request is for extracting shared attenuation, with authored
levels/group assignment/fades/ducking remaining client policy.

**Checked and exercised 2026-10-20.** A fresh upstream fetch at 19:34:53 UTC
confirmed master `52e809a5a0229d37db4373faa7853df4c861a310`; a whole-engine
comparison against that `FETCH_HEAD` passed at 19:50:08 UTC.
`AudioDevice`, `SoundBank` and `AudioResources` still expose no master/group
gain setter. Application's AudioDevice is private, and exposing it alone would
not create a missing operation. `play_wave` returns void; effect setters
address individual retained voices.

The exact synthetic source passed MSVC 19.51.36256.0 under `/W4 /WX` in Debug
(`/MDd /Od /D_DEBUG`) and Release (`/MD /O2 /DNDEBUG`). It compiled directly
with SoundBank, AudioResources, the existing null AudioDevice implementation
and math/scalar.cpp, without an engine static library or CMake configure.
Both runs exited 0 with empty stderr and the same nine asserted recorded
requests/volumes. The report includes the full source list, flags,
reproduction commands, observed output and source SHA-256.

The null backend records requests, not physical mixing or time. This proves
the client's per-emitter arithmetic/refresh and the raw-site negative control;
it establishes no loudness, duration, completion, latency or listening result.
The earlier broad listener/voice-budget asks remain separate unfiled proposals
requiring their own concrete exhibits; they are not bundled into this slice.

Local validation receipts are under
`out/validation/crossfeed-20261020/audio-report-final-v1/`; the master comparison
is in `key-audio-source-recheck.json` beside them. No audio hardware was
opened, and no game/window/assets or engine changes were involved. No issue
has been filed and the engine pin is unchanged. Refresh master before filing.

---

## 12. JSON is read-only, path-only and opaque — **draft with live-tuning exhibit**

**Minimal candidate-parsing slice prepared 2026-10-20.** The sanitized
[request](../engine-reports/json-candidate-parsing.md) and standalone
[synthetic exhibit](../../scripts/engine-reports/json-candidate-file-bridge/json_candidate_file_bridge.cpp)
are ready for review and remain **unfiled**. Fresh upstream master was fetched
at 19:34:53 UTC as `52e809a5a0229d37db4373faa7853df4c861a310`; the checked
HEAD and JSON/file-path sources matched that `FETCH_HEAD`. The exact
three-translation-unit CMake consumer passed MSVC C++20 `/W4 /WX` Debug and
Release build/run checks. Both runs exited 0 with identical expected output
and empty stderr, demonstrating three scratch writes, three removals and
two client diagnostic pathname rewrites.

### Prepared narrow request

The Debug live-tuning client already owns the edited JSON bytes, but must
write a temporary candidate to re-enter its ordinary checked reader
(`game/debug/tune_commands.cpp`, `tune_validate_and_apply`). The reader's
errors name that scratch path, so the client replaces it with the logical
content filename. The prepared request asks only for an in-memory parse
into the same owning `JsonDocument`, with a caller-supplied diagnostic source
label. It is an additional read entry point, not a JSON writer. Once the
client reader accepts that document/root, it could remove the temporary-file
bridge and pathname replacement while retaining its validation/adoption rules.

### Other JSON asks remain open

Object-member/key enumeration and preservation of original source bytes/raw
token spans remain separate, unresolved portions of this report. The client's
`JsonSpanDocument` scanner finds tunable scalar paths and raw token spans;
`TuneDocument` preserves source text and owns splice, save and conflict policy.
A memory parse alone would not delete those mechanisms.

Arrays already support `size()`/`at()`, and filename/key/index diagnostic
context already exists through `source_path()`/`where()`. Enumeration alone
would not turn the consumed-leaf acceptance check into an unknown-key error;
that also requires client knowledge of accepted/consumed fields. Keep these
asks distinct when preparing further exhibits. Review the narrow report and
refresh upstream before authorized filing through the
[publication gate](#publication-gate-engpublish).

---

## 15. Runtime packaging and content staging are repeated in the client — **draft with release exhibit**

**Exhibit: a working release, not a proposed packaging system.**
[game/CMakeLists.txt](../../game/CMakeLists.txt) defines `colourwars_content`,
an always-run target that mirrors assets beside the executable even when only
content changes. It calls
[stage_runtime_assets.cmake](../../cmake/stage_runtime_assets.cmake) to copy the
reviewed runtime list and prune stale assets within four owned content folders.
A removed optional bank disappears from the runtime mirror.
[release_package.cmake](../../cmake/release_package.cmake)
then owns install rules and CPack ZIP generation;
[runtime_assets.cmake](../../cmake/runtime_assets.cmake) owns the reviewed list
shared by staging and packaging; [release_assets.cmake](../../cmake/release_assets.cmake)
checks it against the manifest.

**Development-staging request prepared 2026-10-20.** The sanitized
[report](../engine-reports/runtime-staging.md) and exact tested
[synthetic fixture](../../scripts/engine-reports/runtime-staging/CMakeLists.txt)
are ready for review and remain **unfiled**. This is an optional CMake
convenience/extraction request, not a packaging bug. A maintained documented
recipe is a possible answer. The wider runtime-install proposal below has
no new proof or implementation from this staging fixture.

### What happens

The game's development mirror and release tree use the same runtime content
list. Both include the accepted replacement bank and its definition, excluding
private legacy audio, editable sources and tools. Development can omit the
optional bank for the silent fallback; packaging requires it. The release
script follows `TARGET_RUNTIME_DLLS:ColourWars`, adds
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

Fresh upstream master was fetched on 2026-10-20 at 19:34:53 UTC as
`52e809a5a0229d37db4373faa7853df4c861a310`; the whole engine worktree was
confirmed unchanged against that `FETCH_HEAD` at 19:50:08 UTC. The shared
staging helper is still absent, while executable-relative resolution and the
minimal sample's always-run copy target already exist.

### Measured staging exhibit

The standalone CMake project links only prebuilt engine/Math libraries and
CMake's ordinary Win32 system libraries; it builds no engine/backend targets
and starts no Application or devices. Its console uses the existing public
content-root/path APIs. Its assets are three synthetic text files, with a
caller-owned one-required/one-optional allowlist and one deliberately unlisted
source. It removes only the exact missing declared optional file, never an
unowned directory or broad output tree.

Both MSVC 19.51.36256.0 `/W4 /WX` Debug and Release runs passed baseline,
asset-text edit and optional-source removal phases: six process exits 0 with
empty stderr. Asset-only rebuilds contained no compiler/linker command and
preserved executable SHA-256 and modification time. The unlisted file stayed
out, an unrelated output marker survived, and an executable-relative read
defeated the misleading required file at cwd. Both missing-required builds
exited 1 before any staged-output hash changed. The final fixture produced no
CMake warnings; checks remain active in Release.

Retained v1 Debug configure/build passed, but its phase-0 runtime oracle
correctly failed: `file(WRITE)` generated a CRLF decoy while the binary check
required LF. V2 explicitly generates UNIX newlines and resolves existing
ancestors before appending absent path components, avoiding `REAL_PATH` calls
on files not yet created. No warning was suppressed, and the console oracle,
LF source assets and engine were unchanged. Fresh v2 runs above pass; the v1
failure is a fixture error, not an engine staging failure.

Receipts remain under
`out/validation/crossfeed-20261020/runtime-staging-report-final-v1/` and
`runtime-staging-report-final-v2/`, including the final `result.json`, verbose
build/negative logs, process stdout/stderr and source hashes. The tracked
fixture preserves tested source bytes and uses local Git LF attributes for
its exact text payloads. These runs do not establish install/ZIP/CRT or
clean-machine acceptance.

The frozen gap audit's F8 is broader than this evidence now supports. This
game has a supported shipping mechanism: its own install/CPack rules. Missing
engine install/export/package-config support also means it supplies no CMake
package for a `find_package(Labrador)` consumer, but ColourWars uses
`add_subdirectory` and has no prebuilt-package workaround to delete. That is
context, not a second demand from this caller.

### The bounded ask and remaining runtime-install scope

Consider a documented development-staging convenience taking a consuming
target, source content root and explicit required/optional relative file
lists. It would replace generic always-run copy/removal mechanics in
`colourwars_content`/`stage_runtime_assets.cmake`; a documented recipe may be
sufficient. The caller retains asset selection, manifest/format checks,
generated-file and fallback rules, and any stricter mirror pruning. The
synthetic fixture's single-config MSVC constraints are validation conditions,
not proposed requirements on a generic helper.

The earlier runtime-install proposal remains separate: following engine
runtime DLLs, supplying redistributable CRT files and rejecting a debug CRT
would need its own bounded preparation/validation before claiming an answer.
PI-11 describes that conditional scope. The game retains Release/backend
restrictions, its accepted-bank requirement, package name/layout, build stamp,
notices and CI assertions. No unused `find_package`/export framework or broad
directory copy is requested by this client.

**Status:** local unfiled draft. Review the prepared staging report and refresh
upstream immediately before authorized filing through the publication gate.
Record an engine implementation or a decline with its trade-off here; this
preparation makes neither and changes no engine pin.

---

## 23. Resizing a hidden window activates it — **draft with activation-guard exhibit**

Found 2026-10-22 while spending [report 16](#16-windowed-apply-shows-a-deliberately-hidden-window--7db3d5c),
against the pin `19cf650`; upstream master has not been rechecked since. It is
**unfiled**, and has no sanitized request or synthetic example yet.

### What happens

`Application::set_resolution` calls `Window::resize_client`
(`engine/app/win32/window.cpp`), which sizes with
`SetWindowPos(handle, HWND_TOP, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER)`. With
no `SWP_NOACTIVATE`, the call activates the window even while it is hidden, and
it becomes the foreground window when the process has foreground rights.
Report 16's answer made `enter_fullscreen` and `leave_fullscreen` size a hidden
window with `SWP_NOACTIVATE`, and the `visible` comments now say the engine
never shows a window created hidden; this is the one resize path that still
takes focus. Home alpha's fixture shows it directly: hidden, inactive and not
foreground before `app.set_resolution(1280x720)`; hidden, active and foreground
after.

### Exhibit

`SettingsProbeActivationGuard` in
[save_status_checks.h](../../scripts/acceptance/painter_visual/save_status_checks.h):
a thread-local `WH_CBT` hook that refuses `HCBT_ACTIVATE` for the probe's
window only. Five painter fixtures hold it around a real `set_resolution` -
display trial, Home alpha, Options defaults, paused Options, and settings menu
around its display-trial Applies - because every display trial's callback in
[display_confirmation.cpp](../../game/states/display_confirmation.cpp) calls
`set_resolution`. Without it a hidden probe would steal the desktop's
foreground whenever it had the rights to.

### The ask

That `resize_client` pass `SWP_NOACTIVATE`, or more narrowly that it does so
when the window is hidden, matching `leave_fullscreen`.

### What an answer lets this repository delete

`SettingsProbeActivationGuard` and its five uses; each fixture keeps its
hidden/not-foreground requirement as the regression check.

---

# Closed

Sixteen reports, all answered. Ten were spent here in code; 9 and 10 were
answered as document only and 14 was narrowed, and those three changed only
comments and docs; 17 changed only a manifest comment, 19 needs more evidence
and 21 was declined; and those six spent nothing in code — an answer that leaves the workaround where it is still
closes the report. Kept as outcomes rather than as
reports: what was asked, what the engine answered, what this repository did
with it. The original bodies are in git — see
[What the audit changed](#what-the-audit-changed) for reports 1–5, `f81e937`
for 7 and 8, the parent of `7041dc1` for 13 and 21, `a0e6f7e` for 11, 16,
17 and 19, and the parent of `07e4879` for 9, 10 and 14.

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

## 9. ~~No application-level per-frame hook or input capture~~ — document only, `f1e5244`

Asked for an optional observer between `Application`'s device polls and the
top state's update, so a console or overlay could see every frame whatever is
on the stack, with input capture/routing as a separately scoped second half.
**Document only**, in "Document the root state that sees every frame"; the
capture half is **declined** as game policy. No `Application` hook was added: a
callback there would need its own push/pop, empty-stack, audio-order and
catch-up rules, and a root `State` that owns a nested `StateContext` already
answers all four. `state_context.h`'s class comment now gives that pattern as
contract: the root's `update()` runs its own code and then the nested
`update(dt)`, its `draw()` draws the nested stack and then over it, its
activation callbacks and `init()` forward to the nested `notify_activation`,
`Application::depth()` is 1 for as long as it runs, and a frame capture goes in
its `update()`. CoreTests pin it, and the website's states and input pages say
so. On capture the engine kept to the boundary table: bindings and routing are
the game's, raw reads are never consumed, and a console gates its own readers,
which is the mechanism this client already has.

**Spent: nothing in code.** The forwarding stays where it is, from
`MenuPage`'s update template and `GameLevel`'s match update and draw into the
debug director, because adopting the root would only move it. It would also
change `Application::depth()` to 1 under the client's three readers (the
`perf.capture` precondition and the match-draw eligibility in
`script_commands.cpp`, and the status text in `debug_status_text.cpp`), and
move every `push` and `transition_to` the game makes on `Application` onto the
nested context. The keyboard-seat blank in `debug_director_hooks.cpp` stays as
this client's policy, and its comment and `debug-console.md` stop calling it a
workaround. `DBG#4` stops waiting on this report and is deferred. The draft and
its standalone exhibit are deleted.

**The prediction was wrong about the second half.** The ledger kept capture and
routing as a proposal to scope and file later; the engine declined it outright.
`DBG#4` and `debug-console.md` gave the seat blank's reason as "the engine has
no per-key consume", which was the wrong dependency: the engine will not add
one, and one would not have fixed the overlap the report itself conceded,
between a typed key and the same key played. The ledger also missed a cost the
engine spelled out: the root state changes what `Application::depth()` returns,
and this client reads it in three places. It was right that the observer was a
convenience and that the forwarding would move rather than go.

## 10. ~~Optional host capture convenience~~ — document only, `ac58ca1`

Asked for an optional host capture point between `submit` and `end_frame`, so
the Debug `screenshot` command could read the frame the loop draws instead of
drawing the stack again, with a non-covering final `State` that submits and
reads back from its `draw()` recorded as the existing alternative. **Document
only**, in "State that a frame submitted and never presented is legal". No hook
was added: it would be a branch in every `Application::render` to serve a debug
command, and it would not remove the stall, the PNG work or the client's
measurement flag. `renderer.h` now states the term the redraw rests on, that a
frame submitted and never presented is legal and contributes nothing to the
next one, and documents the redraw as the supported way to capture a frame
inside a running `Application`: `begin_frame`, `app.draw(*app.renderer())`,
`submit`, `read_back_buffer`, no `end_frame`, from an `update()` after the
update whose world it should show, never from a `draw()`. It promises the frame
the stack would draw, not what was displayed, and a draw that reads a clock or
counter is only as exact as that draw. `state.h` now puts any state that calls
`begin_frame`, `submit` or `end_frame` from `draw()` outside the contract. Null
and pixel contract cases ship, with goldens.

**Spent: nothing in code.** `capture_stack` in `debug_screenshot.cpp` is
already the documented shape, and every caller of `capture_screenshot_if_ready`
runs from an update hook or a script tick. Its header comment and
`debug-console.md` now cite the engine's contract instead of this report.
`capturing_` stays: the match draw reads a clock and feeds the perf ring, which
is the case the engine's pure-read caveat names, and the flag also drives the
screenshot's strip and status policy. PNG encoding, naming and paths stay. The
draft, the control and its build recipe (with the `.gitattributes` that guarded
its hash check) are deleted.

**The prediction was wrong about which shape was the answer.** The ledger
withdrew its claim that a second world draw is unavoidable, offered the
final-State pattern as the zero-redraw alternative, and asked the engine,
failing a hook, to document that pattern. The engine did the reverse. The
redraw the ledger called "the current workaround" is the one sanctioned
capture, and the final-State alternative is now a contract violation, not an
option. So the withdrawn claim was in effect right: redrawing is the supported
route, and the engine made it exact rather than avoidable.

## 11. ~~F10 and a lone Alt belong to the system menu, with no opt-out~~ — accepted, narrowed, `0f3b283`

Asked for an optional caller mechanism to suppress the bare system-menu
request, the zero-character `SC_KEYMENU` that F10 and a lone Alt each send,
while keeping key delivery and leaving native defaults available.
**Accepted, narrowed**, in "Let F10 and Alt alone be keys, not the system
menu". The Win32 window now returns 0 for every `WM_SYSCOMMAND` that is
`SC_KEYMENU` with an `lParam` of zero, unconditionally and in every build;
Alt+F4, Alt+Space and Alt+Tab still reach `DefWindowProc` and stay the
system's, and `window.h`'s `on_key_down` says so publicly. The *option* was
declined (T1, T3): its other value is a game frozen in a menu loop on a window
with no menu bar, and nothing in either repository would set it. AppTests pin
the predicate and the F10/Alt key edges. There is no live menu-loop case,
because `SC_KEYMENU` on a hidden window never enters the loop, so a physical
F10 and bare Alt on a focused window remain the engine's attended check.

**Spent** in this commit. The Debug-only `WH_GETMESSAGE` hook in
`debug_director_impl.cpp` and its press counter are deleted, and F10 is read
from `Keyboard` like F11. So is the second channel that existed only because
the hook hid F10 from `Keyboard`: `DebugDirector::capture_blocked_key_down`,
`KeyboardJumpCapture::update`'s `blocked_key_down` parameter and its
double-count guard, `forget_menu_key` and its four callers, and
`reset_menu_key_if_unfocused`. Keyboard edges are per frame and `Keyboard`
drops held keys on focus loss itself, so no counter is left to clear. The
capture acceptance now holds F10 through `Keyboard` and pins the same three
behaviours (a held F10 blocks neutral entry, F10+J is ambiguous, F10 alone is
unsupported); the "reported through both channels" case went with its second
channel. The ownership matrix's posted-F10 cases became `Keyboard` presses,
with the count unchanged. `debug-console.md` and AGENTS.md lose their hook
paragraphs. `check-debug-console.ps1`'s lone-Alt probe stays informational,
without its report-11 framing; it is the nearest local thing to the engine's
attended check, and was not run for this commit. One behaviour moved: Alt+F10
now toggles the debug menu, as Alt+F11 always did, because the hook's bit-29
exemption for it is not reproduced.

**The prediction was wrong three ways.** The draft asked for an option that
left native defaults available; the engine declined the option and swallows
the bare command for every caller. The ledger said an answer "could remove the
native F10 queue hook and manual counter". It removed more: the hook hid F10
from `Keyboard`, so a whole second channel had been built to make F10 visible
to Keyboard Jump capture again, and all of it was spendable. And the ledger
counted only Debug. Release never had the hook, so F10 or a lone Alt froze a
Release game in the system menu loop until the next key; the engine fixed that
with no client change, and `debug-console.md`'s Release section stopped saying
otherwise. The draft's "Alt+F4 and other command contracts remain required
follow-up acceptance" did not survive either: the engine kept them by
construction, narrower than GLFW, which drops Alt+Space too.

## 13. ~~`Key` has no names~~ — `2851ad9`

Asked for `key_name(Key)` and `key_from_name(std::string_view)` next to the
enum, one canonical spelling per key, with defined results for the sentinels.
**Accepted** as asked, in "Name every key, so a game can save a binding and read
it back". The token is the enumerator's own spelling (`digit_1`, `numpad_1`,
`page_up`, `left_bracket`, `del`), exact and case-sensitive, empty or nothing
for `none` and `count`, and declared a file format: `keyboard.h` now also
promises that a future left/right split adds keys and leaves `shift` meaning
either side. Allowed subsets, labels, case folding, aliases and chords were
left to the client, as the report asked.

**Spent** in this commit. The saved Jump preference lost its name column:
`input_key_name` and `input_key_from_name` are the engine's codec behind the
game's A-Z/Space domain check, which stays after the lookup because the engine
accepts `enter`, `f1` and `left`, which Jump must not. Its 27 tokens were
already the enumerator spellings, so no save changes meaning; acceptance now
also pins them to `labrador::key_name`. `debug_keys` lost its enum-order
arithmetic and about fifty hand-written names: an explicit bindable list is
named by the engine, `debug_keys::key_name` is gone (and with it the
unqualified call the pin bump `15cdfc1` had to qualify), and `parse` also takes
the engine's token. `keyboard_jump_collision` looks the chord up directly
instead of round-tripping through two name tables. The standalone draft and
exhibit are deleted.

**The prediction was wrong twice.** It treated the ask as a pure addition, and
it shipped as a **source break** here through ADL: an unqualified
`key_name(labrador::Key)` became ambiguous, in `debug_keys.cpp` and in the
exhibit itself, which stopped compiling at the pin that answered it. And it
said the answer "could replace the canonical token lookup in both consumers".
That held for the save, not for the console. The engine chose identifier
spellings over the console's shorter ones, and 25 console tokens differ:
`0`-`9`, `numpad0`-`9`, `delete`, `pageup`, `pagedown`, `lbracket`,
`rbracket`. They stay the console's canonical spelling as a small override
table, because `binds.cfg` files already hold them and a bind that fails to
parse is only a warning, so dropping them would lose binds silently. The
`del`-or-`delete` question the draft left open went against this spelling.
The bindable subset stays explicit too: walking the whole enum would make bare
`shift` and `control` bindable, which collide with the chord modifiers.

## 14. ~~No frame statistics or GPU timing~~ — accepted, narrowed, `7344610`

Asked for a stable, read-only snapshot of the last completed engine frame: a
sequence with a validity state, its elapsed interval, the Application update's
duration and count, and prepare/record, submit and end-frame durations, as the
observing half of engine `REV#10`; GPU timestamps were left for later.
**Accepted, narrowed**, in "Publish the last frame's CPU phase times from
Application" (engine story PI-19). `application.h` adds `FrameTimings` and
`Application::frame_timings()`, by value: `frame` (0 until the first completed
frame, so no separate valid flag), `updates` (fixed steps that tick), `interval`
(tick start minus the previous one, zero on the first frame and after
`reset_elapsed_time` or `on_resuming`), and `update_time` (polls, every stack
update and audio), `begin_frame_time`, `draw_time` (the whole
`StateContext::draw`, joined fan-out included), `submit_time` (with
`end_marker`) and `end_frame_time`, all `std::chrono::nanoseconds`. It is written
after `end_frame`, so during frame N it describes N-1. A frame that throws or
renders nothing publishes nothing, and a game's own capture frame does not
advance `frame`; its time lands in `update_time`. This was the one part of the
loop reports a client could not build, because those phases run where it cannot
reach. GPU time, history, distributions, counters and an overlay were
**declined**, and `REV#10`'s overlay stays deferred. AppTests feed the recorder
synthetic time points; a null-preset loop case runs a real `Application`.

**Spent: the documentation now; the new columns are `DBG#5`.** `debug_perf.h`,
`debug-console.md` and `polish-frame-pacing.md` stop saying the engine exposes
no frame statistics: `frame_timings()` has the begin_frame/submit/end_frame CPU
split, and GPU, compositor and scan-out stay unmeasured because the engine
declined GPU timing. `DBG#5` is unblocked and restated with its next step in
`debug-console.md`'s Not done: pair the snapshot with the previous committed
match frame by `frame`, add the three columns to the panel and to
`perf.capture` as schema 2, and teach `analyze_frame_pacing.py`, which accepts
only schema 1's exact fields, to read both. **Kept, and not replaced by the
snapshot:** the match-draw QPC stopwatch (`begin_match_draw` to the top of
`draw_status`), because the engine's `draw_time` also holds the debug panels and
live console drawn after it, so swapping it would fold overlay cost into
"draw"; the `Level::update` stopwatch, because one Application update runs the
debug clock's zero or several level updates and `update_time` also holds polls,
audio, hooks, scripts and screenshot redraws; `frame_ms`, which the per-player
`render fps` text reads and which the one-frame-late, tick-to-tick `interval`
would change visibly; level-build timing; and `capturing_`'s screenshot-gap
policy, eligibility, the ring, export and object counts, which the engine lists
as client policy itself. The draft, its standalone example and its build recipe
are deleted.

**The prediction was wrong about the draw stopwatch.** The ledger said an
answer "could replace client stopwatch pairs", and the engine's own record
named the paired stopwatch around the match draw as deletable. It cannot go
without changing what "draw" means here, so the engine's numbers are additive
rather than a replacement. The shape also differed from the ask: no explicit
valid state (`frame == 0` instead), and `begin_frame` split from `draw` rather
than folded into a prepare/record duration, because `begin_frame` can hold a
GPU wait on d3d12. And the ledger left GPU timestamps "open" as the broader
`ENG#14`/`DBG#5` wish; the engine declined them, so `DBG#5`'s GPU half waits on
nothing now. It is not planned.

## 16. ~~Windowed Apply shows a deliberately hidden window~~ — `7db3d5c`

Asked that an Application initialized with `visible=false` stay hidden through
`set_fullscreen(false)`, including an already-windowed Apply. **Accepted, and
widened**, in "Keep a hidden window hidden when it changes full screen".
`enter_fullscreen` had the same defect (`SW_SHOWMAXIMIZED`), so both paths now
read `IsWindowVisible` at call time and size a hidden window with
`SetWindowPos` and `SWP_NOACTIVATE`, never showing it; entering uses a
file-local `monitor_rect`, which throws naming the step if `GetMonitorInfoW`
fails. A shown window keeps its old path. Only the size is promised: a hidden
full-screen window is restored rather than maximised, and `WM_SIZE` reports
both alike. The `visible` comments in `application.h` and `window.h` now say
the engine never shows a window created hidden. The optional monitor-sizing of
hidden full-screen *creation* was left out by the owner.

**Spent, in part,** in this commit. In the seven painter fixtures that wrapped
Apply (save status, settings menu, display trial, Home alpha, Options defaults,
paused Options, settings values) the `SetWindowPos(-32000, -32000)` move off
the desktop, the `ShowWindow(SW_HIDE)` after each Apply, Keep and rollback, and
the keyboard/mouse/stack re-arm that undid them are deleted: the window now
stays hidden unaided. Each fixture keeps its "not visible, not foreground, not
full screen" requirement, which is now a regression check on the engine's
contract instead of something the fixture restored. Save status and settings
values also lose `SettingsProbeActivationGuard`, the `WH_CBT` veto on the
probe's activation; both now require that their Apply takes the direct-save
route, which pushes no display trial and so never calls `set_resolution`.
Production code needed nothing; the game is visible and
`display_confirmation.cpp` already calls the ordinary `set_fullscreen`.

**The guard stays in five fixtures, and why is a new finding** — now
[report 23](#23-resizing-a-hidden-window-activates-it--draft-with-activation-guard-exhibit).
Display trial, Home alpha, Options defaults and paused Options each resize the
hidden window, and without the veto all four failed their foreground check.
Settings menu keeps it around its two display-trial Applies only: every trial's
callback calls `set_resolution`, so it is the same path, and that fixture
passing without the veto in the first cut of this commit proved only that the
probe lacked foreground rights at that moment. Home alpha isolates it: before
`app.set_resolution(1280x720)` the window is hidden, inactive and not
foreground; after, it is still hidden but active and the foreground window.

**The prediction was wrong about the size of the workaround, both ways.** The
ledger named it as "the `apply` helper in `settings_menu_checks.h`". The guard
was defined in `save_status_checks.h` and used in seven fixtures, so the spend
reached seven files, not one. The report asked only about leaving full screen;
the fixtures that trial full screen On were behind the guard for the entering
path, which the report never named. But the guard was also covering a third
path, the windowed resize, so "that guard can go" was true for only two of
the seven. And the hidden controls' 1280x720 "size report" was a premise the
engine corrected: a hidden windowed window makes no size report before its
constructor returns, and the preset is 1280x720, so that observation could not
tell.

## 17. ~~The Win32 window ignores DPI once the client opts in~~ — accepted, narrowed, `dae6c87`

Asked for DPI-correct conversion of an explicit client-pixel request in a
process that has opted into per-monitor awareness, and a documented transition
policy. **Accepted, narrowed**, in "Keep the client size in a
per-monitor-aware process". `outer_size_for_client` takes a DPI and calls
`AdjustWindowRectExForDpi`. Creation sizes at the system DPI and corrects
before the show if a v2 window lands on another density; `resize_client` and
`leave_fullscreen` use the window's own DPI under v2. `WM_GETDPISCALEDSIZE`
answers the outer size that keeps the current client (with no full-screen
special case, and `FALSE` while iconic), and `WM_DPICHANGED` applies the
suggested rect under v2. Per-monitor v1 behaves as before. The policy is that
physical client pixels are kept: Microsoft's default logical-size rect was
declined as fighting `set_resolution`'s contract, and a portable DPI query or
notification was declined under T1. To import the per-DPI calls statically the
owner set the engine's Windows floor at **Windows 10 1607** (G105, with its own
CHANGES entry). Only synthetic-message AppTests ship; real scaled and mixed-DPI
runs stay attended work under the engine's OCA-66.

**Spent: nothing in code**, as predicted. No client code re-adjusted the outer
rect, so nothing was deleted. `game/settings.manifest`'s comment that the
engine "does not yet size itself for a changed DPI" is rewritten: under the
manifest's per-monitor-v2 declaration the engine keeps the requested client
size, across DPI changes too. The declaration and `check-startup.ps1`'s
PerMonitorV2 check stay; they are this client's policy. The manifest's
Vista-8.1 `supportedOS` entries now describe systems the engine cannot load on,
and are left for a housekeeping pass, since they change nothing.

**The prediction was right about the code and missed two things.** It did not
foresee the floor: ColourWars links the engine, so it inherits Windows 10
1607, and a pre-1607 Windows now fails at load on a missing import with no
build-time signal. The README already says Windows 10/11 x64, so nothing this
repository promised breaks. And the draft allowed that Microsoft's
suggested-rect behaviour was as valid an answer as keeping physical pixels; the
engine ruled it out on its own resolution contract. A scaled-display look at
ColourWars is not this report's business: OCA-66 owns the attended runs, and
this repository's own, if it wants one, would be a new `DECISION` row.

<a id="18-a-label-in-a-scene-can-be-neither-hidden-nor-raised--draft-with-station-label-exhibit"></a>

## 18. ~~Independent Label visibility convenience~~ — document only, `30cb06a`

Asked for a `Label`-level visibility convenience independent of text, bounds
and colour, for the Debug station labels, which hid by a tint of zero in all
four channels; `set_text(L"")` and a client wrapper GameObject were offered as
the alternatives already available. **Document only**, in "Point Label at
UiText::set_hidden, and warn of the zero-alpha tint". No `Label::set_visible`:
`UiText` (`engine/ui/widget.h`) is already text a scene can hold, its
`set_hidden` skips the draw and keeps the words, `bounds()` and the colour, and
it is not `final`, so a hidden flag on `Label` would be two spellings of one
mechanism (T4). The decline does not rest on a caller count: under The public
face's test the client writes nothing, it changes a type. `label.h`'s class
comment now points at `UiText`, and names `set_text(L"")`, gated on a change, as
the supported hide when the box may go empty. `colour.h` at `make_transparent`
and `renderer.h` at the `draw_sprite`/`draw_text` tints now say that a
zero-alpha tint still adds its colour under the premultiplied blend
(`SEAM.md#5`). `tests/ui/text_widget_tests.cpp` pins `UiText`'s hidden early-out
and its kept bounds. Category order, layers, retiring plain objects and a
public `set_font` were not decided, and were never filed.

**Spent** in this commit. The station labels are `UiText` now
(`Level::station_labels_`, `station_label()`), built shown in the premultiplied
presentation colour. `refresh_station_labels` sets that colour every time and
then `set_hidden(!overlays_.stations)`, where it used to swap in
`Colour(0, 0, 0, 0)`; the ternary and the comment citing this report are gone,
and the premultiplication comment cites `renderer.h`. A hidden label keeps its
real, edited colour. The `debug_console` stations check asserts `hidden()`, the
kept colour and the kept box, and that a live colour edit is adopted while
hidden without revealing; the painter `tune_colour` check asserts `hidden()` and
that a hidden label draws nothing. The first-use creation and its `end_tick()`
admission stay, which the engine names client policy. `set_text(L"")` was not
taken: it changes the box the scene culls by. The draft and its null-renderer
exhibit are deleted.

**`Label` has no client here again.** The 2026-08-15 audit counted giving it
one as a debt paid ([What the audit changed](#what-the-audit-changed)); the
station labels were that client, and moving them is the engine's own
recommendation.

**The prediction was wrong about the premise.** The draft's case was that no
engine class hides scene text while keeping its words, box and colour, and it
listed only the zero tint, empty text and a wrapper as alternatives; `UiText`,
one module over, already did it. The draft also checked the alpha-only tint as
a side control, where the engine found it the real defect: `make_transparent`
looks like the way to hide something and still draws. The client's zero-RGBA
tint was right by care, not by any documented contract.

<a id="19-the-window-is-shown-before-the-client-has-loaded-anything--draft-with-startup-timing-exhibit"></a>

## 19. ~~No public later-show convenience for a hidden Application~~ — needs more evidence, `cacf112`

Asked for an explicit public later-show operation for an Application
initialized hidden, or a documented supported sequence for a native
`ShowWindow`, with show command, full-screen sizing and focus specified.
**Needs more evidence**, recorded in "Record the engine's decision on each of
ColourWars' fifteen live reports"; the engine's own table leaves the row open
while it waits. There is no `Window::show()` or `Application::show()`, and the
document-only closure the draft offered was declined as "the same commitment
as `show()` made in prose": it would choose show command, focus and full-screen
sizing without first-frame evidence. The engine reframed the question as what a
default-visible launch shows between `ShowWindow` and the first present. The
window is shown inside the `Window` constructor, before the device exists, and
is painted with the `COLOR_WINDOW + 1` class brush until then. If that is the
problem, its preferred fix is engine-side and has no new name: size with 16's
`monitor_rect`, show after the first `end_frame`, and a null or black brush,
which fixes every visible game without a client call. What shipped, in 16's
`7db3d5c`: the `visible` comments say the engine never shows a window created
hidden and that a window shown by other means is outside the contract, and the
`Window` constructor comment says a hidden windowed window makes no size report
before it returns, which answered the draft's open question (a).

**Spent: nothing in code.** `game/main.cpp` keeps its initialize, register,
load/validate, first-state, run order, and no native `ShowWindow` was added:
the engine declined to bless that sequence and puts a natively shown window
outside its contract. The draft and the hidden-creation exhibit are deleted,
and the polish plan's "needs an engine show-after-load" is restated.

**Outstanding, and this repository's** (`GAP#5.1`, now `DECISION`). The engine
asks for two captures, each recorded with commit and date: a default-visible
`x64-release` launch of `samples/minimal` and of ColourWars on the owner's
desktop, from `ShowWindow` to the first present (the class brush or not, and
for how long); and the same with a hidden launch and a native `ShowWindow`
after the first state is constructed. If the first shows a flash and the second
cures it, the engine-side fix above follows. They go back to Labrador as new
evidence on its decision, not as this report reopened. They are attended screen
captures, not ctest.

**The prediction was wrong about what the problem is.** The draft offered a
later-show call or a documented sequence as acceptable; the engine took
neither. The ledger and the polish plan treated the issue as *when* the window
is shown relative to the client's loading, so a client call after validation;
the engine thinks the likely cure is automatic and in the engine, and points
out that the white flash itself has never been observed. The hidden control's
1280x720 size report, offered as the supported half, could not distinguish
anything: a hidden windowed window makes no size report before its constructor
returns.

## 20. ~~Scene drawing gives client objects no view context~~ — accepted, narrowed, `11ec1fc`

Asked that normal world drawing could read or receive its current camera and
viewport, with the shape left upstream and the art policy kept here; the
exhibit was the overview/monitor miniature walk, which re-ran Scene's cull to
hand `Structure` a camera. **Accepted, narrowed**, in "Let a drawable read its
view's camera and viewport from the DrawList". `DrawList::camera()` returns the
last `set_camera` on that list this frame, else `Camera::DEFAULT_CAMERA`, and
`DrawList::viewport()` the viewport as last given to `set_viewport`, not
snapped, else the whole back buffer. Both are valid only on a valid list and
must not be stored; all five backends define them. `renderer.h` adds the
contract that a draw which grows as the camera zooms out must say so through
`cull_bounds(units_per_pixel)`. `game_object.h` now says a view-dependent
`draw()` reads the list, replacing a sentence about a camera no `draw()` could
reach, and `scene.h`'s `ViewOverlay` comment says the list arrives with the
view's camera and viewport. `GameObject::draw`'s signature and Scene's dispatch
are unchanged: a `draw(DrawList&, const DrawContext&)` (T8, and a source break)
and a mutable per-view handoff (T2, under the fan-out) were refused. The engine
supplies the mechanism and says it does not claim the client's deletion works.

**Spent, in part,** in this commit. The threaded camera is gone:
`TerrainMaterial::paint_miniature`, `Structure::draw_miniature` and
`StructurePaintable::draw_miniature` take only the list, and `paint_miniature`
copies `draw_list.camera()` (a copy, because it then sets `DEFAULT_CAMERA` on the
same list) and restores it as before. `miniature_view`, StagePreview and the
painter fixtures drop the argument; every caller already set the camera on the
list, so pixels are unchanged. `projected_miniature_quads(const Camera&)` keeps
its parameter as the test seam it is. The draft and its null-renderer
illustration are deleted.

**Kept, and now `POL#28`:** the sequential overview/monitor walk in
`miniature_view.cpp` and its `dynamic_cast<const Structure*>`,
`Level::draw_miniature_world`'s view offset, the `scenery_` mirror and
StagePreview's own walk. Deleting them means `Structure::draw` choosing the
miniature art itself so that `Scene::draw` can draw those views, and this client
chooses that art by path (overview, monitor, stage preview), not by zoom. A
scale rule would switch 4-player gameplay panes to miniature strokes in a window
under about 540 pixels tall. So the deletion needs a path-set mode flag (the
engine's "a const flag set in update()") and moves those views onto Scene's
fan-out and category order, which is the integration check the decision asks
for. [polish-terrain.md](../polish-terrain.md#small-view-terrain-readability-2026-10-18)
owns it.

**The prediction was wrong about what a reader would buy.** The draft said view
context "could enable this client's normal world-slot projection" and left the
deletion to a separate check, without seeing that a camera alone cannot delete
the borrowed walk: it needs a behaviour decision the draft never named, the
path-set mode. The draft also argued a convenience, where the engine found a
stated rule contradicted: `scene.h` forbids hand-copied per-view walks, and
`game_object.h` described a camera a `draw()` could not get. And it did not
foresee the cull contract. The miniature strokes are clipped to their
structure's own extent, so by this client's reading they already meet it; the
`POL#28` pixel oracles are where that gets checked.

## 21. ~~Digital action evaluation remains client code~~ — declined, `197aff1`

Asked (formerly `GAP#F2`) for generic evaluation of typed physical bindings
with independent per-source edges, on the argument that the new rebinding
screen removed the engine's stated reason for having no action map.
**Declined**, document only, in "Decline report 21 and re-take the reason the
engine has no action map". The engine agreed the stated reason was now false
and replaced it everywhere it was written, starting with `gamepad.h`, on other
grounds: evaluating a binding table *is* the action map, which PHILOSOPHY's
boundary table gives to the game (T1), and combining sources is policy on which
the clients already disagree. LineSweeper rebuilds edges from combined held
state; its pause menu and the components gallery OR each source's `pressed()`;
this client ORs per-source edges for game actions and uses a held-before rule
for its menus. The per-source edge functions the draft offered as a fallback
were already the engine's half. If a third client, or a second rebindable
action here, needs the same evaluator unchanged, the engine names a
`samples/components` piece as the route, not an engine API.

**Spent: nothing in code.** `game/action_bindings.*`,
`effective_action_bindings` and `PlayerInput` stay as they are; the source
merge, the two held-before rules, the trigger thresholds and the binding
tables are this client's by the boundary table, and the decline is the record
of that. The standalone draft and exhibit are deleted, and the roadmap's
filing paragraph is replaced by a pointer here. The frozen gap audit keeps its
`GAP#F2` row as written.

**The prediction was wrong about what the exhibit would buy.** The draft
argued that a real rebinding screen supplied the exhibit the engine's absence
was waiting for, so T1 would support extraction. The exhibit falsified the
reason and moved nothing else: the decision stands on a reason that does not
move when a client grows a screen. The draft's own concession — that an
extraction "must document its semantics or expose source results for the
client to combine" — was the answer, and the engine already did the second.
This is the first report here whose exhibit was declined outright, which
breaks the six-of-seven strike rate [above](#why-the-exhibits-matter-more-than-the-asks)
rather than extending it.

## 22. ~~Handle-holding drawables can only be re-pointed by name~~ — accepted, narrowed, `bec82f0`

Asked for handle overloads beside the name forms
(`set_animation_strip_and_reset(SpriteSheetHandle, strip_handle)`,
`set_frame(SpriteSheetHandle, frame_handle)` and `set_frame(frame_handle)`,
`UiTexture` handle forms, handle constructors for `AnimationObject` and
`TextureObject`) plus `std::less<>` and `string_view` on `Registry`/`NameTable`.
The per-contact splash reset, the per-spawn projectile constructor and the
per-state figure clip change each handed back names on the update path, against
PHILOSOPHY's "Per-frame code touches no string-keyed map". **Accepted,
narrowed**, in "Re-point sprites and animations by a resolved sheet pair".
`render_resources.h` adds `RenderResources::SheetFrame` and `SheetStrip`,
two-handle values only `resolve_sheet_frame`/`resolve_sheet_strip` can make, so
a pair cannot be mismatched; each resolve throws naming the missing name.
`TextureObject` and `AnimationObject` take them through public constructors and
protected `set_frame(SheetFrame)` and `set_animation_strip_and_reset(SheetStrip)`,
which throw at the call for an unresolved pair. The name setters are rebuilt on
the pairs and are now atomic, so a bad name leaves the object unchanged: a
silent behaviour change, recorded in `CHANGES.md`. **Declined:** the raw
two-handle overloads (a frame handle from one sheet passed with another fails
quietly, T2/T6, and T5 puts the check in the type), the frame-only `set_frame`
(`draw_with` already draws another frame of the same sheet), the `UiTexture`
forms (menus re-point on input; a one-line overload if a per-frame caller
appears) and `std::less<>` (once objects hold pairs every remaining resolve is
load-time).

**Spent** in this commit, at all three hot sites. *Splash:* `PaintSplashClips`
holds the tile's build-time clip and one `SheetStrip` per `ProjectileType`,
resolved from `PaintTuning`; each `StructurePaintable` resolves it once and lends
it to its tiles, and is no longer copyable because they borrow it.
`PaintTileSplash::reset_and_play` takes the pair and reads the clip's frame time,
count and origin by index, so the repeat-clip gate (`clip_sheet_`, `clip_name_`)
and the comment citing this report are deleted. *Projectiles:*
`ProjectileBuilder` takes the `RenderResources` and resolves each type's strip
once (`resolve_projectile_strip`); every projectile constructor takes the
`SheetStrip`, `Projectile` builds its `AnimationObject` from it, and
`ProjectileJet` reads its frame size through it instead of resolving the strip
again. *Figure:* `Player` resolves a `FigureClip` per `PlayerAnimationState`
(uniform frame, body strip, paint strip) in its constructor;
`set_animation_state` and `FigureClothing::select` re-point by pair, and the
action-clip table reads from it. Frame times are still read live from tuning.
Acceptance fixtures that build projectiles or tiles directly pass resolved
pairs. There was no draft or exhibit to delete.

**Kept:** the menu icons and the team- and weapon-select pages re-point
`UiTexture`s by name, by the decline; load-time resolves (validation, the idle
frame rectangles, supply strips) stay on names; `DebugRegistry` keeps its own
`std::less<>`. No frame-time gain is claimed, since none was ever measured: this
is conformance to the stated rule. A missing name now throws at level, weapon or
player construction instead of at the first contact or spawn, and startup
validation already checks every one.

**The prediction was wrong about the shape, and about where the splash handles
live.** The draft asked for raw handles and left whether a mismatched pair is
"checked, asserted or documented" upstream; the engine made a mismatch
unrepresentable, so the client spends pairs and cannot assemble one from halves
it resolved separately. Three of the four asks were declined, and so was the
`std::less<>` hygiene the draft said "rides with the main ask". The draft put the
splash handles in "PaintTile's shared paint record", which is `PaintTuning`, read
before any sheet exists; they needed a record of their own beside it. And it did
not foresee the atomic name setters.

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
  units, as `camera_tools.h:13-24` asks. `PlayerCamera` also centres an axis
  wider than the arena.

  **Decision 2026-10-15 (`ENG#CAMERA`): keep this policy in ColourWars; no
  engine request.** The reference height belongs to the full display, and
  the game's split-screen layout determines each pane's visible world extent.
  At 16:9 these are 1920×1080 for one player, 1920×540 for two, and 960×540
  for three or four. Labrador's camera and world-space follow helper already
  provide the operations this policy needs. See the
  [display baseline](../polish-foundation.md#display-baseline).

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

- **`Scene::contacts()` having no reader** (`scene.h:200`) and
  **`labrador::Label` having no client** (`engine/render/label.h:26` — it
  moved out of `scene/`). Neither was an engine defect. Both were things this
  client had not got round to exercising, a debt this repository owed the
  engine rather than the other way round, and both are now paid.
  `Level::update_level_logic` hands `contacts()` to every player's
  `end_contacts`. The Debug overlay `overlay.stations` held one `Label` per
  station in the level's scene. Using `Label` turned up report 18, whose
  answer ([Closed](#18-independent-label-visibility-convenience--document-only-30cb06a))
  moved those labels to `UiText`, so `Label` has no client here again.

  A third item, the anti-tunnelling bound never being evaluated, is now paid.
  `game/tunnelling_limits.cpp` evaluates `can_tunnel` at startup, and on Debug
  `reload` and `tune`, for every object in every stage against a player at its
  speed cap. The step it uses is `SIMULATION_HZ` (`game/simulation.h`), the
  same value `main.cpp` passes to the engine as `target_fps`. The row that
  asked for the check also said "the sniper's 2000 u/s already exceeds it",
  but that has been stale since `70efadf` added `Projectile::resolve_sweep`.
  Every projectile kind except the roller tests its whole path each step, and
  the roller never moves (`is_swept`, `projectile_type.h`). The `tunnelling`
  acceptance case fires a jet at the 5000 u/s cap at a 10-unit slab, a case
  the bound reports as unsafe, and the sweep still stops the jet at the slab.
  No report follows. T3 declined continuous collision, and `tunnelling.h`
  tells a discrete client to "separately test the trajectories that matter".
  The sweep is that test, written where the trajectories are known.

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
