# Menu presentation data

Shared menu geometry and artwork choices live in `presentation.json`. Pages
still own their widgets, navigation, row/column indices and screen transitions.
The menu design coordinate system remains 1920 by 1080; the existing camera maps
it to the physical window.

## Shared layout and fills

| JSON path | Shipped value | Use |
|---|---|---|
| `menu_style.layout.position` | `(150, 150)` | Base widget origin |
| `menu_style.layout.size` | `(300, 75)` | Base widget size |
| `menu_style.layout.spacing` | `(250, 150)` | Per-column and per-row offsets |
| `menu_style.sprite_sheet` | `sprite_sheet_1` | Shared fill atlas |
| `menu_style.fill_frame` | `pixel` | Solid frame stretched and tinted for menu shapes |

`MenuPage` copies the layout when it is constructed. Existing page-specific
positions, sizes and spacing continue to override those defaults. Changing the
shared origin does not replace Home, Options, Controls or pause-page overrides.
Each component must be finite. Position is nonnegative, size is positive, and
the first widget must fit the design rectangle. Spacing is nonnegative and at
most the design width/height; zero is valid for a single row or column. A page's
complete arrangement still needs review with its own row count and text.

The shared fill names feed menu chrome, plain backgrounds, paper artwork, stage
preview/briefing shapes, results bars and stick-response previews. Their
identifiers must be nonempty, and startup validates the named frame against the
named atlas. An art replacement must supply an appropriate solid fill.

## Menu sounds

Every menu sound is an object, `{ "name": "<wave>", "volume": <gain> }`, with
the gain in `0..1` and multiplied by the player's SFX preference when it plays.

| JSON path | Shipped value | Plays on |
|---|---|---|
| `menu_style.sounds.direction` | `ui.move` at `1.0` | Focus moves, on every menu page |
| `menu_style.sounds.confirm` | `ui.confirm` at `1.0` | A row or choice is accepted |
| `menu_style.sounds.cancel` | `ui.cancel` at `1.0` | Back |
| `main_menu.sounds.error` | `ui.error` at `1.0` | A refused setup choice |
| `main_menu.sounds.ready` | `ui.ready` at `1.0` | A player readies |
| `main_menu.sounds.unready` | `ui.unready` at `0.7` | A player unreadies |
| `pause_menu.window_open_sound` | `ui.pause_open` at `1.0` | Entering pause |

The name must be a nonempty wave in `menu_style.sound_bank`; startup reports a
missing one under its `.name` path. The other menu-owned cues (`pad_lost`,
`pad_back`, `resume`, `volume_preview`, `series`, the results stings) take the
same shape. `docs/audio-inventory.md` owns which wave each name is.

## Home return focus

Home's initial focus is a bounded C++ choice among Play, Options and Credits,
selected once after its actions are registered. Both Options Back routes request
Options; Credits requests Credits. Title, both Mode Select return paths and the
developer page factory retain the default Play focus. The Back row and B/Escape
share each page's return choice. Existing page-entry input rearming still
applies, and Controls/audio continues to return to its retained parent page.
No presentation field, saved navigation history, artwork or geometry is added.

## Home Exit confirmation

Home's Exit action pushes a confirmation child with No initially focused. No
or B/Escape resets actions and pops the child, returning to the same Home page
with Exit focused. Back has priority and an update can make only one focus move;
confirmation requires a later accepted action. Yes alone uses the existing
music fade and application quit. Pending exit suppresses ordinary input and
freezes motion; developer dismissal cancels the queued callback before popping.
Pause and End behavior is unchanged.

The new required `menu_text.confirmations.exit` ships as `Exit ColourWars?`.
It follows the existing normalized, nonblank, single-line text rules. The
heading uses `menu_text.items.exit`, with existing Yes/No labels. Fonts and
paper geometry reuse the shared heading/item/detail fonts and `pause_menu.art`,
`box_size`, `layout`, `heading_y` and `status_y`; the surround uses Home's neutral
fill. No numeric field or artwork asset is added. The page captures its text
and uses the existing input rearming, shared motion and Reduce effects policy.

## Options Restore defaults

The required `menu_text.items.restore_defaults`, shipped as `Restore defaults`,
is appended as the thirty-seventh menu item. Options places it at row 4 after
Controls/audio (row 3); Apply and Back are rows 5 and 6. The existing first-row
anchor and status y = `885` remain, with `main_menu.options_spacing.y` changed
to `100` for the seven rows.

Options captures geometry from `main_menu.setup_focus_padding` (`28, 7`) and
`setup_shadow_offset` (`2`) while its paper colours remain live in
`main_menu.art`. The local paper-geometry override keeps the existing item font
and does not change other pages. No numeric field, font or asset is added.

Restore defaults stages only the canonical `SaveData{}` resolution, fullscreen
and palette values: 1920x1080, windowed and Random. It does not write Save,
change the application display or reset play/pad settings. Display-changing
Apply uses the trial below; palette-only or unchanged-display Apply retains
direct persistence. Back discards the pending
values, and returning from Controls/audio retains them. Applied fullscreen
still disables resolution and shows its live dimensions until Apply, even when
the restored pending fullscreen value is Off.

## Options display trial

Changed resolution/fullscreen preferences or application-reported fullscreen state open a
display-trial child above the retained Options page. It owns the three candidate
display/palette fields and the prior configured Save preset plus the prior
application-reported fullscreen state from `Application::fullscreen()`. Save and
file bytes remain unchanged until Keep.
Palette-only and unchanged-display Apply retain the direct path. Keep preserves
current play/pad settings; a failed write leaves the new active display/palette
preferences and prior file intact, with the existing save-failure status.

The appended required `menu_text.items.keep` and `revert` bring the item count
to 39. The question is `menu_text.confirmations.display`, shipped as
`Keep these display settings?`. Required `menu_text.display_trial_countdown`
ships as `Reverting in {seconds}s` and must contain exactly one `{seconds}`
token. This is a bounded countdown substitution, not a general formatting
language. `main_menu.display_trial_seconds` is finite, from `1` to `60`, shipped
as `15`. The page captures Pause art/fonts, overriding focus padding and shadow
with the existing `main_menu.setup_focus_padding` (`28, 7`) and
`setup_shadow_offset` (`2`). It retains the Pause palette and keyline (`4`).
This display-only padding/shadow recipe leaves Pause art unchanged and adds no
numeric layout field. It uses the existing Options heading, Keep row 0,
safe initial Revert row 1 and a centered countdown below the two rows.
Countdown information remains visible with Reduce effects.

Revert, B/Escape, expiry, deactivation, coverage, developer reset and owner unwind
cancel the trial. A covered terminal child closes only itself when uncovered.
Rollback reissues the prior configured preset and `Application::fullscreen()`
state. On Win32 this is the last requested application state and fullscreen uses
borderless window styling, without an independent OS success/readback guarantee.
It does not restore arbitrary dragged size, placement or maximized state. Options can retain its
candidate for retry. Time accounts for elapsed updates and steady-clock time on
the next update; no rollback can run while the process is stopped. The status
roles `options_reverted`, `options_display_failed` and `options_rollback_failed`
report revert and display/rollback exceptions. Human/hardware qualification
remains open under its existing owners.

## Paused Options

Pause's four rows are Resume, Restart, Options and Quit. Options is a pushed
child of the retained Pause page. Both Back routes return to that same Pause
with Options focused, without resuming the match or starting its resume count-in.
Main-menu Options retains its existing Home return. Paused Options, its display
trial and the nested Controls/audio and Pad Sticks pages share the opening-device
and disconnected-device fallback policy. A paused Options page cannot use the
developer match-start route.

The existing `pause_menu.layout.spacing.y` is now `120` rather than `115`.
Options' measured item-font height (`82`) and two 18-unit focus pads require
`118`, leaving two design units of row-envelope clearance. The final row moves
15 units lower, and Pause-derived confirmations reuse the same spacing. No new
field, font or art asset is introduced.

The required `menu_text.options_palette_timing` ships as
`Palette applies next round or restart.` It is persistently shown only in paused
Options, using the existing detail font without scaling at
`(main_menu.options_value_x, main_menu.options_heading_position.y)`. Text and font
are captured when the page opens; the shipped detail-36 caption measures 834 by
62 design units. No numeric geometry field is added. Applying
the saved palette affects the next round/restart, preserving the running match's
player, paint and HUD colours. Display/palette save and trial behavior retain
their existing limits, including configured-preset/application-state rollback
without independent OS success/readback or arbitrary-window-placement guarantees.
Human and hardware qualification remains separate.
The implemented defaults, return focus, Exit confirmation, display trial and
paused access complete `GAP#3.2`.

## Mode and Player Count paper

The required `main_menu.setup_box_size` supplies the common paper panel for Mode
and Player Count. Its shipped value is `(1760, 920)`, centered in
`(1920, menu_style.chrome.footer_y)`. The pages copy `main_menu.art` for paper,
ink, shade colour and pen width, replacing its card padding and shadow offset
with the required `main_menu.setup_focus_padding` (`28, 7`) and
`main_menu.setup_shadow_offset` (`2`). The solid surround uses
`main_menu.home_background_colour`. `MenuPaperArtwork` draws only the enabled
focused row's card and detached arrow; neither page has a persistent
primary-action card.
The legacy brand and focus stripe are suppressed on these two pages. The
controls footer remains visible.

The panel dimensions must be finite and positive, at most `1760` wide and at
most `menu_style.chrome.footer_y - 40` high, leaving at least 20 design units
above and below. Shared paper-art validation includes this panel's size. Startup
requires positive, finite setup padding no greater than `(48, 24)`, with each
component greater than `setup_shadow_offset + main_menu.art.keyline_width`.
The setup shadow offset must be finite and between `0` and `24`. This leaves
clearance for the pressed face, its keyline and its shadow. Startup also checks
each actual heading, row and solo hint against the usable paper
interior, retaining the transparent text-shadow extents. Focused rows reserve
their card padding, detached arrow and extra shade clearance; this reserved
height must fit the authored row spacing. The drawing includes its shade inside
the card, while validation conservatively reserves the extra clearance. The
setup-specific vertical padding preserves the existing 100-unit row stride:
the tallest shipped glyph bounds (`82`) plus retained shadow extent (`1.5`),
both 7-unit pads and 2-unit shade clearance total `99.5` design units. These
checks reject content that would cross the paper edge or another row, without
adding a new fitting path.

Every existing widget anchor, font, scale and logical bound is retained. Heading,
item and hint shadows have zero alpha, while their existing authored offsets
still participate in layout bounds. The duration `OptionRow` retains its label
and value positions and existing fitting path, using a local style copy with
transparent shadows and the paper inks. Startup requires positive value-column
space within the usable paper interior; each duration refresh checks the actual
compound label/value bounds with the full card and arrow reserve. This changes
visible artwork without moving controls, changing text, introducing new choices
or adding RNG draws.
The paper uses the existing shared solid texel with point sampling; the text
layer remains linear-filtered. Each page captures its setup-art copy when it
opens; reopening adopts edited values.

## Join paper

Join reuses `main_menu.setup_box_size` and the captured setup-art copy described
above, including the focus-padding and shadow-offset overrides. Its solid
surround uses `main_menu.home_background_colour`. There are no new data fields
or artwork assets. Join has no shared focus cursor, so `MenuPaperArtwork` draws
the outer panel without a focused-row card or detached arrow. The controls
footer remains visible and the legacy brand is suppressed.

Connected player rows and the existing claim/release gutter stripe use captured
`focus_ink`; unclaimed rows and reserved missing devices use `disabled_ink`.
Heading and status use ordinary `ink`. Heading, item and status shadows have
zero alpha but retain their authored offsets, fonts, anchors and logical
bounds. Device/roster labels, reconnect suffix and status copy remain unscaled;
their words come from `menu_text.labels` and `menu_text.connection`.

Page checks use the actual generated row labels and captured gutter rectangles,
plus heading and status bounds, to require containment in the usable paper
interior. They also preserve the heading, player rows and status in that
vertical order. These checks retain text-shadow extents and add no fitting or
wrapping. The paper and gutter use point sampling; lettering stays linear.
Roster claims, stable empty rows, readiness, reconnect, Back/rescue,
destinations, RNG and audio retain their existing behavior. The bounded gutter
motion and its Reduce effects/interruption policy are unchanged.

## Shared paper focus and press motion

The required `menu_style.motion` object controls the shared paper focus arrow,
accepted-press response and setup-choice accents:

| JSON path | Shipped value | Use and accepted range |
|---|---|---|
| `menu_style.motion.focus_duration` | `0.14` seconds | Arrow settling time, greater than zero and at most `0.5` seconds |
| `menu_style.motion.press_duration` | `0.12` seconds | Card release time, greater than zero and at most `0.5` seconds |
| `menu_style.motion.arrow_travel` | `1.5` keyline widths | Initial inward arrow offset, from `0` to `2` |
| `menu_style.motion.press_fraction` | `0.65` | Initial card depression as a fraction of its existing shadow offset, from `0` to `1` |
| `menu_style.motion.selection_inset_fraction` | `0.04` | Setup-choice inset on each side as a fraction of the icon's shorter side, from `0` to `0.08` |

Every value must be finite. A zero travel, press or selection-inset fraction
disables that part of the drawing. Each page captures these values when
constructed, so reloading presentation does not change an in-flight response;
reopening uses the new values. Paper colours, keyline width, focus padding and
shadow offset retain their page-specific art records.

The arrow responds to an actual change between visible, enabled focused widgets;
initial focus is settled. On retained pages, successful activation such as Apply
and permitted left/right value adjustment restart one visible press response.
The paper face moves within the existing card envelope, leaving labels and
widget/navigation bounds fixed. Title proceed, transitions and pops run
immediately, before an outgoing-card press can be shown. Repeated draws sample the same state; only
positive, finite active updates advance the timers. Suspension, deactivation,
resume and developer input reset clear interrupted responses.

The applied Reduce effects preference suppresses both ornaments immediately
and retains the settled arrow and card. Pending Controls / audio edits take
effect on Apply through the existing preference policy. Turning Reduce effects
off starts settled; the next ordinary event can animate. Title/Home,
Options/Credits, Mode/Player Count, Controls / audio, Pad sticks,
Pause/confirmation and End use this shared drawing. Results has no shared focus
cursor and stays static.

## Setup selection motion

Mode and Player Count use the shared paper focus arrow and press response above.
Their panel, lettering and navigation rectangles stay fixed. Accepted duration
changes briefly depress the focused card face; rejected actions do not create
an acknowledgement.

Team selection owns one motion clock per player row. An actual team-choice
change briefly contracts the chosen team icon inside its unchanged canvas.
Successful ready/unready changes acknowledge that row; a successful team
shuffle restarts one bounded response per row, including rows whose team does
not change. The existing selected frame, confirmation tint and team rules
still determine the visible choice.

Weapon selection uses the same per-row response on actual choice changes and
successful ready/unready changes. Only the tile and border contract, clamped
to their existing icon padding. The tool, paint mask, supply art, lettering and
player badge retain their positions. Initial choices start settled; repeated
navigation replaces the current response rather than stacking it. Confirmation
of the final player advances immediately with the existing random-choice
resolution and transition timing.

The setup clocks share `focus_duration` and `press_duration`; their maximum
inset uses `selection_inset_fraction`. Only finite positive active updates
advance them, and drawing samples the same state. Applied Reduce effects
immediately draws the settled selection. Inactive or missing-player gating,
suspension, resume, deactivation, developer interception and input reset clear
interrupted responses. Input callbacks, readiness, navigation and widget bounds
do not depend on the drawing.

### Join and arena-stage motion

Join retains one response clock for each player row. An accepted claim or release
briefly draws a thin stripe in that row's existing left focus padding; it slides
and shrinks away using the captured press clock and existing accent width. The
final roster is refreshed before the response starts. Releasing and replacing a
seat in the same update restarts one response for that row. Empty rows retain
their positions, so another player's response does not move when a seat leaves.
Row lettering, connection status, heading, roster and widget bounds remain fixed.
The paper treatment above changes the stripe to captured focus ink without
changing its gutter rectangle or clocks.

The stripe is absent at rest and with applied Reduce effects. A connection or
input-slot change quietly clears the old response; reconnecting does not pretend
that a new join was accepted. The existing readiness snapshot, final-claim
barrier and Player 1 Back behavior remain unchanged.

Arena selection moves only the existing briefing stripe inward within the
padding above its text. An actual browsed-slot change uses the captured focus
clock; successful lock and unlock use its press clock. Repeated events replace
one bounded response. The travel uses the existing accent width and is clamped
inside the briefing padding. Initial and reduced-effects stripes are settled.
Preview, briefing text, status, navigation, selection geometry and input timing
remain fixed. The final Random confirmation keeps its outgoing Random view and
music-exit behavior. Resolving the stage adds no motion response or visual
refresh and preserves the existing single Random selection draw. Accepted final
Start settles any unfinished lock or browse response before the outgoing fade,
without refreshing the displayed Random text/preview or adding a new pulse.

Both pages register these clocks with the shared interruption and Reduce effects
policy described above. No new presentation fields or artwork assets are needed.
All planned menu-motion surfaces are implemented, and the author accepted the
focus and selection motion on 2026-10-17
([`POL#10`](polish-plan.md#movement-and-interaction-feedback)).

## Team paper and cards

The required `main_menu.team_box_size` supplies Team Select's outer paper panel,
shipped as `(1840, 920)` and centered in
`(1920, menu_style.chrome.footer_y)`. This wider panel preserves the player
badges and card padding at their existing positions. Dimensions must be finite
and positive, at most 1840 wide and at most `footer_y - 40` high. The page
captures `main_menu.art` with the existing `setup_focus_padding` and
`setup_shadow_offset` overrides; row cards copy this art with their paper fill
set to `focus_fill`. The solid surround uses `home_background_colour`. There
is no shared focus cursor, focused-row card or detached arrow. The controls
footer remains visible and the legacy brand is suppressed.

Each static row card is the union of its actual player label, retained
transparent text-shadow extents, player badge and three choice-icon rectangles,
inflated by setup focus padding. Fonts, player/icon anchors and logical bounds
are unchanged. Heading, label and warning text use captured dark ink. Their
shadows have zero alpha, while the original shadow offsets still participate
in bounds. The required
`main_menu.team_select.warning_position`, shipped as `(150, 860)`, places the
existing warning below the roster. It must be finite, nonnegative and within
the design area above the footer. Generated player labels and the warning use
the captured `menu_text.labels` and `menu_text.connection` copy.

Team spacing and icon size must be finite and positive. Startup resource checks
validate actual heading/warning glyphs and retained shadow bounds. Runtime
checks require complete row cards inside the usable paper interior, rows after
the heading without overlap, labels ending before choice icons and the warning
following the complete cards. No new fitting or scaling path is introduced.
The existing icon motion and readiness tints remain independent of the static
paper; Reduce effects retains the settled choices. Texture artwork uses point
sampling and lettering stays linear. The shared solid texel reproduces the new
paper without a new runtime asset.

## Team-selection pictures

`main_menu.team_select.sheet` selects the team-choice atlas. The `frames` and
`selected_frames` objects each name three fixed roles: `a`, `center`, and `b`.
The selected frame marks the current choice; confirmation still changes its tint.
The shipped atlas is `sprite_sheet_1`, with these unchanged frame names:

| Role | `frames` | `selected_frames` |
|---|---|---|
| `a` | `team_select_a` | `team_select_a_selected` |
| `center` | `team_select_center` | `team_select_center_selected` |
| `b` | `team_select_b` | `team_select_b_selected` |

All seven identifiers are required and checked against loaded resources before
menus can open. The page captures its frame names with its atlas, so an open
page keeps a coherent asset set if presentation is reloaded or saved while it
exists. Opening the next page uses the new choices. Team selection, readiness,
shuffle behavior and frame tinting are unchanged.

Debug `tune` reports shared layout numbers as **next screen**. Rejected values
leave the live records and pending edits intact; strings remain outside the
numeric tuner. See [live tuning](debug-console.md#live-tuning).

## Weapon paper and tiles

The required `main_menu.weapon_box_size` supplies Weapon Select's outer panel,
shipped as `(1840, 920)` and centered in
`(1920, menu_style.chrome.footer_y)`. Dimensions must be finite and positive,
at most 1840 wide and at most `footer_y - 40` high. The wider panel preserves
the player badges at their existing anchors. It captures `main_menu.art` with
the setup focus-padding and shadow-offset overrides on the neutral Home
surround. The controls footer remains visible, with the legacy brand and shared
focus cursor/arrow suppressed.

The existing icon-border envelope becomes a stepped paper tile, shipped as
136 by 80 design units, preserving the 125-unit row stride without an enclosing
row card. The tile uses the captured catalog's sheet and fill frame, including
custom replacements, and the shared paper primitive's existing layers and
premultiplied colours. Its pen uses `menu_style.chrome.rule_height` (`2`), its
shadow uses the captured setup shadow offset (`2`), and its face retains the
live unready/ready white/mint tint. Existing choice/ready/unready motion
contracts only this tile and border; source-fitted tool, paint and final
supply-strip layers stay fixed.

A fixed cream canvas uses the existing menu heading colour beneath the preview.
It covers the original icon rectangle plus keyline bleed bounded by the smaller
of the rule width and minimum icon padding (`2` shipped), within existing
padding. The animated readiness surround stays outside this fixed canvas,
preserving protected inner pixels and all artwork/input geometry during motion.

Heading, player labels and weapon names use captured dark ink, with focus ink
for confirmed names. Descriptions retain the authored
`main_menu.weapon_description.colour`, now shipped as `1B1C20`. All text-shadow
alphas are zero, retaining their original offsets and logical bounds, including
the player label's default `(2, 2)`. Fonts, anchors, scales and captured
catalog/name/description/artwork relationships remain unchanged through refresh;
reopening adopts edits. Random palette preview remains neutral and the final
Random choice preserves its existing RNG draws.

Weapon spacing and icon size must be finite and positive. Resource validation
adds heading containment after the existing checks, preserving their diagnostic
priority. Runtime checks use actual text/shadow, badge, maximum tile-envelope,
name and description bounds to require paper containment, ordered rows and
separate columns. The complete contraction envelope must retain positive paper
faces. No new fitting path or runtime asset is introduced. Texture artwork
remains point-filtered and lettering linear-filtered.

### Weapon description wrapping

Descriptions wrap to the required `main_menu.weapon_description.max_width`,
shipped as `920`, which must be finite and positive. The page draws them with
`WrappedTextDropShadow` ([`game/states/wrapped_text_drop_shadow.h`](../game/states/wrapped_text_drop_shadow.h)),
a `UiTextDropShadow` that passes every string through `text_wrap::wrap`
([`game/text_wrap.h`](../game/text_wrap.h)) in its own font and scale. The wrap
is greedy and measured with the engine's `measure_text`; it only adds breaks at
spaces, keeps every authored `\n`, and never splits a word. Every shipped
description's widest line (`859` for the Mister) is inside the width, so each
draws exactly as authored and no pixel moved when wrapping was turned on.
Startup refuses a description holding a single word wider than the width,
naming the key and the word; the paper containment above still bounds the
wrapped block. Wrapping is opt-in per widget: every other menu text keeps its
authored breaks and width-only scale fitting. `scripts/acceptance/text_wrap.cpp`
pins the breaks, and the Weapon paper probe opens the real page with a
description that only fits once wrapped.

## Weapon-selection names and artwork

`main_menu.weapon_select.sheet` selects the weapon-choice atlas and
`main_menu.weapon_select.fill_frame` names the fill used for its stepped tile.
Both identifiers
are required and nonempty. Their shipped values are `weapon_paint` and `pixel`;
they are independent of the shared menu fill.

The required `main_menu.weapon_select.choices` object has six fixed keys:
`sprayer`, `sniper`, `roller`, `mister`, `bucket`, and `random`. Every choice must
provide all four string fields below, even when an optional layer is empty:

| Field | Meaning |
|---|---|
| `name` | Display name, normalized and checked for the font that draws it |
| `frame` | Required, nonempty base-art frame in the selected atlas |
| `paint_frame` | Team-coloured overlay frame; `""` disables the overlay |
| `supply_strip` | Supply animation strip whose final frame is shown; `""` disables it |

The shipped mappings preserve the former switch's choices:

| Key | `name` | `frame` | `paint_frame` | `supply_strip` |
|---|---|---|---|---|
| `sprayer` | `Sprayer` | `sprayer` | `sprayer_paint` | `sprayer_supply` |
| `sniper` | `Sniper` | `sniper` | `sniper_paint` | `""` |
| `roller` | `Roller` | `roller` | `roller_paint` | `""` |
| `mister` | `Mister` | `mister` | `mister_paint` | `mister_supply` |
| `bucket` | `Bucket` | `bucket` | `bucket_paint` | `""` |
| `random` | `Random` | `random` | `""` | `""` |

Names use the existing UTF-8 punctuation normalization and must be nonblank,
single-line printable text. The content range after normalization is U+0020 to
U+007E, and startup also checks every name against the actual
`main_menu.fonts.weapon_description` atlas. Unsupported glyphs, line breaks and
missing fields fail while the source key is available for the diagnostic.
The measured name width plus the absolute horizontal detail-shadow offset must
fit `main_menu.weapon_description.x_offset`, the finite, positive gap before
the description column. This uses the actual font's measurement rather than an
arbitrary character limit.

Startup resolves the sheet, fill, base frames and each nonempty optional layer.
Base frames must have positive source dimensions. A paint overlay must share
its base frame's canvas size and origin. A supply strip must have a positive
frame count and the same canvas size as its base frame; icons using a supply
strip require zero origin. These checks keep the fitted base artwork and its
team-coloured layers aligned.

The page captures the catalog when opened. An open page retains its names and
artwork together if presentation is reloaded or saved; reopening uses the new
catalog. These strings are edited in JSON and remain outside numeric `tune`.
The fixed keys continue to identify the existing weapon enums, so display-name
edits do not rename save or command identifiers. `random` is a display choice,
not a weapon-tuning record: confirmation still resolves it through the existing
random-primary selection, without changing selection order or RNG draws.

## Stage Select paper and briefing

The required `main_menu.stage_box_size` supplies Stage Select's outer panel,
shipped as `(1840, 920)` and centered in
`(1920, menu_style.chrome.footer_y)`. Dimensions must be finite and positive,
at most 1840 wide and at most `footer_y - 40` high. The page captures
`main_menu.art` with the existing setup padding and shadow overrides on the
neutral Home surround. The legacy brand and shared focus backing are suppressed;
the controls footer remains visible.

The `focus_fill` briefing artwork encloses the authored briefing rectangle and
the actual five briefing rows, status and hint bounds expanded by
`3 * keyline_width + shadow_offset`. This card is recomputed after actual
selection refreshes, so the live hint fit remains covered. Its logical layout,
540-by-665 fitting budget, fonts, anchors and widget bounds stay unchanged.
Runtime checks require complete outer-paper/card containment and separation
from the preview. The heading resource guard follows the existing guards to
preserve diagnostic priority.

The required `main_menu.stage_select.selected_ink`, shipped as `111216`, supplies
the live confirmed stage-name/status colour. Their unconfirmed colour remains
the live `unselected`, now shipped as `1B1C20`. The preview edge retains the live
mint `selected` colour. The briefing label and goal use captured focus ink;
the remaining static paper labels use captured art ink. Paper shadows have zero
alpha, retaining the authored shadow offsets and text extents. The Random
question mark retains its existing light colour and visible shadow on the
unchanged dark preview.

Name/hint fitting, captured text/fonts, eligibility, browse/lock/unlock,
reconnect, the existing final Random draw and latched outgoing view, input,
stripe motion/Reduce effects and music handoff keep their existing behavior.
StagePreview rendering and arena content are unchanged. The existing fill
supplies the new paper without runtime assets or an atlas recipe; textures
remain point-filtered and lettering linear-filtered.

## Shared headings and complete control legends

The required `menu_text` object stores the shared menu brand, eleven headings,
seventeen complete prompt-bar legends, thirty-nine row labels, a solo-practice
hint, a paused-Options palette-timing caption, five Title/Home strings,
three Controls/Pad Sticks help strings, eleven
fixed save-status messages, three shared setting values, two Pause confirmation
questions, one Home Exit question, one display-trial question and countdown,
three End completion headings, two Stage Select status messages,
four fixed Stage Select briefing strings, three fixed Stage Select cues and
two fixed Stage Select mode labels:

| JSON path | Required keys |
|---|---|
| `menu_text.brand` | One brand-line string |
| `menu_text.headings` | `home`, `options`, `credits`, `controls_audio`, `pad_sticks`, `players`, `join_players`, `mode`, `teams`, `weapons`, `arena` |
| `menu_text.prompts` | `title`, `select_back`, `options`, `controls`, `sticks`, `mode`, `join`, `team`, `team_shuffle`, `weapon`, `arena_browse`, `arena_locked`, `missing_player`, `pause`, `pause_missing`, `select`, `credits` |
| `menu_text.items` | `play`, `options`, `credits`, `exit`, `resolution`, `fullscreen`, `team_palette`, `controls_audio`, `apply`, `back`, `single_match`, `best_of_three`, `best_of_five`, `match_length`, `one_player_practice`, `two_players`, `three_players`, `four_players`, `resume`, `restart`, `quit`, `yes`, `no`, `change_teams`, `change_weapons`, `change_level`, `exit_to_main_menu`, `pad_sticks`, `music_volume`, `sfx_volume`, `view_shake`, `aim_marker`, `reduce_effects`, `pad`, `move_deadzone`, `aim_deadzone`, `restore_defaults`, `keep`, `revert` |
| `menu_text.solo_practice_hint` | One Mode Select hint string |
| `menu_text.options_palette_timing` | One persistent paused-Options palette-timing caption |
| `menu_text.title` | `wordmark`, `subtitle`, `start`, `no_controller` |
| `menu_text.home_subtitle` | One Home subtitle string |
| `menu_text.help` | `controls`, `pad_association`, `aim_cutoff` |
| `menu_text.status` | `options_pending`, `options_saved`, `options_save_failed`, `controls_pending`, `controls_saved`, `active_save_failed`, `sticks_pending`, `sticks_saved`, `options_reverted`, `options_display_failed`, `options_rollback_failed` |
| `menu_text.values` | `on`, `off`, `random` |
| `menu_text.confirmations` | `restart`, `quit`, `exit`, `display` |
| `menu_text.display_trial_countdown` | One countdown string with exactly one `{seconds}` token |
| `menu_text.end` | `match_complete`, `round_complete`, `series_complete` |
| `menu_text.stage` | `browse_status`, `locked_status`, `briefing_label`, `goal`, `solo_objective`, `versus_objective`, `random_navigation`, `locked_hint`, `browse_hint`, `solo_mode`, `single_match_mode` |

Every field is required. Each string uses the existing UTF-8 punctuation
normalization and the supported U+0020 to U+007E printable content range after
normalization. Text must be nonblank and single-line, except for the Title
wordmark, Controls/Pad Sticks help blocks and Stage Select objective and hint blocks'
authored line breaks described below. Startup checks the actual drawing font and measured bounds in the menu's
design coordinates.

| Text | Font | Position and lower boundary | Shadow allowance |
|---|---|---|---|
| Brand | `menu_style.chrome.brand_font` | `menu_style.chrome.brand_position`, above `menu_style.chrome.footer_y` | None |
| Headings | `menu_style.fonts.heading` | Page heading position, above `menu_style.chrome.footer_y` | `menu_style.shadow.heading_offset` |
| Complete legends | `prompt_bar.font` | `prompt_bar.position`, above 1080 | `prompt_bar.shadow_offset` |

Home, Options and Credits use their existing authored heading positions, and
Controls / audio and Pad sticks use `controls_settings.heading_position`. The
other headings use `menu_style.layout.position`. Brand, heading and prompt lines
reserve a right margin equal to `menu_style.chrome.brand_position.x`: measured
width plus the absolute horizontal shadow must be at most
`1920 - position.x - right_margin`.
Measured height plus the absolute vertical shadow must be at most
`lower_boundary - position.y`. Equality is accepted. Negative or nonfinite
positions, shadows extending beyond the left/top edges, and overflowing text
fail with the authored key in the diagnostic.
There is no character-count limit, and leading or trailing spacing is not
trimmed.

A prompt value is the entire displayed line, including keyboard equivalents,
spaces and separators. For example, the shipped `title` legend is
`A/Enter Start`; the `missing_player` legend is
`" Reconnect controller    B/Esc Change players"`, including its leading blank.
The shipped strings preserve the previous wording and spacing. Complete legends
are display data: changing `A/Enter` does not remap confirmation, and keys and
actions are not substituted into a format template at runtime. The fixed
heading and prompt keys identify existing C++ flows; page order, input handling
and transitions remain in code.

`MenuPage` captures the text and `PromptBarPresentation` when the page is
constructed. An open page keeps that paired text and prompt font when stage
confirmation, controller availability or pause state changes its legend.
Reloading presentation does not mix a new legend with the old page's prompt
look; reopening adopts the new values. Ordinary pages select a complete legend
through the typed prompt setter. The vector-of-button/action `PromptBar`
constructor remains available for developer tools and tests.

These strings are edited in JSON and remain outside numeric `tune`. The schema
adds no locale axis, automatic wrapping or runtime formatting language.

### Title and Home copy

Five required strings preserve the existing Title and Home copy:

| JSON path | Shipped text | Drawing font |
|---|---|---|
| `menu_text.title.wordmark` | `COLOUR\nWARS` | `main_menu.fonts.title` (144) |
| `menu_text.title.subtitle` | `MAKE A MESS. MAKE IT YOURS.` | `menu_style.fonts.detail` (36) |
| `menu_text.title.start` | `LET'S PLAY` | `menu_style.fonts.item` (48) |
| `menu_text.title.no_controller` | `Connect a controller` | `menu_style.fonts.item` (48) |
| `menu_text.home_subtitle` | `Paint the arena. Claim the win.` | `menu_style.fonts.detail` (36) |

The wordmark permits explicit LF line breaks (`\n` in JSON). Every authored
line must contain nonblank supported text; leading, trailing or consecutive
line breaks that create a blank line are rejected. CR, tabs and other control
characters remain invalid. The other four fields follow the shared single-line
rule. None of these strings introduces automatic wrapping or placeholders.

Startup checks every string against its actual font and requires finite,
positive measured bounds. Title text keeps its measured size, without fitting
or item shadows. Each Title widget is centered horizontally using its actual
measured width and retains its authored y position (`main_menu.title_y`,
`title_subtitle_y`, `title_start_y` or `title_no_pad_y`). Bounds must fit the
Title paper: `main_menu.title_box_size`, centered in
`(1920, menu_style.chrome.footer_y)`. The Start action also includes
`main_menu.art.focus_padding` in its bounds.

Actual measured heights must leave the wordmark, subtitle, padded Start action,
connection text and generated build label in that vertical order without
overlap. The build label remains `build_info::label`, generated by the build;
it is not authored text. Its existing `main_menu.build_id_y` and prompt-bar font
still participate in the Title layout check.

The Home subtitle stays at `main_menu.home_subtitle_position`, with its measured
detail-font size and no shadow or fitting. It must fit the footer-centered Home
paper, begin at or below the actual Home heading's bottom, and end at or above
the Play action's padded top. The Home heading and Play positions retain their
existing layout roles.

Title and Home use their captured `MenuText` and resolve widget fonts on
opening. An open page keeps its text, fonts, measurements and centered positions
after presentation changes; reopening adopts the edits. Text edits do not
change Title confirmation, connection-message visibility, Home navigation or
actions. The strings remain JSON settings outside numeric `tune`.

### Home, Options and Credits row labels

`menu_text.items` uses fixed roles for the existing rows and actions:

| Key | Shipped text | Use |
|---|---|---|
| `play` | `Play` | Home action |
| `options` | `Options` | Home action and Pause row 2 |
| `credits` | `Credits` | Home action |
| `exit` | `Exit` | Home action |
| `resolution` | `Resolution` | Options label |
| `fullscreen` | `Fullscreen` | Options label |
| `team_palette` | `Team palette` | Options label |
| `controls_audio` | `Controls / audio` | Home/paused Options action, row 3 |
| `restore_defaults` | `Restore defaults` | Options action, row 4 |
| `apply` | `Apply` | Options action, row 5 |
| `back` | `Back` | Options row 6; Credits, Mode and Player Count action |

These eleven roles are required and follow the same nonblank, single-line and
punctuation-normalization rules as the other shared text. Startup checks each
against `menu_style.fonts.item` and requires finite, positive measured bounds.
Plain actions retain their measured size. Their text bounds, expanded on each
side by the page's focus padding, must fit its paper rectangle. Home and Credits
use `main_menu.art.focus_padding`; Options uses captured `setup_focus_padding`.
That rectangle uses the page's existing `home_box_size`, `options_box_size` or
`credits_box_size`, centered within `(1920, menu_style.chrome.footer_y)`.
Positive Home/Options row spacing also limits the padded text height; Credits
Back has no row-spacing cap. Boundary equality is accepted. This checks the
outer paper rectangle and adds no text-shadow allowance to plain item text.

The first three Options labels keep the existing `OptionRow` fit. The available
width is `main_menu.options_value_x - label_position.x - focus_padding.x` and
the available height is `main_menu.options_spacing.y - 2 * focus_padding.y`.
The scale starts at one, shrinks to fit width, then shrinks further for height
if needed; it must remain finite and positive. The resulting bounds still
undergo the paper/focus checks. Long supported labels therefore remain valid
when their fitted bounds fit the row; there is no arbitrary character-count
limit.

The keys identify existing C++ actions and do not add or reorder rows. Labels
come from the page's captured `MenuText`, so an open Home, Options or Credits
page retains its current labels after presentation changes, and reopening
adopts the edits. The names are string settings edited in JSON, outside numeric
`tune`.

### Mode and Player Count labels

Eight required `menu_text.items` roles preserve the existing
mode-selection and player-count choices:

| Key | Shipped text | Use |
|---|---|---|
| `single_match` | `Single Match` | Mode row 2 |
| `best_of_three` | `Best of 3` | Mode row 3 |
| `best_of_five` | `Best of 5` | Mode row 4 |
| `match_length` | `Match length` | Mode duration-row label, row 5 |
| `one_player_practice` | `1 Player (practice)` | Player Count row 2 |
| `two_players` | `2 Players` | Player Count row 3 |
| `three_players` | `3 Players` | Player Count row 4 |
| `four_players` | `4 Players` | Player Count row 5 |

Both pages reuse `menu_text.items.back` at row 6. The separate required
`menu_text.solo_practice_hint` defaults to `Solo practice: choose Single Match`
at Mode row 7. Rows use the existing widget origin and each page's authored
spacing; these are display roles, with the existing choice order, actions and
input handling retained.

The same normalized, nonblank, single-line text rules apply. Mode and Player
Count labels use `menu_style.fonts.item` and retain
`menu_style.shadow.item_offset`. The solo hint uses `menu_style.fonts.detail`
with that same **item** shadow offset. The shadows now have zero alpha for the
paper treatment; their offsets and logical extents remain in layout checks.
Startup checks the actual font glyphs and the union of the measured text and
its item shadow using the design-line rule above: reserve the shared right
margin, stay above the footer, and keep the shadow inside the left/top design
edges. These rows retain their measured font size.

For each focusable Mode/Player Count row, the measured text height plus the
absolute item-shadow y offset must be at most the page's vertical spacing
(`main_menu.mode_select_spacing.y` or `main_menu.player_count_spacing.y`). This
retains the existing text and shadow extents within one row's stride. The
additional paper/card checks above include art padding, shade and arrow space.
`match_length` must additionally satisfy
`measured_width + abs(item_shadow.x) <= 2 * main_menu.mode_select_spacing.x`
to fit before its existing duration-value column. That is one complete
`OptionRow`, so this column check adds no inner focus padding. Boundary equality
is accepted.

The nonfocusable solo hint uses its detail-font measurement and item shadow
with the same footer, right-margin and left/top checks. It has no row-height
cap or focus-padding allowance.

The strings come from the page's captured `MenuText`; presentation changes
leave an open page's labels and hint intact, and reopening adopts the edits.
They remain outside numeric `tune`. The hint is a complete display string,
without placeholders or runtime formatting, and changes no practice-mode
behavior. The existing duration values remain separate from the authored
`match_length` label.

### Pause actions and confirmation labels

Five more required `menu_text.items` roles supply the plain Pause labels:

| Key | Shipped text | Use |
|---|---|---|
| `resume` | `Resume` | Initial Pause row 0 |
| `restart` | `Restart` | Initial Pause row 1 |
| `quit` | `Quit` | Initial Pause row 3 |
| `yes` | `Yes` | Confirmation row 0 |
| `no` | `No` | Confirmation row 1 |

Initial Pause row 2 reuses `options`, whose shipped text remains `Options`.
Controls/audio is nested inside that Options page. The shared Options label is
checked in its Home and Pause contexts. Confirmation retains its existing
default focus on No, and the labels
do not change resume, restart, quit or confirmation behavior.

The existing normalized, nonblank, single-line rules apply. Startup checks the
actual `menu_style.fonts.item` glyphs and finite, positive measured bounds.
Pause labels retain their measured font size and plain drawing style. Their
bounds expanded by `pause_menu.art.focus_padding` must fit the outer Pause
paper rectangle: `pause_menu.box_size`, centered in the full 1920 by 1080 design
area. The row-height rule is
`measured_height + 2 * pause_menu.art.focus_padding.y <= pause_menu.layout.spacing.y`.
Rows begin at `pause_menu.layout.position` and use its authored spacing.
Equality is accepted; no item-text shadow or fitting scale is added.

Each Pause page uses its captured `MenuText` and resolves the item font when it
opens. An open initial or confirmation page keeps its current labels through
presentation edits; reopening adopts the new text. These strings remain outside
numeric `tune`. Player-specific titles and connection status remain separate C++
text.

### Pause confirmation questions

Two required `menu_text.confirmations` strings supply the existing questions:

| Key | Shipped text |
|---|---|
| `restart` | `Are you sure you want to restart?` |
| `quit` | `Are you sure you want to quit?` |

They use the same normalized, nonblank, single-line content rules as the other
plain menu strings. Startup checks the actual `menu_style.fonts.detail` glyphs
and requires finite, positive measured bounds. The shipped font remains
`gill_sans_mt_bold_36`.

Questions keep their plain, unscaled drawing style. Their actual glyph rectangle
is centered horizontally within the full 1920 by 1080 design area at
`pause_menu.status_y`. It must fit the outer Pause paper, whose
`pause_menu.box_size` is centered in that same full design area. Its bottom
edge must be at or above the top of the first action expanded by
`pause_menu.art.focus_padding`. Equality is accepted. These checks use actual
unscaled bounds, with no shadow, wrapping, automatic shrinking or upscaling.

Each confirmation page takes its question from its captured `MenuText` and
resolves the detail font when it opens. An open confirmation keeps its question,
font, measured bounds and centered position through presentation replacement;
a newly opened confirmation adopts the edits. The complete strings stay outside
numeric `tune` and introduce no placeholders or runtime formatting. No remains
the first registered action and the safe default focus. Question edits do not
change navigation, Back/cancel, restart or quit actions, or interruption handling.

### End Menu actions

Four required `menu_text.items` roles supply the static End Menu actions:

| Key | Shipped text | Use |
|---|---|---|
| `change_teams` | `Change Teams` | Completed-match/series row 1 |
| `change_weapons` | `Change Weapons` | Completed-match/series row 2 |
| `change_level` | `Change Level` | Completed-match/series row 3 |
| `exit_to_main_menu` | `Exit to Main Menu` | Ongoing-series row 1; completed-match/series row 4 |

The configuration actions remain hidden during an ongoing series. The existing
continuation action at row 0 remains in code.
Editing these four labels does not change which actions are available, their
order, navigation, session continuation or music transitions.

The normalized, nonblank, single-line text rules apply. Startup checks the
actual `menu_style.fonts.item` glyphs and requires finite, positive measured
bounds. End labels retain their measured font size and plain drawing style.
Their bounds expanded by `end_menu.art.focus_padding` must fit the outer End
paper rectangle: `end_menu.box_size`, centered in the full 1920 by 1080 design
area. Rows begin at `end_menu.layout.position` and use its authored spacing.
The vertical spacing must be finite and positive, and each padded text height
must fit within it:
`measured_height + 2 * end_menu.art.focus_padding.y <= end_menu.layout.spacing.y`.
Both Exit positions are checked, along with all three configuration actions,
including when those actions would be hidden during a series. Equality is
accepted; these plain actions add no item-text shadow or fitting scale.

An End page takes its labels from the captured `MenuText` and resolves their
item font when it opens. An open page keeps its text and font after
presentation changes; reopening adopts the edits. The strings are edited in
JSON, outside numeric `tune`. Session-dependent continuation text remains
separate C++ text.

### End Menu completion headings

Three required `menu_text.end` strings supply the existing completion headings:

| Key | Shipped text | Use |
|---|---|---|
| `match_complete` | `Match Complete` | Single match |
| `round_complete` | `Round Complete` | Ongoing series, including a drawn round |
| `series_complete` | `Series Complete` | Completed series |

The normalized, nonblank, single-line content rules apply. Startup checks the
actual `menu_style.fonts.heading` glyphs and finite, positive measured bounds.
The shipped heading font remains `gill_sans_mt_bold_72`.

Headings keep their plain, unscaled drawing style. Their actual glyph rectangle
is centered horizontally within the full 1920 by 1080 design area at
`end_menu.heading_y`. It must fit the outer End paper: `end_menu.box_size`,
centered in that same full design area, shipped as 980 by 820. The heading's
bottom edge must be at or above the first continuation action's padded top,
`end_menu.layout.position.y - end_menu.art.focus_padding.y`. Equality is
accepted. The heading itself has no added padding or text shadow, and no
wrapping, automatic shrinking or upscaling.

An End page takes its completion heading from its captured `MenuText` and
resolves the heading font when it opens. An open page retains its text, font,
measured bounds and centered position through presentation replacement;
reopening adopts the edits. These complete strings stay outside numeric `tune`
and introduce no placeholders or runtime formatting. The existing session
state selects the same single-match, ongoing-series and completed-series
branches. Heading edits do not change continuation/result/score composition,
available actions, navigation, focus, music timing or interrupted transitions.

### Controls / audio and Pad Sticks row labels

Nine required `menu_text.items` roles supply the existing settings labels:

| Key | Shipped text | Use |
|---|---|---|
| `pad_sticks` | `Pad sticks` | Controls / audio action, row 0 |
| `music_volume` | `Music volume` | Controls / audio fitted label, row 1 |
| `sfx_volume` | `SFX volume` | Controls / audio fitted label, row 2 |
| `view_shake` | `View shake` | Controls / audio fitted label, row 3 |
| `aim_marker` | `Aim marker` | Controls / audio fitted label, row 4 |
| `reduce_effects` | `Reduce effects` | Controls / audio fitted label, row 5 |
| `pad` | `Pad` | Pad Sticks fitted label, row 0 |
| `move_deadzone` | `Move deadzone` | Pad Sticks fitted label, row 1 |
| `aim_deadzone` | `Aim deadzone` | Pad Sticks fitted label, row 2 |

Both pages reuse `menu_text.items.apply` and `menu_text.items.back`, at Controls
rows 6/7 and Pad Sticks rows 3/4. All thirteen uses draw plain text with
`controls_settings.item_font`, shipped as `gill_sans_mt_bold_36`. This is a
separate context from `menu_style.fonts.item` (`gill_sans_mt_bold_48`) and from
the shared heading font (`gill_sans_mt_bold_72`). Shared Apply/Back labels are
checked in each font and page where they appear.

The normalized, nonblank, single-line rules apply. Startup checks actual font
glyphs and finite, positive measured text bounds. All eight setting labels keep
the existing `OptionRow` fit: starting at scale one, shrink to the label-column
width if needed, then shrink further to the available row height. The width is
`controls_settings.layout.spacing.x - controls_settings.art.focus_padding.x`;
the height is
`controls_settings.layout.spacing.y - 2 * controls_settings.art.focus_padding.y`.
The fitted scale must remain finite and positive. Long supported labels remain
valid when their fitted bounds fit, without a character-count limit.

The Pad sticks action and both pages' Apply/Back actions retain their measured
font size. Text bounds expanded by `controls_settings.art.focus_padding` must
fit the Controls paper, centered in
`(1920, menu_style.chrome.footer_y)` using `controls_settings.box_size`, and
their row height. For all thirteen uses, the padded right edge must be at most
`controls_settings.controls_position.x`, where the help and preview area
begins, and the padded bottom edge must be at most
`controls_settings.status_position.y`. These checks use the actual measured
label size after any fitting, rather than the nominal widget size. Paper,
row, help-column and status-line boundaries accept equality. Rows start at
`controls_settings.layout.position` and use its authored spacing. These labels
add no item-text shadow.

Each page takes its labels from captured `MenuText` and resolves its item font
on opening. Presentation changes leave an open page's text, font and fitted
label sizes intact; reopening adopts the edits. They remain JSON string
settings outside numeric `tune`. Labels do not change navigation, adjustment
steps, Apply/Back behavior, saved preferences or stick previews. Dynamic
connection and live readout strings remain separate C++ text.

### Controls / audio and Pad Sticks help

The required `menu_text.help` object supplies three fixed strings. All use
`controls_settings.help_font`, shipped as `gill_sans_mt_bold_24`; they do not use
the separate controls item font or shared menu detail font.

| Key | Use |
|---|---|
| `controls` | Multiline Controls / audio instructions |
| `pad_association` | Multiline explanation of Pad 1-4 profile association |
| `aim_cutoff` | Unscaled caption above the right-stick preview |

`controls` preserves this exact shipped text, including the authored line breaks
and spacing:

```text
CONTROLLER
Move / drop: left stick
Aim: right stick    Shoot: RT
Jump: A / LT (hold for height)
Pause: Start    Debug: Back / View
KEYBOARD + MOUSE
Move: A / D    Drop: S / Down
Aim: mouse cursor    Shoot: left click
Jump: Space (hold for height)
Pause: Esc    Debug: F1
MENUS
Navigate: stick / D-pad / arrows
Select: A / Enter    Back: B / Esc
AIM / READINESS
Direction only; roller aim is fixed.
Ready: chevron / Cooldown: bars
No paint: cross
Pad sticks: movement / aim deadzones.
REDUCE EFFECTS
Less shake, flashes and idle detail.
```

`pad_association` preserves these four lines:

```text
Profiles follow Pad 1-4 slots,
not the player number or device.
After reconnecting, check the slot.
Keyboard/mouse is unchanged.
```

`aim_cutoff` defaults to `Right stick: radial cutoff`. Both multiline fields
permit explicit LF line breaks (`\n` in JSON). After punctuation and nonbreaking
space normalization, each authored line must contain nonblank supported text.
Leading, trailing or consecutive line breaks that create blank lines are
rejected, as are CR, tabs and other control characters. The caption follows the
shared normalized, nonblank, single-line rule. No automatic wrapping or format
placeholders are introduced.

Startup checks actual help-font glyphs and finite, positive measured bounds.
The help region begins at `controls_settings.controls_position`, has size
`controls_settings.controls_size` and must fit the footer-centered Controls
paper. The Controls block starts at that region's top-left and retains its
existing fit within it. The association block starts at
`controls_settings.stick_preview.association_position`, at or below the aim
preview's bottom; its available size is
`controls_position + controls_size - association_position`. Each block's scale
is the minimum of one, available width divided by measured width, and available
height divided by measured height. The available area must be finite, positive
and inside the help region. The scale must be finite and positive; long
supported blocks remain valid when their fitted bounds fit their region,
without a character-count or minimum-scale limit. Neither block adds a focus
backing or text shadow.

The aim caption retains its measured size. Its x position is
`controls_settings.stick_preview.aim.x`; its y position remains the preview's
top minus the help font's measured `Move` height and
`controls_settings.art.focus_padding.y`. `Move` is a geometry probe, not another
display string. The caption must fit the right-hand help region, begin at or
below the movement-response preview's bottom, and end at or above the aim
preview's top. Its width can extend beyond the aim square within that region;
the square is not a text-width cap. Boundary equality is accepted.

Both pages use their captured `MenuText` and resolve help fonts when opened. An
open page retains its help text, font, fitted sizes and caption position after
presentation edits, including returning to Controls from Pad Sticks. Reopening
adopts the edits. These strings remain JSON settings outside numeric `tune`.
Changing the instructions does not remap input, alter profile association,
change saved settings or modify stick-response behavior.

### Fixed settings save-status messages

The required `menu_text.status` object supplies eleven complete messages for the
existing Options, Controls / audio and Pad Sticks status lines:

| Key | Shipped text | Use |
|---|---|---|
| `options_pending` | `Apply to save changes.` | Options value adjusted |
| `options_saved` | `Settings applied and saved.` | Options Apply succeeds |
| `options_save_failed` | `Settings active for this run. Could not save for next time.` | Options Apply cannot persist settings |
| `controls_pending` | `Apply to save changes for all players.` | Controls opens or a value is adjusted |
| `controls_saved` | `Applied and saved for all players.` | Controls Apply succeeds |
| `active_save_failed` | `Active for this run. Could not save.` | Controls or Pad Sticks Apply cannot persist settings |
| `sticks_pending` | `Apply to save pad deadzones.` | Pad Sticks opens or a deadzone is adjusted |
| `sticks_saved` | `Pad deadzones applied and saved.` | Pad Sticks Apply succeeds |

The three additional Options trial statuses are required complete authored
messages:

| Key | Use |
|---|---|
| `options_reverted` | Display trial reverted without committing the candidate |
| `options_display_failed` | Applying the display trial failed |
| `options_rollback_failed` | Restoring the previous configured display failed |

Each field follows the shared normalized, nonblank, single-line text rules.
These are complete display messages, with no placeholders or runtime
formatting. Options uses `prompt_bar.font`; Controls and Pad Sticks use
`controls_settings.help_font`. Both fonts ship as `gill_sans_mt_bold_24` but
remain separate contexts. Controls and Pad Sticks share the font and placement
of `active_save_failed`.

Startup checks actual font glyphs and finite, positive measured bounds. Status
text keeps its existing width-only fit: the scale starts at one and shrinks
only when the measured width exceeds the available width. That width is the
page paper's right edge minus its art shadow offset, horizontal focus padding
and status x position. Options uses `main_menu.options_status_position` and its
Options paper with compact setup geometry; Controls and Pad Sticks use
`controls_settings.status_position` and their shared paper/art. The available
width and fitted scale must be finite and positive. Long supported messages
remain valid when the fitted bounds fit; there is no character-count limit,
minimum scale, automatic wrapping, height-driven shrink or upscaling.

The actual fitted status rectangle, without focus padding or text shadow, must
fit the outer paper centered within `(1920, menu_style.chrome.footer_y)`. It
must not intersect the page's actual Back text rectangle expanded by that
page's focus padding. This is a two-dimensional rectangle check: Options'
shipped Back row and status line overlap vertically but remain horizontally
separate. Controls status must also avoid its actual fitted help block. Pad
Sticks status must avoid the actual fitted association block, the three preview
rectangles and the unscaled aim caption. Touching edges are accepted. These
checks use actual rectangles rather than an unconditional help-column width
cap; a long fitted message is valid when neighboring content ends above it.

Messages come from the page's captured `MenuText`, and the status font is
resolved when the page opens. An open page retains its text and font after
presentation edits, including messages set by later pending, successful or
failed Apply callbacks and returning from a child page. Reopening adopts the
edits. These strings remain JSON settings outside numeric `tune`. Wording does
not change success/warning colours, which preferences are applied, persistence,
failure handling, navigation or input reset behavior. Options still opens with
an empty status line until its first value adjustment or Apply. Status callbacks
continue to read live presentation geometry for their width fit, preserving the
existing behavior if that geometry changes while a page is open; fitted sizes
are not a geometry snapshot.

### Shared setting values

The required `menu_text.values` object supplies three existing setting values:

| Key | Shipped text | Use |
|---|---|---|
| `on` | `On` | Options Fullscreen; Controls View shake, Aim marker and Reduce effects |
| `off` | `Off` | The same four toggle rows |
| `random` | `Random` | Options Team palette's random choice; Stage Select's Random display name |

Each value follows the shared normalized, nonblank, single-line rules. Options
values use `menu_style.fonts.item`, shipped as `gill_sans_mt_bold_48`; Controls
values use `controls_settings.item_font`, shipped as `gill_sans_mt_bold_36`.
Startup checks actual font glyphs and finite, positive raw and fitted bounds in
each applicable context. The shared On/Off strings are checked in both fonts.
The nine settings contexts are Options On/Off at row 1 and Random at row 2,
plus Controls On/Off at each of rows 3, 4 and 5.

All five settings uses retain `OptionRow`'s existing value fit. Every value
change resets the scale to one, shrinks for the available width, then shrinks
further for the available height if necessary. Both limit components and the fitted scale must
remain finite and positive; there is no unscaled-width rejection,
character-count cap, minimum
scale or upscaling. Long supported values remain valid when their fitted bounds
fit. Options' available width runs from `main_menu.options_value_x` to the
Options paper's right edge minus the captured `setup_shadow_offset` and
horizontal `setup_focus_padding`; its available height is
`main_menu.options_spacing.y - 2 * main_menu.setup_focus_padding.y`.
The shipped limits are `(930, 86)`.

Controls values start in the second layout column. Their available width is
`controls_settings.controls_position.x - 2 * controls_settings.art.focus_padding.x -
controls_settings.art.shadow_offset - (controls_settings.layout.position.x +
controls_settings.layout.spacing.x)`;
their available height is
`controls_settings.layout.spacing.y - 2 * controls_settings.art.focus_padding.y`.
The shipped limits are `(332, 68)`. These are value limits, separate from the
first column's fitted labels. Values keep their existing plain text style and
colour.

The fitted value rectangle expanded by the page's actual focus padding must
fit its outer paper centered within `(1920, menu_style.chrome.footer_y)`.
The fitted text height plus twice the vertical focus padding must also fit the
page's row spacing. Controls' padded right edge must be at or before
`controls_settings.controls_position.x`, and its padded bottom edge must be at
or above `controls_settings.status_position.y`. All boundaries accept equality.
These checks use fitted bounds, without a raw width or height cap.

Each settings page takes these strings from its captured `MenuText`; each
`OptionRow` retains the font and fitting geometry it was constructed with.
Later callbacks displaying On, Off or Random retain the open page's text, font and limits after
presentation replacement, including return from a child page. Reopening adopts
the edits. The values remain JSON strings outside numeric `tune`. Wording
changes do not alter boolean choices, palette selection, cycling, disabled
fullscreen resolution, Apply/Back behavior, save identifiers or persistence.
Fixed palette labels remain in the existing palette data and are still read
live. Duration/default and numeric settings text, composed setup copy,
connection/readout text, player-specific Pause titles and session/results copy
remain in the [remaining text slices](#remaining-text-slices-gap84).

### Stage Select Random display name

Stage Select reuses the existing `menu_text.values.random` value for its Random
slot's display name. The shipped text remains `Random`; no JSON field or group
is added. The same string continues to supply Options' random team-palette
choice, with its separate `OptionRow` fit.

The Stage Select name uses `menu_style.fonts.item`, shipped as
`gill_sans_mt_bold_48`, and the existing `UiTextDropShadow` style. Its raw bounds
are the union of the actual font's text bounds at zero and those same bounds
translated by `menu_style.shadow.item_offset`. The existing width-only fit is
`min(1, main_menu.stage_select.preview_size.x / raw_width)`. Text and shadow use
that scale, and the shadow offset is scaled by the same amount. Long supported
names remain valid when their fitted bounds fit; there is no character-count
cap, minimum scale, wrapping, height-driven shrink or upscaling.

Startup checks the actual item-font glyphs and finite, positive measured text
and union sizes. Preview width, fitted scale and fitted union size must also be
finite and positive; shadow offsets and rectangle positions must be finite.
The fitted text rectangle starts at `main_menu.stage_select.name_position`
with the measured text size multiplied by the scale. The fitted shadow rectangle
has that same size and starts at the name position plus the shadow offset
multiplied by the scale. Their union includes negative shadow extents. The
complete fitted union must stay within x = 0 to 1920 and y = 0 to
`main_menu.stage_select.preview_position.y - menu_style.chrome.rule_height`.
Equality is accepted. These are fitted bounds: raw text wider than the preview
remains valid when its existing width-only fit satisfies the limits.

The page takes the Random name from its captured `MenuText`, and its name widget
retains the resolved item font, position and shadow colour. An open page keeps
those values through presentation replacement; reopening adopts the edits.
A visual refresh still uses live preview width and item-shadow offset,
preserving the existing behavior; fitted size and shadow offset are not page
snapshots. Name colours use live `stage_select.selected_ink` or `unselected`.
The
string stays outside numeric `tune`. It does not change eligible stages, cycling, lock/unlock,
confirm-again start, random selection or RNG draws. Numbered Arena navigation,
the Random availability count and composed best-of mode/roster text remain separate
C++ copy under GAP#8.4.

### Stage Select status messages

Two required `menu_text.stage` strings supply the existing selection status:

| Key | Shipped text | Use |
|---|---|---|
| `browse_status` | `FIND YOUR ARENA` | Unconfirmed stage choice |
| `locked_status` | `ARENA LOCKED` | Confirmed stage choice |

The shared normalized, nonblank, single-line content rules apply. Status text
uses `menu_style.fonts.detail`, shipped as `gill_sans_mt_bold_36`, with the
existing `UiTextDropShadow` style and no fitting scale. Startup checks actual
font glyphs and finite, positive measured text and union sizes. The text starts
at `main_menu.stage_select.briefing.position` plus `briefing_padding` plus six
times `briefing.spacing`; its shadow has the same size and starts at that
position plus `menu_style.shadow.detail_offset`. The complete union includes
negative shadow extents. The briefing panel and status union must have finite,
positive sizes, and their positions and shadow offsets must be finite. The
complete union must fit both the full briefing panel and the full 1920 by 1080
design area. Its bottom must be at or above the authored hint row's y position:
the briefing position plus padding plus seven times `briefing.spacing`.
Equality is accepted. This uses the full panel rather than a padding inset,
preserving the shipped status wording and layout. The status retains its
unscaled bounds without wrapping, automatic shrinking or upscaling. The hint
boundary is its authored row origin, rather than measured hint glyphs.

Each page takes these messages from its captured `MenuText`. Its status widget
retains the resolved detail font, position, shadow offset and shadow colour
from construction. An open page keeps those values through presentation
replacement, including when cycling or locking/unlocking refreshes its text;
reopening adopts the edits. A refresh reads the live
`stage_select.selected_ink` or `unselected` foreground colour. These complete
strings remain outside numeric `tune`, introduce no placeholders and do not
change eligible stages, random selection, RNG draws, navigation, confirmation,
unlocking or confirm-again start. Composed navigation/count hints and best-of mode/roster
strings remain separate C++ text under GAP#8.4.

### Fixed Stage Select briefing copy

Four more required `menu_text.stage` fields supply the fixed briefing copy:

| Key | Use | Briefing row | Lower row boundary |
|---|---|---|---|
| `briefing_label` | `MATCH SETUP` label | 0 | 1 |
| `goal` | `PAINT TO WIN` goal | 3 | 4 |
| `solo_objective` | Solo-practice objective block | 4 | 6 |
| `versus_objective` | Multiplayer objective block | 4 | 6 |

The two labels use the shared normalized, nonblank, single-line rules. Objective
blocks use the same punctuation normalization and printable range, with authored
LF line breaks between nonblank lines. CR, other control characters and empty
or blank lines are rejected. No fixed line count or automatic wrapping is
introduced. The shipped solo objective is:

```text
Make the arena your canvas.
Try weapons. Cover more ground.
```

The shipped multiplayer objective is:

```text
Cover the arena in your colour.
Most painted surface wins.
```

All four roles use the actual `menu_style.chrome.brand_font`, shipped as
`gill_sans_mt_bold_24`, and the existing unscaled `UiTextDropShadow` style with
`menu_style.shadow.detail_offset`. Startup checks actual font glyphs and finite,
positive measured text and union sizes. A role's text rectangle starts at the
briefing position plus padding plus its row multiplied by `briefing.spacing`;
the shadow has the same size and starts at the text position plus the signed
detail shadow offset. The complete union includes negative shadow extents and
must fit the full 1920 by 1080 design area. Its top and bottom must also stay
within the briefing panel's vertical span, and its bottom must be at or above
its authored lower row's origin, using the same briefing position, padding and
spacing. These common limits apply to all four roles. Equality is accepted.
The panel and union sizes must be finite and positive, and positions and
shadow offsets must be finite. These roles keep their unscaled bounds without
shrinking or upscaling.

The shipped solo objective measures 524 pixels wide including its shadow, from
x = 1262 to 1786. It extends 16 pixels beyond the briefing panel's right edge at
1770. The horizontal design-area boundary preserves this existing wording and
layout.

The paper artwork above encloses this overhang with pen/shadow clearance while
retaining the logical briefing rectangle and existing fitting budget.

Each page takes the four roles from its captured `MenuText` and chooses the
same solo or multiplayer objective during construction. The briefing widgets
retain their text, resolved brand font, position, shadow offset, shadow colour
and foreground ink through presentation replacement and selection refreshes;
reopening adopts the edits. These strings stay outside numeric `tune` and
introduce no placeholders or runtime formatting. They do not change match mode,
roster, stage eligibility, navigation, locking, starting, random selection or
RNG draws. Composed best-of mode/roster copy and navigation/count hints remain C++ text
under GAP#8.4.

### Fixed Stage Select mode labels

Two more required `menu_text.stage` strings supply the fixed mode labels:

| Key | Shipped text | Use |
|---|---|---|
| `solo_mode` | `SOLO PRACTICE` | One player, retaining precedence over the best-of value |
| `single_match_mode` | `SINGLE MATCH` | Multiple players with best-of equal to one |

Both use the normalized, nonblank, single-line rules. They retain the actual
`menu_style.fonts.detail`, shipped as `gill_sans_mt_bold_36`, and the unscaled
`UiTextDropShadow` style. The text starts at the briefing position plus padding
plus `briefing.spacing`, and its shadow has the same size at that position plus
`menu_style.shadow.detail_offset`. Startup checks the actual font glyphs and
finite, positive measured text and signed-shadow union sizes, with finite
positions and shadow offsets. The complete union must fit the full 1920 by
1080 design area and stay within the briefing panel's vertical span. Its bottom
must be at or above the authored row-two origin, calculated from the same
briefing position plus padding plus twice `briefing.spacing`. Equality is
accepted. The labels keep their unscaled bounds without wrapping, shrinking
or upscaling.

Each page takes these labels from its captured `MenuText` during construction
and retains the chosen text, resolved detail font, position, foreground ink,
shadow offset and shadow colour through presentation replacement and selection
refreshes. Reopening adopts the edits. The labels stay outside numeric `tune`
and introduce no placeholders. They preserve the existing solo-first branch,
match settings and mode selection. The multiplayer series branch continues to
compose `BEST OF ` with the best-of value in C++.

### Fixed Stage Select navigation and hints

Three more required `menu_text.stage` fields supply complete fixed cues:

| Key | Use |
|---|---|
| `random_navigation` | Random slot navigation, shipped as `<   SURPRISE ME   >` |
| `locked_hint` | Confirmed stage choice, including Random |
| `browse_hint` | Unconfirmed concrete stage choice |

Random navigation follows the shared normalized, nonblank, single-line rules.
The two hints use the same normalized authored-block rules as the objective
strings: printable text, LF breaks between nonblank lines and no automatic
wrapping. The complete shipped locked hint is:

```text
Confirm again to start.
Back to change your choice.
```

The complete shipped browse hint is:

```text
Left/Right to browse.
Confirm to lock your choice.
```

Random navigation uses the actual `menu_style.fonts.detail`, shipped as
`gill_sans_mt_bold_36`, and the unscaled `UiTextDropShadow` style at
`main_menu.stage_select.navigation_position`. Its signed shadow uses
`menu_style.shadow.detail_offset`. Startup checks actual font glyphs and finite,
positive measured text and union sizes, with finite positions and shadow
offsets. The union of the text rectangle and the same rectangle translated by
the shadow offset must fit the full 1920 by 1080 design area and end at or above
`menu_style.chrome.footer_y`. It must have no positive-area rectangle overlap
with either the full briefing panel or the preview rectangle expanded by
`menu_style.chrome.rule_height`. Touching edges are accepted. Navigation keeps
its unscaled text and shadow without automatic wrapping or fitting.

The hints use the actual `menu_style.chrome.brand_font`, shipped as
`gill_sans_mt_bold_24`, at the briefing position plus padding plus seven times
`briefing.spacing`. Their existing width-only fit measures the union of the
raw text rectangle at zero and the same rectangle translated by the signed
detail shadow offset. The width budget is
`main_menu.stage_select.briefing.size.x - 2 * main_menu.stage_select.briefing_padding.x`,
and the scale is `min(1, budget / raw_union_width)`. Both text and shadow use
that scale; the shadow offset is multiplied by it too. The fitted union is
formed from the text rectangle at the captured hint position with scaled text
size and a second rectangle of the same size at that position plus the scaled
shadow offset. Negative shadow extents are included. Long supported hints can
shrink for width; there is no character-count limit, minimum scale,
height-driven shrink or upscaling.

Startup checks the actual brand-font glyphs and requires finite, positive
measured text, raw union, width budget, fitted scale and fitted union sizes.
Rectangle positions and shadow offsets must be finite. The complete fitted
text-and-shadow union must fit the full briefing panel and the full 1920 by
1080 design area, with its bottom at or above `menu_style.chrome.footer_y`.
Equality is accepted. These checks use the separately transformed text and
shadow rectangles, preserving the existing width fit's floating-point bounds.

The page takes all three cues from its captured `MenuText`. Random navigation
retains its resolved font, position, foreground ink, shadow offset and shadow
colour. Each hint retains its resolved brand font, position, foreground ink
and shadow colour. An actual slot/state visual refresh keeps those values and
the captured text, while the hint's width budget and detail shadow offset read
live presentation values for the fit. Reopening adopts the edits. An idle
update does not force a visual refresh. The strings stay outside numeric
`tune`, introduce no placeholders and do not change navigation, eligibility,
confirmation, unlocking, confirm-again start, random selection or RNG draws.
Numbered Arena navigation and the composed Random availability-count hint
remain C++ text under GAP#8.4.

## Verification and boundary

The `menu_presentation` consumer case checks layout limits, required identifiers,
custom sparse atlases, resource errors and actual paper-art construction without
a graphics or audio device. The `weapon_catalog` case checks the weapon catalog's
required fields, display-text rules and resource/layer compatibility.
The `menu_text` case checks required shared headings/legends/row labels, text
normalization, font coverage and measured design/paper bounds, including fitted
Options and Controls/Pad Sticks labels, Mode/Player Count rows and solo
hint, and plain Pause actions/confirmation and End action labels. Pause questions
add required-key, normalization, detail-font, centered-paper and action-clearance
checks without a fitting scale. End completion headings add their required-key,
normalization, heading-font, centered-paper and continuation-clearance checks,
also without a fitting scale. Controls
labels use their separate item font, including shared Apply/Back checks and
width-then-height fitting. Title/Home copy adds required-key, authored-line,
actual-font, paper and vertical-order checks while preserving generated build
identity. The fixed Controls/Pad Sticks help adds required-key, normalized-line,
actual help-font and fitted-region checks. The unscaled aim caption uses its
actual bounds and existing probe-based position, with clearance between the
movement-response and aim previews; association text stays below the aim box.
Fixed save-status coverage checks required messages, normalization, actual
context fonts, width-only fitting, paper containment and separation from the
actual Back/help/preview rectangles.
Shared setting-value coverage checks required fields, normalization, actual
Options/Controls fonts and the existing value fit, including long values and
resetting the scale before shorter values, all nine contexts and actual
paper/row/help-column/status boundaries.
Stage Select's shared Random name adds actual item-font and text-plus-shadow
checks with its distinct width-only fit.
Its two fixed selection statuses add required-key, normalization, actual
detail-font and unscaled signed-shadow checks inside the full briefing panel
and design area, with the authored hint row as the lower boundary.
The four fixed briefing roles add required-key, normalized line/block, actual
brand-font and unscaled signed-shadow checks within the design area, the
briefing panel's vertical span and their authored lower row boundaries.
The two fixed mode labels add required-key, normalized-line and actual
detail-font checks using their unscaled signed-shadow bounds within the design
area, panel vertical span and authored row-two boundary.
Fixed cue coverage checks the navigation line and authored hint blocks against
their actual detail/brand fonts and signed shadows, including the hints'
distinct width-only fit. It also checks navigation's design/footer bounds and
separation from preview and briefing rectangles, plus fitted hint containment
in the full panel/design area and above the footer.
Debug-console acceptance checks layout edit scopes, rollback and reset.

The native painter checks exercise real page construction, copied geometry,
page overrides, keyboard navigation, focus, settings persistence, team setup,
results and pause/end transitions. Existing matrices check text/panel bounds,
frozen redraw and resizing at 720p, 1080p, QHD, 4:3, 16:10 and ultrawide sizes.
These are automated layout checks; human readability, physical-controller
comfort and performance acceptance remain separate TODO items.

Run `scripts/run-acceptance.ps1 -Preset x64-debug` (and `x64-release`), then use
the focused `MainMenuOnly`, `SettingsMenuOnly`, `ControlsMenuOnly`, `PauseMenuOnly`,
`ResultsMenuOnly`, `EndMenuOnly`, `TeamSetupOnly` and `MenuTextOnly` switches of
`scripts/check-painter.ps1` for the affected pages. `TeamSetupOnly`,
`MenuTextOnly`, `SetupMotionOnly`, `JoinStageMotionOnly`, `JoinPaperOnly`,
`TeamPaperOnly`, `WeaponPaperOnly` and `StagePaperOnly` use a scripted gamepad backend, including
deterministic claims, releases, connections and multiplayer stage input in the
Join/Stage scope. The other scopes retain the real gamepad backend.
`MenuTextOnly` retains its public keyboard checks as well.
`WeaponCatalogOnly` checks the catalog on real weapon pages, including page
snapshots across catalog replacement and reopening.
`MenuTextOnly` checks authored headings, complete legends and Home/Options/Credits
and Mode/Player Count row labels and hint on real pages, including their captured
text/look when a page updates its prompt or reopens. It also checks the shared
Random stage name with an eligible-stage fixture, including text/shadow fitting,
cycling, lock/unlock, confirm-again start and presentation replacement/reopening.
The same scope checks both authored Stage Select statuses, their retained
text/font/position/shadow through source replacement, live foreground colour
refresh and newly opened pages.
It also checks the four fixed briefing roles in solo and multiplayer setups,
including their retained text/font/geometry/shadow/ink through presentation
replacement and selection refreshes, plus newly opened pages. Its scripted
gamepad fixture supplies connected participants for the two-player briefing.
The full painter without the scripted-pad test macro checks the one-player
briefing and explicitly skips that two-player fixture. Physical-pad acceptance
remains open.
The focused scope also checks the two fixed mode labels, their retained
text/font/geometry/shadow/ink and reopening, solo precedence and the unchanged
best-of composition.
The same focused scope checks the fixed Random navigation and locked/browse
hints, retained text/font/position/ink/shadow properties, live hint fit geometry
on actual selection refreshes, reopening and the unchanged composed navigation
and Random-count branches.
`PauseMenuOnly` exercises the authored initial/confirmation labels and questions
through repeated and interrupted pause, restart and quit flows, including safe
No focus and question/detail-font snapshots across presentation replacement and
newly opened confirmations. `ResumeOnly` retains the match-resume transition
checks.
`EndMenuOnly` checks the four authored actions, both Exit positions, all three
completion headings and page text/font snapshots through repeated and
interrupted end-page flows. Heading checks include a drawn round in an ongoing
series, reopening after presentation replacement and the unchanged continuation
choices.
Use `ControlsMenuOnly` for the authored Controls / audio and Pad Sticks labels,
their fitted columns and retained text/font across presentation replacement,
returning from the child page and reopening. The same scope covers the authored
help blocks' fitting and the aim caption's retained text/font and placement.
`SettingsMenuOnly` and `ControlsMenuOnly` exercise authored status messages
through pending edits, successful and failed isolated saves, interrupted
preferences flows, presentation replacement, child-page return and reopening.
They check the retained text/font without treating live geometry as captured.
Those scopes also exercise authored On/Off/Random values, width/height fitting,
retained callback text/font/limits, reopening, cycling and isolated save/discard
flows.
Use `MainMenuOnly` for Title/Home wording, centering, focus and navigation,
including retained text/font across presentation replacement and reopening.

`JoinStageMotionOnly` checks real Join and Arena pages with deterministic input,
including accepted and repeated claims/releases, retained row positions,
connection changes, browse and lock/unlock responses, interrupted and reduced
motion, stable redraw and unchanged geometry. It also checks that final Random
confirmation keeps the outgoing view and the existing RNG/transition behavior.
The focused motion checks provide implementation evidence; human motion review,
physical-pad comfort and reference-match acceptance remain open.

`SetupPaperOnly` uses the normal gamepad backend and public keyboard input for
the actual Mode and Player Count paper pages. The existing `MenuTextOnly` and
`SetupMotionOnly` scopes retain their scripted reader for text snapshots and
setup motion; their Mode/Player Count checks use the shared paper card/arrow.

`JoinPaperOnly` uses the scripted reader to exercise the actual Join paper page
with deterministic devices, claims, releases and reconnections. Its raster and
layout checks retain the generated text, original gutter geometry and existing
roster/readiness/transition behavior. It does not establish physical-pad or
human readability acceptance.

`TeamPaperOnly` uses the scripted reader for actual Team Select pages with one
to four players. It checks complete paper/card composition, readiness, warning
placement, unchanged choice motion and the existing team/shuffle/recovery
behavior. Scripted devices provide implementation evidence; physical-pad and
human readability acceptance remain open.

`WeaponPaperOnly` uses the scripted reader for actual Weapon Select pages. It
checks the paper/tile composition and the retained catalog, preview, readiness,
motion and selection behavior without establishing physical-pad or human
readability acceptance.

`StagePaperOnly` uses the scripted reader for real Stage Select pages. It
compares full frames and exact StagePreview output across shipped arenas,
eligible Random and solo/multiplayer setups, retaining browsing, confirmation,
live ink/fit refresh, captured content, accent motion and latched music-exit
behavior. These checks do not establish human readability, physical-pad,
listening or performance acceptance.

The non-menu primitive texel retains its own startup checks. Together with
[gameplay tuning](gameplay-tuning.md), the shared layout and sheet choices define
the data boundary implemented for gap-audit 8.5. The weapon catalog implements
the name/artwork part of GAP#8.4, and `menu_text` implements shared headings,
control legends, Home/Options/Credits, Mode/Player Count, Pause, static End and
Controls/Pad Sticks row labels and help, the solo-practice hint, fixed
Title/Home copy, fixed settings save-status messages, shared On/Off/Random
setting values, Stage Select's shared Random display name, fixed selection
statuses, fixed briefing copy, fixed mode labels and fixed navigation/hints, fixed Pause confirmation questions and End
completion headings, Join/Pause connection messages and player-specific Pause titles,
the Team Select warning, Pad Sticks connection/raw/response readouts, and shared
menu device/roster labels including the packed Pause missing-pad list.
Other setting values,
composed stage navigation/count hints, best-of mode and roster summaries,
session continuation text, composed match/results text and other
player-visible C++ text remain open under that item. Font
regeneration, wrapping and locale selection remain separate work. The historical
audit is unchanged.

## Connection messages and player-specific Pause titles

Eight required `menu_text.connection` fields supply the existing copy:

| Key | Shipped text |
|---|---|
| `join_reconnect_suffix` | ` - Reconnect` (including its leading space) |
| `join_ready` | `Everyone joined. Player 1 can continue.` |
| `join_waiting` | `Press A / Enter on each device to join. Reconnect reserved pads.` |
| `pause_title` | `Pause Menu` |
| `pause_player_title` | `Pause Menu P{player}` |
| `controller_lost` | `Controller lost` |
| `all_connected` | `All players connected.` |
| `teams_required` | `Both teams need a player` |

These use the shared normalized, nonblank, single-line rules. Whitespace in the
reconnect suffix is retained. The player-title template requires exactly one
`{player}` and no other braces. Its formatter accepts only visible player
numbers 1 through 4. Pause resolves the participant's roster index before
formatting; an input slot does not determine the displayed player number.

Join uses its captured item font and retained text/shadow bounds for generated
player/device labels plus the reconnect suffix, and its detail font for both
statuses. Startup checks all player numbers, ordinary device/waiting labels and
real reserved-pad reconnect combinations against the setup-paper inset and
heading/row/status order, including the acknowledgement strip. Keyboard/mouse
does not disconnect under the existing input policy. Pause headings use the
plain heading font, centered on the full design area; its connected status uses
the plain detail font. Fallback, all four player titles and controller-loss
headings must fit the Pause paper in order above status and the first action.
Team's warning uses its existing detail-font/shadow and warning-position guard.
Actual glyphs and finite positive measured bounds are required for each role.

Pages retain captured copy and resolved fonts through refreshes; reopening
adopts edits. Reconnect acknowledgement, readiness, input ownership, centering,
navigation and RNG are unchanged. Shared device/roster labels now come from the
bounded label fields below; Pause still assembles its missing-player list. This
adds only the bounded player-number substitution, with no general formatting
language.

Validation (2026-10-18): Debug and Release builds and all 37 consumer CPU cases
per preset pass. CPU coverage includes exact defaults, normalized whitespace,
required fields/types, malformed player tokens and numbers, missing glyphs and
inclusive full-text/shadow layout edges for every new role. The old Count
bottom-edge positive now checks Join's later status edge while retaining the
original Count overflow rejection; unrelated Weapon/Stage heading fixtures keep
their moved-origin Join legend short so the intended heading owns the failure.

The focused `ConnectionTextOnly` native scope passes on both presets: 89
independent literal-text/bounds comparisons and 136 zero-time page updates at
720p/1080p, with exactly eight compact text crops per preset. It exercises sparse
Join claims, neutral return without autojoin, fallback and all four Pause roster
numbers, interrupted Restart/controller loss, reconnect fresh-press Resume
gates, Team warning recovery, custom copy/fonts retained through real refreshes
and adopted/reversed on reopening, and frozen drawing/RNG. Debug/Release crops
and CSV are byte-identical; the contact sheet was inspected. Both isolated
published-bank/System-only-PATH startup checks pass with per-monitor v2 DPI
awareness. Failed fixture/compile epochs are retained beside the final receipts
under `out/validation/connection-text/`. This establishes no physical-pad,
listening, human readability/style or performance acceptance.

## Pad Sticks preview readouts

Six required `menu_text.stick_readouts` templates preserve the shipped preview
copy. `connected` and `disconnected` contain exactly one `{device}`; the caller
supplies a Pad 1-4 slot name. `raw_movement` and `movement_response` contain
exactly one `{value}`. Their defaults are:

```text
{device} - connected
{device} - disconnected
Left stick raw X: {value}%
Move response: {value}%
```

`aim_inactive` requires `{raw_x}` and `{raw_y}` exactly once:

```text
Raw X: {raw_x}%
Raw Y: {raw_y}%
No new aim
Raw: hollow
Aim: solid
```

`aim_active` additionally requires `{aim_x}` and `{aim_y}` exactly once:

```text
Raw X: {raw_x}%
Raw Y: {raw_y}%
Aim X: {aim_x}%
Aim Y: {aim_y}%
Raw: hollow
Aim: solid
```

Only those complete tokens are accepted; missing, repeated, nested or unknown
braces are content errors. The four short templates use the existing normalized
single-line rules. Both aim blocks use normalized nonblank LF-separated lines.
Direct formatter calls also reject malformed templates. Pad slots are 0-3;
each numeric argument is an independently bounded integer from -100 through
100. The caller retains `std::lround(value * 100.0f)`, its active/zero-aim branch,
and aim normalization. Percent signs are authored literals, and no input,
deadzone, rounding, persistence or preview drawing rule changes.

Resource validation checks all four device names in each connection state and
every permitted numeric substitution against the actual Controls help font.
It retains separate maximum pen and measured extents across token alternatives:
glyph bearing/clamp/advance steps are monotone, so each token needs only its 201
legal values, even with negative advances or several tokens on the same line.
The bounds cover the public formatter's independent arguments; they do not
assume the physically reachable stick vectors or that `100%` is widest.

All six readouts remain plain, unscaled text in their original positions. The
connection line ends before the raw caption, raw and response captions stay
above their tracks, and the full aim block fits beside the aim box and before
association help. Caption positions retain the existing measured `Move` probe
and focus-padding offset. Pages capture their copy and font, refresh values from
the existing input snapshots, and adopt edits only when reopened. The Pad row
and connection readout share the authored device label below. Both deadzone
setting percentage values remain for later text slices.

Validation (2026-10-19): both game builds and all 37 CPU cases per preset pass.
The text case checks the six defaults, normalization and malformed/nested tokens,
all 201 signed values on each argument, generated pad glyphs, full multiline
regions and exact inclusive edges. Independent measurements of all 40,401
inactive-aim value pairs use adversarial glyph bearings/advances; the widest
measured number and the number with greatest terminal pen differ, and the full
readout bounds still match. Active-aim extrema and same-line/reordered tokens
also pass. The source-review regression rejects nested fragments that could
otherwise manufacture a later required token.

`StickReadoutsOnly` passes on Debug and Release at 720p/1080p: 164 independent
literal-text raster/bounds checks and 107 page updates per preset, with exactly
eight compact crops each. It checks signed half-percent rounding, all four pad
slots, disconnected stale inputs, normalized aim and radial cutoff, inherited
and pending deadzones, discard without saving, captured custom templates/help
font through real refreshes and reopening/reversal, and frozen drawing/RNG.
Both presets' CSV/crops are byte-identical and the contact sheet was inspected.
Both isolated published-bank/System-only-PATH startup checks also pass with
per-monitor v2 DPI awareness. Corrected fixture failures and final receipts are
retained under `out/validation/stick-readouts/`; no physical-pad, listening,
human readability/style or performance acceptance is claimed.

## Shared menu device and roster labels

Four required `menu_text.labels` fields preserve the existing menu wording:

| Key | Shipped text | Required tokens |
|---|---|---|
| `pad` | `Pad {pad}` | One `{pad}` |
| `keyboard_mouse` | `Keyboard/mouse` | None |
| `waiting` | `Waiting to join` | None |
| `roster_row` | `{player} - {device}` | One of each |

Loading normalizes supported punctuation and spaces; each field must be
nonblank and single-line. Directly mutated formatter strings must already be
printable ASCII with the same text rules. Tokens are scanned once in
the original template: missing, repeated, nested, unknown or unmatched braces
are content errors, including direct formatter calls. Authored outer spaces are
retained. The device formatter accepts unclaimed `-1`, pad slots `0` through
`3`, and keyboard/mouse `4`; the pad display number is one through four. The
roster formatter accepts seat indices zero through three and retains the
canonical `P1` through `P4` badge identity, independently of its assigned device.
The HUD/world device helper and badge identity grammar are unchanged.

Join, Team Select, Weapon Select, Pause and Results pass their captured text
explicitly to the roster helper. The settings model stores no presentation
pointer. Pad Sticks uses the same captured device label in its fitted Pad row
and unscaled connection readout. Refreshes retain copy/fonts; reopening adopts
edits. Pause still packs missing rows two per line, separated by three spaces
and LF. Results still appends the existing team description and truncated
one-decimal percentage. Claims, readiness, input, numeric formatting and RNG
are unchanged.

Resource checks use each consumer's actual font, shadow and paper geometry.
Join checks waiting, all devices and reserved-pad reconnect suffixes. Team and
Weapon check claimed rows and their badge/icon ordering; Weapon retains its
implicit `(2, 2)` shadow bounds even when the shadow is transparent. Pause
checks all 208 reachable ordered missing-pad lists. Results appends all 1,001
legal percentage strings, from `0.0%` through `100.0%`, in the font's exact
sequential float order; proportional glyphs can make another value wider than
`100.0%`. Each full row includes its zero-offset shadow and vertically centered
badge before the envelope is checked. Pad values retain OptionRow fitting, and both
connection states fit the existing unscaled readout region.

Validation (2026-10-19): both game builds and all 38 CPU cases per preset pass.
The text case covers required fields/types, normalized words, original-template
token guards and direct mutations, six device names and 24 roster outputs,
unsupported glyphs, complete two-line Pause lists, Weapon's exact two-unit
shadow edge and an independent 40,040-string Results measurement oracle with
nonuniform glyph metrics. Older synthetic-layout/font fixtures isolate unrelated
menu geometry while retaining their target edges and overflow rejections. Team's
former heading-contact fixture now rejects its impossible complete roster; a
separate inclusive full-card contact and quarter-unit overflow cover that edge.

`MenuLabelsOnly` passes on both presets at 720p/1080p: 107 independent literal
raster/bounds comparisons, 58 page updates and eight compact crops each. It
checks sparse/reordered claims, waiting and explicit joining, reconnects, packed
Pause lists, unchanged contribution percentages, actual captured fonts/copy
through assignment, refresh, reopening and reversal, fitted Pad values, and
frozen pixels/settings/save/RNG. CSVs and crops are byte-identical between
presets, and the contact sheet was inspected. Both isolated published-bank,
System-only-PATH startups pass with per-monitor-v2 DPI awareness.

The native captures precede a diagnostic-only follow-up that retains the
preceding Join envelope's contributing keys in overlap errors. Independent
review confirms unchanged bounds, comparisons, domains and valid rendering;
the final builds, CPU suites and startup checks use that follow-up. Original,
failed, diagnostic and final evidence remains under `out/validation/menu-labels/`.
These checks establish no physical-pad, listening, human readability/style or
performance acceptance.

## Remaining text slices (`GAP#8.4`)

**OPEN.** Preserve the existing wording and move it into presentation data in
bounded increments. Shared On/Off/Random values and the fixed labels described
above, shared menu device/roster labels and packed missing-pad lists are already
implemented. The next slices are:

1. **Composed setup:** Arena navigation and Random availability count, best-of
   mode and player/team roster lines, retaining their current numbers and order.
2. **Numeric settings:** duration units and Default notation, resolution and
   percentage values, and inherited/default deadzone notation. Preserve current
   numeric precision, rounding, choices and saved identifiers.
3. **Session and results:** continuation actions, round/results headings and
   proceed prompt, contribution/team descriptions and composed match/result
   lines. Retain reveal timing and solo/series distinctions.

Each increment adds only the fields and bounded substitutions its callers need;
there is no general formatting language. Required tokens must occur exactly as
the caller expects; reject missing, repeated or unknown tokens with a named
content error. Check actual font glyphs and measured bounds for the composed
output, including long supported text and the caller's numeric limits.

Keep the existing page capture/adoption behavior: a page retains its captured
copy where it does today, and reopening adopts edits. Acceptance covers actual
refreshes and reopening as well as malformed content. Navigation, readiness,
input ownership, RNG, gameplay values, persistence and results stay unchanged.
Use the current shipped copy; new wording or localisation is separate work.
