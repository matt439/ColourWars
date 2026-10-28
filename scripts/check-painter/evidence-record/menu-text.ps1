# Dot-sourced by evidence-record.ps1 after its explicit precedence selection.
# Keep these bodies in caller scope: result key order and dynamic inputs matter.
param([string] $PainterMenuTextScope)

switch -Exact ($PainterMenuTextScope) {
    'ResultPercentageOnly' {
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
    'ContributionTeamOnly' {
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
    'CompetitiveWinnerOnly' {
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
    'RoundHeadingOnly' {
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
    'SeriesScoreOnly' {
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
    'SessionResultOnly' {
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
    'ResultsTextOnly' {
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
    'EndContinuationOnly' {
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
        $result.checks = 'Dedicated four-label continuation leaf. Independent literal copy and raw-font references compare the actual EndMenu primary label and complete primary ornament across single/series rematch, completed series, drawn round and next round. Captured copy/font assignment, reopening and reversal retain the existing geometry, persistent primary card and focus-arrow policy. Typed local callbacks prove restart/exit selection and deferred music actions without launching gameplay; frozen Session/setup/private Save/RNG and zero-update resize checks remain separate. Exactly 72 StateContext updates, 94 readbacks, 31 CSV rows, fourteen page openings, one font negative control, six 540x96 crops (311040 pixels), fourteen typed actions (thirteen restart and one exit), four deferred actions, four music starts and one successful private Play-settings seed Save are checked. Both presets run the same shared consumer scope without developer targets or commands. No broad menu matrix, match launch, foreground/global input, physical-device, listening, human readability or performance acceptance.'
    }
    'ResolutionTextOnly' {
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
    'DurationTextOnly' {
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
        $result.checks = 'Dedicated two-template duration leaf. Independent literal duration rows and the complete compound ornament are compared with actual Mode page drawing through keyboard cycling, explicit/default choices, fractional and scientific defaults, captured text/font replacement, reopening and reversal. Frozen redraw and RNG, one font negative control, byte-identical private preferences and zero-update 720/1080/720 resize checks are retained. Exactly 86 StateContext updates, 76 readbacks, 25 CSV rows, eight page openings, five 1168x72 crops (420480 pixels) and one successful private Play-settings seed Save per preset. No broad menu matrix, gameplay, foreground/global input, physical-device, listening, human readability or performance acceptance.'
        if ($Preset -eq 'x64-debug') {
            $result.checks += ' Debug separately exercises the real TuningTarget and PresentationTarget resource-only validation: two legacy stand-in acceptances, paired-content refusals, primed candidate refusals, inert failed applies, valid retries and consumed candidates. Six target validations and six private candidate writes add zero state updates, readbacks, Save calls or commands; actual content bytes and draw RNG remain unchanged.'
            $result['target_validation_scope'] = 'debug-resource-targets-only'
        }
        else {
            $result['target_validation_scope'] = 'not-run-release'
        }
    }
    'PercentageTextOnly' {
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
        $result.checks = 'Dedicated both-preset leaf for menu_text.values.percentage. Independent literal strings and UiText/OptionRow-equivalent linear references cover Controls music/SFX rows4/5 and Sticks movement/aim rows1/2 values, saved17/0/100, inherited movement0.999 rounding to100 and explicit0/90. Existing captured controls item fonts and width-then-height value fitting remain intact. Old long copy with tilde, a new font ID missing tilde and distinct numeric bearings, real adjustments, child return, reopen/reversal and zero-update720/1080/720 drawing comparisons prove retention and adoption. Actual child Apply/reload/repeat preserves pending parent volumes and unrelated preferences; controlled private-save failure keeps active10/85 while disk remains5/85, and later Escape discards pending edits. Full private save bytes, observed status colours, unchanged current preview across source replacement, frozen redraw and draw RNG are checked. Exactly189 state updates(hard189),87 readbacks,52 value CSV rows,4 status comparisons and8 compact crops per preset. No broad Controls matrix, gameplay, foreground/global input, physical-pad, hardware qualification, listening, human readability or performance acceptance.'
    }
    'StageCountsOnly' {
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
    'StageSummaryOnly' {
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
    'MenuLabelsOnly' {
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
    'TeamFocusOnly' {
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
    'StickReadoutsOnly' {
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
    'ConnectionTextOnly' {
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
    'BotPlayersOnly' {
        $result.scope = 'bot-players-only'
        $result.Remove('stage')
        $result.stages = @('crossfeed', 'movement_course')
        $result.player_counts = @(1, 2, 3, 4)
        $result.resolutions = @('1280x720', '1920x1080')
        $result.palettes = @('runtime-save-default')
        $result.weapons = @('sprayer', 'sniper', 'roller', 'mister')
        $result.input_slots = @(4, 3, -1)
        $result.actions = @('host-bot-setup', 'human-view-mapping', 'runtime-inputs', 'pause-focus-disconnect', 'restart-series', 'normal-launch-save')
        $result.inputs = 'Local keyboard snapshots and the scripted GamepadReader seam drive actual setup pages and GameLevel. Nested private profiles isolate every save and record. Native D3D11 uses the hardware adapter.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.checks = 'Focused bot setup, runtime pane mapping, lifecycle and identity checks. Difficulty feel, full platform-route/combat/reserve qualification and physical pads remain open. See run.log and bot-players CSVs for the executed bounded cases.'
    }
    'RememberedSetupOnly' {
        $result.scope = 'remembered-setup-only'
        $result.Remove('stage')
        $result.stages = @('king_of_the_hill', 'turbulence', 'close_quarters', 'movement_course')
        $result.player_counts = @(2)
        $result.palettes = @('runtime-save-default')
        $result.weapons = @('sprayer', 'sniper', 'mister', 'bucket')
        $result.resolutions = @('1280x720')
        $result.actions = @('actual-normal-menu-start', 'remembered-bot-restoration', 'last-match-save', 'editable-cancelled-choices', 'fresh-sparse-claims', 'fresh-confirmation', 'pause-restart', 'series-continuation', 'developer-bot-replay-exclusion', 'separate-process-relaunch')
        $result.input_slots = @(3, 4)
        if ($Preset -eq 'x64-debug') {
            $result.player_counts += 4
            $result.input_slots += -1
            $result.weapons += 'roller'
        }
        $result.inputs = 'Actual GameMenu owns setup pages and the typed normal/developer launch callback. The existing scripted GamepadReader seam and local keyboard snapshots drive menus without foreground or global input. All saves, developer files and replay evidence are under a nested private profile.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.checks = 'Two hidden720p processes share only a new private remembered-setup profile. The writer restores a remembered Hard bot into actual Mode/Count/Join/Team with a fresh human claim and unchanged save, then drives actual menus, compares fresh independent Mode/Count/Join/Team/Weapon/Stage page rasters, preserves the prior save through provisional/cancelled changes and the first arena confirmation, then saves resolved choices through the real normal GameMenu callback. A normal Pause Restart and two drawn best-of-three rounds update records independently of last_match. Debug additionally starts a sparse pad/keyboard/device-less bot match through an actual private script, records and replays it through the real director/menu path, and changes a normal menu choice while an explicitly claimed seat is bot-driven; all preserve the user setup. A separate native reader process reloads Save and GameMenu, restores choices, begins with a missing pad and fresh claims, reconnects without autojoining, requires each player to confirm again and cancels a changed arena without saving. Ten compact crops, CSV and command/replay evidence retain frozen page pixels/RNG. Release runs normal menus/relaunch and excludes developer branches. No physical-pad, listening, human readability or performance acceptance.'
    }
    'TeamSetupOnly' {
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
    'StickSettingsOnly' {
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
    'RumbleOnly' {
        $result.scope = 'rumble-owner-settings-only'
        $result.stage = 'movement_course'
        $result.player_counts = @(3)
        $result.input_slots = @(3, 1, 4)
        $result.Remove('palettes')
        $result.palette_selection = 'Default Random; no palette comparison.'
        $result.weapons = @('sprayer')
        $result.inputs = 'Scripted reader and a recording sink factory; no native motor API or physical pad.'
        foreach ($key in @($result.Keys)) { if ($key -like '*_checks') { $result.Remove($key) } }
        $result.checks = 'Actual GameLevel event consumption and lifecycle stops, per-slot settings Apply/Back/failure and parent merge. Fake motors only; physical feel and hardware acceptance remain separate.'
    }
    'KeyboardJumpOnly' {
        $result.scope = 'keyboard-jump-only'
        $result.Remove('stage')
        $result.stages = @('movement_course')
        $result.player_counts = @(1, 4)
        $result.palettes = @('blue_yellow')
        $result.weapons = @('sprayer', 'sniper', 'mister', 'bucket')
        $result.resolutions = @('1280x720', '1920x1080')
        $result.actions = @('actual-controls-child', 'held-entry-neutral-gate', 'single-j-release', 'repeated-frame', 'conflict-chord-overlap-unsupported-navigation', 'escape-and-focus-cancellation', 'pending-active-reset-apply-back', 'independent-parent-audio', 'failed-write-active-choice-and-retry', 'private-save-reload', 'fitted-help-status-action-labels-and-disabled-ink', 'captured-child-copy-and-effective-parent-help', 'fresh-space-menu-proceed', 'paused-countdown-effective-reminder', 'captured-layout-key-reversal', 'frozen-public-state-rng-and-debug-fingerprint', 'pad-face-and-slot-selectors', 'pad-multi-slot-latest-copy-merge', 'pad-reset-discard', 'pad-connected-owner-isolation', 'four-pad-mixed-reminder-fit')
        $result.input_slots = @(0, 1, 2, 3, 4)
        $result.inputs = 'Public Keyboard.on_key_down/on_key_up and poll through actual Controls/KeyboardJumpMenu/StateContext and a one-player GameLevel Pause route. Scripted GamepadReader seam for owner/fallback checks; private save/debug profile; hidden owned window. No global/foreground input or physical-device claim.'
        $result.inputs += ' PadButtonsMenu adds32 actual slot/face Apply/reload cases, a skipped-conflict Shoot Apply and66 independent native glyph pairs; scripted physical-owner navigation reaches the actual GameLevel Pause/Options/Controls/Pad Buttons route and updates its countdown reminder while covered. A public LevelBuilder fixture shows all four distinct pad mappings in a reordered four-player roster.'
        if ($Preset -eq 'x64-debug') {
            $result.player_counts = @(1, 2, 4)
            $result.actions += 'actual-reader-post-mapping-record-play-with-different-saved-faces'
            $result.inputs += ' Debug additionally records360 actual-reader inputs after production slot-to-seat mapping in a seeded public two-pad build, writes/reads the production tape, persists different face choices and rebuilds from decoded setup; changed live inputs cannot alter the exact played seat streams, per-tick fingerprints, six checkpoints, final state or terminal RNG. The local test tuning changes only countdown and its contained push to a quarter step.'
        }
        $result.audio = 'Private menu save starts muted; pending parent music is applied without a music owner, then muted again before the actual GameLevel fixture. No listening acceptance.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.checks = 'Bounded both-preset keyboard Jump UI/save/native-render slice at720/1080. Actual Controls entry and independent child capture/Reset/Apply/Back check held entry and opener neutral gating, single J gesture/release, repeated same-frame reads, chord/overlap/A-conflict/unsupported and navigation ownership, Escape and focus cancellation, pending versus active keys, independent parent pending audio, two controlled failed writes with active preference and byte-identical prior JSON, retry and ten fresh Save reloads. Independently fitted native linear help/status text and four visible action labels plus disabled ink are checked; captured child templates survive replacement, parent effective help refreshes on return, and Space capture requires a separate fresh menu confirmation. Two actual GameLevel Pause/Options/Controls/Jump routes update the countdown reminder via draw/refresh at unchanged resolution, without a covered Level simulation update; key reversal preserves original text/layout despite unrelated live geometry changes. Frozen draw RNG/public state and Debug fingerprint remain checked. Native counters require42 help/status pairs in Release or44 in Debug, ten Save reloads and two opening captures; CSV and compact native crops accompany the result. Debug adds a private bare-J bind warning and removal. A separate actual-menu ownership leaf requires eleven common scripted-pad observations plus eleven Debug observations: owner Back/key drain, other-pad exclusion, disconnected-owner participant and spare fallback, reconnect rearming and fresh Space; Debug checks queued work, grave/F11/chord ownership and Keyboard-held F10 through capture, release and focus loss without opening the debug menu or global input. Actual OS focus delivery remains outside this probe. Process relaunch, physical devices, exhaustive key domain, human readability, listening and performance are outside this leaf.'
    }
    'ControlsMenuOnly' {
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
    'SettingsMenuOnly' {
        $result.player_counts = @()
        $result.palettes = @('blue_orange', 'pink_green', 'blue_yellow', 'random')
        $result.weapons = @()
        $result.actions = @('row-change', 'apply', 'failed-save', 'controls-return', 'discard', 'disabled-resolution', 'credits-back', 'custom-status', 'status-font-snapshot', 'save-recovery', 'interrupted-apply')
        $result.actions += @('custom-setting-values', 'value-width-height-fit', 'value-font-snapshot', 'value-child-return')
        $result.input_slots = @(4)
        $result.inputs = 'Public keyboard through production Options/Credits and StateContext, with a second isolated save profile; fullscreen metadata exercises disabled rows. Only false/Off is applied using an isolated hidden, windowed fixture, which must stay hidden and inactive.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.Remove('stage')
        $result.checks = 'Options/Credits paper, all enabled focus cards/arrows, native ink/paper contrast, measured labels, all palette values, long failure status and complete authored credits at 720p/1080p/QHD, 4:3, 16:10 and ultrawide. Actual row edits, staged Apply, persisted reload and failed-write warning, Controls/audio return preserving pending edits/focus, Back discard, disabled fullscreen resolution and retained windowed preset, Credits Select/Back, frozen redraw, draw RNG, static Reduce effects and resize. Independent Options pending/saved/failure rasters pass across six sizes/aspects; authored width fitting and scale reset, missing-old-glyph status font, retained/reopened/reversed callbacks, interrupted/repeated Apply, controlled failure/recovery, isolated reload and both discard routes pass at 720p. No physical-controller, human readability or listening acceptance.'
        $result.checks += ' Independent Options On/Off/Random defaults pass across six sizes/aspects. An Options item-font fixture exercises width and tall-glyph height fitting, complete value/focus-card unions, Random-to-short-fixed-palette scale reset and replacement without an old glyph. Retained/reopened/reversed callbacks, both fullscreen toggle directions and Random palette cycles, one Controls child On/Off pair and return, interrupted/repeated windowed-Off Apply, controlled failure/recovery, isolated reload and discard pass at 720p.'
    }
    'PausedOptionsOnly' {
        $result.scope = 'paused-options-only'
        $result.player_counts = @(1, 2, 4)
        $result.palettes = @('blue_yellow', 'pink_green', 'random')
        $result.weapons = @('sprayer')
        $result.actions = @('actual-match-pause-options', 'all-seven-focus-rows', 'caption-font-snapshot', 'own-back-discard', 'palette-next-build', 'sparse-owner', 'joined-fallback', 'keyboard-recovery', 'held-reconnect', 'controls-sticks-merge', 'windowed-display-trial', 'revert', 'escape', 'timeout', 'focus-loss', 'cover', 'keep-failed-write-recovery', 'reduce-effects', 'nonpositive-time', 'developer-unwind', 'frozen-draw-rng')
        $result.input_slots = @(4, 3, 0, 2, 1)
        $result.inputs = 'Actual1/2/4 player GameLevel to Pause row2 Options, scripted sparse pad rosters and local keyboard. Nested isolated profile with nondefault shared preferences and four distinct deadzone profiles. Whole-fixture activation guard (set_resolution activates a hidden window); hidden private windowed display changes only.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.Remove('stage')
        $result.checks = 'Independent complete seven-row paused Options paper, caption, values, status and footer at six native sizes/aspects. Retained Level identity, roster/layout/resume clock, full observation and actual Level/HUD raster at a common720 size cover painted match preservation; no direct private Session or timer-field claim. Owner-only input, joined fallback, keyboard recovery and reconnect arming propagate into actual Controls/Pad Sticks and display child. Separate nested saves preserve other preferences; palette-only Apply leaves the live arena unchanged and Restart uses its new palette. Private windowed trial safe Revert/Escape, elapsed timeout, focus/cover rollback, failed Keep preserving old JSON and recovery reload. Back returns retained Pause; only Pause Resume starts count-in. Captured caption/font, lifecycle, reduction and frozen draw RNG. No physical-pad, user-save, hardware-fullscreen, global-input, human or performance acceptance.'
    }
    'DisplayTrialOnly' {
        $result.scope = 'display-trial-only'
        $result.player_counts = @()
        $result.palettes = @('blue_yellow', 'random')
        $result.weapons = @()
        $result.actions = @('actual-options-apply', 'default-revert', 'keep', 'escape', 'controlled-timeout', 'wall-clock-timeout', 'focus-loss', 'cover-rollback', 'developer-dismiss-reset', 'clear-rollback', 'held-entry', 'pad-reconnect', 'conflicting-input', 'move-confirm-separation', 'back-priority', 'keep-failed-write', 'recovery', 'metadata-fullscreen-keep-off', 'reduce-effects', 'nonpositive-time', 'frozen-rng')
        $result.input_slots = @(4, 3)
        $result.inputs = 'Actual Options changed-resolution Apply and its pushed trial child; local keyboard and sparse scripted pad slot3. Separate nested profile with nondefault controls and four distinct pad profiles. One nonnested activation guard covers all private-window resizes and rollbacks, including destruction, because set_resolution activates a hidden window; it stays hidden unaided. Actual tests stay windowed Off; fullscreen is saved metadata only.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.Remove('stage')
        $result.checks = 'Independent complete trial background, compact card geometry with captured Pause palette, centered Options/question, Keep/Revert card/arrow, countdown and footer at six native sizes/aspects. Explicit finite simulation time leads wall capture time under a configured60-second bound; the shipped15-second timeout is separately sampled after3.25 simulated seconds, and a1-second trial proves zero-dt wall expiry. Real Options Apply first changes only its private window to1920x1080; Save/file remain unchanged until fresh Keep. Default Revert/Escape, simulated exact deadline, wall deadline, focus loss, cover, developer dismiss/reset and clear restore the old configured1280 preset/prior actual windowed mode without preference/file writes. Cover remains present until explicitly popped and delivers one typed result afterward. Keep/reload, repeated unchanged Apply, one controlled failed Keep preserving old JSON and recovery preserve every shared preference and all four profiles. Keyboard/pad conflicts, held reconnect/entry, move-confirm separation, Back priority, immediate reduction and frozen pixels/RNG pass; nonpositive updates preserve nonclock geometry while the wall countdown may advance. Native backbuffer resizing for the six-aspect references does not replace configured rollback targets. No user-save, hardware-fullscreen, global input, physical-pad, human readability, listening or performance acceptance.'
    }
    'OptionsDefaultsOnly' {
        $result.scope = 'options-defaults-only'
        $result.player_counts = @()
        $result.palettes = @('blue_yellow', 'random')
        $result.weapons = @()
        $result.actions = @('all-seven-rows', 'staged-restore', 'repeat-restore', 'back-discard', 'escape-discard', 'controls-return', 'controls-save-merge', 'windowed-apply', 'repeat-apply', 'reload', 'failed-write', 'recovery', 'held-confirm-rearm', 'pad-reconnect', 'fullscreen-metadata-off', 'reduce-effects', 'lifecycle-reset', 'frozen-rng')
        $result.input_slots = @(4, 3)
        $result.inputs = 'Actual Options and Controls child in StateContext; public local keyboard messages and sparse scripted pad slot3. Nested isolated save profile with nondefault display/play settings and four distinct pad profiles. Windowed Apply must leave the hidden probe hidden and inactive; fullscreen is metadata only.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.Remove('stage')
        $result.checks = 'Independent complete seven-row Options paper/cards/arrow, fitted labels/values, plain text, pending/saved/failure status and footer at six sizes/aspects. Canonical display/palette defaults remain staged with active/window/file/play/profile state unchanged until Apply; repeated restore, both discard routes and Controls return retain intended values/focus. Hidden windowed Apply/repeated Apply/reload and six controlled temporary-file write refusals/recovery preserve every unrelated play field and all four slot profiles. At720p keyboard deactivation and held pad reconnect require fresh Restore, reduced effects/lifecycle/developer reset remain bounded, applied-fullscreen metadata keeps the original disabled/live-resolution policy until Off Apply, and a Controls-saved preference merges into the pending display Apply. Frozen pixels and draw RNG; no user-save, hardware-fullscreen, physical-pad, human readability, listening or performance acceptance.'
    }
    'HomeExitOnly' {
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
    'HomeReturnFocusOnly' {
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
    'StagePaperOnly' {
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
    'WeaponPaperOnly' {
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
    'TeamPaperOnly' {
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
    'JoinPaperOnly' {
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
    'SetupPaperOnly' {
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
    'JoinStageMotionOnly' {
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
    'SetupMotionOnly' {
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
    'MenuMotionOnly' {
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
    'MenuTextOnly' {
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
    'WeaponCatalogOnly' {
        $result.player_counts = @(1)
        $result.palettes = @('runtime-save-default')
        $result.weapons = @('sprayer', 'sniper', 'roller', 'mister', 'bucket', 'random')
        $result.actions = @('browse', 'wrap-left', 'wrap-right', 'confirm-release', 'stage-back', 'mode-back', 'reopen')
        $result.input_slots = @(4)
        $result.inputs = 'Public keyboard input through actual Weapon/Stage/Mode Select pages on the normal GamepadReader backend.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.checks = 'Six historical default names and icon/body/paint/full-supply bindings compared with independent fixtures at 720p/1080p/QHD, both wraps, provisional settings, fresh release confirmation, seeded concrete Random primary, actual Stage/Mode Back preserving all roster and match settings. Edited name/body/paint/supply and a sparse alternate atlas/fill become visible on reopen; existing pages keep their snapshots across edits, reversals and cycling. Frozen raster and draw RNG, native detail/context images and CSV retained. No physical-controller, human readability, performance or listening acceptance.'
    }
    'MainMenuOnly' {
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
    'ResultsMenuOnly' {
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
    'PauseMenuOnly' {
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
    'EndMenuOnly' {
        $result.player_counts = @()
        $result.palettes = @()
        $result.weapons = @()
        $result.actions = @('rematch', 'change-teams', 'change-weapons', 'change-stage', 'exit', 'next-round', 'replay-draw', 'play-series-again', 'custom-labels', 'custom-headings', 'font-snapshot', 'pending-music-exit', 'focus-return', 'between-rounds-tools', 'between-rounds-stage')
        $result.input_slots = @(4)
        $result.inputs = 'Public keyboard input through the actual EndMenuInitial, its between-rounds Weapon and Stage Select pages and StateContext; no physical pad claim.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.Remove('stage')
        $result.checks = 'End-menu paper, primary continuation, four independent legacy action labels and three independent legacy headings at native 720p/1080p/QHD, 4:3, 16:10 and ultrawide; single match, tied/continuing/completed series, both Exit rows and hidden setup choices. Complete measured heading bounds use the unscaled actual heading font, full-paper centering and a stepped-paper reference with production point filtering. Custom labels/headings and replacement item/heading fonts missing an old glyph; retained/reopened/reversed snapshots through focus movement and repeated typed actions preserve the Session ledger. Owner-controlled pending music exits reject further input and preserve their original action through focus interruption; interrupted Enter releases require fresh selection. Frozen redraw/RNG, Reduce effects and resize remain covered. No physical controllers, human readability or listening acceptance.'
    }
    default { throw "Unknown painter menu-text evidence scope: $PainterMenuTextScope" }
}
