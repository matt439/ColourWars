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

## Verification and boundary

The `menu_presentation` consumer case checks layout limits, required identifiers,
custom sparse atlases, resource errors and actual paper-art construction without
a graphics or audio device. The `weapon_catalog` case checks the weapon catalog's
required fields, display-text rules and resource/layer compatibility.
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

The non-menu primitive texel retains its own startup checks. Together with
[gameplay tuning](gameplay-tuning.md), the shared layout and sheet choices define
the data boundary implemented for gap-audit 8.5. The weapon catalog implements
the name/artwork part of GAP#8.4; other player-visible C++ text remains open under
that item. It introduces no locale selection, font regeneration or text wrapping.
The historical audit is unchanged.
