# Bug: resizing an explicitly hidden Window requests activation

Prepared local report with guarded native evidence. It remains unfiled under
the current task instruction.

## What happened

A hidden capture client keeps a thread-local CBT activation veto around ordinary
resolution Apply calls; five fixtures currently need it. The visible=false
contract describes a shell that takes no focus.

At checked master 19cf650a6aa5569125dac577a2281561f5a215f4,
Application::set_resolution forwards to Window::resize_client. The Windows
resize method passes SWP_NOMOVE|SWP_NOZORDER to SetWindowPos, omitting
SWP_NOACTIVATE. Hidden enter_fullscreen/leave_fullscreen already include that
flag. The earlier hidden-visibility bug was answered; this is the remaining
ordinary resize path.

Expected: resizing an explicitly hidden window should not require an activation
veto. A narrow fix could use SWP_NOACTIVATE only when hidden, preserving the
existing visible-window policy and requiring no new API.

Guarded native observations on 2026-10-24:

| Configuration | Activation requests vetoed | Focus requests | Client pixels | Foreground after resize/destruction | Exit |
|---|---:|---:|---|---|---:|
| Debug | 1 | 0 | 800x600 | unchanged / unchanged | 0 |
| Release | 1 | 0 | 800x600 | unchanged / unchanged | 0 |

The owned window remains hidden, inactive and unfocused at the checked
boundaries. This exhibit observes and refuses requests; it neither exercises
nor establishes actual foreground/focus takeover.

## Smallest guarded reproduction

Build hidden_resize_exhibit.cpp against the checked engine's existing Window
implementation. It creates one hidden synthetic Window and calls
resize_client({800,600}), without Application, renderer, audio, assets or states.
A current-thread CBT veto is installed before construction and stays through
destruction. It records/vetoes activation and focus requests only for this
process's windows on that thread. No unguarded negative control is present.

An opaque pre-existing foreground snapshot is compared after resize and after
Window destruction while the veto is still alive. Its HWND/title is never
printed or modified. The output contains only counts, equality/isolation
booleans and client dimensions; two equality samples do not establish continuous
desktop inactivity.

Exit 0 means an activation request was observed and vetoed with own-window
isolation, requested client size and an available unchanged foreground at both
samples. Exit 2 is inconclusive: no activation request observed, an unavailable
initial foreground, a desktop foreground change, or client-size mismatch. Focus
request counts can independently be positive even when no activation request
was observed. Exit 1 is setup or own-window isolation failure. These outcomes
do not prove a source fix or actual foreground/focus theft. A Windows desktop
and real HWND are required; this is device-free native, not true headless.

## Revision and necessary conditions

Upstream master resolved to 19cf650a6aa5569125dac577a2281561f5a215f4,
matching the local pin, in a read-only check on 2026-10-24 at 02:24:30 UTC.
Windows x64 (reported OS version 10.0.26200), MSVC toolset 14.51.36231; Debug (/MDd) and Release (/MD).
No private game
source/assets/captures, unrelated diagnostics or user/machine identifiers are
needed. Recheck master before any future filing. No external report is filed.

## Workaround an answer removes

The client workaround returns 1 for HCBT_ACTIVATE when the notification targets
its own probe HWND and otherwise calls CallNextHookEx. A hidden-only
no-activation resize would remove that hook and five uses; hidden/nonforeground
regression requirements stay. This fits the Bug or build problem intake form,
not a new window/input framework request.
