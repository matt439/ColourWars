# Request: parse JSON candidate text with a logical source name

Local **ENG#12** draft. **Unfiled.** Use Labrador's
[Engine request form](https://github.com/matt439/Labrador/issues/new/choose)
when publishing; this document records no upstream request or implementation.
Freshly fetch and recheck upstream master before any filing. The dated check
below does not waive that requirement.

## Need and exhibit

A live-tuning client already has an edited JSON document in memory. It needs
to run its ordinary checked configuration reader, including its own range
and cross-field rules, before adopting the edit. The public engine API only
constructs `JsonDocument` through `read_json_file(const char* path)`. The
client therefore writes an owned temporary candidate file, reads it back,
removes it, and rewrites its physical pathname in diagnostics to the logical
content filename.

The [standalone synthetic exhibit](../../scripts/engine-reports/json-candidate-file-bridge/json_candidate_file_bridge.cpp)
reduces that workaround to one range reader and three ASCII candidates.
`CandidateFileBridge`, the range rule and the diagnostic rewrite are
client-owned code. Engine parsing and checked `JsonValue` access are real
calls to the current implementation. There are no game headers, game data,
assets, application, window or device setup.

The valid candidate yields the range `1..4`. Its owning document remains
usable after the scratch file is removed, but `source_path()` retains that
file's physical pathname. An engine type error and a client cross-field
error both retain the scratch source and the relevant key context. The
client has to replace that pathname before displaying each diagnostic.
This behavior is consistent with a file-reading API; the request is to
avoid the bridge when the caller already owns the candidate text.

## Checked revision and observed gap

Upstream master was freshly fetched on **2026-10-20 at 19:34:53 UTC**:
`52e809a5a0229d37db4373faa7853df4c861a310`. The checked engine HEAD and
`FETCH_HEAD` matched. The JSON and file-path sources used below were
confirmed unchanged against that fetched revision.

- [`engine/assets/json.h`](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/assets/json.h)
  exposes `read_json_file(const char*)`, checked borrowed `JsonValue` views,
  and a move-only owning `JsonDocument`. Its implementation constructor is
  private; there is no public text-and-source-name parse entry point.
- [`engine/assets/json.cpp`](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/assets/json.cpp)
  reads the file into a local string, parses it internally, and records the
  supplied path as the document's diagnostic source.
- [`tests/assets/json_tests.cpp`](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/tests/assets/json_tests.cpp)
  already uses a temporary-file helper and documents the absence of an
  in-memory entry point. This request adds an existing live-editing client
  need; it does not report the file reader as broken.

Actual: validating an already-owned candidate through the checked reader
requires filesystem IO and produces scratch-path diagnostics. Expected:
parse those bytes into the same owning document with a caller-supplied
logical source label, without a temporary file.

## Reproduce

The supplied [CMake target](../../scripts/engine-reports/json-candidate-file-bridge/CMakeLists.txt)
compiled with MSVC **19.51.36256.0**, Ninja, C++20,
`/W4 /WX /permissive- /EHsc /utf-8`. Debug used
`/Zi /Ob0 /Od /RTC1 /MDd`; Release used `/O2 /Ob2 /DNDEBUG /MD`.
Each configuration compiled exactly three translation units and linked one
console executable:

1. `json_candidate_file_bridge.cpp`;
2. Labrador `engine/assets/json.cpp`;
3. Labrador `engine/core/file_path.cpp`.

The only nonstandard dependency is header-only RapidJSON. No prebuilt
engine library or full engine/game target is an input to this CMake project.
Its engine-root and RapidJSON include directories are explicit cache inputs.

From an x64 VS Developer PowerShell at this checkout's root, these equivalent
commands use the identical tested source and CMake bytes in their committed
locations. The output directory differs from the retained run. For another
checkout, substitute its engine root and RapidJSON include directory. Use
new scratch directory names on every invocation: the exhibit refuses an
existing scratch directory and never overwrites its contents.

```powershell
$exhibit = (Resolve-Path 'scripts/engine-reports/json-candidate-file-bridge').Path
$engine = (Resolve-Path 'external/labrador').Path
$rapidjson = (Resolve-Path 'external/rapidjson/include').Path
$output = Join-Path (Get-Location).Path 'out/validation/json-candidate-parsing'

cmake -S $exhibit -B "$output/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug `
    "-DLABRADOR_SOURCE_DIR=$engine" "-DRAPIDJSON_INCLUDE_DIR=$rapidjson"
if ($LASTEXITCODE -ne 0) { throw 'Debug configure failed' }
cmake --build "$output/debug"
if ($LASTEXITCODE -ne 0) { throw 'Debug build failed' }
& "$output/debug/json_candidate_file_bridge.exe" "$output/scratch-debug"
if ($LASTEXITCODE -ne 0) { throw 'Debug exhibit failed' }

cmake -S $exhibit -B "$output/release" -G Ninja -DCMAKE_BUILD_TYPE=Release `
    "-DLABRADOR_SOURCE_DIR=$engine" "-DRAPIDJSON_INCLUDE_DIR=$rapidjson"
if ($LASTEXITCODE -ne 0) { throw 'Release configure failed' }
cmake --build "$output/release"
if ($LASTEXITCODE -ne 0) { throw 'Release build failed' }
& "$output/release/json_candidate_file_bridge.exe" "$output/scratch-release"
if ($LASTEXITCODE -ne 0) { throw 'Release exhibit failed' }
```

**Actual and expected:** both retained configurations built successfully and
exited 0, with empty stderr and exactly these normalized stdout lines:

```text
valid.range=1..4
valid.source_is_scratch=true
valid.file_removed=true
typed_error.has_scratch_source=true
typed_error.has_context=true
semantic_error.has_scratch_source=true
semantic_error.has_context=true
client.scratch_writes=3
client.scratch_removals=3
client.logical_source_rewrites=2
PASS json-candidate-file-bridge
```

Both scratch directories contained no files after their runs. Each failed
runtime check exits nonzero, including in Release. The source SHA-256 is
`FAC153F2270FABC6B3AC26A64A85CFB4C69B930E20D60A3B29AC3A8D9AA94E3E`;
the CMake SHA-256 is
`36A1CF572AF6C9591E43D71C401794572B8156962D443063936735500589794C`.
These runs verify the existing engine behavior and the synthetic client
workaround, not a memory-parse API that the engine does not yet supply.

## Requested mechanism and client boundary

Provide an in-memory parse that produces the same owning, checked
`JsonDocument`, with a caller-supplied diagnostic source label. For example,
`parse_json(std::string_view text, std::string source_name)` could share the
existing parser and error path. Exact naming and parameter types remain
maintainer decisions. The document must own its parsed state independently
of the caller's text and source-name buffers; its borrowed `JsonValue`
lifetime rules remain explicit. Type and client semantic errors should
retain the logical source and existing key/index context.

Once the client's configuration reader accepts that document or its root,
this mechanism would let it remove the candidate-file/read-back/cleanup
bridge and scratch-path diagnostic replacement. Validation rules, resource
checks, live adoption, save policy and conflict handling stay with the client.

**T1:** extract a parsing entry point demonstrated by an existing client's
temporary-file workaround. **T3:** keep one shared read mechanism and a
diagnostic source label; do not add a JSON editor, schema system or live-tune
framework. The maintenance cost is one additional public entry point and
its ownership/error-path tests. Keeping the current file-only boundary is
also a decision the maintainer can explain in those terms.

## Other JSON requests remain separate

Object-member/key enumeration and retention of original source bytes/raw
token spans remain open portions of ENG#12, outside this fileable slice.
Arrays already have `size()`/`at()`, and filename/key/index diagnostics
already exist through `source_path()`/`where()`.

A memory parse alone would not replace the client's `JsonSpanDocument`
scanner, token-preserving splice or `TuneDocument` save/conflict policy.
Enumeration alone would not implement consumed-key tracking or unknown-key
diagnostics; those also need client knowledge of accepted/consumed fields.
This report requests no writer, mutation or source-span API and makes no
claim that its addition would delete those separate mechanisms.
