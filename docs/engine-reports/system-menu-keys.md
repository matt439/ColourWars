# Request: optional opt-out for bare system-menu keys

Local ENG#11 draft, unfiled. The standalone exhibit reproduces the limited
synthetic request path in Debug and Release. The default Windows behavior is
documented and intentional; this is an optional mechanism request, not an
assertion that Windows or the engine's current key translation is broken.

## Need and current API

A game uses F10 for a developer action. The engine translates its key message
and calls the keyboard callback, then forwards the message to `DefWindowProc`.
The default system-menu path follows that delivery. The client currently uses
a thread `WH_GETMESSAGE` hook to remove bare F10 messages and count the press
itself before the engine's message pump dispatches them.

The source-level gap is narrower than saying F10 never reaches the game:
`WindowNotify::on_key_down` and `on_key_up` return `void`, and current
ApplicationOptions/WindowOptions expose no bare-system-menu opt-out. A caller
can write its own native Win32 tap or hook, as the workaround does, but cannot
choose that behavior through the engine's existing public options.

## Checked source and Windows contract

Engine master revision `52e809a5a0229d37db4373faa7853df4c861a310` was freshly
fetched on 2026-10-20 at 19:34:53 UTC and
the entire engine worktree was confirmed to match that tree. Its Win32
`window_proc` forwards normal/system key down/up to the void callbacks and
then falls through to `DefWindowProc`. No system-menu option is present in
the checked public options. Refresh upstream immediately before filing.

Microsoft documents [F10's default key-up menu request](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-keydown)
and [the corresponding F10/Alt system-key-up request](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-syskeyup).
The [system-command contract](https://learn.microsoft.com/en-us/windows/win32/menurc/wm-syscommand)
identifies `SC_KEYMENU`; its low four command bits are masked when testing it.
These sources justify trapping the command before default menu handling.

## Minimal synthetic experiment

[The standalone source](../../scripts/engine-reports/system_keys_exhibit.cpp)
uses the actual hidden D3D11/XAudio2 Application,
one worker/view, no manifest/state/assets and no played sound. It reuses the
checked hidden-shell isolation foundation. Only its own top-level HWND is
moved outside measured virtual-desktop bounds; a current-thread CBT guard
vetoes activation only of that HWND.

Before stimuli, it installs an outer WndProc tap on that owned HWND. The tap
always returns 0 for masked `SC_KEYMENU` before calling the engine's previous
WndProc, so the engine/default procedure never handles that menu command.
A direct owned-HWND command self-test checks the barrier first. The tap leaves
the engine's `GWLP_USERDATA` intact and restores the original procedure; its
context outlives Application on failed-detach paths.

Only synchronous `SendMessageW` calls to the owned HWND feed these pairs:

| Pair | Messages | Down lParam | Up lParam |
|---|---|---|---|
| ordinary A control | `WM_KEYDOWN` / `WM_KEYUP`, `'A'` | `0x001E0001` | `0xC01E0001` |
| bare F10 | `WM_SYSKEYDOWN` / `WM_SYSKEYUP`, `VK_F10` | `0x00440001` | `0xC0440001` |
| left Alt alone | `WM_SYSKEYDOWN` / `WM_SYSKEYUP`, `VK_MENU` | `0x20380001` | `0xC0380001` |

Each uses repeat count 1, a set-1 scan position and no extended flag. Release
sets previous/transition bits. Bare F10 uses the documented
[system-key message route](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-syskeydown)
with zero Alt context. Alt context is set on Alt-down and cleared on
Alt-up, following the documented [message context flags](https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input).
No `TranslateMessage`, global input, Win32 keyboard-state setter, foreground call
or general message pump is used. After an initial Keyboard poll, the fixture
records `Keyboard::focused()` and requires that logical snapshot value to
remain unchanged before each pair, after each down/up poll and before cleanup.
It prints the initial logical value rather than assuming it is false.
Application's foreground/shown/keyboard-presence members default to true and
initialize the engine input focus from those facts; hidden creation need not
produce a transition that clears them. This logical flag is independent of
the fixture's repeated native hidden/offscreen/nonforeground checks, and is
not an OS-focus trace. Raw `KeyboardState::is_down` samples prove the real key
translation and callback path without claiming physical or focused gameplay
input delivery.

The program prints the initial logical focus value, then the self-test and
three pair observations as CSV. The ordinary key must produce no menu request.
Both special pairs must reach the
raw engine key snapshots and generate intercepted, zero-character
`SC_KEYMENU` requests to count as a reproduction. Hidden/offscreen/foreground
and menu-entry observations must remain valid, and the engine's window context
must be unchanged.

## Measured result

On 2026-10-20, both x64 Debug and Release builds compiled with MSVC
19.51.36256.0, `/W4 /WX`, against the existing normal engine/math/import
libraries. Both runs exited 0 with empty stderr and identical stdout:

```text
initial_logical_keyboard_focus=1
phase,down_messages,up_messages,raw_down,raw_up,menu_requests,zero_character_requests
guard,0,0,0,0,1,1
ordinary_a,1,1,1,1,0,0
f10,1,1,1,1,1,1
bare_alt,1,1,1,1,1,1
REPRODUCED synthetic system keys: F10 and bare Alt reached engine Key snapshots and requested SC_KEYMENU; no menu-loop entry, global input or OS-focus-delivery claim
```

The guard row is the deliberate command-barrier self-test, not a generated key
request. Each key pair reached the real engine's raw down/up snapshots exactly
once. A generated no menu request; F10 and bare Alt each generated exactly one
intercepted zero-character request. All repeated native isolation checks,
logical-focus invariance checks, no-menu-entry observation, engine-context
check and WndProc restoration completed. The initial logical focus was 1 in
both runs; that is not evidence of OS focus gain.

The program returns:

- 0 with `REPRODUCED` only if both special-key request paths are observed.
- 2 with `NOT_REPRODUCED` if hidden/inactive delivery does not establish those
  requests; that outcome is not permission to activate the window or enter
  native menu mode to force evidence.
- 1 for initialization, barrier, translation, isolation or cleanup failure.

This experiment observes synthetic owned-window dispatch and a prevented menu
request. It does not deliver physical input through the OS, enter menu mode,
measure a modal game pause, test Alt+F4 or test foreground/focus transitions.

## Reproduction and local receipts

From an x64 MSVC Developer shell, invoke
[`build-system-keys.ps1`](../../scripts/engine-reports/build-system-keys.ps1)
with the existing engine source/build directories, matching vcpkg dependency
and game runtime directories, `Debug` or `Release`, and a private output
directory. The recipe compiles one source against prebuilt libraries and
copies only `DirectXTK.dll` and `xaudio2_9redist.dll`; it does not configure or
rebuild the engine, link game objects or launch the exhibit. The ordinary
graphics and audio services must be available even though no assets or sound
are used. Launch the resulting console program hidden with stdout/stderr
redirected and retain its actual exit code before interpreting the output.

The executed source SHA-256 is
`03F7BE3B241E5C18D342FD3EC8D5162BD6C15ED1C5E5337BEF174E209F27805C`.
Local receipts are under
`out/validation/crossfeed-20261020/system-keys-report-final-v2/`, including
`result.json`, each preset's exit receipt, compile arguments/log and stdout/
stderr. The earlier fixture's false-logical-focus assumption and incomplete
wrapper receipt are retained privately; they do not establish OS focus gain.
The final source samples logical focus as described above. The engine remains
unchanged, and this report remains unfiled. Refresh upstream immediately
before filing.

## Optional mechanism and client boundary

Consider a caller-owned option that suppresses bare keyboard-initiated system
menu requests while leaving the default desktop behavior available. The
observed zero-character requests would support a narrowly defined filter;
the exact option and semantics remain the maintainer's decision. Existing
key callbacks should continue receiving their translated keys.

Alt+F4, explicit window commands and mnemonic/chord behavior need to retain
their intended contracts; this exhibit does not establish their acceptance.
No generic raw-event interception framework, key-name codec, capture UI,
input ownership model or physical-pad API is requested. T1 applies: the client
chooses its developer action and the engine supplies a way to choose whether
bare system-menu handling follows it. An answer could remove the native F10
queue hook and its manual press counter; keeping or removing F11 is client UI
policy, not an engine requirement.
