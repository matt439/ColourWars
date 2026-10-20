# Request: optional observer before state updates

Local **ENG#9 observer slice**, prepared from the existing per-frame/input-capture
draft. **Unfiled.** Use Labrador's Engine request form when publishing. The
capture/routing half remains separately scoped; this document records no
upstream request, implementation or closure.

## Need and exhibit

A client-wide developer overlay inspects input before whichever menu or
gameplay state is currently on top. The client currently forwards from its menu
base and gameplay state to the overlay controller, then explicitly gates
lower-priority readers while a console or physical-key capture owns input.

The [standalone synthetic exhibit](../../scripts/engine-reports/input_observer.cpp)
reduces this to current Keyboard, State and StateContext APIs. It shows:

- A pushed non-covering state suspends the underlying update. covers_screen
  affects drawing, not which states update.
- Two raw readers correctly observe the same edge and typed-text snapshot.
  Reading raw input does not consume it.
- A client can instead use a permanent root State with a nested StateContext,
  schedule an overlay before that stack, and gate the ordinary action reader
  explicitly. The world keeps updating. The selected closing-release frame is
  also gated; a direct raw reader still observes the release.

These are documented behaviors and a working composition alternative. The
request is to reduce repeated orchestration, not to correct device edges or
make an otherwise impossible live overlay possible. A real composed root must
also forward its chosen lifecycle, drawing and result behavior; the example
does not supply a full application host.

## Checked revision and source observation

Upstream master was freshly fetched on **2026-10-20 at 19:34:53 UTC**:
`52e809a5a0229d37db4373faa7853df4c861a310`. A whole-engine-worktree comparison
at 19:50:08 UTC confirmed the reviewed sources match that FETCH_HEAD.

`engine/app/application.cpp` polls Gamepads, Keyboard, Mouse and Touch, then
calls StateContext::update directly. Application's update/render methods are
private, and its public API exposes no client callback between polling and
the stack update. `engine/core/state_context.cpp` updates only its top state.

That absence is source evidence. The example does not instantiate Application
or execute its real frame loop.

## Reproduce

The example was compiled directly against **prebuilt** engine libraries with
MSVC **19.51.36256.0**, C++20, `/EHsc /W4 /WX /permissive- /Zc:__cplusplus`.
Debug used `/MDd /Od /D_DEBUG`; Release used `/MD /O2 /DNDEBUG`. The source,
`LabradorEngine.lib` and `MattMath.lib` were the only explicit link inputs.
MattMath was included in the checked commands; this source does not establish
that it is required. No game object files or new CMake consumer were built,
and the direct check did not inherit labrador_settings.

From an x64 VS Developer PowerShell at this checkout's root, with those
libraries already built at the checked revision, these commands reproduce the
flags and inputs. Source/output filenames differ from the retained run; the
source bytes are identical. For a separate engine checkout, substitute its
include root and matching library paths.

```powershell
$exhibitSource = 'scripts/engine-reports/input_observer.cpp'
$engineSource = 'external/labrador'
$exhibitOutput = 'out/validation/input-observer'
New-Item -ItemType Directory -Force $exhibitOutput | Out-Null

$debugLibraries = 'out/build/x64-debug/external/labrador/engine'
cl.exe /nologo /std:c++20 /EHsc /W4 /WX /permissive- /Zc:__cplusplus `
    /MDd /Od /D_DEBUG /I $engineSource $exhibitSource `
    "$debugLibraries/LabradorEngine.lib" "$debugLibraries/math/MattMath.lib" `
    "/Fo$exhibitOutput/debug.obj" "/Fe$exhibitOutput/debug.exe"
if ($LASTEXITCODE -ne 0) { throw 'Debug compile failed' }
& "$exhibitOutput/debug.exe"
if ($LASTEXITCODE -ne 0) { throw 'Debug exhibit failed' }

$releaseLibraries = 'out/build/x64-release/external/labrador/engine'
cl.exe /nologo /std:c++20 /EHsc /W4 /WX /permissive- /Zc:__cplusplus `
    /MD /O2 /DNDEBUG /I $engineSource $exhibitSource `
    "$releaseLibraries/LabradorEngine.lib" "$releaseLibraries/math/MattMath.lib" `
    "/Fo$exhibitOutput/release.obj" "/Fe$exhibitOutput/release.exe"
if ($LASTEXITCODE -ne 0) { throw 'Release compile failed' }
& "$exhibitOutput/release.exe"
if ($LASTEXITCODE -ne 0) { throw 'Release exhibit failed' }
```

**Actual and expected:** both retained executions returned 0, with empty stderr
and these four lines, comparing text after normalizing CRLF/LF:

```text
PASS pushed non-covering state suspends the lower update
PASS independent raw readers observe the same edge and text
PASS composed root host and client gate preserve live updates
PASS synthetic overlay scheduling and cooperative input routing
```

The original Debug receipt helper rejected line endings despite recording
exit 0 and all four lines. Its retained output was reconciled after line-ending
normalization; the source and executable were unchanged and Debug was not
rerun. No Debug process ID was retained.

Checks use explicit exceptions and a failing return value, so Release retains
them. Source SHA-256:
`D944D382B5F7F119E70CB20E0CF6F07780EC59CE95B85CC7914736D4DF704C0E`.
The example initializes no Application, window, graphics/audio/physical-input
devices or game objects. These results verify the existing APIs and synthetic
workaround, not a callback or capture API that does not exist. They establish
no human, hardware, performance or cross-platform acceptance.

## Optional mechanism and client boundary

Consider an optional client-owned observer after device polling and before
ordinary state updates. No new API spelling is assumed. **T1** applies:
extract a shared mechanism from actual client forwarding while the client
retains policy. The cost is another shell callback with lifetime, ordering
and state-mutation rules. The permanent-root alternative already provides the
necessary expressiveness, so the maintainer may reasonably retain composition.

An observer could remove or relocate global forwarding calls. It would not
delete the client's whole menu-update template or its state-specific debug
hooks, which also perform music, motion, scripts, rebuild and timing work.

An observer alone does not hide raw input. A filtered view or capture mechanism
is the separately scoped, unanswered half of ENG#9. It would require a precise
contract and reader adoption; the synthetic gate does not establish a universal
consumption policy. Client policy includes priority, pausing, affected readers,
keyboard/pad ownership, neutral entry, release/fresh-confirmation rules, chords,
and queued developer work. Capture cannot distinguish playing a key from
typing that same key. Whole-keyboard-seat suppression remains a client choice.

Existing coordinated polls, snapshots, edges, caller-supplied trigger
thresholds, focus/connection notifications and mouse capture-cancellation
epochs are not requested again. Key labels, system-key interception,
control-key repeat, post-stack drawing and pad identity are outside this
slice. Refresh upstream immediately before filing.
