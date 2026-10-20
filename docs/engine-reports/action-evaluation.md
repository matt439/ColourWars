# Request: evaluate digital bindings from device snapshots

Local **ENG#21** draft, carrying the former `GAP#F2` request. **Unfiled.**
Use Labrador's Engine request form when publishing; this document records no
upstream request or implementation.

## Need and exhibit

A client applies a saved keyboard action binding while input polling continues
across menus. It also maps a pad button and a trigger to one action. The engine
provides the physical-source operations; the client repeats the code that
evaluates binding data and combines those results.

The [standalone synthetic exhibit](../../scripts/engine-reports/action_evaluation.cpp)
reduces that workaround to `Sample`, `merge`, `keys` and `button_and_trigger`.
It uses only the public keyboard/gamepad snapshot APIs and the standard library.
It has no game objects, assets, save files, window, renderer, audio device or
physical input. Its binding change is an in-memory stand-in for an applied
preference.

It demonstrates two requirements:

- Changing Space to J while J is held in both snapshots reports held without
  inventing a press. A subsequent J release and fresh press retain their edges.
- With A held, LT crossing its supplied threshold still reports a source press.
  Releasing A while LT remains held reports both held and released.

The latter combines source edges independently. Reconstructing edges from the
aggregate held level gives different results. Both policies can be useful; an
extraction must document its semantics or expose source results for the client
to combine explicitly.

## Checked revision and observed gap

Upstream master was freshly fetched on **2026-10-20 at 19:34:53 UTC**:
`52e809a5a0229d37db4373faa7853df4c861a310`. The input source, application polling,
input tests, sample consumers and contribution/request instructions were
rechecked against that `FETCH_HEAD`.

`engine/input/gamepad.h` explicitly defers action mapping as a layer above
devices because the earlier clients had no rebinding screen. The new consumer
supplies that exhibit. The checked revision still requires the local evaluator
shown here; this is an extraction request, not a defect in its existing edge
functions.

## Reproduce

The example was compiled directly against **prebuilt** engine libraries, with
MSVC **19.51.36256.0**, C++20, `/EHsc /W4 /WX /permissive- /Zc:__cplusplus`.
Debug used `/MDd /Od /D_DEBUG`; Release used `/MD /O2 /DNDEBUG`. Only the source,
`LabradorEngine.lib` and `MattMath.lib` were explicit link inputs. It did not
build a new CMake consumer or inherit `labrador_settings` during that check.

From an x64 VS Developer PowerShell at this checkout's root, with the libraries
already built at the checked revision, these commands reproduce the same flags
and inputs. The output directory and source filename differ from the retained
run; the source bytes are identical. For a separate engine checkout, substitute
its source root and corresponding library paths.

```powershell
$exhibitSource = 'scripts/engine-reports/action_evaluation.cpp'
$engineSource = 'external/labrador'
$exhibitOutput = 'out/validation/action-evaluation'
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

**Actual and expected:** both retained runs exited 0, with empty stderr and:

```text
PASS saved-key replacement and independent source-edge composition
```

Each failed check returns nonzero; the checks remain active in Release. The
source SHA-256 is
`EDD1F412BD597D8B3171A6D7974A4D9D97361C09C2C13E65307C861C0C13B81C`.
These executions verify the synthetic wrapper and assertions, not an engine
action evaluator that does not yet exist. They establish no physical-device,
human comfort, hardware or performance acceptance.

## Optional mechanism and client boundary

Consider extracting a small, pure evaluator over typed physical bindings and
current/previous snapshots. An answer could remove the generic helper bodies
above while retaining their binding data and assertions. **T1** applies: extract
mechanism from a working client while the client supplies policy. The exact API
and whether this amount of code warrants extraction remain maintainer decisions.

Action IDs/defaults, supplied threshold values, gameplay interpretation,
persistence, labels, conflicts/capture UI, menu release ownership/repeat and
roster policy stay in the client. Menu-specific previous-held reconstruction is
also outside this example; it is not a proposed universal primitive.

`Gamepads` already offers per-slot current/previous state, button edges,
caller-supplied trigger thresholds and connection transitions. Keyboard focus,
mouse button epochs and coordinated application polling also exist. None is
requested again. Pad capture/introspection, device identity, rumble, analog
movement/aim, a global action registry and a general input framework have no
additional exhibit here and are outside this request.
