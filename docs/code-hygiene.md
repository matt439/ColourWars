# Code hygiene and maintenance

This is the live owner for `HYG` rows in [TODO](../TODO.md). The author asked
for source-file splits and comment audits on 2026-10-28. These are independent
maintenance slices alongside the gameplay backlog; they do not change its
arena/tuning order.

## Scope and working rules

File length identifies candidates to inspect, not a maximum or a reason to cut
a cohesive implementation. Preserve the game's inheritance, public signatures,
names, layouts and behavior. Engine and vendored code are outside this pass.
Keep historical review documents as written.

For a split, move whole definitions and their private helpers together wherever
possible. Preserve evaluation, callback, RNG, diagnostic and floating-point
operation order. Do not combine a split with bug fixes, renaming, broad
formatting, a new abstraction model or gameplay changes. An extraction that
needs new private parameters must say so; it is not a literal source move.

Register new game translation units in [game/CMakeLists.txt](../game/CMakeLists.txt),
with the existing PCH, include and warning conventions. The
[acceptance runner](../scripts/run-acceptance.ps1) already compiles a case's
same-named folder of `.cpp` files. Header-based visual/document fixtures have
their own inclusion context; preserve it or wire an explicit replacement.

Review the moved definition bodies and narrow build/include changes, then build
Debug and Release and run the existing checks named in each scope. Do not add
tests that merely mirror a move. Comment-only work needs caller/implementation
review and a comment-only diff, not a new runtime test suite. Keep structural
changes and behavioral defects in separate commits when implementation lands.

## Source-size snapshot (2026-10-28)

These counts include blank lines and comments in tracked authored files. Refresh
the affected files before starting; this is audit evidence, not a permanent size
gate. The earlier figure/animation and other source splits already exist.

| file | lines | audit result |
|---|---:|---|
| [presentation_validation.cpp](../game/presentation_validation.cpp) | 2,502 | Ordered helper extraction, `HYG#1`; one large function prevents a simple method move |
| [player.cpp](../game/objects/player.cpp) | 1,848 | Retain the coupled movement/contact/carrier core; figure code is already separate |
| [input_bindings.cpp](../scripts/acceptance/input_bindings.cpp) | 1,433 | Topic-based acceptance split, `HYG#5` |
| [evidence-record.ps1](../scripts/check-painter/evidence-record.ps1) | 1,403 | Evidence-metadata groups, `HYG#6` |
| [painter_visual.cpp](../scripts/acceptance/painter_visual.cpp) | 1,347 | Retain the explicit include/setup/dispatch entry; no registry redesign selected |
| [level.cpp](../game/objects/level.cpp) | 1,316 | Horizontal-motion subsystem, `HYG#2` |
| [tune_colour_document_checks.h](../scripts/acceptance/debug_console/tune_colour_document_checks.h) | 1,044 | Independent fixture families, `HYG#7` |
| [tuning.cpp](../game/tuning.cpp) | 907 | Retain cohesive key-by-key parsing and validation |
| [level_document.cpp](../editor/level_document.cpp) | 899 | Retain the shared document transaction; save protection remains `ED#7` |
| [projectile.cpp](../game/objects/projectile.cpp) | 867 | Sweep/contact subsystem, `HYG#3` |
| [weapon.cpp](../game/objects/weapon.cpp) | 855 | Retain the current firing/lifecycle core; motion already has its own file |
| [save.cpp](../game/save.cpp) | 802 | Profile/effective-preference overlay, `HYG#4` |

## Ordered presentation validation (`HYG#1`)

**OPEN, L.** `Presentation::validate_resources` accounts for roughly 2,150
lines of the largest source file. Its lambdas and page/layout variables cross
topic boundaries, and its first-error order is intentional. Moving arbitrary
blocks to separate files without carrying that context would change or break
validation.

Extract private helpers by page/validation family with explicit required
context and an ordered coordinator, then distribute the helpers into coherent
translation units. This requires private helper extraction, rather than only
moving whole existing methods. Keep the public API, calculations, lookup order
and diagnostic text/precedence intact; do not introduce a generic validation
framework. `validate_stage_counts` and `sound_references` can move as complete
methods where they fit.

Acceptance: Debug/Release builds and existing `menu_text`, `menu_presentation`,
`weapon_catalog`, `stage_selection` and startup/content checks. Compare the
first diagnostic for existing invalid fixtures, including candidates with
multiple invalid fields. Keep `ROAD#25`'s paper-face correction separate and
refresh this split's baseline if that correction lands first.

## Level horizontal motion (`HYG#2`)

**OPEN, M.** Move the `horizontal_*` admission helpers and Level's horizontal
motion query, budget/admission, prepare, commit, zero-step and state methods
together into `game/objects/level_horizontal_motion.cpp`. Their private
geometry/budget arithmetic belongs with this subsystem rather than the main
match implementation.

Leave `update_level_logic` and its call order in place. Preserve the existing
Level API, member layout, admission diagnostics and pre-step ownership timing.
Keep the helper bodies and float/double arithmetic unchanged. Acceptance:
Debug/Release builds and existing `moving_structure`, `paint_ferry`,
`fixed_slab`, movement and replay checks. Review that the change only moves
definitions and adjusts includes/source registration.

## Projectile contacts and sweeps (`HYG#3`)

**OPEN, M.** The private sweep/relative-impact helpers and the contiguous
`resolve_launch_obstruction` through `flush_paint_audio` definitions form one
contact subsystem. Move them together into
`game/objects/projectile_contacts.cpp`, leaving construction, diffusion,
movement, lifetime and accessors in the existing file.

Preserve sweep interval ordering, launch exemptions, relative moving-surface
geometry, endpoint deduplication, contact/paint callbacks and audio flush order
exactly. Keep helpers private to their new translation unit. Acceptance:
Debug/Release builds and existing `blocked_shot`, `tunnelling`,
`moving_structure`, `fixed_slab`, paint and replay checks. A discovered contact
defect gets a separate fix; it is not folded into the move.

## Save profile overlay (`HYG#4`)

**OPEN, M.** Move `copy_slot`, the preference/binding setters and
effective-preference/profile operations (`refresh_effective`, wearing,
creation/rename/deletion and Boost persistence) into `game/save_profiles.cpp`.
Keep JSON sections, schema migration, file loading and atomic writing together
in `save.cpp`; this separates profile policy from the persistence mechanism.

Preserve effective-setting reference identity, per-slot association, change
ordering and existing write-failure behavior. Do not change the save schema or
repair unrelated save semantics. Acceptance: Debug/Release builds and existing
`player_profiles`, `settings`, `input_bindings`, `handicap` and
`remembered_setup` checks, including their existing failed-save and relaunch
paths. Use isolated profiles throughout.

## Input-binding acceptance topics (`HYG#5`)

**OPEN, M.** Split the long `input_bindings` acceptance case into domain and
independent oracle checks, actual reader/resume checks, save/schema checks and
keyboard-action checks under `scripts/acceptance/input_bindings/`. Keep `main`
and the single fake `GamepadReader` implementation in the entry file, with only
the required fixtures/declarations shared across topics.

Preserve every literal oracle, assertion and invocation order. Keep each
scratch `LOCALAPPDATA` lifetime and retained-profile relaunch path unchanged;
the split must not accidentally substitute production helpers for independent
expectations. The runner already discovers the topic `.cpp` files.
Acceptance: the existing `input_bindings` case in both presets, including the
same retained-profile child/relaunch invocations, with identical outcomes.

## Painter evidence metadata (`HYG#6`)

**OPEN, M.** The evidence script has a base ordered record, a long focused-scope
precedence chain and common final fixups. Extract its branch bodies into
coherent arena, menu/text, colour and action/HUD groups; keep the base,
selection precedence and final fixups explicit in the entry script.

Preserve dot-sourced variable scope, ordered keys, exact evidence strings,
path selection and failure behavior. The runner loads this before its `try`,
so relocating a failure is also a behavior change. Avoid replacing the chain
with an implicit registry or changing the painter matrix.

Acceptance: parse all resulting PowerShell files and evaluate old/new metadata
construction for every focused switch in both presets, comparing ordered
output with only revision/working-tree metadata normalized. Exercise combined
switch precedence too, then run representative existing focused native leaves.
A full painter matrix is not needed solely for moved metadata, and a higher
timeout is not a substitute for preserving it.

## Colour-document fixture families (`HYG#7`)

**OPEN, M.** `check_colour_document()` is almost an entire 1,044-line header.
Its already scoped blocks divide into generic splice/alias behavior,
selectors/feedback, opening/story colours and HUD/figure/identity colours.
Extract these families into topic helpers and keep the existing function as an
ordered coordinator.

Preserve literal JSON bytes, source-span assertions, cases and invocation
order. The header is included inside an anonymous namespace by
`tune_document_checks.cpp`; retain that context or explicitly supply topic
translation units through the existing `debug_console` case folder. Avoid
runtime registration or new production accessors. Acceptance: the existing
`debug_console` case in Debug and Release, retaining Release exclusion checks
and all byte-preservation/conflict/reset fixtures.

## Source-comment contract audit (`HYG#8`)

**OPEN, M.** Audit the following bounded groups against implementation and
callers, then correct comments that overstate, omit or misdescribe today's
contract. Current exhibits establish that this is real maintenance work:

- **Participant, slot and pane identity:** `player_builder.h` says settings
  order determines view indices, while `build_players` allocates panes only
  to humans. Review the related Level, player-builder and menu-settings
  declarations and their mixed human/bot callers. Describe each index domain
  precisely without renaming APIs or changing roster behavior.
- **Shipping versus Debug replay:** `level.h` says the final seat filter is
  null in Release, although `game_level_dev_tools.cpp` supplies the shipping
  `RoundTapeRecorder`. That source's Debug-only caller claim also misses the
  Release forwarding adapter in `debug_director.h`. `round_tape.h` claims any
  between-step rule edit spoils the tape, but live tuning edits are an
  acknowledged exception. Follow the actual filter and invalidation paths;
  preserve all existing shipping/developer-tool boundaries.
- **Catalog and editor metadata:** `game_data.h` describes only manifest
  order, and `menu_level_settings.h` calls a stage ID a manifest key, omitting
  the accepted user-stage path. `LevelDocument::unresolved_resources` still says
  Stage Select draws stage icons, although it uses a live preview and `ED#6`
  is retired. Keep legacy icon validation described as such. Retain the
  accurate rescan, registry ownership and no-match lifetime restrictions.

For each edited claim, record the declaration and implementation/caller used
to verify it in the completion evidence. Preserve useful explanations of
ownership, threading, geometry, determinism and why a workaround exists;
remove only demonstrably stale narration or redundant restatements. Do not
expand this into rewording every comment, deleting historical reviews or
reopening answered engine design decisions. Acceptance is a reviewed
comment-only diff with unchanged executable tokens and verified named callers.
