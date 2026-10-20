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

Use `x64-release` for the Release checks.
Reference-arena
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

### Options Restore defaults

Implemented on 2026-10-16, the restore-defaults part of `GAP#3.2`. Restore
defaults stages the canonical `SaveData{}` display and palette values: 1920x1080,
windowed and Random. It changes only the Options page's pending choices. Save
data, the application display, play preferences and pad profiles are unchanged
by the action. Display-changing Apply uses the Keep/Revert trial below;
palette-only or unchanged-display Apply retains the direct save path. Either
Back route discards the pending defaults. Returning from Controls/audio retains them.

Resolution availability still follows the applied fullscreen state. Restoring
pending windowed defaults while fullscreen keeps the resolution row disabled
and showing the live dimensions until Apply changes the display.

The page has seven rows: resolution, fullscreen, palette, Controls/audio,
Restore defaults, Apply and Back. Their existing first-row anchor is unchanged;
`options_spacing.y` is now `100`, and the status remains at y = `885`. Options
captures compact geometry from `setup_focus_padding` (`28, 7`) and
`setup_shadow_offset` (`2`), while its paper colours remain live in
`main_menu.art`. This local geometry override preserves the existing item font
and leaves the other pages' artwork unchanged. No numeric field, font or asset
is added.

The appended thirty-seventh menu item is the required
`menu_text.items.restore_defaults`, shipped as `Restore defaults`. It uses the
existing captured menu-text and plain-action bounds rules.

The focused native defaults matrix passes both presets: 104 independent
complete-frame references per preset (92 keyboard and 12 scripted pad-slot-3
references) across six native sizes/aspects, with 60 byte-identical
Debug/Release capture pairs. It checks canonical three-field staging without
changing the save file, window, play preferences or four pad profiles;
both discard routes; Controls/audio return and shared-preference merge;
repeated Restore/Apply; six controlled failed writes and recovery per preset;
reload, focus loss/held reconnect, Reduce effects, lifecycle resets and frozen
draw/RNG. These checks do not establish human, physical-pad, listening or
performance acceptance.

Human and hardware acceptance retain their existing gates.

### Home return focus

Implemented on 2026-10-16, the return-focus part of `GAP#3.2`. Both Back routes
from Options return to Home with Options focused; Credits returns with Credits
focused. The Back row and B/Escape take the same route for each page.

Home accepts one local initial-focus choice: Play, Options or Credits. It selects
that widget once at the end of initialization, after all actions are registered.
Play remains the default for Title, both Mode Select return paths and the
developer page factory. This is a bounded choice for the newly opened Home page;
it adds no saved navigation history, presentation field or artwork change.
The existing input rearming, navigation and immediate transitions remain in
effect. Returning from Controls/audio still preserves the existing Options page,
including its pending display/palette choices and focus.

The hidden `-HomeReturnFocusOnly` native matrix passes both presets: 108
independent complete Home references and 72 matching Debug/Release captures per
preset. It covers six keyboard sizes/aspects and sparse scripted pad slot 3 at
720p, including repeated returns, fresh confirmation, held-input interruption,
quiet entry, lifecycle resets, Reduce effects, resize and frozen draw/RNG.

Human and hardware acceptance retain their existing gates.

### Home Exit confirmation

Implemented on 2026-10-16, the Exit-confirmation part of `GAP#3.2`. Home's Exit
action pushes a `MainMenuExitConfirmation` child page, with No registered first
and initially focused. No or B/Escape resets actions and pops back to the same
Home page with Exit still focused. Back takes priority, and each update permits
one focus move before accepting a later confirmation, so simultaneous navigation
and confirmation cannot bypass the safe initial choice.

Yes alone starts the existing `finish_with_music` fade and application quit.
While exit is pending, the dialog suppresses ordinary input and freezes its
motion. Dismissing it through developer page control cancels the queued quit
callback before popping. The retained Home page, menu music ownership and
ordinary input rearming keep their existing behavior. Pause and End flows are
unchanged.

The dialog uses the neutral Home surround and existing `pause_menu.art`,
`box_size`, `layout`, `heading_y` and `status_y`, with the shared heading, item
and detail fonts. The heading reuses `menu_text.items.exit`; Yes and No reuse
their existing item labels. The new required `menu_text.confirmations.exit`
ships as `Exit ColourWars?`. No numeric presentation fields or artwork assets
are added. The shared paper motion and Reduce effects policy apply.

The hidden `-HomeExitOnly` native matrix passes both presets: 48 independent
complete confirmation references and 30 matching Debug/Release captures per
preset. Six keyboard sizes/aspects and sparse scripted pad slot 3 cover safe
No/Back returns, repeated entry, interrupted/reconnected and simultaneous input,
motion/reduction/lifecycle resets, frozen redraw/RNG, one pending music exit
and deliberate closure of the probe's own hidden window.

Human and hardware acceptance retain their existing gates.

### Options display Keep/Revert

Implemented on 2026-10-16, the resolution-revert part of `GAP#3.2`. Applying a
changed resolution/fullscreen preference or application-reported fullscreen state starts a
pushed display-trial child above the retained Options page. Palette-only or
unchanged-display Apply retains the direct path. The child owns the three
candidate display/palette values and snapshots the prior configured Save
resolution preset plus the prior application-reported fullscreen state from
`Application::fullscreen()`.

Revert is initially focused. Revert, B/Escape, timeout, focus loss, coverage,
developer reset or owner unwind cancels the trial. Save display/palette values
and file bytes remain unchanged until
Keep. A covered child resolves rollback but stays terminal until uncovered,
then closes only its own frame. Options can retain pending choices for retry.
Rollback reissues the prior configured preset and `Application::fullscreen()`
state. On Win32 that value is the last requested application state and fullscreen
uses borderless window styling, without an independent OS success/readback
guarantee. Rollback does not promise an arbitrary dragged window's exact size,
position or maximized state.

Keep uses the existing display/palette Save path, preserving current play
preferences and pad profiles. If writing fails, the accepted new preferences
remain active for the run and the old file remains intact. The existing save
failure status reports that result. Separate authored statuses report revert,
display-application failure and rollback failure; these failures are not
reported as successful persistence or guaranteed rollback.

The dialog uses captured Pause paper/fonts and the existing Options heading.
Only this display dialog overrides focus padding and shadow with the existing
`main_menu.setup_focus_padding` (`28, 7`) and `setup_shadow_offset` (`2`), keeping
the Pause palette and keyline (`4`). This fits Keep's descenders and pressed-face
clearance within the authored row spacing. This display-only padding/shadow
recipe leaves Pause art unchanged and adds no numeric layout field.
Startup rejects incompatible combinations unless each
`main_menu.setup_focus_padding` component is strictly greater than
`main_menu.setup_shadow_offset + pause_menu.art.keyline_width`.
Keep is row 0, Revert row 1, with the centered countdown at the layout origin
plus two row spacings. The appended Keep/Revert items bring the menu item count
to 39. Required text is `menu_text.confirmations.display` (`Keep these display
settings?`) and `menu_text.display_trial_countdown` (`Reverting in {seconds}s`),
with exactly one validated `{seconds}` token. The finite
`main_menu.display_trial_seconds` ships as `15`, allowed from `1` to `60`.
The timeout accounts for elapsed updates and steady-clock time on the next
update. A stopped process cannot roll back until it runs again, so this is not
an unconditional wall-clock guarantee. Reduce effects retains the countdown.

The hidden native trial matrix passes both presets: 35 independent complete
frames per preset (31 keyboard and four scripted pad-slot-3 references) across
six native sizes/aspects, with 42 byte-identical capture pairs and CSV evidence.
A configured 60-second fixture provides stable countdown references at 50, 27
and 1; a separate shipped 15-second fixture displays 12 after 3.25 simulated
seconds. A one-second real deadline also expires on a zero-dt update after a
1.1-second wait.

Checks cover one controlled failed Keep write and recovery per preset, unchanged
file bytes before Keep, preserved play preferences and all four pad profiles,
focus-loss/coverage/developer/teardown cancellation without orphan pops, and
frozen redraw/RNG. The private fixture applies windowed Off only; it does not
qualify actual fullscreen or system-wide display changes, physical controllers,
human readability, listening or performance.

Actual fullscreen/monitor, physical-controller and other human/hardware acceptance retain their existing
gates; hidden checks do not qualify a system-wide display configuration change.

### Options from Pause

Implemented on 2026-10-16, the paused-access part of `GAP#3.2`. Pause now offers
Resume, Restart, Options and Quit. Options is pushed above the same retained
Pause page; both its Back row and B/Escape pop only Options, leaving Pause
focused on Options. Closing settings does not resume the match or begin the
resume count-in. Home's Options entry still returns to Home with Options focused.

The existing `pause_menu.layout.spacing.y` changes from `115` to `120`:
Options' actual item-font height (`82`) plus both 18-unit focus pads occupies
`118`, leaving two design units between row envelopes. Quit moves 15 design
units lower; shared Pause-derived confirmations follow the same authored
spacing. No field, font or art asset is added.

Paused Options inherits the opening device and disconnected-device fallback
policy. Its display trial, Controls/audio and Pad Sticks children carry that
ownership through their returns. Controls/audio is nested inside Options for
both Home and Pause entry. Restore defaults, Apply, failed persistence and
Keep/Revert retain their existing transaction boundaries. Paused Options rejects
the developer match-start route, so that settings child cannot start a new match.

Applying a palette preference affects the next round or restart; it does not
recolour the running match's players, paint or HUD. Paused Options displays the
required captured `menu_text.options_palette_timing` throughout its own view:
`Palette applies next round or restart.` It uses the existing detail font at
`(main_menu.options_value_x, main_menu.options_heading_position.y)`, unscaled,
without a new numeric geometry field. The shipped detail-36 text measures
834 by 62 design units.

The hidden `-PausedOptionsOnly` matrix passes both presets: 81 independent
complete-frame comparisons and sixteen retained-world checkpoints per preset,
with 32 matching PPM pairs and both CSV outputs byte-identical across Debug
and Release. Actual one-, two- and four-player sparse-roster matches start with
positive paint. Checks retain the Level/HUD pixels, observable player physics,
roster, layout, GameLevel/Level identity and zero resume count-in; Debug adds a
deterministic fingerprint check. The fixture also covers nested ownership and
fallback, preference merge, saving a palette without changing the current world
then applying it on Restart, private windowed trial rollback, one controlled
Keep-write failure per preset, recovery and developer behavior.
Evidence is in `out/validation/2026-10-16-paused-options/paused-options-parity.json`
and `visual-qa.json` under that batch directory.

Final Debug and Release builds and all 32 consumer cases per preset pass. The
batch passes twenty-four native runs across twelve scopes in each preset,
including the 81-reference paused Options pair and the existing 35-reference,
42-capture-pair display-trial checks. Four isolated startup checks pass,
including extracted Release with the published bank and optional-bank silent
fallback under a system-only PATH. The 53-file Release ZIP contains all 26
reviewed runtime assets byte for byte and excludes source, debug and private
audio. This completes the implementation recorded as `GAP#3.2`.

Display rollback still reissues the configured preset and prior
`Application::fullscreen()` state. Win32's borderless styling provides no
independent OS success/readback guarantee, and arbitrary dragged window size or
placement is outside that contract. Human readability, physical-pad, actual
fullscreen/monitor, listening and performance acceptance remain open.

## Controls/audio

Implemented on 2026-10-15, part of `POL#16`. The page shared by Home's Options
and paused Options uses the same pale stepped paper, dark ink and solid neutral surround.
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
pending display choices and focus; returning to paused Options leaves the match paused.
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

Use `x64-release` for Release.
Physical-pad, movement-feel, human readability
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

Human menu/readability review remains open. Accepted arenas
remain `POL#16`; movement-feel (`POL#3`),
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

Human menu/readability review remains open. Accepted arenas
remain `POL#16`; movement-feel (`POL#3`), physical-pad
and listening acceptance retain their existing gates.

## Team Select

Implemented on 2026-10-16, part of `POL#16`. Team Select uses the shared stepped
paper, dark ink, quiet shade and neutral Home surround. The required
`main_menu.team_box_size` ships as `(1840, 920)`, centered above the controls
footer. Its wider panel preserves the existing player-badge position and row
padding. A captured `main_menu.art` copy uses the existing setup focus-padding
and shadow-offset overrides. Each player row has a static paper card filled
with `focus_fill`; the outer panel has ordinary paper fill. The page has no
shared focus cursor, focused-row card or detached arrow. It suppresses the
legacy brand and retains the controls footer.

Each row card encloses the union of the actual unscaled player label, retained
transparent-shadow extents, player badge and three choice-icon rectangles,
inflated by the setup focus padding. Player labels, badges, heading and icons
retain their fonts, anchors and logical bounds. The required
`main_menu.team_select.warning_position` places the existing both-teams warning
at `(150, 860)`, below all four cards and above the footer. Heading, player
labels and warning use captured dark ink with zero-alpha text shadows; their
shadow offsets and logical extents remain intact. Generated labels and the
warning remain C++ copy under `GAP#8.4`.

The existing atlas and frame pairs still identify Team A, unassigned and Team B.
Readiness colours remain live, and each row keeps its existing independent
choice/ready/unready/shuffle motion and Reduce effects policy. Only the chosen
icon contracts; its card and identity stay fixed. Provisional team choices,
both-teams validation, simultaneous actions, Back, missing-device recovery,
shuffle, final commit, transitions, RNG and audio keep their existing behavior.

Startup requires positive, finite panel dimensions, Team spacing and icon size.
The panel is at most 1840 wide and leaves 20 design units above and below within
the footer area. The warning position must be finite, nonnegative and above the
footer. Resource checks validate the actual heading/warning fonts and retained
shadow bounds. Page checks require the actual row cards to fit the usable paper
interior in order after the heading, player labels to end before the icons, and
the complete warning to follow the cards. No text fitting or scaling is added.
Point sampling reproduces the paper and existing icons; lettering remains
linear-filtered. The existing solid texel supplies all new artwork, with no new
runtime asset or atlas recipe.

Batch evidence is under
`out/validation/2026-10-16-team-paper/verification.json`.

Debug and Release build under `/W4 /WX` and each pass all 31 consumer cases.
The `TeamPaperOnly` pair uses the
scripted reader for one to four players at 1280×720, 1920×1080, 2560×1440,
1600×1200, 1920×1200 and 2560×1080. Each preset passes 273 independent full-frame
comparisons. Motion coverage includes fifteen moving cases and thirty moving
row instances; all 132 retained PPM captures and the CSV evidence match byte
for byte between Debug and Release.

The related `JoinPaperOnly`, `JoinStageMotionOnly`, `SetupPaperOnly`,
`MenuTextOnly`, `SetupMotionOnly`, `TeamSetupOnly`, `WeaponCatalogOnly` and
`ResumeOnly` scopes also pass in both presets, bringing the batch to eighteen
passing native runs.

Four startup checks pass, including extracted Release with its published bank
and optional-bank silent fallback under a system-only PATH. The 53-file Release
ZIP contains all 26 reviewed runtime assets byte for byte and excludes source,
debug and private audio.

The retained native 720p views show [team choices](images/polish-main-menu/team-choices-720p.png),
[a ready player](images/polish-main-menu/team-ready-720p.png) and the
[both-teams warning below the cards](images/polish-main-menu/team-warning-720p.png).

Human menu/readability review remains open. Accepted arenas
remain `POL#16`; movement-feel (`POL#3`), physical-pad
and listening acceptance retain their existing gates.

## Weapon Select

Implemented on 2026-10-16, part of `POL#16`. Weapon Select uses the shared
stepped paper, dark ink and neutral Home surround. The required
`main_menu.weapon_box_size` ships as `(1840, 920)`, centered above the controls
footer, preserving the existing player-badge position. The page captures
`main_menu.art` with the setup focus-padding and shadow-offset overrides. It
has no shared focus cursor or detached arrow and suppresses the legacy brand.
The controls footer remains visible.

The existing icon border envelope, shipped as 136 by 80 design units, becomes
a stepped paper tile. This keeps the existing 125-unit row stride without
adding an enclosing row card. The tile uses the captured weapon catalog's
sheet and fill frame, including authored replacements; the shared paper
primitive preserves its layer order and premultiplied colours. Its pen uses
the existing `menu_style.chrome.rule_height` (`2`), and its shadow uses the
captured setup shadow offset (`2`). The face retains the existing live white
unready and mint ready tints.

The readiness surround stays outside a fixed cream canvas using the existing
menu heading colour. This canvas covers the original icon rectangle with a
bounded keyline bleed inside its existing padding: the smaller of the rule
width and minimum padding, shipped as `2`. It keeps the protected preview
pixels unchanged through motion without moving the tool, paint or supply art.

Only the tile/border contracts during the existing independent choice and
ready/unready response. Base tool art, paint masks and final supply-strip frame
retain their source fitting and positions. Player labels, badges, heading,
weapon names and descriptions keep every font, anchor, scale and logical
bound. Heading, player and name text use captured dark ink; confirmed names
use captured focus ink. Description text retains its authored
`main_menu.weapon_description.colour`, now shipped as `1B1C20`. All text
shadows have zero alpha while retaining their previous offsets, including the
player label's default `(2, 2)` shadow.

Catalog names, descriptions, fonts and artwork roles remain paired through
presentation changes; reopening adopts new content. Random palette preview
stays neutral and final Random weapon resolution retains its existing RNG
draws. Choice order, readiness, simultaneous actions, Back, missing-device
recovery, final commit, transitions, audio and Reduce effects keep their
existing behavior.

Startup requires positive, finite panel dimensions, Weapon spacing and icon
size. The panel is at most 1840 wide and leaves 20 design units clear above and
below within the footer area. Heading containment is checked after existing
resource validations, preserving diagnostic priority. Page checks use actual
text/shadow, badge, maximum tile-envelope, name and description bounds to
require paper containment, ordered rows, separate columns and positive paper
faces throughout the existing motion envelope. No fitting or scaling path is
added. The existing fill assets reproduce the drawing with no new runtime
asset or atlas recipe; textures remain point-filtered and lettering linear.

The focused native scope is `WeaponPaperOnly`, using the scripted reader.
Batch evidence is under
`out/validation/2026-10-16-weapon-paper/verification.json`.

Debug and Release build under `/W4 /WX` and each pass all 31 consumer cases.
The focused `WeaponPaperOnly` pair passes 482 independent full-frame references
per preset. Motion checks cover fifteen moving cases and twenty-one moving row
instances.
All 148 retained PPM captures and CSV evidence match byte for byte between
Debug and Release; the existing outer profile is preserved.

The related `TeamPaperOnly`, `JoinPaperOnly`, `JoinStageMotionOnly`,
`SetupPaperOnly`, `MenuTextOnly`, `SetupMotionOnly`, `TeamSetupOnly`,
`WeaponCatalogOnly` and `ResumeOnly` scopes also pass in both presets, bringing
the batch to twenty passing native runs. Four isolated startup checks pass,
including extracted Release with the published bank and optional-bank silent
fallback under a system-only PATH. The 53-file Release ZIP contains all 26
reviewed runtime assets byte for byte and excludes source, debug and private
audio.

The retained native 720p views show [weapon choices](images/polish-main-menu/weapon-choices-720p.png),
the [partly ready roster](images/polish-main-menu/weapon-ready-720p.png) and
[all Random choices](images/polish-main-menu/weapon-random-720p.png).

Accepted arenas and human readability review remain `POL#16`;
movement-feel (`POL#3`), physical-pad and listening acceptance retain their
existing gates.

## Stage Select

Implemented on 2026-10-16, part of `POL#16`. Stage Select uses the shared stepped
paper, dark ink and neutral Home surround. The required
`main_menu.stage_box_size` ships as `(1840, 920)`, centered above the controls
footer, and is limited to 1840 wide and `footer_y - 40` high. The page captures
`main_menu.art` with the existing setup padding and shadow overrides. The
legacy brand and shared focus backing are suppressed; the controls footer
remains visible.

The briefing has a `focus_fill` paper card enclosing its authored rectangle
and the actual five briefing rows, status and hint. Text bounds are expanded
by three keyline widths plus the shadow offset before being included in that
union. The card is recomputed after actual selection refreshes, covering the
existing live hint fit. The authored 540-by-665 layout and fitting budget,
anchors, fonts and logical widget bounds remain unchanged. This includes the
existing solo-objective text that extends beyond the logical briefing edge.

Stage name and selection status retain live state colours:
`main_menu.stage_select.selected_ink` (required, shipped as `111216`) when
confirmed and the existing `unselected` (now `1B1C20`) otherwise. The preview
edge keeps the live mint `selected` colour. The briefing label and goal use
captured focus ink; the remaining static paper labels use captured art ink.
Paper text shadows have zero alpha while retaining authored offsets
and measured extents. The Random question mark keeps its existing light ink
and visible shadow on the unchanged dark preview.

`StagePreview` rendering is unchanged. Name and hint width fitting, captured
text/font/artwork roles, stage eligibility, browsing, lock/unlock, missing-device
recovery, final Random selection draw and latched outgoing view, music fade,
input and transition timing retain their existing behavior. The briefing stripe
keeps its existing bounded motion and Reduce effects policy. No arena artwork,
stage definition, runtime asset or atlas recipe is added.

Page checks require actual outer-paper and briefing-card containment and
separation from the preview. The heading resource guard is appended after the
existing text/resource checks, preserving diagnostic priority. Paper textures
remain point-filtered and lettering linear-filtered.

Batch evidence is under
`out/validation/2026-10-16-stage-paper/verification.json`.

Debug and Release build under `/W4 /WX` and each pass all 31 consumer cases.
The focused `StagePaperOnly` scripted-reader pair passes 711 independent
full-frame references and 711 exact StagePreview references per preset. It
covers all shipped arenas and eligible Random with solo, two- and four-player
setups across six display sizes. Fifteen moving cases and 123 pending-fade rows
retain the original motion envelope, latched view and immutable outer profile.
All 246 retained PPM captures and CSV evidence match byte for byte between
Debug and Release.

The related `MenuTextOnly`, `JoinStageMotionOnly`, `WeaponPaperOnly`,
`TeamPaperOnly`, `JoinPaperOnly`, `SetupPaperOnly`, `SetupMotionOnly`,
`TeamSetupOnly`, `WeaponCatalogOnly` and `ResumeOnly` scopes also pass in both
presets, bringing the batch to twenty-two passing native runs.

Four isolated startup checks pass, including extracted Release with the
published bank and optional-bank silent fallback under a system-only PATH.
The 53-file Release ZIP contains all 26 reviewed runtime assets byte for byte
and excludes source, debug and private audio.

The retained native 720p views show [four-player best-of-five browsing](images/polish-main-menu/stage-browse-720p.png),
the [locked arena](images/polish-main-menu/stage-locked-720p.png) and
[solo Random with the complete objective](images/polish-main-menu/stage-solo-random-720p.png).

Accepted-arena artwork and human presentation/readability review remain
`POL#16`. The current arena material/prop kit is approved for this stage.
Improve the reference layout (`POL#17`) first, then tune movement (`POL#3`),
then iterate with small level and movement adjustments
([development order](polish-plan.md#development-order-2026-10-17)). Physical-pad and listening
acceptance remain open; menu-motion review (`POL#10`) was accepted 2026-10-17.
