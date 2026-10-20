# Reports to file against Labrador

> The running list of API complaints this client owes the engine. Each one is a
> case where the game wanted something ordinary and the API forced a bad
> answer, and each carries an **exhibit**: the workaround that is in this
> repository, in a file you can open, written because the engine offered
> nothing better.
>
> **Fourteen are live as unfiled drafts with exhibits (6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21). Seven are closed** and kept below as one paragraph
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

The thirteen drafts present at approval remain unfiled. Report 21 now adds a
fourteenth unfiled draft. Recording this approval posts no reports and leaves
the engine pin unchanged. The next work is a fresh upstream-master
check and preparation of the minimal report/reproducer for each surviving gap;
filing uses the approved scope and established route above. This supporting
work does not change TODO's level-first priority.

Before authorized filing, refresh engine master from upstream and recheck the
concrete client exhibit against that revision. Record the checked commit with
the report. A cached `origin/master` ref or local branch is not evidence of
live upstream master. An answered claim is withdrawn with a reason; an
unanswered one is filed only with the details and destination the author has
authorized.

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
| 6 | Shared Music/SFX attenuation remains emitter work | per-emitter saved gains and standalone `audio_gain_exhibit.cpp` | local unfiled draft - master checked 2026-10-20; synthetic Debug/Release runs pass |
| 9 | Per-frame overlay orchestration and separately scoped input routing | client debug forwarding and reader gates; standalone `input_observer.cpp` | local unfiled observer draft - master checked 2026-10-20; synthetic Debug/Release runs pass; capture/routing remains separately scoped |
| 10 | No way to read back the frame that was presented | `capture_stack` in `game/debug/debug_screenshot.cpp` | draft — exhibit present; recheck upstream before filing |
| 11 | Optional opt-out for bare system-menu keys | native F10 queue hook and standalone `system_keys_exhibit.cpp` | local unfiled draft - master checked 2026-10-20; synthetic Debug/Release requests reproduce |
| 12 | JSON is read-only, path-only and opaque | the `%TEMP%` candidate file in `tune_commands.cpp`, `json_spans.cpp`, `check_tune_consumed` | draft — exhibit present; recheck upstream before filing |
| 13 | `Key` has no names | Debug bind names, saved keyboard Jump tokens and the standalone `key_names.cpp` | local unfiled draft - master checked 2026-10-20; synthetic Debug/Release runs pass |
| 14 | No frame statistics or GPU timing | client perf hooks and standalone `frame_phase_boundaries.cpp` | local unfiled CPU-phase request - master checked 2026-10-20; manual public-call example passes Debug/Release |
| 15 | Runtime packaging and content staging are repeated in the client | existing client release/staging rules and the synthetic `runtime-staging/` fixture | local unfiled draft - staging slice checked against master 2026-10-20; Debug/Release phases pass; runtime install remains separate |
| 16 | Windowed Apply shows a deliberately hidden window | standalone `hidden_apply_exhibit.cpp` and the client Apply guard | local unfiled bug draft - master checked 2026-10-20; Debug/Release reproduce |
| 17 | DPI-correct client-size conversion for an opted-in application | existing awareness manifest and standalone hidden `window-dpi/` baseline/arithmetic fixture | local unfiled request - master checked 2026-10-20; Debug/Release 96-DPI baselines pass; scaled/transition acceptance remains open |
| 18 | Independent Label visibility convenience | supported zero-tint/empty-text alternatives and standalone `label-visibility/` null recording | local unfiled draft - master checked 2026-10-20; null Debug/Release verified; order/retirement/font questions stay separate |
| 19 | No public later-show convenience for an Application initialized hidden | exact standalone hidden-creation control and `game/main.cpp` startup order | local unfiled request - master checked 2026-10-20; four hidden Debug/Release cases pass; first present remains open |
| 20 | Active view context during normal world drawing | miniature traversal and standalone `view-context/` caller-context null illustration | local unfiled draft - master checked 2026-10-20; API source audit and null Debug/Release verified; Scene/order integration stays separate |
| 21 | Digital action evaluation remains client code | runtime saved Jump mapping and the standalone `action_evaluation.cpp` | local unfiled draft - master checked 2026-10-20; synthetic Debug/Release runs pass |

Nothing else is outstanding, on either side, beyond reports 6 and 9-21 below. The fullscreen workaround the
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

## 9. No application-level per-frame hook or input capture — **draft with console exhibit**

**Observer slice prepared 2026-10-20.** The minimal
[request](../engine-reports/input-observer.md) and
[standalone example](../../scripts/engine-reports/input_observer.cpp) cover an
optional observer between Application's device polls and the top-state update.
Upstream master was freshly checked at 19:34:53 UTC, and the reviewed engine
worktree matched FETCH_HEAD at 19:50:08 UTC:
`52e809a5a0229d37db4373faa7853df4c861a310`. Direct MSVC 19.51.36256.0 Debug and
Release runs against prebuilt engine/math libraries passed all four synthetic
checks with exit 0 and empty stderr. The original Debug receipt required CRLF/LF
normalization of retained logs; Debug was not rerun and its process ID was not
retained.

Application has no built-in observer at that point, and StateContext updates
only its top state. These are source observations, not edge-function defects.
A pushed transparent console therefore suspends the underlying update by
design. A permanent root State containing a nested StateContext is a valid
existing-API alternative; the example also demonstrates that composition.
The request is a convenience that could remove or relocate global forwarding,
not an unavoidable engine limitation. The full menu-update template and
`before_*` hooks also serve motion, music, scripts, rebuild and timing work.

**Capture/routing remains a separately scoped proposal.** Raw device snapshots
correctly remain readable by multiple consumers. The client's live console
uses explicit keyboard-seat and closing-frame filters; mouse aim reads Mouse
directly and is not hidden by those PlayerInputData filters. Its pad chord
clears selected per-slot match flags and resets only that pad's menu arming;
the older claim about losing another pad's A press no longer applies.

The physical-key capture page now guards raw ownership before ordinary debug
hooks, resets its readers on entry/exit, and drains its gesture under client
neutral/focus/release rules. It also chooses how to handle queued developer
work. An observer alone would not replace these policies or hide raw reads.
A capture/filtered-view extraction would require its own precise contract and
reader adoption. It cannot distinguish typing a movement key from intending
that same key for gameplay, so simultaneous play/type and deletion of the
whole-seat filter are not promised.

Existing polling, edges, thresholds, focus/connection notifications and mouse
capture-cancellation epochs are not missing. System-key interception remains
ENG#11; key labels, control-key repeat, post-stack drawing and pad identity are
outside the prepared observer slice. Both halves remain unfiled, the engine
is unchanged, and upstream must be refreshed immediately before filing.

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

The [minimal optional-mechanism report](../engine-reports/system-menu-keys.md)
and [standalone Application exhibit](../../scripts/engine-reports/system_keys_exhibit.cpp)
make the existing Debug F10 workaround concrete. Key messages do reach the
engine's void callbacks before the Win32 procedure continues to
`DefWindowProc`; the gap is a caller option to suppress the resulting bare
system-menu request. F10 is not untranslated or inherently unbindable.

Fresh engine master `52e809a5a0229d37db4373faa7853df4c861a310` was checked on
2026-10-20 at 19:34:53 UTC, with the complete engine worktree matched to that
tree. No bare-system-menu option is present. The client still uses its
Debug-only `WH_GETMESSAGE` hook in `debug_director_impl.cpp` to consume bare
F10 before dispatch and count the press itself.

Both MSVC 19.51.36256.0 `/W4 /WX` Debug/Release runs exit 0 with empty stderr.
The real hidden Application receives only synthetic messages directed to its
owned HWND. Each A/F10/Alt pair reaches raw engine down/up snapshots once;
A produces no menu request, while F10 and bare Alt each produce one
zero-character `SC_KEYMENU`. An owned WndProc barrier consumes that command
before default handling, after a direct command self-test. No menu loop is
entered. Repeated native hidden/offscreen/nonforeground checks pass, and the
initial engine logical keyboard-focus value of 1 remains unchanged at all
sample points; this logical value is not an OS-focus trace.

The normal D3D11/XAudio2 services initialize with one worker/view, but no
manifest, assets, state or game objects are loaded and no sound is played.
Only the owned HWND is moved offscreen, a current-thread CBT guard vetoes only
its activation, and its original WndProc is restored. No global input or
foreground manipulation is used.

The request is an optional bare-menu suppression mechanism that preserves
key delivery and leaves native defaults available. Alt+F4 and other command/
mnemonic contracts remain required follow-up acceptance, not tested facts.
An answer could remove the native F10 queue hook and manual counter; F11
retention is client UI policy. The fixture does not measure physical input
delivery, an actual modal pause or human/hardware acceptance.

Receipts are under
`out/validation/crossfeed-20261020/system-keys-report-final-v2/`. The earlier
fixture's invalid false-logical-focus assumption and incomplete wrapper
receipt remain private. The engine is unchanged, the draft remains unfiled,
and upstream must be refreshed immediately before filing.

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

## 13. `Key` has no names — **draft, low priority**

**Exhibit added by the developer debug system (phase 4).** `bind <key> <line>`
and `binds.cfg` name keys in text. The saved keyboard Jump preference now adds
a Release-safe naming consumer in `game/input_bindings.cpp`.

**Minimal report prepared 2026-10-20.** The sanitized
[request](../engine-reports/key-names.md) and standalone
[synthetic exhibit](../../scripts/engine-reports/key_names.cpp) are ready for
review and remain **unfiled**. Fresh upstream master was fetched at 19:34:53
UTC as `52e809a5a0229d37db4373faa7853df4c861a310`; the engine worktree was
confirmed unchanged against that `FETCH_HEAD`. The exact synthetic source
passed direct MSVC C++20 `/W4 /WX` Debug and Release compile/run checks. Its
nine-entry subset proves canonical parse/format round trips and strict error
handling, not a complete key map or physical-input acceptance. Review the
prepared report and refresh upstream before authorized filing through the
[publication gate](#publication-gate-engpublish).

### What happens

`labrador::Key` has no portable canonical `key_name(Key)`/`key_from_name`
codec. So `game/debug/debug_keys.cpp` carries its own table of about
eighty names (`a`-`z`, `0`-`9`, `f1`-`f12`, `numpad0`-`9`, `pageup`,
`lbracket`, ...) built against the enum's order. The saved keyboard preference
carries its own explicit A-Z/Space token table and separate UI labels. Clients
that persist key names must maintain those translations as the enum changes.

The existing web `key_from_code` is acknowledged: it decodes DOM events,
collapses multiple modifier/Enter codes to the same enum key and supplies no
reverse formatter. It does not answer the portable token round trip. Typed
characters, localized labels and hardware identity are separate concerns.

### The ask, when it goes

`std::string_view key_name(Key)` and `std::optional<Key> key_from_name(std::string_view)`
next to the enum, with one documented canonical spelling per real key and
defined unknown/sentinel results. Answering it could replace the canonical
token lookup in both consumers. Existing file aliases may remain if the engine
chooses different spellings; allowed subsets, UI labels, case normalization
and `ctrl+`/`shift+` chord grammar stay the game's.

---

## 14. No frame statistics or GPU timing — **draft with perf exhibit**

The [CPU frame-statistics request](../engine-reports/cpu-frame-statistics.md)
now has an exact tested [standalone manual public-call example](../../scripts/engine-reports/frame_phase_boundaries.cpp).
It remains **unfiled**. Source inspection of fresh upstream master
`52e809a5a0229d37db4373faa7853df4c861a310` (checked 2026-10-20) confirms no
Application completed-frame timing accessor and no Renderer timestamp-query
service. This first request is CPU elapsed phase data, not GPU execution.

The client already times Level updates and match draws with QPC. Its
draw-to-draw intervals include intervening submit/Present wall time indirectly;
their separate durations and whole-Application phases remain unavailable.
Target simulation dt is not an elapsed rendered-frame interval.

The synthetic source initializes the actual hidden D3D11/XAudio2 Application
but owns a separate real StateContext/manual Renderer loop. Both MSVC
19.51.36256.0 `/W4 /WX` Debug/Release runs pass three empty-view iterations,
exit 0 and have empty stderr. Each output has the exact scope/header, three
finite nonnegative CPU wall rows and final PASS line. No Application phase or
frame interval, GPU, timing threshold or performance qualification is measured.
Raw transient durations are evidence, not a benchmark.

The first useful request is a stable read-only completed-engine-frame sequence
with validity and documented elapsed interval, aggregate Application-update
duration/count, prepare/record duration, submit-call duration and end-frame-call
duration. This is the current client for the observability half of engine
`REV#10`; it does not ask for its whole engine-drawn overlay.

An answer could replace client stopwatch pairs, with an explicit change from
match-only to engine phase semantics. Match eligibility, frame deduplication,
capture/export, object counts and level-build timing remain client code.
The draw ingestion hook may remain, and screenshot-gap policy remains because
capture work inside an Application update can affect its elapsed duration.
The old blanket deletion claim for the draw hook/screenshot suppression is
therefore narrowed.

Retained receipts are under
`out/validation/crossfeed-20261020/frame-stats-report-final-v1/`. The example
and recipe hashes match both executed configurations. No engine change or
public issue was made. Refresh upstream unconditionally before filing.
GPU timestamps, scan-out, broader counters/overlay and visible uncontended
hardware qualification remain outside this prepared CPU slice; the broader
`ENG#14`/`DBG#5` wishes remain open.

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

## 16. Windowed Apply shows a deliberately hidden window — **draft with probe exhibit**

Observed in the actual Options page on 2026-10-15 and now reproduced by the
[minimal engine-only bug report](../engine-reports/hidden-window-apply.md) and
[standalone Application exhibit](../../scripts/engine-reports/hidden_apply_exhibit.cpp).
An application initialized with `visible=false` and `fullscreen=false` becomes
visible according to `IsWindowVisible` after `set_fullscreen(false)`.

Fresh upstream master was checked on 2026-10-20 at 19:34:53 UTC:
`52e809a5a0229d37db4373faa7853df4c861a310`. Application still calls
`Window::leave_fullscreen` unconditionally for false, and the Win32 method
still calls `ShowWindow(handle, SW_SHOWNORMAL)`.

Both MSVC 19.51.36256.0 `/W4 /WX` Debug/Release runs reproduce the same
hidden -> visible -> hidden flag sequence, exit 0 and have empty stderr.
The actual D3D11/XAudio2 Application initializes one worker/view but loads no
manifest, game objects, assets or state and plays no sound. Its owned HWND is
placed outside the virtual desktop, a current-thread CBT guard vetoes only its
activation, and the program samples then immediately hides it. Its before/after
foreground/bounds checks pass; this is not a continuous OS-focus trace.

The client workaround remains the `apply` helper in
[`settings_menu_checks.h`](../../scripts/acceptance/painter_visual/settings_menu_checks.h):
move the owned window offscreen, guard activation, Apply, hide and rearm local
input. The request is to preserve deliberate hidden visibility, including an
already-windowed Apply, so that guard can go.

Receipts are under
`out/validation/crossfeed-20261020/hidden-apply-report-final-v1/`, including
`result.json`, both compile argument/log sets and process stdout/stderr. No
engine change or public issue was made. Refresh upstream immediately before
filing the unfiled bug draft. Fullscreen, DPI/monitor, human, pad, listening
and performance gates remain open.

---

## 17. The Win32 window ignores DPI once the client opts in — **draft with manifest exhibit**

**Minimal report prepared 2026-10-20.** The
[DPI-correct sizing request](../engine-reports/window-dpi.md),
[synthetic source](../../scripts/engine-reports/window-dpi/dpi_window_baseline.cpp)
and dedicated manifest remain **unfiled**. Fresh upstream master was fetched
at 19:34:53 UTC as `52e809a5a0229d37db4373faa7853df4c861a310`; the engine
worktree was checked against that tree. Refresh upstream before filing through
the [publication gate](#publication-gate-engpublish).

ColourWars already opts into per-monitor-v2 in
[its executable manifest](../../game/settings.manifest), and
[startup acceptance](../../scripts/check-startup.ps1) checks its process/window
awareness. The executable chooses this policy. A manifest is the recommended
declaration; an API call before window creation is also supported.

At the checked engine revision, `outer_size_for_client` uses the DPI-unaware
`AdjustWindowRectEx` for creation and windowed resize arithmetic. Window has
no `WM_DPICHANGED` case or public DPI query/notification. Existing `WM_SIZE`
handling already adopts actual client dimensions in layout and renderer state.
The primary API contract establishes a source mismatch for per-monitor-aware
callers; no exact scaled-monitor pixel error is claimed from this host.

Both standalone Debug/Release builds pass under MSVC 19.51.36256.0 `/W4 /WX`,
exit 0 with empty stderr and share seven identical normalized stdout lines.
The actual hidden HWND reports per-monitor-v2 context at 96 DPI, an exact
1280x720 client, and matching layout/back buffer. Independent rectangle queries
return 1296x759 at 96 DPI, 1298x767 at 120, 1302x776 at 144 and 1306x791 at
192; those queries do not move the window or change its effective DPI.
Source/recipe hashes, compile logs and explicit process receipts are retained
under `out/validation/crossfeed-20261020/dpi-report-final-v1/`.

The request is DPI-correct conversion of an explicit client-pixel request and
a documented transition policy. Microsoft's suggested-rectangle behavior and
always keeping a requested physical-pixel size are different choices; the
latter is not prescribed as the only correct handler. A portable DPI accessor
is a separate potential request; this Windows client already has a native
HWND escape hatch.

The source changes no display settings, moves no monitor/window, sends no
synthetic DPI message, starts no loop and renders no pixels. It is a 100%
baseline, not mixed-DPI or sharpness acceptance. No client workaround is
deleted, and the awareness manifest stays. Engine
[G151](../../external/labrador/docs/product/GAP-REGISTER.md#g151) and
[OCA-66](../../external/labrador/docs/product/STORIES.md#oca-66) still own the
template awareness decision and attended 150% result; this report does not
close them.

---

<a id="18-a-label-in-a-scene-can-be-neither-hidden-nor-raised--draft-with-station-label-exhibit"></a>

## 18. Independent Label visibility convenience — **draft with station-label exhibit**

**Visibility convenience slice prepared 2026-10-20.** The sanitized
[request](../engine-reports/label-visibility.md) and standalone
[synthetic null-renderer exhibit](../../scripts/engine-reports/label-visibility/label_visibility_recording.cpp)
are ready for review and remain **unfiled**. The checked upstream master was
freshly fetched at 19:34:53 UTC as
`52e809a5a0229d37db4373faa7853df4c861a310`. The exact 26-translation-unit
CMake consumer passed MSVC C++20 `/W4 /WX` Debug and Release builds and runs;
both exited 0 with empty stderr and thirteen exact expected output lines.

### Supported hiding and the convenience request

The station-label client in `Level::refresh_station_labels` keeps scene-owned
Labels and sets all four tint channels to zero while its overlay is off.
Showing them restores the presentation colour. That is a supported current
workaround, and `set_text(L"")` is another working alternative: it emits no
glyphs, while changing the words and measured box until the client restores
them. A client-owned GameObject can also contain a Label and choose whether
to forward draw. The current API can already hide a label.

The prepared request is only a Label-level visibility convenience independent
of text, bounds and colour. The real null recording checks zero-RGBA vertex
colour, unchanged words/bounds/geometry, alpha-only retained RGB, and tint/text
restoration. It does not prove pixels, blending, readability or performance.
The engine remains free to prefer these small client workarounds.

### Other portions remain open and separate

Scene category-order documentation, a possible layer mechanism, retirement of
ordinary objects and public font rebinding remain outside this narrow report.
The current renderer explicitly documents call order and ignored
`layer_depth`; that is a contract, not an unproven renderer defect. The null
fixture calls Label directly and does not test Scene's cull, ownership,
admission, retirement or traversal. `TextObject::set_font` is protected, not a
public Label font setter.

A visibility flag alone would not delete the client's lazy first-use
`end_tick()` admission; that depends on when the client creates the pending
labels. Keep these questions separate when preparing further exhibits.
Freshly fetch and recheck upstream master before any filing through the
[publication gate](#publication-gate-engpublish).

---

<a id="19-the-window-is-shown-before-the-client-has-loaded-anything--draft-with-startup-timing-exhibit"></a>

## 19. No public later-show convenience for a hidden Application - **draft with hidden-creation exhibit**

The sanitized [request](../engine-reports/startup-visibility.md),
[exact tested source](../../scripts/engine-reports/startup-visibility/startup_hidden_creation.cpp)
and [public build recipe](../../scripts/engine-reports/startup-visibility/build-startup-hidden.ps1)
are prepared and remain **unfiled**. The stable old section anchor is retained
above; its former unretained timing is not current supporting evidence.

### Existing capability and bounded gap

`ApplicationOptions::visible=false` already creates a hidden window, including
its device/services. `Application` and `Window` expose native handles but no
public later-show convenience. A Windows client can use `ShowWindow` through
`Application::window()`; that alternative is available and is not proven
broken. It requires the caller to choose platform show-command, sizing and
focus policy. This client has no production later-show workaround to delete.

Fresh upstream master was fetched on 2026-10-20 at 19:34:53 UTC as
`52e809a5a0229d37db4373faa7853df4c861a310`, with the whole engine worktree
confirmed unchanged against that `FETCH_HEAD`. Win32 chooses `SW_HIDE` for
hidden creation; its default-visible branch requests `SW_SHOWDEFAULT` or
`SW_SHOWMAXIMIZED`. Actual windowed visibility also follows launcher policy.
A hidden smoke launch does not establish normal first-visible startup pixels.

The order in [game/main.cpp](../../game/main.cpp) still matters: initialize,
register kinds, load/validate content, construct the first state and run. The
initial default-visible request precedes device/content setup, but neither the
historical unretained timing nor hidden controls measure an actual first
present. The old claim that one `app.show()` after validation closes startup
is withdrawn: first-state construction and first rendering still follow.

### Measured positive control

Two direct MSVC 19.51.36256.0 `/W4 /WX` builds link the existing D3D11,
XAudio2 and XInput shell/dependencies. Four sequential Debug/Release
windowed/fullscreen-option runs all exited 0 with empty stderr. Each observed
an owned hidden HWND, 1280x720 client/layout/back-buffer sizes and Direct3D 11,
then verified destruction during scope teardown. The fullscreen-option data
is a hidden-creation observation, not monitor-sized geometry or a transition
guarantee.

The source loads no assets or save, starts no state loop, and performs no
drawing, presentation, showing, input injection or foreground request. It initializes
real devices; asset-free does not mean device-free. Runtime checks remain
active in Release. Header inspection establishes the absent public method;
the runtime control establishes only the already-supported hidden half.

Receipts are retained under
`out/validation/crossfeed-20261020/startup-visibility-report-final-v1/`, with
two compile argument/log sets, four stdout/stderr pairs and `result.json`.
The exact source SHA-256 is
`E4CE532B903884BC2B6A06D3CF228554E042251D1FDCD93E7F2BBB9C48DC6A1E`.
The source/build recipe preserve prepared bytes; local Git attributes retain
LF so the recipe's source-hash guard survives Windows checkout.

### The request and remaining startup gate

Consider an explicit public later-show operation or a documented supported
sequence, with launcher show-command, fullscreen sizing and focus/input
effects specified. Intentionally hidden tools must remain hidden unless they
explicitly request otherwise. Documenting native-handle use is a possible
answer; no failing direct-Win32 sequence is claimed. The fullscreen-Apply
visibility issue remains separate report 16.

First-visible game content and `GAP#5.1` remain open. Showing after validation
alone, automatic clear/present, loading-card/transition design, startup timing
and hardware/human acceptance are outside this control's evidence. A retained
rendered/visibility check at the actual first-frame boundary is still needed
before the client's startup half can close.

**Status:** local unfiled request, low priority. Review the prepared report and
refresh upstream immediately before authorized filing through the publication
gate. No engine change, first-frame closure or public issue was made.

---

## 20. Scene drawing gives client objects no view context - **draft with miniature-terrain exhibit**

**View-context request prepared 2026-10-20.** The sanitized
[request](../engine-reports/view-context.md) and standalone
[synthetic null-renderer illustration](../../scripts/engine-reports/view-context/view_context_recording.cpp)
are ready for review and remain **unfiled**. Fresh upstream master was fetched
at 19:34:53 UTC as `52e809a5a0229d37db4373faa7853df4c861a310`; the checked
worktree matched it. The exact 21-translation-unit CMake consumer passed MSVC
C++20 `/W4 /WX` Debug and Release builds/runs, both exit0 with empty stderr
and ten exact expected output lines.

### Source evidence and client workaround

`GameObject::draw(DrawList&)` has no active view index or context parameter.
DrawList exposes camera/viewport setters but no current-value readers. A
caller can already query `Scene::view(index)`, and `cull_bounds` already gets
inverse zoom for pure conservative extent. Neither gives the normal draw its
active range context; culling must not become a mutable handoff between views.

The client miniature path in `draw_miniature_view` and
`Level::draw_miniature_world` retains explicit camera/viewport values, borrowed
scenery admissions and a repeated cull/draw walk. Structure material stays
before its paint and later actors. Its projected strokes and ruler-detail
selection are client art policy. ViewOverlay runs after the world, so it is
not an equivalent dispatch slot for replacing terrain before later objects.

The fixture demonstrates an existing explicit caller-owned context helper:
an ordinary world rectangle records 16x1 and 4x0 at the selected views; a
client minimum-height policy records 16x1 and 4x1. Local coordinates, viewport
origin, caller-state restoration, unchanged object bounds and existing cull
allowance are checked. The helper is client code, not an invented engine API.
Missing read access is public-source evidence, not a runtime API-absence test.

### Narrow ask and limits

Let normal world drawing read or receive its current camera and viewport,
while the client retains art policy. The exact accessor/context/dispatch
shape is upstream's decision. The null fixture does not instantiate Scene or
test ownership, culling, ordering, admission, retirement or render-worker
fan-out. It records geometry, not pixels, readability or performance.

Scene category order and the collision span's unspecified order remain
separate contract concerns. A context API could enable this client's normal
world-slot projection, but deleting every borrowed list or repeated traversal
would need a separate integration check. Freshly fetch and recheck upstream
master before any filing through the
[publication gate](#publication-gate-engpublish).

---

## 21. Digital action evaluation remains client code - **local unfiled draft**

The former `GAP#F2` request is now **ENG#21**. The working
[action evaluator](../../game/action_bindings.cpp) and
[saved Jump mapping](../../game/input_bindings.cpp), consumed by the actual
[PlayerInput](../../game/objects/player_input.cpp), provide the client exhibit.
The proposed extraction is limited to generic physical-source evaluation and
independent source-edge combination; action names/defaults, threshold choices,
save/UI conflicts, menu ownership and roster policy stay in the client.

The [sanitized request](../engine-reports/action-evaluation.md) accompanies a
[standalone synthetic example](../../scripts/engine-reports/action_evaluation.cpp).
It demonstrates changing the binding to an already-held key without inventing
a press, and overlapping button/trigger sources whose edges remain independent.
It asks for no new slot lookup, trigger threshold or pad-capture API.

**Checked and exercised 2026-10-20.** A fresh upstream fetch at 19:34:53 UTC
confirmed master `52e809a5a0229d37db4373faa7853df4c861a310`; the relevant source
matches that `FETCH_HEAD`. The engine still explicitly defers action mapping
above its existing device primitives. The synthetic example passed with MSVC
19.51.36256.0 under `/W4 /WX` in Debug and Release, linked only to the prebuilt
`LabradorEngine.lib` and `MattMath.lib`, without game objects, window or
graphics/audio/physical-input devices.
The report records the exact flags, reproduction commands and observed output.
T1 supports considering extraction from this newly exercised consumer; the
engine's API shape and decision remain upstream's.

Local receipts are under
`out/validation/crossfeed-20261020/action-evaluation-repro/`, with the fresh-fetch
and source checks alongside it in `action-report-upstream.txt` and
`action-report-source-recheck.txt`. These are synthetic implementation checks,
not hardware, physical-input, human or performance acceptance. No issue has
been filed, the engine is unchanged, and the approved intake route above still
governs any later publication. Refresh upstream immediately before filing.

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
  `end_contacts`. The Debug overlay `overlay.stations` holds one `Label` per
  station in the level's scene. Using `Label` turned up report 18 above.

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
