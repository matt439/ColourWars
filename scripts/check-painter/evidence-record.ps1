# Dot-sourced into the scope of scripts/check-painter.ps1.
# The result.json evidence record: its base shape, every focused scope's
# overrides and the post-chain fixups. Runs before the try, so an override
# error still skips the result.json write.

$result = [ordered]@{
    outcome = 'failed'
    scope = if ($IdentityOnly) { 'player-identity-only' } elseif ($QuietOnly) { 'quiet-only' } elseif ($MonitorOnly) { 'monitor-only' } elseif ($CountdownOnly) { 'countdown-only' } elseif ($ClosingOnly) { 'closing-only' } elseif ($SupplyOnly) { 'supply-only' } elseif ($HudOnly) { 'gameplay-hud-only' } elseif ($ResumeOnly) { 'resume-only' } elseif ($EndMenuOnly) { 'end-menu-only' } elseif ($PauseMenuOnly) { 'pause-menu-only' } elseif ($ResultsMenuOnly) { 'results-menu-only' } elseif ($MainMenuOnly) { 'main-menu-only' } elseif ($WeaponCatalogOnly) { 'weapon-catalog-only' } elseif ($MenuTextOnly) { 'menu-text-only' } elseif ($MenuMotionOnly) { 'menu-motion-only' } elseif ($SettingsMenuOnly) { 'settings-menu-only' } elseif ($ControlsMenuOnly) { 'controls-menu-only' } elseif ($StickSettingsOnly) { 'stick-settings-only' } elseif ($TeamSetupOnly) { 'team-setup-only' } else { 'full-painter-matrix' }
    preset = $Preset
    game_revision = (& git rev-parse HEAD)
    engine_revision = (& git -C external/labrador rev-parse HEAD)
    worktree_status = @(& git status --short)
    stage = 'movement_course'
    player_counts = @(1, 4)
    palettes = @('blue_orange', 'pink_green', 'blue_yellow')
    weapons = @('sprayer', 'sniper', 'mister', 'bucket', 'roller')
    actions = @('takeoff', 'land', 'fire', 'hurt', 'elimination')
    input_slots = @(3, 1, 2, 0)
    inputs = 'Scripted pad-slot inputs at the real 60 Hz simulation step; no physical-device claim.'
    audio = 'Muted; this check establishes no listening acceptance.'
    hud_checks = 'Real InterfaceGameplay native full/half/empty health and paint, independently supplied quantities and negative/oversized clamps. Meter differences stay inside exact tracks; the half raster agrees with full/empty on either side of its midpoint and dark keylines remain outside the track. Dark text/pale paper raster contrast, sparse keyboard/pad-slot labels, 1P/2P/3P/4P720 and all three 1P/4P palettes; representative 4P1080/QHD, 4:3, portrait and ultrawide. Isolated owner-pane artwork remains clear of pane edges, including the unused three-player quadrant. Ordinary clock and static information match under Reduce effects, repeated drawing preserves pixels/draw RNG. Real 4P match frozen pixels, public state, Debug replay fingerprint and exact 720p/1080p resize round-trip. Existing final-ten clock boundary matrix rerun; reveal cards/story timing are outside the focused HUD run. Native empty/half/full detail PNGs and full-context PPMs retained. Raster checks establish no human readability, physical-device or listening acceptance.'
    contact_checks = 'Real resolved 4P dry steps/reversals; controlled step/skid initial/middle/expired rasters in all palettes at 720p/1080p/QHD. Dry/painted and left/right raster differences, enabled/disabled comparisons, frozen direction/tint/timers and culling bounds.'
    weapon_checks = 'Actual Weapon firing workflow with locally retained projectiles: 1P all five tools and 4P mixed tools in all three palettes, four aim quadrants, then live aim/flip changes while each paid-shot pulse remains active. Roller has no muzzle pulse. Controlled 4P muzzle/empty/recovered event triggers at initial/middle/expiry, enabled/disabled at the same simulation state, at 720p/1080p/QHD. Frozen redraw and observed feedback timers, current-nozzle attachment and raster culling bounds. Empty/recovery gameplay producers are covered by separate CPU acceptance checks. Mechanism raster comparisons suppress the new pulses.'
    quiet_checks = 'Actual builder/Scene 1P and 4P sprayers in all three palettes at 720p: settled breath crest, bead growth/stretch/release/fall, blink, movement interruption and left lean. Sniper residual beads and bucket rim runoff add growth/stretch/fall, same-bead continuous four-quadrant aim with authored lip/stripe anchors, both body facings, per-tool enabled suppression, and same-state reduced-effects hiding/restoration for attached and falling liquid in every 1P/4P720 palette. Representative attached-bead extents at 1080p/QHD. Mature bead raster touches the actual rendered tool silhouette within one pixel. Same-state enabled/disabled breathing/shift rasters and controlled blink-only comparison at playing size, distinct roster phases, frozen cosmetic timers/drops, separate Scene drip raster bounds and combined nozzle/rim-context crops. All runs use muted play settings; no listening acceptance.'
    aim_marker_checks = 'Isolated save loads aim_marker=false and remains byte-identical. Existing matrices disable markers. Actual four aimed weapons enter ready/recovering/empty through paid shots and strict cooldown, in all three palettes and four aim quadrants at 4P720; representative 4P1080/QHD. Marker fixture uses a silent bank while paying many shots without wall-clock audio updates. Same-state Level on/off raster changes lie only inside each owning pane marker bounds; all three shapes have distinct isolated native rasters. A mixed-angle/readiness roster checks independent owner panes. Repeated frozen draws, suspension, simulation state and replay fingerprint are checked. CPU acceptance separately verifies affordability boundaries, all lifecycle gates and gameplay equivalence.'
    blocked_shot_checks = 'Detached contacts in actual four-pane Level drawing; controlled opaque-cover/nozzle overlap and four outward normals at native 4P720 in every palette, plus representative 1080p/QHD. Initial/middle/expired, enabled/disabled pixels, projected cue and aggregate bounds, rebound/fade raster changes, repeated frozen redraws, cosmetic timers, simulation state and Debug replay fingerprint. CPU acceptance separately checks actual launch/flight contact classification and lifecycle.'
    player_identity_checks = 'Real same-team builder rosters with identical tools in all three 4P720 palettes; overlapping figures, actual scripted teammate crossing and sparse/reordered input slots. Quiet, requested emphasis and reduce-effects captures, representative 1080p/QHD extents, pause, dead/respawn and story suppression. Same-state zero-world-size comparison preserves pane HUD badges and constrains every changed pixel to the owning world badge bounds. Distinct shared P1-P4 native glyph rasters, native context and badge crops, frozen cosmetic state, repeated redraw and Debug replay fingerprint. CPU acceptance separately verifies lifecycle and gameplay equivalence; human reading acceptance remains separate.'
    monitor_checks = 'Bounded monitor matrix: six authored body shades, adult with mug/ring/crack/notes and child with stickers; manual picture-edge note overlap at 480p/768p/720p/1080p/QHD and one bounded glass push. All 26 Muted Day atlas swatches retain authored order, square cells and visible gutters, followed by one contained empty slot; saving preserves every existing cell. Both picker colours and the complete panel stay inside the program at narrow aspects. Foreground notes survive picture and preview, picture/palette interiors stay unchanged, saving fills the empty slot and closing replaces the picture with the desktop. Isolated adult/child/mug seated, sigh, stand and partial-leave foreground rasters at all three native sizes; full departure equals blank, rim/outline/shading differ from plain fill, offset-window translation is exact. Native cursor hotspot stays fixed, detached initial/mid-click changes stay local and leave every resting arrow pixel unchanged, negative/expired ages and reduced effects equal rest. Native foreground and pointer detail crops supplement actual sparse-slot 2P/4P shipped full-drift story frames. A second identical story uses a private stage copy with only decorative cloud drift removed; restoring those fields must reproduce the original JSON exactly. This isolates unchanged before/at/after winning-save and no-paint-close reduced-effects assertions from legitimate cloud-to-anchor movement, retaining response containment, frozen state/RNG and Debug fingerprints with compact cursor/card crops and CSV. Solo practice has no monitor. Frozen rasters, persona/pose/public simulation state, draw RNG and Debug replay fingerprint remain unchanged. Captures establish no listening or physical-device acceptance.'
    countdown_checks = 'Isolated 0-9/GO glyphs differ at native 720p, with representative 1080p/QHD. Animated and reduced/negative/expired ages stay inside the supplied number box; reduced effects retain the settled readable signal. Actual keyboard solo and mixed keyboard/sparse-pad 2P/4P full/quick/off openings cover native 720p/1080p/QHD, reminder-card bounds, frozen initial/beat/settled/pre-GO/GO/tail/expiry captures, paused and nonpositive-time draws, exact GO control-release/card-clear boundary and resize round-trip. All four authored stage names appear on fitted shared paper headers; a three-player roster and 4:3, portrait and ultrawide resize round-trips cover stage/title bounds and separation from countdown/device cards. Header information remains static with Reduce effects, hides during early story opening and at GO, and survives skips from both opening and monitor countdown. Same-state GO art on/off differences remain inside the former card, persist through its tail and disappear after one second. Frozen rasters, public simulation/cosmetic state, draw RNG and Debug replay fingerprint remain unchanged. Native stage-header, card and glyph crops accompany full-context PPMs. This is muted hardware rendering, not human readability acceptance.'
    closing_checks = 'Actual HUD last-ten-second clock at 1P/2P/4P720 and 4P1080/QHD: initial/early beat, settled, pre-closing and zero states, local normal/reduced-effects raster comparisons and frozen redraw. Same-state paper suppression checks the entire settled card gate in every pane; short ten/one-second clocks show neither card nor ticks until live play. Native settled/click A/B coverage cards in all three palettes at 720p, representative 1080p/QHD, negative/expired/reduced-effects equivalence, supplied bounds and draw RNG. Actual sparse-pad 2P A/B winners, 4P no-paint draw and solo finish, before/at/after click card gating, repeated frozen pixels, public state and Debug replay fingerprint. Native card/clock detail PNGs and full story contexts are preserved. Muted rendering establishes no listening, physical-device or human readability acceptance.'
    supply_checks = 'Sprayer and mister full/half/empty reservoirs at actual builder-owned camera scales and layouts: all three 1P/4P720 palettes, four aim quadrants, both facing inputs, all four poses, representative 4P1080/QHD and motion-disabled tools. Independent authored full/half-layer composition must match actual Weapon pixels exactly; half pixels stay within full supply and filling introduces no new silhouette pixels or changes to permanent hardware/nozzle paint. Negative/oversized ammo clamps, unchanged reduced-effects information, frozen pixels/weapon state/draw RNG, paid full-shot depletion, natural regeneration/reset and unchanged match observation/Debug fingerprint. Menu previews retain full supply and switching tools clears it. Native detail PNGs and full-context PPMs are preserved; human readability and physical-device acceptance remain separate.'
    checks = 'Real builder/Scene rosters, both movement facings and four aim quadrants at 720p; representative facings also at 1080p/QHD; three 4P palettes; four same-team figures in both facings. Five body action clips on actual events, both frames and distinct pinned body rasters. All five tools at 1P720 in both facings: paid fire/hold/release/recovery, hose/bucket travel and roller travel/lift. Three 4P palettes: right-facing mixed aimed tools and left-facing roller, plus 1080p/QHD extent captures. Sampled non-idle pinned tools differ from idle at gameplay zoom, including released roller travel/lift. Frozen draws byte-equal, public simulation/action/tool motion state and Debug replay fingerprint unchanged. Isolated player/tool raster output stays inside projected cull bounds. Death, outline, fill, respawn, tool-free squash and whole-dead story poses; early elimination whole-dead raster equals later idle whole-dead raster. Combat on/off comparisons at identical simulation state: hurt and elimination across all three 4P palettes at 720p; controlled 4P hurt initial/recovery/expiry and elimination initial/middle/late/expiry at 720p/1080p/QHD. Combat timers/death snapshot stay unchanged during drawing; whole-dead story figures suppress the burst.'
    profile = $profile
    diagnostic = ''
}
if ($ResultPercentageOnly) {
    $result.scope = 'result-percentage-only'
    $result.Remove('stage')
    $result.stages = @('movement_course')
    $result.player_counts = @(1, 2)
    $result.palettes = @()
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('literal-result-percentage-text', 'actual-results-reveal-and-proceed', 'captured-templates-and-fonts', 'reopen-and-reversal', 'actual-level-monitor-inner-cards', 'frozen-session-level-setup-save-and-rng', 'resize')
    $result.input_slots = @(4, -1, 3, 1)
    $result.inputs = 'Actual ResultsMenuInitial with normal GamepadReader and public local keyboard events through StateContext; seeded actual LevelBuilder monitor fixtures use controlled local PlayerInputData. No physical-controller claim.'
    $result.audio = 'Focused Results and monitor inner-card fixture; this renderer probe establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated result percentage text leaf. Independent literal percentage labels compare actual Results pages and inner cards on three seeded public LevelBuilder monitor fixtures. Captured templates/fonts, retained pages, reopening/reversal and actual reveal/proceed transitions are checked. Drawing and resize preserve Session/Level state and RNG; private preferences and bytes remain unchanged. The three monitor fixtures intentionally advance eighty Level updates each. Exactly 60 StateContext updates and a separate 240 Level updates, 120 readbacks, 105 target CSV rows, eight Results openings and proceed pops, six font role controls, zero-update resize, eight compact crops (681072 pixels) and one successful private seed Save form the approved scope. Both presets run the same shared consumer scope without developer targets or commands. Monitor evidence covers the inner cards. No broad monitor-story or menu matrix, foreground/global input, physical-device, listening, human readability or performance acceptance.'
}
elseif ($ContributionTeamOnly) {
    $result.scope = 'contribution-team-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @(4)
    $result.palettes = @()
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('two-literal-contribution-team-separators', 'sparse-roster-owner-credit', 'actual-results-reveal-and-proceed', 'captured-copy-and-font', 'reopen-and-reversal', 'frozen-session-setup-save-and-rng', 'resize')
    $result.input_slots = @(4, 2, -1, 0)
    $result.inputs = 'Actual ResultsMenuInitial with the normal GamepadReader and public local keyboard events through StateContext; local proceed callbacks do not launch a match.'
    $result.audio = 'Focused Results page fixture; this renderer probe establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated contribution team text leaf. Independent complete literal rows compare both team separators on the actual sparse four-player Results roster and preserve owner-credit percentages. Captured copy/font assignment, retained pages, reopening/reversal, actual reveal/proceed transitions and frozen Session/setup/private Save/RNG retain the existing layout. Exactly 45 StateContext updates, 56 readbacks, 72 CSV rows across eighteen four-row observations, five page openings and proceed pops, four font row controls, four wrong-view-credit row controls, zero-update resize, six compact crops (599508 pixels) and one successful private seed Save are checked with exact schema, case, metric and RGB-payload gates. Both presets run the same shared consumer scope without developer targets or commands. No broad menu matrix, match launch, foreground/global input, physical-device, listening, human readability or performance acceptance.'
}
elseif ($CompetitiveWinnerOnly) {
    $result.scope = 'competitive-winner-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @(1, 2)
    $result.palettes = @()
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('three-literal-competitive-winner-labels', 'solo-copy-and-visibility-control', 'actual-results-reveal-and-proceed', 'captured-copy-and-font', 'reopen-and-reversal', 'frozen-session-setup-save-and-rng', 'resize')
    $result.input_slots = @(4)
    $result.inputs = 'Actual ResultsMenuInitial with the normal GamepadReader and public local keyboard events through StateContext; local proceed callbacks do not launch a match.'
    $result.audio = 'Focused Results page fixture; this renderer probe establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated competitive winner text leaf. Independent complete literal copy and raw-font references compare Team A, Team B and Draw winner labels, with unchanged solo copy and explicit hidden-winner controls. Captured copy/item-font assignment, retained pages, reopening/reversal, actual reveal/proceed transitions and frozen Session/setup/private Save/RNG retain the existing unscaled zero-offset layout. Exactly 102 StateContext updates, 99 readbacks, 32 CSV rows, fourteen page openings and proceed pops, three font negative controls, zero-update resize, eight compact crops (312480 pixels) and one successful private seed Save are checked with exact schema, case, metric and RGB-payload gates. Both presets run the same shared consumer scope without developer targets or commands. No broad menu matrix, match launch, foreground/global input, physical-device, listening, human readability or performance acceptance.'
}
elseif ($RoundHeadingOnly) {
    $result.scope = 'round-heading-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @(2)
    $result.palettes = @()
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('literal-numbered-round-headings', 'single-match-heading-control', 'actual-results-reveal-and-proceed', 'captured-template-and-font', 'reopen-and-reversal', 'frozen-session-setup-save-and-rng', 'resize')
    $result.input_slots = @(4)
    $result.inputs = 'Actual ResultsMenuInitial with the normal GamepadReader and public local keyboard events through StateContext; local proceed callbacks do not launch a match.'
    $result.audio = 'Focused Results page fixture; this renderer probe establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated numbered Results heading leaf. Independent literal headings and raw-font references compare actual all-draw Session rounds 1, 10 and 100, with the unchanged single-match heading under the replacement template/font as a separate control. Captured prefix/suffix template and heading-font assignment, retained pages, reopening/reversal, actual reveal/proceed transitions, unchanged literal score/outcome rows and frozen Session/setup/private Save/RNG retain the existing unscaled zero-offset layout. Exactly 66 StateContext updates, 81 readbacks, 26 CSV rows, eleven page openings and proceed pops, three font negative controls, zero-update resize, eight compact crops (722400 pixels) and one successful private seed Save are checked with exact schema, case, metric and RGB-payload gates. Both presets run the same shared consumer scope without developer targets or commands. No broad menu matrix, match launch, foreground/global input, physical-device, listening, human readability or performance acceptance.'
}
elseif ($SeriesScoreOnly) {
    $result.scope = 'series-score-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @(2)
    $result.palettes = @()
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('literal-complete-series-scores', 'literal-outcome-and-round-heading-controls', 'single-match-score-absence', 'actual-results-reveal-and-proceed', 'captured-template-and-font', 'reopen-and-reversal', 'frozen-session-setup-save-and-rng', 'resize')
    $result.input_slots = @(4)
    $result.inputs = 'Actual ResultsMenuInitial with the normal GamepadReader and public local keyboard events through StateContext; local proceed callbacks do not launch a match.'
    $result.audio = 'Focused Results page fixture; this renderer probe establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated generated series-score leaf. Independent complete literal scores and raw-font references compare scoreless draw, ongoing series and both maximum-round winners, with literal outcome-neighbor and generated heading controls; the score remains absent for a single match. Captured template/font assignment, reordered tokens, retained pages, reopening/reversal, actual reveal/proceed transitions, frozen Session/setup/private Save/RNG and zero-update resize checks retain the existing unscaled zero-offset layout. Exactly 81 StateContext updates, 97 readbacks, 31 CSV rows, fourteen page openings and proceed pops, four font negative controls, eight compact crops (710640 pixels) and one successful private seed Save are checked with exact schema, case, metric and RGB-payload gates. Both presets run the same shared consumer scope without developer targets or commands. No broad menu matrix, match launch, foreground/global input, physical-device, listening, human readability or performance acceptance.'
}
elseif ($SessionResultOnly) {
    $result.scope = 'session-result-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @(2)
    $result.palettes = @()
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('four-literal-session-outcomes', 'literal-score-and-round-heading-controls', 'actual-results-reveal-and-proceed', 'captured-copy-and-font', 'reopen-and-reversal', 'frozen-session-setup-save-and-rng', 'resize')
    $result.input_slots = @(4)
    $result.inputs = 'Actual ResultsMenuInitial with the normal GamepadReader and public local keyboard events through StateContext; local proceed callbacks do not launch a match.'
    $result.audio = 'Focused Results page fixture; this renderer probe establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated four-outcome Session result leaf. Independent literal copy and raw-font references compare the actual series result line across Team A/Team B completion, drawn round and ongoing series, with literal score-neighbor and generated Round heading controls. Captured copy/font assignment, retained pages, reopening/reversal, actual reveal/proceed transitions, frozen Session/setup/private Save/RNG and zero-update resize checks retain the existing unscaled zero-offset layout. Exactly 76 StateContext updates, 94 readbacks, 30 CSV rows, thirteen page openings and proceed pops, four font negative controls, eight compact crops (710640 pixels) and one successful private seed Save are checked with exact schema, case, metric and RGB-payload gates. Both presets run the same shared consumer scope without developer targets or commands. No broad menu matrix, match launch, foreground/global input, physical-device, listening, human readability or performance acceptance.'
}
elseif ($ResultsTextOnly) {
    $result.scope = 'results-text-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @(2)
    $result.palettes = @()
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('three-independent-plain-results-labels', 'actual-results-reveal-and-proceed', 'captured-copy-and-fonts', 'reopen-and-reversal', 'series-heading-control', 'frozen-session-setup-save-and-rng', 'resize')
    $result.input_slots = @(4)
    $result.inputs = 'Actual ResultsMenuInitial with the normal GamepadReader and public local keyboard events through StateContext; local proceed callbacks do not launch a match.'
    $result.audio = 'Focused Results page fixture; this renderer probe establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated three-label Results leaf. Independent literal copy and raw-font references compare the actual single-match heading, proceed line and contribution heading with their surrounding paper; generated series headings retain their existing behavior. Captured copy/fonts, reopening and reversal, actual reveal/proceed transitions, frozen Session/setup/private Save/RNG and zero-update resize checks are bounded separately. Exactly 43 StateContext updates, 52 readbacks, 51 CSV rows (seventeen observations across three roles), six page openings, six proceed pops, three font negative controls, eight compact crops (1428000 pixels) and one successful private seed Save are checked with exact schema, case, metric and RGB-payload gates. Both presets run the same shared consumer scope without developer targets or commands. No broad menu matrix, match launch, foreground/global input, physical-device, listening, human readability or performance acceptance.'
}
elseif ($EndContinuationOnly) {
    $result.scope = 'end-continuation-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @(1, 2)
    $result.palettes = @()
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('four-literal-continuation-labels', 'actual-end-menu-keyboard-focus', 'captured-copy-and-fonts', 'reopen-and-reversal', 'typed-callbacks-and-deferred-music', 'frozen-session-setup-save-and-rng', 'resize')
    $result.input_slots = @(4)
    $result.inputs = 'Actual EndMenu with the normal GamepadReader and public local keyboard events through StateContext; typed callbacks terminate at a local dummy root without launching a match or leaving the fixture.'
    $result.audio = 'The fixture checks deferred music actions; this renderer probe establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated four-label continuation leaf. Independent literal copy and raw-font references compare the actual EndMenu primary label and complete primary ornament across single/series rematch, completed series, drawn round and next round. Captured copy/font assignment, reopening and reversal retain the existing geometry, persistent primary card and focus-arrow policy. Typed local callbacks prove restart/exit selection and deferred music actions without launching gameplay; frozen Session/setup/private Save/RNG and zero-update resize checks remain separate. Exactly 68 StateContext updates, 94 readbacks, 31 CSV rows, fourteen page openings, one font negative control, six 540x96 crops (311040 pixels), fourteen typed actions (thirteen restart and one exit), four deferred actions, four music starts and one successful private Play-settings seed Save are checked. Both presets run the same shared consumer scope without developer targets or commands. No broad menu matrix, match launch, foreground/global input, physical-device, listening, human readability or performance acceptance.'
}
elseif ($ResolutionTextOnly) {
    $result.scope = 'resolution-text-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @()
    $result.palettes = @()
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080', '1600x900', '1001x777')
    $result.actions = @('literal-resolution-rows', 'actual-options-keyboard-cycles', 'captured-copy-and-fonts', 'reopen-and-reversal', 'frozen-state-and-draw-rng', 'resize')
    $result.input_slots = @(4)
    $result.inputs = 'Actual Options page with the normal GamepadReader and public local keyboard events through StateContext; no scripted pad seam, foreground/global input or match launch.'
    $result.audio = 'Focused menu fixture; this renderer probe establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated resolution-template leaf. Independent literal copy and raw-font references compare the actual Options resolution row through normal keyboard changes, captured template/font assignment, reopening and reversal. Saved windowed presets and saved-fullscreen live client values retain their existing enablement and pending-toggle behavior; the application remains windowed, with no Apply or display trial. Exactly 43 StateContext updates, 76 readbacks, 25 CSV rows, seven page openings, one font negative control, six compact crops (1050840 pixels) and three successful private metadata seed Saves are checked with exact summary, case, metric and RGB-payload gates. Both presets run the same shared consumer scope without developer targets or commands. No broad menu matrix, display Apply, gameplay, foreground/global input, physical-device, listening, human readability or performance acceptance.'
}
elseif ($DurationTextOnly) {
    $result.scope = 'duration-text-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @()
    $result.palettes = @()
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('literal-duration-rows', 'compound-ornament', 'actual-mode-keyboard-cycles', 'captured-text-and-fonts', 'reopen-and-reversal', 'frozen-draw-state-and-rng')
    $result.input_slots = @(4)
    $result.inputs = 'Actual Mode page with the normal GamepadReader and public local keyboard events through StateContext; no scripted pad seam, foreground/global input or match launch.'
    $result.audio = 'Muted page fixture; this renderer probe establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated two-template duration leaf. Independent literal duration rows and the complete compound ornament are compared with actual Mode page drawing through keyboard cycling, explicit/default choices, fractional and scientific defaults, captured text/font replacement, reopening and reversal. Frozen redraw and RNG, one font negative control, byte-identical private preferences and zero-update 720/1080/720 resize checks are retained. Exactly 86 StateContext updates, 76 readbacks, 25 CSV rows, eight page openings, five 1168x80 crops (467200 pixels) and one successful private Play-settings seed Save per preset. No broad menu matrix, gameplay, foreground/global input, physical-device, listening, human readability or performance acceptance.'
    if ($Preset -eq 'x64-debug') {
        $result.checks += ' Debug separately exercises the real TuningTarget and PresentationTarget resource-only validation: two legacy stand-in acceptances, paired-content refusals, primed candidate refusals, inert failed applies, valid retries and consumed candidates. Six target validations and six private candidate writes add zero state updates, readbacks, Save calls or commands; actual content bytes and draw RNG remain unchanged.'
        $result['target_validation_scope'] = 'debug-resource-targets-only'
    }
    else {
        $result['target_validation_scope'] = 'not-run-release'
    }
}
elseif ($ExposedColourAlphaOnly) {
    $result.scope = 'exposed-colour-alpha-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @()
    $result.palettes = @('opaque-default', 'literal-rgba128', 'literal-alpha0-nonzero-rgb')
    $result.weapons = @()
    $result.resolutions = @('1280x720')
    $result.actions = @('thirteen-home-background-menu-sites', 'captured-retained-reopened-and-restored-copy', 'movement-feedback-alpha', 'debug-label-alpha', 'independent-straight-channel-references', 'frozen-draw-state-and-rng')
    $result.input_slots = @()
    $result.inputs = 'Normal GamepadReader backend with isolated native menu/drawable fixtures; no scripted pad seam, foreground/global input or active match route.'
    $result.audio = 'No gameplay audio events; this renderer probe establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated exposed-colour alpha leaf. Thirteen actual Home-background menu sites retain captured defaults/RGBA128/alpha0, adopt reopened values and restore defaults; literal channel references cover black/nonblack backgrounds. Home evidence is 235 readbacks, 52 page openings, 130 CSV rows, four 128x96 crops, eight controls and zero StateContext updates. Display confirmation uses the same 1280x720 windowed candidate with a private activation guard and rollback; no Keep action. Movement feedback and debug text compare independent raw-source/literal-channel references, opaque parity, straight-alpha/double-fade controls and retained captured paint; their evidence is 50 readbacks, 12 CSV rows, eight compact crops, ten negative controls and four opaque parity controls. Shared evidence totals 285 readbacks, 142 CSV rows, twelve crops, two successful private seed Saves and zero StateContext/gameplay updates. No broad painter matrix, gameplay, physical-device, listening, human readability or performance acceptance.'
    if ($Preset -eq 'x64-debug') {
        $result.actions += @('debug-console-alpha', 'validated-private-tune-reset')
        $result.checks += ' Debug additionally compares frozen/live console text, panel shadows and performance text against independent references through eight real private tune/reset commands: 104 readbacks, 48 CSV rows, four extra crops and zero Save write commands. Debug totals 389 readbacks, 190 CSV rows and sixteen crops.'
        $result['developer_command_scope'] = 'debug-validated-commands'
    }
    else {
        $result.checks += ' Release remains shared-consumer-only and emits no console command transcript, console summary, console CSV or console scratch tree.'
        $result['developer_command_scope'] = 'not-run-release-consumer-only'
    }
}
elseif ($LimbTuneColoursOnly) {
    $result.scope = 'limb-tune-colours-only'
    $result.Remove('stage'); $result.stages = @(); $result.player_counts = @(); $result.input_slots = @(); $result.weapons = @()
    $result.palettes = @('authored-arm000000-handFFFFFF','arm35BCE1-handE17C35','arm2060C080-handC0406080','armA532D600-handD65A3200'); $result.resolutions = @('1280x720')
    $result.inputs = 'Retained direct FigureAttachments with ten actual builder tools/facings, two active and two disabled blink panels, four separate captured rest/sleeve guards. Ten zero-delta preparation Weapon calls per entrypoint; no observation Weapon update, Player/Level/Scene/State/input/time update or input poll.'
    $result.audio = 'No audio/listening acceptance; isolated private preference seed Saves only.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.actions = @('two-live-figure-limb-colours','ten-tool-facing-panels','two-active-and-two-disabled-blink-panels','four-captured-art-guards','12-independent-procedural-ROIs','396-ordered-literal-rectangles','21142-raw-RGBA-pixels-per-case','six-role-specific-controls252witnesses','observed-control-counts-not-precision-oracle','four-native1280x480-fixture-crops','post-contact-fullRGB-PNG-parity')
    $result.checks = 'Shared22readbacks/4rows/4native1280x480 crops. Independent proof covers only12 procedural ROIs/21142RGBA per case using engine-only frozen rectangles; copied full-fixture equality proves adoption only. Six controls require 248 discriminating witness differences and 4 mandatory exact opaque-WHITE RGBA equalities at saturated eyelid sites (actual/correct/wrong), preserving exact full-frame exterior outside frozen supports. Diagnostic changed counts remain observed. Every read renders/checks32 WHITE/BLACK RGBA calibration pixels with no mask exemption or extra read. Record/resource/tool/input/RNG guards and all nine zero observation/update/poll fields remain exact, separately from10 zero-dt preparation Weapon calls per entrypoint. Four lossless PNGs complete after contact creation with strict IHDR/native dimensions/fullRGB-to-PPM/opaquealpha and flat Depth5-safe metadata.'
    if ($Preset -eq 'x64-debug') { $result.actions += @('9commands9states','one-two-token-private-save-rebase','named-RED-token','saved-arm-reset-retains-hand-pending','two-private-preference-seed-Saves'); $result.checks += ' Debug58reads/13rows/4crops,20 combined preparatory Weapon calls,1856 calibration comparisons; exact two-token Save and sibling-preserving saved resets are native-owned.' }
    else { $result.actions += @('22reads4rows4crops','no-Debug-or-private-Save-artifacts') }
    $result.checks += ' No independent full character/body/weapon/sleeve art, actual Player/Level integration, natural blink scheduling, human art/readability/contrast, hardware/listening, balance, GPU precision or performance acceptance; aggregate save refusal is CPU-owned.'
}
elseif ($CrackTuneColourOnly) {
    $result.scope = 'crack-tune-colour-only'
    $result.Remove('stage'); $result.stages = @(); $result.player_counts = @(); $result.input_slots = @(); $result.weapons = @()
    $result.palettes = @('authored-opaque-3A3A3A','literal-35BCE1','literal-2060C080','literal-A532D600'); $result.resolutions = @('1280x720')
    $result.inputs = 'Retained direct Monitor::draw_front_details only: independently derived left/right five-point personas, no roll or update; Canvas32/64/96 then front details then fixed WHITE/BLACK calibration overlays. No Monitor body, Level/world, physical input or input polling.'
    $result.audio = 'No audio consumer/listening acceptance; private preferences unchanged.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.actions = @('one-live-opening-crack-field','two-retained-left-right-paths','segment-ink-then-thin-ordered-77-and-165-writes','eight-native-contiguous115x34-crops','ten-role-specific-controls','1274-changed37826-unchanged-control-pixels','31280-independent-retained-RGB-pixels','rendered-WHITE-BLACK32pixel-calibration-per-read')
    $result.checks = 'One live presentation.opening.crack field reaches retained actual Monitor front-details consumers. Shared42 reads/8 rows/8 native115x34 crops31280pixels93952P6bytes. Independent proof is limited to left [2,472,117,506) and right [1162,472,1277,506) ROIs: Canvas plus left-only EEEEEE prefix, then each segment broad BLACK ink followed by selected thin rectangles, all77 ink+165 selected writes per path with repeated dabs, opaque erasure and per-write target storage. Complete3910 actual/raw RGBA pixels and alpha255 are checked first per case; retained crops independently check all31280 RGB pixels with one coherent complete source model and compatible left/right membership, all matching names/equivalence retained. Effective support179left/178right bounds full-frame palette deltas; ever-selected support182 each includes erased-zero sites3left/4right. Ten wrong references separately test straight RGB, squared base, collapsed multiplicity, all-ink-first order and alpha0 leakage:1274changed/37826unchanged ROI pixels, exact complements and full-frame exterior guards. Alpha0 retains the BLACK crack skeleton and unselected highlight/Canvas. Actual/repeat and copied complete front-details parity prove repeat/adoption only. Every read includes32 exact rendered WHITE/BLACK calibration RGBA pixels without extra reads or mask exemptions. Borrowed records, complete unselected style/resources/captured personas/caller inputs, screen/camera/filter and RNG remain fixed; zero all eight updates/input-poll fields and no event fade.'
    if ($Preset -eq 'x64-debug') { $result.actions += @('9-real-commands9states-left-only','36-additional-reads9rows','totals78reads17rows8crops','one-crack-token-private-save-rebase-reset','two-private-PlaySettings-and-JumpJ-seed-Saves'); $result.checks += ' Debug left only; original/saved whole-front resets, exact one-crack-token private Save/rebase and two successful private PlaySettings+JumpJ seed Saves preserve default display, real content/private preferences and queued work.' }
    else { $result.actions += @('literal-consumer-only','no-developer-transcript-scratch-or-Save') }
    $result.checks += ' After contact export, eight same-stem PNGs retain native115x34 dimensions, bit_depth8 and colour_type2or6; strict IHDR and complete decoded RGB parity to PPM are checked with flat per-crop encoded metadata and no extra GPU reads. The two native contiguous crops are not mirrored integer approximations or a full Monitor reconstruction. Source models are bounded representatives without per-pixel mixing, tolerance, capture fitting or universal GPU precision claims. Rendered calibration does not establish full art correctness. Complete-candidate conflict/force refusal remains CPU-owned. No actual Level/story/roster/phase, arbitrary crack path/transform, physical input/hardware, human contrast/readability/comfort, listening, balance or performance acceptance.'
}
elseif ($HudTrackTuneColourOnly) {
    $result.scope = 'hud-track-tune-colour-only'
    $result.Remove('stage'); $result.stages = @(); $result.player_counts = @(); $result.input_slots = @(); $result.weapons = @()
    $result.palettes = @('authored-opaque-B8B8B8','literal-35BCE1','literal-0161C180','literal-A532D600'); $result.resolutions = @('1280x720')
    $result.inputs = 'Retained direct InterfaceGameplay only: paired supplied health/ammo0/0,.5/.5,1/1, TeamA blue_orange, keyboard-and-mouse slot, timer123.25, ordinary effects and running clock; three paired cases, not all nine Cartesian combinations. No Player/Level/Scene, polling or local/Player/Level/State/input/time updates.'
    $result.audio = 'No audio consumer/listening acceptance; private preferences unchanged.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.actions = @('one-live-hud-track-field','three-paired-health-ammo-contexts','single-opaque-ink-underlay','two-disjoint-ROIs-lossless192x32-stack','ink-track-unchanged-health-team-prefix-order','six-controls-twelve-track-discriminations','fully-filled-four-palette-invariance','unchanged-fill-prefixes','73728-independent-retained-RGB-pixels')
    $result.checks = 'One live presentation.hud.meter_track_colour field reaches both tracks of the retained actual HUD consumer. Shared54 reads/12 rows/12 lossless paired192x32 ROI stacks73728pixels221352P6bytes. Independent raw geometry/alpha is limited to two disjoint192x16 interiors: health [1036,64,1228,80) and PAINT [1036,122,1228,138), stacked without resampling and excluding the42-row source gap. Ordered opaque ink20 then selected track then unchanged opaque health/team prefixes, widths0/96/192. Six wrong references separately discriminate both tracks:12discriminations,27648changed control pixels and9216unchanged complement pixels. Retained composites independently check all73728 RGB pixels; native C++ owns RGBA and exact actual/raw comparison precedes whole-composite source precision checks. Fully filled pairs are byte-identical across all four palettes; alpha0 reveals ink20 in visible suffixes while filled prefixes remain fixed. Opaque ink erases earlier panel layers; one Canvas adds no selected-material backdrop. Whole copied-HUD frames prove adoption only. Borrowed HUD/identity records, complete unselected style/resources, caller inputs, viewport and RNG remain fixed; zero all updates/no event fade.'
    if ($Preset -eq 'x64-debug') { $result.actions += @('9-real-commands9states-half-half-only','36-additional-reads9rows','totals90reads21rows12crops','one-track-token-private-save-rebase-reset','two-private-PlaySettings-and-JumpJ-seed-Saves'); $result.checks += ' Debug half/half only; original/saved whole-HUD resets, exact one-track-token private Save/rebase and unchanged default display/real content/private preferences/queued work.' }
    else { $result.actions += @('literal-consumer-only','no-developer-transcript-scratch-or-Save') }
    $result.checks += ' Three paired amount contexts are not all nine Cartesian cases; the192x32 stack is not one contiguous native crop. Supplementary source models must agree across one whole composite, without per-pixel mixing, tolerance, capture-fitting or universal GPU precision claims. Supplied fractions do not prove gameplay health/ammunition behavior or an actual Level sequence. No independent full HUD/text/border/art reconstruction, extra layout/roster/aspect/clamp matrix, human/readability/listening/physical-input or performance acceptance.'
}
elseif ($HudHealthTuneColourOnly) {
    $result.scope = 'hud-health-tune-colour-only'
    $result.Remove('stage'); $result.stages = @(); $result.player_counts = @(); $result.input_slots = @(); $result.weapons = @()
    $result.palettes = @('authored-opaque-76E8B5','literal-35BCE1','literal-2060C080','literal-A532D600'); $result.resolutions = @('1280x720')
    $result.inputs = 'Retained direct InterfaceGameplay only: supplied remaining health0/.5/1, TeamA blue_orange, keyboard-and-mouse slot, ammo.5, timer123.25, ordinary effects and running clock; no Player/Level/Scene, polling or local/Player/Level/State/input/time updates.'
    $result.audio = 'No audio consumer/listening acceptance; private preferences unchanged.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.actions = @('one-live-hud-health-field','three-supplied-health-contexts','single-opaque-track-underlay','full192x16-health-ROI-only','zero-or-one-selected-prefix-write','six-exact-mask-controls','empty-health-four-palette-invariance','unchanged-separate-PAINT-bytes','36864-independent-retained-RGB-pixels')
    $result.checks = 'One live presentation.hud.health_colour field reaches the retained actual HUD consumer. Shared54 reads/12 rows/12 direct192x16 crops36864pixels110760P6bytes. Independent raw geometry/alpha is limited to health ROI [1036,64,1228,80): opaqueB8 track then zero or one selected prefix, widths0/96/192. Six wrong references discriminate13824selected control pixels and preserve4608complement pixels. Retained PPMs independently check all36864 RGB pixels; native C++ owns RGBA. Empty health is byte-identical across all four palettes; alpha0 retains track184. The opaque track erases earlier underlays, and one Canvas adds no selected-material backdrop. Whole copied-HUD frames prove adoption only. Separate PAINT bytes, borrowed HUD/identity records, complete unselected style, resources, caller inputs, viewport and RNG remain fixed; zero all updates/no event fade.'
    if ($Preset -eq 'x64-debug') { $result.actions += @('9-real-commands9states-full-health-only','36-additional-reads9rows','totals90reads21rows12crops','one-token-private-save-rebase-reset','two-private-PlaySettings-and-JumpJ-seed-Saves'); $result.checks += ' Debug full health only; original/saved whole-HUD resets, exact one-health-token private Save/rebase and unchanged default display/real content/private preferences/queued work.' }
    else { $result.actions += @('literal-consumer-only','no-developer-transcript-scratch-or-Save') }
    $result.checks += ' Supplied fractions do not prove gameplay damage/regeneration or an actual Level sequence. No independent full HUD/text/border/art reconstruction, extra layout/roster/aspect/clamp matrix, human/readability/listening/physical-input or performance acceptance.'
}
elseif ($ScribbleTuneColourOnly) {
    $result.scope = 'scribble-tune-colour-only'
    $result.Remove('stage'); $result.stages = @(); $result.player_counts = @(); $result.input_slots = @(); $result.weapons = @()
    $result.palettes = @('authored-opaque-3050C0','literal-35BCE1','literal-2060C080','literal-A532D600'); $result.resolutions = @('1280x720')
    $result.inputs = 'Retained direct Monitor.draw_front_details only, with supplied captured paired notes: A yellow-flat colour0/tilt0 and B blue-tilted colour2/tilt0.1 at(.015,.47); no persona roll, Level, polling or local/Player/Level/State/input/time updates.'
    $result.audio = 'No audio consumer/listening acceptance; private preferences unchanged.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.actions = @('one-live-scribble-field','two-paired-opaque-paper-contexts','full67x67-note-ROI-only','five-fixed-layers-74-ordered-dabs','eight-controls24-row-discriminations','retained-consumer-repeat','35912-independent-retained-RGB-pixels')
    $result.checks = 'One live presentation.opening.scribble field reaches the existing retained note consumer. Shared40 reads/8 rows/8 native67x67 crops35912pixels107840P6bytes. Independent raw geometry/alpha is limited to ROI [18,337,85,404): five ordered static rectangles then74 literal selected dabs with per-write quantization. Eight wrong references qualify24 row discriminations, exact selected/unselected masks and collapsed single/double membership. Retained PPMs independently check all35912 RGB pixels; native C++ owns RGBA. Alpha0 retains both authored papers and fixed shadow/ink/header/underline. Two paired contexts are not a full colour-by-tilt matrix, and the one Canvas adds no selected-material underlayer. Whole copied-front frames prove adoption only. Borrowed records/resources, captured/caller persona values, unselected style/arrays, public geometry and RNG remain fixed; zero all updates/no event fade.'
    if ($Preset -eq 'x64-debug') { $result.actions += @('9-real-commands9states-contextA-only','36-additional-reads9rows','totals76reads17rows8crops','one-token-private-save-rebase-reset','two-private-PlaySettings-and-JumpJ-seed-Saves'); $result.checks += ' Debug Context A only; original/saved whole-frame resets, exact one-Scribble-token private Save/rebase and unchanged default display/real content/private preferences/queued work.' }
    else { $result.actions += @('literal-consumer-only','no-developer-transcript-scratch-or-Save') }
    $result.checks += ' No independent full Monitor geometry, universal note/persona/matrix coverage, actual Level/story visibility, human/readability/listening/physical-input or performance acceptance.'
}
elseif ($SilhouetteBodyTuneColourOnly) {
    $result.scope = 'silhouette-body-tune-colour-only'
    $result.Remove('stage'); $result.stages = @(); $result.player_counts = @(); $result.input_slots = @(); $result.weapons = @()
    $result.palettes = @('authored-opaque-0E0E0E','literal-35BCE1','literal-2060C080','literal-A532D600'); $result.resolutions = @('1280x720')
    $result.inputs = 'Retained direct Silhouette only: adult-rest-mug, adult-half-stand and child-rest with fixed persona/poses/screens/camera; no Level, input polling or local/Player/Level/State/input/time updates.'
    $result.audio = 'No audio consumer/listening acceptance; private preferences unchanged.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.actions = @('one-live-body-field','nine-local-ordered-RGBA-patches','one-effective-opaqueBLACK-underlay','thirteen-per-reference-controls','retained-consumer-repeat','324-literal-retained-RGB-pixels')
    $result.checks = 'One live presentation.opening.silhouette field reaches the existing retained body consumer. Independent raw proof is limited to nine3x3 patches with ordered1/2/3 selected writes; complete copied-Silhouette frames prove adoption only. Shared61reads/12rows/12lossless native crops759808pixels and2279604P6bytes;13controls,34affected-patch rejections and8unchanged guards. Adult crops are320x224 two-panel native montages; child crops208x224. Alpha0 reveals opaqueBLACK ink at the selected patches and preserves unselected rim/shade. Borrowed records/resources, unselected style, captured persona/poses/screens/camera, exterior bounds/deltas and RNG remain fixed; zero all updates/no event fade.'
    if ($Preset -eq 'x64-debug') { $result.actions += @('9-real-commands9states3contexts','108-additional-reads27rows','totals169reads39rows12crops','one-token-private-save-rebase-reset','two-private-PlaySettings-and-JumpJ-seed-Saves'); $result.checks += ' Debug original/saved whole-frame resets, exact one-body-token private Save/rebase and default display/actual content/private preferences/queued-work retention.' }
    else { $result.actions += @('literal-consumer-only','no-developer-transcript-scratch-or-Save') }
    $result.checks += ' No independent complete body mask/footprint, universal poses/scales, actual Level/story visibility, human/readability/listening/physical-input or performance acceptance.'
}
elseif ($GlassTuneColourOnly) {
    $result.scope = 'glass-tune-colour-only'
    $result.Remove('stage'); $result.stages = @(); $result.player_counts = @(); $result.input_slots = @(); $result.weapons = @()
    $result.palettes = @('authored-opaque-161616','literal-rgb','literal-rgba128','literal-nonblack-alpha0'); $result.resolutions = @('1280x720')
    $result.inputs = 'Retained Monitor.draw/draw_picker/draw_close_button with empty deterministic persona and fixed caller swatches; no Level, input polling, local/Player/Level/State/input/time updates or persona roll.'
    $result.audio = 'No audio consumer/listening acceptance; private preferences unchanged.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.actions = @('four-selected-writes','five-ordered-main-ROIs','two-standalone-Canvas-backgrounds','per-write-wrong-alpha-controls','retained-consumer-repeat')
    $result.checks = 'One live opening.glass_colour path covers all four Monitor writes. Independent raw main proof is only five fixed ROIs: opaque beige, four separate quarter-black passes, lip, glass, opaque program then palette; copied complete Monitor frame is adoption only. Literal standalone panel/close references include white frame/caller swatches/24X dabs and guarded ROIs. Main one effective backdrop, picker+close two genuine Canvas backgrounds. Shared100reads/20rows/20lossless native crops55272pixels,12partial and6alpha0 per-write controls; alpha0 retains lip/program/Canvas and unselected white overlays. Borrowed records/resources, complete unselected style, captured persona/geometry, caller inputs and RNG remain fixed; zero all updates/no event fade.'
    if ($Preset -eq 'x64-debug') { $result.actions += @('9-real-commands9states3methods','108-additional-reads27rows','totals208reads47rows20crops','one-token-private-save-rebase-reset','two-private-PlaySettings-and-JumpJ-seed-Saves'); $result.checks += ' Debug standalone black only; shared owns the second background. Exact one-token private Save/rebase, original+saved full-raster restoration and default display/real content/queued-work retention.' }
    else { $result.actions += @('literal-consumer-only','no-developer-transcript-scratch-or-Save') }
    $result.checks += ' Direct methods only: no actual Level visibility, universal geometry/all phases, human/readability/listening/physical-input or performance acceptance.'
}
elseif ($ActingTuneColoursOnly) {
    $result.scope = 'acting-tune-colours-only'
    $result.Remove('stage'); $result.stages = @(); $result.player_counts = @(); $result.input_slots = @(); $result.weapons = @()
    $result.palettes = @('authored-WHITE-and-8FD3FF','literal-paired-rgb','literal-asymmetric-paired-rgba','literal-paired-nonblack-alpha0'); $result.resolutions = @('1280x720')
    $result.inputs = 'Retained FigureActing only: fixed figure100,100,340,600, both facings/glance and sweat time.2 supplied directly; no Level/Player/state/input/time updates or persona roll.'
    $result.audio = 'No audio consumer/listening acceptance; private preferences unchanged.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Two exact live acting.face/sweat fields. Full-frame independent literal2eye masks/opaque pupils+8sweat strips with explicit per-write premultiplication/no fade; separate frozen-record FigureActing proves adoption only. Four paired palettes×2facings×2opaqueCanvas=16cases, repeated draws, all mask/exterior guards and alpha0 retained BLACK pupils.24per-field straight/squared controls must differ only in the faulty field mask; opaque texel calibration/parity. Shared90reads/16rows/16crops320x128(655360pixels),2blank reads. Borrowed opening/redraw addresses, complete unselected style/resources and all stroke parts/point order/numerics retained; RNG unchanged.'
    if ($Preset -eq 'x64-debug') { $result.actions = @('14-real-statements14-states','right-facing-black-Canvas-only','56-additional-reads14rows0crops','totals146reads30rows16crops','both-path-named-RGB-RGBA-alpha0','Face-reset-retains-partialSweat','Sweat-reset-original-before-one-Face-token-save-rebase','saved-Face-reset-retains-alpha0Sweat','exact-saved-eye-and-final-pair-raster-reversal','two-private-PlaySettings-and-JumpJ-seed-Saves','unchanged-default-display-real-content-private-bytes-queued-work') }
    else { $result.actions = @('literal-consumer-only','no-developer-commands-transcript-scratch-or-Save') }
    $result.checks += ' Actual Level/Player/composed-story visibility remains unrun; real callers source-audited only. Native controlled Save failure absent; CPU owns invalid merged acting.hop rollback. No new Reduce Effects behavior or human pose/readability/comfort/timing, physical input/focus, listening or performance acceptance.'
}
elseif ($ProgramLevelVisibilityOnly) {
    $result.scope = 'program-level-visibility-only'
    $result.stage = 'crossfeed'; $result.player_counts = @(2); $result.input_slots = @(4,3); $result.weapons = @('roller')
    $result.palettes = @('blue_orange'); $result.resolutions = @('1280x720')
    $result.inputs = 'One actual two-player full-opening Level built with seed9411 and neutral setup input; one explicit positive .4f Level update, then no observation updates or input polling.'
    $result.audio = 'Quiet fixture music/sfx0; setup starts room tone at zero gain. No listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.actions = @('baseline-repeat','live-literal-opaque-RGB-repeat','restore-whole-frame-repeat')
    $result.checks = 'Same retained actual Crossfeed Level.draw composition, with local builder borrowing the same mutable local Presentation. Six readbacks/3cases+9samples/12total rows/2full PPMs/0crops, in both presets, no developer commands or Save. Independent literal exposed-program16x16 patch, shipped opaque paperC9D3D9 patch and glass22 guard, mask-only deltas, whole-frame repeats/restoration, frozen public state/unselected style/resources and one RNG bracket around all observations. Construction Scene admission and private story checkpoint are source-derived, not instrumented snapshots. One roster/resolution/seated frame only; Program34 retains separate alpha/command proof. No all-phase, human/readability/listening/physical-input/performance claim.'
}
elseif ($ProgramTuneColourOnly) {
    $result.scope = 'program-tune-colour-only'
    $result.Remove('stage'); $result.stages = @(); $result.player_counts = @(); $result.input_slots = @(); $result.weapons = @()
    $result.palettes = @('authored-opaque-2C2C2C','literal-rgb','literal-rgba128','literal-nonblack-alpha0'); $result.resolutions = @('1280x720')
    $result.inputs = 'Retained public Monitor.draw only with empty deterministic persona; no Player/Level/State/input/time updates or RNG roll.'
    $result.audio = 'No audio consumer/listening acceptance; private preferences unchanged.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'One live opening.program_colour field. Whole-Monitor reference is adoption only. Independent rawROI[135,58,1144,540) writes opaque glassRGB22 then program mask[136,59,1143,539), per-write premultiplication/no fade. EntireROI/glass guards, visible-mask-only delta, opaque parity/calibration, alpha0 retained glass and3straight/squared controls. One effective Canvas backdrop because opaque Monitor/glass erases it. Shared20reads/4rows/4corner crops320x192(245760pixels). Borrowed opening/art, complete unselected style/resources/persona/RNG retained.'
    if ($Preset -eq 'x64-debug') { $result.actions = @('9-real-commands9-states','36-additional-reads9rows','totals56reads13rows4crops','one-token-private-save-rebase-reset','exact-saved-whole-Monitor-reversal','two-private-PlaySettings-and-JumpJ-seed-Saves','unchanged-default-display-real-content-queued-work') }
    else { $result.actions = @('literal-consumer-only','no-developer-commands-transcript-scratch-or-Save') }
    $result.checks += ' Direct draw is not composed story-world visibility evidence. Native controlled Save failure absent; CPU owns invalid merged palette-height rollback. No physical input/focus/human art-readability/listening/performance acceptance.'
}
elseif ($PowerLightTuneColourOnly) {
    $result.scope = 'power-light-tune-colour-only'
    $result.Remove('stage'); $result.stages = @(); $result.player_counts = @(); $result.input_slots = @(); $result.weapons = @()
    $result.palettes = @('authored-opaque-C5D6AA','literal-rgb','literal-rgba128','literal-nonblack-alpha0'); $result.resolutions = @('1280x720')
    $result.inputs = 'Retained Monitor with empty deterministic captured persona; public draw_front_details only at screen(0,0,1280,720). No persona roll, reader sampling/events or local/Player/Level/StateContext/input/time updates.'
    $result.audio = 'No audio consumer/listening acceptance; private preferences unchanged.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'One exact live monitor_art.power_light field. Full-front copied Monitor reference is adoption only. Independent32x32 raw ROI at1237,642 submits opaque BLACK10x10 at1246,652 followed by premult5x4 light at1249,655, preserving ordered target writes/no fade. AllROI, five explicit sites, opaque calibration/parity, six straight/squared controls and alpha0 retained ink; all palette deltas confined to inner5x4. Shared40readbacks/8rows/8crops32x32(8192pixels). Borrowed opening/art addresses, complete unselected Opening style/resources including desktop, persona/RNG frozen.'
    if ($Preset -eq 'x64-debug') { $result.actions = @('9-real-commands9-raster-states','72-additional-reads18rows','totals112-reads26rows8crops','one-token-private-save-rebase-reset','exact-saved-full-front-reversal','two-private-PlaySettings-and-JumpJ-seed-Saves','unchanged-default-display-real-content-queued-work') }
    else { $result.actions = @('literal-consumer-only','no-developer-commands-transcript-scratch-or-Save') }
    $result.checks += ' Native controlled Save failure absent; CPU owns invalid merged shadow-width rollback. No full story visibility route, physical/global input, OS-focus, human small-view/contrast/readability, listening or performance acceptance.'
}
elseif ($DesktopTuneColourOnly) {
    $result.scope = 'desktop-tune-colour-only'
    $result.Remove('stage'); $result.stages = @(); $result.player_counts = @(); $result.input_slots = @(); $result.weapons = @()
    $result.palettes = @('authored-opaque-teal','literal-rgb','literal-rgba128','literal-nonblack-alpha0'); $result.resolutions = @('1280x720')
    $result.inputs = 'One retained actual Monitor with deterministic captured persona; only public draw_closed called on literal screen(0,0,1280,720). No persona roll, reader sampling/events or local/Player/Level/StateContext/input/time updates.'
    $result.audio = 'No audio consumer or listening acceptance; private saved preferences stay unchanged.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'One exact live desktop colour. Full actual RGBA raster equals independent raw white-frame rectangle [136,59,1143,588), width1007 and height529 from per-edge truncation. Manual RGB*alpha once, no event fade. Copied-dependency Monitor comparison proves adoption only. Five explicit centre/boundary sites, all exterior pixels, opaque raw calibration/parity and alpha0 whole-backdrop restoration; six straight/squared controls. Shared40readbacks/8rows/8corner montage crops320x192(491520pixels). Full unrelated Opening style/resources and captured persona/RNG frozen; no production getters.'
    if ($Preset -eq 'x64-debug') { $result.actions = @('9-real-commands-and9-raster-states','72-additional-readbacks-18rows','totals112-readbacks26-rows8-crops','exact-one-token-private-save-rebase-reset','exact-saved-raster-reversal','two-private-PlaySettings-and-JumpJ-seed-Saves','unchanged-default-display-real-content-and-queued-work') }
    else { $result.actions = @('literal-consumer-only','no-developer-command-transcript-scratch-or-Save') }
    $result.checks += ' Native controlled Save failure absent; CPU owns invalid merged glass-width rollback. Isolated closed-desktop consumer, not full story visibility qualification. No physical/global input, OS-focus delivery, human art/readability, listening or performance acceptance.'
}
elseif ($CursorMaterialTuneColoursOnly) {
    $result.scope = 'cursor-material-tune-colours-only'
    $result.Remove('stage'); $result.stages = @(); $result.player_counts = @(); $result.input_slots = @(); $result.weapons = @()
    $result.palettes = @('unchanged-default','distinct-opaque-pair','distinct-partial-rgba-pair','nonblack-zero-alpha-pair')
    $result.resolutions = @('1280x720')
    $result.inputs = 'Synthetic ScreenCursor tip(640,240), draw screen_height720, click_age.09 and1280x720 viewport; no local/Level/State/input updates or retained timers.'
    $result.audio = 'No gameplay audio events; no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.actions = @('retained-rest-click-material-consumer','copied-dependency-full-shape-adoption-only','ordered-shadow-then-shade-raw-writes','separate-shadow-only-write','per-field-straight-squared-controls','zero-selected-alpha-keeps-ink-fill-ticks','reduced-click-retains-rest-arrow','fixed-resources-ink-fill-geometry-global-rng')
    $result.checks = 'Exactly two live cursor shade/shadow fields, no drawing/font/art/default changes. Two independently ordered raw sample patches preserve per-write UNORM quantization: one shifted shadow then foreground S at649,262; one shifted shadow-only run at656,273. Correct base alpha once with no event fade. Four literal pairs/two underlays retain defaults, reject16 targeted wrong-channel comparisons (including shadow under partial shade), and retain ink/fill body/ticks at zero selected alpha. Shared68 readbacks/24CSVrows/8crops72x80(46080pixels),ten wrong-reference frames,two blanks; zero local/Level/State/input updates. Old Cursor23 helper and evidence counts stay unchanged. Full-shape production reference is adoption only; hashes/counts retained.'
    if ($Preset -eq 'x64-debug') {
        $result.actions += @('15-real-named-rgb-rgba-reset-save-statements','independent-leaf-resets-retain-siblings','exact-two-token-private-save-and-rebase','two-private-playsettings-jumpj-saves-default-display-retained')
        $result.checks += ' Debug12 states add144 readbacks/72rows, totals212/96/8. Private source/staged two-token save/rebase/reset and unchanged preferences/JumpJ/default display are checked. Release remains literal consumers without developer commands or Save.'
    }
    $result.checks += ' No physical/global input, OS-focus delivery, human readability, small-view, performance or listening acceptance.'
}
elseif ($RevealTuneColoursOnly) {
    $result.scope = 'reveal-tune-colours-only'
    $result.Remove('stage'); $result.stages = @(); $result.player_counts = @(); $result.input_slots = @(); $result.weapons = @()
    $result.palettes = @('unchanged-default','independent-ink-paper-shade-rgb','independent-ink-paper-shade-rgba128','independent-nonblack-alpha0','combined-rgba')
    $result.resolutions = @('1280x720')
    $result.inputs = 'Fixed synthetic RevealArt bounds(80,80,360,100), two numeric cards at y240, rest/half-age .09; no local/Level/State/input updates. SPACE must measure exactly ZERO in the actual captured font.'
    $result.audio = 'No gameplay audio events; no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.actions = @('retained-space-material-card-and-two-wrapped-team-labels','immutable-copied-dependency-adoption-reference','independent-eleven-site-ordered-raw-alpha-reference','stepped-single-double-material-overlaps','one-three-airbrush-passes-at-point12','half-age-quadratic-point25','straight-squared-base-and-fade-controls','per-field-alpha0-underlayer-retention','reduced-expired-settled-parity','fixed-resources-wrappers-bounds-teams-global-rng')
    $result.checks = 'Exactly three live RevealArt colour leaves; no drawing/font/art/default change. Production-derived full-card/text reference establishes adoption only. Seven independent material sites include each ordered lower layer and repeated overlap; four raw tick sites retain both caller team colours. SPACE is nonempty and measured ZERO before use; glyph pixels are never a solid-channel oracle. Eleven palettes/two backgrounds, 28 wrong-reference frames and96 rejected sample controls. Shared228 readbacks/264 CSVrows/10 crops (six400x120 material crops plus four800x120 label crops,672000 pixels). Zero local/Level/State/input updates; hashes/counts retained.'
    if ($Preset -eq 'x64-debug') {
        $result.actions += @('21-real-named-rgb-rgba-reset-save-statements','independent-leaf-resets-retain-siblings','exact-three-token-private-save-and-rebase','two-private-playsettings-jumpj-saves-default-display-retained')
        $result.checks += ' Debug14 command states add196 readbacks/336 rows, totals424/600/10. Private source/staged/token-only save, rebase/reset and unchanged preferences/JumpJ/default display are checked. Release runs literal consumers without developer commands or Save.'
    }
    $result.checks += ' No physical/global input, OS-focus delivery, human readability, small-view, performance or listening acceptance.'
}
elseif ($SilhouetteTuneColoursOnly) {
    $result.scope = 'silhouette-tune-colours-only'
    $result.Remove('stage'); $result.stages = @(); $result.player_counts = @(); $result.input_slots = @(); $result.weapons = @()
    $result.palettes = @('authored-default','three-distinct-rgb','three-distinct-rgba','three-nonblack-alpha0')
    $result.resolutions = @('1280x720')
    $result.inputs = 'Normal reader backend without sampling/events, persona roll or Player/Level/StateContext/input/time updates. Literal adult+mug and child personas, seated pose, screen rectangles and child-panel camera stay fixed around one preconstructed borrowed Silhouette.'
    $result.audio = 'No audio consumer or listening acceptance; private preferences remain unchanged.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Three exact live silhouette_art fields only; separate opaque body fill stays outside allowlist. Independent raw white-frame 3x3 mug patches use two ink writes, one rim write over unchanged body and four shade writes with (R*A/4,G*A/4,B*A/4,A/4), quantized per target write. Actual patch equality/uniformity and nonzero role deltas qualify shipped sample interiors; five negative controls per underlay reject straight/squared alpha and wrong ink/shade pass counts. Alpha0 retains body and restores exterior underlay. Fixed adult/child footprints, caller persona/pose/screen/camera/resources and draw RNG; exact default restoration. Child panel is aggregate colour adoption, without independent full child geometry. Shared38readbacks/10rows/8crops440x240(844800pixels)/10controls.'
    if ($Preset -eq 'x64-debug') {
        $result.actions = @('13-actual-validated-commands','78-additional-readbacks','26-additional-rows','totals116-readbacks-36-rows-8-crops','three-exact-token-private-save-rebase-reset','sibling-retaining-ink-and-rim-resets','all-three-alpha0-retain-body','named-rim-and-exact-saved-raster-reversal','two-private-PlaySettings-and-JumpJ-seed-Saves','unchanged-default-display-and-real-content','no-queued-match-work')
    }
    else { $result.actions = @('literal-consumer-only','no-developer-commands-transcript-scratch-or-Save') }
    $result.checks += ' Native controlled Save failure absent; CPU owns semantic merged-width rollback. No active story route, physical/global input, OS-focus delivery, human art/readability, listening or performance acceptance.'
}
elseif ($CursorTuneColoursOnly) {
    $result.scope = 'cursor-tune-colours-only'
    $result.Remove('stage'); $result.stages = @(); $result.player_counts = @(); $result.input_slots = @(); $result.weapons = @()
    $result.palettes = @('opaque-default','ink-only-rgb','fill-only-rgb','ink-only-rgba128','fill-only-rgba128','paired-rgba128','paired-alpha0-nonzero-rgb')
    $result.resolutions = @('1280x720')
    $result.inputs = 'Normal reader backend with no sampling/events. Synthetic ScreenCursor inputs: tip(640,240),draw screen_height720,click_age.09. No cursor/Level/State updates or retained age.'
    $result.audio = 'No gameplay audio events; no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.actions = @('preconstructed-resting-and-clicking-consumers','independent-ink-fill-adoption','copied-baseline-full-shape-parity','raw-seven-site-base-alpha-and-layer-samples','straight-and-squared-base-alpha-controls','alpha0-shade-shadow-residuals','reduce-effects-keeps-arrow','unchanged-borrowed-record-geometry-and-global-rng')
    $result.checks = 'Two exact live cursor fields. Full-shape reference is a production cursor with immutable copied dependencies, proving adoption/default parity only. Independent raw patches cover tip,contour,interior,shade,shadow and two isolated click sites, including wide ink beneath narrow fill. Base alpha is already premultiplied once; no production draw change or event fade. Seven authored pairs/two underlays retain shade/shadow and reject wrong straight/squared alpha. Shared124readbacks/112CSVrows/12crops72x80(69120pixels),ten wrong-reference frames,two blank frames; zero local/Level/State updates. Fixed720 draw height is explicit synthetic input,not small-view acceptance. Hashes/counts retained.'
    if ($Preset -eq 'x64-debug') {
        $result.actions += @('fifteen-real-named-rgb-rgba-reset-save-commands','independent-leaf-reset-with-sibling-edit-retained','exact-two-token-private-save-rebase','unchanged-private-preferences-and-content-bytes')
        $result.checks += ' Debug adds168readbacks/192rows across twelve command states,totals292/304/12. Two successful private preference seed Saves; source/staged token-only save and reset retain unrelated values. Release remains literal-consumer-only.'
    }
    $result.checks += ' No physical/global input,OS-focus delivery,human readability,small-view,performance or listening acceptance.'
}
elseif ($RespawnPenTuneColoursOnly) {
    $result.scope = 'respawn-pen-tune-colours-only'
    $result.Remove('stage'); $result.stages = @(); $result.player_counts = @(); $result.input_slots = @()
    $result.palettes = @('opaque-default','literal-rgb','literal-rgba128','literal-alpha0-nonzero-rgb')
    $result.weapons = @('sprayer-builder-fixture-only-no-weapon-activity')
    $result.resolutions = @('1280x720')
    $result.inputs = 'Normal reader backend, no sampling/events/Level/StateContext/input updates. Actual PlayerBuilder prepares one dead sparse-slot Player through3local Player calls(shared) and2more Debug calls; snapshots freeze before captures/commands. Direct outline progress is an argument, with no simulation clock.'
    $result.audio = 'Local Player fixture preferences mute gameplay cues; private saved preferences remain unchanged. No listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'One exact live pen field; four direct outlines(two facings/two partial path budgets) plus actual dead Player. Independent ordered nib/channel oracle preserves overlap blending, source alpha once/no fade, opaque parity, straight/squared-alpha controls and alpha0 underlay restoration. Below/at boundary blank, Reduce Effects essential outline and actual whole/filled Player bypass. Shared62readbacks/18rows/8crops440x160(563200pixels)/10controls/3Player preparations; public Player timer/progress/health/pose/geometry/cull/camera and draw RNG remain frozen. No production getters or gameplay route.'
    if ($Preset -eq 'x64-debug') {
        $result.actions = '9actual tune/reset/save commands and54readbacks/18rows/2Player preparations; totals116/36/8/prep5. One-token private source/staged save/rebase/reset;2successful private PlaySettings+InputBindings seed Saves preserve JumpJ and default display. No controlled Save failure or queued match work.'
    }
    else { $result.actions = 'Literal consumer-only; no developer director/commands/transcript/scratch or Save.' }
    $result.checks += ' No physical/global input, OS-focus, listening, human readability or performance acceptance.'
}
elseif ($IdleDripTuneColoursOnly) {
    $result.scope = 'idle-drip-tune-colours-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @()
    $result.palettes = @('opaque-default', 'literal-rgb', 'literal-rgba128', 'literal-alpha0-nonzero-rgb')
    $result.weapons = @('isolated-sprayer-idle-drip', 'isolated-sniper-idle-drip', 'isolated-bucket-idle-drip')
    $result.resolutions = @('1280x720')
    $result.actions = @('six-preconstructed-attached-and-falling-consumers', 'five-stable-profile-records', 'black-and-nonblack-underlays', 'independent-raw-geometry-and-base-alpha-channels', 'opaque-default-parity', 'straight-and-squared-base-alpha-controls', 'captured-opaque-paint-without-event-fade', 'enabled-and-reduce-effects-blank-restore-guards', 'frozen-public-state-bounds-global-rng')
    $result.input_slots = @()
    $result.inputs = 'Normal GamepadReader backend with no reader sampling, input events, scripted pads, Level/StateContext updates or match route. Six isolated IdleDrip objects borrow Sprayer/Sniper/Bucket profile records and a local Reduce Effects fixture; fifteen local advance calls prepare separate attached/falling states. All five profile addresses, original enabled flags and tool geometry remain stable.'
    $result.audio = 'No gameplay audio events; this renderer proof establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'One common idle-drip keyline reaches all five profile records and all six retained consumers. Literal raw-source rectangles and explicit channels verify source alpha exactly once, with no event fade and no captured-paint conversion. Four palettes/two underlays retain opaque-default legacy parity, reject straight/squared-base-alpha RGB and leave all six opaque paint interiors present at nonblack alpha0. Disabled and Reduce Effects draws remain blank and restore identical pixels without advancing public bead/interval/drop/bounds state. Shared evidence is46 readbacks,ten CSV rows,eight380x260 crops(790400 pixels),eight alpha/parity controls,six guard comparisons,fifteen local advance preparations and zero Level/StateContext updates. Global RNG/public-state checks execute; private cosmetic sequence invariance is source constness only, without an executed future-stream claim. Counts and artifact hashes retained. No Level gameplay, physical/global input, OS-focus delivery, listening, human readability or performance acceptance.'
    if ($Preset -eq 'x64-debug') {
        $result.actions += @('nine-real-named-rgb-rgba-reset-save-commands', 'exact-one-token-private-save-and-rebase', 'unchanged-private-preferences-and-content-bytes')
        $result.inputs += ' Debug adds a private DebugDirector with isolated source/staged/user/profile/TEMP/TMP paths. Two successful private preference seed Saves retain unrelated active/saved preferences; no controlled save failure. These saved preferences are separate from the local renderer Reduce Effects guard.'
        $result.checks += ' Debug adds nine commands,60 readbacks,twenty rows and fifteen local preparation advances. Exact one-token private save/rebase and reset-to-saved are checked alongside all active play/deadzone/display/palette/Jump preferences, saved preference bytes, actual content and no queued match work. Debug totals106 readbacks,thirty CSV rows,eight crops,thirty local preparations,zero Level/StateContext updates.'
        $result['developer_command_scope'] = 'debug-validated-commands'
    }
    else {
        $result.inputs += ' Release remains consumer-only without a developer director, command transcript, scratch tree or nested Save.'
        $result['developer_command_scope'] = 'not-run-release-consumer-only'
    }
}
elseif ($BlockedShotTuneColoursOnly) {
    $result.scope = 'blocked-shot-tune-colours-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @()
    $result.palettes = @('opaque-default', 'literal-rgb', 'literal-rgba128', 'literal-alpha0-nonzero-rgb')
    $result.weapons = @()
    $result.resolutions = @('1280x720')
    $result.actions = @('eight-retained-four-normal-full-half-age-contacts', 'quadratic-half-age-fade-0.75', 'black-and-nonblack-underlays', 'independent-raw-source-geometry-and-premultiplied-channels', 'opaque-default-legacy-parity', 'double-fade-and-straight-alpha-controls', 'stored-team-beads-retained-at-zero-authored-alpha', 'complete-paint-owner-plus-invalid-budgets-and-cue-bounds')
    $result.input_slots = @()
    $result.inputs = 'Normal GamepadReader backend; no reader sampling, keyboard events, scripted pads, Level/StateContext updates or match objects. Two preconstructed BlockedShotFeedback instances retain four synthetic cardinal contacts each; only the second receives one local half-age preparation update.'
    $result.audio = 'No gameplay audio events; this renderer proof establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Both presets compare retained full/half-age contacts against independent raw geometry, literal stored team colours and explicit source-alpha channels. Half age keeps the existing quadratic0.75 fade; no linear0.5 substitution. Four literal highlight/keyline palettes on two underlays include opaque legacy parity, wrong double-fade and straight-alpha controls, and nonblack RGB with both authored alpha values zero while stored team beads remain. Frozen snapshots include every contact tint/position/normal/owner/age, all paint-owner repeat budgets plus the shared invalid bucket, per-cue/aggregate bounds, assets/geometry and draw RNG. Shared evidence:32 readbacks,eight CSV rows,eight440x240 crops(844800 pixels),eight controls,one local age preparation,zero Level/StateContext updates. Artifact counts and hashes retained. No shot-launch classification, Level route, gameplay, physical/global input, OS-focus delivery, listening, human readability or performance acceptance.'
    if ($Preset -eq 'x64-debug') {
        $result.actions += @('fifteen-real-named-rgb-rgba-reset-save-commands', 'exact-two-token-private-save-and-rebase', 'unchanged-private-preferences-and-source-staged-bytes')
        $result.inputs += ' Debug adds a private DebugDirector with isolated source/staged/user/profile/TEMP/TMP paths and exactly two successful private preference seed Saves; no controlled save failure.'
        $result.checks += ' Debug adds15 commands,60 readbacks,20 CSV rows,zero crops and one local age preparation on retained borrowed consumers. Exact two-token private save/rebase and reset-to-saved baseline are checked with actual content, all active play/deadzone/display/palette/Jump preferences and saved preference bytes unchanged, with no queued match work. Debug totals92 readbacks,28 CSV rows,eight crops,two local age preparations,zero Level/StateContext updates.'
        $result['developer_command_scope'] = 'debug-validated-commands'
    }
    else {
        $result.inputs += ' Release remains consumer-only without a developer director, command transcript, scratch tree or nested Save.'
        $result['developer_command_scope'] = 'not-run-release-consumer-only'
    }
}
elseif ($CombatTuneColoursOnly) {
    $result.scope = 'combat-tune-colours-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @()
    $result.palettes = @('opaque-default', 'literal-rgb', 'literal-rgba128', 'literal-alpha0-nonzero-rgb')
    $result.weapons = @()
    $result.resolutions = @('1280x720')
    $result.actions = @('four-isolated-full-half-hurt-elimination-cues', 'black-and-nonblack-underlays', 'independent-raw-source-geometry-and-premultiplied-channels', 'opaque-default-legacy-parity', 'double-fade-and-straight-alpha-negative-controls', 'unchanged-opaque-team-interior', 'frozen-public-timers-death-bounds-rng')
    $result.input_slots = @()
    $result.inputs = 'Normal GamepadReader backend; no reader sampling, keyboard events, scripted pads, Level/StateContext updates or match objects. Four preconstructed CombatFeedback consumers receive only local cue preparation: hurt/eliminate plus exactly two effect-age update calls per fixture.'
    $result.audio = 'No gameplay audio events; this renderer probe establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Both presets compare frozen full/half hurt and elimination cues across four literal highlight/keyline palettes and two underlays against independent raw source rectangles and explicit RGBA channels. Opaque defaults retain legacy pixels; controls reject double-fade and straight-alpha tint. Public timers/death rectangles, geometry/bounds, team interior and draw RNG remain unchanged. Consumer evidence is32 readbacks,eight CSV rows,eight420x360 crops(1209600 pixels),eight controls,two local half-age preparations and zero Level/StateContext updates. Retained artifact hashes and counts are checked; no gameplay, physical/global input, OS-focus delivery, listening, human readability or performance acceptance.'
    if ($Preset -eq 'x64-debug') {
        $result.actions += @('fifteen-real-validated-named-rgb-rgba-reset-save-commands', 'two-token-private-save-and-rebase', 'unchanged-private-preferences-and-content-bytes')
        $result.inputs += ' Debug adds a private DebugDirector with isolated source/staged/user/profile/TEMP/TMP directories and exactly two successful private preference seed Saves, with no controlled save failure.'
        $result.checks += ' Debug adds15 commands,60 readbacks,20 CSV rows,zero crops and two local half-age preparations on preconstructed borrowed consumers. Exact private two-token save/rebase and reset-to-saved baseline are checked with actual content/preferences unchanged and no queued match work. Debug totals92 readbacks,28 CSV rows,eight crops,four age preparations,zero Level/StateContext updates.'
        $result['developer_command_scope'] = 'debug-validated-commands'
    }
    else {
        $result.inputs += ' Release remains consumer-only without a developer director, command transcript, scratch tree or nested Save.'
        $result['developer_command_scope'] = 'not-run-release-consumer-only'
    }
}
elseif ($FeedbackTuneColoursOnly) {
    $result.scope = 'feedback-tune-colours-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @()
    $result.palettes = @('opaque-default', 'literal-rgb', 'literal-rgba128', 'literal-alpha0-nonzero-rgb')
    $result.weapons = @('isolated-sprayer-feedback-only')
    $result.resolutions = @('1280x720')
    $result.actions = @('nine-isolated-event-and-readiness-cells', 'full-and-half-life', 'black-and-nonblack-backgrounds', 'independent-raw-source-premultiplied-channels', 'opaque-legacy-parity', 'double-fade-and-straight-alpha-negative-controls', 'reduce-effects-static-information', 'opaque-team-centres', 'frozen-timers-bounds-rng')
    $result.input_slots = @()
    $result.inputs = 'Normal GamepadReader backend; no reader sampling, keyboard events, scripted pads, StateContext updates or match objects. The nine-cell fixture directly calls existing WeaponFeedback consumers with frozen cue states and literal colours.'
    $result.audio = 'No gameplay audio events; this renderer probe establishes no listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Both presets render empty/recovered/Sprayer muzzle at full and half life plus all three readiness shapes over black and nonblack. Independent raw-source sprite geometry and explicit RGBA channels verify four literal palettes, opaque-default legacy parity, rejection of RGB double-fade and straight-alpha submissions, unchanged opaque team centres, Reduce Effects policy and frozen timers/bounds/draw RNG. Shared evidence is exactly zero StateContext updates,56 readbacks,eight CSV rows,eight360x280 crops and eight parity/negative controls. No gameplay, global input, physical-device, listening, human readability or performance acceptance.'
    if ($Preset -eq 'x64-debug') {
        $result.actions += @('fifteen-real-validated-named-rgb-rgba-reset-save-commands', 'two-token-private-save-and-rebase', 'unchanged-active-preferences-and-private-bytes')
        $result.inputs += ' Debug additionally uses a private DebugDirector and isolated source/staged/user/profile/TEMP/TMP paths; exactly two successful Save calls seed private preferences, with no controlled save failure.'
        $result.checks += ' Debug adds15 real validated named/RGB/RGBA/reset/save commands on the same preconstructed borrowed consumers,66 readbacks,20 CSV rows and zero extra crops. Exact two-token source/staged save and ledger rebase, unchanged tuning/actual content/private preferences and absence of queued match work are checked. Total Debug evidence is122 readbacks,28 CSV rows and eight crops.'
        $result['developer_command_scope'] = 'debug-validated-commands'
    }
    else {
        $result.inputs += ' Release runs consumer-only evidence and creates no developer director, command transcript or nested save.'
        $result.checks += ' Release evidence remains consumer-only:56 readbacks,eight CSV rows and eight crops. No developer commands are executed or claimed.'
        $result['developer_command_scope'] = 'not-run-release-consumer-only'
    }
}
elseif ($PercentageTextOnly) {
    $result.scope = 'percentage-text-only'
    $result.Remove('stage')
    $result.stages = @()
    $result.player_counts = @()
    $result.palettes = @()
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('four-literal-percentage-value-rows', 'saved-volume-17-0-100', 'inherited-cutoff-rounded-100', 'explicit-deadzone-0-90', 'actual-music-sfx-movement-aim-adjustments', 'long-value-column-fitting', 'captured-template-and-visible-numeric-font-metrics', 'refresh-child-return-reopen-reversal', 'apply-reload-repeat', 'active-versus-disk-save-failure', 'pending-discard', 'frozen-preview-rng-resize')
    $result.input_slots = @(4)
    $result.inputs = 'Actual Controls/audio parent and Pad Sticks child with the normal GamepadReader and public local keyboard events. All four nested private pad profiles are seeded identically; the existing page chooses its initial slot. No scripted pad backend, foreground/global input, physical-device or gameplay claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated both-preset leaf for menu_text.values.percentage. Independent literal strings and UiText/OptionRow-equivalent linear references cover Controls music/SFX rows2/3 and Sticks movement/aim rows1/2 values, saved17/0/100, inherited movement0.999 rounding to100 and explicit0/90. Existing captured controls item fonts and width-then-height value fitting remain intact. Old long copy with tilde, a new font ID missing tilde and distinct numeric bearings, real adjustments, child return, reopen/reversal and zero-update720/1080/720 drawing comparisons prove retention and adoption. Actual child Apply/reload/repeat preserves pending parent volumes and unrelated preferences; controlled private-save failure keeps active10/85 while disk remains5/85, and later Escape discards pending edits. Full private save bytes, observed status colours, unchanged current preview across source replacement, frozen redraw and draw RNG are checked. Exactly123 state updates(hard123),87 readbacks,52 value CSV rows,4 status comparisons and8 compact crops per preset. No broad Controls matrix, gameplay, foreground/global input, physical-pad, hardware qualification, listening, human readability or performance acceptance.'
}
elseif ($StageCountsOnly) {
    $result.scope = 'stage-counts-only'
    $result.Remove('stage')
    $result.stages = @('movement_course', 'close_quarters', 'king_of_the_hill', 'turbulence')
    $result.player_counts = @(1)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('bucket')
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('literal-navigation-hint-status-record-rasters', 'mixed-four-three-eligible', 'excluded-one-zero-eligible', 'mixed-two-one-eligible', 'observable-random-lock-unlock-concrete', 'positive-record-room-and-fit', 'random-record-absence', 'captured-templates-fonts-positions', 'cached-versus-refreshed-hint-fit-shadow', 'reopen-reversal', 'private-save-plan-keyboard-claim-rng')
    $result.input_slots = @(4)
    $result.inputs = 'Actual solo Stage Select with the normal gamepad reader and public local keyboard events only. Mixed four-stage/three-eligible, excluded singleton/zero-eligible and mixed two-stage/one-eligible lists exercise actual navigation and Random admission. A nested private profile seeds long practice records and muted Reduce effects preferences. No scripted pad backend, foreground/global input or physical-device claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated both-preset probe for menu_text.stage.numbered_navigation/random_hint at720/1080. Independent literal navigation/hint/status/record strings and UiTextDropShadow rasters verify all four actual mixed-list indices, eligible0/1/3, unchanged singular available-arenas wording, excluded-only navigation refusal, first Random lock/unlock and concrete return. Every state proves actual status, record presence or empty record column, finite positive record room before an independently right-aligned width fit, and full shadow/paper bounds. A private paper reference derives card expansion from the supplied literal hint and fixed solo rows, without the older hardcoded Random-count leaf or new production formatters. Old/new templates, separate visible-digit detail/brand font IDs and captured navigation position/shadow persist through presentation assignment and actual refresh; hints preserve constructor copy/font/position/ink while adopting live width-fit/shadow on slot/state changes, then reopen/reverse correctly. Exact private save/records, match plan, keyboard ownership and frozen/provisional RNG stay unchanged. Exactly104 updates(hard120),54 observed states with four literal roles each,216 CSV data rows and8 compact crops per preset. No final Random draw, match launch, broad matrix, physical-device, listening, human readability or performance acceptance.'
}
elseif ($StageSummaryOnly) {
    $result.scope = 'stage-summary-only'
    $result.Remove('stage')
    $result.stages = @('close_quarters', 'movement_course', 'eligible-random-browse-lock-only')
    $result.player_counts = @(1, 2, 3, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('bucket', 'sniper', 'sprayer', 'mister')
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('literal-composed-mode-roster-rasters', 'best-of-three-five', 'solo-best-five-precedence', 'single-match-preserved', 'all-four-sparse-rosters', 'captured-font-ids-visible-digit-bearing', 'clear-shadow-bounds', 'actual-browse-lock-unlock-refresh', 'literal-navigation-status-state-guards', 'captured-copy-reopen-reversal', 'private-save-plan-claims-input-rng')
    $result.input_slots = @(4, 3, 0, 2)
    $result.inputs = 'Actual Stage Select with public local keyboard events and neutral scripted connected sparse pad snapshots. Four contexts cover solo best-of-five precedence,2P best-of-three,3P best-of-five and4P Single Match. A nested private profile applies Reduce effects and muted volumes, keeping static text responses settled without extra timing updates. No foreground/global input or physical pad.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated compact both-preset probe for menu_text.stage.best_of_mode/solo_roster/versus_roster. Independent literal mode/roster strings and unscaled UiTextDropShadow references use the actual captured detail/brand fonts, positions, paper ink and zero-alpha shadow with retained full shadow bounds; no production summary formatter supplies expectations. Defaults cover every roster count and both best-of values plus solo precedence/Single Match at720/1080. Short old/new templates preserve old pages through actual concrete-to-Random browse, first lock, unlock and return refresh; reopening adopts the new copy and separate font IDs whose numeric bearings visibly differ, and reversal retains the reopened snapshot until another reopen restores defaults. Independent summary rows fit the authored paper and precede following rows; only the existing paper leaf is reused. Match settings/stage/duration/layout, sparse claims/teams/tools, neutral input, exact private save bytes and draw/provisional-action RNG stay unchanged. Exactly112 updates(hard120),60 summary comparisons,64 additional independent literal navigation/status raster guards after32 actual taps,120 CSV data rows with a refresh_state_verified column and8 compact two-row crops. No final Random confirmation, match launch, broad briefing/MenuText matrix, Arena counter/availability text, physical-device, listening, human readability or performance acceptance.'
}
elseif ($MenuLabelsOnly) {
    $result.scope = 'menu-labels-only'
    $result.Remove('stage')
    $result.player_counts = @(3, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('literal-player-device-labels', 'sparse-reordered-rosters', 'join-waiting-explicit-claim-reconnect', 'captured-copy-fonts-refresh-reopen-reversal', 'team-weapon-labels', 'pause-missing-packing', 'results-contribution-truncation', 'stick-fitted-device-value', 'stick-connected-disconnected-readout', 'frozen-rng-resize')
    $result.input_slots = @(3, 4, 0, 2, 1)
    $result.inputs = 'Actual Join/Team/Weapon/Pause/Results/Stick pages with deterministic scripted pad snapshots and local keyboard menu events. Sparse/reordered pad/keyboard claims and an all-pad missing list exercise device numbers independently of player numbers. A nested private profile preserves exact save bytes; no foreground/global input or physical-pad claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Bounded actual-page literal rasters cover the four authored label fields and every menu consumer without calling device_label/roster_label/player_label in the oracle. Defaults preserve Waiting to join. Short alternate pad/keyboard/waiting/row templates reorder device and player text; captured copy/fonts survive presentation assignment and actual Join/Pause/Stick refresh, adopt on reopen, and reverse on reopen. Independent UiText/UiTextDropShadow references preserve Join item shadow, Team item shadow/card, Weapon default2,2 transparent shadow, plain point-filtered Pause, zero-shadow Results contribution rows and fitted plain Stick option/help fonts. Explicit Join claim/reconnect preserves readiness/ownership, Pause keeps its three-space/two-per-line packing, and Results retain supplied canonical player numbers/Team suffixes plus literal fractional truncation12.375to12.3 and21.625to21.6. Save/settings/claims/tools/result data, frozen pixels and RNG stay unchanged. At most120 page updates, concise CSV and exactly eight small crops total per preset across720/1080. No broad matrices, full matches/series, developer commands, physical-device, human readability or performance acceptance.'
}
elseif ($WeaponTuneColoursOnly) {
    $result.scope = 'weapon-tune-colours-only'
    $result.Remove('stage')
    $result.player_counts = @(2)
    $result.palettes = @('blue-orange')
    $result.weapons = @('sprayer', 'mister')
    $result.resolutions = @('1280x720')
    $result.actions = @('validated-two-weapon-colour-commands', 'frozen-before-update-retention', 'inactive-missing-player-retention', 'neutral-reconnect-refresh', 'path-reset-reversal', 'independent-premultiplied-card-rasters', 'captured-canvas-and-outside-card-invariants', 'private-content-profile', 'frozen-draw-rng')
    $result.input_slots = @(3, 4)
    $result.inputs = 'Four actual DebugDirector tune/tune.reset commands use private source/staged/user/profile and TEMP/TMP directories. Actual Weapon Select uses sparse Pad4/keyboard seats, fixed Sprayer/Mister and blue-orange paint, with P1 ready and P2 unready. Only the GamepadReader seam is scripted; no foreground/global input or physical pad.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated Debug-only hidden720p proof for presentation.main_menu.weapon_select.selected/unselected. Two validated RGBA tune commands change both leaves; frozen draws, inactive updates and missing-claimed-device updates retain their prior card tints, then one neutral active connected update adopts both without another confirmation. Independent explicit RGBA inputs and fixed Sprayer/Mister artwork references verify premultiplied readiness paper separately from the unchanged opaque body/paint/supply canvases. Missing-device footer changes are checked within their own state; after reconnect every pixel outside the two separate card rectangles equals the connected baseline, preserving captured names/descriptions/fonts/catalog/paper. Sparse roster/tools/settings/neutral inputs, fixed palette, exact private save bytes and draw RNG remain unchanged. Two path-specific resets retain frozen widgets until refresh, then restore the exact same-page baseline with an empty edit ledger. Actual content and scratch source/staged bytes stay unchanged. At most16 bounded updates, exactly two compact crops, six-row CSV and echoed director transcript. No broad matrices, full matches or performance/human/physical-pad acceptance.'
}
elseif ($TeamTuneColoursOnly) {
    $result.scope = 'team-tune-colours-only'
    $result.Remove('stage')
    $result.player_counts = @(2)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @()
    $result.resolutions = @('1280x720')
    $result.actions = @('validated-two-team-colour-commands', 'frozen-before-update-retention', 'active-connected-refresh', 'path-reset-reversal', 'ready-unready-cursor-invariants', 'captured-frames-fonts-paper', 'private-content-profile', 'frozen-draw-rng')
    $result.input_slots = @(3, 4)
    $result.inputs = 'Actual DebugDirector tune/tune.reset commands use private source/staged/user/profile and TEMP/TMP directories. Actual Team Select has opposing sparse Pad4/keyboard seats, with P1 ready and P2 unready. Only the GamepadReader seam is scripted; no foreground/global input or physical pad.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Dedicated Debug-only hidden720p proof for presentation.main_menu.team_select.selected/unselected. Two real validated RGB tune commands change both leaves; frozen draws retain the previous refresh colours, then a neutral active connected page update adopts both. Independent fixed A/B and P1-ready/P2-unready icon/paper rasters verify colours without FocusGroup or production formatting; all outside-strip pixels, captured frames/fonts/lettering/paper, sparse roster/tools/settings/input snapshots, exact private save bytes and draw RNG remain unchanged. Two path-specific tune.reset commands retain frozen widgets until refresh and then restore the exact same-page baseline with an empty edit ledger. Actual content and scratch source/staged bytes remain unchanged. At most12 bounded page updates, exactly two small icon-strip crops, three-row CSV and echoed director transcript. Existing TeamFocusOnly and general TuneColoursOnly matrices, full matches, broad save/refusal tests and performance/human/physical-pad acceptance are not invoked.'
}
elseif ($TeamFocusOnly) {
    $result.scope = 'team-focus-only'
    $result.Remove('stage')
    $result.player_counts = @(1, 2, 3, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('semantic-team-cursors', 'nonwrapping-endpoints', 'sparse-reordered-slots', 'ready-seat-tint-retention', 'cursor-frame-local-motion', 'confirm-back-priority', 'shuffle-priority-readiness-reset', 'empty-team-refusal', 'reconnect-fresh-press', 'committed-weapon-route-return', 'captured-atlas-frames-text-fonts', 'live-colours-on-refresh', 'nonzero-origin-reversed-physical-order', 'frozen-draw-rng-resize')
    $result.input_slots = @(3, 4, 0, 2)
    $result.inputs = 'Actual Team Select and Weapon Select pages over a callback sentinel root. Deterministic local keyboard and scripted GamepadReader snapshots exercise sparse/reordered seats, including keyboard/mouse slot4. No physical pad, foreground/global input or full match.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Compact720/1080 native scope: independent semantic choices/readiness and plain icon/paper/title/label rasters cover1-4players, A/none/B nonwrap endpoints, no-team confirmation refusal, a locked ready seat while another cursor moves, local selection motion/reduction, simultaneous Confirm/Back and global Shuffle priority, empty-team final confirmation, missing/reconnected pad fresh-press gating, exact team commit into actual Weapon Select and unready return. Sparse claims/tools/stage/layout/duration/best-of and the ordinary callback sentinel remain intact. Captured atlas/frame names/text/fonts retain old widgets through assignment/reversal and adopt on reopen; existing live team colours adopt on visual refresh. An in-memory uniquely named sheet clones the six shipped64-square Team frames over the existing texture with A/B pivots-128/+128; their50-unit canvases exchange physical positions while semantic navigation stays unchanged. No source assets are written. Frozen drawing/RNG and resize checks, at most240 updates, concise CSV and exactly eight compact success crops total per preset. Broad TeamSetupOnly/TeamPaperOnly matrices, full matches/series and performance/human/physical-pad acceptance are not invoked.'
}
elseif ($StickReadoutsOnly) {
    $result.scope = 'stick-readouts-only'
    $result.Remove('stage')
    $result.player_counts = @()
    $result.palettes = @('runtime-save-default')
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('literal-stick-readouts', 'all-four-pad-slots', 'disconnected-zero', 'half-percent-rounding', 'signed-movement', 'radial-cutoff', 'active-aim-direction', 'inherited-pending-deadzones', 'discard-no-save', 'captured-help-font-templates', 'reopen-reversal', 'frozen-draw-rng')
    $result.input_slots = @(0, 1, 2, 3)
    $result.inputs = 'Actual StickSettingsMenu and local keyboard menu snapshots, with the focused scripted GamepadReader supplying deterministic raw stick vectors and connection state. A nested private profile holds the immutable preview/discard save fixture; no foreground/global input or physical-pad claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Small720/1080 readout scope with prewritten numeric/literal copy instead of production formatting or stick-response helpers. Actual Pad cycling covers every slot; zero and stale disconnected input, positive/negative half-percent rounding, signed movement, a 3-4-5 radial boundary/diagonal and active/inactive aim are compared as complete unscaled plain-text rasters in independent bounds over a standalone paper background. Pending inherited50-to55percent movement/aim cutoffs alter only preview readouts; refreshes and discard preserve active settings and exact private save bytes, and reopen restores inherited responses. All six custom templates and a replacement loaded help font remain captured through assignment and live input/connection refresh, adopt on reopen and reverse only on reopen; the existing Move-height anchor remains independent of template wording. At most180 zero-dt updates, concise CSV and exactly eight compact crops total per preset. Existing broad StickSettingsOnly navigation/Apply/save checks are not invoked. No physical-pad, listening, human readability or performance qualification.'
}
elseif ($ConnectionTextOnly) {
    $result.scope = 'connection-text-only'
    $result.Remove('stage')
    $result.player_counts = @(2, 3, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('literal-connection-copy', 'sparse-claims', 'neutral-no-autojoin', 'pause-player-numbering', 'restart-loss-gate', 'fresh-reconnect-resume', 'team-warning', 'captured-text-font-refresh', 'reopen-reversal', 'frozen-draw-rng')
    $result.input_slots = @(3, 4, 0, 2)
    $result.inputs = 'Actual Join/Pause/Team pages and local MenuInput snapshots; the focused GamepadReader seam supplies deterministic sparse connections/buttons. No foreground/global input or physical-device claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Bounded connection-copy scope at720/1080: independently supplied literal text, unscaled full text/shadow bounds and native rasters over a standalone paper background; fallback Pause title and all four visible player numbers use reordered device slots. Actual Join distinguishes unclaimed, lost, returned-neutral and explicitly claimed states. Actual Restart confirmation falls back on a missing participant, Resume/Restart labels disable, missing-player Back cannot resume, and a held/released reconnect action needs a fresh press. Team warning shows only when the last confirmation would leave one team empty and clears after a provisional valid edit. All eight customized connection strings and three replacement font roles remain captured across presentation assignment and real refreshes, adopt on reopen and reverse only on reopen. Choices/claims remain unchanged except the one accepted Join action; frozen drawing preserves pixels/RNG. At most300 zero-dt page updates, concise CSV and exactly eight compact crops total per preset; no broad painter matrix, duplicate fullscreen captures, human readability, listening or performance qualification.'
}
elseif ($RememberedSetupOnly) {
    $result.scope = 'remembered-setup-only'
    $result.Remove('stage')
    $result.stages = @('king_of_the_hill', 'turbulence', 'close_quarters', 'movement_course')
    $result.player_counts = @(2)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('sprayer', 'sniper', 'mister', 'bucket')
    $result.resolutions = @('1280x720')
    $result.actions = @('actual-normal-menu-start', 'last-match-save', 'editable-cancelled-choices', 'fresh-sparse-claims', 'fresh-confirmation', 'pause-restart', 'series-continuation', 'developer-bot-replay-exclusion', 'separate-process-relaunch')
    $result.input_slots = @(3, 4)
    if ($Preset -eq 'x64-debug') {
        $result.player_counts += 4
        $result.input_slots += -1
        $result.weapons += 'roller'
    }
    $result.inputs = 'Actual GameMenu owns setup pages and the typed normal/developer launch callback. The existing scripted GamepadReader seam and local keyboard snapshots drive menus without foreground or global input. All saves, developer files and replay evidence are under a nested private profile.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Two hidden720p processes share only a new private remembered-setup profile. The writer drives actual menus, compares fresh independent Mode/Count/Join/Team/Weapon/Stage page rasters, preserves the prior save through provisional/cancelled changes and the first arena confirmation, then saves resolved choices through the real normal GameMenu callback. A normal Pause Restart and two drawn best-of-three rounds update records independently of last_match. Debug additionally starts a sparse pad/keyboard/device-less bot match through an actual private script, records and replays it through the real director/menu path, and changes a normal menu choice while an explicitly claimed seat is bot-driven; all preserve the user setup. A separate native reader process reloads Save and GameMenu, restores choices, begins with a missing pad and fresh claims, reconnects without autojoining, requires each player to confirm again and cancels a changed arena without saving. Seven compact crops, CSV and command/replay evidence retain frozen page pixels/RNG. Release runs normal menus/relaunch and excludes developer branches. No physical-pad, listening, human readability or performance acceptance.'
}
elseif ($TuneColoursOnly) {
    $result.scope = 'tune-colours-only'
    $result.Remove('stage')
    $result.stages = @('movement_course')
    $result.player_counts = @(1)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('roller')
    $result.resolutions = @('1280x720')
    $result.actions = @('validated-colour-command', 'live-unpainted-movement', 'live-debug-text', 'live-station-rgba', 'hidden-station-zero', 'captured-home-mode', 'reopen-reset-reversal', 'resource-rejection', 'queued-reload-guards', 'staged-reload-fallback', 'scratch-save', 'frozen-state-rng')
    $result.input_slots = @(4)
    $result.inputs = 'Real DebugDirector validated commands and actual consumers/pages/match; no foreground input or physical-device claim. Source/staged/user/profile and candidate TEMP/TMP directories are private to this probe.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Debug-only hidden720p renderer proof for seven audited existing colour strings through the real validated tune path: unpainted movement fallback/highlight/keyline, ordinary DebugText foreground/shadow and existing station Label premultiplied RGBA update live; Home and Mode retain captured backgrounds until reopening, including reset reversal. Named/RGB/RGBA values, same-size station refresh and hidden zero tint, complete resource candidate rejection, actual queued scratch reload refusing tune/reset/save until rebuild/rebase, staged fallback when source files are absent, private save/reload and original content preservation. Compact native crops/CSV, repeated frozen pixels, draw RNG and real match observations/Debug fingerprint retained. No Release native run, physical-pad, human readability, listening or performance acceptance.'
}
elseif ($ParallaxOnly) {
    $result.scope = 'parallax-comparison-only'
    $result.Remove('stage')
    $result.stages = @('close_quarters', 'king_of_the_hill', 'turbulence', 'movement_course')
    $result.player_counts = @(1, 4)
    $result.palettes = @('blue_orange')
    $result.weapons = @()
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('32-clean-heavy-gameplay-camera-cases', 'ordinary-scene-factor-zero-parity', 'independent-raw-atlas-offset-reference', '16-literal-clipped-edges', 'opaque-static-foreground-masks', 'matching-opaque-prop-reference-samples', 'live-reduced-static-parity', 'actual-static-stage-preview', 'resize-return', 'frozen-draw-rng')
    $result.input_slots = @()
    $result.inputs = 'Comparison fixture only: real builder-owned scenery and painted terrain in Scene; PlayerCamera::follow uses the actual full-screen-height gameplay scale for 1P/4P, authored spawns and explicitly placed landmark-near focus points with authored camera bounds. Independent panes, normal reader, no synthetic native activation or input. Placed points establish no route reachability.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Bounded static/A/B current-arena prototype only. A moves far sun/clouds/birds by2percent horizontal camera-centre lag capped32worldunits; B adds one shared1percent hill lag capped16, both snapped in4worldunits about each unchanged overview centre. Actual objects draw with local per-view cameras and matching culls; static props/shadows/ordinary terrain and paint restore the original camera. Ordinary Scene/factor0 parity precedes independent unculled raw-atlas/destination comparisons; frozen cloud phase is read through rectangle(), while isolated nondrifting Sun/hill edges use literal sources/displacements and negative/fractional origins outside the ordinary cull.32 clean/heavy cases, measured>99percent real paint ownership, opaque black/white foreground and matching prop-reference samples,4P independence, resize/back, Reduce effects plus cloud-anchor parity, neutral anchor and actual unchanged StagePreview.372 readbacks and exactly8 native1536x720 contact sheets, each512x360 cells: columns static/A/B, rows1Pspawn/4Pplaced-focus. Conservative far/hill cull-candidate counts do not imply raster contribution. Per-view CSV rows repeat framewide A/static and B/A deltas; count totals increment once per primary case. Far absence in gameplay panes is recorded, not hidden by changing zoom. No shipped parallax, new art/levels/schema, live-match transitions, route, human style/readability, physical-device, listening or performance acceptance.'
}
elseif ($LandmarkOnly) {
    $result.scope = 'static-landmarks-only'
    $result.Remove('stage')
    $result.stages = @('close_quarters', 'king_of_the_hill', 'turbulence', 'movement_course')
    $result.player_counts = @(1, 4)
    $result.palettes = @('blue_orange', 'blue_yellow', 'pink_green')
    $result.weapons = @('roller')
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('48-clean-heavy-pairs-96-primary-states', 'authored-overview-pane-framing', 'measured-heavy-owned-paint', 'positive-each-landmark-pixels', 'independent-raw-atlas-reference', 'terrain-paint-occlusion-order', 'native-scene-edge-parity', 'static-reduce-effects-update', 'actual-stage-preview', 'actual-level-monitor-overview', 'frozen-state-rng-debug-fingerprint', 'resize-return')
    $result.input_slots = @(3, 0, 1, 2)
    $result.inputs = 'Static actual LevelObjectBuilder/Scene geometry uses one/four pane layouts with unchanged authored overview cameras; it does not claim player gameplay framing. Supplemental actual LevelBuilder solo/four-player stories use neutral sparse PlayerInputData and private scratch profiles. Ordinary GamepadReader backend, no foreground/global input or physical-pad claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Exactly48 clean/heavy paired comparisons /96 primary coverage states across4 arenas x3 palettes x1P/4P pane layouts x720/1080. Heavy coverage is actual alternating-team projectile contact on every StructurePaintable, measured above99percent; each prop has positive surviving pixels. Literal native placement/WHITE/Visual/nonpaintable checks and raw atlas-source draws independently replace only the two scenery calls before actual terrain/paint, proving pixels, occlusion order and culling. Frozen drawing preserves geometry/paint attribution/RNG and native resize returns. Supplemental64 native clipped-edge cases compare actual Scene and miniature traversal to unculled raw atlas, with static update/Reduce effects silhouette invariance. Eight actual StagePreview size cases compare independently specified physical panes/cameras;24 actual Level monitor-preview/overview size cases compare identically seeded twins with a private stage copy omitting exactly the two props, preserving gameplay observations and frozen Debug replay fingerprints. Solo bypasses the monitor. All reference/repeat readbacks and paired story ticks are counted separately. Exactly8 compact native clean/heavy crops total,48-row primary CSV and bounded integration CSV; no broad older terrain matrix, full match gameplay route, human readability, listening, physical-device or performance acceptance.'
}
  elseif ($RouteWarningOnly) {
      $result.scope = 'route-warning-only'
      $result.Remove('stage')
      $result.stages = @('crossfeed', 'movement_course')
      $result.player_counts = @(1, 2, 3, 4)
      $result.palettes = @('blue_orange')
      $result.weapons = @('sprayer', 'sniper', 'mister', 'bucket')
      $result.resolutions = @('1280x720')
      $result.actions = @('continuous-landing-interval-observations', 'actual-moving-approaches', 'single-paid-launch', 'post-tick-projectile-candidates', 'first-damage', 'selected-native-captures', 'frozen-state-rng')
      $result.input_slots = @(3, 1, 2, 0)
      $result.inputs = 'Actual Level/Player/Scene fixtures constructed through public builders. Controlled initial stations, fixed launch inputs and direct projectile heavy-paint contacts are labeled; all later movement and launch flight use ordinary per-slot PlayerInputData and the real simulation update order. Simulated 60 Hz ticks establish no physical-input or display-latency claim.'
      foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
      $result.checks = 'Focused native720p route/warning evidence with continuous post-Level-update observations and selected frozen captures: Crossfeed moving approaches and revised Movement Course tap/held/running landings in sparse 1/2/3/4-player layouts, clean and controlled-heavy states, plus representative single-paid-burst incoming aimed tools. Body-width landing strips and useful near edges are observed relative to actual commitment/contact; actor presence, projectile collision/cull candidates, HUD overlap and first damage are separate clocks. Bounds entering a pane do not establish surviving on-screen projectile pixels or human readability. No-hit cases are censored; launch/contact retirement can leave zero drawable frame opportunities. At most40 full frames,100 native crops,15000 Level updates and25000 observation rows; readbacks equal twice the full-frame count. Frozen drawing preserves pixels, observed state, draw RNG and Debug replay fingerprints. No other painter matrices, attended play, physical pads, listening, display/input latency, reaction-time or competitive-balance acceptance.'
      $result.checks += ' Mister first-candidate timing is the earliest geometric candidate among five pellets from one paid burst; it does not identify the damaging pellet or prove its warning duration. Native crop RGB bytes must match the corresponding full-frame pane exactly.'
  }
  elseif ($CrossfeedOnly) {
      $result.scope = 'crossfeed-only'
      $result.stage = 'crossfeed'
      $result.player_counts = @(1, 2, 3, 4)
      $result.palettes = @('blue_orange')
      $result.weapons = @('sprayer', 'sniper', 'roller', 'mister', 'bucket')
      $result.resolutions = @('1280x720')
      $result.actions = @('clean-heavy-approach', 'moving-camera-context', 'home-exits', 'runtime-overview', 'actual-stage-preview', 'frozen-state-rng')
      $result.input_slots = @(3, 1, 2, 0)
      $result.inputs = 'Actual Level/Player fixtures constructed through public builders; controlled initial stations and direct projectile heavy-paint contacts are labeled separately from real scripted movement. Ordinary per-slot PlayerInputData; no physical-input claim.'
      foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
      $result.checks = 'Twenty-one full native720p frames, 51 pane crops/CSV rows and 43 readbacks: clean/heavy 1/2/3/4-player approaches before/after movement, clean/heavy four-player home exits, actual solo runtime overviews and actual StagePreview. Measured heavy ownership exceeds99percent initially. Frozen pixels/public state/RNG and Debug replay fingerprint remain unchanged by drawing. Target visibility is recorded as an observation, not asserted human readability. Route traversal and weapon flight are separately qualified by CPU acceptance; attended matches, balance, physical pads and listening remain open.'
  }
  elseif ($CqBlockoutOnly) {
    $result.scope = 'cq-blockout-only'
    $result.stage = 'close_quarters'
    $result.player_counts = @(1, 4)
    $result.palettes = @('blue_orange')
    $result.weapons = @('sprayer', 'sniper', 'roller', 'mister', 'bucket')
    $result.resolutions = @('1280x720')
    $result.actions = @('main-contest', 'lower-bypass', 'distributed-four-pane', 'runtime-overview', 'actual-stage-preview', 'frozen-state-rng')
    $result.input_slots = @(3, 1, 2, 0)
    $result.inputs = 'Actual builder-owned CQ Levels with players deliberately placed on authored route surfaces for composition inspection; neutral scripted per-slot input and ordinary GamepadReader. Route traversal is checked separately by cq_blockout CPU acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Five bounded native720p frames: solo main and lower routes, four mixed-tool players distributed across the loop and home, actual short-match runtime overview, and actual StagePreview miniature. Full frames and unscaled native crops accompany camera/position CSV receipts. Repeated frozen rendering preserves pixels, public simulation, draw RNG and Debug replay fingerprint. This establishes no attended play, human readability, physical-pad, listening or competitive balance acceptance.'
}
elseif ($MiniatureTerrainOnly) {
    $result.scope = 'miniature-terrain-only'
    $result.Remove('stage')
    $result.player_counts = @(1, 4)
    $result.stages = @('close_quarters', 'king_of_the_hill', 'turbulence', 'movement_course')
    $result.palettes = @('blue_orange')
    $result.weapons = @('roller')
    $result.actions = @('independent-edge-tick-pixels', 'masked-seam', 'lit-thin-ruler', 'ramp-silhouette', 'owned-paint-overlay', 'legacy-comparison', 'native-material-parity', 'native-scene-order-culling', 'opening-preview-push', 'active-pullback-overview', 'resize-return', 'frozen-state-rng')
    $result.input_slots = @(3, 1, 2, 0)
    $result.inputs = 'Direct material/Structure and actual builder-owned Level draws with neutral sparse PlayerInputData. Normal GamepadReader backend; no physical-device claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Independent complete exposed-edge/masked-top-span/body pixel oracle and actual major-ruler tick/light-gap, one-pixel light ruler and original filled ramp silhouette checks. Legacy and adapted material comparisons at 1P/4P720p/1080p/QHD; exact point-sampled material equality at one cell per pixel and larger. Actual projectile-owned paint remains above terrain without changing score or attribution. All four real arena overview compositions compare legacy/adapted rasters; native Scene versus client traversal preserves painter order/culling/pixels. Actual sparse-roster solo and four-player opening/preview/push/active/pullback/overview contexts at all three native sizes preserve frozen public observations, RNG and Debug replay fingerprints, with exact resize returns. Selected native detail/context crops accompany compact comparison CSV; no video or full painter matrix. Muted rendering under possible concurrent Unreal validation does not establish human readability/style, physical-pad, listening or performance acceptance.'
}
elseif ($DriftOnly) {
    $result.scope = 'cloud-drift-only'
    $result.player_counts = @(1, 4)
    $result.stages = @('close_quarters', 'king_of_the_hill', 'turbulence', 'movement_course')
    $result.palettes = @('blue_orange')
    $result.weapons = @('sprayer')
    $result.actions = @('cloud-anchor', 'cloud-right', 'cloud-left', 'cloud-loop', 'scene-edge-culling', 'reduce-effects', 'nonpositive-nonfinite-time', 'frozen-redraw', 'draw-update-rng', 'resize-round-trip')
    $result.input_slots = @(3, 0, 1, 2)
    $result.inputs = 'Real DriftingVisual in Scene with ordinary render fan-out and direct unculled reference; neutral scripted per-slot PlayerInputData for real arena context captures. Normal GamepadReader backend; no physical-device claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Native cloud_big art in real Scene at 1P/4P 720p/1080p/QHD: anchor exactly matches static Visual, both 48-unit extremes visibly differ, period closes exactly, all changed pixels lie inside projected cull bounds. Both edges compare Scene output against direct unculled drawing while the original anchor lies wholly outside the view. Immediate applied Reduce effects restores the anchor and freezes the phase; switching back restores the same displaced raster. Nonpositive/nonfinite time, repeated frozen drawing and draw/update RNG remain unchanged. Exact 720p resize round trip. Actual builder-owned solo contexts from all four arenas and a 4P1080 Close Quarters sky composition with explicitly placed players preserves frozen public simulation and Debug fingerprints. The 4P composition makes no gameplay-route or jump-reachability claim. Muted hardware rendering; human motion/readability, listening and performance acceptance remain separate.'
}
elseif ($IdentityOnly) {
    $result.player_counts = @(2, 4)
    $result.weapons = @('sprayer', 'sniper')
    $result.actions = @('identity-ink-live-consumers', 'identity-background-live-consumers', 'independent-badge-paper-substitution', 'retained-black-glyphs-and-borders', 'unchanged-scene-pixels-outside-badges', 'hidden-and-focus-policy', 'frozen-geometry-state-rng-and-debug-fingerprint')
    foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'monitor_checks', 'countdown_checks', 'closing_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.player_identity_checks
    $result.checks += ' The additional background-colour companion preserves the existing ink proof and uses preconstructed PlayerBadge/UiPlayerBadge roster cells plus the frozen four-view world/HUD badges. Default WHITE, named RED, RGB35BCE1, opaque RGBA2060C0FF and exact restoration require28 additional readbacks,five consumer CSV rows,four separate identity-background crops and zero state updates per preset. Literal baseline WHITE paper is substituted only inside independently computed badge rectangles, with nonzero paper and unchanged BLACK glyph/border pixels in every direct/UI/world/HUD roster. Exact full-frame references preserve all unrelated scene white and every pixel outside those bounds; captured geometry, enabled/hidden/focus policy, public simulation/cosmetic state,RNG and Debug fingerprint remain fixed. Team/Weapon/Results are source-audited users of the shared badge renderer; their pages are not newly opened. No human contrast/readability, physical-device,listening or performance acceptance is claimed.'
    if ($Preset -eq 'x64-debug') {
        $result.actions += @('identity-background-real-tune-reset-save', 'identity-background-atomic-semantic-refusals', 'identity-background-one-token-save-rebase', 'preserved-private-play-and-jump-preferences')
        $result.checks += ' Debug background commands separately require40 additional readbacks,ten attempts(seven accepted,three semantic refusals),eight tune CSV rows and zero additional crops/state updates. Translucent,zero-alpha and current-ink-equal backgrounds must leave live colour/text/ledger/private bytes unchanged. Real named/RGB/opaque-RGBA edits, targeted reset, one-token private save/rebase and saved-baseline reset operate on the same preconstructed consumers; actual content and unrelated private play and Jump preferences remain unchanged. These are the source-enforced background budgets in addition to the preserved ink evidence.'
    }
}
elseif ($QuietOnly) {
    $result.weapons = @('sprayer', 'sniper', 'bucket')
    $result.actions = @()
    foreach ($key in 'contact_checks', 'weapon_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks', 'countdown_checks', 'closing_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.quiet_checks
}
elseif ($MonitorOnly) {
    $result.player_counts = @(1, 2, 4)
    $result.palettes = @('blue_orange')
    $result.weapons = @('roller')
    $result.actions = @()
    foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'countdown_checks', 'closing_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.monitor_checks
}
elseif ($CountdownOnly) {
    $result.player_counts = @(1, 2, 4)
    $result.palettes = @('blue_orange')
    $result.weapons = @('roller')
    $result.actions = @()
    $result.input_slots = @(4, 3, 1, 0)
    $result.inputs = 'Scripted keyboard/mouse seat and sparse pad-slot inputs; no physical-device claim.'
    foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks', 'closing_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.countdown_checks
}
elseif ($ClosingOnly) {
    $result.player_counts = @(1, 2, 4)
    $result.weapons = @('roller')
    $result.actions = @()
    foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks', 'countdown_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.closing_checks
}
elseif ($SupplyOnly) {
    $result.weapons = @('sprayer', 'mister')
    $result.actions = @()
    foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks', 'countdown_checks', 'closing_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.supply_checks
}
elseif ($TeamSetupOnly) {
    $result.player_counts = @(2, 3, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('sprayer', 'sniper', 'roller', 'bucket')
    $result.actions = @('shuffle', 'concurrent-confirm', 'reconfirm', 'weapon-return', 'manual-team-edit', 'held-entry', 'conflicting-actions', 'disconnect', 'focus-return', 'solo-no-op', 'warning-clear', 'pause-restart', 'series-next-round')
    $result.input_slots = @(4, 3, 0, 2)
    $result.inputs = 'Real MainMenuTeamSelect/MainMenuWeaponSelect, StateContext and MenuInput; test-only concrete GamepadReader backend supplies connected states and armed button releases. Public keyboard messages supply R/Enter/Escape. No physical-controller or native-foreground acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Actual 2/3/4-player seeded balanced shuffles, both three-player majority sides, requests from every joined seat, concurrent confirm discarded, all readiness cleared and every player required to reconfirm. Provisional teams match a fresh actual page raster and independently specified complete X/R legend; final confirmation commits just teams. Weapon return preserves teams, tools, sparse slots, stage, duration and best-of; manual edits, held entry, conflicting actions, missing pad, reconnect-held X and focus-return-held R are gated. Shuffle clears the empty-team warning; solo return hides Random teams and ignores R. Native 720p/1080p rows/footer captures and measured four-row/legend bounds, frozen page raster. One committed shuffled 2P roster retains actual Player teams, input slots and weapons through Pause Restart and two best-of-three rounds using ordinary Results/End continuation, with the ongoing-series primary card checked against its expected ledger. Physical pads, human readability and listening acceptance remain separate.'
}
elseif ($StickSettingsOnly) {
    $result.player_counts = @()
    $result.palettes = @('blue_yellow')
    $result.weapons = @()
    $result.actions = @('slot-selection', 'deadzone-clamps', 'apply', 'failed-save', 'back-discard', 'parent-apply')
    $result.input_slots = @(0, 1, 2, 3, 4)
    $result.inputs = 'Public keyboard through production Pad sticks, Controls/audio and StateContext, with a separate isolated save profile. Preview reads the current pad snapshot; no physical controller is required.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Actual Pad sticks child at 720p/1080p/QHD: five bounded focus cards/arrows, full labels/values/status and response/association copy, native linear ink and contrast, initial and pausing slot selection, slot/deadzone clamps and pending cutoff previews, Apply/reload/failure and Back/Escape discard. Suspended parent Apply preserves child profiles. Frozen redraw, draw RNG, static Reduce effects and resize. No physical-controller comfort, human readability or listening acceptance.'
}
elseif ($KeyboardJumpOnly) {
    $result.scope = 'keyboard-jump-only'
    $result.Remove('stage')
    $result.stages = @('movement_course')
    $result.player_counts = @(1)
    $result.palettes = @('blue_yellow')
    $result.weapons = @('sprayer')
    $result.resolutions = @('1280x720', '1920x1080')
    $result.actions = @('actual-controls-child', 'held-entry-neutral-gate', 'single-j-release', 'repeated-frame', 'conflict-chord-overlap-unsupported-navigation', 'escape-and-focus-cancellation', 'pending-active-reset-apply-back', 'independent-parent-audio', 'failed-write-active-choice-and-retry', 'private-save-reload', 'fitted-help-status-action-labels-and-disabled-ink', 'captured-child-copy-and-effective-parent-help', 'fresh-space-menu-proceed', 'paused-countdown-effective-reminder', 'captured-layout-key-reversal', 'frozen-public-state-rng-and-debug-fingerprint')
    $result.input_slots = @(0, 1, 2, 4)
    $result.inputs = 'Public Keyboard.on_key_down/on_key_up and poll through actual Controls/KeyboardJumpMenu/StateContext and a one-player GameLevel Pause route. Scripted GamepadReader seam for owner/fallback checks; private save/debug profile; hidden owned window. No global/foreground input or physical-device claim.'
    $result.audio = 'Private menu save starts muted; pending parent music is applied without a music owner, then muted again before the actual GameLevel fixture. No listening acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Bounded both-preset keyboard Jump UI/save/native-render slice at720/1080. Actual Controls entry and independent child capture/Reset/Apply/Back check held entry and opener neutral gating, single J gesture/release, repeated same-frame reads, chord/overlap/A-conflict/unsupported and navigation ownership, Escape and focus cancellation, pending versus active keys, independent parent pending audio, two controlled failed writes with active preference and byte-identical prior JSON, retry and ten fresh Save reloads. Independently fitted native linear help/status text and four visible action labels plus disabled ink are checked; captured child templates survive replacement, parent effective help refreshes on return, and Space capture requires a separate fresh menu confirmation. Two actual GameLevel Pause/Options/Controls/Jump routes update the countdown reminder via draw/refresh at unchanged resolution, without a covered Level simulation update; key reversal preserves original text/layout despite unrelated live geometry changes. Frozen draw RNG/public state and Debug fingerprint remain checked. Native counters require42 help/status pairs in Release or44 in Debug, ten Save reloads and two opening captures; CSV and compact native crops accompany the result. Debug adds a private bare-J bind warning and removal. A separate actual-menu ownership leaf requires eleven common scripted-pad observations plus eleven Debug observations: owner Back/key drain, other-pad exclusion, disconnected-owner participant and spare fallback, reconnect rearming and fresh Space; Debug checks queued work, grave/F11/chord ownership and private posted F10 down/repeat/up/focus hook messages consumed as WM_NULL without dispatch or global input. Actual OS focus delivery remains outside this probe. Process relaunch, physical devices, exhaustive key domain, human readability, listening and performance are outside this leaf.'
}
elseif ($ControlsMenuOnly) {
    $result.player_counts = @(1)
    $result.palettes = @('blue_yellow')
    $result.weapons = @('sprayer')
    $result.actions = @('row-change', 'numeric-clamps', 'apply', 'failed-save', 'back-discard', 'options-return', 'pause-return', 'custom-labels', 'long-label-fit', 'controls-font-snapshot', 'focus-return', 'custom-help', 'multiline-help-fit', 'help-font-snapshot', 'preview-refresh', 'custom-status', 'status-font-snapshot', 'save-recovery', 'interrupted-apply')
    $result.actions += @('custom-setting-values', 'value-width-height-fit', 'value-font-snapshot', 'value-child-return')
    $result.input_slots = @(4)
    $result.inputs = 'Public keyboard through production Controls/audio, Options, Pause and actual GameLevel/StateContext, with a separate isolated save profile. Preview gain is zero for this raster check; saved music/SFX remain muted.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Controls/audio paper, all nine focus cards and detached arrows; measured full labels/values, bounded complete controller/keyboard/readiness/effects help and save status, native linear text and ink/paper contrast across 720p/1080p/QHD, 4:3, 16:10 and ultrawide. Actual clamps and pending edits, Apply with persisted reload, controlled save failure, Back discard, pending Options values/focus and actual paused GameLevel return. Actual Pad sticks child at 720p/1080p/QHD: five focus cards, full labels/values/status and response/association copy, initial and pausing slot selection, slot/deadzone clamps and pending cutoff previews, Apply/reload/failure and Back/Escape discard. Fourteen independent historical label substitutions across all six sizes/aspects; long OptionRow fitting, full label/value focus-card bounds, a separate Controls item font missing an old glyph, retained/reopened/reversed text/font snapshots, repeated child return and Apply, interrupted Enter requiring a fresh selection, save reload and pending-edit discard. Three independent static-help defaults; authored Controls and association blocks exercise both width and multiline height fitting. Unscaled aim caption uses the Move height probe and the full help column beyond its diagram width. A replacement help font missing an old glyph, retained/reopened/reversed help snapshots, value/preview refresh, old-parent/new-child return, repeated/interrupted Apply and discard pass at 720p. Independent pending/saved/failure status rasters pass across six sizes/aspects on Controls and Pad Sticks. Authored width fitting with compact help, scale reset to short saved copy, retained/reopened/reversed status callbacks using a font missing an old glyph, interrupted/repeated Apply, controlled failure/recovery, isolated reload, both discard routes and old-parent/new-child profile merge pass at 720p. Suspended parent Apply preserves child profiles. Frozen redraw/state, draw RNG, static Reduce effects and resize. No physical-controller, human readability, preview audition or listening acceptance.'
    $result.checks += ' Independent Controls On/Off defaults pass across six sizes/aspects. A Controls item-font fixture exercises width and tall-glyph height fitting, complete value/focus-card unions, refitting between width/height limits, reopened short copy and replacement without an old glyph. Retained/reopened/reversed callbacks, Left/Right boolean assignments, Pad sticks child return/profile merge, interrupted/repeated Apply, controlled failure/recovery, isolated reload and discard pass at 720p.'
}
elseif ($SettingsMenuOnly) {
    $result.player_counts = @()
    $result.palettes = @('blue_orange', 'pink_green', 'blue_yellow', 'random')
    $result.weapons = @()
    $result.actions = @('row-change', 'apply', 'failed-save', 'controls-return', 'discard', 'disabled-resolution', 'credits-back', 'custom-status', 'status-font-snapshot', 'save-recovery', 'interrupted-apply')
    $result.actions += @('custom-setting-values', 'value-width-height-fit', 'value-font-snapshot', 'value-child-return')
    $result.input_slots = @(4)
    $result.inputs = 'Public keyboard through production Options/Credits and StateContext, with a second isolated save profile; fullscreen metadata exercises disabled rows. Only false/Off is applied using an isolated offscreen, windowed fixture, which is immediately hidden again.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Options/Credits paper, all enabled focus cards/arrows, native ink/paper contrast, measured labels, all palette values, long failure status and complete authored credits at 720p/1080p/QHD, 4:3, 16:10 and ultrawide. Actual row edits, staged Apply, persisted reload and failed-write warning, Controls/audio return preserving pending edits/focus, Back discard, disabled fullscreen resolution and retained windowed preset, Credits Select/Back, frozen redraw, draw RNG, static Reduce effects and resize. Independent Options pending/saved/failure rasters pass across six sizes/aspects; authored width fitting and scale reset, missing-old-glyph status font, retained/reopened/reversed callbacks, interrupted/repeated Apply, controlled failure/recovery, isolated reload and both discard routes pass at 720p. No physical-controller, human readability or listening acceptance.'
    $result.checks += ' Independent Options On/Off/Random defaults pass across six sizes/aspects. An Options item-font fixture exercises width and tall-glyph height fitting, complete value/focus-card unions, Random-to-short-fixed-palette scale reset and replacement without an old glyph. Retained/reopened/reversed callbacks, both fullscreen toggle directions and Random palette cycles, one Controls child On/Off pair and return, interrupted/repeated windowed-Off Apply, controlled failure/recovery, isolated reload and discard pass at 720p.'
}
elseif ($PausedOptionsOnly) {
    $result.scope = 'paused-options-only'
    $result.player_counts = @(1, 2, 4)
    $result.palettes = @('blue_yellow', 'pink_green', 'random')
    $result.weapons = @('sprayer')
    $result.actions = @('actual-match-pause-options', 'all-seven-focus-rows', 'caption-font-snapshot', 'own-back-discard', 'palette-next-build', 'sparse-owner', 'joined-fallback', 'keyboard-recovery', 'held-reconnect', 'controls-sticks-merge', 'windowed-display-trial', 'revert', 'escape', 'timeout', 'focus-loss', 'cover', 'keep-failed-write-recovery', 'reduce-effects', 'nonpositive-time', 'developer-unwind', 'frozen-draw-rng')
    $result.input_slots = @(4, 3, 0, 2, 1)
    $result.inputs = 'Actual1/2/4 player GameLevel to Pause row2 Options, scripted sparse pad rosters and local keyboard. Nested isolated profile with nondefault shared preferences and four distinct deadzone profiles. Whole-fixture activation guard; hidden private windowed display changes only.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Independent complete seven-row paused Options paper, caption, values, status and footer at six native sizes/aspects. Retained Level identity, roster/layout/resume clock, full observation and actual Level/HUD raster at a common720 size cover painted match preservation; no direct private Session or timer-field claim. Owner-only input, joined fallback, keyboard recovery and reconnect arming propagate into actual Controls/Pad Sticks and display child. Separate nested saves preserve other preferences; palette-only Apply leaves the live arena unchanged and Restart uses its new palette. Private windowed trial safe Revert/Escape, elapsed timeout, focus/cover rollback, failed Keep preserving old JSON and recovery reload. Back returns retained Pause; only Pause Resume starts count-in. Captured caption/font, lifecycle, reduction and frozen draw RNG. No physical-pad, user-save, hardware-fullscreen, global-input, human or performance acceptance.'
}
elseif ($DisplayTrialOnly) {
    $result.scope = 'display-trial-only'
    $result.player_counts = @()
    $result.palettes = @('blue_yellow', 'random')
    $result.weapons = @()
    $result.actions = @('actual-options-apply', 'default-revert', 'keep', 'escape', 'controlled-timeout', 'wall-clock-timeout', 'focus-loss', 'cover-rollback', 'developer-dismiss-reset', 'clear-rollback', 'held-entry', 'pad-reconnect', 'conflicting-input', 'move-confirm-separation', 'back-priority', 'keep-failed-write', 'recovery', 'metadata-fullscreen-keep-off', 'reduce-effects', 'nonpositive-time', 'frozen-rng')
    $result.input_slots = @(4, 3)
    $result.inputs = 'Actual Options changed-resolution Apply and its pushed trial child; local keyboard and sparse scripted pad slot3. Separate nested profile with nondefault controls and four distinct pad profiles. One nonnested activation guard covers all private-window changes and rollbacks, including destruction. Actual tests stay windowed Off; fullscreen is saved metadata only.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Independent complete trial background, compact card geometry with captured Pause palette, centered Options/question, Keep/Revert card/arrow, countdown and footer at six native sizes/aspects. Explicit finite simulation time leads wall capture time under a configured60-second bound; the shipped15-second timeout is separately sampled after3.25 simulated seconds, and a1-second trial proves zero-dt wall expiry. Real Options Apply first changes only its private window to1920x1080; Save/file remain unchanged until fresh Keep. Default Revert/Escape, simulated exact deadline, wall deadline, focus loss, cover, developer dismiss/reset and clear restore the old configured1280 preset/prior actual windowed mode without preference/file writes. Cover remains present until explicitly popped and delivers one typed result afterward. Keep/reload, repeated unchanged Apply, one controlled failed Keep preserving old JSON and recovery preserve every shared preference and all four profiles. Keyboard/pad conflicts, held reconnect/entry, move-confirm separation, Back priority, immediate reduction and frozen pixels/RNG pass; nonpositive updates preserve nonclock geometry while the wall countdown may advance. Native backbuffer resizing for the six-aspect references does not replace configured rollback targets. No user-save, hardware-fullscreen, global input, physical-pad, human readability, listening or performance acceptance.'
}
elseif ($OptionsDefaultsOnly) {
    $result.scope = 'options-defaults-only'
    $result.player_counts = @()
    $result.palettes = @('blue_yellow', 'random')
    $result.weapons = @()
    $result.actions = @('all-seven-rows', 'staged-restore', 'repeat-restore', 'back-discard', 'escape-discard', 'controls-return', 'controls-save-merge', 'windowed-apply', 'repeat-apply', 'reload', 'failed-write', 'recovery', 'held-confirm-rearm', 'pad-reconnect', 'fullscreen-metadata-off', 'reduce-effects', 'lifecycle-reset', 'frozen-rng')
    $result.input_slots = @(4, 3)
    $result.inputs = 'Actual Options and Controls child in StateContext; public local keyboard messages and sparse scripted pad slot3. Nested isolated save profile with nondefault display/play settings and four distinct pad profiles. Windowed Apply uses the existing narrow CBT activation guard and offscreen hidden probe; fullscreen is metadata only.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Independent complete seven-row Options paper/cards/arrow, fitted labels/values, plain text, pending/saved/failure status and footer at six sizes/aspects. Canonical display/palette defaults remain staged with active/window/file/play/profile state unchanged until Apply; repeated restore, both discard routes and Controls return retain intended values/focus. Hidden windowed Apply/repeated Apply/reload and six controlled temporary-file write refusals/recovery preserve every unrelated play field and all four slot profiles. At720p keyboard deactivation and held pad reconnect require fresh Restore, reduced effects/lifecycle/developer reset remain bounded, applied-fullscreen metadata keeps the original disabled/live-resolution policy until Off Apply, and a Controls-saved preference merges into the pending display Apply. Frozen pixels and draw RNG; no user-save, hardware-fullscreen, physical-pad, human readability, listening or performance acceptance.'
}
elseif ($HomeExitOnly) {
    $result.scope = 'home-exit-only'
    $result.player_counts = @()
    $result.palettes = @()
    $result.weapons = @()
    $result.actions = @('home-exit-push', 'safe-no', 'back-no-yes', 'same-home-resume', 'fresh-reopen', 'held-entry', 'interrupted-enter', 'held-pad-reconnect', 'conflicting-actions', 'simultaneous-devices', 'back-priority', 'reduce-effects', 'nonpositive-time', 'cover-developer-reset', 'pending-music-latch', 'muted-immediate-quit', 'frozen-rng', 'resize')
    $result.input_slots = @(4, 3)
    $result.inputs = 'Actual Home Exit pushed child in StateContext, public local keyboard messages across six sizes/aspects and sparse scripted pad slot3 at720p. Hidden no-activate window; final deliberate Yes closes only this probe window. Intermediate Yes is held at the existing music callback seam. No save writes.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Independent complete Home Exit neutral background, stepped confirmation paper, unscaled centered Exit/question, Yes/No cards/arrow and footer across six native sizes/aspects. Safe No, Back from either answer and repeated fresh reopen resume the exact retained Home Exit frame; source-copy replacement verifies retained parent text. At720p keyboard deactivation and sparse-pad held reconnect reject stale Yes, same-device conflicting actions remain inert, simultaneous confirmations push/pop once, cross-device held entry is disarmed, navigation cannot confirm Yes in the same update and Back has priority. Ordinary motion, immediate reduction, nonpositive time, cover/activation/developer resets, safe developer dismissal, resize and frozen pixels/empty plan/draw RNG pass. Deliberate Yes starts one pending music exit, keeps outgoing pixels fixed and suppresses repeated input; developer dismissal cancels that pending exit. Final no-update/muted Yes closes only the hidden probe window. Scripted input establishes no physical-pad, human readability, listening or performance acceptance.'
}
elseif ($HomeReturnFocusOnly) {
    $result.scope = 'home-return-focus-only'
    $result.player_counts = @()
    $result.palettes = @()
    $result.weapons = @()
    $result.actions = @('initial-play', 'title-entry-play', 'mode-back-play', 'options-back-action', 'options-back-row', 'credits-back-action', 'credits-back-row', 'fresh-confirm-reopens-origin', 'initial-focus-api', 'held-enter-deactivation', 'held-pad-developer-reset', 'cover-return', 'reduce-effects', 'nonpositive-time', 'frozen-rng', 'resize')
    $result.input_slots = @(4, 3)
    $result.inputs = 'Actual Home/Title/Mode/Options/Credits in StateContext; public local keyboard messages across six sizes/aspects and sparse scripted pad slot3 at720p. The existing scripted GamepadReader link seam uses no physical devices. Hidden no-activate window; no save writes.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Complete independent Home background, outer paper, persistent Play card, selected-row card, detached arrow, plain text and footer rasters across six sizes/aspects. Initial Home, Title entry and Mode Back focus Play; Options and Credits action Back and authored Back-row returns restore their origin. Fresh confirmation without navigation reopens the exact real destination; repeated loops and explicit initial-focus API preserve the same rows. At720p keyboard deactivation cancels held Enter and sparse pad developer reset cancels held A, requiring a fresh confirmation. Ordinary cursor motion, nonpositive time, immediate Reduce effects, activation/cover/developer motion resets, static reduced entry/child return, resize and frozen pixels/empty plan/draw RNG pass. No phantom focus or press animation on return; no protected focus accessors, physical-pad, human readability, listening or performance acceptance.'
}
elseif ($StagePaperOnly) {
    $result.scope = 'stage-paper-only'
    $result.player_counts = @(1, 2, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('sprayer', 'sniper', 'roller', 'bucket')
    $result.stage = @('king_of_the_hill', 'turbulence', 'close_quarters', 'movement_course', 'eligible-random')
    $result.actions = @('all-arenas', 'eligible-random', 'both-wraps', 'solo-single-series-roster', 'lock-unlock', 'locked-arrows-refused', 'start-real-random', 'latched-music-exit', 'back-weapon', 'missing-reconnect', 'missing-back-count', 'accent-motion', 'reduce-effects', 'lifecycle-resets', 'captured-art-font-copy', 'live-name-status-ink-fit', 'overflow-navigation', 'preview-separation', 'no-eligible-random', 'resize')
    $result.input_slots = @(4, 3, 0, 2)
    $result.inputs = 'Actual Stage Select and ordinary Weapon/Count destinations in StateContext; local keyboard messages and the existing scripted GamepadReader link seam provide solo and sparse connected seats. The existing music-exit callback holds outgoing pages for exact raster checks. Hidden no-activate window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Independent complete outer paper, dynamically measured briefing paper, unscaled/fitted dark lettering and actual miniature StagePreview raster at its original physical pane/camera. Every shipped arena and eligible Random across six sizes/aspects in solo, two-player single/best-of-three and four-player best-of-five contexts. Both wraps, lock/unlock, refused locked arrows, immediate real/Random start with zero/one exact RNG draw, latched outgoing Random during pending music exit, one state pop, ordinary Weapon Back, missing/reconnect/held rearming and Count rescue. Native720p/1080p/QHD accent initial/middle/rest, rapid final lock/start, nonpositive time, immediate Reduce effects, activation/cover/developer resets, interrupted confirmation, fixed text/preview and resize. Captured art/font/copy survives source replacement, refresh/reopen/reversal while live name/status ink and name/hint fit remain explicit. Empty eligible set omits Random, a single excluded stage is stable, and resource-valid navigation overflow/text-padded preview overlap fail precise runtime geometry guards. Frozen pixels/plan/RNG; scripted reader only, no physical-pad, human readability, listening or performance acceptance.'
}
elseif ($WeaponPaperOnly) {
    $result.scope = 'weapon-paper-only'
    $result.player_counts = @(1, 2, 3, 4)
    $result.palettes = @('random-neutral', 'blue_orange', 'pink_green', 'blue_yellow')
    $result.weapons = @('sprayer', 'sniper', 'roller', 'mister', 'bucket', 'random')
    $result.actions = @('all-choices', 'both-wraps', 'ready-unready', 'confirmed-refusal', 'all-random-commit', 'ordered-rng', 'missing-reconnect', 'back-mode-team', 'commit-stage', 'stage-back', 'missing-back-count', 'choice-motion', 'reduce-effects', 'lifecycle-resets', 'captured-art-font-catalog', 'live-readiness-fill', 'overflow-name-description', 'resize')
    $result.input_slots = @(4, 3, 0, 2)
    $result.inputs = 'Actual Weapon Select and ordinary Mode/Team/Stage/Count destinations in StateContext; local keyboard messages and the existing scripted GamepadReader link seam provide sparse connected seats. Palette changes write only the isolated probe profile. Hidden no-activate window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Independent complete outer paper, individual tile paper, unscaled heading/player/name/description lettering, P1-P4 badge cells and fixed tool/paint/final-supply layers across six native sizes/aspects, one-to-four sparse seats and every choice. Right/left wrap, provisional browsing, readiness/unreadiness, refused confirmed input, exact ordered per-seat Random draws, immediate committed Stage and Back Mode/Team/Stage-return routes; missing-player footer, held reconnect and Count rescue. All fixed palettes and neutral Random preview preserve draw RNG. Native720p/1080p/QHD simultaneous/rapid tile contraction, initial/middle/rest, fixed inner art/masks/text/badges, nonpositive time, immediate Reduce effects, activation/cover/developer resets and resize. Captured art/font/catalog/description/palette snapshots survive refresh/reopen/reversal, including sparse custom catalog fill; icon readiness fill remains live while readable name inks stay captured. Actual unscaled name and last-row description overflow are rejected by precise runtime diagnostics. Frozen pixels/plan/RNG; scripted reader only, no physical-pad, human readability, listening or performance acceptance.'
}
elseif ($TeamPaperOnly) {
    $result.scope = 'team-paper-only'
    $result.player_counts = @(1, 2, 3, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('sprayer', 'sniper', 'roller', 'bucket')
    $result.actions = @('initial-roster', 'unassigned', 'team-a-b', 'ready-unready', 'both-teams-warning', 'warning-clear', 'seeded-shuffle', 'missing-reconnect', 'back-count', 'commit-weapon', 'choice-motion', 'reduce-effects', 'lifecycle-resets', 'captured-art-font-atlas', 'overflow-warning', 'resize')
    $result.input_slots = @(4, 3, 0, 2)
    $result.inputs = 'Actual Team Select and ordinary Count/Weapon destinations in StateContext; local keyboard messages and the existing scripted GamepadReader link seam provide sparse connected seats. Hidden no-activate window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Independent full paper/static row-card/text/warning/icon/badge rasters for solo and two/three/four sparse-player rosters at720p/1080p/QHD,4:3,16:10 and ultrawide; unchanged unscaled font anchors, logical shadow unions, native contrast, all three choices, readiness/unreadiness and refused no-team/same-team completion, warning recovery, seeded balanced shuffle with exact RNG boundary, ordinary committed Weapon and Back Count routes. Missing-player footer and held reconnect rearming preserve provisional settings. Native720p/1080p/QHD simultaneous choice contraction, middle/rest, readiness press, local fixed paper/text/badge pixels, immediate Reduce effects, nonpositive time, activation/cover/developer resets and resize. Captured paper art, fonts and atlas/frame groups survive source replacement, choice/shuffle refresh, reopen and reversal while existing icon tint policy remains live. Actual warning overflow is rejected by its specific runtime guard. Frozen pixels/plan/RNG; scripted reader only, no physical-pad, human readability, listening or performance acceptance.'
}
elseif ($JoinPaperOnly) {
    $result.scope = 'join-paper-only'
    $result.player_counts = @(1, 2, 3, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('sprayer')
    $result.actions = @('reserved-p1', 'join-claims', 'complete-roster', 'reserved-reconnect', 'reconnected', 'solo-weapon-route', 'multiplayer-team-route', 'p1-back', 'missing-p1-rescue', 'claim-acknowledgement', 'reduce-effects', 'lifecycle-resets', 'captured-copy-font-art', 'invalid-status-font', 'resize')
    $result.input_slots = @(4, 3, 0, 2)
    $result.inputs = 'Actual Join Players and ordinary Count/Team/Weapon destinations in StateContext. Local keyboard messages and the existing scripted GamepadReader link seam supply keyboard or pad P1 and sparse claims/reconnects. Hidden no-activate window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Independent complete paper, unscaled transparent-shadow heading/roster/status and dark acknowledgement-gutter rasters. One-to-four-player rosters with keyboard or pad P1 cover neutral reserved P1, all joined, missing reserved pads and reconnects at720p/1080p/QHD,4:3,16:10 and ultrawide; native paper containment, ordering, lettering contrast, frozen plan/pixels/RNG and immediate ordinary continuation. Native720p/1080p/QHD gutter initial/middle/rest, nonpositive time, applied Reduce effects, activation/cover/developer resets and resize. Heading/item/detail fonts and captured paper inks survive source replacement, refresh/reconnect, reopen and reversal. Fresh destination rasters verify owner Back, missing-P1 rescue and solo/multiplayer continuation; interrupted owner confirmation requires fresh input. An actual Join page rejects overflowing status-font bounds with its specific diagnostic. Scripted reader only; no physical-pad, human readability, listening or performance acceptance.'
}
elseif ($SetupPaperOnly) {
    $result.scope = 'setup-paper-only'
    $result.player_counts = @(1, 2, 3, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @()
    $result.actions = @('all-mode-focus', 'all-count-focus', 'disabled-solo', 'duration-cycle', 'mode-choice', 'count-join', 'back-routes', 'custom-copy-font-snapshots', 'captured-effective-art', 'invalid-duration-union', 'focus-motion', 'duration-press', 'rapid-navigation', 'reduce-effects', 'deactivation', 'covering-page-return', 'developer-reset', 'resize')
    $result.input_slots = @(4)
    $result.inputs = 'Local keyboard messages through actual Mode Select, Player Count, Join and Home transitions in StateContext; normal GamepadReader backend, hidden no-activate window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Independent complete paper/card/arrow and transparent-shadow text rasters for every Mode/Count focus and disabled-solo series at 720p/1080p/QHD, 4:3, 16:10 and ultrawide, with measured card/text containment and native ink/paper contrast. All duration cycles, three modes, one-to-four-player count/layout/keyboard claims and ordinary Back/Join/Home routes retain behavior; authored text/font snapshots survive replacement, reopen, reversal, duration and child return. Effective paper art, compact focus padding and shadow remain captured through source replacement, focus callbacks, reopening and reversal. An actual Mode page rejects an overflowing compound duration value with its specific diagnostic. Native Mode/Count arrow initial/middle/rest and duration paper press/release, local envelopes, fixed opaque label/value pixels, rapid navigation, nonpositive time, immediate Reduce effects, activation/cover/developer reset, frozen redraw/RNG and resize round-trip. No physical-pad, human readability, listening or performance acceptance.'
}
elseif ($JoinStageMotionOnly) {
    $result.scope = 'join-stage-motion-only'
    $result.stage = @('movement_course', 'close_quarters', 'random')
    $result.player_counts = @(4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('bucket', 'sprayer')
    $result.actions = @('join', 'release', 'stable-hole', 'simultaneous-claims', 'same-update-replacement', 'rapid-row-restart', 'duplicate-full-refusal', 'final-claim-barrier', 'missing-p1-rescue', 'browse', 'real-random-wrap', 'lock', 'unlock', 'locked-arrow-refusal', 'final-random-start', 'rapid-final-start', 'pending-fade', 'single-excluded-stage', 'reduce-effects', 'interruption-resets', 'held-reconnect', 'resize')
    $result.input_slots = @(4, 3, 0, 2, 1)
    $result.inputs = 'Real Join Players and Arena Stage Select pages in StateContext. Local keyboard messages and the existing scripted GamepadReader link seam provide sparse seats, claims, disconnects and held reconnects. Hidden no-activate window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Native initial/middle/rest Join gutter acknowledgements and Stage briefing stripe at 720p/1080p/QHD; each changed pixel stays inside independent row-gutter or briefing-padding envelopes, preserving all text, previews, camera, edges and other rows. Stable release holes, simultaneous claims, same-update release/replacement, rapid row restart, duplicate/full refusal, final-claim snapshot barrier and independent missing-P1 Back destination; real/Random wrapping, lock/unlock, refused locked arrows, no eligible Random and deliberate settled/rapid final eligible Random commits with exactly one RNG choice, settled outgoing Random raster and gated pending fade through focus interruption and repeated input. Immediate applied Reduce effects, nonpositive time, activation/covering-page/developer-input resets, missing/reconnect rearming, frozen redraw/plan/RNG and resize round-trip. Scripted reader only; no physical-controller, human readability, listening or performance acceptance.'
}
elseif ($SetupMotionOnly) {
    $result.scope = 'setup-motion-only'
    $result.player_counts = @(4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('sprayer', 'sniper', 'roller', 'mister', 'bucket', 'random')
    $result.actions = @('changed-focus', 'duration-press', 'independent-team-choice', 'independent-weapon-choice', 'ready', 'unready', 'shuffle', 'rejected-input', 'rapid-navigation', 'reduce-effects', 'deactivation', 'missing-reconnect', 'covering-page-return', 'resize')
    $result.input_slots = @(4, 3, 0, 2)
    $result.inputs = 'Real Mode Select, Player Count, Team Select, Weapon Select and StateContext. Public local keyboard messages and the existing scripted GamepadReader link seam supply sparse connected seats and armed button releases. Hidden no-activate window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Moving/middle/rest native setup ornaments at 720p/1080p/QHD with independent per-row pixel envelopes and fixed weapon artwork, masks, names, descriptions and player badges. Four-seat setup pages, initial settled pages, simultaneous and rapid choices, readiness/unreadiness, shuffle, refused confirmation, endpoint and confirmed-row input, immediate Count-to-2P Join and final Team/Weapon transitions, applied Reduce effects suppression, activation, missing/reconnect and test-pushed covering-page return, frozen redraw/RNG and resize. No physical-controller, human readability, listening or performance acceptance.'
}
elseif ($MenuMotionOnly) {
    $result.player_counts = @()
    $result.palettes = @()
    $result.weapons = @()
    $result.actions = @('changed-focus', 'adjust-value', 'apply', 'rapid-navigation', 'reduce-effects', 'deactivation', 'child-return', 'resize')
    $result.input_slots = @(4)
    $result.inputs = 'Public local keyboard messages through actual MainMenuHome, ControlsSettingsMenu, StickSettingsMenu and StateContext; normal GamepadReader backend. Hidden no-activate window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Initial focus stays still. Actual changed-focus arrows and accepted OptionRow paper presses have distinct initial/middle/rest native rasters at 720p/1080p/QHD; successful Apply has distinct pressed/rest rasters; all changed pixels stay inside independent ornament envelopes and label/value lettering remains fixed. Nonpositive time, rapid focus, immediate applied reduced-effects drawing, real Controls Apply/reload/reversal, deactivation, nested child return, frozen redraw/RNG and resize round-trip. No physical-pad, human readability, listening or performance acceptance.'
}
elseif ($MenuTextOnly) {
    $result.player_counts = @(1, 2, 3, 4)
    $result.palettes = @()
    $result.weapons = @()
    $result.actions = @('named-prompt-rebuild', 'arena-browse', 'arena-lock', 'arena-unlock', 'random-stage-copy', 'random-stage-width-fit', 'random-stage-eligible-start', 'home-options-credits-items', 'fitted-options-labels', 'mode-count-items', 'duration-cycle', 'mode-choice', 'count-join-back', 'solo-disabled', 'focus-round-trip', 'credits-select-home', 'reopen', 'presentation-replacement', 'title-home-fixed-copy', 'title-start-card', 'interrupted-title-home-enter')
    $result.input_slots = @(4, 0)
    $result.inputs = 'Public keyboard messages through actual Title, Home, Stage Select, Options, Credits, Mode Select, Player Count and Join Players plus a concrete MenuPage fixture. This focused binary uses the existing scripted GamepadReader link seam; pads remain neutral until the briefing fixture supplies one connected second seat. No physical-controller or native-foreground acceptance.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Independent shipped brand, all 11 static headings and all 17 formatted legends compared through real MenuPage construction/prompt rebuild against native text/chrome fixtures at 720p/1080p/QHD. Ten legacy static item roles compared on actual Home/Options/Credits at those sizes; long Options labels fit before their value columns, focus round trips and Credits Select retain routing. Eight Mode/Player Count labels and the solo hint match independent complete page rasters across six sizes/aspects, including duration-column placement and the existing item-shadow hint. Actual duration cycles/wrap/confirm, all three mode choices, disabled solo series, all 1-4-player count/layout/keyboard claims and Join/Mode/Home Back routes. Reachable Random-stage display uses an eligible real arena alongside excluded Movement Course; independent full text/shadow crops at six sizes, real/Random cycling without RNG or plan commits, long width-only fit/short reset, retained/reopened/reversed copy/item font and live width/negative-shadow fitting on refresh. Lock/unlock, interrupted Enter and deliberate two-release Random start preserve eligibility and the pending music-exit plan. Title wordmark/subtitle/Start and Home subtitle retain independent linear rasters and complete Start-card bounds across source assignments, reopen and reversal with separate title/item/detail fonts missing an old glyph. Repeated Title/Home/Back and Mode return, Home focus movement and interrupted Enter rearming are checked at 720p; the connection hint remains hidden under the existing always-connected keyboard policy, with its full independently measured absence crop. Custom brand/heading/prompt/item/hint/font/layout, old-page snapshots, repeated prompt rebuilds, duration edits, focus and child return, reopen adoption, frozen redraw and draw RNG. Native crops and CSV retained. No visible controller-loss, physical-controller, human readability, performance or listening acceptance.'
    $result.actions += @('arena-status-copy', 'arena-status-captured-geometry')
    $result.checks += ' Both fixed arena status strings have independent complete text/shadow crops on real/Random browse/locked pages across six sizes. Unscaled detail-font status stays inside the full briefing before its hint row. Forced cycling/lock/unlock after source replacement, reopen and reversal retain captured text/font/position/shadow offset/shadow colour while refresh uses live ink; short custom copy was measured in the shipped font first. Provisional plan and RNG remain unchanged.'
    $result.actions += @('fixed-stage-briefing', 'connected-scripted-versus-briefing', 'briefing-captured-ink')
    $result.checks += ' Four fixed stage-briefing strings and independently specified adjacent mode/roster have complete text/shadow crops at six sizes for solo and connected scripted 2P single/best-of-three/best-of-five. Unscaled brand-font copy fits the design and briefing vertical span before authored following rows; the full solo objective overhang is retained. Captured copy/brand font/position/shadow offset/shadow colour/ink survive source replacement/reopen/reversal and cycling/lock/unlock, with unchanged provisional plan and RNG. The normal-reader full matrix runs the solo briefing subset and explicitly reports the scripted 2P contexts skipped; ResumeOnly retains the real backend.'
    $result.actions += @('fixed-random-navigation', 'fixed-stage-hints', 'hint-width-offset-refresh')
    $result.checks += ' Fixed Random navigation and browse/locked hints have independent full glyph/shadow crops across real/Random browse/locked states at six sizes, alongside unchanged numbered arenas and the eligible Random-count branch. Navigation remains unscaled with constructor-owned detail font/position/ink/shadow. Hints retain captured copy/brand font/position/ink/shadow colour while width budget and signed detail-shadow offset refresh on slot/state changes; long hints, short reset, separate old-glyph-missing fonts, cached fit, reopen/reversal and locked-arrow refusal are covered without changing the plan or RNG.'
    $result.actions += @('fixed-stage-mode-labels', 'stage-mode-detail-font-snapshot')
    $result.checks += ' The two fixed mode labels use independent fixture words for solo/single-match cases, while BEST OF 3/5 remain independent composed-text literals. Native mode crops remain unscaled in the actual detail font before roster row two across six sizes. Custom old-glyph copy, a separate detail font missing that glyph, constructor-owned position/shadow/ink and source replacement/reopen/reversal through cycling/lock/unlock preserve the connected roster, plan and RNG.'
}
elseif ($WeaponCatalogOnly) {
    $result.player_counts = @(1)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('sprayer', 'sniper', 'roller', 'mister', 'bucket', 'random')
    $result.actions = @('browse', 'wrap-left', 'wrap-right', 'confirm-release', 'stage-back', 'mode-back', 'reopen')
    $result.input_slots = @(4)
    $result.inputs = 'Public keyboard input through actual Weapon/Stage/Mode Select pages on the normal GamepadReader backend.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Six historical default names and icon/body/paint/full-supply bindings compared with independent fixtures at 720p/1080p/QHD, both wraps, provisional settings, fresh release confirmation, seeded concrete Random primary, actual Stage/Mode Back preserving all roster and match settings. Edited name/body/paint/supply and a sparse alternate atlas/fill become visible on reopen; existing pages keep their snapshots across edits, reversals and cycling. Frozen raster and draw RNG, native detail/context images and CSV retained. No physical-controller, human readability, performance or listening acceptance.'
}
elseif ($MainMenuOnly) {
    $result.player_counts = @()
    $result.palettes = @()
    $result.weapons = @()
    $result.actions = @('title-start', 'home-play', 'home-options', 'home-credits', 'home-exit', 'back')
    $result.input_slots = @(4)
    $result.inputs = 'Public keyboard input through actual MainMenuTitle/MainMenuHome and StateContext; Exit closes only this probe hidden window.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Title/home paper, measured centered title text, heading/subtitle/build bounds, native ink/paper contrast and all four Home focus cards/arrows at 720p/1080p/QHD, 4:3, 16:10 and ultrawide. Independent historical multiline wordmark, Title subtitle/Start and Home subtitle rasters plus complete Start-card bounds use native linear sampling at all six sizes. The connection hint retains the existing always-connected keyboard hidden policy; its complete independently measured extent matches paper, without a visible controller-loss claim. Actual keyboard Start/Back/Play/Options/Credits/Exit, frozen redraw, draw RNG, static Reduce effects and resize round-trip. No physical-controller, human readability or listening acceptance.'
}
elseif ($ResultsMenuOnly) {
    $result.player_counts = @(1, 2, 4)
    $result.palettes = @('blue_orange', 'pink_green', 'blue_yellow')
    $result.weapons = @()
    $result.actions = @('fill', 'revealed', 'solo', 'draw', 'series', 'contributions', 'continue')
    $result.input_slots = @(4, 3, 0, 2)
    $result.inputs = 'Public keyboard input through the actual ResultsMenuInitial and StateContext; sparse pad slots are roster fixtures.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Results paper, centered lettering and score cards at native 720p/1080p/QHD and differing aspects; competitive/solo score bars and percentage rendering, absent teams, sparse-slot contributions, revealed and fill timing gates, draws and series. Frozen redraw, static Reduce effects, draw RNG, resize and deliberate continuation. No human readability or listening acceptance.'
}
elseif ($PauseMenuOnly) {
    $result.player_counts = @(1, 2, 4)
    $result.palettes = @()
    $result.weapons = @()
    $result.actions = @('resume', 'restart', 'controls-audio', 'quit', 'confirm-yes', 'confirm-no', 'missing-controllers', 'custom-labels', 'custom-questions', 'font-snapshot', 'focus-return', 'missing-seat-gate-clear', 'restart-missing-seat-cancel')
    $result.input_slots = @(4, 0, 1, 2, 3)
    $result.inputs = 'Public keyboard input through actual pause/confirmation pages and StateContext; absent pad slots are roster fixtures, with no physical pad claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'Pause and confirmation paper plus six independent legacy label and two independent legacy question rasters at native 720p/1080p/QHD, 4:3, 16:10 and ultrawide; actual initial Resume, safe No, deliberate Restart/Quit and Controls/audio return. Questions use the unscaled actual detail font and full-paper centering; their complete measured bounds are compared over the stepped paper with production point filtering. Custom labels/questions and replacement item/detail fonts missing an old glyph; retained/reopened/reversed snapshots through focus movement, Controls return, No/Back and repeated typed actions. Focus interruption and synthetic missing-seat Restart cancellation/gate clearing require fresh input, alongside missing-participant refusal, enabled navigation, all-absent keyboard rescue, frozen redraw/RNG, Reduce effects and resize. No physical reconnection, human readability or listening acceptance.'
}
elseif ($EndMenuOnly) {
    $result.player_counts = @()
    $result.palettes = @()
    $result.weapons = @()
    $result.actions = @('rematch', 'change-teams', 'change-weapons', 'change-stage', 'exit', 'next-round', 'replay-draw', 'play-series-again', 'custom-labels', 'custom-headings', 'font-snapshot', 'pending-music-exit', 'focus-return')
    $result.input_slots = @(4)
    $result.inputs = 'Public keyboard input through the actual EndMenuInitial and StateContext; no physical pad claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.Remove('stage')
    $result.checks = 'End-menu paper, primary continuation, four independent legacy action labels and three independent legacy headings at native 720p/1080p/QHD, 4:3, 16:10 and ultrawide; single match, tied/continuing/completed series, both Exit rows and hidden setup choices. Complete measured heading bounds use the unscaled actual heading font, full-paper centering and a stepped-paper reference with production point filtering. Custom labels/headings and replacement item/heading fonts missing an old glyph; retained/reopened/reversed snapshots through focus movement and repeated typed actions preserve the Session ledger. Owner-controlled pending music exits reject further input and preserve their original action through focus interruption; interrupted Enter releases require fresh selection. Frozen redraw/RNG, Reduce effects and resize remain covered. No physical controllers, human readability or listening acceptance.'
}
elseif ($ResumeOnly) {
    $result.player_counts = @(1, 2, 4)
    $result.palettes = @('runtime-save-default')
    $result.weapons = @('sprayer')
    $result.actions = @('pause', 'resume', 'jump', 'fire', 'drop', 'death', 'focus', 'restart', 'quit')
    $result.input_slots = @(4, -1)
    $result.inputs = 'Public keyboard/mouse message input through actual GameLevel, StateContext and pause pages; other seats are idle dummies, with idle bots in Debug. No physical pad claim.'
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.checks = 'Actual 1P/2P/4P pause-page Resume freezes the world through count-in, nonpositive time, re-pause, focus loss/return and a long GO-crossing update. Public player health/ammo/cooldown/dead respawn, live projectile count, detailed cosmetic observation and underlying world/HUD raster remain fixed. Held jump/fire/drop remain suppressed until neutral and a deliberate new request; fresh pause interrupts the count. Actual keyboard-driven Restart clears the gate and Quit leaves immediately. Shared RESUMING paper is bounded and frozen at native 720p/1080p/QHD with exact resize round-trip. Muted hardware rendering; no human readability or listening acceptance.'
}
elseif ($HudOnly) {
    $result.player_counts = @(1, 2, 3, 4)
    $result.weapons = @('sprayer', 'sniper', 'mister', 'bucket')
    $result.actions = @()
    $result.input_slots = @(4, 3, 1, 0)
    $result.inputs = 'Direct HUD quantities and sparse keyboard/pad-slot labels; one real four-player builder-owned frozen match. No physical-device claim.'
    foreach ($key in 'contact_checks', 'weapon_checks', 'quiet_checks', 'aim_marker_checks', 'blocked_shot_checks', 'player_identity_checks', 'monitor_checks', 'countdown_checks', 'closing_checks', 'supply_checks') {
        $result.Remove($key)
    }
    $result.checks = $result.hud_checks
}
if (-not $HudOnly -and ($IdentityOnly -or $QuietOnly -or $MonitorOnly -or $CountdownOnly -or $ClosingOnly -or $SupplyOnly)) {
    $result.Remove('hud_checks')
}
if (-not $SupplyOnly -and ($IdentityOnly -or $QuietOnly -or $MonitorOnly -or $CountdownOnly -or $ClosingOnly -or $HudOnly)) {
    $result.Remove('supply_checks')
}
if ($result.Contains('countdown_checks')) {
    $result.Remove('stage')
    $result.stages = @('movement_course', 'close_quarters', 'king_of_the_hill', 'turbulence')
    $result.player_counts = @(1, 2, 3, 4)
}
