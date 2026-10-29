# Every consumer acceptance case, declared once. run-acceptance.ps1 reads this
# file for selection, sources, arguments and scheduling; CI selects through the
# runner's switches rather than writing case names out. A case compiles
# scripts/acceptance/<Name>.cpp, then its Sources, then every .cpp in a
# same-named folder, against the game library and both platform adapters.
#
#   Group        'default' (omitted): in the default list of both presets.
#                'debug-only': in the Debug default list; refused in Release.
#                'opt-in': never run by default; -OptIn selects all of them.
#   Sources      Extra sources, relative to the repository root.
#   Arguments    What the probe is given, in order: 'game' is the staged game
#                folder, 'repo' the checkout. Omitted means @('game').
#   StartRank    Started first, lowest rank first, when cases run in parallel:
#                the slowest cases in CI timings (acceptance-summary.json).
#   SlowInDebug  Left out by -SkipSlowDebug, which the Debug pull-request job
#                passes: minutes each under Debug's checked iterators.
#   Utf8Output   Its output is decoded as UTF-8; such a case runs serially.
@{
Cases = @(
    @{ Name = 'display_awake' }
    @{ Name = 'rumble' }
    @{ Name = 'rumble_adapter' }
    @{ Name = 'action_bindings' }
    @{ Name = 'input_bindings' }
    @{ Name = 'keyboard_jump_capture' }
    @{ Name = 'ammo' }
    @{ Name = 'settings' }
    @{ Name = 'player_profiles' }
    @{ Name = 'match_records' }
    @{ Name = 'remembered_setup' }
    @{ Name = 'bot_roster' }
    @{ Name = 'stick_settings' }
    @{ Name = 'paint' }
    @{ Name = 'camera_framing' }
    @{ Name = 'view_shake' }
    @{
        Name = 'movement'
        StartRank = 8
    }
    @{ Name = 'movement_contact' }
    @{ Name = 'combat_feedback' }
    @{ Name = 'tool_motion' }
    @{ Name = 'weapon_feedback' }
    @{ Name = 'low_paint' }
    @{ Name = 'aim_marker' }
    @{ Name = 'player_identity' }
    @{ Name = 'blocked_shot' }
    @{ Name = 'idle_drip' }
    @{ Name = 'quiet_character' }
    @{ Name = 'stage_selection' }
    @{ Name = 'team_palette' }
    @{ Name = 'team_selection' }
    @{ Name = 'menu_navigation' }
    @{
        Name = 'movement_course'
        StartRank = 6
    }
    @{
        Name = 'cq_blockout'
        StartRank = 9
    }
    @{
        Name = 'crossfeed'
        StartRank = 1
        SlowInDebug = $true
    }
    @{
        Name = 'press_tower'
        StartRank = 3
        SlowInDebug = $true
    }
    @{ Name = 'turbulence_blockout' }
    @{ Name = 'resume_countdown' }
    @{
        Name = 'debug_console'
        StartRank = 4
        SlowInDebug = $true
    }
    @{
        # The real game objects use the engine's audio seam unchanged. Its
        # recording backend, compiled ahead of the static engine archive, lets
        # this case observe sound calls without opening hardware.
        Name = 'story_audio'
        Sources = @('external/labrador/engine/audio/null/audio_device.cpp')
        StartRank = 5
    }
    @{
        Name = 'level_format'
        Arguments = @('game', 'repo')
    }
    @{ Name = 'fatal_report' }
    @{ Name = 'gameplay_tuning' }
    @{
        Name = 'menu_presentation'
        StartRank = 7
    }
    @{
        Name = 'weapon_catalog'
        StartRank = 14
    }
    @{
        Name = 'menu_text'
        StartRank = 2
        SlowInDebug = $true
    }
    @{ Name = 'menu_focus_motion' }
    @{ Name = 'display_trial' }
    @{ Name = 'tunnelling' }
    @{ Name = 'text_wrap' }
    @{ Name = 'terrain' }
    @{ Name = 'lethal_provenance' }
    @{ Name = 'skirmish_bot' }
    @{
        Name = 'paint_surfaces'
        StartRank = 11
    }
    @{
        Name = 'balance_regions'
        StartRank = 10
    }
    @{
        Name = 'paint_underfoot'
        StartRank = 13
    }
    @{
        Name = 'moving_structure'
        StartRank = 12
    }
    @{ Name = 'fixed_slab' }
    @{ Name = 'practice_workshop' }
    @{ Name = 'results_awards' }
    @{ Name = 'paint_ferry' }
    @{ Name = 'team_gate' }
    @{ Name = 'bot_driver' }
    @{ Name = 'bot_runtime' }
    @{ Name = 'bot_recovery' }
    @{ Name = 'bot_ballistics' }
    @{ Name = 'bot_navigation' }
    @{ Name = 'bot_territory' }
    @{ Name = 'bot_gate_routes' }
    @{ Name = 'handicap' }
    @{ Name = 'instant_replay' }
    @{ Name = 'user_stages' }
    @{ Name = 'series_stage_rule' }
    @{ Name = 'teammate_markers' }
    @{ Name = 'cursor_marker' }
    @{
        # Reuses only the reviewed measurement and serializer, not their
        # fixture or main. Its console output is UTF-8, decoded on its own
        # after any parallel batch because native decoding is process-wide.
        Name = 'balance_runner'
        Group = 'debug-only'
        Sources = @('scripts/acceptance/balance_regions/metrics.cpp', 'scripts/acceptance/balance_regions/csv.cpp')
        Utf8Output = $true
    }
    @{
        Name = 'bot_trials'
        Group = 'opt-in'
    }
    @{
        Name = 'bot_combat_trials'
        Group = 'opt-in'
    }
    @{
        # Reuses balance_regions' reviewed measurement.
        Name = 'bot_team_trials'
        Group = 'opt-in'
        Sources = @('scripts/acceptance/balance_regions/metrics.cpp')
    }
    @{
        Name = 'bot_ascents'
        Group = 'opt-in'
    }
    @{
        Name = 'bot_arena_routes'
        Group = 'opt-in'
    }
    @{
        Name = 'bot_hill_routes'
        Group = 'opt-in'
    }
    @{
        Name = 'bot_turbulence_routes'
        Group = 'opt-in'
    }
    @{
        Name = 'bot_scan_cost'
        Group = 'opt-in'
    }
    @{
        Name = 'bot_mover_routes'
        Group = 'opt-in'
    }
    @{
        Name = 'bot_slab_routes'
        Group = 'opt-in'
    }
    @{
        Name = 'bot_blockout_routes'
        Group = 'opt-in'
    }
    @{
        Name = 'turbulence_movers'
        Group = 'opt-in'
    }
    @{
        Name = 'turbulence_slopes'
        Group = 'opt-in'
    }
    @{
        Name = 'turbulence_slab_routes'
        Group = 'opt-in'
    }
    @{
        Name = 'turbulence_slab_stage'
        Group = 'opt-in'
        Arguments = @('game', 'repo')
    }
    @{
        Name = 'turbulence_slab_tools'
        Group = 'opt-in'
    }
)
}
