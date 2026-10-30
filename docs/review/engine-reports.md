# Reports to file against Labrador

> The running list of API complaints this client owes the engine. Each one is a
> case where the game wanted something ordinary and the API forced a bad
> answer, and each carries an **exhibit**: the workaround that is in this
> repository, in a file you can open, written because the engine offered
> nothing better.
>
> **Three are live (23, 25 and 26), all found after the engine's last tagged
> answers.** Engine master answered 23 and 25 on 2026-10-27, after `v0.2.0`, so
> they wait for the first minor tag carrying those answers; 26 is an unfiled
> draft. **Twenty-three are closed** (1–22 and 24) and kept below as one paragraph
> each — what was asked, what the engine answered (shipped, document only,
> declined or waiting on evidence), and what this repository spent the answer
> on, if anything. A report is closed when it is answered, whether or not the
> answer was the one asked for.
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
[Report 15](#15-runtime-packaging-and-content-staging-are-repeated-in-the-client--document-only-48fb236)
updates gap-audit F8 against the release machinery this game has since built.
The engine's PI-09/PI-10 already reserve filing and a decision; PI-11 is a
conditional implementation story, not an implemented answer.

**Moved 2026-10-22 to upstream master `0c15f34`.** Two commits: the engine's
agent brief became `AGENTS.md` imported by `CLAUDE.md`, plus website
explorations. Under `engine/` only a comment in `engine/CMakeLists.txt`
changed, so nothing closed and no citation here moved; both presets, consumer
acceptance and the startup check pass on the new pin.

**Moved 2026-10-22 to upstream master `19cf650`**, in `6a5d83c`. Fifteen
commits, and they answered every live report, 6 and 9-22: each has a recorded
decision and all fifteen are now [closed](#closed). One source break, from
report 13's answer. See
[What the 2026-10-22 bump changed](#what-the-2026-10-22-bump-changed).

**Moved 2026-10-25 to upstream master `1e60fb2`** (`v0.2.0`). Fifteen commits,
and one of them, `814f2ee`, answered report 24 the day it was drafted, without
it being filed. See
[What the 2026-10-25 bump changed](#what-the-2026-10-25-bump-changed).

**Re-checked 2026-10-29 against upstream master `b96bad6`**, fetched that day;
the pin stays at `1e60fb2` (`v0.2.0`), 182 commits behind. `42bc478` answers
report 23 and `cadc962` answers report 25, both on 2026-10-27, and neither is
in `v0.2.0`. Labrador's `docs/review/colourwars-reports/README.md` ("Filing
recheck, 2026-10-27") records both as answered without a public issue, which
closes its `PI-09` filing queue. Neither report needs filing. Their workarounds
are deleted at the first minor tag that carries the answers, under the pin
policy in AGENTS.md. Report 26 is unanswered on master (`3a14f11` only
rewrote `Registry`'s comments). Master also has source breaks and a manifest
rule that will stop this game at that bump; they are scoped under
[Preparing the next bump (2026-10-29)](#preparing-the-next-bump-2026-10-29).

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

**None of the approved drafts was ever filed, because none needed to be.**
Thirteen were present at approval; report 21 later added a fourteenth and
report 22 (2026-10-22) a fifteenth. Labrador read all fifteen from this
repository and recorded a decision on each (`cacf112`) before any went through
the issue route, then answered them by `19cf650`, and all fifteen are
[closed](#closed). The 2026-10-21 provenance recheck of fourteen exhibit
sources against `52e809a` (under `out/validation/crossfeed-20261020/`) is
historical with them; every exhibit it covered has been deleted.

**The route is now for future reports**, and report 23 (2026-10-22), found
while spending 16, is the first. Its minimal request and guarded example were
prepared and qualified locally on 2026-10-24. The read-only upstream-master
check at 02:24:30 UTC resolved to the same pin `19cf650`. The current task
forbids filing; separate author authorization and a fresh upstream recheck
are required before using the established route above.
This supporting work does not change TODO's level-first priority.

**2026-10-29:** the route is no longer needed for 23 or 25. Engine master
answered both without a filing (see the 2026-10-29 recheck above), so the
route now applies only to report 26 and to later drafts.

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

The fifteen after those (6, 9-22) were answered without being filed, all in
the 2026-10-22 bump: eight accepted, most of them narrowed (6, 11, 13, 14, 16,
17, 20, 22), one split (12), four document only (9, 10, 15, 18), one waiting on
evidence (19) and one declined (21). Every one had an exhibit, and the
declines and document-only answers were argued against the exhibit, not
against the ask.

Report 6 got its client-side mixing exhibit and was answered before it was
filed ([Closed](#6-no-audio-bus-master-gain-listener-or-voice-cap--accepted-narrowed-19cf650)).

---

# Live

| # | Report | Exhibit | Status |
|---|---|---|---|
| 23 | Resizing a hidden window activates it | Five `SettingsProbeActivationGuard` uses and a [guarded Window-only example](hidden-window-resize/hidden_resize_exhibit.cpp) | answered on untagged master in `42bc478` (2026-10-27), rechecked 2026-10-29 at `b96bad6`; no filing needed; the guards go at the first minor tag carrying it |
| 25 | No public controller vibration output | [Isolated XInput adapter](../../game/rumble_xinput.cpp) and [callback-only checks](../../scripts/acceptance/rumble_adapter.cpp) | answered, narrowed, on untagged master in `cadc962` (2026-10-27), rechecked 2026-10-29 at `b96bad6`; no filing needed; the native transport goes at the first minor tag carrying it |
| 26 | `Registry` cannot forget a name | [`StageCatalog`](../../game/stage_catalog.h) keeps a vanished user stage's definition registered after a rescan | drafted locally 2026-10-28 against pin `1e60fb2`; rechecked 2026-10-29 at master `b96bad6`, still unanswered; narrowed 2026-10-30 to reclaiming memory (`HYG#47`); unfiled, awaiting separate authorization |
| 27-32 | Six owed by the 2026-10-30 architecture review | See [Owed from the architecture review](#owed-from-the-architecture-review-2026-10-30) | found 2026-10-30; not yet drafted or rechecked against master |

Nothing else is outstanding, on either side, beyond reports 23, 25 and 26 below, the six
[owed by the architecture review](#owed-from-the-architecture-review-2026-10-30), and the
evidence [report 19](#19-no-public-later-show-convenience-for-a-hidden-application--needs-more-evidence-cacf112)
leaves this repository to supply. The fullscreen workaround the
audit flagged under [What is deliberately not here](#what-is-deliberately-not-here)
has since been driven and fixed — keyboard support made the Options page
reachable on a machine with no pad.

---

## Owed from the architecture review (2026-10-30)

The [2026-10-30 architecture review](2026-10-30-architecture-review.md#engine-reports-owed)
found six places where the engine's API forced the answer this repository has,
and working its `HYG#49` found a seventh (33).
Each is numbered here so TODO can point at it. All seven were drafted on
2026-10-30 against engine master `47be5a3`, below report 26, each fact-checked
by a second reader. 31 and 32 are already answered on untagged master and need
no filing; they wait on the next minor tag like 23 and 25. 27-30 and 33 are
drafts that wait for the author's authorization to file.

| # | Report | Exhibit at `96c65373` |
|---|---|---|
| 27 | No headless or recording pad backend; a consumer test can substitute `GamepadReader` only by redefining it | the `namespace labrador` blocks in `scripts/acceptance/action_bindings.cpp`, `input_bindings.cpp`, `keyboard_jump_capture.cpp` and `painter_visual.cpp`; a test seam, separate from the pad-coverage "XInput is the only pad reader" entry under [What is deliberately not here](#what-is-deliberately-not-here), which stays |
| 28 | Public types removed without a deprecation release, after report 7's procedure | `game/carried/` and the `HYG#25` removed-member list |
| 29 | `Scene` gives no typed access to short-lived objects and no retirement notice | the tag test and downcast loops at `game/objects/level.cpp:442-452` and `:606-614`; since `HYG#42` (2026-10-30) those loops ask each object for its role, and the downcast they paid is the one cross-cast in [`Collider::of`](../../game/objects/collider.cpp), which every scene walk and every contact now pays instead |
| 30 | A device-unavailable startup failure can be classified only by its message text | `game/fatal_report.cpp:113`, `scripts/check-startup.ps1:225` and `:248` |
| 31 | `JsonValue` cannot report keys nobody read, so a misspelled optional key is silent | the optional `has()` reads in `game/tuning.cpp` (`:461`, `:477`, `:484` and others) |
| 32 | Word wrap by measured width is client code; the engine backlog's OCA-77 premise ("no caller in either client wraps by width") is false | `game/text_wrap.h` and its caller `WrappedTextDropShadow` |
| 33 | `CollisionObject::on_contact` hands the other party over `const` (unchanged on master `8e91aab`), so a response that changes both parties has no route back to the other one | [`Projectile::answer_paint_contact`](../../game/objects/projectile.h), the one `const_cast` in `game/`: a painted surface's answer reaches the projectile through it, where it used to reach `mutable` flags through a `const` method (`HYG#49`, 2026-10-30) |

## 23. Resizing a hidden window activates it — **draft with activation-guard exhibit**

**Answered on untagged master (rechecked 2026-10-29 at `b96bad6`).**
`42bc478` (2026-10-27, *Keep window resizing from requesting activation*)
adds `SWP_NOACTIVATE` to `resize_client`'s `SetWindowPos` call, with an engine
regression for both hidden window modes. Engine `CHANGES.md` lists it as a
silent behaviour change under Unreleased. It is not in `v0.2.0`, so the pin
does not have it. No filing is needed. At the first minor tag that carries
it, delete the guard and its five uses as below, then close this report.
The text below is the draft as prepared.

Found 2026-10-22 while spending [report 16](#16-windowed-apply-shows-a-deliberately-hidden-window--7db3d5c),
against the pin `19cf650`. A read-only upstream check on 2026-10-24 at
02:24:30 UTC resolved master to the same full pinned revision.

**Prepared locally (2026-10-24), unfiled:** the
[sanitized request](hidden-window-resize/sanitized-request.md),
[guarded Window-only example](hidden-window-resize/hidden_resize_exhibit.cpp)
and [portable build recipe](hidden-window-resize/build-exhibit.ps1) are ready.
Debug and Release builds from that repository location each observe one
activation request and veto it, count zero focus requests, report an 800x600
client, and preserve sampled foreground equality after resize and destruction.
The caller's console-free launch preserves the foreground too. No renderer,
audio device, game assets or unguarded takeover control is used.
Exact source/compiler/cache/library/binary pins and results are under
`out/validation/hidden-resize-20261024/final-v5/qualification-v5.json`; the
earlier private source-review and qualification records remain separate.

Filing requires separate author authorization; the current task forbids it.
This preparation records the client's request, not an engine answer. Keep the
five activation guards until an answer is consumed and requalified.

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

The synthetic example above installs a current-thread CBT veto before Window
construction and retains it through destruction. It filters activation/focus
requests by its own process and thread so it can guard the not-yet-returned
HWND. Exit 0 records a vetoed activation request and the sampled isolation
conditions; exit 2 is explicitly inconclusive. It does not demonstrate actual
foreground/focus theft. The README records the Windows-reported OS version,
compiler/runtime and build/run instructions without machine or user identifiers.

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

## 25. No public controller vibration output - **draft with isolated adapter exhibit**

**Answered, narrowed, on untagged master (rechecked 2026-10-29 at `b96bad6`).**
`42bc478` recorded the decision (slot-addressed output belongs behind the
input seam, as engine story `PI-21`). `cadc962` (2026-10-27) implements it:
`Gamepads::vibrate(slot, Vibration)` returns a `VibrationResult` and
`stop_vibration(slot)` stops the pad. Each request is bounded to
`max_vibration_seconds` (5 s), and the engine ends requests at a stop, a newer
request, a pad leaving its slot, suspension or destruction. Neither commit is
in `v0.2.0`. No filing is needed. At the first minor tag that carries it,
replace the native request and factory and the `Xinput.lib` link with that
call. Pulse timing, preferences and the pause, replay and bot gates stay in
the client. Then close this report. The text below is the draft as prepared.

**Upstream checked 2026-10-26, 14:55 UTC.** GitHub's published `master` resolved
to [`c03a6f653f3d524fae53b6cb662074ec3f2bb48a`](https://github.com/matt439/Labrador/commit/c03a6f653f3d524fae53b6cb662074ec3f2bb48a).
The immutable [Gamepads header](https://github.com/matt439/Labrador/blob/c03a6f653f3d524fae53b6cb662074ec3f2bb48a/engine/input/gamepads.h),
[reader header](https://github.com/matt439/Labrador/blob/c03a6f653f3d524fae53b6cb662074ec3f2bb48a/engine/input/gamepad_reader.h),
`gamepad.h` and XInput reader implementation still expose snapshots, edges,
connection and suspend/resume, with no vibration output or capability query.
Separate repository issue searches for rumble and vibration returned no match.
This check did not move the client's engine pin or change the engine checkout.

**Exhibit.** [game/rumble_xinput.cpp](../../game/rumble_xinput.cpp) converts two
normalized motor amplitudes into 16-bit speeds and calls `XInputSetState` using
the same physical user indices as the engine's reader. ColourWars consequently
names a native pad API and links SDK `Xinput.lib` outside that reader's backend.
The [factory seam](../../game/rumble_sink.h) isolates this exception. Its injected
transport exercises conversion, invalid-value stop, slot bounds, unavailable
devices, failure and lifetime stop through the same adapter code.

**Reproduction and expected result.** A client can read slot 0 through
`Application::gamepads()->state(0)`, but neither that service nor its reader has
an operation for setting that slot's motor output. The client must add a second
native transport, as this exhibit does. A small engine output operation using
the existing slot contract would remove that transport. Keep pulse timing,
game events, per-slot preferences, pause/replay/bot gates and ownership in the
client; they are not part of this request. Unsupported/disconnected output
should be a defined harmless result. Zero output must have an explicit stop
meaning. Exact spelling, validation and backend support remain engine decisions.
Capability discovery is a separate proposal: this client has no demonstrated
capability-dependent caller, so the report does not require one.

**Qualification and limits.** Both game presets and `rumble_adapter` pass;
the callback-only case never invokes XInput. Both-preset actual-GameLevel and
settings probes use a recording factory, including lifecycle and automation
gates. No physical motor or feel acceptance is claimed. Receipts are under
`out/validation/overnight-20261026/phase5/`. An engine answer could replace the
native request/factory and direct SDK dependency; the client policy and its
regressions remain useful. This is a local draft, not a filed issue. Separate
authorization and a fresh upstream recheck are required before filing.

## 26. `Registry` cannot forget a name - **draft with rescan exhibit**

**Drafted against the pin** (`1e60fb2`, 2026-10-28). `labrador::Registry`
(`engine/core/registry.h`) offers `add`, which replaces the resource of a name
it already holds, plus `resolve`, `get` and `contains`. It has no way to remove
a name.

**Rechecked 2026-10-29 against upstream master `b96bad6`: still unanswered.**
The only change to the header since the pin is `3a14f11` (2026-10-29,
*Clarify Registry presence and retained resource handles*), and it changes
comments only. Master's header comment says a name owns its slot for the
registry's lifetime. `contains` answers false for an empty or released slot,
the name still resolves to its handle, and `get` throws until the slot is
refilled. Per-name removal is still absent. The only bulk operation is
`release_all`, which empties every slot and keeps every name. That empty-slot
behaviour is the pin's code too; `3a14f11` only documents it. So the
`contains` trap below is a client choice. The rescan could
`add(name, nullptr)` a vanished stage, after which `contains` says no and
`get` throws "released". The name and its slot would still stay, so the ask
narrows to forgetting the name itself. Filing still needs separate author
authorization.

**Exhibit.** `ROAD#16` rereads the player's stage folder each time Stage Select
opens from the main menu ([`UserStageFolder::rescan`](../../game/stage_loading.cpp)).
A new or edited file goes through `add`, which is what the rescan needs. A
file the player deleted, or one that now fails its checks, can only leave the
stage list. Its old `LevelLoadedInfo` stays in the registry, unreachable from
the list, until the game exits.

**Narrowed 2026-10-30 (`HYG#47`).** The list and the registry now have one
owner and one writer, [`StageCatalog`](../../game/stage_catalog.h), and its
by-name lookup `definition` answers from the list, so a stage that is gone is
not found there. The registry's own `contains` still answers yes for one, but
only whole-registry readers (`LevelBuilder`, the content and sound-group
checks) are handed it. What is left is the memory the vanished definitions
hold until the game exits. Every client that reloads user content over a
session would still meet that.

**The ask.** A way to remove a name (or to replace the whole set) with a
defined meaning for handles already resolved to it. The spelling, and
whether stale handles throw or stay invalid, are engine decisions.

**What an answer lets this repository delete.** The paragraph about
unreachable definitions in `game/stage_catalog.h` and the roadmap's note
about stale entries. `StageCatalog::remove_user_stages` would remove the
names that left the list, so the registry would agree with it. This is a
local draft, not a filed issue.

---

## 27. No headless or recording pad backend - **draft with link-seam exhibit**

**Checked against upstream master `47be5a3` (2026-10-30): still unanswered.**
`LABRADOR_INPUT_BACKEND` accepts only `xinput` and `web`
(`engine/CMakeLists.txt:360-379`), and `engine/input/` has only those two
backend folders. Render and audio each have a `null` backend that records what
it was asked to do. Input has none. `GamepadReader`
(`engine/input/gamepad_reader.h`) is still one concrete class with a private
`Impl`, and its implementation is chosen at build time. `Gamepads` takes only
that class (`engine/input/gamepads.h:36`). The engine's backlog has hit the same
wall from the sample side. The OCA-45 notes in `docs/product/STORIES.md` say,
of `samples/minimal`, that `Gamepads` "is built only over the real backend's
GamepadReader ... so a headless test is not available without new seams".
SM-40 (closed 2026-10-27) and G125 add a headless test of the XInput
*translation*, `gamepad_state_from_xinput`. They do not add a reader a consumer
can script. The engine's own `InputTests` build the real reader and use empty
slots only (`tests/input/vibration_tests.cpp:279-280`). A search of the
repository's issues and PRs for `GamepadReader` found nothing relevant.

This is not the "XInput is the only pad reader" entry under
[What is deliberately not here](#what-is-deliberately-not-here). That entry is
about pad coverage: a GameInput backend for DualSense, DualShock or Switch Pro,
under `ROAD#2`. It stays, and its "no exhibit" still holds for that subject.
This report is about a test seam. It has an exhibit, in four places.

### Exhibit

Four acceptance probes need to set a slot's state without a pad, a window or
XInput. Each one does it by redefining every `labrador::GamepadReader` member,
`Impl` included, in its own translation unit. The probe's object file then wins
over `LabradorEngine.lib`'s `xinput/gamepad_reader.obj` at link time:

- [action_bindings.cpp:18-36](../../scripts/acceptance/action_bindings.cpp) feeds `action_test_pads::states` through `sample()` and keeps a `suspended` flag.
- [input_bindings.cpp:9-23](../../scripts/acceptance/input_bindings.cpp) does the same thing with `binding_test_pads::states`.
- [keyboard_jump_capture.cpp:23-34](../../scripts/acceptance/keyboard_jump_capture.cpp) defines a reader whose slots are always empty, only so a `Gamepads` can be built.
- [painter_visual.cpp:77-109](../../scripts/acceptance/painter_visual.cpp) uses `painter_scripted_pads::states` under `COLOURWARS_PAINTER_SCRIPTED_PADS`. `scripts/check-painter.ps1:409` passes that define only for the scripted-pad variants, so the other painter binaries keep XInput.

### What happens

This substitution depends on the engine's private layout and on link order, not
on any API. It also breaks whenever the seam grows. On master,
`GamepadReader` has two more members, `vibrate` and `stop_vibration` (PI-21,
`cadc962`), and `Gamepads::vibrate`/`stop_vibration` call them
(`engine/input/gamepads.cpp:111`, `:119`). At the next pin bump, each of the
four blocks has to define them as well. If a block misses one, the linker
should pull the XInput object to resolve it, and that object defines the other
members a second time. This is inferred from the source; nothing has been built.
Every consumer that wants to drive `Gamepads` in a test meets the same problem.

Reproduction: in a test target that links `LabradorEngine` (xinput), try to put
`GamepadState{ .connected = true, ... }` into slot 0 of a `Gamepads` without a
physical pad. No public way exists. The only route is to define
`labrador::GamepadReader` yourself, as the four files above do.

### The ask

A pad reader that a consumer's test can link next to the normal engine build
and set per slot. A test must be able to set what `read(slot)` answers, and it
must see `suspend`/`resume` and any vibration requests. It must also not need a
second engine build, because the probes link the same `LabradorEngine` as the
game. A `null` reader that records what it was asked, like the render and audio
ones, would do, but only if it ships beside the normal backend and not in its
place through a configure-time choice. A scripted reader in its own library, or
a constructor seam on `Gamepads`, would also do. The spelling and the mechanism
are the engine's call.

### What an answer lets this repository delete

All four `namespace labrador` blocks. The three state arrays they read
(`action_test_pads`, `binding_test_pads`, `painter_scripted_pads`) become calls
to the engine's setter. This repository would also no longer have to edit four
probes whenever `GamepadReader` gains a member. The PI-21 bump will be the first
time that is needed. `COLOURWARS_PAINTER_SCRIPTED_PADS` can stay if it still
selects scripted pads over physical ones.

This is a local draft, not a filed issue. Filing it needs separate author
authorization and a fresh check of upstream master.

---

## 28. Public types and members removed with an engine-only caller count - **draft with carried-copy exhibit**

**The row's original ask, a deprecation release, is already answered on the pin
and on master: it is declined by written policy. This report narrows to the
count.** Upstream checked 2026-10-30 against engine `origin/master` `47be5a3`
(2026-10-30). The only tags are `v0.1.0` and `v0.2.0`, so both removals below
sit under `## Unreleased` in `CHANGES.md` (`:78`). `c369e8b` (2026-10-13,
*Version releases now, and say how a public name leaves*) was already in the
pin `1e60fb2`. On master, `PHILOSOPHY.md:835-840` says a name leaves through a
`CHANGES.md` entry carried by the next tag, and that "Before 1.0 there is no
alias and no `[[deprecated]]`" (`:837`). `CHANGES.md:47` says "There is no
deprecation machinery". Engine story `OCA-69` (gap `G19`) is checked done in
`docs/product/BACKLOG.md:117`. So this report does not ask for a deprecation
release.

The same section also states the rule two removals did not follow. Report 7's
answer (`343433c`, 2026-08-15) added the requirement that a caller count say
which repositories it counted (`PHILOSOPHY.md:810-819`). `c369e8b` extended it
at `:820-833`, and both are in the pin. Under the extended rule, the commit
that removes a public name names a count over three places: the engine, the
paint-shooter's `master`, and the public repositories pinning the engine. The
same passage says "having no caller is never the reason a name goes" (`:827`)
and "a name with no caller and no cost stays" (`:830`).

- `730dd99` (2026-10-27, *Remove engine types nothing uses*) deletes
  `MovingObject`, `RotationOrigin`, `SoundBankObject`, `AnimationObject`,
  `CameraTools`, `TextDropShadow` and `UiTextDropShadow`. Its message counts
  only "the engine, the samples, the bench or the tools". Its entry
  (`CHANGES.md:320-339`) gives no cost argument. At `:333` it says "There is
  no replacement. A game that used one should copy the class from the
  previous release".
- `fdfc24e` (2026-10-27, *Remove public members that nothing calls or
  tests*) removes 28 members. Its count covers "the engine, the samples, the
  tests or the benchmark" (`CHANGES.md:259-278`). Unlike `730dd99`, it gives
  a cost: each member is "a promise the engine keeps without evidence that it
  holds", with "no test, which a public primitive needs". It also gives a
  migration. Only the count is missing here.

Neither count names the paint-shooter's `master`, and that branch held callers
of both. At `7c0b7aba`, `master` on the removal date, `player.h`,
`projectile.h`, `figure_clothing.h`, `paint_tile.h` and `debug_menu_page.h`
derived from `labrador::` copies of the removed types. `level.cpp` and
`level_intro.cpp` called `Camera::calculate_intermediate_camera`. No later
master commit revisits either removal. `gh issue list -R matt439/Labrador
--state all` returned no issues at all.

**Exhibit (HEAD `f5303302`, branch `arch-review-3`; the carried copies arrived
in `cc0a2fff`, 2026-10-29, and are not on `master` yet).**
[game/carried/](../../game/carried/) holds 13 files and 1,028 lines copied
from `v0.2.0` into `namespace colourwars`. Each file has a provenance note
(`game/carried/moving_object.h:3`). The copies are hierarchy rungs, not
helpers. Fifteen class heads outside `game/carried/` name a carried type as a
base:

- `player.h:106-109` (`MovingObject`, `AnimationObject` and `SoundBankObject`)
- `projectile.h:27-28`, `menu_page.h:34`, `level.h:85`, `weapon.h:48`
- `figure_clothing.h:10`, `paint_tile.h:29`, `screen_music.h:10`
- `connection_audio.h:10`, `resume_audio.h:8`, `match_voices.h:14`
- `movement_audio.h:20`, `paint_audio.h:11`
- `prompt_bar.h:13` and `wrapped_text_drop_shadow.h:13`

Outside `game/carried/`, 41 files include a carried header. 94 source files
name a carried type as `colourwars::`, and 56 of them name
`colourwars::UiTextDropShadow`.

The repository still builds against the pin, where `fdfc24e`'s members exist.
Each of these calls becomes a source break at the bump:

- `Camera::calculate_intermediate_camera`: `game/objects/level.cpp:396`,
  `:627` and `level_intro.cpp:246`
- `RectangleF::set_position_y_from_bottom`: `player.cpp:910`, `:1308` and
  `:1317`
- `RectangleF::from_top_left_bottom_right`: `level_object_builder.cpp:117`
- `UiTexture::set_position_from_top_right_origin`:
  `game/states/results_menu.cpp:445`
- acceptance, all `from_top_left_bottom_right`:
  - `scripts/acceptance/cq_blockout/fixtures.cpp:79`
  - `crossfeed/fixtures.cpp:79`
  - `press_tower/fire_checks.cpp:38`
  - `press_tower/fixtures.cpp:100`
  - `terrain.cpp:110`
  - `painter_visual/surface_finish_checks.h:114`

  The last one is missing from the removed-member list under
  [Trial-build against engine master (`HYG#24`)](#trial-build-against-engine-master-hyg24),
  and that list's line numbers are stale (for example, `level.cpp:384` there
  is `:396` now).

**Reproduction.**

1. On engine master, run `git show 730dd99 -s` and `git show fdfc24e -s`.
2. Compare each count with `PHILOSOPHY.md:820-833` on the same revision.
3. Take any client that builds at `v0.2.0` and derives from
   `labrador::SoundBankObject` or calls `Camera::calculate_intermediate_camera`.
   A three-line class and one call are enough. Build it against master. It
   fails to compile.

Neither commit nor its `CHANGES.md` entry says whether a counted client held
the name. `730dd99` also gives no reason beyond having no caller, which `:827`
says is never the reason a name goes.

**The ask.** Before the tag that carries `730dd99` and `fdfc24e`, bring both
under the engine's own rule.

- For both entries, add the three-place count: what the engine, the
  paint-shooter's `master` and pinning public repositories each held.
- For `730dd99`, also either state the cost of keeping each type (T1, T2, T4
  or T5), or restore the types whose only stated reason was having no caller.

Which names go, and whether any come back, are engine decisions. An alias or a
`[[deprecated]]` period is not asked for, because `PHILOSOPHY.md:837` declines
both before 1.0.

**What an answer lets this repository delete.**

- If the types come back: `game/carried/`, its includes in 41 files, and the
  `colourwars::` qualifiers in 94. The rungs go back on `labrador::` at the
  same depth.
- If the members come back: the local replacements the bump would otherwise
  need at the 8 game and 6 acceptance sites above.
- If the entries are only amended: nothing in code changes. The carried copies
  are then the documented migration, and this report closes as answered.

This is a local draft, not a filed issue. Filing needs separate author
authorization and a fresh upstream recheck.

Ledger table row, refreshed:

| 28 | Public types and members removed with an engine-only caller count, against the rule from report 7 (`343433c`) as extended by `c369e8b` (`PHILOSOPHY.md:810-833`); the deprecation-release ask is declined by policy (`PHILOSOPHY.md:837`) | `game/carried/` (13 files; 15 class heads derive from it), plus the `fdfc24e` call sites `game/objects/level.cpp:396`, `:627`, `level_intro.cpp:246`, `level_object_builder.cpp:117`, `player.cpp:910`, `:1308`, `:1317` and `game/states/results_menu.cpp:445`, at `f5303302` |

---

## 29. `Scene` gives no typed access to short-lived objects and no retirement notice - **draft with scene-walk exhibit (narrowed: may not be owed)**

**Checked 2026-10-30 against engine master `47be5a3` (2026-10-30): unanswered as asked, but the premise does not hold.**
Between the pin (`1e60fb2`, `v0.2.0`) and master, `engine/scene/scene.h` and
`engine/collision/collision_object.h` changed only cosmetically: include order,
`Camera::default_camera`, and unqualified `RectangleF`/`Shape`/`Vector2F`.
`Scene::add<T>` returns the object's own type ("the object's own type rather than
the list's"). That pointer "stops being valid when the object leaves the
scene". An object leaves through `remove()`, which the caller asks for, or
through retirement. Retirement is checkable from public state. `end_tick()`
(`engine/scene/scene.cpp:150-177` on master) clears the contacts. It then calls the
object's virtual `set_for_deletion(true)` on every collision object that is
`!in_bounds()`, and erases exactly those whose `for_deletion()` is true.
`for_deletion()` and `Scene::in_bounds()` are both public. Between `resolve()`
and `end_tick()`, a caller can therefore tell which kept pointers are about to die.
The engine's backlog calls `set_for_deletion` "a collidable's retirement hook"
(`docs/product/BACKLOG.md:154`, G49/OCA-59, done). On master, `scene.h` also
states the engine's position on the phase this report wants. Under "PHASE 3 is
the game's, and it has no function on this class", it declines a per-object
virtual as the frame-loop tax T8 refuses, and prescribes "a loop over the objects
it already holds pointers to, between resolve() and end_tick()". The
`collision_objects()` comment says the game's own questions are "a walk, once,
not a virtual on the object". `CHANGES.md` records the stable-erase order of
retirement. No story, decision or plan covers typed access or a destroy-time
notice. A read-only `gh` issue search on `matt439/Labrador` found no match.
Filing needs separate author authorization. Read the next section before you
ask for it.

### What happens

A projectile is the scene's commonest object and the shortest-lived. The game
creates it as a `std::unique_ptr<Projectile>` and gives it to `Scene::add`,
which returns a `Projectile*`. The game discards that pointer. Every tick it
reaches the projectiles again by walking `collision_objects()`, which yields
`CollisionObject*`, and recovering the game type from each element. It drains
each projectile's pending output (paint audio, a blocked-shot cue) in a second
such walk before `end_tick()`.

None of this is forced by the engine. The game could keep `add`'s typed
pointers in a list of its own and, in the documented PHASE 3 loop, drain and
drop each one for which `for_deletion() || !scene.in_bounds(*p)`. Pointers that
`add` returned this tick are still pending and are not swept until the next
`end_tick()`. That is the pattern `Level` already uses for long-lived objects
(`projectile_targets_`, `game/objects/level.h:351-353`). What the engine does
not give is a single guarantee, stated as such, that this predicate *is* the
retire set. Today a client relies on reading `end_tick()`'s order (bounds
sweep, then retirement on `for_deletion()`) together with `set_bounds`'s and
`add`'s comments.

### Exhibit (client HEAD `f5303302`)

- [`game/objects/level.cpp`](../../game/objects/level.cpp) `:555-569`
  (`update_players`) gives each new projectile to `scene_->add` at `:566` and
  discards the `Projectile*` it returns.
- `:571-582` (`resolve_projectile_movement`) walks every collision object to
  find the projectiles before contacts. The role query through `Collider::of`
  is at `:577`.
- `:454-464` (in `update_level_logic`, after `scene_->resolve()` at `:453`)
  walks every collision object again through `Collider::of`. It drains each
  projectile's paint audio and blocked contact before `end_tick()` at
  `:485-487`.
  [`game/objects/player.h`](../../game/objects/player.h) `:148-149` records the
  same flush-before-retire rule for shots that retire inside the weapon update.
- `:821-832` (`count_projectiles`) walks a third time, testing each tag
  (`:826`, no cast), to count what a kept list's size would already say.
- [`Collider::of`](../../game/objects/collider.cpp) `:9-19` is one
  `dynamic_cast` from `labrador::CollisionObject` to the game's base type.
  Since `HYG#42` (2026-10-30), the two role-query walks above pay it once per
  object, for every structure and player as well as each projectile. That cost
  comes from walking rather than keeping `add`'s typed return. The contact path
  pays the same cast in `Collider::on_contact`, but that is report 33's
  neighbourhood (`on_contact` hands over a `CollisionObject`), not this one.
  [`collider.h`](../../game/objects/collider.h) `:25-28` names this report.

### Reproduction

This needs no game assets. Derive `Shot` from `CollisionObject`, overriding
`for_deletion`/`set_for_deletion` to record the flag, and set it after one
update. Give one to a `Scene` with `add` and keep the returned `Shot*`. Before
each `end_tick()`, test `shot->for_deletion() || !scene.in_bounds(*shot)`. The
test is true on exactly the tick whose `end_tick()` destroys the shot. So a
typed pointer *can* be kept safely today, but only by restating the scene's
retirement rule in client code.

### The ask

Narrowed to what the engine owns. A document-only answer would close it.

1. **State the retire set as a contract**: between `resolve()` and
   `end_tick()`, a collision object will be retired by that `end_tick()` if and
   only if `for_deletion()` is true or `in_bounds()` is false (after the
   object's own `set_for_deletion(true)` answer). Then a client keeping `add`'s
   pointers may rely on it. Optionally, expose it as one query (for example a
   `Scene` member answering "retires at this `end_tick()`") so the rule lives in
   one place. The spelling is the engine's call.

A destroy-time callback or a scene-kept per-type view is deliberately not
asked. Master's PHASE 3 comment already declines the first under T8. The
second would have the engine index game types that `add`'s typed return
already lets the client keep.

### What an answer lets this repository delete

Nothing that cannot be deleted now. With or without (1), `Level` can keep a
typed list of live projectiles, filled from `add`'s return and pruned in the
PHASE 3 loop by the predicate above. That removes the three walks at `:454-464`,
`:571-582` and `:821-832`, the per-object cross-cast the first two pay, and the
tag test in `count_projectiles`. (1) only makes the pruning rule an engine
promise rather than a reading of `end_tick()`. `Collider::of` would remain for
the contact's other party (report 33). Its comment's pointer to this report
can then go. Because the exhibit is the game's choice and not forced by the API,
the author may prefer to withdraw this row and do the client-side change
instead. This is a local draft, not a filed issue.

---

## 30. A device-unavailable startup failure can be classified only by its message text - **draft with string-match exhibit**

**Checked against upstream master `47be5a3` (2026-10-30): still unanswered.**
That is 214 commits past the pin `1e60fb2` (`v0.2.0`). The Direct3D 11 backend
still refuses a machine with no hardware adapter by throwing a plain
`std::runtime_error("No Direct3D hardware device found")` in Release
(`engine/render/d3d11/device_resources.cpp:161-165` on master; `:164` at the
pin). The other backends' equivalent refusals are also plain
`std::runtime_error`s, each with different text:
- `engine/render/d3d12/device_resources.cpp:226`: `No Direct3D 12 hardware device found`
- `engine/render/vulkan/device_resources.cpp:560`: `This machine has a Vulkan loader but no Vulkan device. ...`
- `engine/render/vulkan/device_resources.cpp:667` (`:647` at the pin): `No Vulkan device on this machine can run this backend:` plus one line per device
- the OpenGL backend's driver refusals in `engine/render/gl/wgl_context.cpp`, each with its own message

No exception type, error code or category marks "no usable device". The one
engine exception subclass on this path, `com_exception`
(`engine/render/throw_if_failed.h:50`), carries a COM `HRESULT` for any failed
call, and its own comment says a catch site cannot tell one from another.
`Renderer::device_info()` (`engine/render/SEAM.md:85-88`) classifies a device
as hardware, software or null only after one has been created. The engine's
own release guide (`website/src/content/docs/docs/guides/windows-release.mdx:199-212`)
tabulates the refusals by their message text, so the text is the only
interface there is. Nothing in master's `CHANGES.md`, `docs/product/BACKLOG.md`
or `docs/design` plans a typed refusal. `GAP-REGISTER.md:2741` mentions the
D3D11 refusal only as a raw string the sample shows in a message box. Two
read-only `gh` issue listings on Labrador (all states, and a search for
"device") exited 0 with no rows. Filing still needs separate author
authorization.

**Exhibit** (client at `f5303302`). This repository has to tell one startup
failure apart from the rest, in two places:

- [`FatalReport::capture`](../../game/fatal_report.cpp) at `:117-120` compares
  `what()` with `std::strcmp` against `"No Direct3D hardware device found"`. On
  a match it records `cause_0_reason=no_hardware_device` in `last-error.log`.
  `scripts/acceptance/fatal_report.cpp:108-112` pins that match.
- [`check-startup.ps1`](../../scripts/check-startup.ps1) at `:225` and `:248`
  lets a Release run skip on a CI host with no adapter only if stderr is exactly
  `startup failure: No Direct3D hardware device found`.
  `scripts/sandbox/logon.ps1` holds the same literal at `:192` and compares
  against it at `:208` and `:238`. The literal is also in
  [docs/failure-reporting.md](../failure-reporting.md) (`:34-38`).

**What happens.** The classification depends on English prose that the engine
is free to reword, and nothing tells the client when it does. If the wording
changes, the reason quietly disappears from the failure record and the
missing-adapter skip becomes a CI failure. Switching backend has the same
effect. A `d3d12` build refuses with `No Direct3D 12 hardware device found`,
and a Vulkan build refuses with one of two other messages, so none of them is
recognised. Release has no WARP fallback by design (T6), so this refusal is the
failure a player without a usable GPU will actually see. That makes it the one
a client most needs to recognise.

**The ask.** Let a consumer recognise "no usable graphics device", whatever
the backend, without reading the message text. For example, each backend's
device-creation refusal could throw a distinct exception type or carry a
`std::error_code` in an engine category. The `what()` text can stay as it is.
The spelling, and whether a missing audio device shares the same type, are
engine decisions.

**What an answer lets this repository delete.** The `strcmp` at
`game/fatal_report.cpp:117` goes, and a type or code test replaces it. The
exact-text checks at `scripts/check-startup.ps1:225` and `:248` and
`scripts/sandbox/logon.ps1:208` and `:238` also go. In their place the scripts
would read the `cause_0_reason` that `last-error.log` already records, which
neither script checks today. The acceptance check would test the reason instead
of the literal. This is a local draft, not a filed issue.

---

## 31. `JsonValue` cannot report keys nobody read, so a misspelled optional key is silent - **draft with optional-read exhibit**

**Answered on untagged master (checked 2026-10-30 at `47be5a3`, 214 commits
past the pin).** `e166dcc` (2026-10-29, *Reject unknown manifest and
sprite-sheet keys*) adds `std::vector<std::string> JsonValue::keys() const` to
`engine/assets/json.h`. It copies an object's member names in stored order,
duplicates included, and throws unless the node is an object. This is engine
story `PI-20`, split out of
[report 12](#12-json-is-read-only-path-only-and-opaque--split-3af746b) on
2026-10-22. The engine's `docs/product/BACKLOG.md` now ticks it, and
`docs/2026-10-29-strict-asset-keys.md` plus `CHANGES.md` (Unreleased) record
it. The engine's own refusal, `asset_details::reject_unknown_keys` in
`engine/assets/json_object_keys.h`, stays private to the manifest and
sprite-sheet loaders. The backlog says plainly that "game-owned JSON remain
unrestricted". Which fields a game file accepts is the game's to say. Report
12's split already made that decision. `e166dcc` is not in `v0.2.0`
(`1e60fb2`), so the pin does not have `keys()`. A `gh` issue search of
`matt439/Labrador` for this found nothing. **No filing is needed.** At the
first minor tag that carries `keys()`, spend it as described below, then
close this report. The text below is the draft as prepared.

Found by the [2026-10-30 architecture review](2026-10-30-architecture-review.md#engine-reports-owed).
The exhibit is cited at HEAD `f5303302`.

### Exhibit

`GameTuning` reads every optional field of `tuning.json` with `has()` and
then a typed accessor, because that is all the pin's `JsonValue` offers. The
reads are at:

- `game/tuning.cpp:109`: `frame_time`
- `:455`: `paint_underfoot`
- `:471`: `enemy_blocks_health_regen`
- `:478` and `:756`: `paint_underfoot` again, for validation
- `:585`: `loop_sounds`
- `:648`: `add_player_velocity`
- `:663`: the optional feedback sounds `stop`, `ready`, `lift`, `settle` and `low`
- `:702`: `diffusion`

The only guard is in acceptance. `check_tune_consumed`
(`scripts/acceptance/debug_console/tune_document_checks.cpp:319`) uses the
client's own span scanner, `JsonSpanDocument`, to replace each number and
boolean leaf of the shipped `tuning.json` and `presentation.json` with a
string, one leaf at a time. It reparses each candidate and requires the
loader to throw and name the key. It skips string leaves and any path with a
`_`-prefixed segment, so a misspelt key whose leaves are all strings passes.
The check is compiled only under `COLOURWARS_DEV_TOOLS`
(`tune_document_checks.cpp:4`), so it runs only in Debug. It runs there inside
the `debug_console` case, which is declared `SlowInDebug`
(`scripts/acceptance/cases.psd1:74-78`). It checks the shipped file at test
time, not the file the game loads at startup, and it is quadratic in the
file's size.

### Reproduction

This synthetic example uses the pin's `JsonValue`:

```cpp
const auto doc = labrador::parse_json(
    R"({"player":{"paint_undrfoot":{"enabled":true}}})", "tuning.json");
const labrador::JsonValue player = doc.root().object("player");
const bool on = player.has("paint_underfoot");   // false: misspelt key ignored
```

Actual: the loader takes the default (here the effect is off), and nothing
reports the key it never read. Expected: the loader can refuse with
`'tuning.json': player has unknown key 'paint_undrfoot'`. With the pin, it
cannot list `player`'s members to find this out.

### The ask

A way to list an object's member names, so a client loader can refuse names
outside its own field list. Master's `keys()` is exactly that. This report does
not ask for two related things: making the engine's private
`reject_unknown_keys` public, and any record of which members were read. Either
would need an exhibit of its own, made after the client has spent `keys()` and
has its own copy of that helper to show.

### What an answer lets this repository delete

After the bump, each `tuning.json` object reader can check its own members
against its field list with `keys()`, at load time and in every build. The
optional `has()` reads stay, because leaving a field out is still allowed; a
misspelling becomes a load failure. `check_tune_consumed` could then shrink to
the part `keys()` does not cover: proving that each listed field really
changes the loaded value. Whether that part is worth its Debug cost is the
bump's decision, as report 12 already said of `PI-20`. `JsonSpanDocument` and
`TuneDocument` stay, because report 12's decline of spans still stands.

Filing would need separate author authorization and a fresh upstream recheck.
Because master already answers this report, no filing is planned.

---

## 32. Word wrap by measured width is client code - **draft with text-wrap exhibit**

**Answered on untagged master (checked 2026-10-30 at `47be5a3`).**
`80024e5` (2026-10-29, *Wrap translated text with the font's measured glyph
layout*) closes engine story OCA-77. It adds `Font::wrap(std::wstring_view
text, float width)` (`engine/render/font.h:149` on master) and the handle
query `RenderResources::wrap_text(FontHandle, std::wstring_view, float)`
(`engine/render/render_resources.h:260`). Each returns an owned copy of the
text. Selected U+0020 spaces become LF, and the lines are measured with the
same glyph walk `measure` uses. Existing line feeds are kept. Words are never
split, and a word wider than the width on its own is let through. The width is
in unscaled font texels and must be finite and positive. Engine `CHANGES.md`
does not list the change. `80024e5` is not in `v0.2.0` and no tag carries it,
so the pin does not have it. A read-only search of the engine's issues for
"wrap" found nothing. The same search over pull requests finds only #44, the
weighing that split out OCA-77. No filing is needed. At the first minor tag
that carries it, delete as below and close this report. The text below is the
draft as prepared.

**The premise this corrects.** OCA-77 was weighed on 2026-10-24. It is still
recorded on master in `docs/product/STORIES.md` (OCA-77, Notes) as:

> No caller in either client wraps by width: ColourWars' debug console cuts at
> a column count derived from a monospaced advance, deliberately.

`docs/product/BACKLOG.md`'s closed OCA-77 row says the same: "no caller in
either client wraps by width yet: the one wrap there is a debug console
cutting at a column count on purpose". That was not true when it was written.
This repository had wrapped by measured width since `e1e17178` (2026-10-17).
The story then gave OCA-42's translated strings as the first caller. The
answer doesn't depend on which caller came first, so this asks for nothing.
The engine may want to correct the note the next time it touches the story.

### Exhibit

The exhibit is at HEAD `25af4d16`.

- [`game/text_wrap.h`](../../game/text_wrap.h):28-32 declares
  `text_wrap::wrap(render, font, text, max_width, scale)`. Its body in
  `game/text_wrap.cpp`:39-99 does a greedy wrap. For each authored line it
  builds the next candidate line and measures it with
  `RenderResources::measure_text` (`:11-15`). It breaks before the next word
  when the candidate's measured width times `scale` would be greater than
  `max_width`.
- [`WrappedTextDropShadow`](../../game/states/wrapped_text_drop_shadow.h) is the
  caller. It wraps in its constructor (`wrapped_text_drop_shadow.cpp:13-18`,
  because the base constructor measures), in `set_text` (`:44-49`) and in
  `set_scale` (`:51-56`). It is built for the weapon description at
  `game/states/main_menu_weapon_select.cpp:363`.
- `text_wrap::check_fits` (`text_wrap.h:37-42`, `text_wrap.cpp:101-130`)
  refuses at load time any word wider than the width. It is called from
  `game/content_validation.cpp:172`. The `text_wrap` acceptance case
  (`scripts/acceptance/text_wrap.cpp`) checks both against the engine's own
  measurement.

### What happens

At the pin, a client whose text must fit a panel has `measure_text` and
nothing else. So it carries its own break loop, re-measuring a growing prefix
once per word. It also carries its own rules for authored line feeds, spaces at
a break and words that do not fit. Two clients would carry two loops that
disagree at the edges.

### The ask

That the font that measures a string can also break it to a width, measured by
the same walk. Master's `Font::wrap`/`RenderResources::wrap_text` is that
answer. No further ask.

### What an answer lets this repository delete

- `text_wrap::wrap` (`text_wrap.h:23-32`, `text_wrap.cpp:78-99`) and its helper
  `wrap_line` (`text_wrap.cpp:39-75`). `WrappedTextDropShadow` would call
  `render_resources_->wrap_text(font, text, max_width / scale)`, because the
  engine's width is unscaled and the client's is the drawn width.
- The wrap half of the `text_wrap` acceptance case, re-pointed at the engine
  call.

These stay:

- `check_fits` and its load-time diagnostic, with the helpers it uses
  (`measured_width`, `require_width`, `narrow`). The engine lets an overlong
  word through, and refusing it with the content key's name is client policy.
- `WrappedTextDropShadow` itself, which keeps the authored text.

**Edge differences to requalify, not to report.** At a break, the client drops
the whole run of spaces. The engine replaces one selected space and documents
that repeated or leading spaces "can become empty lines at narrow widths".
Today's weapon descriptions use one space between words, so the output should
match. The painter's weapon-paper check and `text_wrap` confirm that when the
pin moves. Filing would need separate author authorization, but none is needed
while master answers it.

---

## 33. `on_contact` hands the other party over `const` - **draft with const_cast exhibit**

**Drafted 2026-10-30. Upstream checked the same day at master `47be5a3` (2026-10-30): not raised or planned upstream.** On master, `CollisionObject::on_contact` (`engine/collision/collision_object.h:49-50`) is still `on_contact(const CollisionObject& other, const Vector2F& normal, float penetration)`. The pin `1e60fb2` (`v0.2.0`) has the same parameter. Since the pin, only `e0536cc` (constant spelling) and `afac4a9` (2026-10-27, which moved the math library into `namespace labrador`, so `mattmath::Vector2F` became `Vector2F`) have touched `collision_object.h`, `contacts.h` or `contacts.cpp`. Neither changes `other`. Master's `CHANGES.md`, `docs/product/BACKLOG.md` and `docs/product/STORIES.md` mention neither `on_contact` nor a mutable other party. Master's other `on_contact` mentions don't plan such a change either: `docs/api-teachability.md:143`, `docs/product/JOURNEYS.md:91`, `docs/review/all-findings.md:3750`, the rtcd notes on pair-once and NaN normals, and the website guides (`guides/your-collision.mdx:88,187,435`, `concepts/game.mdx`, `concepts/tick.mdx`). A `gh` search of `matt439/Labrador` issues for `on_contact` and `const_cast` returned nothing. This is a local draft, not a filed issue. Line numbers in this repository are at ColourWars `f5303302`.

### Exhibit

The one `const_cast` in `game/` is in [`Projectile::answer_paint_contact`](../../game/objects/projectile_contacts.cpp). It is declared at `game/objects/projectile.h:94` and defined at `game/objects/projectile_contacts.cpp:540-544`, with the cast at `:543`:

```cpp
void Projectile::answer_paint_contact(const Projectile& toucher,
    bool painted, bool captured)
{
    const_cast<Projectile&>(toucher).note_paint_contact(painted, captured);
}
```

Before `HYG#49` (`de2b6195`, 2026-10-30), the same answer reached `mutable` flags through a `const` `note_paint_contact`. That commit made the game's rule "const means draw-safe" and removed every `mutable` member. That left this cast as the one place where the game breaks the rule on purpose.

### What happens

A paint shot hits a paintable surface. Only the surface knows the outcome: whether it painted any tiles, and whether it took any from the other team. `StructurePaintable::on_projectile_contact` (`game/objects/structure_paintable.cpp:140`, answer at `:243`) and `FixedSlabPaintable::on_projectile_contact` (`game/objects/fixed_slab_paintable.cpp:191`, answer at `:210`) work that out while they paint. The projectile needs the outcome for two things. It decides its impact sound in `Projectile::flush_paint_audio` (`projectile_contacts.cpp:565`), which runs only once the contact batch is complete (`projectile.h:85-86`). It also decides whether to draw a blocked-shot mark in `take_blocked_contact` (`:556`).

The game's contacts reach the engine through `Level` -> `scene_->resolve()` (`game/objects/level.cpp:453`) -> `dispatch_contacts`. That calls `on_contact` once on each party (`engine/collision/contacts.cpp:112-115` on master, `:113-116` at the pin). Each side receives the other as `const CollisionObject&`. The projectile's own half, `Projectile::on_structure_contact` (`projectile_contacts.cpp:518-528`), can see that it hit geometry, but not what the surface painted. The surface keeps no per-shot result, and only its own half sees the outcome while it paints. So the answer has to cross to the projectile inside that call, and the signature only allows that through a cast.

The write is defined, because every projectile is owned non-const by its scene or by a probe. The caller holds both parties mutably too. `Contact::a`/`b` are non-const `CollisionObject*` (`engine/collision/contacts.h:21-22`), and the game's own sweeps call both halves on non-const objects (`projectile_contacts.cpp:438-439`, `:514-515`). The pair is mutable up to the call and becomes `const` only at the virtual's signature.

**Reproduction.** Take any two `CollisionObject`s where the response to a contact changes both parties and only one party can compute it, for example a surface that absorbs a shot and tells the shot how much it absorbed. The responding party is handed `const CollisionObject& other`. The contact gives it no defined way to change `other`. What is left is a `const_cast`, `mutable` state on `other`'s class, or a side channel outside the contact (the answer queued by identity and settled by the game after `resolve`).

### The ask

A defined way for a contact response to change the other party, through the contact itself. It might be a non-const `other` in `on_contact`, or a separate hook for responses that reach across the pair. Whether that is a signature change, an overload or a contact object, and how it fits the "each is told exactly once" rule (`collision_object.h:39-41`), are engine decisions. The ask is only that a game can answer a contact on both sides without a `const_cast`, `mutable` state, or a side channel of its own.

### What an answer lets this repository delete

- `Projectile::answer_paint_contact` and its `const_cast`. The two surface callers would call `note_paint_contact` on the other party directly.
- The paragraph at `projectile.h:88-93`.
- The exception named in `Collider::on_contact`'s comment (`game/objects/collider.cpp:24-26`).

After that, the game has no `const_cast` and no `mutable`.

Filing requires separate author authorization. This draft does not file, comment on or open anything upstream.

---

# Closed

Twenty-three reports, all answered. Sixteen were spent here in code. Seven
spent nothing in code: 1 was fixed before it was filed; 9 and 10 were answered
as document only, and those two changed only comments and
docs; 15 was answered as document only, 17 changed only a manifest comment, 19
needs more evidence and 21 was declined — an answer that leaves the workaround where it is still
closes the report. Kept as outcomes rather than as
reports: what was asked, what the engine answered, what this repository did
with it. The original bodies are in git — see
[What the audit changed](#what-the-audit-changed) for reports 1–5, `a7da897`
for 7 and 8, the parent of `487b8d2` for 13 and 21, `d9f1656` for 11, 16,
17 and 19, the parent of `d1c6063` for 9, 10 and 14, the parent of
`83544c2` for 18, 20 and 22, the parents of `d647cf8`, `b4f2201` and
`1553ba5` for 6, 12 and 15, and the parent of the commit that moved the pin
to `1e60fb2` for 24.

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

**Spent** in `b6b9561`. All three exhibits deleted:
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

**Spent** in `c7944a9`. Mode Select registers its three unbuilt modes and
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

**Spent** in `0028ec8`. `PromptBar` derives instead of composing, losing four
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

**Spent** in `9dc5e2b`, and `drawable_text` stayed, as predicted. Question
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

## 6. ~~No audio bus, master gain, listener or voice cap~~ — accepted, narrowed, `19cf650`

Asked, in its fileable slice, for shared Music/SFX attenuation: the saved
gains in `play_settings.h` were multiplied into every emitter by hand, and the
resume paths re-applied the user gain to each retained effect, with authored
levels, group assignment, fades and ducking left to the client. **Accepted,
narrowed**, in "Mix named sound group volumes into every level a bank sends".
`engine/audio/sound_group.h` adds `SoundGroupHandle`, which belongs to the
`AudioResources` that issued it. `AudioResources` adds `add_sound_group`
(starts at 1, throws on a duplicate), `resolve_sound_group`, and a
non-const `set_group_volume`, clamped to [0,1], beside `group_volume`.
`SoundBank` adds the load-time `set_wave_group` and `set_effect_group`; a
second assignment moves a sound, and a silent bank checks the call and
discards it. The level sent is the clamped local level times the group's
volume, with an ungrouped sound at 1. A one-shot already playing keeps the
level it launched with. A group change pushes one `set_voice_volume` to each
playing or paused effect in the group, and a stopped one takes it at its next
`play_effect`. **Declined:** a master bus, fades, ducking, a listener and a
voice cap. The engine recorded the listener and the voice budget as unfiled
and not covered by the decision.

**Spent** in this commit. `game/sound_groups.{h,cpp}` adds `music` and `sfx`
at load, in `wWinMain` after `validate_game_content`, and again for a
reloaded candidate on Debug `reload`. The engine cannot list a bank's waves,
so the assignment walks the game's own references. `GameTuning` and
`Presentation` now list their sounds once (`sound_references`,
`match_sound_references`, `SoundReference` in `game/sound_reference.h`), and
`validate_resources` and `validate_match_sounds` check that same list. A sound
cannot be checked and still be left out of its group. Stage music, end stings
and the menu and results music are music, everything else is sfx, and the
Controls volume preview stays in neither. The assignment refuses, naming both
keys, a sound named under two different groups. `sound_groups::apply` pushes
`PlaySettings` into both groups at startup and on Controls Apply, through
`GameData`'s non-const `AudioResources`, whether or not the save succeeds.
Apply's `update_music(0)` re-push is deleted. Every `* sfx_gain()` and
`* music_gain()` at the emitters is gone: `Level`, its opening and reveal,
`MovementAudio`, `PaintAudio`, `Player` (`redraw_gain` became
`redraw_volume`), `Weapon` and the bucket and sniper, `ConnectionAudio`,
`ResumeAudio`, `ScreenMusic::apply_gain`, and `MenuPage`, whose `play_wave`
and `play_effect` wrappers are now the base class's through a `using`.
`MovementAudio`, `PaintAudio`, `ConnectionAudio` and `ResumeAudio` read
`PlaySettings` for nothing else and no longer take it. `Level::resume`,
`Player::resume_sounds` and `Weapon::resume_sounds` lost their seventeen
re-pushes before `resume_effect`. Acceptance's `story_audio` world assigns
the groups the same way and changes a volume only through
`set_sfx_volume`/`set_music_volume`. A new `check_sound_groups` plays every
listed sound with one group muted and then the other, and fails if any sound
is out of its group. The silent-bank fallback takes the same assignment. The
four hardware probes (`painter_visual`, `paint_fx_visual`,
`movement_feedback_visual`, `frame_pacing`) assign and apply too, so their
zero volumes still silence them. The draft and its standalone exhibit are
deleted.

**Kept:** `music_gain()`/`sfx_gain()`, which now feed `apply`. The volume
preview, which multiplies the *pending* `selection_.sfx_gain()` into its
ungrouped wave. `ScreenMusic::finished()`, which reads `music_gain() == 0` as
policy. The music fade and pause duck in `Level` and `ScreenMusic`, which
compose with the group volume through `set_effect_volume`. The redraw pen's
per-tick `set_effect_volume`. The save format, which still stores two 0-100
integers. `Level::resume_music` still pushes fade times duck before resuming.

**The prediction was wrong about where the policy would end up.** The draft
said group assignment would "remain client policy" as if it were a handful of
calls. Because the engine cannot list a bank's waves, the client has to walk
its own authored references, and that made it worth giving validation and
assignment one list. Membership is fixed per wave or effect at load, not per
call, so the one emitter that bypasses the applied gain, the volume preview,
works only because `ui.volume_preview` is its own wave. Had it shared a wave
with a menu sound, this answer would not have fitted, and the assignment now
refuses that case. The heading's master gain was declined, and nothing is
lost, since the game has no master slider. The draft's cost estimate was
right. Both halves it named, the per-emitter multiplication and the resume
re-push, are gone.

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

## 12. ~~JSON is read-only, path-only and opaque~~ — split, `3af746b`

Asked, in its fileable slice, for an in-memory parse into the same owning
`JsonDocument` with a caller-supplied diagnostic name, so the Debug live-tuning
client could stop writing each candidate to a `%TEMP%` file to reach the
checked reader and rewriting that scratch path out of its errors. Object-key
enumeration and the preservation of raw source spans stayed unfiled asks of
the same report. **Split three ways.** The parse is **accepted**, in "Parse
JSON from memory through the file reader's own parse":
`parse_json(std::string_view text, std::string source_name)` in
`engine/assets/json.h`. It throws `invalid_argument` for an empty name and
`runtime_error` "parse_json - '<name>' is not valid JSON: ..." on bad text.
The document copies the text rather than borrowing it, and `source_path()` is
the name it was given. `read_json_file` is now file IO plus the same private
parse, with its messages unchanged byte for byte. Enumeration is **accepted as
its own story**, PI-20, "A misspelt key in a manifest or sprite sheet is
refused" (engine BACKLOG), argued from the engine's loaders, which read a
misspelt optional key as its default. This client's consumed-key check is a
second caller, not the reason. PI-20 is not built, and `JsonValue` has no key
accessor yet. Spans and source-preserving splicing are **declined**, citing
ARCHITECTURE's "`assets` only reads", PHILOSOPHY's no hot-reload and no
editor, T9 and T1. Preserving and splicing source text is a debug tool's
policy, so it stays in the client.

**Spent** in this commit. `GameTuning` and `Presentation` gain a constructor
from a `JsonDocument`, and the path constructors delegate to it, so startup
diagnostics are unchanged. `TuneTarget::validate` takes the candidate text
and parses it with `parse_json` under the logical name (`tuning.json`,
`presentation.json`). `tune_validate_and_apply` lost its `%TEMP%` bridge: the
scratch path, the write and removal, the `replace_all` that rewrote the
scratch path into the logical name in messages, and the comment citing this
report. Rejections read as before: "tune: rejected: tuning.json: ...". The
one-time sweep of empty `ColourWars-tune-<pid>` folders left by older builds
went with it (`stale_tune_directories`, `tune_swept_`), along with its
session check, the planted folders in `check-debug-console.ps1` phase 4, and
`debug-console.md`'s mention. Acceptance's `check_tune_consumed` was a second
scratch-file bridge, one temp candidate per leaf, and now parses each
replaced text in memory. `tune_checks`' two guards stay as regression guards:
a rejection names the content file, not a temporary, and no candidate file is
left in `%TEMP%`. `json_spans.h` and `debug-console.md` say what is now the
engine's and what is the client's by decision. The draft and its CMake
exhibit are deleted.

**Kept:** `JsonSpanDocument` and `TuneDocument`, the span scanner and the
source-preserving splice, save and conflict policy, which stay client-owned
by the decline. The consumed-key check also needs the client's own knowledge
of which fields it accepts, so PI-20 would not replace it. No TODO row
tracks PI-20. A pin bump that brings it can decide whether
`check_tune_consumed` or `JsonSpanDocument` has anything to gain.

**The prediction was wrong about the open halves and about the size of the
bridge.** The ledger called enumeration and spans "separate, unresolved
portions" that would each need an exhibit of their own before filing. The
engine decided both ahead of filing and without them: it accepted
enumeration on its own grounds and declined spans outright. The draft also
understated the workaround in two ways. Acceptance's `check_tune_consumed`
wrote a second scratch file per leaf, which the ledger never listed. And an
applied candidate was copy-assigned over the live object together with its
deleted scratch path in `source_path_`, so a later diagnostic from the live
tuning or presentation would name that file. `replace_all` only ever
rewrote the candidate's own messages. The logical name fixes that as a side
effect. One wording changes: a syntax error in a candidate now reads
"parse_json - 'tuning.json' ..." where it used to read "read_json_file -
'tuning.json' ...". Nothing pins the old prefix. The ledger was right that a
memory parse alone would not delete `JsonSpanDocument` or `TuneDocument`.

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
unqualified call the pin bump `6a5d83c` had to qualify), and `parse` also takes
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

**Spent in two steps:** documentation in `d1c6063`, then completed CPU-phase
consumption in `364f8cc` (2026-10-24, `DBG#5`). The overlay and schema 2 capture
pair each pending match sample with the exact completed Application frame;
final publication is awaited before export, including target-one captures.
The analyzer accepts both schemas and rejects incomplete engine pairing.
[Frame-pacing qualification](../polish-frame-pacing.md#completed-engine-phase-pairing-2026-10-24)
records the build, CPU, analyzer and hidden native evidence. GPU, compositor
and scan-out stay unmeasured; engine phases are CPU wall time, not CPU busy
time or a separate Present measurement. **Kept, and not replaced by the
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

## 15. ~~Runtime packaging and content staging are repeated in the client~~ — document only, `48fb236`

Asked, in its fileable slice, for an optional development-staging convenience:
a consuming target, a source content root and explicit required and optional
file lists, to replace the generic always-run copy and removal mechanics in
`colourwars_content`/`stage_runtime_assets.cmake`. A documented recipe was
named as a possible answer, and the runtime-install proposal (engine runtime
DLLs, CRT, refusing a debug CRT) was kept separate. **Document only**, in
"Say the template's content mirror never removes a file". No CMake staging
helper was added. The template's always-run copy target is the documented
recipe, and the staging target is part of what a game owns (ARCHITECTURE *A
game project*, T1). `samples/minimal/CMakeLists.txt` now says that
`copy_directory_if_different` never removes a file. A stale mirror hides a
missing required file, and keeps an optional bank from being replaced by the
silent one. The assets guide gained "Optional content and stale copies", a
`cmake -P` list-and-remove recipe that copies the listed files, fails on a
missing required one and removes a missing optional one. The template's
README notes the change. The runtime-install half is PI-11, which the engine
says "stays undecided". Its reopen condition is a second move of the
content-root rule or PI-11 being accepted, and not a count of callers.

**Spent: nothing.** The recipe is a strict subset of this game's stager. It
copies and removes only what it lists. It has no path-safety checks, no
refusal of linked entries, no audio or font validation and no pruning of
unlisted stale files inside the owned content roots. Adopting it would weaken
`stage_runtime_assets.cmake`, which the engine now records as legitimately
the client's. `colourwars_content`, `runtime_assets.cmake`,
`release_assets.cmake`, `release_package.cmake` and
`cmake/check_runtime_staging.cmake` all stay. The last is this game's own
regression test of the stager, not part of the exhibit, and
`runtime-staging-<id>` there is only a scratch-folder name. The draft and its
synthetic `scripts/engine-reports/runtime-staging/` fixture are deleted. The
runtime-install residue moves to
[What is deliberately not here](#what-is-deliberately-not-here) and
`ENG#PARKED`. Gap-audit F8 stays as written. Its staging half is answered as
document only, and its install/export half is PI-11.

**The prediction was wrong about what an answer would delete.** The bounded
ask said an engine convenience or recipe "would replace generic always-run
copy/removal mechanics" here. It replaces nothing. The engine's own decision
says the client "could delete nothing directly". The hedge that "a documented
recipe may be sufficient" was right as a description of the answer, and
wrong about what it would let this repository delete.

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
and are deliberately left untracked: they change nothing, so no row owns them.

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

**Initially kept as `POL#29`:** the first spend of report 20 retained the
sequential overview/monitor walk in `miniature_view.cpp`, its structure
`dynamic_cast`, `Level::draw_miniature_world`'s view offset, the `scenery_` mirror
and StagePreview's render-time dispatch. The camera accessors answered the
engine mechanism; selecting miniature art by path remained the client's work.

**Completed follow-up (2026-10-24):** ordinary
`Structure::draw` selects the material painter from a path-set miniature flag.
Level prepares the flag at construction, update/zero-dt transitions and the
existing pre-draw layout refresh before Scene fan-out. Overview/monitor views
then call `Scene::draw`, retiring the production manual traversal and borrowed
scenery mirror. StagePreview marks new structures during selection and keeps
its own supplied-list/pane traversal; it no longer dispatches structure drawing
by a render-time cast. A small camera scale alone never selects miniature art.
The unchanged explicit `draw_miniature` methods remain as the independent native
probe oracle, rather than being claimed as deleted production APIs.
[polish-terrain.md](../polish-terrain.md#small-view-terrain-readability-2026-10-18)
owns the implementation and its completed order/cull/pixel qualification. The
miniature checks include all five stages and an actual low-scale active Crossfeed
counterexample; landmark/parallax checks retain four authored prop stages.
Both game/editor builds, four terrain/paint CPU case runs and all six native
mode/preset combinations pass. Original v3 Miniature/Landmark binaries and
fresh v4 Parallax binaries retain separate provenance after a Parallax-only C2
fixture correction; production inputs are identical. The reconciled receipts
and retained initial fixture failures are under
`out/validation/miniature-scene-20261024/`. Human acceptance remains open.

**The prediction was wrong about what a reader would buy.** The draft said view
context "could enable this client's normal world-slot projection" and left the
deletion to a separate check, without seeing that a camera alone cannot delete
the borrowed walk: it needs a behaviour decision the draft never named, the
path-set mode. The draft also argued a convenience, where the engine found a
stated rule contradicted: `scene.h` forbids hand-copied per-view walks, and
`game_object.h` described a camera a `draw()` could not get. And it did not
foresee the cull contract. The miniature strokes are clipped to their
structure's own extent, so by this client's reading they already meet it; the
`POL#29` pixel oracles are where that gets checked.

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
This is the first report here whose exhibit was declined outright. It sits
outside the six-of-seven strike rate [above](#why-the-exhibits-matter-more-than-the-asks),
which counts filed reports; this one was answered before it was filed.

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
it resolved separately. The asks' raw-handle shape was declined and their
substance answered with pairs: the strip and frame setters and the handle
constructors, as `SheetStrip`/`SheetFrame` constructors and protected setters.
The frame-only `set_frame` and the `UiTexture` forms were declined outright, and
so was the `std::less<>` hygiene the draft said "rides with the main ask". The draft put the
splash handles in "PaintTile's shared paint record", which is `PaintTuning`, read
before any sheet exists; they needed a record of their own beside it. And it did
not foresee the atomic name setters.

---

## 24. ~~A Debug `AudioDevice` breaks into the debugger on a machine with no audio endpoint~~ — `814f2ee`

Found 2026-10-25, when the Debug CI job's movement feedback probe and then
its startup check died with `STATUS_BREAKPOINT` on a GitHub runner. A Debug
`AudioDevice` passed DirectXTK's `AudioEngine_Debug`, which sets XAudio2's
break mask to errors. With no audio endpoint, XAudio2 hit `int 3` twice while
DirectXTK fell back to silence, and with no debugger attached that ended the
process. The ask was a Debug device that does not break there: drop the flag,
empty the break mask, or an opt-out. **Answered the same day, unfiled**, in
"Keep a debug build running where there is no audio endpoint". The engine's
own template startup check had found the same crash. The debug layer is now
asked for only when `IsDebuggerPresent()`, so a Debug build without a
debugger goes silent exactly as Release does. A parallel engine pull request
from this repository, which kept the trace and emptied the break mask, was
closed as superseded.

**Spent:** the XAudio2 branch of `step_over_debug_break` in
[movement_feedback_visual.cpp](../../scripts/acceptance/movement_feedback_visual.cpp)
and its stdout count were deleted. The handler stays, because it makes any
other debug-layer break report its stack instead of ending the probe
silently. The Debug startup check now passes on a runner with no change on
this side.

# What is deliberately not here

Re-checked 2026-08-15 against `6b5c0bd`; citations re-pointed 2026-09-17 at
`984cf13`, where the comment sweeps moved every one of them by a few lines and
changed none of the facts. Re-read 2026-10-23 at `19cf650`: the bump answered
none of them, and added the runtime-install and listener/voice entries below
as residue of reports 15 and 6. `window_size_changed` is still ignored
(`application.cpp:468,495`), there is still no desktop-size query, and `json.h`
still only reads.

- **Who owns a resolution-independent world height.** This entry used to say
  `CameraTools` never sets a scale, so a 4K player saw nine times the world a
  720p player did. That stopped being true here, not in the engine: `b69d085`
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
  (`application.cpp:468,495`, both `std::ignore =`), and **nothing reports the
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

- **`MouseButton` has no names, as `Key` had none before report 13.** Found
  2026-10-28 by the [keyboard actions slice](../feature-roadmap.md#keyboard-actions-slice-gap42),
  against the pin `1e60fb2`: Shoot may take a mouse button, and the save spells
  the five buttons itself (`mouse_left`, `mouse_right`, `mouse_middle`,
  `mouse_x1`, `mouse_x2`) in `game/input_bindings.cpp` beside the keys, which
  use `labrador::key_name`. The exhibit is that five-row table. It is too small
  to file alone: the ask is `mouse_button_name`/`mouse_button_from_name` next to
  `MouseButton`, with the same file-format promise, and it rides with the next
  input report. The game's tokens are prefixed so that a bare engine spelling
  (`left`, `x1`) cannot collide with a key name in the same field; an answer
  would replace the table's name column and keep the prefix as the client's.
  Parked as `ENG#PARKED`.

- **Runtime install facts: the engine's runtime DLLs, the redistributable CRT
  and refusing a debug CRT.** Report 15 kept this half separate from its
  staging slice, and the engine left it to PI-11, which "stays undecided". The
  reopen condition is PI-11 being accepted or the content-root rule moving a
  second time. Nothing here is a workaround waiting on it:
  `release_package.cmake` follows `TARGET_RUNTIME_DLLS:ColourWars`, adds the
  MSVC runtime through `InstallRequiredSystemLibraries`, refuses
  configurations outside Release x64 D3D11/XAudio2, and CI inspects the ZIP.
  It works, so there is no exhibit to delete. If PI-11 is accepted, read it
  against that file. A clean-PC run is still `ROAD#1`. Parked as `ENG#PARKED`.

- **A listener model and a voice budget.** Report 6 carried both as broader
  proposals beside its attenuation slice, never with an exhibit, and the engine
  answered the slice alone: its decision records them as unfiled and says it
  does not cover them. Nothing here works around either. Positional audio has
  no caller in a split-screen game whose views share one mix, and no voice
  limit has been hit. Any limit this game wants stays client-side: the
  per-owner paint budgets in `PaintAudio` and the movement rate limits are
  that already. If a voice cap is ever needed, the exhibit is the code that
  had to count voices.

- **Losing the audio device is not recovered** (added 2026-10-24).
  `AudioDevice::update` discards `AudioEngine::Update()`'s false return on
  purpose (`engine/audio/xaudio2/audio_device.cpp`, beside the platform), so
  unplugging headphones or switching the default output silences the game
  until it is relaunched. DirectXTK's documented reply is `IsCriticalError()`
  then `Reset()`, and only the engine can make it. Nothing here works around
  it, so there is no exhibit yet. The `ROAD#3` listening session observes the
  case. A report follows if that session shows it in ordinary play.
  **Rechecked 2026-10-29 at master `b96bad6`:** engine `2b3b97b` (2026-10-26,
  *Retry unavailable XAudio2 output without restarting voices*, not in
  `v0.2.0`) retries an unavailable default output once a second while active.
  Banks and persistent voice handles stay valid, but one-shots are dropped and
  persistent voices stop, and the game has to play them again. So no report
  is owed. Both music owners now replay a looping voice stopped from outside
  ([`ROAD#36`](../audio-inventory.md#looping-music-after-device-recovery-road36),
  done 2026-10-29 ahead of the bump), so the bump leaves nothing here for music.
  Other persistent loops are outside that scope.

- **The display-idle owner is client-side** (added 2026-10-24).
  `ROAD#6` now supplies the [client exhibit](../../game/display_awake.cpp):
  one `GameLevel` owner attempts `ES_CONTINUOUS | ES_DISPLAY_REQUIRED` on
  construction and clears with `ES_CONTINUOUS` on destruction only after
  successful acquisition. Pause/focus/round changes retain that same owner;
  clearing is best effort. It adds no system/away-mode request, screen-saver
  policy or user-Sleep override. Physical timer observation remains `ROAD#2`.
  The window shell is still the more natural owner. This stays parked and
  unfiled, small enough to ride with another report.

- **XInput is the only pad reader.** `engine/input/gamepad_reader.h` is one
  function wide, so a GameInput backend (DualSense, DualShock, Switch Pro
  without Steam Input) would be an engine addition behind an existing seam. No
  exhibit: nothing here works around it. `ROAD#2` brings one such pad to
  observe the limit.

- **The write side of `json.h`.** Still absent, still correct (the engine's
  OCA-53 plans a guide, not an API, and declines a save facility under T1):
  `ARCHITECTURE.md` names the read-only asymmetry and settles it, and T12
  declines automatic serialisation. What survives is narrower — the ~50-line
  atomic-write protocol in `game/save.cpp` is entirely game-agnostic and no
  second client should re-derive it — and is a small ask best made alongside
  something else.

- **`Scene::contacts()` having no reader** (`scene.h:204`) and
  **`labrador::Label` having no client** (`engine/render/label.h:35` — it
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
  but that has been stale since `9604211` added `Projectile::resolve_sweep`.
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
`a694f78` and one `git show` away.

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
world height since `b69d085`; the entry is now the narrower ownership question.

**Validation:** `x64-debug` and `x64-release` reconfigured and built under
`/W4 /WX`; consumer acceptance passed on both, `check-startup` passed on both,
and the movement-feedback, workshop-playable and workshop-style probes passed
with their windows hidden through `visible`. No pad, touchscreen or audio
playback was exercised.

# What the 2026-10-22 bump changed

Moved from `0c15f34` to upstream master at `19cf650` in `6a5d83c`: fifteen
commits, and every one of them about this repository. The first, `cacf112`,
records Labrador's decision on each of the fifteen live reports in its
`docs/review/colourwars-reports/` (a table in the README and one file per
area: window, loop, input, draw, content); the other fourteen implement or
document those decisions. **Every live report was answered**, which no bump
before had done for more than one, and none of them had been filed.

| # | Decision | Engine commit | Spent here |
|---|---|---|---|
| 6 | accepted, narrowed: named sound groups; master bus, fades, ducking, listener and voice cap declined | `19cf650` | `d647cf8` |
| 9 | document only: a root `State` is the per-frame observer; capture declined | `f1e5244` | `d1c6063`, docs only |
| 10 | document only: the redraw is the supported capture | `ac58ca1` | `d1c6063`, docs only |
| 11 | accepted, narrowed: the bare `SC_KEYMENU` is swallowed, no option | `0f3b283` | `a1238ee` |
| 12 | split: `parse_json` accepted, key enumeration a separate story (PI-20), spans declined | `3af746b` | `b4f2201` |
| 13 | accepted: `key_name` and `key_from_name` | `2851ad9` | `487b8d2` |
| 14 | accepted, narrowed: CPU `frame_timings()`, GPU time declined | `7344610` | `d1c6063`, docs; later CPU spend below |
| 15 | document only: the template's mirror and a recipe; install is PI-11, undecided | `48fb236` | `1553ba5`, nothing in code |
| 16 | accepted, widened to entering full screen | `7db3d5c` | `a1238ee`, in part |
| 17 | accepted, narrowed: client pixels kept under per-monitor v2; floor Windows 10 1607 | `dae6c87` | `a1238ee`, a manifest comment |
| 18 | document only: `UiText::set_hidden`; zero-alpha tint warned | `30cb06a` | `83544c2` |
| 19 | needs more evidence: two first-frame captures asked for | `cacf112` | `a1238ee`, nothing in code; captures are `GAP#5.1` |
| 20 | accepted, narrowed: `DrawList::camera()` and `viewport()` | `11ec1fc` | `83544c2`, in part; later Scene integration below |
| 21 | declined: the action map stays the game's | `197aff1` | `487b8d2`, nothing in code |
| 22 | accepted, narrowed: `SheetFrame`/`SheetStrip` pairs | `bec82f0` | `83544c2` |

**Later client spends (2026-10-24):** report 14's CPU columns landed in
`364f8cc`; report 20's path-set Scene integration and native qualification are
recorded in its dated follow-up above. These are later work, not claims about
what the 2026-10-22 bump itself completed.

**One source break, and it was report 13's own answer.** `labrador::key_name`
made the unqualified `key_name(labrador::Key)` in `game/debug/debug_keys.cpp`
ambiguous through argument-dependent lookup; `6a5d83c` qualified the call to
build at the new pin, and `487b8d2` then deleted the client function the
qualification pointed at. The engine's `CHANGES.md` now says in its preamble
that a new free function taking an engine type can break a game this way.
Three silent changes reach this game and are recorded under their reports:
Windows 10 1607 as the load floor (17), F10 and a lone Alt no longer freezing a
Release build in the system menu (11), and atomic name setters on
`TextureObject` and `AnimationObject` (22).

**What that closing round left.** Spending 16 found report 23 (`cc1a0c6`), then
the one live report. Report 19 left this repository the two captures under
`GAP#5.1`; 14 left `DBG#5`'s three columns; 20 left `POL#29`; 12's PI-20 and
15's PI-11 were engine stories with no row here, and 15's install half and 6's
listener and voice cap were parked under
[What is deliberately not here](#what-is-deliberately-not-here). Every draft
in `docs/engine-reports/` and every exhibit in `scripts/engine-reports/` was
deleted, and both folders with them. The ledger's counts were reconciled after
each closing commit (`d9f1656`, `6400ffa`, `83885f4`, `7505a4a`) and once more
in that commit. The live sections above record later client follow-ups; they do
not reopen the answered engine reports.

**Validation:** this commit built `x64-debug` and passed Debug consumer
acceptance; the closing commits record their own.

# What the 2026-10-25 bump changed

Moved from `19cf650` to upstream master at `1e60fb2`, `v0.2.0`: fifteen
commits. One answered [report 24](#24-a-debug-audiodevice-breaks-into-the-debugger-on-a-machine-with-no-audio-endpoint--814f2ee).
The rest are additive, or belong to the engine's template and website.

Three silent changes reach this game, as the engine's `CHANGES.md` lists them
under `v0.2.0`:

- **A Debug build runs silent with no audio endpoint** unless a debugger is
  attached (`814f2ee`), which is report 24's answer.
- **A scene's collision objects keep the order they were added in**, through
  retirement, and so draw in that order. Projectiles retire constantly here.
  Both presets' consumer acceptance and the movement feedback
  probe pass with the new order.
- **A browser build defaults to the `gl` render backend.** That does not reach
  a Windows build.

The new names (`Camera`'s world point under a pixel, `StepTimer`'s
`step_position`, the reserved `engine:white` texture, removing any object at
the end of a tick) are not used here yet. No source break.

**Validation:** the CI run on pull request #3 at this pin, `x64-debug` and
`x64-release`: build, consumer acceptance, the movement feedback probe, the
startup check and, in Release, the package.

# Preparing the next bump (2026-10-29)

Fetched 2026-10-29: upstream master is `b96bad6`, 182 commits past the pin
`1e60fb2` (`v0.2.0`). No tag has been cut since. Engine `TODO.md` gates
`v0.3.0` only on `SITE#3`, the website snapshot acceptance, and its `SRL-07`
row waits on this client to migrate the renamed constants at its next pin
move. AGENTS.md gives a week from a minor tag to move the pin. The work below
can start before the tag, so that week is spent on validation rather than
discovery. None of it moves the pin or checks out anything in
`external/labrador`.

Master's Unreleased section in `CHANGES.md` also carries the answers to
reports 23 and 25 (see [Live](#live)) and the audio-recovery change under
[What is deliberately not here](#what-is-deliberately-not-here). Those are
spent at the bump itself, not here.

## Remove the manifest's `_comment` key (`HYG#23`)

**Done 2026-10-29.** The key is gone; README's Audio section already said
everything it did, so no wording moved. A JSON check of
`game/content/manifest.json` finds only `assets` at the root and only `kind`,
`directory`, `names` and `optional` in its four groups. At the current pin the
x64-debug consumer acceptance passed (69 cases, the pull-request exclusions)
and x64-release passed all 73 (`movement` rerun alone after the parallel
compile ran out of compiler heap). `check-startup.ps1` passed the Debug
executable with `-RequireAudio`, and a copy with `sounds/paint_exe.xwb`
removed with `-RequireNoAudio`, reporting the silent bank once on stderr.
The record as filed:

Engine `e166dcc` (2026-10-29, *Reject unknown manifest
and sprite-sheet keys*, not in `v0.2.0`) makes `read_asset_manifest` refuse
any key it does not know. The root may hold only `assets`
(`engine/assets/asset_manifest_loader.cpp:16` on master). A group may hold
only `kind`, `directory`, `names` and `optional` (`:25`).
`game/content/manifest.json:48` puts a `_comment` in the `sound_bank` group.
It is the only key outside that schema, and at the bump startup would refuse
the manifest. The six sprite sheets under `game/content/textures/` use only
keys the new sprite-sheet checks accept.

- **Scope:** delete that one key. `README.md`'s Audio section (lines 412-413)
  already says what the comment says: the bank is optional, and a missing bank
  gets a silent substitute reported once on stderr. Move any wording the
  README lacks there. `presentation.json`'s `_comment` keys are read by this
  game, not by the engine, and are out of scope.
- **First step:** remove the key and build `x64-debug`. The pin's loader
  ignores unknown keys, so the change is safe now.
- **Acceptance:** a JSON check finds no group key outside the four allowed
  keys and no root key but `assets`. Startup and consumer acceptance pass at
  the current pin. The missing-bank fallback still starts silent when the bank
  is renamed away, checked as AGENTS.md describes, with no audio hardware
  needed.
- **Does not decide:** whether the bank stays optional. That is the release's
  call, and AGENTS.md says to keep the fallback.

## Trial-build against engine master (`HYG#24`)

**Done 2026-10-29.** This game was built once against engine master `b96bad6`
(*Synchronize LineSweeper page-test key phases with UI updates*, 182 commits
after `v0.2.0`). The build ran in a scratch worktree of `a992d8f1` whose own
submodule was checked out at that commit, and the worktree was then removed.
The real pin stays at `v0.2.0` (`1e60fb2`), and `git submodule status` is
unchanged. Configuring `x64-debug` succeeded with no CMake change, because
nothing here names the math target.

The game, the editor and every acceptance case were built in passes. Each
pass added scratch-only stand-ins for one class, so the next class could
surface. A raw error count mostly measures cascades: the first pass printed
12,237 error lines, and `C1003` (too many errors) stopped 92 of them early.
The counts below are therefore distinct error sites, each a file and line.

| Pass and scratch stand-in | Game (`ColourWars`) | Acceptance (88 cases, `-CompileOnly`) |
|---|---|---|
| 1. none | 165 translation units fail. In 132 the first error is `mattmath`, and in 33 it is one of the seven removed types. The editor builds and links unchanged. | `run-acceptance.ps1` stops before compiling: "A required build output is missing" for `engine/math/MattMath.lib`, now `LabradorMath.lib` |
| 2. `namespace mattmath = labrador;` and the seven types copied from `v0.2.0` | 117 fail, every first error a constant: `ZERO` at 228 sites in 85 files, `PI`/`PI_OVER_2` at 21 in 11, `DEFAULT_CAMERA` 3, `ONE` 3 | with the library path patched: 61 fail; first errors are 54 constants, 4 removed members and 3 link failures; constants at 355 sites in 92 files; no `mattmath` errors, since the alias rides in `game/pch.h` |
| 3. master's constant table applied mechanically | 6 fail: 8 removed-member sites in 5 files, plus the `pi` collision below | 7 fail: 5 removed-member sites, plus the three link failures below; the other 81 cases build |
| 4. v0.2.0 bodies for the removed members | builds and links; `check-startup.ps1 -RequireAudio` passes on the master executable | not run |

Breaks by class, measured:

- **`mattmath` namespace and `LabradorMath`:** the first error in 132 game
  translation units. `MattMath.lib` is named in six scripts:
  `run-acceptance.ps1:222`, `check-display-awake.ps1`, `check-frame-pacing.ps1`,
  `check-movement-feedback.ps1`, `check-paint-fx.ps1` and `check-painter.ps1`.
  Each refuses to start without that library.
- **Constant renames:** 255 sites in 91 files in `game/` (pass 2) and 355 in
  92 files under `scripts/` (pass 2, acceptance). The grep estimate of about
  900 `ZERO` uses counted every use, not the sites a compiler reports first.
  Master's rename table covers every one, applied mechanically.
- **A new name collision, not in the estimate:** `game/debug/bot.cpp:22`
  declares its own `constexpr float pi`. Under `using namespace labrador` it
  is ambiguous with master's `labrador::pi` at lines 279, 318 and 430.
- **Removed members:** exactly the estimated set. In `game/`:
  `calculate_intermediate_camera` (`level.cpp:384`, `:659`,
  `level_intro.cpp:254`), `set_position_y_from_bottom` (`player.cpp:920`,
  `:1319`, `:1328`), `from_top_left_bottom_right`
  (`level_object_builder.cpp:114`) and `set_position_from_top_right_origin`
  (`results_menu.cpp:444`). In acceptance there are five
  `from_top_left_bottom_right` sites: `cq_blockout/fixtures.cpp:84`,
  `crossfeed/fixtures.cpp:84`, `press_tower/fire_checks.cpp:38`,
  `press_tower/fixtures.cpp:105` and `terrain.cpp:112`.
- **Seven removed types:** the first error in 33 game translation units.
  [`HYG#25`](#carry-the-seven-removed-engine-types-into-the-client-hyg25)
  owns them. The `v0.2.0` copies compiled with no error against master's
  public API once the alias and the constant table were applied to them.
  `UiTextDropShadow` was carried as a separate header over master's
  `widget.h`.
- **A new link break, not in the estimate:** `action_bindings`,
  `input_bindings` and `keyboard_jump_capture` each define a fake
  `labrador::GamepadReader`, and `painter_visual.cpp` has the same kind of
  fake. Master's reader adds `vibrate` and `stop_vibration` (`cadc962`, the
  [report 25](#25-no-public-controller-vibration-output---draft-with-isolated-adapter-exhibit)
  answer), and `Gamepads` calls them. The fakes lack them, so the linker pulls
  the engine's `gamepad_reader.cpp.obj` as well, and every reader member is
  defined twice (`LNK2005`, `LNK1169`). At the bump the fakes need both
  members.
- **`RectangleI` include:** no error. Every use reaches
  `engine/math/rectanglei.h` through another header.
- **`/utf-8`:** no error. `game/` and `editor/` are ASCII. The three
  non-ASCII acceptance sources (`team_palette.cpp`, `user_stages.cpp` and
  `level_editor/user_stage_checks.cpp`) are compiled by `run-acceptance.ps1`,
  whose own flag list does not inherit `labrador_settings`. At the bump the
  game objects get `/utf-8` but those probes do not, unless the runner adds it.
- **Runtime:** none in the master build beyond the manifest key `HYG#23`
  removed. Putting the old `_comment` back into the staged manifest made
  master refuse startup: "assets[3] has unknown key '_comment'".
- **Configure:** no change needed.

What this left for the bump: the rename sweep (namespace, the six script
library paths and the constants), the `pi` collision, the eight game and five
acceptance removed-member sites, the three fake readers' two members, `/utf-8`
in the acceptance runner, and `HYG#25`. It does not decide when to bump. Engine
releases set that, under the pin policy, and adopting master's names beyond
what compiles is the bump's work.

## Carry the seven removed engine types into the client (`HYG#25`)

**Done 2026-10-29.** The seven types now live in
[game/carried/](../../game/carried/), copied from the pinned engine
(`v0.2.0`, the last release that has them) into `namespace colourwars`. The
namespace keeps them from ever being ambiguous with an engine type: at the
pin, where the engine still declares its own copies, and at any later engine
type of the same name. Each copy keeps the original shape: the same bases,
virtuals, members, defaults and access. A diff against the engine sources
differs only in the namespace, `labrador::` qualifiers on engine names in the
headers (comments left as written), a file-scope `using namespace labrador` in
the sources that need it, and a provenance note. `UiTextDropShadow`, which was
part of `engine/ui/widget.h`, is its own header. Every use in `game/` and
`scripts/` names `colourwars::X` and includes `game/carried/*.h`. No file
includes one of the removed engine headers, and the editor uses none of the
types.

Hierarchy rungs that moved from the engine into the client, at the same depth:

- `MovingObject` under `Player` and `Projectile`, so the
  `ProjectileMist -> DiffusingProjectile -> Projectile -> MovingObject`
  chain AGENTS.md describes is unchanged.
- `AnimationObject` under `Player`, `Projectile`, `FigureClothing` and
  `PaintTileSplash`.
- `SoundBankObject` under `MenuPage` (so every main-menu and pause page),
  `ScreenMusic`, `ConnectionAudio` and `ResumeAudio`.
- `UiTextDropShadow`, itself on `labrador::UiWidget` and the carried
  `TextDropShadow`, under `PromptBar` and `WrappedTextDropShadow`.
- `CameraTools` and `RotationOrigin` are used rather than derived from, in
  `player_camera.h`, `debug_overlay_draw.cpp`, `weapon.h`, `weapon.cpp`,
  `projectile_details.h` and `tuning.cpp`.

`docs/oop-guide.md` names none of the seven, so it needs no change.

Verification at the pin:

- Default acceptance passes: x64-debug with CI's pull-request exclusions,
  and all 73 cases in x64-release.
- `menu_presentation` first failed on an include. Its `paper_checks.h` is a
  fragment with no includes of its own, and the old name had arrived through
  the engine's `widget.h`. `menu_presentation.cpp` now includes the carried
  header, and the case passes in both presets.
- `check-painter.ps1 -IdentityOnly` (Release) compiles the painter probe,
  whose 31 `painter_visual` fragments reach the carried header through
  `main_menu.h` and `results_menu.h`, and passes.
- A read-only review workflow (shape fidelity, coverage and master readiness,
  each finding put to a skeptic) confirmed only that `menu_presentation`
  include.

Against master, the `HYG#24` scratch trial was rebuilt at `b96bad6` with this
change applied. It used the namespace alias, the constant table and the
removed-member stand-ins, but no type shims. The game built and linked, and
`check-startup.ps1 -RequireAudio` passed. So at the bump the carried copies
need only the same namespace and constant renames as the rest of `game/`.

The scope as filed:

`730dd99` (2026-10-27, *Remove engine
types nothing uses*) deletes `MovingObject`, `RotationOrigin`,
`SoundBankObject`, `AnimationObject`, `CameraTools`, `TextDropShadow` and
`UiTextDropShadow`. Its `CHANGES.md` entry gives no replacement: "A game that
used one should copy the class from the previous release into its own
source." This game builds hierarchy rungs on four of them: `Player` and
`Projectile` on `MovingObject` (`player.h:104`, `projectile.h:25`), `MenuPage`,
`ScreenMusic` (`screen_music.h:10`), `ConnectionAudio` and `ResumeAudio` on
`SoundBankObject`, and `FigureClothing` and `PaintTileSplash` on
`AnimationObject`. `UiTextDropShadow` appears in 57 files. At `v0.2.0` the six
headers and their sources come to about 860 lines.

- **Scope:** copy each type from `v0.2.0` into `game/` with its shape
  unchanged: same bases, virtuals and members. Use names or a namespace that
  cannot become ambiguous with a later engine type. Repoint includes and base
  lists. Do not flatten, merge or turn a rung into a value type; AGENTS.md
  explains why. Record here which rungs moved from the engine into the
  client, and update `docs/oop-guide.md` at the bump.
- **First step:** list each type's engine dependencies at `v0.2.0` against
  master, for example `UiTextDropShadow` against master's `UiWidget`, which is
  still not `final`. That shows each copy compiles on master's public API
  alone.
- **Acceptance:** the copies build at the current pin and in the `HYG#24`
  scratch trial against master. No `game/` or `scripts/` file includes one of
  the seven removed engine headers. The hierarchy depth that AGENTS.md
  describes is unchanged. Consumer acceptance passes in both presets.
- **Does not decide:** whether any of these rungs should become an engine
  report. A copied class that keeps forcing a bad answer gets its own report
  in [Live](#live), with the copy as its exhibit.
