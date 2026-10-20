# Request: canonical names for physical Key values

Local **ENG#13** draft. **Unfiled.** Use Labrador's Engine request form when
publishing; this document records no upstream request or implementation.

## Need and exhibit

A client has a text-based developer binding file and a saved keyboard-action
preference. Both need to serialize and parse physical `labrador::Key` values
consistently. The client maintains key-to-token tables and lookup loops beside
the enum the engine owns.

The [standalone synthetic exhibit](../../scripts/engine-reports/key_names.cpp)
reduces that workaround to an explicit nine-entry table plus `key_name` and
`key_from_name`. Its only nonstandard include is `engine/input/keyboard.h`.
It opens no window, samples no hardware and reads no files or assets.

The example covers letters, a number-row key versus a numpad key, a function
key and named punctuation/navigation keys. It checks unique names/keys, both
round-trip directions and rejected unknown/noncanonical tokens, chords,
embedded NULs and enum sentinels. Its deliberately partial table is an exhibit,
not a proposed complete implementation. Runtime checks remain active under
`NDEBUG`.

This asks for stable serialization tokens. Localized display labels and typed
characters are separate concerns. Action choices, permitted key subsets,
chord grammar, case-normalization policy and compatibility with existing file
spellings remain client policy.

## Checked revision and observed gap

Upstream master was freshly fetched on **2026-10-20 at 19:34:53 UTC**:
`52e809a5a0229d37db4373faa7853df4c861a310`. The entire engine worktree was
confirmed unchanged against that `FETCH_HEAD` before these runs.

- [`engine/input/keyboard.h`](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/input/keyboard.h#L35)
  defines physical positions and a separate typed-text channel, but provides
  no canonical name parser/formatter.
- [`engine/app/win32/window.cpp`](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/app/win32/window.cpp#L14)
  already translates scan positions to `Key`; `WM_CHAR` supplies text. No new
  event translation is requested.
- [`engine/app/web/dom_events.h`](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/app/web/dom_events.h#L17)
  already exposes `key_from_code` for DOM `KeyboardEvent.code`. It is a
  platform event decoder with many-to-one modifier/Enter mappings and no
  reverse formatter. Its implementation is selected under `EMSCRIPTEN` in
  `engine/CMakeLists.txt`; it does not provide the portable canonical-token
  round trip demonstrated here.

Actual: the client repeats a canonical key-token table even though the engine
owns the physical-key enumeration. Expected: a small codec beside `Key`
supplies a documented canonical token per real enum key and a defined result
for unknown tokens, invalid values and sentinels. Tokens round-trip without
depending on enum ordinals, keyboard layout or backend event names.

## Reproduce

The example was compiled directly with MSVC **19.51.36256.0**, C++20,
`/EHsc /W4 /WX /permissive- /Zc:__cplusplus`. Debug used `/MDd /Od /D_DEBUG`;
Release used `/MD /O2 /DNDEBUG`. The source and the prebuilt
`LabradorEngine.lib` and `MattMath.lib` were the explicit link inputs. This
source calls no out-of-line engine API. These runs did not build a new
CMake consumer or inherit `labrador_settings`.

From an x64 VS Developer PowerShell at this checkout's root, with the
libraries already built at the checked revision, these commands reproduce the
same flags and inputs. The output directory and source filename differ from
the retained run; the source bytes are identical. For a separate engine
checkout, substitute its source root and corresponding library paths.

```powershell
$exhibitSource = 'scripts/engine-reports/key_names.cpp'
$engineSource = 'external/labrador'
$exhibitOutput = 'out/validation/key-names'
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
PASS canonical key names: nine explicit physical-key tokens; two-way round trips, distinct number/numpad, strict errors and sentinels; no window, devices or content
```

Each failed runtime check exits nonzero. The source SHA-256 is
`DE44E0FA7F966C98A2A1A512A0E2C5DD5F90FDA44654F6704319D5D13C094315`.
These executions verify the synthetic client workaround and its assertions,
not a naming API that the engine does not yet supply. They establish no
physical-device, layout-label or human acceptance.

## Optional mechanism and client boundary

Consider `std::string_view key_name(Key)` and
`std::optional<Key> key_from_name(std::string_view)` beside the enum, with
documented canonical spellings and unknown/sentinel results. Exact names,
return shapes and accepted spellings remain maintainer decisions.

An engine codec could replace the client's canonical token lookup. Existing
on-disk spellings may still need compatibility aliases if engine canonical
tokens differ; the client's allowed-key subset, UI copy, case normalization
and chord grammar remain. This changes neither the engine's deliberately
collapsed left/right modifiers and Enter representation nor physical-device
identity.

**T1:** extract the demonstrated naming mechanism while the client supplies
binding policy. **T3:** keep it a finite enum codec, without a localization
system, input framework, capture API or chord parser. The maintenance cost is
one complete documented mapping beside the enum; keeping small token tables
client-side is also a decision the maintainer can explain in those terms.
