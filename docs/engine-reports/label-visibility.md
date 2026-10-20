# Request: independent optional visibility for Label

Local **ENG#18** draft. **Unfiled.** Use Labrador's
[Engine request form](https://github.com/matt439/Labrador/issues/new/choose)
when publishing. Freshly fetch and recheck upstream master before any filing;
the dated check below does not waive that requirement.

## Client exhibit and supported alternatives

A client's optional station-name overlay creates scene-owned Labels on first
use. To hide them while retaining their words and measured extent, it sets
their tint to `Colour(0, 0, 0, 0)`. Showing them restores the current
presentation colour. This is a supported workaround. The proposed Label
visibility flag is a convenience for preserving text, bounds and colour as
independent state, rather than a claim that Label cannot be hidden.

The [standalone synthetic exhibit](../../scripts/engine-reports/label-visibility/label_visibility_recording.cpp)
reduces that workaround to one Label with A/B glyphs and a small RGBA atlas,
all constructed in memory through public APIs. It creates the null renderer
with `nullptr` and copies the actual submitted sprite recording. It loads no
asset, opens no window, samples no hardware and replaces no engine method.

Zero RGBA preserves the words and measured box and still records two glyph
quads, all with zero vertex colour. Restoring the shown tint restores the
same geometry and colour. An alpha-only control checks that
`make_transparent()` retains RGB in recorded vertices; it makes no pixel or
blending assertion.

The same exhibit also verifies an existing alternative: `set_text(L"")`
records no glyphs and changes the measured extent to empty. Restoring the
saved words restores the original geometry. A client-owned GameObject that
contains a Label could also choose whether to forward draw. These remain
valid alternatives to adding an engine visibility operation.

## Checked revision and scope

Upstream master was freshly fetched on **2026-10-20 at 19:34:53 UTC**:
`52e809a5a0229d37db4373faa7853df4c861a310`. The engine worktree was confirmed
unchanged against that fetched master before the retained runs.

- `engine/render/label.h` declares Label final; `label.cpp` forwards its draw
  to TextObject and exposes its measured bounds.
- `engine/render/text.h` exposes text, colour, scale and position setters,
  with no independent visibility setter. `text_object.h` has a protected
  font setter, which is a separate public-API question.
- `engine/render/resource_factory.h`, `font.h` and
  `engine/render/null/recording.h` provide the real synthetic atlas, font and
  recording seam used here.

These declarations establish the missing convenience operation. They do not
establish a renderer defect, inability to hide text, or need for a broader
Scene visibility framework.

## Reproduce

The supplied [CMake target](../../scripts/engine-reports/label-visibility/CMakeLists.txt)
built with MSVC **19.51.36256.0**, Ninja, C++20,
`/W4 /WX /permissive- /EHsc /utf-8`. Debug used
`/Zi /Ob0 /Od /RTC1 /MDd`; Release used `/O2 /Ob2 /DNDEBUG /MD`.
Each build compiled 26 translation units: the exhibit, fourteen render units
selecting the null backend, and eleven math units. It then linked one console
executable using the normal toolchain runtime/system inputs. No prebuilt
engine, D3D/Vulkan/OpenGL backend library, RapidJSON or asset loader is an
input to this CMake project. It does not configure the full engine target.

From an x64 VS Developer PowerShell at this checkout's root, these equivalent
commands use the same tested source/CMake bytes at their committed paths.
The source/output directory locations differ from the retained run. For a
separate checkout, substitute its Labrador source root.

```powershell
$exhibit = (Resolve-Path 'scripts/engine-reports/label-visibility').Path
$engine = (Resolve-Path 'external/labrador').Path
$output = Join-Path (Get-Location).Path 'out/validation/label-visibility'
cmake -S $exhibit -B "$output/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug "-DLABRADOR_SOURCE_DIR=$engine"
if ($LASTEXITCODE -ne 0) { throw 'Debug configure failed' }
cmake --build "$output/debug"
if ($LASTEXITCODE -ne 0) { throw 'Debug build failed' }
& "$output/debug/label_visibility_recording.exe"
if ($LASTEXITCODE -ne 0) { throw 'Debug exhibit failed' }
cmake -S $exhibit -B "$output/release" -G Ninja -DCMAKE_BUILD_TYPE=Release "-DLABRADOR_SOURCE_DIR=$engine"
if ($LASTEXITCODE -ne 0) { throw 'Release configure failed' }
cmake --build "$output/release"
if ($LASTEXITCODE -ne 0) { throw 'Release build failed' }
& "$output/release/label_visibility_recording.exe"
if ($LASTEXITCODE -ne 0) { throw 'Release exhibit failed' }
```

**Actual and expected:** both retained configurations built successfully and
exited 0 with empty stderr and exactly these thirteen normalized stdout lines:

```text
shown.sprites=2
zero_tint.sprites=2
zero_tint.rgba_zero=true
zero_tint.text_and_bounds_preserved=true
zero_tint.geometry_matches_shown=true
alpha_only.sprites=2
alpha_only.retains_rgb=true
restored_tint.sprites=2
restored_tint.matches_shown=true
empty_text.sprites=0
empty_text.bounds_empty=true
restored_text.matches_shown=true
PASS label-visibility-recording
```

The checks remain active in Release and any failure exits nonzero. Source
SHA-256:
`C132437E6E5F5106308870F22AD8CE38FA445C5FA3168243868A86D72810C514`.
CMake SHA-256:
`D546794545BD191AB6402CC20ADF0F8A381F6755DB2BF3BA1BD63F2A4D729E52`.
The executable writes no files. These are actual geometry/tint/state
observations, not an implemented visibility API or a pixel/performance result.

## Proposed mechanism and trade-off

Consider a Label-level operation such as `set_visible(bool)` that can decline
glyph emission while retaining text, measured bounds and colour. Exact naming
and whether client composition is preferred remain maintainer decisions. A
Label-level flag need not change Scene's culling or add a generic retirement
mechanism.

**T1:** the existing client encodes optional display into a style property
and restores that style from presentation state. A direct Label choice would
separate those concerns without a general visibility framework.
Its maintenance cost is one state choice and tests; retaining zero tint,
saved-text restoration or client composition is also a defensible decision.
No measured CPU/GPU saving or frame-time improvement is claimed.

## Separate ENG#18 portions

This fixture calls Label directly and does not exercise Scene's culling,
ownership, admission, retirement or draw traversal. Null recording observes
submitted quads and vertex colours; it does not rasterize, prove pixel
invisibility, assess readability or measure performance.

Scene category order, a possible layering mechanism, retirement of ordinary
objects and public font rebinding remain distinct open questions. The current
renderer already documents that call order determines coverage and
`layer_depth` does not order draws; this report does not call that contract a
bug. Those portions need their own narrowly scoped exhibits or source audit.

A visibility flag would not itself remove lazy first-use `end_tick()`
admission. That call is still needed for pending scene objects unless the
client changes when it creates them. The only convenience requested here is
independent Label visibility; the other ENG#18 questions remain outside it.
