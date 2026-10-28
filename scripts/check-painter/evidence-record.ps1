# Dot-sourced into the scope of scripts/check-painter.ps1.
# The result.json evidence record: its base shape, every focused scope's
# selection and post-chain fixups. Topic bodies stay in dot-sourced helpers.
# Runs before the native-run try, so an override error still skips result.json.

$worktreeStatus = $null
if (-not $OmitWorktreeStatus) { $worktreeStatus = @(& git status --short) }

$result = [ordered]@{
    outcome = 'failed'
    scope = if ($IdentityOnly) { 'player-identity-only' } elseif ($QuietOnly) { 'quiet-only' } elseif ($MonitorOnly) { 'monitor-only' } elseif ($CountdownOnly) { 'countdown-only' } elseif ($ClosingOnly) { 'closing-only' } elseif ($SupplyOnly) { 'supply-only' } elseif ($HudOnly) { 'gameplay-hud-only' } elseif ($ResumeOnly) { 'resume-only' } elseif ($EndMenuOnly) { 'end-menu-only' } elseif ($PauseMenuOnly) { 'pause-menu-only' } elseif ($ResultsMenuOnly) { 'results-menu-only' } elseif ($MainMenuOnly) { 'main-menu-only' } elseif ($WeaponCatalogOnly) { 'weapon-catalog-only' } elseif ($MenuTextOnly) { 'menu-text-only' } elseif ($MenuMotionOnly) { 'menu-motion-only' } elseif ($SettingsMenuOnly) { 'settings-menu-only' } elseif ($ControlsMenuOnly) { 'controls-menu-only' } elseif ($StickSettingsOnly) { 'stick-settings-only' } elseif ($TeamSetupOnly) { 'team-setup-only' } else { 'full-painter-matrix' }
    preset = $Preset
    game_revision = (& git rev-parse HEAD)
    engine_revision = (& git -C external/labrador rev-parse HEAD)
    worktree_status = $worktreeStatus
    worktree_status_omitted = [bool]$OmitWorktreeStatus
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
if ($TurbulenceSlabOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/arena.ps1') 'TurbulenceSlabOnly'
}
elseif ($TurbulenceShoulderOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/arena.ps1') 'TurbulenceShoulderOnly'
}
elseif ($TurbulenceMoverOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/arena.ps1') 'TurbulenceMoverOnly'
}
elseif ($TurbulenceBlockoutOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/arena.ps1') 'TurbulenceBlockoutOnly'
}
elseif ($PressTowerOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/arena.ps1') 'PressTowerOnly'
}
elseif ($FerryDirectionOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/arena.ps1') 'FerryDirectionOnly'
}
elseif ($GatePermissionOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/arena.ps1') 'GatePermissionOnly'
}
elseif ($SurfaceFinishOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/arena.ps1') 'SurfaceFinishOnly'
}
elseif ($ResultPercentageOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'ResultPercentageOnly'
}
elseif ($ContributionTeamOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'ContributionTeamOnly'
}
elseif ($CompetitiveWinnerOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'CompetitiveWinnerOnly'
}
elseif ($RoundHeadingOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'RoundHeadingOnly'
}
elseif ($SeriesScoreOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'SeriesScoreOnly'
}
elseif ($SessionResultOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'SessionResultOnly'
}
elseif ($ResultsTextOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'ResultsTextOnly'
}
elseif ($EndContinuationOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'EndContinuationOnly'
}
elseif ($ResolutionTextOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'ResolutionTextOnly'
}
elseif ($DurationTextOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'DurationTextOnly'
}
elseif ($ExposedColourAlphaOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'ExposedColourAlphaOnly'
}
elseif ($LimbTuneColoursOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'LimbTuneColoursOnly'
}
elseif ($CrackTuneColourOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'CrackTuneColourOnly'
}
elseif ($HudPaperTuneColoursOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'HudPaperTuneColoursOnly'
}
elseif ($HudClosingShadeOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'HudClosingShadeOnly'
}
elseif ($HudClosingMaterialsOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'HudClosingMaterialsOnly'
}
elseif ($StagePreviewBackgroundTuneColourOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'StagePreviewBackgroundTuneColourOnly'
}
elseif ($HudLabelTuneColoursOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'HudLabelTuneColoursOnly'
}
elseif ($HudTimerTuneColourOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'HudTimerTuneColourOnly'
}
elseif ($HudTrackTuneColourOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'HudTrackTuneColourOnly'
}
elseif ($HudHealthTuneColourOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'HudHealthTuneColourOnly'
}
elseif ($ScribbleTuneColourOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'ScribbleTuneColourOnly'
}
elseif ($SilhouetteBodyTuneColourOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'SilhouetteBodyTuneColourOnly'
}
elseif ($GlassTuneColourOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'GlassTuneColourOnly'
}
elseif ($ActingTuneColoursOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'ActingTuneColoursOnly'
}
elseif ($ProgramLevelVisibilityOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'ProgramLevelVisibilityOnly'
}
elseif ($ProgramTuneColourOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'ProgramTuneColourOnly'
}
elseif ($PowerLightTuneColourOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'PowerLightTuneColourOnly'
}
elseif ($DesktopTuneColourOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'DesktopTuneColourOnly'
}
elseif ($CursorMaterialTuneColoursOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'CursorMaterialTuneColoursOnly'
}
elseif ($RevealTuneColoursOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'RevealTuneColoursOnly'
}
elseif ($SilhouetteTuneColoursOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'SilhouetteTuneColoursOnly'
}
elseif ($CursorTuneColoursOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'CursorTuneColoursOnly'
}
elseif ($RespawnPenTuneColoursOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'RespawnPenTuneColoursOnly'
}
elseif ($IdleDripTuneColoursOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'IdleDripTuneColoursOnly'
}
elseif ($BlockedShotTuneColoursOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'BlockedShotTuneColoursOnly'
}
elseif ($CombatTuneColoursOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'CombatTuneColoursOnly'
}
elseif ($FeedbackTuneColoursOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'FeedbackTuneColoursOnly'
}
elseif ($PercentageTextOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'PercentageTextOnly'
}
elseif ($StageScanOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'StageScanOnly'
}
elseif ($StageCountsOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'StageCountsOnly'
}
elseif ($StageSummaryOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'StageSummaryOnly'
}
elseif ($MenuLabelsOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'MenuLabelsOnly'
}
elseif ($WeaponTuneColoursOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'WeaponTuneColoursOnly'
}
elseif ($TeamTuneColoursOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'TeamTuneColoursOnly'
}
elseif ($TeamFocusOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'TeamFocusOnly'
}
elseif ($StickReadoutsOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'StickReadoutsOnly'
}
elseif ($ConnectionTextOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'ConnectionTextOnly'
}
elseif ($BotPlayersOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'BotPlayersOnly'
}
elseif ($RememberedSetupOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'RememberedSetupOnly'
}
elseif ($TuneColoursOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/colour.ps1') 'TuneColoursOnly'
}
elseif ($ParallaxOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/arena.ps1') 'ParallaxOnly'
}
elseif ($LandmarkOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/arena.ps1') 'LandmarkOnly'
}
  elseif ($RouteWarningOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/arena.ps1') 'RouteWarningOnly'
}
  elseif ($CrossfeedOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/arena.ps1') 'CrossfeedOnly'
}
  elseif ($CqBlockoutOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/arena.ps1') 'CqBlockoutOnly'
}
elseif ($MiniatureTerrainOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/arena.ps1') 'MiniatureTerrainOnly'
}
elseif ($DriftOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/arena.ps1') 'DriftOnly'
}
elseif ($IdentityOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/action-hud.ps1') 'IdentityOnly'
}
elseif ($QuietOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/action-hud.ps1') 'QuietOnly'
}
elseif ($MonitorOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/action-hud.ps1') 'MonitorOnly'
}
elseif ($CountdownOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/action-hud.ps1') 'CountdownOnly'
}
elseif ($ClosingOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/action-hud.ps1') 'ClosingOnly'
}
elseif ($SupplyOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/action-hud.ps1') 'SupplyOnly'
}
elseif ($TeamSetupOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'TeamSetupOnly'
}
elseif ($StickSettingsOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'StickSettingsOnly'
}
elseif ($RumbleOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'RumbleOnly'
}
elseif ($KeyboardJumpOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'KeyboardJumpOnly'
}
elseif ($ControlsMenuOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'ControlsMenuOnly'
}
elseif ($SettingsMenuOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'SettingsMenuOnly'
}
elseif ($PausedOptionsOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'PausedOptionsOnly'
}
elseif ($DisplayTrialOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'DisplayTrialOnly'
}
elseif ($OptionsDefaultsOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'OptionsDefaultsOnly'
}
elseif ($HomeExitOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'HomeExitOnly'
}
elseif ($HomeReturnFocusOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'HomeReturnFocusOnly'
}
elseif ($StagePaperOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'StagePaperOnly'
}
elseif ($WeaponPaperOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'WeaponPaperOnly'
}
elseif ($TeamPaperOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'TeamPaperOnly'
}
elseif ($JoinPaperOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'JoinPaperOnly'
}
elseif ($SetupPaperOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'SetupPaperOnly'
}
elseif ($JoinStageMotionOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'JoinStageMotionOnly'
}
elseif ($SetupMotionOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'SetupMotionOnly'
}
elseif ($MenuMotionOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'MenuMotionOnly'
}
elseif ($MenuTextOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'MenuTextOnly'
}
elseif ($WeaponCatalogOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'WeaponCatalogOnly'
}
elseif ($MainMenuOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'MainMenuOnly'
}
elseif ($ResultsMenuOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'ResultsMenuOnly'
}
elseif ($PauseMenuOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'PauseMenuOnly'
}
elseif ($EndMenuOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/menu-text.ps1') 'EndMenuOnly'
}
elseif ($ResumeOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/action-hud.ps1') 'ResumeOnly'
}
elseif ($HudOnly) {
    . (Join-Path $PSScriptRoot 'evidence-record/action-hud.ps1') 'HudOnly'
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
if ($PracticeReplayOnly) {
    foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
    $result.scope = 'practice-replay-only'
    $result.Remove('stage')
    $result.stages = @('movement_course', 'crossfeed')
    $result.player_counts = @(1, 2)
    $result.palettes = @('saved-default')
    $result.weapons = @('sprayer')
    $result.actions = @('sustained-practice', 'pause-resume-focus', 'restart', 'timed-to-practice', 'timed-solo', 'timed-developer-course', 'timed-multiplayer', 'Watch Replay')
    $result.input_slots = @(4)
    $result.inputs = 'Normal reader with synthetic keyboard/mouse input to the claimed host; multiplayer adds one shipping bot. Actual GameLevel build/update/pause/restart/replay methods on a hidden native shell. Private-state inspection is confined to explicit template instantiations in the probe.'
    $result.checks = 'Untimed practice retains no shipping tape steps or gaps through sustained play, pause/resume, focus changes, Restart and a timed-to-practice rebuild. Three timed controls offer Watch Replay through the actual End page and reproduce public player gameplay bits, projectile counts and hashes of every paint tile at each recorded step. Zero rendering/readbacks; no physical-pad, listening, performance, human-readability or attended acceptance claim.'
    if ($Preset -eq 'x64-debug') { $result.checks += ' Debug additionally forwards a real ReplayRecorder and its inner driver through the actual GameLevel shipping filter while practice capture is disabled.' }
}
