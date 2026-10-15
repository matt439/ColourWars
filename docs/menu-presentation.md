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

## Weapon-selection names and artwork

`main_menu.weapon_select.sheet` selects the weapon-choice atlas and
`main_menu.weapon_select.fill_frame` names its solid tile frame. Both identifiers
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

## Shared headings and complete control legends

The required `menu_text` object stores the shared menu brand, eleven headings,
seventeen complete prompt-bar legends, twenty-seven row labels and a solo-practice
hint:

| JSON path | Required keys |
|---|---|
| `menu_text.brand` | One brand-line string |
| `menu_text.headings` | `home`, `options`, `credits`, `controls_audio`, `pad_sticks`, `players`, `join_players`, `mode`, `teams`, `weapons`, `arena` |
| `menu_text.prompts` | `title`, `select_back`, `options`, `controls`, `sticks`, `mode`, `join`, `team`, `team_shuffle`, `weapon`, `arena_browse`, `arena_locked`, `missing_player`, `pause`, `pause_missing`, `select`, `credits` |
| `menu_text.items` | `play`, `options`, `credits`, `exit`, `resolution`, `fullscreen`, `team_palette`, `controls_audio`, `apply`, `back`, `single_match`, `best_of_three`, `best_of_five`, `match_length`, `one_player_practice`, `two_players`, `three_players`, `four_players`, `resume`, `restart`, `quit`, `yes`, `no`, `change_teams`, `change_weapons`, `change_level`, `exit_to_main_menu` |
| `menu_text.solo_practice_hint` | One Mode Select hint string |

Every field is required. Each string uses the existing UTF-8 punctuation
normalization, must contain printable nonblank text on one line, and must use
the supported U+0020 to U+007E content range after normalization. Startup checks
the actual drawing font and measured bounds in the menu's design coordinates.

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

### Home, Options and Credits row labels

`menu_text.items` uses fixed roles for the existing rows and actions:

| Key | Shipped text | Use |
|---|---|---|
| `play` | `Play` | Home action |
| `options` | `Options` | Home action |
| `credits` | `Credits` | Home action |
| `exit` | `Exit` | Home action |
| `resolution` | `Resolution` | Options label |
| `fullscreen` | `Fullscreen` | Options label |
| `team_palette` | `Team palette` | Options label |
| `controls_audio` | `Controls / audio` | Options and Pause action |
| `apply` | `Apply` | Options action |
| `back` | `Back` | Options, Credits, Mode and Player Count action |

These ten roles are required and follow the same nonblank, single-line and
punctuation-normalization rules as the other shared text. Startup checks each
against `menu_style.fonts.item` and requires finite, positive measured bounds.
Plain actions retain their measured size. Their text bounds, expanded on each
side by `main_menu.art.focus_padding`, must fit the page's paper rectangle.
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
Count labels use `menu_style.fonts.item` and
`menu_style.shadow.item_offset`. The solo hint uses `menu_style.fonts.detail`
with that same **item** shadow offset, preserving its existing drawing style.
Startup checks the actual font glyphs and the union of the measured text and
its item shadow using the design-line rule above: reserve the shared right
margin, stay above the footer, and keep the shadow inside the left/top design
edges. These rows retain their measured font size.

For each focusable Mode/Player Count row, the measured text height plus the
absolute item-shadow y offset must be at most the page's vertical spacing
(`main_menu.mode_select_spacing.y` or `main_menu.player_count_spacing.y`). This
retains the existing overlap of focus backings while keeping the text and its
shadow within one row's stride.
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

Initial Pause row 2 reuses `controls_audio`, whose shipped text remains
`Controls / audio`. That shared label is checked in both its Options and Pause
contexts. Confirmation retains its existing default focus on No, and the labels
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
numeric `tune`. Player-specific titles, connection status and confirmation
questions remain separate C++ text.

### End Menu actions

Four required `menu_text.items` roles supply the static End Menu actions:

| Key | Shipped text | Use |
|---|---|---|
| `change_teams` | `Change Teams` | Completed-match/series row 1 |
| `change_weapons` | `Change Weapons` | Completed-match/series row 2 |
| `change_level` | `Change Level` | Completed-match/series row 3 |
| `exit_to_main_menu` | `Exit to Main Menu` | Ongoing-series row 1; completed-match/series row 4 |

The configuration actions remain hidden during an ongoing series. The existing
continuation action at row 0 and the session-dependent heading remain in code.
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
JSON, outside numeric `tune`. Session-dependent continuation and heading text,
and other composed End copy, remain separate C++ text.

## Verification and boundary

The `menu_presentation` consumer case checks layout limits, required identifiers,
custom sparse atlases, resource errors and actual paper-art construction without
a graphics or audio device. The `weapon_catalog` case checks the weapon catalog's
required fields, display-text rules and resource/layer compatibility.
The `menu_text` case checks required shared headings/legends/row labels, text
normalization, font coverage and measured design/paper bounds, including fitted
Options labels, shadowed Mode/Player Count rows and solo hint, and plain Pause
actions/confirmation and End action labels.
Debug-console acceptance checks layout edit scopes, rollback and reset.

The native painter checks exercise real page construction, copied geometry,
page overrides, keyboard navigation, focus, settings persistence, team setup,
results and pause/end transitions. Existing matrices check text/panel bounds,
frozen redraw and resizing at 720p, 1080p, QHD, 4:3, 16:10 and ultrawide sizes.
These are automated layout checks; human readability, physical-controller
comfort and performance acceptance remain separate TODO items.

Run `scripts/run-acceptance.ps1 -Preset x64-debug` (and `x64-release`), then use
the focused `MainMenuOnly`, `SettingsMenuOnly`, `ControlsMenuOnly`, `PauseMenuOnly`,
`ResultsMenuOnly`, `EndMenuOnly` and `TeamSetupOnly` switches of
`scripts/check-painter.ps1` for the affected pages. In that set, `TeamSetupOnly`
uses its scripted input backend; the other checks retain the real backend.
`WeaponCatalogOnly` checks the catalog on real weapon pages, including page
snapshots across catalog replacement and reopening.
`MenuTextOnly` checks authored headings, complete legends and Home/Options/Credits
and Mode/Player Count row labels and hint on real pages, including their captured
text/look when a page updates its prompt or reopens.
`PauseMenuOnly` exercises the authored initial/confirmation labels through
repeated and interrupted pause, restart and quit flows.
`EndMenuOnly` checks the four authored actions, both Exit positions and page
text/font snapshots through repeated and interrupted end-page flows.

The non-menu primitive texel retains its own startup checks. Together with
[gameplay tuning](gameplay-tuning.md), the shared layout and sheet choices define
the data boundary implemented for gap-audit 8.5. The weapon catalog implements
the name/artwork part of GAP#8.4, and `menu_text` implements shared headings,
control legends, Home/Options/Credits, Mode/Player Count, Pause and static End
row labels, and the solo-practice hint. Controls and stick-settings labels,
status/value text, composed match/results/pause/end text, title-screen and
home-hero copy, and other
player-visible C++ text remain open under that item. Font regeneration, wrapping
and locale selection remain separate work. The historical audit is unchanged.
