# Request: read the active view context while drawing a world object

Local **ENG#20** draft. **Unfiled.** Use Labrador's
[Engine request form](https://github.com/matt439/Labrador/issues/new/choose)
when publishing. Freshly fetch and recheck upstream master before any filing;
the dated check below does not waive that requirement.

## Client need and current workaround

A client chooses different art for small arena previews: terrain is projected
into view pixels, selected strokes retain a minimum thickness, and minor
ruler marks may be omitted. Projection, minimum thickness, detail selection
and the choice of affected views are client art policy.

Those decisions need the current camera where the object's terrain is drawn.
The client keeps a camera-taking miniature method beside ordinary
`GameObject::draw(DrawList&)` and calls it from its own view traversal. That
walk sets the camera/viewport, repeats culling, mirrors scene-owned scenery
admissions into borrowed pointers, and dispatches structures before their own
paint and later actors. The actual client exhibits are `draw_miniature_view`,
`Level::draw_miniature_world` and `Structure::draw_miniature`. The engine
continues to own the world objects; the caller already holds the view values
outside the object's normal draw call.

## Current API evidence

Upstream master was freshly fetched on **2026-10-20 at 19:34:53 UTC**:
`52e809a5a0229d37db4373faa7853df4c861a310`. The engine worktree was checked
unchanged against that master before the retained runs. The following public
declarations establish the narrow access gap; the runtime illustration does
not pretend to exercise an absent API.

- `engine/core/game_object.h`: the normal virtual is
  `draw(DrawList&) const`, receiving neither Scene nor an active view index.
- `engine/render/renderer.h`: DrawList has camera/viewport setters, including
  per-range camera changes, but no read accessor for those current values.
- `engine/scene/scene.h`: a caller can read `Scene::view(index)`. This existing
  access does not give an object's normal draw its active index or range state.
- `GameObject::cull_bounds(float units_per_pixel)` already receives inverse
  zoom for conservative extent. It is not full draw context. A mutable cache
  there would be an unsafe handoff under shared-object parallel-view drawing.
- Scene's ViewOverlay receives an index after world objects are drawn. It can
  recover caller-owned view values, but moving terrain into that final pass
  can change its relation to later paint/actors. This is an ordering boundary,
  not a broken callback.

No nonexistent engine call or intentional compile-failure test is used.
Ordinary world geometry already draws correctly through the current API.

## Minimal synthetic illustration

The [standalone exhibit](../../scripts/engine-reports/view-context/view_context_recording.cpp)
uses the null renderer and an actual GameObject-derived client object with
an authored rectangle `(16,16,16,1)`. Its 2x2 RGBA texture is constructed
entirely in memory. Two caller-owned views use scales 1 and 0.25; the second
also has camera translation `(8,8)` and a nonzero viewport origin.

Ordinary drawing applies the existing camera mapping and edge truncation,
recording boxes of 16x1 and 4x0 view pixels. That is the expected result for
authored world geometry, not a renderer defect. A separately named,
**client-owned** `ClientViewContext` and `draw_with_context` helper project
the rectangle, apply a labelled minimum-one-pixel art policy, draw in local
pixel coordinates, and restore the supplied caller values. The boxes are
then 16x1 and 4x1. A following ordinary draw checks normal-path restoration;
the object itself remains immutable.

The second pane's geometry starts at local `(2,2)`, while its viewport starts
at back-buffer `(64,0)`. The fixture checks them separately, without adding
the pane origin twice. It also checks the existing cull-bounds allowance for
inverse scales 1 and 4, using a pure calculation rather than a state cache.

This is an explicit-context alternative callers can already implement. The
helper is not an engine method or proof that every custom draw needs a new
API. Its caller must supply values matching the current draw range; it
cannot inspect or repair stale context.

## Reproduce and observed result

The supplied [CMake target](../../scripts/engine-reports/view-context/CMakeLists.txt)
built with MSVC **19.51.36256.0**, Ninja, C++20,
`/W4 /WX /permissive- /EHsc /utf-8`. Debug used
`/Zi /Ob0 /Od /RTC1 /MDd`; Release used `/O2 /Ob2 /DNDEBUG /MD`.
Each compiled 21 translation units: the fixture, nine render units selecting
the null backend, and eleven math units. It linked one console executable
with the normal toolchain runtime/system inputs. No prebuilt engine or
graphics-backend library, loader, asset file, application or Scene closure
is an input. The target does not configure the full engine.

From an x64 VS Developer PowerShell at this checkout's root, these equivalent
commands use the identical tested source/CMake bytes at their committed
locations. Source/output directory locations differ from the retained run;
substitute the engine source root when using a separate checkout.

```powershell
$exhibit = (Resolve-Path 'scripts/engine-reports/view-context').Path
$engine = (Resolve-Path 'external/labrador').Path
$output = Join-Path (Get-Location).Path 'out/validation/view-context'
cmake -S $exhibit -B "$output/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug "-DLABRADOR_SOURCE_DIR=$engine"
if ($LASTEXITCODE -ne 0) { throw 'Debug configure failed' }
cmake --build "$output/debug"
if ($LASTEXITCODE -ne 0) { throw 'Debug build failed' }
& "$output/debug/view_context_recording.exe"
if ($LASTEXITCODE -ne 0) { throw 'Debug exhibit failed' }
cmake -S $exhibit -B "$output/release" -G Ninja -DCMAKE_BUILD_TYPE=Release "-DLABRADOR_SOURCE_DIR=$engine"
if ($LASTEXITCODE -ne 0) { throw 'Release configure failed' }
cmake --build "$output/release"
if ($LASTEXITCODE -ne 0) { throw 'Release build failed' }
& "$output/release/view_context_recording.exe"
if ($LASTEXITCODE -ne 0) { throw 'Release exhibit failed' }
```

**Actual and expected:** both retained configurations built successfully,
exited 0 with empty stderr, and produced exactly these ten normalized lines:

```text
ordinary.view0.pixels=16x1
ordinary.view1.pixels=4x0
explicit.view0.pixels=16x1
explicit.view1.pixels=4x1
explicit.view1.local_origin=2,2
explicit.view1.viewport_origin=64,0
explicit.restores_caller_camera_and_viewport=true
object.bounds_unchanged=true
cull_bounds.scale_allowance_supported=true
PASS view-context-recording
```

Runtime checks remain active in Release and failures exit nonzero. The source
SHA-256 is
`448F8AE731B2BF1F5F06B77BADD54DD5B4B81B9CA78EB8FAB0AE79A8307CDA3A`;
CMake SHA-256 is
`C243576A5DD76D3DA2CA7B75CABCE4041F8824A9F8C52FD778ED9E502A713A0F`.
The executable writes no files and samples no hardware. Null recording
observes submitted geometry and viewport stamps, not pixels, blending,
readability or performance.

## Proposed narrow mechanism and boundaries

Allow a normal world-object draw to read its current camera and viewport
without entering a backend or keeping mutable per-view state on the object.
Read-only DrawList accessors or explicit context at normal draw dispatch are
possible shapes; the maintainer decides the API. Current per-range state
matters because one list may switch from world to HUD coordinates.

This requests context access, not a minimum-stroke renderer, miniature-art
framework, layer, reordering or Scene ownership change. Culling, ordered
traversal and fan-out remain separate mechanisms. Context access could let
this client's normal draw choose its projection in its existing world slot,
but removing borrowed lists and a repeated traversal would still need a
separate client integration check. Collision-span ordering is a separate
contract concern.

The fixture directly calls GameObject drawing. It does not instantiate or
test Scene ownership, admission, retirement, cull traversal, world ordering
or worker fan-out. The source audit and actual client exhibit establish those
integration concerns; the small runtime illustration does not validate them.

**T1:** the client already carries an explicit-camera path. Exposing the
small read-only information it needs would retain art policy in the client.
Caller-owned context and a custom traversal remain valid alternatives; the
API and maintenance trade-off belong upstream.
