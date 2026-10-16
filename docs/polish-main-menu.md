# Paint.exe main-menu pages

Implemented on 2026-10-15, part of `POL#16`. The title and home pages now use
the same pale stepped paper, dark ink, black keyline and quiet edge shade as
the match menus. A solid neutral surround replaces the textured backdrop on
these two pages. The title, subtitle, start action and build label center from
actual font measurements. Home keeps its existing heading and four actions;
Play has a persistent paper card, and a detached ink arrow follows focus.

`MenuPaperArtwork` draws both panels and their action cards from the existing
solid texel. The paper belongs to the texture layer, with point sampling;
lettering retains the text layer's linear sampling. These pages suppress the
shared brand line and legacy focus backing while retaining the common controls
footer. The remaining setup pages keep their existing artwork until their own
pass; the Mode/Player Count increment is recorded below.

Editable colours, pen width, shade and card padding live in
`presentation.json.main_menu.art`. `title_box_size` and `home_box_size` set the
panels centered above the shared controls footer. The title's `*_y` values set
its vertical rhythm; `home_position`, `home_spacing` and
`home_subtitle_position` set Home's layout.
Startup rejects malformed paper, unbounded geometry and anchors outside their
panels. Actual font and card fitting is checked with the renderer. Normal
builds reproduce the drawing without an atlas-generation step.

The panel and lettering stay fixed. The later
[shared focus and press motion](menu-presentation.md#shared-paper-focus-and-press-motion)
settles the detached arrow and pressed card face; Reduce effects retains the
settled artwork. Existing copy,
keyboard/pad prompts, title Start, Home Back, Play, Options, Credits and Exit
retain their behavior, as do music and effects bindings and gains. The keyboard
seat continues to make the title's controller hint unnecessary; its authored
position and font bounds are retained for that existing hint.

## Validation

Debug and Release build under `/W4 /WX` and each pass all 22 consumer
acceptance cases. Content variants reject malformed paper, invalid panel and
footer geometry, missing/nonfinite anchors and overlapping anchor order, while
retaining legal panel boundaries. The D3D11 hardware matrix passes in both
configurations on the NVIDIA GeForce RTX 5080, checking the actual pages
through `StateContext` and public keyboard input:

- Title Start, Home Back, all four Home focus cards/arrows, Play/Options/Credits
  destinations and Exit closing only the probe's own hidden window. Navigation
  supplies the existing neutral input required to arm each new page.
- Measured lettering and card separation at 720p, 1080p, QHD, 4:3, 16:10 and
  ultrawide, including the panel and build label clearing the controls footer.
- Native ink/paper contrast, independent comparisons against forced-linear
  title/Home lettering, and a forced-point negative control at 720p.
- Frozen redraws, draw randomness, static Reduce effects and exact resize
  round trips without changing focus.

The retained native 720p views show the [title](images/polish-main-menu/title-720p.png),
[Home with Play focused](images/polish-main-menu/home-720p.png) and
[Home with Options focused](images/polish-main-menu/options-720p.png).

The 53-file Release ZIP contains all 26 reviewed runtime assets byte for byte,
including the unchanged accepted bank. Isolated Debug and extracted Release
startup pass with a system-only PATH and graceful shutdown. A separate
extracted copy passes without its optional bank, with the expected single
silent-fallback diagnostic. No player's save is opened.

Evidence is under `out/acceptance/` and `out/validation/main-menu/`, with build,
acceptance and visual logs in `out/validation/main-menu-*.log`. Successful
renderer runs are Debug `01bdcf75043b4130909a578d8a9cc0e3` and Release
`429ebc53b8d84633a991ef3e7eeaa636`. Package evidence is in
`package-9105a56f9af248f8ba33f0f2143709a3/` below that evidence root. These scripted
checks and native image inspection do not establish physical-controller,
human movement/readability or listening acceptance.

Rerun from an x64 Visual Studio Developer shell:

```powershell
scripts/run-acceptance.ps1 -Preset x64-debug
scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -MainMenuOnly
```

Use `x64-release` for the Release checks. Team/Weapon selection cards and stage
preview remain under `POL#16`. Reference-arena
composition, movement feel, physical controllers and listening acceptance
retain their TODO owners.

## Options and Credits

Implemented on 2026-10-15, part of `POL#16`. Both pages use the existing
`MenuPaperArtwork`, solid neutral Home surround, dark plain lettering and
detached focus arrow. Their paper stays in the point-filtered texture layer;
lettering retains linear filtering. The common controls footer stays visible.

Options focus cards enclose the label and value together. Disabled fullscreen
resolution has separate readable label ink while its live dimensions remain
visible. The value column accommodates every fixed team-palette name; the
status line has room for the complete save-failure message in dark warning ink.
Credits retains every authored attribution and notice line above Back.

The editable layout is in `presentation.json.main_menu`: `options_box_size`,
`options_heading_position`, `options_position` (the first row),
`options_spacing`, `options_value_x` and `options_status_position`, plus
`credits_box_size` and the nested Credits heading, body and Back positions.
`art.disabled_ink` and `options_warning_colour` supply the extra inks.
Startup rejects invalid panels, anchors, row order and unusable row or column
space. Labels and values can fit narrower authored columns, and the Credits
body and status can fit their available areas; shipped text uses its natural
font size. No new atlas or generation step is required.

Keyboard and pad actions, staged display changes, Apply, save-failure behavior,
the retained windowed preset, Back discard and return from Controls/audio keep
their existing rules. Existing Credits copy and music/sound bindings are
unchanged. The Controls/audio paper increment is recorded below. Reduce effects keeps
both pages static and fully readable.

The retained native 720p views show [Options](images/polish-main-menu/settings-720p.png),
[the complete Credits page](images/polish-main-menu/credits-720p.png) and
[the save-failure status](images/polish-main-menu/settings-save-failed-720p.png).

Debug and Release build under `/W4 /WX`. All 22 consumer cases pass in each
configuration, with focused final content reruns after tightening usable row
and column bounds. Existing title/Home hardware matrices also pass in both
builds. The 53-file Release ZIP contains all 26 reviewed runtime assets byte
for byte; isolated Debug and extracted Release startup pass with the accepted
bank and a system-only PATH. A separate extracted copy passes its missing-bank
fallback with one expected diagnostic. No player's real save is opened.

Both D3D11 settings matrices pass on the NVIDIA GeForce RTX 5080. Actual pages
retain natural-size text and linear font sampling at 720p, 1080p, QHD, 4:3,
16:10 and ultrawide. Checks cover every enabled focus card and detached arrow,
palette values, complete Credits text, save success/failure, delayed Apply,
reload, discard, Controls/audio return and disabled-resolution wrap navigation.
Frozen redraw, drawing randomness, Reduce effects and exact resize round trips
also pass. The controlled write failure emits exactly its expected isolated-path
diagnostic and leaves the prior save intact.

Evidence is under `out/validation/settings-menu/` and `out/acceptance/`, with
logs in `out/validation/settings-menu-*.log`. Final settings runs are Debug
`5eeb7aeae8cd4ac6a53e0a03185dc599` and Release
`b81ed1fe12c34cdbbd72cd3841f0525e`. Package evidence is in
`package-659b7db52a22437a9300e4670a71b6b1/` below that evidence root.

Rerun the settings-page hardware check from an x64 Visual Studio Developer shell:

```powershell
scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -SettingsMenuOnly
```

Use `x64-release` for Release. The checks use keyboard input and isolated save
files; fullscreen metadata exercises disabled-resolution navigation while only
Off is applied to the offscreen windowed probe. Physical-pad, human readability,
movement-feel and listening acceptance remain with their TODO owners.

## Controls/audio

Implemented on 2026-10-15, part of `POL#16`. The page shared by Options and
Pause now uses the same pale stepped paper, dark ink and solid neutral surround.
Each setting's focus card contains its label and value, with a detached ink
arrow; Apply and Back use the same focus treatment. The complete existing
controller, keyboard, menu, readiness and reduced-effects legend remains
visible in the right column. Status messages use dark ink, with a separate
warning ink when a save fails. The common controls footer stays visible.

Editable geometry is in `presentation.json.controls_settings`: `box_size`,
`heading_position`, `layout`, `controls_position`, `controls_size` and
`status_position`. The dedicated `item_font` keeps eight setting/action rows
clear of one another. Its `art` uses the shared paper recipe with compact
vertical card padding and a smaller shadow. Settings use plain lettering;
the texture layer retains point sampling and the text layer linear sampling.
The legend fits its measured width and height without removing lines; labels,
values and status can fit narrower authored spaces. No new atlas is needed.

The six existing settings retain their ranges and staged Apply behavior.
SFX audition still uses the pending volume, including when the saved volume
is mute. Back discards unapplied changes. Returning to Options preserves its
pending display choices and focus; returning to Pause leaves the match paused.
The opening device and disconnected-device fallback retain their existing
rules. Reduce effects keeps the drawing static.

Debug and Release build under `/W4 /WX` and each pass all 22 consumer cases.
After the final status-width validation fix, both focused content cases pass
again, including coordinated layouts that previously allowed zero or negative
status space. Startup rejects malformed paper, missing fonts, nonfinite or
unusable geometry, overlapping rows/help and status positions outside the paper.

The D3D11 check uses the actual page and public keyboard input through
`StateContext`, with its own isolated save. It checks all eight focus positions
at 720p, 1080p, QHD, 4:3, 16:10 and ultrawide; complete labels, values, help and
status are compared with independently drawn linear text. The twenty-line help
measures 565 by 744.078 design units and fits at scale 0.940761. Settings and
status retain their natural font size. Checks also cover numeric clamps,
pending changes, Apply and reload, controlled write failure, both Back routes,
Options' retained pending choices/focus and return to an actual paused match.
Frozen redraw, draw randomness, Reduce effects and exact resize round trips
are included. Preview audio is muted for these raster/navigation checks.

Both focused D3D11 matrices pass on the NVIDIA GeForce RTX 5080. The Release
Options/Credits matrix also passes after the shared runner changes. The retained
native 720p views show [Controls/audio](images/polish-main-menu/controls-720p.png)
and its [save-failure status](images/polish-main-menu/controls-save-failed-720p.png).

The 53-file Release ZIP contains all 26 reviewed runtime assets byte for byte
and excludes authoring/debug/source files. Isolated Debug and extracted Release
startup pass with the accepted bank and a system-only PATH; a separate extracted
copy passes the missing-bank fallback with its single expected diagnostic.
Every process closes gracefully without opening the player's real save.

Evidence is under `out/acceptance/` and `out/validation/controls-menu/`, with
build, content, acceptance and visual logs in `out/validation/controls-menu-*.log`.
Successful controls runs are Debug `04a44aa6c4244b29ac0f85399c7d838d` and Release
`93499a368244445ca609c60b97d91517`. Package evidence is in
`package-99714b468c0a496da36d48f0c754fb29/` below that evidence root.

Rerun from an x64 Visual Studio Developer shell:

```powershell
scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -ControlsMenuOnly
```

Use `x64-release` for Release. Team/Weapon selection cards and stage
preview remain under `POL#16`. Physical-pad, movement-feel, human readability
and listening acceptance retain their existing TODO owners.

## Mode and Player Count

Implemented on 2026-10-16, part of `POL#16`. Both setup pages use the same
stepped pale paper, dark ink, quiet edge shade and neutral Home surround.
`MenuPaperArtwork` draws the panel and the enabled focused row's card, with a
detached ink arrow. There is no persistent primary-action card. The shared
controls footer remains visible; these two pages suppress the old brand line
and focus stripe. Texture artwork uses point sampling and lettering remains
linear-filtered.

The required `presentation.json.main_menu.setup_box_size` is shared by these
two pages and ships as `(1760, 920)`, centered in
`(1920, menu_style.chrome.footer_y)`. A copy of `main_menu.art` supplies the
colours, keyline and shade colour, with its card padding and shadow offset
replaced by the required `main_menu.setup_focus_padding` (`28, 7`) and
`main_menu.setup_shadow_offset` (`2`). The surround reuses
`main_menu.home_background_colour`.
The existing shared widget origin, `mode_select_spacing` and
`player_count_spacing` continue to place every row. Normal builds reproduce
the drawing from the existing solid texel without a new atlas or asset recipe.

Startup requires positive, finite panel dimensions, at most 1760 wide and with
20 design units clear above and below the panel within the footer area. Actual
heading, row and hint bounds must fit the usable paper interior, including
retained transparent-shadow extents and the focused card's padding, shade and
detached arrow. Each padded action card with extra shade clearance must fit its
row spacing. The drawing includes its shade inside the card; validation reserves
extra clearance. The tallest shipped glyph bounds, retained text-shadow extent,
compact setup padding and extra shade clearance total `99.5` design units within
the unchanged 100-unit row spacing. Padding must be positive and at most
`(48, 24)`, with each component greater than the
setup shadow offset plus the keyline width; the shadow offset must be in
`[0, 24]`. These checks preserve pressed-face, keyline and shadow clearance.

Existing heading/item/detail fonts, text, anchors, scales and widget bounds are
retained. Text shadows draw with zero alpha, preserving their authored offsets
and logical extents while making the lettering appear as plain ink. The Mode
duration `OptionRow` uses a local style copy with the same transparent shadow
and paper inks; its label/value positions and existing fitting path remain
unchanged. Startup requires positive value-column space within the paper
interior, and duration refreshes check the complete label/value bounds with
their card and arrow reserve. Disabled one-player practice has the shared
readable disabled ink.

The pages use the existing captured focus and press clocks: navigation briefly
settles the arrow and accepted duration changes depress the focused card face
inside its existing shadow. Repeated events restart one bounded response;
applied Reduce effects draws the settled artwork. The existing interruption
policy clears unfinished responses on inactive updates,
suspension/resume, deactivation, developer interception and input reset. Labels
and navigation geometry stay fixed, and drawing does not advance the clocks.

Single Match, series choices, duration cycling, solo-practice availability,
initial focus, Back, input-device claiming and immediate transitions retain
their behavior. The drawing changes no RNG, music or sound bindings.

The focused native check is
`scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -SetupPaperOnly` (or
`x64-release`), with batch evidence under
`out/validation/2026-10-16-setup-paper/verification.json`.

Debug and Release build under `/W4 /WX` and each pass all 31 consumer cases.
Eight native scopes pass per preset: `SetupPaperOnly`, `MenuTextOnly`,
`SetupMotionOnly`, `JoinStageMotionOnly`, `MenuMotionOnly`, `MainMenuOnly`,
`WeaponCatalogOnly` and `ResumeOnly`, for sixteen native runs. Each preset
passes 117 independent full-frame checks and 24 motion-envelope checks;
all 174 retained native PPM captures match byte for byte between Debug and
Release.

Both normal published-bank startup checks pass. The 53-file Release ZIP
contains all 26 reviewed runtime assets byte for byte and excludes source,
debug and private-audio content. Extracted Release startup passes with the
published bank and separately without the optional bank, using a system-only
PATH; the latter reports the expected silent fallback. These complete four
startup checks. Package and extracted-startup evidence is in
`out/validation/2026-10-16-setup-paper/package-verification.json`.

The retained native 720p views show [Mode](images/polish-main-menu/mode-720p.png),
the [focused duration row](images/polish-main-menu/mode-duration-720p.png) and
[series Player Count with solo practice disabled](images/polish-main-menu/player-count-series-720p.png).

Human menu/readability review remains open. Team/Weapon selection cards,
stage preview and accepted arenas remain `POL#16`; movement-feel (`POL#3`),
physical-controller and listening acceptance retain their existing gates.

## Join

Implemented on 2026-10-16, part of `POL#16`. Join uses the same stepped pale
paper, dark keyline, quiet shade and neutral Home surround as Mode and Player
Count. It reuses `main_menu.setup_box_size` and a captured copy of
`main_menu.art` with `setup_focus_padding` and `setup_shadow_offset` overrides;
there are no new presentation fields or artwork assets. Join has no shared
focus cursor, so its paper has no focused-row card or detached arrow. The
legacy brand line is suppressed and the controls footer remains visible.

Connected rows use the paper's focus ink; reserved missing devices and unclaimed
rows use its disabled ink. The heading and status use ordinary paper ink. Text
shadows have zero alpha while retaining all font measurements, authored offsets,
anchors and logical bounds. The generated device labels, reconnect suffix and
status copy stay unchanged and unscaled. They remain C++ copy under `GAP#8.4`.
Paper and gutter artwork retain point sampling; the lettering remains
linear-filtered.

An accepted claim or release keeps the existing bounded gutter stripe and clocks,
now drawn in focus ink. Its rectangle and travel remain within the original
left gutter. Stable empty rows, simultaneous claims, a same-update replacement,
quiet disconnect/reconnect, final-claim readiness barrier, Player 1 Back and
missing-Player-1 rescue retain their existing rules. Solo continues to Weapon
Select; multiplayer continues to Team Select. The drawing changes no input,
readiness, transition, RNG, music or sound behavior. Reduce effects suppresses
the acknowledgement; inactive updates and the existing interruption/reset policy
settle unfinished responses.

Page checks use the actual generated row labels, heading and
status bounds, including retained transparent-shadow extents and the captured
gutter. They require containment within the usable paper interior and preserve
heading, player rows and status in that vertical order. No new fitting or
wrapping path is added.

The focused native check is
`scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -JoinPaperOnly` (or
`x64-release`), using the scripted reader for deterministic devices and claims.
Batch evidence is under
`out/validation/2026-10-16-join-paper/verification.json`.

Debug and Release build under `/W4 /WX` and each pass all 31 consumer cases.
The `JoinPaperOnly` pair passes at 1280×720, 1600×1200, 1920×1080, 1920×1200,
2560×1080 and 2560×1440. Each preset passes 216 independent full-frame
comparisons: 180 roster/state, 27 motion/reduction/lifecycle and nine
snapshot/recovery checks. All 162 retained PPM captures match byte for byte
between Debug and Release.

The related `JoinStageMotionOnly`, `SetupPaperOnly`, `MenuTextOnly`,
`SetupMotionOnly`, `TeamSetupOnly`, `WeaponCatalogOnly` and `ResumeOnly` scopes
also pass in both presets, bringing the batch to sixteen passing native runs.

Four startup checks pass: the normal published-bank Debug/Release pair and
extracted Release with the published bank and separately with the optional-bank
silent fallback under a system-only PATH. The 53-file Release ZIP contains all
26 reviewed runtime assets byte for byte and excludes source, debug and private
audio. Package evidence is in
`out/validation/2026-10-16-join-paper/package-verification.json`.

The retained native 720p views show [waiting for players](images/polish-main-menu/join-waiting-720p.png),
the [complete roster](images/polish-main-menu/join-ready-720p.png) and
[a reserved controller needing reconnection](images/polish-main-menu/join-reconnect-720p.png).

Human menu/readability review remains open. Team/Weapon selection cards, stage
preview and accepted arenas remain `POL#16`; movement-feel (`POL#3`), physical-pad
and listening acceptance retain their existing gates.
