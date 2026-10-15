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
seventeen complete prompt-bar legends, thirty-six row labels, a solo-practice
hint, five Title/Home strings, three Controls/Pad Sticks help strings, eight
fixed save-status messages and three shared setting values:

| JSON path | Required keys |
|---|---|
| `menu_text.brand` | One brand-line string |
| `menu_text.headings` | `home`, `options`, `credits`, `controls_audio`, `pad_sticks`, `players`, `join_players`, `mode`, `teams`, `weapons`, `arena` |
| `menu_text.prompts` | `title`, `select_back`, `options`, `controls`, `sticks`, `mode`, `join`, `team`, `team_shuffle`, `weapon`, `arena_browse`, `arena_locked`, `missing_player`, `pause`, `pause_missing`, `select`, `credits` |
| `menu_text.items` | `play`, `options`, `credits`, `exit`, `resolution`, `fullscreen`, `team_palette`, `controls_audio`, `apply`, `back`, `single_match`, `best_of_three`, `best_of_five`, `match_length`, `one_player_practice`, `two_players`, `three_players`, `four_players`, `resume`, `restart`, `quit`, `yes`, `no`, `change_teams`, `change_weapons`, `change_level`, `exit_to_main_menu`, `pad_sticks`, `music_volume`, `sfx_volume`, `view_shake`, `aim_marker`, `reduce_effects`, `pad`, `move_deadzone`, `aim_deadzone` |
| `menu_text.solo_practice_hint` | One Mode Select hint string |
| `menu_text.title` | `wordmark`, `subtitle`, `start`, `no_controller` |
| `menu_text.home_subtitle` | One Home subtitle string |
| `menu_text.help` | `controls`, `pad_association`, `aim_cutoff` |
| `menu_text.status` | `options_pending`, `options_saved`, `options_save_failed`, `controls_pending`, `controls_saved`, `active_save_failed`, `sticks_pending`, `sticks_saved` |
| `menu_text.values` | `on`, `off`, `random` |

Every field is required. Each string uses the existing UTF-8 punctuation
normalization and the supported U+0020 to U+007E printable content range after
normalization. Text must be nonblank and single-line, except for the Title
wordmark and the Controls/Pad Sticks help blocks' authored line breaks described
below. Startup checks the actual drawing font and measured bounds in the menu's
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

The required `menu_text.status` object supplies eight complete messages for the
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
Options paper/art; Controls and Pad Sticks use
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
| `random` | `Random` | Options Team palette's random choice |

Each value follows the shared normalized, nonblank, single-line rules. Options
values use `menu_style.fonts.item`, shipped as `gill_sans_mt_bold_48`; Controls
values use `controls_settings.item_font`, shipped as `gill_sans_mt_bold_36`.
Startup checks actual font glyphs and finite, positive raw and fitted bounds in
each applicable context. The shared On/Off strings are checked in both fonts.
The nine checked contexts are Options On/Off at row 1 and Random at row 2,
plus Controls On/Off at each of rows 3, 4 and 5.

All five uses retain `OptionRow`'s existing value fit. Every value change resets
the scale to one, shrinks for the available width, then shrinks further for the
available height if necessary. Both limit components and the fitted scale must
remain finite and positive; there is no unscaled-width rejection,
character-count cap, minimum
scale or upscaling. Long supported values remain valid when their fitted bounds
fit. Options' available width runs from `main_menu.options_value_x` to the
Options paper's right edge minus `main_menu.art.shadow_offset` and horizontal
focus padding; its available height is
`main_menu.options_spacing.y - 2 * main_menu.art.focus_padding.y`.
The shipped limits are `(924, 84)`.

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

Each page takes these strings from its captured `MenuText`; each `OptionRow`
retains the font and fitting geometry it was constructed with. Later callbacks
displaying On, Off or Random retain the open page's text, font and limits after
presentation replacement, including return from a child page. Reopening adopts
the edits. The values remain JSON strings outside numeric `tune`. Wording
changes do not alter boolean choices, palette selection, cycling, disabled
fullscreen resolution, Apply/Back behavior, save identifiers or persistence.
Fixed palette labels remain in the existing palette data and are still read
live. Resolution and
numeric/percentage formatting, default-deadzone notation, stage Random copy,
connection/readout text and composed match/results/pause/end copy remain
separate work under GAP#8.4.

## Verification and boundary

The `menu_presentation` consumer case checks layout limits, required identifiers,
custom sparse atlases, resource errors and actual paper-art construction without
a graphics or audio device. The `weapon_catalog` case checks the weapon catalog's
required fields, display-text rules and resource/layer compatibility.
The `menu_text` case checks required shared headings/legends/row labels, text
normalization, font coverage and measured design/paper bounds, including fitted
Options and Controls/Pad Sticks labels, shadowed Mode/Player Count rows and solo
hint, and plain Pause actions/confirmation and End action labels. Controls
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

The non-menu primitive texel retains its own startup checks. Together with
[gameplay tuning](gameplay-tuning.md), the shared layout and sheet choices define
the data boundary implemented for gap-audit 8.5. The weapon catalog implements
the name/artwork part of GAP#8.4, and `menu_text` implements shared headings,
control legends, Home/Options/Credits, Mode/Player Count, Pause, static End and
Controls/Pad Sticks row labels and help, the solo-practice hint, fixed
Title/Home copy, fixed settings save-status messages and shared On/Off/Random
setting values. Other setting values, stage Random copy, connection/readout
text, composed match/results/pause/end text and other
player-visible C++ text remain open under that item. Font
regeneration, wrapping and locale selection remain separate work. The historical
audit is unchanged.
