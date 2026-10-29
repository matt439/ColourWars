# Level editor

`ColourWarsEditor.exe` is a separate Windows desktop application for editing
the game's level JSON files. It preserves fields that the editor does not know
about. The canvas
shows schematic geometry, type colours, paintable edges, ramps, spawn points and
bounds. It has no textures, audio or game simulation; test the saved level in
the game to judge how it plays.

## Build and open

Use an x64 Visual Studio Developer shell with `VCPKG_ROOT` set, as for the game:

```powershell
cmake --preset x64-debug
cmake --build --preset x64-debug
./out/build/x64-debug/editor/ColourWarsEditor.exe
```

You can also pass a level path as the first argument:

```powershell
./out/build/x64-debug/editor/ColourWarsEditor.exe ./game/content/levels/turbulence.json
```

The editor's file dialogs start in `game/content/levels/` in this checkout.
Edit source files there: the copies beside `ColourWars.exe` are replaced by
the next build. **File > Save to My Stages** instead saves into the
player's own stage folder, which the game reads without a rebuild ([Play the
level](#play-the-level)). The editor does not read or write the player's save.

Open a level with **File > Open** or start with **File > New**. Save As lets
you keep an existing arena while experimenting with a copy. The title marks
unsaved changes, and opening another level, creating one or closing the editor
asks what to do with those changes.

New levels declare the top-level integer `"format": 1`. Files that omit
`format` load as legacy version 1; opening them preserves the original document,
and a successful save adds the version field. An explicit version must be the
integer `1`: unsupported versions and other types, including `1.0` or `1e0`,
are rejected before level fields are decoded. Version errors identify the file
and `format`; a rejected load or failed save preserves the open document,
its path and undo/redo history. Unknown fields survive the legacy upgrade.
See [the level format reference](level-format.md) for the file contract.

## Editing

### Authoring follow-up shortlist (2026-10-28)

The author authorized autonomous bounded improvements while attended gates
wait. A fresh source review ranked these independent authoring defects:

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | `Editor::nudge` changes the document during an active drag without cancelling its old preview; button release can overwrite the arrow edit. | Selected as `ED#13`: adopt the existing command gesture barrier. Small change with a native message/history regression; preserve pending-property admission and snap rules. |
| 2 | Ramp drawing is triangular, but `Editor::hit` accepts the entire rectangle, including its empty half. | Improve selection of overlapping geometry; define edge tolerance and preserve selection priority before implementation. |
| 3 | Debug `quote_word` does not protect a quoted word's trailing backslash, which the tokenizer consumes with the closing quote. | Useful for paths and saved binds; needs grammar and persistence qualification. Larger scope than the canvas fix. |

After those three fixes, the next source-based shortlist is:

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | The shared `user_stage_contract::usable_name` accepts `CON`, `NUL`, `COM1` and other Windows device stems, although Save to My Stages produces ordinary `.json` paths. | Selected as `ED#15`: reject unusable names before destination access, sharing the rule between editor and game. Small change with pure-name and private-target checks. |
| 2 | The editor validates UTF-8, but the game's default-flag level/fingerprint parses do not validate unknown string bytes or raw-NUL suffixes. | A client-owned snapshot check may align admission without an engine update. Moderate diagnostic/precedence risk; scope separately. |
| 3 | Debug `binds.reset` ignores failure to remove the saved overrides, even though ordinary writes already use atomic replacement. | A blocked private file can expose misleading success and old binds on restart. Small feedback/persistence fix; avoid rewriting the existing atomic writer. |

After completing those slices, fresh source review ranked these independent
follow-ups:

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | Empty first metadata keys produce generated paths such as `/display_name`, which the setter interprets as an absolute root field; preview maps also strip that slash and can shadow real object fields. | Selected as `ED#16`: give the empty first token an explicit JSON-pointer slash. Small generation change; qualify exact targets and native inspector/preview behavior while preserving setter aliases and escaping. |
| 2 | Left-button drag can start during a middle-button pan without ending the pan; middle-button release then cancels the still-held drag. | Bounded input usability work, but define gesture ownership and qualify both button orders before changing it. |
| 3 | Several check scripts resolve relative evidence directories with `Path.GetFullPath`, which can differ from PowerShell's current location. | Concrete tooling usability candidate; reproduce with separate private provider/process locations before selecting a shared resolution policy. |

After those fixes and tooling slices, source review found no further small
defect in the inspected gesture/property/save paths. The next additive
authoring shortlist is:

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | Canvas hit testing returns the first object in category/reverse-document order. A buried collider or decoration needs the Level Contents list even when its geometry is under the pointer. | Selected as `ED#18`: Ctrl+click cycles the existing eligible hits, with selection only. Preserve plain click and all current shape predicates; bounded native selection/history checks. |
| 2 | Drag and arrow snap round displacement, so an imported off-grid origin stays off-grid. | Selected next as `ED#19`: align the authored origin in one validated, undoable operation, preserving ordinary relative movement and slab geometry. |

#### Cycle overlapping canvas objects (`ED#18`)

**Done (2026-10-28).** Ctrl+click is a selection-only way to reach
overlapping objects. It uses the existing hit eligibility and priority: team B
spawns, team A spawns, collisions, decorations; each category visits later
document entries first. If the current selection is eligible at that point,
it selects the next hit and wraps. Otherwise it selects the first hit. One hit
selects it; no hit retains the selection. Bounds retain their ordinary outline-edit
behavior and do not join this object cycle.

One ordered hit collector owns the existing ramp/slab tolerances and mover
anchor/initial-pose tests. A mover counts once even when both rectangles hit.
Ordinary clicks retain their first-hit selection and move/resize behavior.
Ctrl+click applies pending properties through the existing guard, cancels an
older gesture, changes only selection and acquires no capture. Mouse movement
and button release after that selection cannot create an edit. The shortcut
appears in the existing Help and eight-line hint; the inspector layout
is unchanged.

The private actual-message baseline failed to select the literal buried entry.
Both presets now pass 28 Ctrl-click queries covering cycling/wrap and category
order, absent current selection, single/no hit, shape exclusions/boundaries,
mover deduplication, valid pending edits, cancelled previews, unchanged
view/source/history and no capture. Ordinary click and selected-bounds resize
controls retain their behavior. These tests run in the separate `--gestures`
process; the original aggregate DPI/font fixture is unchanged and still reports
28 transitions and GDI 21 to 21.

All three editor targets and complete checkers pass in Debug and Release,
including earlier metadata, gestures, 219 ramp queries, save guidance and four
hidden launches. Independent review and six-file source-immutability checks
pass. Invalid-property modal behavior, aggregate input-activated GDI lifetime
and attended GUI/saved-stage play (`ED#5`) remain separate qualifications.

Evidence under `out/validation/implementation-loop-20261028/`: baseline
`ed18-baseline/ac87f34221bf41508231cf6157a712cb`, both-preset batch
`ed18-candidate/2e2b7a80952e40d696ca823907656dbf`. Full checker receipts:
`out/acceptance/x64-debug/level-editor/82aadf314c82420f9ba8571ce1c2c7ba`
and `out/acceptance/x64-release/level-editor/716a3770d7034f6a990da6b6b8cf5489`.

#### Align authored origins to the grid (`ED#19`)

**Done (2026-10-28).** **Edit > Align origin to grid** aligns authored origins.
Imported geometry can start off-grid; relative dragging and nudging retain
that offset. The command aligns both coordinates to the current
1/10/25/50/100-unit grid with one undo step.
This explicit command uses `std::round` (half steps away from zero) regardless
of Shift, while existing drag/nudge controls keep their behavior.

**Qualification.** Initial Debug checks passed. Release builds and model checks passed, but its
DPI fixture stopped at `DPI transition retained its replaced owned font` before
gesture and hidden-launch checks. The failed run is retained under
`out/validation/implementation-loop-20261028/ed19-candidate/1cf1306d7d2544c3b610c62f5af65630`
(Release checker `436b5efa01ad4901a70ecba01b9b6a5b`). A separate original
Release input process also failed its old-font type assertion after the eight
alignment contexts passed (`ed19-release-input/2999ef9cea9e40028a0e84fc0ea55162`).

A bounded private lifecycle experiment captured the invalid assumption:
an unused, unselected font returned successful deletion and retained no GDI
objects, yet `GetObjectType` returned `OBJ_FONT` while `GetObjectW` returned
zero bytes. The first capture stopped the experiment; the other five cells
were skipped (`ed19-font-matrix-run/f2d206d26b364289a2cad8890ea020bf`).
No cache mechanism or exact cause of the earlier editor samples is claimed.

The two retirement assertions now query the actual logical font record with
[`GetObjectW`](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-getobjectw),
whose documented failure result is zero. Input transitions also positively
check the live replacement record, DPI height and handle identity. Default
transitions retain their existing live-record/control checks, all 28 DPI
transitions, exact GDI-count equality and complete pending/history invariants.
Production font handling is unchanged. All three editor targets and complete
checks now pass in both presets, including default GDI `21 -> 21`.
Input-activated aggregate GDI lifetime and attended `ED#5` acceptance remain
separate gates.

`LevelDocument::align_origin(const Entry&, int grid)` is the atomic model
operation. It rejects nonpositive grids and level metadata. For rectangular/TLBR
objects, ramps, bounds and moving anchors, it aligns the geometric left/top.
Spawns use their stored point; fixed slabs use the authored `slab/x,y`
endpoint. Existing size, direction, motion and metadata are preserved through
the current geometry writers.
An already aligned origin returns before mutation, preserving number types,
the saved point and redo history. Other changes use one `Impl::mutate` and its
existing validation, so refused geometry leaves the entire model unchanged.

The Edit menu command has its identifier appended after existing IDs. It is
enabled for selected geometry, including bounds/spawns/slabs and already
aligned entries, and disabled for no selection or level metadata. The command
uses the current gesture barrier and pending-property guard, then refreshes
the inspector. Help explains grid/Shift behavior and slab origins.

The old-source native baseline failed on literal off-grid coordinates before
implementation. Model checks qualify every supported grid,
positive/negative half steps, rectangle/TLBR/ramp/bounds/spawn/mover/slab
origins, no-op types/redo, exact one-step history and private save/reload.
Refusal controls include the team-spawn midpoint, blocked mover corridors and
slab endpoint limits. Eight native contexts qualify menu enablement, valid pending
edits, gesture cancellation/late releases, view/source/history and caller
restoration. Direct controller calls may catch geometry errors before the
window's modal handler; invalid pending-property dialogs remain unqualified.
The separate gesture process also retains prior overlap selection, button
handoffs and all 219 ramp-hit queries. Independent source/test review and
twelve-file source-immutability checks pass. Four hidden launches and complete
destination/save guidance pass in each preset.

Evidence under `out/validation/implementation-loop-20261028/`: baseline
`ed19-baseline/436742cf1bc148d2a313cc3e36d6f921`, final both-preset batch
`ed19-corrected/2cfde2a15b334278afafb8411425fd12`. Full checker leaves are
`out/acceptance/x64-debug/level-editor/4497dc8e8bc94ad98e566acc3a63cf64`
and `out/acceptance/x64-release/level-editor/779b68cf562d437cb6a54df051b962de`.

#### Warmed input resource lifetime (`ED#20`)

**Done (2026-10-29).** The separate `--gestures` process drives real hidden
canvas input and DPI changes, but its aggregate GDI lifetime remains
unqualified. The original unactivated 28-transition fixture has a separate
exact GDI-count gate and stays unchanged.

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | All six input helpers return their temporary GDI objects, yet `run_gestures` has no warmed aggregate counter. | Selected as `ED#20`: repeat the actual helper sequence in one hidden editor and require stable own-process GDI counts. Bounded software qualification, with private cycle artifacts and no claimed defect. |
| 2 | Invalid pending-property dialogs remain unqualified. | A later explicit modal-driving scope with owned dialog and cancellation checks. |
| 3 | Editor creation/destruction lifetime is distinct from one warmed editor. | Keep separate; do not introduce unsupported multiple-editor registration assumptions into this slice. |

**Retained experimental plan.** Run all six helpers once for warm-up, then repeat the complete sequence three
times using fresh `w`, `c1`, `c2`, `c3` subdirectories. Measure only after each
helper's locals have died. Retain the raw count, execute one successful
[`GdiFlush`](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-gdiflush)
for the calling thread, then retain its settled count. Require each checked
settled count to equal the nonzero warmed settled count. Stop at the first
assertion, flush failure or count mismatch, with no extra warm-ups or tolerance.

The first Debug qualification recorded raw counts `58, 58, 56` with every
shell-state flag true; it stopped at `c2` and did not run Release. The failed
receipt and all nine unchanged fixture hashes are retained in
`ed20-candidate/8e426c0a4f854b9b850f70ab2f63bc68`, with native evidence under
`out/acceptance/x64-debug/level-editor/1010ea5ae27c410fb29ba4f888716541`.
That decrease is not evidence of a leak or proof of batching as its cause.
The last ramp DPI restoration follows its final font-record query and may
leave Boolean GDI operations queued; the explicit barrier gives the fixture
a defined completion point.

After warm-up, one owned pen control must increase the settled count by
exactly one while held, then restore the exact baseline after successful
deletion and flushing. The pen is never selected into a DC, and RAII cleanup
retains ownership on exceptional exits. Persist the two control observations
before their assertions. A changed settled count must still fail qualification.

The settled Debug candidate also failed: `46` after warm-up, then `56` at
`c1`, with successful flushes and every shell-state flag true. The pen control
recorded `46 -> 47 -> 46`. Evidence is
`ed20-candidate/8724a26569604dcd95d8089f745b59d5`; Release was not reached.
Thus a thread flush alone does not resolve the discrepancy.

A separate private diagnostic retained the same helper/state assertions and
ran a predetermined six-cycle cap, recording aggregate mismatches without
calling them passes. Its settled counts were `57, 58, 56, 56, 56, 56`;
per-helper observations localized the later `+1` and `-2` changes to the
empty-key inspector helper. All six cycles restored their caller state and
the owned child exited with cleanup complete. Evidence is
`ed20-phase-trace-run/fb50f06dc87647c1b93cdc89d0083483`. This does not establish
a leak, a native-control cache explanation, or qualification. The next bounded
investigation was to distinguish the helper's explicit bitmap/DC cleanup from
its native inspector refreshes while preserving the failed candidate.

That private six-cycle trace completed in
`ed20-emptykey-trace-run/f5959706dea549d18e07fc429ade7ce3`: all 108 milestones,
six exact bitmap restores and all bitmap/DC deletions succeeded. All settled
counts were `56`; the earlier discrepancy did not reproduce. Observed changes
occurred during bitmap creation/drawing/cleanup, with no changes at the native
inspector milestones in this run. This narrows the observations without
identifying the objects behind the earlier count differences.

The unqualified counter candidate is retained byte-for-byte in
`ed20-retained-experiment/gesture_checks.h`, with its hash and all receipt
references in the sibling manifest. Only that new experimental `run_gestures`
replacement was removed from the working harness; all established helpers and
assertions remain. No aggregate pass is claimed. The next source step is to
define checks for the explicit owned resources and their cleanup results before
proposing another process-wide count gate. The
[`GetGuiResources`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getguiresources)
contract counts the process's GDI objects; the calling-thread flush does not
establish ownership or promise stable native-control resource usage after one
traversal. Keep any future aggregate criterion explicit and independently
qualified.

Every cycle must preserve shell source bytes/path, clean history and pending
state, selection, viewport, snap/grid, DPI/window rectangle, thread keys,
focus/capture and hidden state. Compare activation with its warmed value;
input legitimately activates this thread's hidden parent. Preserve every
existing helper assertion and all original unactivated DPI checks.

Any replacement must qualify the full editor checker in both presets and
retain source hashes, cycle artifacts and counts. A pass would establish only
the replacement's stated bounded contract;
creation/destruction lifetime, invalid-property dialogs and attended `ED#5`
acceptance remain separate. No game graphics/audio device or player data is
needed.

**Ownership receipt (2026-10-29).** The source step above is complete for the
only GDI objects the input fixture explicitly owns. `MechanismBitmap` keeps a
ledger of the bitmaps it created and still holds. It also counts failed cleanup
calls: restoring the original selection, `DeleteObject` and `DeleteDC`.
Its constructor refuses a failed bitmap selection. `run_gestures` keeps the
six helpers in their unchanged order on one hidden editor. After each helper it
writes a `bitmap-ownership.csv` row, including an `aborted` row if the helper
throws, and then asserts. No fixture bitmap may outlive its helper, and no
cleanup call may fail. `empty_key_properties` and `ramp_hits` are the only
helpers that draw, and each must record at least one bitmap.

Both presets pass the full editor checker with 19 fixture bitmaps. The earlier
DPI and paint-failure count gates are unchanged. Two temporary negative
controls each failed at the intended assertion in `empty_key_properties`, and
the sources were restored byte for byte afterwards. One control leaked a bitmap;
the other called `DeleteDC` before the checked cleanup. Evidence is
`out/acceptance/x64-debug/level-editor/7b798d8d6dd743c98390a35707ce58d5`,
`out/acceptance/x64-release/level-editor/5df1d18b3af3445d97f35acae5a2099f` and
the `ed20-*` logs under `out/validation/road35/`.

Two consecutive full Debug checker runs on the final source failed before any
receipt assertion. The failure was the existing `overlap_selection` check that
the probe thread's 256-byte keyboard state survives `MK_CONTROL` messages.
Their receipts recorded that helper as `aborted` with no bitmaps. An earlier
Debug run, a run with a temporary per-key diagnostic and the final run passed.
The diagnostic build then passed 12 standalone `--gestures` runs with no
differing key. The cause is unexplained; nothing shows that the
receipt causes it, or that it is an editor defect.

This qualifies fixture-owned bitmap and DC cleanup only. Pens, brushes and fonts
that production drawing selects into a fixture DC are not counted. Neither are
native-control resources or process-wide GDI stability. The archived aggregate
candidates and traces above stay unqualified, and no aggregate gate is
proposed. Editor creation/destruction lifetime remains separate, as does
attended `ED#5`.

#### Paint exception cleanup (`ED#21`)

**Done (2026-10-29).** `canvas_message(WM_PAINT)` formerly released its
compatible bitmap/DC and calls `EndPaint` only after `paint` returns normally.
`paint` likewise restored the caller's selected font only on normal return.
Drawing allocates strings and vectors and converts UTF-8 labels; an exception
escapes to `canvas_proc`, which reports it and continues using the editor.
The resources must be released before that handler runs.

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | Paint cleanup is skipped on a concrete throwing path. | Selected as `ED#21`: scope the paint session, buffer and selected font so unwinding releases them. Bounded robustness repair with a deterministic malformed-label regression. |
| 2 | Empty-key input changes process-wide GDI counts without an identified owner. | Continue the separate bounded `ED#20` diagnostic; do not attribute those successful cycles to this exception defect. |
| 3 | Invalid pending-property dialogs remain unqualified. | Keep the explicit modal-driving and cancellation scope separate. |

Exercise the production drawing methods through the existing acceptance
friend, using an invalid UTF-8 label in private in-memory draw data. Call the
canvas message method directly so the test catches the exception without
opening its outer error dialog. Observe the caller's selected font and
repeated failed buffered paints; retain the failing baseline before the fix.
Preserve the original document, source, controls and input state. Keep this
after the existing unactivated DPI checks, and preserve their count gate.

Small local guards now restore the borrowed font and release the selected
bitmap, compatible DC and paint session during unwinding. The paint owner is
fully constructed before later acquisitions can throw, and partial buffer
creation failures use the same cleanup. Every attempted `BeginPaint` has its
scoped `EndPaint`; drawing and the outer error policy stay intact.

The unchanged-production Debug baseline reproduced both failures. The direct
paint retained the editor's font in the caller's private DC. Three buffered
exceptions increased settled GDI counts from `22` to `26`, `30`, then `34`;
the following valid recovery paint still recorded `34`. Every controlled
exception and caller-state check succeeded. The original 28-transition DPI
fixture and 45 mechanism contexts passed before the ownership assertion failed.
Evidence: `ed21-baseline/b6512abc4a8440e094c5d5c051de3093`, with native CSVs at
`out/acceptance/x64-debug/level-editor/8e5b350655a64e04acace476c2035e1b/dpi-window`.

All three editor targets and the complete checker pass in Debug and Release.
Both controlled exception fixtures restore the caller's exact font and retain
GDI `22 -> 22` through all three failures and recovery. All caller/source/history
flags remain true. The original DPI gate remains `21 -> 21`, and mechanism,
gesture, save-guidance and four hidden-launch checks pass in each preset.
Independent source/test review, 14 unchanged source hashes and owned-process
cleanup checks pass. Evidence: `ed21-candidate/cb444061cf02453491324e3606a4de63`;
full checker leaves are `34b7de2ae6d142bdb08b0c933fb46cbd` under Debug and
`ad957d94c8374a97bac5ae0e8cc38636` under Release's `out/acceptance` tree.
Forced Win32 acquisition failures and the modal error UI were not exercised;
partial cleanup and paint-session pairing also received source review.
`ED#20` and attended `ED#5` remain independent gates.

#### Invalid pending-property refusal (`ED#22`)

**Done (2026-10-29).** A separate native `--pending` acceptance process now
qualifies invalid numeric text across Apply, Save, Save As, Save to My Stages,
object-list selection, Home navigation, Close/Cancel, Close/Yes followed by the
validation error, and a controlled expired Close prompt. This adds evidence for
the existing guards; no production editor behavior changed.

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | Invalid pending text reaches a modal error before the guarded action proceeds. | Selected as `ED#22`: qualify real Apply, save, selection, navigation and Close refusal paths against private stage files. Direct data-preservation value; isolate modal driving and its deadline in a separate acceptance process. |
| 2 | The input fixtures' explicit bitmap/DC cleanup results are unchecked. | Continue `ED#20` with exact ownership receipts before proposing another aggregate resource gate. This does not establish a production leak. |
| 3 | External capture loss clears drag/pan but leaves the resize flag. | Investigate the complete capture-loss contract separately; no lost-edit defect has been established. |

The existing acceptance friend prepares real undo/redo history, a selected
spawn coordinate and pending `true` text. Actual window messages exercise each
guard. All nine contexts preserve the private stage bytes, document/path,
history, selected rows, pending text and text selection, viewport, thread keys,
window/font state and empty private profile. Capture and gesture flags remain
idle. Both CRT and Win32 profile views are restored after the fixture.

One timer lives for the whole fixture. Its nonthrowing callbacks use fixed
storage and inspect only dialogs with the exact current PID/thread, editor
owner, class, caption and body. Yes and Cancel require their enabled button
IDs; the exact finite-number error requires its single enabled acknowledgement
button. Unexpected dialogs receive no affirmative answer. Each action has a
ten-second deadline; the controlled expiry is already expired and must cancel
without saving. Cleanup clears the callback context, kills the timer and checks
that no owned dialogs remain. The new native invocation alone has a 120-second
private Job Object deadline, merged logs and a terminal cleanup receipt.

Two private diagnostics corrected fixture assumptions before qualification.
The error's sole enabled OK control had native ID `2`, with no ID `1`; using its
actual control follows the existing save-guidance checker. Invalid and valid
dirty Close/Cancel controls both returned focus from the edit field to the
hidden editor window in the local qualification. Validation errors still require
exact value-field focus. Hidden Cancel and expiry permit NULL current-thread
focus or focus in the editor tree; non-null foreign targets still fail.
Raw focus/capture values and the separate expectations remain in the receipts.
The original failed runs and diagnostic-only builds are retained under
`ed22-button-diagnostics-run/9d5cecce4a4e4a60ba3a5389df8ec296` and
`ed22-focus-diagnostics-run/9d574280804244f28fe432564a23357a` in the validation
cohort; they are not acceptance passes.

All three editor targets and complete checkers pass in Debug and Release under
PowerShell 7, and the complete Debug checker passes under Windows PowerShell
5.1. The checker explicitly imports its own host's standard Utility module so
later file-hash checks remain available after nested native-probe setup. Each
run has nine passing action rows, ten verified dialog rows, timer cleanup,
both profile-view restorations and an owned zero exit with cleanup confirmed.
All 22 guarded source hashes remain unchanged. Existing document, 28-transition
DPI/font, paint-failure, gesture, save-guidance and four hidden-launch checks
also pass; the original DPI fixture retains GDI `21 -> 21`.

Evidence: `ed22-candidate/93e462a4f2d14a3f9af471b54303c9a6` under
`out/validation/implementation-loop-20261028/`. Full checker leaves under
`out/acceptance/<preset>/level-editor/` are Debug
`c560caab08d24c93a9a2f40122d9c7c6`, Release
`c5e565c500704b75afe66adea3addd30`, and PowerShell 5.1 Debug
`e442f2c3f97c4f0ea2196b13f80a987e`. No foreground input, player files,
gameplay or engine update was involved. Modal appearance and attended `ED#5`
remain separate, as do the archived `ED#20` aggregate measurements.

Publication CI exposed the local focus assumption. Both Windows jobs in
[run 290](https://github.com/matt439/ColourWars/actions/runs/40824582511)
recorded NULL focus/activation after Cancel while every other state predicate
passed. The failure logger retains the CSVs and terminal process receipt.
[Run 291](https://github.com/matt439/ColourWars/actions/runs/40826079863), at
`bcb6b0eb`, added one valid-dirty Cancel control after the original failed row
was flushed. Its Debug control also returned NULL focus/activation, with all
thirteen state flags and the strict owned Cancel response passing. The original
assertion still failed; this diagnostic is not an acceptance pass.

`confirm_discard` vetoes closing without calling `SetFocus`; validation errors
explicitly refocus the edit. [GetFocus](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getfocus)
permits NULL when the calling thread has no focused window. The correction
qualifies that hidden-fixture outcome without changing product behavior or
repairing focus after the observation. The temporary control is removed; all
error-focus, data/history/selection, capture/gesture, modal and deadline checks
remain. Debug and Release native targets and full PowerShell 7 checkers pass;
the full Windows PowerShell 5.1 Debug checker also passes. Their leaves are
`8a3961bd554449e582e7da85ccf17544`, `6f0937a736b047099ea3d3dbe4a73642` and
`c070f0c237be41ff9d1b6ab0dcf44fa0`. Attended `ED#5` remains separate.

#### Empty-key property targets (`ED#16`)

**Done (2026-10-28).** Unknown metadata may contain an empty object-member
name. The property generator formerly emitted `/display_name` for
`"":{"display_name":"nested"}`; applying that generated path edited the actual
root field instead. Object-relative properties had the same alias, and the
preview map could mistake metadata `type` or `collision_type` for real fields.

An empty first token now gets an explicit `/`, producing `//display_name` for
its child. Ordinary generated paths, bare/absolute setter aliases, existing
`~0`/`~1` escaping, unknown metadata and arrays are preserved. No setter API or
gameplay change was needed.

Both public-model and actual hidden-inspector baselines reproduced wrong-leaf
edits before the fix. Model checks now cover root/object targets, nested empty
names, scalar empty leaves, escaped-name siblings, unchanged runtime fields and
arrays, complete history and private save/reopen. Two actual inspector edits
retain the real names and preview type; two literal GDI body samples and hit
points preserve the rectangle beside ramp-shaped metadata. Native history and
Save/reopen retain the exact two edits and saved clean point.

All three editor targets and the complete checker pass in Debug and Release,
including prior gestures, ramp hits, save guidance and four hidden launches.
The original unactivated 28-transition DPI/font fixture still reports GDI
21 to 21. New inspector input remains in the separate gesture process and does
not qualify its aggregate GDI lifetime. Independent review and eight-file
source-immutability checks pass. Attended GUI/saved-stage play remains `ED#5`.

Evidence under `out/validation/implementation-loop-20261028/`: failing model
and native baseline `ed16-baseline/a3aa67efddf3457191620ea144c8477e`, final
batch `ed16-candidate/242b010b86634c48b615eb77ee134fdd`. Full checker receipts
are `out/acceptance/x64-debug/level-editor/cf4ae5c17ae54adb89d413ae48ceb86e`
and `out/acceptance/x64-release/level-editor/edd33c1f31d64d4ea6c98cb3c1b1d30d`.

#### Mouse-button gesture ownership (`ED#17`)

**Done (2026-10-28).** This ranked authoring follow-up fixed a lost canvas
edit when a left drag took over a middle-button pan. Left down reacquired the
pan's existing capture after setting its new drag flag; the native baseline
already observed cleared gesture flags at that point and a lost final edit.
Middle up also unconditionally cancelled a newer left preview. Middle down
already cancelled an older left drag, so the handoff was asymmetric.

The latest admitted button-down now owns the gesture. Left down cancels the
older gesture after the existing pending-property guard; middle up ends only
an active pan. Releasing an older button is harmless, and releasing the owner
ends the gesture without resuming another held button. An empty-space left
click cancels the previous pan and leaves the canvas idle. Property admission,
snap, resize and ordinary drop behavior are unchanged.

The actual private hidden-canvas baseline reproduced a lost final rectangle
with cleared gesture flags while capture remained on the canvas. Ten native
contexts now pass both button/release orders,
move/resize, empty-space and valid pending-property handoffs, ordinary controls,
capture, exact document history, source bytes and caller restoration. These
checks remain in the separate gesture process. All three editor targets and
the complete checker pass in both presets, including the prior metadata,
ramp, save-guidance and four hidden-launch checks. The original 28-transition
DPI/font fixture remains unchanged and reports GDI 21 to 21 in both presets;
input-activated aggregate GDI lifetime remains unqualified.
Invalid inspector text remains guarded before cancellation; its modal error
and external capture-loss lifecycle are separate scopes. Attended GUI/play
acceptance remains `ED#5`.

Independent review and four-file source-immutability checks pass. Evidence under
`out/validation/implementation-loop-20261028/`: failing native baseline
`ed17-baseline/04e0fd1ccde64c25bc6e9b1bf71797c7`, final both-preset batch
`ed17-candidate/772dbc04693a4b5a8062f8d7a95873c6`. Full checker receipts are
`out/acceptance/x64-debug/level-editor/5652c47762c747ba81f4fabdcf6c44ea`
and `out/acceptance/x64-release/level-editor/c7a3f648b379493b881b4947abf635fb`.

#### Reserved stage filenames (`ED#15`)

**Done (2026-10-28).** The stage id becomes a Windows filename, so the shared
editor/game contract now enforces the documented
[reserved-name rules](https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file)
alongside the existing ASCII/length/leading-character rules. It rejects `CON`,
`PRN`, `AUX`, `NUL`, `COM1`-`COM9` and `LPT1`-`LPT9` without case; their
`.json` extension does not make them ordinary files. Non-ASCII spellings
remain covered by the existing ASCII-only rule. Explicit ASCII folding keeps
the new match independent of locale. Valid neighboring identifiers, name
collisions, source registration and folder policy are unchanged.

The pure old-source contract first reproduced accepting `CON`, before any
reserved path access. Tests now cover 22 reserved stems in three ASCII cases,
17 valid neighbors and prior character/length rules. Sixty-six actual editor
target refusals preserve an absent ordinary folder, private source and full
history; only safe `console_map.json` performs save/reopen. The shared refusal
explains the Windows restriction. No reserved destination is opened, written
or inspected, and no player's stage folder is involved.

Both game/editor builds, consumer user-stage checks, the full editor checker
and native stage-scan details pass in both presets. Native checks include the
longer shared refusal text, pagination and real menu transitions. Independent
review and source-immutability checks pass. This does not establish attended
wording, editor play or hardware acceptance.

Evidence under `out/validation/implementation-loop-20261028/`: baseline
`ed15-baseline/98db2c0ac3f74994a601e83d9f8255f2`, final both-preset batch
`ed15-candidate/dbeb4001dff44ec49b4cda9caa3757ee`. Full editor runs under
`out/acceptance/<preset>/level-editor/`: Debug
`c912cc96102b452dac0628b5f08d090d`, Release
`2bda6f3f55fb406e8cc19627ebf0b816`.

#### Arrow edits during canvas gestures (`ED#13`)

**Done (2026-10-28).** Actual hidden-window messages first reproduced a mouse
release overwriting the preceding arrow edit. Nudging now cancels an active
drag, resize or pan before pending-property admission, matching other editor
commands. A late release cannot commit its discarded preview.

Native checks cover move/resize/pan interruption, capture and selection,
ordinary drops, grid/Shift nudges, valid pending properties, exact undo/redo
and unchanged private source files. All three editor targets build and the
full editor checker passes in Debug and Release. Evidence under
`out/acceptance/<preset>/level-editor/`: Debug
`e9675199e60d4271918c9dc0c8f76d10`, Release
`4d11991322ff4eca89b3eac164269f18`. The original failing case is retained in
`out/validation/implementation-loop-20261028/ed13-baseline/8c1b0ca3f3304ee1b98da6ef2ed52bea`.

Gesture qualification runs in a separate `--gestures` invocation of the same
native acceptance target, with its own scratch/log. Canvas
[`SetFocus`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setfocus)
also activates the hidden parent; this host could not restore the preceding
null activation state. Mixing those lifetimes changed aggregate GDI counts.
The default DPI check therefore retains its original 28 transitions and exact
21-to-21 resource equality, with an added assertion that every replaced font
is released. Aggregate GDI stability after input activation remains unqualified;
no production resource workaround was added. Independent review passes.
The attended editor/play acceptance remains `ED#5`.

#### Ramp body selection (`ED#14`)

**Done (2026-10-28).** The old-source native baseline reproduced an empty
ramp corner intercepting the block beneath it. Ramp selection now follows the
drawn screen-space triangle, with a three-logical-pixel edge tolerance.
Collapsed triangles retain only finite edge/endpoint targets. Category and
authored-order priority, selected resize handles, ordinary rectangles, slabs
and mover poses are preserved.

Both presets pass the full editor checker and all three editor builds.
The isolated canvas-input mode checks 18 direction/DPI/view contexts,
219 immutable queries and 36 independent actual GDI body samples. It covers
edge/end-cap limits, legal zoom/clipping collapse, overlap/spawn/slab priority,
resize handles and exact history/source preservation. Degenerate geometry is
isolated from unrelated spawn targets; full-scene priority has separate checks.
Input-activated DPI changes also assert disposal of each replaced owned font.
Independent source/oracle review passes. Aggregate GDI stability after input
activation and attended editor acceptance remain outside these checks.

Evidence under `out/acceptance/<preset>/level-editor/`: Debug
`c330ea6646f2401a871653f1ed28636a`, Release
`a1a256d6df694473bbe5ed870b99ede7`; each retains CSV queries and six BMPs.
The failing baseline is
`out/validation/implementation-loop-20261028/ed14-baseline/c37833dd52514e31a373b22a3ee62eeb`.

Select an object in the list or click it on the canvas. Drag its body to move
it, or drag the handle at the bottom-right corner to resize it. Ramp editing
moves or scales all three vertices together. Spawn markers can be moved;
their boxes use `player.size` from source `game/content/tuning.json`.
Fixed scored slabs move without a resize handle; their footprint is fixed.
Arrow keys cancel an in-progress mouse gesture and nudge the committed position.
Ramp selection follows the triangular body, so its empty corner lets you
select an object beneath it.
Ctrl+click cycles through overlapping objects without starting a drag. Repeated
clicks wrap through the same hit order; empty space retains the selection.

Use the palette to add paintable platforms, solid structures, jump-through
platforms, [horizontal moving paintables](#moving-structure-authoring-gapd5),
left/right ramps, [fixed scored slabs](#fixed-scored-slab-authoring-gapd5)
and decorative visuals. Team spawns and the four
bounds rectangles are also selectable and editable. The level entry exposes
the stage name, menu icon and sound settings. New levels use the replacement
`paint_exe` bank, `music.close_quarters` and its matching ending sting. Omit both
music name and volume for a music-free arena; `end_sting_name` and
`end_sting_volume` are independently optional as a pair. Movement Course omits
both pairs. Music and ending-sting gains must be 0..1. The editor warns about a bank,
music or sting name the bank does not define ([Resource names](#resource-names));
it does not audition them.

The property inspector lists individual fields for the current selection.
Choose a field, edit its value and click **Apply**. Numbers use a decimal
point; boolean fields use `true` or `false`. Text fields are plain text.
An invalid change displays an error and keeps the preceding valid document.
Enter also applies the current field. Apply or revert pending text before
choosing another property row. Selecting another object or saving applies
the pending value automatically; an invalid value blocks that action until
it is corrected or discarded with Escape or **Revert**.
Use the inspector for exact coordinates, paintable faces, sprite-sheet and
frame names, colours, and other object properties.

Imported metadata containing an embedded NUL character is shown as escaped,
read-only text. The inspector keeps those values and keys intact when you
save or change selection. You can select and copy the preview; edit the JSON
file externally to change it. String previews use quotes and escapes such as
`\u0000` and `\\`, while numbers and booleans keep their usual form.

### Materials

Collision objects carry an optional `material`, the terrain-atlas look the game
draws them with ([Materials](level-format.md#materials)). The palette gives each
new collision object its usual one: a solid `Structure` is `ground`, a
jump-through `ruler`, a paintable platform `pencil` and either ramp
`set_square`. Decorative visuals get none. Duplicates keep their material.
Fixed scored slabs require their dedicated `eraser` material.

The inspector's `material` row lists, in its label, the values the object's
collision type allows, such as `material (ground, pencil, eraser, paint_tin)`
on a paintable platform. It is a text field checked on Apply: an unknown name,
or one the collision type does not allow, is refused like any other invalid
edit. An object without a material still shows the row, empty; applying a value
adds the field, as the Random setting does, and undo restores the omission.
Applying an empty value removes the field, returning the object to the legacy
stretched sheet/frame draw, except on a fixed slab whose material is required.
The inspector adds no other missing field, and
never a material on a decoration. Changing `collision_type` to one the current
material does not fit is refused too: change or clear the material first.
The canvas stays schematic and does not draw materials; play the stage to see
them.

| Action | Control |
|---|---|
| Select and move | Left click and drag |
| Select overlapping objects without moving them | Ctrl+click repeatedly |
| Resize selected geometry | Drag its bottom-right handle |
| Pan | Middle mouse button and drag |
| Zoom | Mouse wheel |
| Fit the arena in the viewport | Home in the canvas, or Fit arena button |
| Frame the current selection | F in the canvas or object list |
| Return to the match overview | Match view button |
| Move selection by the grid spacing | Arrow keys in the canvas |
| Grid spacing | Choose 1, 10, 25, 50 or 100 |
| Temporarily bypass grid snapping | Hold Shift while dragging |
| Duplicate selection | Ctrl+D |
| Delete selection | Delete |
| Undo / redo | Ctrl+Z / Ctrl+Y |
| New / open / save | Ctrl+N / Ctrl+O / Ctrl+S |
| Save as | Ctrl+Shift+S |
| Save to My Stages ([play without a rebuild](#play-the-level)) | Ctrl+M |

The document keeps undo/redo history for edits. A successful save marks the
current revision as saved; undoing back to that revision clears the unsaved
indicator. Failed loads and saves keep the current document and its history.
Saves replace the destination only after writing the complete new JSON.
The initial view uses the level's zoom-out finish bounds.

## Blockout aids

**View > Blockout aids (reload tuning)** toggles the guides. They start enabled;
turn them off and on to reload source tuning after editing it outside the editor.
Document edits also refresh tuning and diagnostics. These reads do not edit the
level or the player's save. A missing or invalid aid file displays a diagnostic.

- **Cyan edges** show exposed scoring faces. Flat structures, including
  jump-through platforms, bury faces reached from their outward side. Ramps and
  decorations do not. Moving anchors never become permanent static blockers;
  their own scoring faces stay local to the body. The editor and game share the interval calculation in
  `game/paint_face_geometry.h`. With aids off, cyan shows authored faces instead.
  Fixed slabs clip their diagonal faces and neighbouring flat faces against
  the actual adjacency polygon, using the shared slab paint-geometry helper.
- **Scoring area** shows the whole level and the part inside the selected
  rectangle. Select an object or one of the bounds rectangles to choose a
  region; selecting the level shows only the total. Area clips the inward paint
  strips using current `paint.thickness`, in world units squared. Corners and
  overlapping strips contribute separately, as in runtime scoring. It measures
  available scoring surface, without assigning teams or predicting reachability.
  Movers appear at their initial phase, labelled in the summary. Their local
  strip dimensions and area are measured before translating the displayed
  positions, so changing phase does not change the available score.
  Slabs retain local face areas and display actual world quads; selection
  regions clip those quads, including their empty bounding-box corners.
  During a drag, measurements and scoring edges wait for the committed drop.
- **Purple/gold boxes** show the recorded tap/held jump envelope in both
  directions from the top centre of the selected geometry, or the feet of a
  selected spawn. They bound maximum rise and same-height horizontal travel;
  the top corners are not a trajectory. The data and matching movement-tuning
  snapshot live in `editor/movement-baseline.json`, from the actual Player/Scene
  [measurements](polish-movement-course.md#measured-baseline-and-current-evidence).
  These assume full running speed and a fixed timestep, exclude collisions and
  acceleration from rest, and do not guarantee a gap or landing. A changed
  movement snapshot hides the boxes and asks for new measurements; spawn sizes
  and scoring diagnostics still follow current tuning. After changing movement
  code, rerun the movement acceptance measurement before updating the baseline.

`ED#1` implemented on 2026-10-15. Debug and Release editor document checks cover
overlapping blockers, buried/touching faces, ramps versus flat collision classes,
disabled faces, clipped region area, undo and changed-tuning invalidation. Hidden
GUI checks select a spawn, toggle/reload the aids and frame the selection. Both
game consumer acceptance suites also pass with the shared geometry, reproducing
the baseline jump measurements. These checks do not close the manual canvas or
saved-stage play checks in `ED#5`.

## Resource names

The game refuses a stage at startup when a name it resolves is missing
(`LevelLoadedInfo::validate_content` and `LevelObjectBuilder::validate_objects`).
The editor checks the same names against the same source files, so a typo shows
while you edit rather than at the next game launch:

| Field | Must name |
|---|---|
| `icon_frame` | a frame in `sprite_sheet_1` |
| `sheet_name`, when the object uses generic sprite fields | a `sprite_sheet` listed in `game/content/manifest.json` |
| that object's `frame_name` | an entry in that sheet's `sprite_frames`; an animation strip is not a frame |
| fixed slab's dedicated material | sprite sheet `terrain` and its `swatch_eraser_fill` frame |
| `sound.sound_bank_name` | a `sound_bank` listed in the manifest |
| `sound.music_name`, `sound.end_sting_name` | an effect of that bank: every wave when `create_effect_instance_for_each_wave` is true, plus each `sound_effect_instances` name |

`colour` is not in the table because it is already a structural check: only the
game's grey names and `RRGGBB` greys are accepted, and anything else is refused
like any other invalid edit.

These checks only warn, because a stage is often saved halfway through adding its
art. The status line counts the unresolved names and shows the first one. Save
and Save As list them and ask before writing; No is the default. The editor
rereads the manifest, its sheets and its banks from source `game/content/` on
each refresh, as it does tuning. If the manifest or a listed file cannot be read,
the status line says *Resource names unchecked* and only the readable names are
checked. Audio names are checked against the tracked `paint_exe.json`
definition, not the `.xwb`. They are still checked when the bank is absent, even
though the game then substitutes its silent fallback, which accepts any name.

The game's startup validation is still the authority. The editor's catalog
(`editor/content_catalog.cpp`) is a second reader of the sheet and bank formats,
so the game-side `level_format` consumer case holds it to the engine's loaders on
the staged content: every frame it lists resolves in the engine's sprite sheet,
every animation strip it leaves out is refused as a frame, and its effect names
equal the engine's sound-bank definition.

`ED#3` implemented on 2026-10-17. The editor document checks cover every
shipped level, the template and new-level defaults without warnings. They
mutate each field in the table, including a strip name used as a frame and a
frame borrowed from another sheet, and expect exactly one line naming the
field and value. They also check undo, sheet scoping, the wave rule and a
partial or missing content folder. A hidden GUI launch opens a level with an
unknown `icon_frame` and reads the warning from the status line. The save
prompt is a modal box, so it belongs to the manual pass in `ED#5`.

## Play the level

For an existing stage, save its source JSON, rebuild the game and choose the
stage from the game menu. The build copies the revised file beside the game.

To play a level without rebuilding, choose **File > Save to My Stages...**
(Ctrl+M). The save dialog opens in `%LOCALAPPDATA%\ColourWars\stages\`, beside
the save, which is the folder the game reads player-made stages from. It
offers the open file's name. A copy of a shipped stage is offered `my_<name>`,
and a new level `my_stage`. The editor then applies the game's own rules
(`game/user_stage_contract.h`, shared with the game's scan). The name must be
1 to 64 ASCII letters, digits, `_` or `-`, not starting with `-`. It may not
be a shipped stage's name in any case; the names come from the source
manifest. The file must go directly in that folder: use Save As for any other
folder. If one of your stages already has that name (in any case), the editor
asks whether to replace it; No is the default. The replacement keeps the
existing file's spelling. The folder is created only by this command. If you
cancel, or the name is refused, a folder the command created is removed again
while it is still empty. After the save, the open document edits the saved
file, so Ctrl+S saves there too. Without `LOCALAPPDATA` the command refuses:
the game would then read `save/stages` beside its own executable, which the
editor cannot know. Like the other saves, it never touches the player's save.

The game reads that folder at startup and again each time Stage Select opens
from the main menu. A stage saved while the game runs joins the list, and an
edited one plays its new version. You can also copy a file there by hand. A
user stage appears in Stage Select after the shipped stages, with "Built by
you." in the briefing, and `+stage <name>` starts it from the Debug command
line. It is never a Random candidate. The game checks it as it checks a
shipped stage. A file that fails prints one `user stage refused: <file>:
<reason>` line on stderr and is left out; the game goes on without it. Main-menu
Stage Select also shows an issue count: Down while browsing opens the retained
filename/reason details, with issue/page controls and a return to the same
arena choice. See
[User stages](feature-roadmap.md#user-stages-road16).

To ship a stage with the game, register it instead. A new file in
`game/content/levels/` is not automatically a menu stage. To register it:

1. Save it as `game/content/levels/<name>.json`.
2. Add `<name>` without the extension to the `level` asset group's `names`
   in `game/content/manifest.json`.
3. Add `levels/<name>.json` to `COLOURWARS_RUNTIME_ASSETS` in
   `cmake/runtime_assets.cmake`, shared by development staging and Release.
   Configure checks that these two lists agree.
4. Configure/build again, then start the game and choose the new stage.

The file's `display_name` is the stage's label on the stage-select page, which
also draws a live preview of the arena itself.
Stage names use the glyphs supported by the game's compiled fonts; unsupported
characters such as emoji are rejected before saving.
`icon_frame` is required legacy icon metadata that no screen draws today; it
must still name an existing frame in `sprite_sheet_1`. Object sprite-sheet
and frame names and music names must also exist in the game's assets. The
editor warns about any that do not ([Resource names](#resource-names)); the
game's startup validation resolves the actual resources.

An optional top-level `include_in_random` boolean defaults to `true`. Set it to
`false` in the level entry's property inspector for an experimental course that
should remain explicitly selectable without appearing in Random matches. When
the JSON omits the field, the inspector shows its default `true` without changing
the document. Applying a changed value adds the field; undo restores its omission.

Each team must retain at least four spawns inside `out_of_bounds`. The other
bounds control camera limits and the match's zoom-out presentation. Resizing
`out_of_bounds` must keep every spawn inside it. Objects outside those bounds
can be intentional boundary geometry and are allowed.

## Local implementation backlog (2026-10-28)

These independent source slices need neither pads nor an author decision.
Their entries record implementation status and document/native qualification;
actual GUI comfort and saved-stage play remain the attended `ED#5` review.

### External-change save protection (`ED#7`)

**Done (2026-10-28).** `LevelDocument` retains the exact bytes read by a
successful load or written by a successful save, separately from normalized
JSON used for dirty-state comparison. Same-destination Save compares those
bytes before creating its sibling temporary and again after writing, flushing
and closing it, directly before replacement. Changed, missing or unreadable
files are refused with a diagnostic pointing to Save As. Target identity
handles relative/case variants and existing equivalent files.

A refusal retains external bytes, open JSON, path, dirty state and complete
undo/redo history, and removes any temporary. Save As preserves the open work
elsewhere and establishes its new baseline; a failed Save As retains the
previous one. The existing atomic replacement remains unchanged. The checks
detect external writes visible at either comparison; they do not introduce
file locking or an atomic filesystem compare-and-replace.

The isolated `level_editor/save_guard_checks.cpp` covers raw versus normalized
source, unchanged saves, case variants, hard-link aliases, competing editors,
same-length external edits with retained timestamps, deletion, exclusive-read
failure, complete history, Save As and latest-success retries. Existing
write/replacement-failure checks remain in `editing_checks.cpp`.

Independent review and both builds pass. Full editor acceptance passes in
Debug and Release, including hidden explicit-level launches, controls/status
checks and graceful shutdown. All files and profiles are isolated. Attended
GUI/saved-stage play remains `ED#5`; no player save is involved.

### Per-monitor editor DPI (`ED#8`)

**Done (2026-10-28).** The editor requests per-monitor
v2 awareness, with a per-monitor v1 fallback. Fonts, controls, spacing, toolbar
wrapping and minimum sizes use one 96-DPI layout. Initial and minimum windows
are clamped to the monitor work area; compact layouts keep property actions
reachable before the optional hint. `WM_DPICHANGED` adopts the suggested window
rectangle, replaces the shared control font before deleting its predecessor,
and relays out the controls.

A DPI transition cancels an uncommitted pointer preview without applying pending
property text, fitting the canvas or editing the document. World coordinates,
view, selection and full history remain intact. The game display-policy decision
(`POL#34`) and Labrador pin are independent.

`window_layout_checks.cpp` checks 96/144/192 geometry. The separate hidden,
in-process `colourwars_editor_dpi_acceptance` target sends actual DPI messages
with valid same-process rectangles and checks font/control/minimum geometry,
suggested placement, pending text and selection, coordinate/hit conversion,
undo/redo and GDI resource counts. Actual mixed-monitor comfort remains `ED#5`.

All three editor targets build in Debug and Release. Complete editor checks
pass in both, including 28 DPI transitions and stable GDI counts (21 to 21),
the document model and hidden launch/status/close checks. No monitor
arrangement or gameplay display policy is accepted by those synthetic messages.
CI explicitly builds all three targets before its `-SkipBuild` editor check.

### Paint-mechanism canvas guides (`ED#9`)

**Done (2026-10-28).** Paint gates have a schematic
lintel/posts and `A / B` label. Paint ferries have a `PAINT FERRY` plaque with
explicit A/B letters and opposing arrows from the authored `team_a_heads`.
Ordinary erasers and automatic cycle movers have no mechanism plaque. These
marks read current inspector properties and follow initial/provisional poses;
they describe authored behavior without simulating ownership, permission or
movement. Runtime cues remain `ROAD#26`/`ROAD#27`.

Screen-space plaques scale with DPI. Actual GDI font line height controls row
spacing, arrow centers and panel height, so the labels and schematic stay apart.
They do not extend hit geometry or alter travel/standing envelopes.

`mechanism_guide_checks.cpp` covers both headings, inspector/history,
duplication and save/reopen. The native DPI target draws actual selected,
unselected and provisional canvas objects at 96/144/192 DPI, comparing text
and stroke pixels with independent expected guides. It retains three BMP
sheets and a CSV, with font extents, separation, history and hit/envelope checks.
An attended look remains `ED#5`.

Both presets pass 45 native contexts and 90 actual canvas raster passes,
including label/stroke containment and separation, selected/provisional
translation and unchanged complete history, hit and envelope behavior.
Review included the retained 96, 144 and 192 DPI sheets.

### Destination-aware save guidance (`ED#10`)

**Done (2026-10-28).** Save and refresh classify the
current destination through the shared folder/name contract. A file directly
in My Stages keeps the main-menu Stage Select/no-rebuild instruction after
ordinary Save or refresh. Registered source files need a rebuild; new source
filenames also need registration. Elsewhere/nested destinations say they are
not discovered. Unavailable manifest/folder context gives qualified guidance.
Destinations, registration and Random policy are unchanged.

The native check exposed a dialog boundary bug: cancellation could leave the
process working directory in the newly created stage folder, preventing its
empty-directory cleanup. The
[legacy dialog calls](https://learn.microsoft.com/en-us/windows/win32/api/commdlg/ns-commdlg-openfilenamew)
now restore the previous directory before returning. The initial folder uses
an absolute native path; the suggested basename, chosen destination and
existing validation/overwrite rules stay the same.

The model checks cover each destination and unavailable context. The hidden
window helper targets only its own HWNDs under scratch `LOCALAPPDATA`: it saves
to My Stages, edits and ordinarily saves, refreshes, cancels dialogs, refuses an
externally changed file and uses Save As elsewhere. A source-level launch is
read-only. File hashes and the absence of a player save bound the checks.

The complete checker passes in Debug and Release. Each native guidance run
records seven status transitions and three strict cancellation/failure
invariants, exact chosen-file writes, graceful exit and no player save.
The helper supplies a complete scratch profile (Local/Roaming AppData and
Desktop), verified to resolve there through the Shell; it restores the parent
environment afterward. This keeps an unavailable real Desktop from becoming
a fixture dependency.

### NUL-bearing metadata preservation (`ED#11`)

**Done (2026-10-28).** Valid unknown JSON metadata such as
`"note":"keep\u0000suffix"` retained all bytes in the document, but the
inspector passed its decoded value to a
[null-terminated Win32 text message](https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-settext).
The truncated control text appeared pending and was silently auto-applied by
an untouched Save or a selection change. NUL-bearing property keys also reached
a null-terminated JSON Pointer constructor and could address a prefix sibling.

Ordinary plaintext editing is unchanged. A property whose value, path or label
contains NUL now has a typed, escaped read-only preview in the list and enabled
value control. An early label explains its read-only status; Apply/Revert are
disabled, and pending/apply excludes the preview. Underlying metadata and
history survive Save, selection and refresh. No editable escape grammar was
added. RapidJSON's explicit-length Pointer constructor is used
so direct model edits address the complete key rather than a prefix sibling.
The same audit found null-terminated writes when duplicating object names and
inserting missing material/motion strings. These now retain their full lengths:
copies keep metadata suffixes, and NUL-suffixed invalid enums reach validation
rather than becoming accepted prefixes. Escaped previews distinguish a literal
backslash from a decoded NUL. The preview also covers NUL-bearing object-list
names and individual unresolved resource warnings, retaining the remaining
reason and save question. Surrounding prompts, raw diagnostics and model text
are unchanged.

Both failures were reproduced before the fix under
`out/validation/implementation-loop-20261028/ed11-baseline/84d7f8ddd9d34945b1d71abd6df26ea1`:
the model edited the wrong key and an actual untouched native Save wrote only
`keep`. Independent review, all three editor targets and the complete editor
checker now pass in Debug and Release. The added checks cover exact keys/bytes,
Save/reopen, selection/refresh, full history, typed previews, duplication,
invalid-enum refusals, normal editing and complete resource status text.
Final checker evidence is under `out/acceptance/<preset>/level-editor/`:
Debug `940f7c6bacab493eaa91fb01919eeb47`, Release
`e05985fefdae496a978b3d85c1750c52`.

A normal configure attempt stopped at an unavailable GitHub vcpkg-registry
connection before compilation. Qualification used the already-installed
dependencies with manifest installation disabled only for the offline
configure/build, then restored the original `VCPKG_MANIFEST_INSTALL=ON`
cache settings and ran the full checker with `-SkipBuild`. No network fetch
was retried. These automated checks do not close the attended `ED#5` gate.

### Raw-NUL JSON admission (`ED#12`)

**Done (2026-10-28).** Source review recorded this shortlist after `ED#11`;
the rows describe source before their associated fixes:

| rank | candidate and evidence | value, scope and risk |
|---|---|---|
| 1 | The bundled RapidJSON reader treats a raw zero byte after a complete JSON root as EOF, even inside the supplied length. `level_json::parse` checks only the parser error, so Load or a numeric edit can accept a prefix and later Save drops the suffix. | Selected as `ED#12`: prevent silent source-byte loss with a narrow client parse guard and transactional regressions. Escaped JSON `\u0000` remains valid. |
| 2 | The editor checker redirects `LOCALAPPDATA` before configure, giving vcpkg a fresh registry cache even when dependencies are installed. A normal configure hit an unavailable GitHub connection during `ED#11`. | Implemented separately as [HYG#14](code-hygiene.md#editor-build-profile-isolation-hyg14): isolate runtime after build preflight. |

The public Load regression first reproduced accepted raw-NUL suffixes in
`out/validation/implementation-loop-20261028/ed12-baseline/6e140c3b1bb04fc79522a1a9019a647b`.
The client parse helper now checks the complete input after existing parser-error
handling, retaining malformed-JSON diagnostic precedence. It rejects actual zero
bytes in otherwise successful input with the existing BOM-stripped byte-offset
convention; decoded NUL in valid escaped JSON remains accepted.
This follows JSON's [grammar and string-escape rules](https://www.rfc-editor.org/rfc/rfc8259):
a raw zero is neither trailing whitespace nor an unescaped string character.
The regressions cover bare/trailing-whitespace/second-root/garbage suffixes
with and without BOM, prior malformed diagnostics, numeric property input,
escaped-NUL positives and ordinary trailing whitespace. Refusals retain exact
model/history snapshots, paths, saved baselines and both open and rejected
source files. Independent review, all three editor targets and the full editor
checker pass in both presets. Evidence is under
`out/acceptance/<preset>/level-editor/`: Debug
`9d50fd1d0bdb4f8fa92d2319b0746843`, Release
`423da374a22a46ff9a80f8b2ba7c6a8d`.

Builds used the installed-dependency offline workflow above, restoring both
manifest-install cache settings to `ON` before runtime checks. No engine or
vendored RapidJSON source changed. The attended `ED#5` gate remains open.

## Station and copy backlog (2026-10-29)

A verified source brainstorm on 2026-10-29 found these editor gaps. None needs
a pad or the author, and none settles a layout, route or attended result: those
stay with `ED#5` and the stage's own `DECISION` rows. `ED#25` follows `ED#24`
and `ED#26` builds on `ED#23`, now done.

### Station validation in the editor (`ED#23`)

**Done 2026-10-29.** The engine-free station and lesson rules now live in
[station_contract.h](../game/station_contract.h), beside
`user_stage_contract.h` and `team_gate_contract.h`.
`LevelLoadedInfo::validate_stations` and `validate_lesson` call them with
unchanged messages, and `level_json::validate_document` calls them too, so
every editor edit and load refuses what the game refuses: name spelling,
reserved and repeated names, a station outside `out_of_bounds`, and a lesson
area that is flat, leaves the camera bounds or misses its station, or a blank
lesson title or text. The editor names the entry (`stations[2] ('crown')`)
where the game names the JSON path. Whether lesson words can be drawn is
checked by the game alone, since it owns the fonts.

Acceptance (`scripts/acceptance/level_editor/station_checks.cpp`, in
`check-level-editor`): shrinking Press Tower's `out_of_bounds` past `crown` at
(1774, 620) is refused in the game's words with the earlier undo step intact;
eight station files are refused at load without changing the open document;
Crossfeed, Close Quarters, Press Tower and the Movement Course save and reopen
with their stations. On the game side, the station refusals in `debug_console`
and the lesson refusals in `practice_workshop` pass unchanged through the shared
rules, and `level_format` still admits every registered level and the template.
Station editing in the canvas remains `ED#26`.

### Mirror copy across the arena centre (`ED#24`)

**Open, not started.** The command list in [editor.h](../editor/editor.h)
(lines 20-26) has no mirror command, and `LevelDocument::duplicate`
([level_document.cpp](../editor/level_document.cpp), lines 808-865) only
offsets the copy. Mirrored arenas are checked after the fact by three copies of
the same `mirror()` helper in acceptance (`press_tower/fixtures.cpp` lines
51-55, `crossfeed/` and `cq_blockout/fixtures.h` line 42).

Scope: **Edit > Mirror copy** (Ctrl+Shift+D, currently unassigned) adds one
validated, undoable copy reflected across the `out_of_bounds` centre: X becomes
left + right - x - width, ramp handedness and left/right paint faces swap, slab
rise flips, cycle movers reflect their whole path and add 0.5 to phase, spawn A
maps to spawn B, and a trailing `_a` name becomes `_b`. Paint ferries are
refused in this slice. On the Mover Trial (left + right = 4000) the rule maps
`mover_a` at x 600 with travel 320 and width 320 to x 2760, phase 0.5, which is
the shipped `mover_b`. First step: document checks that mirror each object kind
from a shipped stage and compare with its authored partner. Acceptance: those
checks and a refused-ferry check pass in both builds, with one undo step per
copy. It does not decide whether any stage should be symmetric.

### Symmetry report in the blockout aids (`ED#25`)

**Open, not started; after `ED#24`.** The [blockout aids](#blockout-aids)
summary reports total and selected scoring area
([editor_canvas.cpp](../editor/editor_canvas.cpp), lines 331-332, using
`scoring_area` from [blockout_aids.h](../editor/blockout_aids.h), lines 37-42)
but not whether the two halves match. Press Tower checks mirror partners by
geometry only in acceptance (`press_tower/geometry_checks.cpp`, lines
385-397).

Scope: add a symmetry line to the summary that lists objects without a mirror
partner under the `ED#24` rule and the left and right scoring areas, splitting
centre-straddling objects by region. The Ferry Trial's two ferries both use
`"team_a_heads": "right"` (`turbulence_ferry_trial.json`, lines 494 and 523);
report that as a known authored asymmetry rather than a defect. First step: a
document check on Press Tower expecting no unmatched objects and equal halves.
Acceptance: that check plus one on a deliberately unmatched copy pass in both
builds. It reports only; fairness and side-swap acceptance stay play review.

### Station editing (`ED#26`)

**Open, not started; `ED#23` is done.** `EntryKind`
([level_document.h](../editor/level_document.h), line 15) has no station kind,
and stations cannot be seen or moved: the editor only validates them, so a
bounds edit that would strand one is refused, but nothing shows where they are.

Scope: an `EntryKind::station` listed in Level Contents, drawn as a marker with
its name, added from the palette and edited through a `name` inspector row and
drag, all validated by the `ED#23` shared rules. Spawn entries already show the
pattern (`entries()` and the spawn branch of add, `level_document.cpp` lines
420-426 and 707-711). Lesson blocks remain preserved metadata with no
controls. First step: document checks that add, rename, move, duplicate and
delete a station with undo, and refuse a reserved or repeated name. Acceptance:
those checks and a lesson round trip on the Movement Course pass in both
builds. It does not place any station.

### Copy and paste between documents (`ED#27`)

**Open, not started.** Neither [editor.h](../editor/editor.h) (lines 20-26) nor
the accelerator table in [editor_window.cpp](../editor/editor_window.cpp)
(lines 157-164) has Copy or Paste; `duplicate` copies only within one
document.

Scope: **Edit > Copy** puts one selected object on the clipboard as
`CF_UNICODETEXT` JSON, and **Edit > Paste** appends it to the open document as
one undo step through full validation, refusing anything that does not
validate. The inspector's edit field keeps its own Ctrl+C and Ctrl+V. First
step: a document-level paste of a copied object into a second stage, including
refusal of malformed text and of an object that would break validation.
Acceptance: those checks and a hidden-window clipboard round trip pass in both
builds. It does not add multi-selection.

## Acceptance checks

`ED#5` is `DECISION`: the remaining GUI and saved-stage gameplay checks need an
attended session and a recorded result. The automated checks below establish
document behaviour and hidden-window operation, not that manual acceptance.

```powershell
./scripts/check-level-editor.ps1 -Preset x64-debug
./scripts/check-level-editor.ps1 -Preset x64-release
```

These build and run the native document model, a hidden in-process DPI/canvas
probe and hidden GUI launch/open/respond/close and save-guidance checks. None
creates a game graphics/audio device.
They cover the shipped levels, the canonical template, unknown-field preservation, both rectangle
encodings, ramps, the palette, undo/redo and save
state, Unicode paths and failure recovery. Evidence and generated test levels
remain in `out/acceptance/<preset>/level-editor/`. They also check the optional
Random setting's default, insertion, validation, undo/redo and save/reopen,
and each palette kind's default material, refusal of unknown and mismatched
materials (in edits and loaded files), and inserting and clearing a material.
They check the resource-name warnings against source content, and a third
hidden launch reads the status line of a level the game would refuse.
The Save to My Stages model checks find the folder
under a scratch `LOCALAPPDATA` without creating it, and refuse a case-only
shipped-name collision, a space, a leading hyphen, a 65-character name, a
non-ASCII name, a non-`.json` file, and files outside the folder or in a
sub-folder. They accept the 64-character limit and the offered names. They
check that the first save creates the folder and file and reloads unchanged,
that a second save under another spelling is reported as a replacement of
the same file and leaves one file, and that no save file appears.
The script points `LOCALAPPDATA` at its run folder for every launch. It also
checks that opening the editor creates neither a save nor the stage folder.
The hidden save-guidance helper also drives its own file dialogs, checking
Save to My Stages, ordinary Save/refresh, cancellation, failed-save recovery
and Save As elsewhere. It waits for a responsive owner and the operation's
observable results, including folder cleanup and saved file/status changes,
before issuing the next action. Attended dialog usability and the replace
question remain part of the manual pass.
The document checks also cover the shared current version, legacy save upgrades,
unknown-field preservation, strict format type/version rejection before other
field decoding, filename diagnostics, failed-upgrade recovery and retained
undo/redo branches across a successful upgrade.
Use `-SkipBuild` only after
building `colourwars_editor_acceptance`, `colourwars_editor_dpi_acceptance`
and `ColourWarsEditor` for the
current source.

On 2026-10-15, both presets passed the document checks (including the optional
Random setting) and hidden GUI launch/open/respond/close checks. Evidence:
`out/acceptance/x64-debug/level-editor/088ef35b371047ee97c715f163c61008/`
and `out/acceptance/x64-release/level-editor/2ca17df03ee24d7683cb8131b44fb245/`.
The manual GUI and saved-stage play checks below remain open (`ED#5`).

On 2026-10-28, both presets also pass the expanded DPI/canvas and native
save-guidance checks: Debug `8c23b3aacabc4e26a2b4e25bb1986fcd`, Release
`9f85cf04ebba419c94034365360a57bf`, under the same evidence root. These are
software checks; the attended GUI and saved-stage play boundary remains.

For a manual GUI check, open each shipped arena, select a platform, move and
resize it, edit a field, duplicate/delete, undo/redo, and save a copy. Reopen
the copy and check its coordinates. Also check wheel zoom, middle-button pan,
grid snapping and the unsaved-changes prompt on New, Open and Close. Give a
copy more than eight unknown resource names and check that Save and Save As
list the first eight and "and N more", that No is the default and writes
nothing, and that answering Yes to the unsaved-changes prompt on Close and then
No here keeps the editor open. Rebuild
and play a saved stage to check collisions, paintable faces, spawns, camera
bounds and ramps. Document acceptance does not establish those play qualities.

## Moving-structure authoring (`GAP#D5`)

**Implemented and qualified 2026-10-26.** The palette's
**Moving paintable block** and **Moving paintable platform** create the existing
solid and jump-through horizontal paintables. They write `StructurePaintable`
with the optional [motion record](level-format.md#horizontal-paintable-motion),
using `pencil` and `ruler` materials respectively. No new motion kind is added.

The rectangle is the **leftmost anchor**. Dragging, nudging, resizing and exact
coordinate edits change that anchor; horizontal travel remains an absolute
distance and the period and phase remain unchanged. The inspector exposes
`motion/type` (`horizontal`), `motion/travel_x` in world units,
`motion/period` in round-trip seconds and `motion/phase` in cycles [0,1).
New movers start at travel 120, period 6.4 and phase 0 as editable authoring
defaults, not an accepted arena placement or balance choice.

A mover is made a [paint ferry](feature-roadmap.md#paint-ferry-road15-first-slice)
(`ROAD#15`) in the same inspector. `motion/drive (cycle or paint)` is always
listed, as `cycle` when the record omits it; `motion/team_a_heads (right or
left)` is listed when present, and a paint drive's phase is labelled
`0 left end to 0.5 right end`. Setting the drive to `paint` adds team A heading
`right` unless the mover already has a heading. Setting `cycle`, or clearing the
drive, removes the heading, and a cleared drive is omitted rather than written
as `cycle`. Typing `cycle` over an omitted drive changes nothing. Any other
value is written as typed and then refused.

The drive is checked by the game's own rule, not a copy:
`horizontal_motion_contract::decode_drive` reads the two text fields and
`validate_drive` the paint-drive phase, and the game's `LevelObjectBuilder`
calls the same two. An unknown drive, a paint drive without a heading or with
one other than right/left, a heading on a cycle (explicit or by default), a
paint drive whose phase is past 0.5, and a non-text drive or heading are each
refused. As an edit, the refusal leaves the document and history unchanged; as
a file, the load is refused, and every drive rule above is in the same words
as the game's load error (only the non-text type error is each side's own).
So a bad drive never reaches a save.

The filled body shows its initial phase. A white dashed outline marks the
anchor, blue dashes the complete travel envelope, and gold dots the expanded
standing corridor. Selection and resize handles edit the anchor; clicking the
initial body also selects that object. **F** frames the complete travel and
standing guides. Drag guides are provisional until the drop commits. These
are geometry guides, without simulated riding or a route-feasibility claim.

**Ctrl+D** retains motion, material and unknown metadata and places the copy
below the original's complete body/standing corridor. It does not use the
ordinary object's overlapping 50-unit offset. If the new envelope leaves Scene
bounds or intersects another structure, duplication is refused. Anchor, motion,
static-geometry and bounds edits validate the complete candidate before changing
the document or undo/redo branch. Save/reopen retains unknown root, object and
motion metadata, rectangle/TLBR encoding and ignored alternative geometry.
The `level_editor` document checks open, save and reopen the shipped Ferry Trial
(`turbulence_ferry_trial`) unchanged, flip and switch its drives with undo, turn
a palette mover into a ferry, and refuse each bad drive shape both as an edit
and as a file.

The application supplies source `game/content/tuning.json` for player size;
the registration helper supplies the same repository context. Tools using the
document API pass that path to `LevelDocument(tuning_path)`. The default
constructor still works for static documents, but cannot load or add a mover
without this explicit context. Mover validation rereads current `player.size`
and shares the runtime's float field, travel and standing-envelope arithmetic
through the engine-free `game/horizontal_motion_contract.h`. Changes to source
size can therefore invalidate a formerly usable corridor, including an undo
candidate. No guessed player size is substituted.

The complete platform and standing envelopes must lie inside Scene bounds and
clear the other flat/ramp structures and mover paths. Full actual-player
rate/displacement admission remains authoritative in production `LevelBuilder`;
passing editor geometry validation does not qualify that budget or gameplay.
Scoring diagnostics keep mover dimensions and area in anchor space and translate
only guide locations, including at fractional float boundaries. Gameplay's
per-tile float accumulation can round differently from continuous editor strips;
neither total changes merely because initial phase changes.

The document checks cover both palette kinds, properties, phase/standing guides,
transactional rejection, anchor move/resize, duplication, undo/redo, changed
source dimensions, metadata/TLBR preservation and save/reopen. The CPU
`moving_structure` case compiles the actual editor model and passes its saved
stage through production `LevelBuilder`, with material/scoring checks and Debug
Recorder/Player reconstruction. Debug/Release editor builds, document checks and
`moving_structure` passed after the fractional-area fix. Debug includes 180
exact editor-save replay updates; Release explicitly excludes developer replay.
Receipts: `out/validation/overnight-20261026/phase4/` (`editor-*` and `mover/`).
No new GUI, native-pixel or human acceptance is claimed here.

Human GUI handling and saved-stage play remain `ED#5`; Turbulence
route/readability acceptance remains `POL#21`.

## Team gate authoring (`ROAD#15`)

The palette's **Team gate** entry adds a
[team gate](feature-roadmap.md#team-gate-road15-second-slice): a solid
`StructurePaintable` with the `eraser` material, `"gate": "paint"`, and every
face paintable except the bottom, which stands on the floor. The inspector
lists `gate (paint: open to the team holding more of it)`. Clearing it removes
the field and leaves an ordinary paintable block; any other value is written as
typed and then refused. A gate cannot be added to an existing block in the
inspector, which only edits fields a record already has; use the palette entry.

The gate is checked by the game's own rule, `game/team_gate_contract.h`: an
unknown or non-text value, a gate on anything but a `StructurePaintable`, a
one-way (`STRUCTURE_JUMP_THROUGH`) gate, a moving gate and a gate with no
paintable face are refused in the game's words, as an edit (document and
history unchanged) or as a file. So is a gate overlapping a spawn body, which
the editor sizes from `tuning.json` as it does for movers, with the game's
startup words. The canvas draws a gate like any eraser block; it does not show
which team it is open to, which is runtime state. The `level_editor` document
checks cover the field, palette entry, every refusal and save/reopen of the
Gate Trial.

## Fixed scored-slab authoring (`GAP#D5`)

The **Scored slab rising right** and **Scored slab rising left** palette entries
write `FixedSlabPaintable` with `STRUCTURE_FIXED_SLAB` collision and the required
`eraser` material. The fixed footprint spans 160 horizontal units, rises 80
units and has 24 units of thickness measured along the slab normal. It is a
solid scored slope, distinct from the existing unscored paint-pass ramp.

`slab/x` and `slab/y` locate the top endpoint with the smaller world X;
`slab/rise` selects `left` or `right`. A right-rising slab at `(1240,880)` ends
at `(1400,800)`; its mirrored left-rising counterpart starts at `(2600,800)`.
Endpoints must remain finite and within 65,536 world units. Dragging, nudging
and rectangle-coordinate edits translate the body; resizing is refused. The
canvas draws the actual sloping polygon and top support segment, and selects
its body without treating an empty bounding-box corner as solid. Duplicates
move below the complete body. Intersecting slabs are refused using the same
strict positive-overlap test as runtime; a separated pair with overlapping
bounding boxes remains valid.

The four paint-face switches retain local face identity. Areas are measured
before transforming their strips into world quads. Shared canonical paint
adjacency corrects only checked OBB reconstruction drift at an authored join;
the collider, visible tile pose and guide stay at their represented coordinates.
It does not round an obstacle or erase a real small authored gap. Arbitrary
rotation, resizing, motion and combined shape records are refused. Unknown
metadata survives edits, duplication, undo/redo and save/reopen. The new type
uses its dedicated material rather than generic sheet/frame/colour fields.

Debug and Release game/editor builds, the full editor document suite and three
hidden editor launch/status/close checks per preset pass. New document checks
include mirrored
normal extrusion, translation and immutable size, both-direction duplication,
transactional pair/field refusals, saved metadata/schema round trips, actual
polygon scoring, reciprocal face clipping, exact buried-cap zero and retained
`0.00004` authored gaps. The CPU `fixed_slab` case separately loads an actual
editor-saved trial through fresh 1/4-player public builders. Receipts are under
`out/acceptance/x64-debug/level-editor/30f6ffeacf6a466a904168cdb7192c11/`
and the matching Release receipt `78fd5457eb954c6da74d8b73355ac283`, alongside
`out/validation/overnight-20261026/phase11/` (`slab-v4/x64-debug`, `regression/x64-release`).
These checks do not establish attended GUI handling, saved-stage play, native
composition or competitive value.

## Registration assistance (`ED#4`)

`ColourWarsRegisterStage` previews and explicitly registers a level already
saved directly under `game/content/levels/`. Build the opt-in tool, then preview
the two additions before choosing to apply them:

```powershell
cmake --build --preset x64-debug --target ColourWarsRegisterStage --parallel 2
out/build/x64-debug/editor/ColourWarsRegisterStage.exe --repo . --stage game/content/levels/my_stage.json
out/build/x64-debug/editor/ColourWarsRegisterStage.exe --repo . --stage game/content/levels/my_stage.json --apply
```

The target is excluded from normal builds and release packaging. Relative stage
paths are relative to `--repo`; wide Windows paths are retained. `--help` explains
the arguments. Exit codes are 0 for success/no-op, 2 for usage errors and 1 for a
validation or apply failure. Ordinary Save/Save As remains independent, and the
editor Help names `cmake/runtime_assets.cmake` and this separate helper.

Preview writes nothing. It checks the existing level's structure and references
against a complete content catalog, verifies that the complete manifest/runtime
asset lists already agree, then shows the exact added name and runtime path.
The basename uses ASCII letters, digits, `_` or `-`, with no leading hyphen or
Windows device name. The file must have a `.json` extension and be directly in
this source tree's levels directory; linked/reparse paths are refused. Duplicate
or case-alias names, one-sided registration and unsupported/nonliteral CMake list
syntax are diagnosed rather than guessed at. An entry already present in both
lists is a validated no-op.

Apply revalidates the proposal and unchanged stage/source bytes. It prepares and
flushes both candidates and both original backups before replacing either list.
Only the two entries are inserted; surrounding bytes, line endings and unrelated
metadata remain. If the second replacement fails, the first is restored while
it is still this operation's candidate. A failed rollback retains both originals
at the recovery paths printed in the error; inspect both lists before retrying.
Each replacement is atomic; the pair is not crash-atomic. The saved stage itself
is never rewritten.

Configure/build the game after registration to copy the stage beside the
executable. The game's startup/content validation remains authoritative;
registration does not establish route, spawn or gameplay acceptance (`ED#5`).
GUI integration can follow actual use of the command-line helper.

The explicit-only [Turbulence slab trial](../game/content/levels/turbulence_slab_trial.json)
was saved/reopened by `LevelDocument` and registered through this API on
2026-10-26. It preserves the qualified 24-object geometry and all eight earlier
level files. The [arena record](polish-turbulence.md) owns its public-loader,
route and composition evidence; this automated use does not close `ED#5`.

The document acceptance case also checks exact LF/CRLF and BOM-preserving
appends, malformed/partial/aliased lists, resource validation, stale inputs and
proposals, first/second replacement failures, locked/read-only files and named
original backups after a failed rollback. The separate integration check uses
a fresh scratch repository and only the reviewed runtime assets:

```powershell
./scripts/check-stage-registration.ps1 -Preset x64-debug
./scripts/check-stage-registration.ps1 -Preset x64-release
```

On 2026-10-18, both presets passed the editor document and three hidden-window
checks, then preview/apply/no-op and rejection checks, including a directory
junction. Each scratch registration changed only the manifest and runtime list,
passed the existing CMake agreement check, staged all 29 registered assets
byte-for-byte, and passed isolated game startup with the published audio bank
and per-monitor v2 DPI awareness. No hardware skip was used. Evidence is under
`out/validation/stage-registration/`; the integration runs are
`integration/x64-debug/6821efb469a946b89b2b943419b58153/` and
`integration/x64-release/3850fed0b130438a814d23c6ee7817aa/`.
These automated checks complete `ED#4`; the attended editor and saved-stage
play review remains `ED#5`.

## Movement Course icon (`ED#6`, retired)

Retired 2026-10-18 during the backlog audit: `icon_frame` is required legacy
metadata, and no current screen displays it. Stage Select draws the actual
arena preview. Keep the existing valid frame rather than commission invisible
artwork. A distinct icon becomes work only if an accepted screen needs one.
