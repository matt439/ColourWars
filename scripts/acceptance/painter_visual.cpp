#include "game/pch.h"
#include "game/sound_groups.h"
#include "engine/app/application.h"
#include "engine/core/file_path.h"
#include "game/input_slots.h"
#include "game/objects/level_builder.h"
#include "game/objects/projectile_spray.h"
#include "game/objects/weapon_feedback.h"
#include "game/objects/combat_feedback.h"
#include "game/objects/blocked_shot_feedback.h"
#include "game/objects/idle_drip.h"
#include "game/objects/screen_cursor.h"
#include "game/objects/player_badge.h"
#include "game/objects/countdown_art.h"
#include "game/objects/reveal_art.h"
#include "game/objects/figure_acting.h"
#include "game/objects/drifting_visual.h"
#include "game/objects/terrain_material.h"
#include "game/objects/fixed_slab_paintable.h"
#include "game/objects/fixed_slab_shadow.h"
#include "game/objects/team_gate_structure_paintable.h"
#include "game/objects/translating_structure_paintable.h"
#include "game/objects/translated_draw_scope.h"
#include "game/objects/player_builder.h"
#include "game/game_data.h"
#include "game/states/stage_preview.h"
#include "game/states/user_stage_refusals_menu.h"
#include "game/states/match_start.h"
#include "game/stage_loading.h"
#include "game/stage_scan_diagnostics.h"
#include "engine/math/triangle.h"
#include "game/states/menu_panel.h"
#include "game/states/game_states.h"
#include "game/states/end_menu.h"
#include "game/states/pause_menu.h"
#include "game/states/results_menu.h"
#include "game/states/main_menu.h"
#include "game/states/menu_content_validation.h"
#include "game/text_wrap.h"
#include "game/states/display_confirmation.h"
#include "game/states/controls_settings_menu.h"
#include "game/states/stick_settings_menu.h"
#include "game/states/keyboard_jump_menu.h"
#include "game/states/pad_buttons_menu.h"
#include "game/controls_text.h"
#include "game/objects/stick_response.h"
#include "game/build_info.h"
#include "game/save.h"
#include "game/debug/dev_tools.h"
#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_fingerprint.h"
#include "game/debug/debug_director_impl.h"
#include "game/debug/debug_console_view.h"
#include "game/debug/debug_perf.h"
#include "game/debug/tune_document.h"
#include "game/debug/tune_target.h"
#include "game/debug/replay.h"
#endif

#include <filesystem>
#include <bit>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <string_view>
#include <thread>
#include <cstdio>
#include <io.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include "scripts/acceptance/painter_visual/rumble_sink_fixture.h"
#include "scripts/acceptance/support/probe.h"
#include "scripts/acceptance/support/slot_seats.h"
#include "scripts/acceptance/support/stage_catalog.h"

#if defined(COLOURWARS_PAINTER_SCRIPTED_PADS)
// This focused probe supplies the concrete input backend at the link seam.
// Normal painter binaries retain Labrador's XInput implementation.
namespace painter_scripted_pads
{
    std::array<labrador::GamepadState, labrador::Gamepads::max_count> states{};
}
namespace labrador
{
    struct GamepadReader::Impl
    {
        std::array<GamepadState, Gamepads::max_count> states{};
        bool suspended = false;
    };
    GamepadReader::GamepadReader() : impl_(std::make_unique<Impl>()) {}
    GamepadReader::~GamepadReader() = default;
    GamepadReader::GamepadReader(GamepadReader&&) noexcept = default;
    GamepadReader& GamepadReader::operator=(GamepadReader&&) noexcept = default;
    void GamepadReader::sample()
    {
        this->impl_->states = this->impl_->suspended ?
            std::array<GamepadState, Gamepads::max_count>{} : painter_scripted_pads::states;
    }
    GamepadState GamepadReader::read(int slot) const
    {
        return slot >= 0 && slot < Gamepads::max_count ?
            this->impl_->states[static_cast<size_t>(slot)] : GamepadState{};
    }
    void GamepadReader::suspend() { this->impl_->suspended = true; }
    void GamepadReader::resume() { this->impl_->suspended = false; }
}
#endif

using namespace labrador;
using namespace mattmath;

namespace
{
#include "scripts/acceptance/painter_visual/painter_fixtures.h"
#include "scripts/acceptance/painter_visual/scripted_menu_owner.h"
#include "scripts/acceptance/painter_visual/action_combat_checks.h"
#include "scripts/acceptance/painter_visual/tool_checks.h"
#include "scripts/acceptance/painter_visual/blocked_shot_checks.h"
#include "scripts/acceptance/painter_visual/movement_contact_checks.h"
#include "scripts/acceptance/painter_visual/quiet_checks.h"
#include "scripts/acceptance/painter_visual/reduced_effects_checks.h"
#include "scripts/acceptance/painter_visual/view_shake_checks.h"
#include "scripts/acceptance/painter_visual/aim_marker_checks.h"
#include "scripts/acceptance/painter_visual/drift_checks.h"
#include "scripts/acceptance/painter_visual/foreground_checks.h"
#include "scripts/acceptance/painter_visual/monitor_checks.h"
#include "scripts/acceptance/painter_visual/miniature_reference_checks.h"
#include "scripts/acceptance/painter_visual/miniature_terrain_checks.h"
#include "scripts/acceptance/painter_visual/countdown_checks.h"
#include "scripts/acceptance/painter_visual/cq_blockout_checks.h"
#include "scripts/acceptance/painter_visual/crossfeed_checks.h"
#include "scripts/acceptance/painter_visual/gate_permission_checks.h"
#include "scripts/acceptance/painter_visual/ferry_direction_checks.h"
#include "scripts/acceptance/painter_visual/turbulence_mover_checks.h"
#include "scripts/acceptance/painter_visual/turbulence_shoulder_checks.h"
#include "scripts/acceptance/painter_visual/turbulence_slabs_checks.h"
#include "scripts/acceptance/painter_visual/press_tower_checks.h"
#include "scripts/acceptance/painter_visual/turbulence_blockout_checks.h"
#include "scripts/acceptance/painter_visual/miniature_path_checks.h"
#include "scripts/acceptance/painter_visual/surface_finish_checks.h"
#include "scripts/acceptance/painter_visual/route_warning_checks.h"
#include "scripts/acceptance/painter_visual/landmark_checks.h"
#include "scripts/acceptance/painter_visual/parallax_checks.h"
#include "scripts/acceptance/painter_visual/reveal_checks.h"
#include "scripts/acceptance/painter_visual/hud_checks.h"
#include "scripts/acceptance/painter_visual/supply_checks.h"
#include "scripts/acceptance/painter_visual/resume_fixture.h"
#include "scripts/acceptance/painter_visual/music_flow_checks.h"
#include "scripts/acceptance/painter_visual/debug_frame_text_checks.h"
#include "scripts/acceptance/painter_visual/resume_checks.h"
#include "scripts/acceptance/painter_visual/end_menu_checks.h"
#include "scripts/acceptance/painter_visual/end_heading_checks.h"
#include "scripts/acceptance/painter_visual/pause_menu_checks.h"
#include "scripts/acceptance/painter_visual/pause_item_checks.h"
#include "scripts/acceptance/painter_visual/pause_question_checks.h"
#include "scripts/acceptance/painter_visual/results_menu_checks.h"
#include "scripts/acceptance/painter_visual/main_menu_checks.h"
#include "scripts/acceptance/painter_visual/title_home_text_checks.h"
#if COLOURWARS_DEV_TOOLS
#include "scripts/acceptance/painter_visual/tune_colour_checks.h"
#endif
#include "scripts/acceptance/painter_visual/feedback_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/combat_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/blocked_shot_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/idle_drip_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/respawn_pen_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/cursor_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/silhouette_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/reveal_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/cursor_material_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/desktop_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/power_light_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/program_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/program_level_visibility_checks.h"
#include "scripts/acceptance/painter_visual/acting_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/glass_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/silhouette_body_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/scribble_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/hud_health_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/hud_track_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/crack_colour_fixture.h"
#include "scripts/acceptance/painter_visual/crack_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/limb_colour_fixture.h"
#include "scripts/acceptance/painter_visual/limb_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/quiet_face_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/hud_timer_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/hud_closing_text_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/hud_closing_material_checks.h"
#include "scripts/acceptance/painter_visual/hud_closing_shade_checks.h"
#include "scripts/acceptance/painter_visual/hud_paper_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/hud_label_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/player_identity_ink_checks.h"
#include "scripts/acceptance/painter_visual/player_identity_background_checks.h"
#include "scripts/acceptance/painter_visual/player_identity_checks.h"
#include "scripts/acceptance/painter_visual/setup_paper_checks.h"
#include "scripts/acceptance/painter_visual/weapon_catalog_checks.h"
#include "scripts/acceptance/painter_visual/menu_item_page_checks.h"
#include "scripts/acceptance/painter_visual/menu_setup_text_checks.h"
#include "scripts/acceptance/painter_visual/menu_text_checks.h"
#include "scripts/acceptance/painter_visual/stage_random_text_checks.h"
#include "scripts/acceptance/painter_visual/stage_briefing_text_checks.h"
#include "scripts/acceptance/painter_visual/stage_cue_text_checks.h"
#include "scripts/acceptance/painter_visual/stage_count_checks.h"
#include "scripts/acceptance/painter_visual/stage_scan_checks.h"
#include "scripts/acceptance/painter_visual/save_status_checks.h"
#include "scripts/acceptance/painter_visual/settings_value_checks.h"
#include "scripts/acceptance/painter_visual/settings_menu_checks.h"
#include "scripts/acceptance/painter_visual/controls_help_checks.h"
#include "scripts/acceptance/painter_visual/controls_menu_item_checks.h"
#include "scripts/acceptance/painter_visual/controls_menu_checks.h"
#include "scripts/acceptance/painter_visual/stick_settings_checks.h"
#include "scripts/acceptance/painter_visual/keyboard_jump_checks.h"
#include "scripts/acceptance/painter_visual/keyboard_jump_ownership_checks.h"
#include "scripts/acceptance/painter_visual/pad_buttons_checks.h"
#include "scripts/acceptance/painter_visual/rumble_settings_checks.h"
#include "scripts/acceptance/painter_visual/rumble_owner_checks.h"
#include "scripts/acceptance/painter_visual/debug_background_checks.h"
#include "scripts/acceptance/painter_visual/percentage_checks.h"
#include "scripts/acceptance/painter_visual/menu_motion_checks.h"
#include "scripts/acceptance/painter_visual/home_alpha_checks.h"
#include "scripts/acceptance/painter_visual/movement_debug_alpha_checks.h"
#if COLOURWARS_DEV_TOOLS
#include "scripts/acceptance/painter_visual/debug_console_alpha_checks.h"
#endif
#include "scripts/acceptance/painter_visual/duration_text_checks.h"
#include "scripts/acceptance/painter_visual/resolution_text_checks.h"
#include "scripts/acceptance/painter_visual/end_continuation_checks.h"
#include "scripts/acceptance/painter_visual/results_text_checks.h"
#include "scripts/acceptance/painter_visual/session_result_checks.h"
#include "scripts/acceptance/painter_visual/series_score_checks.h"
#include "scripts/acceptance/painter_visual/round_heading_checks.h"
#include "scripts/acceptance/painter_visual/competitive_winner_checks.h"
#include "scripts/acceptance/painter_visual/contribution_team_checks.h"
#include "scripts/acceptance/painter_visual/result_percentage_checks.h"
#include "scripts/acceptance/painter_visual/series_repick_checks.h"
#include "scripts/acceptance/painter_visual/practice_replay_checks.h"
#if defined(COLOURWARS_PAINTER_SCRIPTED_PADS)
#include "scripts/acceptance/painter_visual/team_setup_checks.h"
#include "scripts/acceptance/painter_visual/remembered_setup_checks.h"
#include "scripts/acceptance/painter_visual/bot_setup_checks.h"
#include "scripts/acceptance/painter_visual/bot_runtime_checks.h"
#include "scripts/acceptance/painter_visual/connection_text_checks.h"
#include "scripts/acceptance/painter_visual/stick_readout_checks.h"
#include "scripts/acceptance/painter_visual/setup_motion_checks.h"
#include "scripts/acceptance/painter_visual/join_stage_motion_checks.h"
#include "scripts/acceptance/painter_visual/join_paper_checks.h"
#include "scripts/acceptance/painter_visual/team_paper_checks.h"
#include "scripts/acceptance/painter_visual/team_focus_checks.h"
#include "scripts/acceptance/painter_visual/menu_label_checks.h"
#include "scripts/acceptance/painter_visual/weapon_paper_checks.h"
#include "scripts/acceptance/painter_visual/weapon_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/stage_paper_checks.h"
#include "scripts/acceptance/painter_visual/stage_preview_background_tune_colour_checks.h"
#include "scripts/acceptance/painter_visual/stage_summary_checks.h"
#include "scripts/acceptance/painter_visual/home_return_focus_checks.h"
#include "scripts/acceptance/painter_visual/home_exit_checks.h"
#include "scripts/acceptance/painter_visual/options_defaults_checks.h"
#include "scripts/acceptance/painter_visual/display_trial_checks.h"
#include "scripts/acceptance/painter_visual/paused_options_checks.h"
#endif
}

int wmain(int argc, wchar_t** argv)
{
    return acceptance::run_probe("painter visual", [&]
    {
        require(argc == 3 || (argc == 5 && (std::wstring(argv[3]) == L"turbulence-mover" || std::wstring(argv[3]) == L"turbulence-shoulder" || std::wstring(argv[3]) == L"turbulence-slab") && !std::wstring(argv[4]).empty()) || (argc == 4 && (std::wstring(argv[3]) == L"hud-paper-tune-colours" || std::wstring(argv[3]) == L"hud-closing-shade" || std::wstring(argv[3]) == L"hud-closing-materials" || std::wstring(argv[3]) == L"stage-preview-background-tune-colour" || std::wstring(argv[3]) == L"hud-label-tune-colours" || std::wstring(argv[3]) == L"hud-timer-tune-colour" ||
            std::wstring(argv[3]) == L"identity" ||
            std::wstring(argv[3]) == L"quiet" || std::wstring(argv[3]) == L"drift" || std::wstring(argv[3]) == L"landmark" || std::wstring(argv[3]) == L"parallax" || std::wstring(argv[3]) == L"miniature-terrain" || std::wstring(argv[3]) == L"cq-blockout" || std::wstring(argv[3]) == L"crossfeed" || std::wstring(argv[3]) == L"press-tower" || std::wstring(argv[3]) == L"turbulence-blockout" || std::wstring(argv[3]) == L"gate-permission" || std::wstring(argv[3]) == L"ferry-direction" || std::wstring(argv[3]) == L"surface-finish" || std::wstring(argv[3]) == L"route-warning" || std::wstring(argv[3]) == L"tune-colours" || std::wstring(argv[3]) == L"monitor" ||
            std::wstring(argv[3]) == L"countdown" || std::wstring(argv[3]) == L"closing" ||
            std::wstring(argv[3]) == L"supply" || std::wstring(argv[3]) == L"hud" ||
            std::wstring(argv[3]) == L"practice-replay" || std::wstring(argv[3]) == L"resume" || std::wstring(argv[3]) == L"end-menu" ||
            std::wstring(argv[3]) == L"pause-menu" || std::wstring(argv[3]) == L"results-menu" ||
            std::wstring(argv[3]) == L"main-menu" || std::wstring(argv[3]) == L"settings-menu" ||
            std::wstring(argv[3]) == L"controls-menu" || std::wstring(argv[3]) == L"stick-settings" ||
            std::wstring(argv[3]) == L"keyboard-jump" || std::wstring(argv[3]) == L"rumble" ||
            std::wstring(argv[3]) == L"percentage-text" || std::wstring(argv[3]) == L"feedback-tune-colours" || std::wstring(argv[3]) == L"combat-tune-colours" || std::wstring(argv[3]) == L"blocked-shot-tune-colours" || std::wstring(argv[3]) == L"idle-drip-tune-colours" || std::wstring(argv[3]) == L"respawn-pen-tune-colours" || std::wstring(argv[3]) == L"cursor-tune-colours" || std::wstring(argv[3]) == L"silhouette-tune-colours" || std::wstring(argv[3]) == L"reveal-tune-colours" || std::wstring(argv[3]) == L"cursor-material-tune-colours" || std::wstring(argv[3]) == L"desktop-tune-colour" || std::wstring(argv[3]) == L"power-light-tune-colour" || std::wstring(argv[3]) == L"program-tune-colour" || std::wstring(argv[3]) == L"program-level-visibility" || std::wstring(argv[3]) == L"acting-tune-colours" || std::wstring(argv[3]) == L"glass-tune-colour" || std::wstring(argv[3]) == L"silhouette-body-tune-colour" || std::wstring(argv[3]) == L"scribble-tune-colour" || std::wstring(argv[3]) == L"hud-health-tune-colour" || std::wstring(argv[3]) == L"hud-track-tune-colour" || std::wstring(argv[3]) == L"crack-tune-colour" || std::wstring(argv[3]) == L"limb-tune-colours" ||
            std::wstring(argv[3]) == L"exposed-colour-alpha" || std::wstring(argv[3]) == L"duration-text" ||
            std::wstring(argv[3]) == L"resolution-text" || std::wstring(argv[3]) == L"end-continuation" || std::wstring(argv[3]) == L"results-text" || std::wstring(argv[3]) == L"session-result" || std::wstring(argv[3]) == L"series-score" || std::wstring(argv[3]) == L"round-heading" || std::wstring(argv[3]) == L"competitive-winner" || std::wstring(argv[3]) == L"contribution-team" || std::wstring(argv[3]) == L"result-percentage" ||
            std::wstring(argv[3]) == L"team-setup" || std::wstring(argv[3]) == L"bot-players" || std::wstring(argv[3]) == L"remembered-setup" ||
            std::wstring(argv[3]) == L"remembered-setup-read" || std::wstring(argv[3]) == L"connection-text" ||
            std::wstring(argv[3]) == L"stick-readouts" || std::wstring(argv[3]) == L"team-focus" || std::wstring(argv[3]) == L"team-tune-colours" || std::wstring(argv[3]) == L"weapon-tune-colours" || std::wstring(argv[3]) == L"stage-summary" || std::wstring(argv[3]) == L"menu-labels" ||
            std::wstring(argv[3]) == L"weapon-catalog" || std::wstring(argv[3]) == L"stage-counts" || std::wstring(argv[3]) == L"stage-scan" ||
            std::wstring(argv[3]) == L"menu-text" || std::wstring(argv[3]) == L"menu-motion" ||
            std::wstring(argv[3]) == L"setup-motion" || std::wstring(argv[3]) == L"join-stage-motion" ||
            std::wstring(argv[3]) == L"setup-paper" || std::wstring(argv[3]) == L"join-paper" || std::wstring(argv[3]) == L"team-paper" || std::wstring(argv[3]) == L"weapon-paper" || std::wstring(argv[3]) == L"stage-paper" || std::wstring(argv[3]) == L"home-return-focus" || std::wstring(argv[3]) == L"home-exit" || std::wstring(argv[3]) == L"options-defaults" || (std::wstring(argv[3]) == L"display-trial" || std::wstring(argv[3]) == L"paused-options"))),
            "usage: painter_visual <staged-content> <output> [identity|quiet|drift|landmark|parallax|miniature-terrain|cq-blockout|crossfeed|press-tower|turbulence-blockout|gate-permission|ferry-direction|surface-finish|route-warning|tune-colours|feedback-tune-colours|combat-tune-colours|blocked-shot-tune-colours|idle-drip-tune-colours|respawn-pen-tune-colours|cursor-tune-colours|silhouette-tune-colours|reveal-tune-colours|cursor-material-tune-colours|desktop-tune-colour|power-light-tune-colour|program-tune-colour|program-level-visibility|acting-tune-colours|glass-tune-colour|silhouette-body-tune-colour|scribble-tune-colour|hud-health-tune-colour|hud-track-tune-colour|hud-timer-tune-colour|hud-label-tune-colours|crack-tune-colour|limb-tune-colours|exposed-colour-alpha|duration-text|resolution-text|end-continuation|results-text|session-result|series-score|round-heading|competitive-winner|contribution-team|result-percentage|monitor|countdown|closing|supply|hud|practice-replay|resume|end-menu|pause-menu|results-menu|main-menu|settings-menu|controls-menu|keyboard-jump|rumble|stick-settings|percentage-text|team-setup|bot-players|remembered-setup|remembered-setup-read|connection-text|stick-readouts|team-focus|team-tune-colours|weapon-tune-colours|stage-summary|stage-counts|stage-scan|menu-labels|weapon-catalog|menu-text|menu-motion|setup-motion|join-stage-motion|setup-paper|join-paper|team-paper|weapon-paper|stage-paper|stage-preview-background-tune-colour|hud-closing-materials|hud-closing-shade|hud-paper-tune-colours|home-return-focus|home-exit|options-defaults|display-trial|paused-options] | painter_visual <staged-content> <output> (turbulence-mover|turbulence-shoulder|turbulence-slab) <private-trial.json>");
        const bool turbulence_mover_only = argc == 5 && std::wstring(argv[3]) == L"turbulence-mover";
        const bool turbulence_shoulder_only = argc == 5 && std::wstring(argv[3]) == L"turbulence-shoulder";
        const bool turbulence_slab_only = argc == 5 && std::wstring(argv[3]) == L"turbulence-slab";
        const bool identity_only = argc == 4 && std::wstring(argv[3]) == L"identity";
        const bool quiet_only = argc == 4 && std::wstring(argv[3]) == L"quiet";
        const bool monitor_only = argc == 4 && std::wstring(argv[3]) == L"monitor";
        const bool countdown_only = argc == 4 && std::wstring(argv[3]) == L"countdown";
        const bool closing_only = argc == 4 && std::wstring(argv[3]) == L"closing";
        const bool supply_only = argc == 4 && std::wstring(argv[3]) == L"supply";
        const bool hud_only = argc == 4 && std::wstring(argv[3]) == L"hud";
        const bool practice_replay_only = argc == 4 && std::wstring(argv[3]) == L"practice-replay";
        const bool resume_only = argc == 4 && std::wstring(argv[3]) == L"resume";
        const bool end_menu_only = argc == 4 && std::wstring(argv[3]) == L"end-menu";
        const bool pause_menu_only = argc == 4 && std::wstring(argv[3]) == L"pause-menu";
        const bool results_menu_only = argc == 4 && std::wstring(argv[3]) == L"results-menu";
        const bool main_menu_only = argc == 4 && std::wstring(argv[3]) == L"main-menu";
        const bool weapon_catalog_only = argc == 4 && std::wstring(argv[3]) == L"weapon-catalog";
        const bool menu_text_only = argc == 4 && std::wstring(argv[3]) == L"menu-text";
        const bool menu_motion_only = argc == 4 && std::wstring(argv[3]) == L"menu-motion";
        const bool settings_menu_only = argc == 4 && std::wstring(argv[3]) == L"settings-menu";
        const bool controls_menu_only = argc == 4 && std::wstring(argv[3]) == L"controls-menu";
        const bool keyboard_jump_only = argc == 4 && std::wstring(argv[3]) == L"keyboard-jump";
        const bool rumble_only = argc == 4 && std::wstring(argv[3]) == L"rumble";
        const bool stick_settings_only = argc == 4 && std::wstring(argv[3]) == L"stick-settings";
        const bool remembered_setup_only = argc == 4 && (std::wstring(argv[3]) == L"remembered-setup" ||
            std::wstring(argv[3]) == L"remembered-setup-read");
        const bool bot_players_only = argc == 4 && std::wstring(argv[3]) == L"bot-players";
        const bool connection_text_only = argc == 4 && std::wstring(argv[3]) == L"connection-text";
        const bool stick_readouts_only = argc == 4 && std::wstring(argv[3]) == L"stick-readouts";
        const bool team_focus_only = argc == 4 && std::wstring(argv[3]) == L"team-focus";
        const bool team_tune_colours_only = argc == 4 && std::wstring(argv[3]) == L"team-tune-colours";
        const bool weapon_tune_colours_only = argc == 4 && std::wstring(argv[3]) == L"weapon-tune-colours";
        const bool feedback_tune_colours_only = argc == 4 && std::wstring(argv[3]) == L"feedback-tune-colours";
        const bool combat_tune_colours_only = argc == 4 && std::wstring(argv[3]) == L"combat-tune-colours";
        const bool blocked_tune_colours_only = argc == 4 && std::wstring(argv[3]) == L"blocked-shot-tune-colours";
        const bool idle_drip_tune_colours_only = argc == 4 && std::wstring(argv[3]) == L"idle-drip-tune-colours";
        const bool respawn_pen_tune_colours_only = argc == 4 && std::wstring(argv[3]) == L"respawn-pen-tune-colours";
        const bool cursor_tune_colours_only = argc == 4 && std::wstring(argv[3]) == L"cursor-tune-colours";
        const bool silhouette_tune_colours_only = argc == 4 && std::wstring(argv[3]) == L"silhouette-tune-colours";
        const bool reveal_tune_colours_only = argc == 4 && std::wstring(argv[3]) == L"reveal-tune-colours";
        const bool cursor_material_tune_colours_only = argc == 4 && std::wstring(argv[3]) == L"cursor-material-tune-colours";
        const bool desktop_tune_colour_only = argc == 4 && std::wstring(argv[3]) == L"desktop-tune-colour";
        const bool power_light_tune_colour_only = argc == 4 && std::wstring(argv[3]) == L"power-light-tune-colour";
        const bool program_tune_colour_only = argc == 4 && std::wstring(argv[3]) == L"program-tune-colour";
        const bool program_level_visibility_only = argc == 4 && std::wstring(argv[3]) == L"program-level-visibility";
        const bool acting_tune_colours_only = argc == 4 && std::wstring(argv[3]) == L"acting-tune-colours";
        const bool glass_tune_colour_only = argc == 4 && std::wstring(argv[3]) == L"glass-tune-colour";
        const bool silhouette_body_tune_colour_only = argc == 4 && std::wstring(argv[3]) == L"silhouette-body-tune-colour";
        const bool scribble_tune_colour_only = argc == 4 && std::wstring(argv[3]) == L"scribble-tune-colour";
        const bool hud_health_tune_colour_only = argc == 4 && std::wstring(argv[3]) == L"hud-health-tune-colour";
        const bool hud_track_tune_colour_only = argc == 4 && std::wstring(argv[3]) == L"hud-track-tune-colour";
        const bool hud_timer_tune_colour_only = argc == 4 && std::wstring(argv[3]) == L"hud-timer-tune-colour";
        const bool hud_label_tune_colours_only = argc == 4 && std::wstring(argv[3]) == L"hud-label-tune-colours";
        const bool crack_tune_colour_only = argc == 4 && std::wstring(argv[3]) == L"crack-tune-colour";
        const bool limb_tune_colours_only = argc == 4 && std::wstring(argv[3]) == L"limb-tune-colours";
        const bool exposed_colour_alpha_only = argc == 4 && std::wstring(argv[3]) == L"exposed-colour-alpha";
        const bool duration_text_only = argc == 4 && std::wstring(argv[3]) == L"duration-text";
        const bool resolution_text_only = argc == 4 && std::wstring(argv[3]) == L"resolution-text";
        const bool end_continuation_only = argc == 4 && std::wstring(argv[3]) == L"end-continuation";
        const bool results_text_only = argc == 4 && std::wstring(argv[3]) == L"results-text";
        const bool session_result_only = argc == 4 && std::wstring(argv[3]) == L"session-result";
        const bool series_score_only = argc == 4 && std::wstring(argv[3]) == L"series-score";
        const bool round_heading_only = argc == 4 && std::wstring(argv[3]) == L"round-heading";
        const bool competitive_winner_only = argc == 4 && std::wstring(argv[3]) == L"competitive-winner";
        const bool contribution_team_only = argc == 4 && std::wstring(argv[3]) == L"contribution-team";
        const bool result_percentage_only = argc == 4 && std::wstring(argv[3]) == L"result-percentage";
        const bool menu_labels_only = argc == 4 && std::wstring(argv[3]) == L"menu-labels";
        const bool stage_scan_only = argc == 4 && std::wstring(argv[3]) == L"stage-scan";
        const bool stage_summary_only = argc == 4 && std::wstring(argv[3]) == L"stage-summary";
        const bool hud_closing_materials_only = argc == 4 && std::wstring(argv[3]) == L"hud-closing-materials";
        const bool hud_closing_shade_only = argc == 4 && std::wstring(argv[3]) == L"hud-closing-shade";
        const bool hud_paper_tune_colours_only = argc == 4 && std::wstring(argv[3]) == L"hud-paper-tune-colours";
        const bool stage_preview_background_tune_colour_only = argc == 4 && std::wstring(argv[3]) == L"stage-preview-background-tune-colour";
#if !COLOURWARS_DEV_TOOLS
        require(argc != 4 || std::wstring(argv[3]) != L"tune-colours", "TuneColoursOnly requires a Debug build");
        require(!team_tune_colours_only, "TeamTuneColoursOnly requires a Debug build");
        require(!weapon_tune_colours_only, "WeaponTuneColoursOnly requires a Debug build");
#endif
#if defined(COLOURWARS_PAINTER_SCRIPTED_PADS)
        require(!stage_scan_only, "StageScanOnly requires the normal gamepad reader");
        require(!result_percentage_only, "ResultPercentageOnly requires the normal gamepad reader");
        require(!contribution_team_only, "ContributionTeamOnly requires the normal gamepad reader");
        require(!competitive_winner_only, "CompetitiveWinnerOnly requires the normal gamepad reader");
        require(!round_heading_only, "RoundHeadingOnly requires the normal gamepad reader");
        require(!series_score_only, "SeriesScoreOnly requires the normal gamepad reader");
        require(!session_result_only, "SessionResultOnly requires the normal gamepad reader");
        require(!results_text_only, "ResultsTextOnly requires the normal gamepad reader");
        require(!end_continuation_only, "EndContinuationOnly requires the normal gamepad reader");
        require(!resolution_text_only, "ResolutionTextOnly requires the normal gamepad reader");
        require(!duration_text_only, "DurationTextOnly requires the normal gamepad reader");
        require(!exposed_colour_alpha_only, "ExposedColourAlphaOnly requires the normal gamepad reader");
        require(!feedback_tune_colours_only, "FeedbackTuneColoursOnly requires the normal gamepad reader");
        require(!combat_tune_colours_only, "CombatTuneColoursOnly requires the normal gamepad reader");
        require(!blocked_tune_colours_only, "BlockedShotTuneColoursOnly requires the normal gamepad reader");
        require(!idle_drip_tune_colours_only, "IdleDripTuneColoursOnly requires the normal gamepad reader");
        require(!respawn_pen_tune_colours_only, "RespawnPenTuneColoursOnly requires the normal gamepad reader");
        require(!cursor_tune_colours_only, "CursorTuneColoursOnly requires the normal gamepad reader");
        require(!silhouette_tune_colours_only, "SilhouetteTuneColoursOnly requires the normal gamepad reader");
        require(!reveal_tune_colours_only, "RevealTuneColoursOnly requires the normal gamepad reader");
        require(!cursor_material_tune_colours_only, "CursorMaterialTuneColoursOnly requires the normal gamepad reader");
        require(!desktop_tune_colour_only, "DesktopTuneColourOnly requires the normal gamepad reader");
        require(!power_light_tune_colour_only, "PowerLightTuneColourOnly requires the normal gamepad reader");
        require(!program_tune_colour_only, "ProgramTuneColourOnly requires the normal gamepad reader");
        require(!program_level_visibility_only, "ProgramLevelVisibilityOnly requires the normal gamepad reader");
        require(!acting_tune_colours_only, "ActingTuneColoursOnly requires the normal gamepad reader");
        require(!glass_tune_colour_only, "GlassTuneColourOnly requires the normal gamepad reader");
        require(!silhouette_body_tune_colour_only, "SilhouetteBodyTuneColourOnly requires the normal gamepad reader");
        require(!scribble_tune_colour_only, "ScribbleTuneColourOnly requires the normal gamepad reader");
        require(!hud_health_tune_colour_only, "HudHealthTuneColourOnly requires the normal gamepad reader");
        require(!hud_track_tune_colour_only, "HudTrackTuneColourOnly requires the normal gamepad reader");
        require(!hud_timer_tune_colour_only, "HudTimerTuneColourOnly requires the normal gamepad reader");
        require(!hud_label_tune_colours_only, "HudLabelTuneColoursOnly requires the normal gamepad reader");
        require(!hud_closing_materials_only, "HudClosingMaterialsOnly requires the normal gamepad reader");
        require(!hud_closing_shade_only, "HudClosingShadeOnly requires the normal gamepad reader");
        require(!hud_paper_tune_colours_only, "HudPaperTuneColoursOnly requires the normal gamepad reader");
        require(!crack_tune_colour_only, "CrackTuneColourOnly requires the normal gamepad reader");
        require(!limb_tune_colours_only, "LimbTuneColoursOnly requires the normal gamepad reader");

        require(argc == 4 && (std::wstring(argv[3]) == L"team-setup" || bot_players_only || remembered_setup_only || connection_text_only || stick_readouts_only || team_focus_only || team_tune_colours_only || weapon_tune_colours_only || stage_summary_only || stage_preview_background_tune_colour_only || menu_labels_only || menu_text_only || keyboard_jump_only || rumble_only ||
            std::wstring(argv[3]) == L"setup-motion" || std::wstring(argv[3]) == L"join-stage-motion" || std::wstring(argv[3]) == L"join-paper" || std::wstring(argv[3]) == L"team-paper" || std::wstring(argv[3]) == L"weapon-paper" || std::wstring(argv[3]) == L"stage-paper" || std::wstring(argv[3]) == L"home-return-focus" || std::wstring(argv[3]) == L"home-exit" || std::wstring(argv[3]) == L"options-defaults" || (std::wstring(argv[3]) == L"display-trial" || std::wstring(argv[3]) == L"paused-options")),
            "The scripted gamepad backend is restricted to focused team/setup/menu-text acceptance");
#else
        require(argc != 4 || (!rumble_only && !keyboard_jump_only && !bot_players_only && !remembered_setup_only && !connection_text_only && !stick_readouts_only && !team_focus_only && !team_tune_colours_only && !weapon_tune_colours_only && !stage_summary_only && !stage_preview_background_tune_colour_only && !menu_labels_only && std::wstring(argv[3]) != L"team-setup" &&
            std::wstring(argv[3]) != L"setup-motion" && std::wstring(argv[3]) != L"join-stage-motion" && std::wstring(argv[3]) != L"join-paper" && std::wstring(argv[3]) != L"team-paper" && std::wstring(argv[3]) != L"weapon-paper" && std::wstring(argv[3]) != L"stage-paper" && std::wstring(argv[3]) != L"home-return-focus" && std::wstring(argv[3]) != L"home-exit" && std::wstring(argv[3]) != L"options-defaults" && std::wstring(argv[3]) != L"display-trial" && std::wstring(argv[3]) != L"paused-options"),
            "Use check-painter.ps1 focused scripted input switches to compile the scripted input seam");
#endif
        std::cout << std::unitbuf;
        const std::filesystem::path content = std::filesystem::absolute(argv[1]);
        const std::filesystem::path output = std::filesystem::absolute(argv[2]);
        std::filesystem::create_directories(output);
        std::ofstream records(output / "painter.csv");
        records << "capture,view,input_slot,painter_pixels,facing_right,tool_angle,respawn_progress,animation_state,action_age,action_frame,tool_pose,pressure,valve,travel_phase,mist_tail_remaining,muzzle_remaining,empty_remaining,recovered_remaining,hurt_remaining,elimination_remaining,death_x,death_y,death_width,death_height,breath,lean,blinking\n";
        Registry<LevelLoadedInfo> levels("Level");
        StageList stages;
        GameTuning tuning((content / "tuning.json").string().c_str());
        Presentation presentation((content / "presentation.json").string().c_str());
        Save saved_preferences;
        saved_preferences.load_save_file();
        require(!saved_preferences.play_settings().aim_marker, "Isolated visual save did not preserve aim_marker=false");
        PlaySettings preferences = saved_preferences.play_settings();
        preferences.music_volume = 0;
        preferences.sfx_volume = 0;
        // Existing comparisons isolate their own visual feature. The marker
        // matrix enables its preference explicitly at identical match state.
        preferences.aim_marker = false;
        ApplicationOptions options;
        options.window_class_name = L"ColourWarsPainterProbe";
        options.window_title = L"ColourWars painter validation";
        options.resolution = (hud_paper_tune_colours_only || hud_closing_shade_only || hud_closing_materials_only || hud_timer_tune_colour_only || hud_label_tune_colours_only || stage_preview_background_tune_colour_only) ? ScreenResolution::s_1920_1080 : ScreenResolution::s_1280_720;
        options.fullscreen = false;
        options.max_threads = 4;
        if (stage_scan_only || turbulence_mover_only || turbulence_shoulder_only || turbulence_slab_only || (argc == 4 && (std::wstring(argv[3]) == L"miniature-terrain" ||
            std::wstring(argv[3]) == L"cq-blockout" || std::wstring(argv[3]) == L"crossfeed" || std::wstring(argv[3]) == L"press-tower" || std::wstring(argv[3]) == L"turbulence-blockout" || std::wstring(argv[3]) == L"gate-permission" || std::wstring(argv[3]) == L"ferry-direction" || std::wstring(argv[3]) == L"surface-finish" || std::wstring(argv[3]) == L"route-warning" || keyboard_jump_only))) { options.view_capacity = 5; }
        options.visible = false;
        Application app(options);
        app.initialize();
        const auto& device = app.renderer()->device_info();
        std::cout << "DEVICE backend=" << device.backend << " api=" << device.api
            << " name=" << device.device_name << " vendor=" << device.vendor_id
            << " device=" << device.device_id << '\n';
        require(device.backend == "d3d11" && device.kind == RenderDeviceKind::hardware,
            "visual evidence requires a D3D11 hardware device");
        app.resource_loader()->register_kind("level", {
            [&](const std::string& directory, const std::string& name, bool) {
                auto info = std::make_unique<LevelLoadedInfo>((directory + name + ".json").c_str());
                info->validate();
                stages.push_back(Stage{ name });
                levels.add(name, std::move(info));
            }, nullptr });
        app.load_manifest((content / "manifest.json").string().c_str());
        // Silence through the groups the game assigns, as main.cpp does.
        sound_groups::assign(*app.audio_resources(), tuning, presentation, levels, stages);
        sound_groups::apply(*app.audio_resources(), preferences);
        tuning.validate_resources(*app.render_resources(), *app.audio_resources());
        presentation.validate_resources(*app.render_resources(), *app.audio_resources());
        LevelBuilder builder(app.viewport_manager(), app.render_resources(),
            app.audio_resources(), &preferences, &levels, &tuning, &presentation,
            app.resolution_manager(), app.thread_pool(), app.partitioner());
        const Canvas canvas(app.render_resources());
        if (result_percentage_only)
        {
            result_percentage_checks(app, levels, tuning, presentation, output);
            return 0;
        }
        if (contribution_team_only)
        {
            contribution_team_checks(app, canvas, tuning, presentation, output);
            return 0;
        }
        if (competitive_winner_only)
        {
            competitive_winner_checks(app, canvas, tuning, presentation, output);
            return 0;
        }
        if (round_heading_only)
        {
            round_heading_checks(app, canvas, tuning, presentation, output);
            return 0;
        }
        if (series_score_only)
        {
            series_score_checks(app, canvas, tuning, presentation, output);
            return 0;
        }
        if (session_result_only)
        {
            session_result_checks(app, canvas, tuning, presentation, output);
            return 0;
        }
        if (results_text_only)
        {
            results_text_checks(app, canvas, tuning, presentation, output);
            return 0;
        }
        if (end_continuation_only)
        {
            end_continuation_checks(app, canvas, tuning, presentation, output);
            return 0;
        }
        if (resolution_text_only)
        {
            resolution_text_checks(app, canvas, tuning, presentation, output);
            return 0;
        }
        if (duration_text_only)
        {
            duration_text_checks(app, canvas, tuning, presentation, output);
#if COLOURWARS_DEV_TOOLS
            duration_target_checks(app, tuning, presentation, content, output);
#endif
            return 0;
        }
        if (exposed_colour_alpha_only)
        {
            home_alpha_checks(app, levels, tuning, presentation, content, output);
            movement_debug_alpha_checks(app, levels, tuning, presentation, output);
#if COLOURWARS_DEV_TOOLS
            debug_console_alpha_checks(app, levels, tuning, presentation, content, output);
#endif
            return 0;
        }
        if (limb_tune_colours_only)
        {
            limb_colour_consumer_checks(app, tuning, presentation, output);
#if COLOURWARS_DEV_TOOLS
            limb_tune_colour_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS limb-tune-colours-only Release direct-consumer-only scope; no developer commands executed\n";
#endif
            quiet_face_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            quiet_face_tune_colour_checks(app, levels, tuning, presentation, content, output);
#endif
            return 0;
        }
        if (crack_tune_colour_only)
        {
            crack_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            crack_tune_colour_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS crack-tune-colour-only Release consumer-only scope; no developer commands executed\n";
#endif
            return 0;
        }
        if (hud_paper_tune_colours_only)
        {
            hud_paper_tune_colour_checks(app, levels, tuning, presentation, content, output);
            return 0;
        }
        if (hud_closing_shade_only)
        {
            hud_closing_shade_checks(app, levels, tuning, presentation, content, output);
            return 0;
        }
        if (hud_closing_materials_only)
        {
            hud_closing_material_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            hud_closing_material_tune_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS hud-closing-materials-only Release consumer scope; no developer commands executed\n";
#endif
            return 0;
        }
        if (stage_preview_background_tune_colour_only)
        {
#if defined(COLOURWARS_PAINTER_SCRIPTED_PADS)
            const auto backing = stage_preview_background_consumer_checks(app, levels, canvas, tuning, presentation, output);
#if COLOURWARS_DEV_TOOLS
            stage_preview_background_command_checks(app, levels, canvas, tuning, presentation, content, output, backing);
#else
            (void)backing;
#endif
#else
            throw std::runtime_error("StagePreviewBackgroundTuneColourOnly requires the scripted gamepad reader");
#endif
            return 0;
        }
        if (hud_label_tune_colours_only)
        {
            hud_label_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            hud_label_tune_colour_checks(app, levels, tuning, presentation, content, output);
#endif
            return 0;
        }
        if (hud_timer_tune_colour_only)
        {
            hud_timer_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            hud_timer_tune_colour_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS normal HUD timer Release consumer scope; no developer commands executed\n";
#endif
            hud_timer_colour_consumer_checks(app, presentation, output, HudTimerSubject::shadow);
#if COLOURWARS_DEV_TOOLS
            hud_timer_tune_colour_checks(app, levels, tuning, presentation, content, output, HudTimerSubject::shadow);
#endif
            hud_closing_text_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            hud_closing_text_tune_colour_checks(app, levels, tuning, presentation, content, output);
#endif
            return 0;
        }
        if (hud_track_tune_colour_only)
        {
            hud_track_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            hud_track_tune_colour_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS hud-track-tune-colour-only Release consumer-only scope; no developer commands executed\n";
#endif
            return 0;
        }
        if (hud_health_tune_colour_only)
        {
            hud_health_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            hud_health_tune_colour_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS hud-health-tune-colour-only Release consumer-only scope; no developer commands executed\n";
#endif
            return 0;
        }
        if (scribble_tune_colour_only)
        {
            scribble_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            scribble_tune_colour_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS scribble-tune-colour-only Release consumer-only scope; no developer commands executed\n";
#endif
            return 0;
        }
        if (silhouette_body_tune_colour_only)
        {
            silhouette_body_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            silhouette_body_tune_colour_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS silhouette-body-tune-colour-only Release consumer-only scope; no developer commands executed\n";
#endif
            return 0;
        }
        if (glass_tune_colour_only)
        {
            glass_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            glass_tune_colour_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS glass-tune-colour-only Release consumer-only scope; no developer commands executed\n";
#endif
            return 0;
        }
        if (acting_tune_colours_only)
        {
            acting_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            acting_tune_colour_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS acting-tune-colours-only Release consumer-only scope; no developer commands executed\n";
#endif
            return 0;
        }
        if (program_level_visibility_only)
        {
            program_level_visibility_checks(app, levels, tuning, presentation, preferences, output);
            return 0;
        }
        if (program_tune_colour_only)
        {
            program_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            program_tune_colour_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS program-tune-colour-only Release consumer-only scope; no developer commands executed\n";
#endif
            return 0;
        }
        if (power_light_tune_colour_only)
        {
            power_light_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            power_light_tune_colour_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS power-light-tune-colour-only Release consumer-only scope; no developer commands executed\n";
#endif
            return 0;
        }
        if (desktop_tune_colour_only)
        {
            desktop_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            desktop_tune_colour_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS desktop-tune-colour-only Release consumer-only scope; no developer commands executed\n";
#endif
            return 0;
        }
        if (cursor_material_tune_colours_only)
        {
            cursor_material_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            cursor_material_tune_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS cursor-material-tune-colours-only Release consumer-only scope; no developer commands executed\n";
#endif
            return 0;
        }
        if (reveal_tune_colours_only)
        {
            reveal_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            reveal_tune_colour_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS reveal-tune-colours-only Release consumer-only scope; no developer commands executed\n";
#endif
            return 0;
        }
        if (silhouette_tune_colours_only)
        {
            silhouette_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            silhouette_tune_colour_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS silhouette-tune-colours-only Release consumer-only scope; no developer commands executed\n";
#endif
            return 0;
        }
        if (cursor_tune_colours_only)
        {
            cursor_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            cursor_tune_colour_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS cursor-tune-colours-only Release consumer-only scope; no developer commands executed\n";
#endif
            return 0;
        }
        if (respawn_pen_tune_colours_only)
        {
            respawn_pen_consumer_checks(app, levels, tuning, presentation, output);
#if COLOURWARS_DEV_TOOLS
            respawn_pen_tune_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS respawn-pen-tune-colours-only Release consumer-only scope; no developer commands executed\n";
#endif
            return 0;
        }
        if (idle_drip_tune_colours_only)
        {
            idle_drip_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            idle_drip_tune_colour_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS idle-drip-tune-colours-only Release consumer-only scope; no developer commands executed\n";
#endif
            return 0;
        }
        if (blocked_tune_colours_only)
        {
            blocked_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            blocked_tune_colour_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS blocked-shot-tune-colours-only Release consumer-only scope; no developer commands executed\n";
#endif
            return 0;
        }
        if (combat_tune_colours_only)
        {
            combat_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            combat_tune_colour_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS combat-tune-colours-only Release consumer-only scope; no developer commands executed\n";
#endif
            return 0;
        }
        if (feedback_tune_colours_only)
        {
            feedback_colour_consumer_checks(app, presentation, output);
#if COLOURWARS_DEV_TOOLS
            feedback_tune_colour_checks(app, levels, tuning, presentation, content, output);
#else
            std::cout << "PASS feedback-tune-colours-only Release consumer-only scope; no developer commands executed\n";
#endif
            return 0;
        }
        if (rumble_only)
        {
#if defined(COLOURWARS_PAINTER_SCRIPTED_PADS)
            rumble_owner_checks(app, levels, canvas, tuning, presentation, content, output);
            rumble_settings_checks(app, canvas, tuning, presentation, output);
#if COLOURWARS_DEV_TOOLS
            debug_background_checks(app, levels, tuning, presentation, content, output);
#endif
#else
            throw std::runtime_error("RumbleOnly requires the scripted gamepad reader seam");
#endif
            return 0;
        }
        if (keyboard_jump_only)
        {
            keyboard_jump_matrix(app, levels, canvas, tuning, presentation, content, output);
#if defined(COLOURWARS_PAINTER_SCRIPTED_PADS)
            keyboard_jump_ownership_matrix(app, levels, canvas, tuning, presentation, content, output);
            pad_buttons_matrix(app, levels, canvas, tuning, presentation, content, output);
#else
            throw std::runtime_error("KeyboardJumpOnly requires the scripted gamepad reader seam");
#endif
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"percentage-text")
        {
            percentage_text_checks(app, canvas, tuning, presentation, output);
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"parallax")
        {
            parallax_matrix(app, levels, tuning, presentation, preferences, output);
            return 0;
        }
        if (stage_scan_only)
        {
            stage_scan_matrix(app, canvas, tuning, presentation, content, output);
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"stage-counts")
        {
            stage_count_matrix(app, levels, canvas, tuning, presentation, output);
            return 0;
        }
#if COLOURWARS_DEV_TOOLS
        if (argc == 4 && std::wstring(argv[3]) == L"tune-colours")
        {
            tune_colour_matrix(app, levels, tuning, presentation, content, output);
            return 0;
        }
#endif
        if (argc == 4 && std::wstring(argv[3]) == L"landmark")
        {
            landmark_matrix(app, levels, builder, tuning, presentation, preferences, content, output);
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"miniature-terrain")
        {
            miniature_terrain_matrix(app, levels, builder, canvas, tuning, presentation, preferences, output);
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"route-warning")
        {
            route_warning_matrix(app, levels, tuning, presentation, preferences, output);
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"gate-permission")
        {
            gate_permission_matrix(app, builder, levels, canvas, tuning, presentation, preferences, output);
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"ferry-direction")
        {
            ferry_direction_matrix(app, levels, canvas, tuning, presentation, preferences, content, output);
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"surface-finish")
        {
            surface_finish_matrix(app, levels, canvas, tuning, presentation, preferences, output);
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"crossfeed")
        {
            crossfeed_matrix(app, levels, canvas, tuning, presentation, preferences, output);
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"press-tower")
        {
            press_tower_matrix(app, levels, tuning, presentation, preferences, output);
            return 0;
        }
        if (turbulence_slab_only)
        {
            turbulence_slab_matrix(app, levels, canvas, tuning, presentation, preferences, std::filesystem::path(argv[4]), output);
            return 0;
        }
        if (turbulence_shoulder_only)
        {
            turbulence_shoulder_matrix(app, levels, canvas, tuning, presentation, preferences, std::filesystem::path(argv[4]), output);
            return 0;
        }
        if (turbulence_mover_only)
        {
            turbulence_mover_matrix(app, levels, canvas, tuning, presentation, preferences, std::filesystem::path(argv[4]), output);
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"turbulence-blockout")
        {
            turbulence_blockout_matrix(app, levels, canvas, tuning, presentation, preferences, output);
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"cq-blockout")
        {
            cq_blockout_matrix(app, levels, builder, canvas, tuning, presentation, preferences, output);
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"drift")
        {
            drift_matrix(app, levels, canvas, builder, preferences, output);
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"setup-paper")
        {
            setup_paper_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS setup-paper-only actual pages; normal reader, keyboard-driven only\n";
            return 0;
        }
#if defined(COLOURWARS_PAINTER_SCRIPTED_PADS)
        if (stage_summary_only)
        {
            stage_summary_matrix(app, levels, canvas, tuning, presentation, output);
            return 0;
        }
        if (menu_labels_only)
        {
            menu_label_matrix(app, levels, canvas, tuning, presentation, output);
            return 0;
        }
#if COLOURWARS_DEV_TOOLS
        if (weapon_tune_colours_only)
        {
            weapon_tune_colour_matrix(app, levels, canvas, tuning, presentation, content, output);
            return 0;
        }
        if (team_tune_colours_only)
        {
            team_tune_colour_matrix(app, levels, canvas, tuning, presentation, content, output);
            return 0;
        }
#endif
        if (team_focus_only)
        {
            team_focus_matrix(app, levels, canvas, tuning, presentation, output);
            return 0;
        }
        if (stick_readouts_only)
        {
            stick_readout_matrix(app, canvas, tuning, presentation, output);
            return 0;
        }
        if (connection_text_only)
        {
            connection_text_matrix(app, levels, canvas, tuning, presentation, output);
            return 0;
        }
        if (remembered_setup_only)
        {
            remembered_setup_matrix(app, levels, canvas, tuning, presentation, content, output,
                std::wstring(argv[3]) == L"remembered-setup-read");
            return 0;
        }
        if (bot_players_only)
        {
            check_playable_bot_setup(app, levels, tuning, presentation, output);
            check_playable_bot_runtime(app, levels, tuning, presentation, content, output);
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"paused-options")
        {
            paused_options_matrix(app, levels, canvas, tuning, presentation, output);
            std::cout << "PASS paused-options-only actual live match/pages; scripted reader, private windowed profile\n";
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"display-trial")
        {
            display_trial_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS display-trial-only actual pages; scripted reader, no physical-pad acceptance\n";
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"options-defaults")
        {
            options_defaults_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS options-defaults-only actual pages; scripted reader, no physical-pad acceptance\n";
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"home-exit")
        {
            home_exit_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS home-exit-only actual pages; scripted reader, no physical-pad acceptance\n";
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"home-return-focus")
        {
            home_return_focus_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS home-return-focus-only actual pages; scripted reader, no physical-pad acceptance\n";
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"stage-paper")
        {
            stage_paper_matrix(app, levels, canvas, tuning, presentation, output);
            std::cout << "PASS stage-paper-only actual pages; scripted reader, no physical-pad acceptance\n";
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"weapon-paper")
        {
            weapon_paper_matrix(app, levels, canvas, tuning, presentation, output);
            std::cout << "PASS weapon-paper-only actual pages; scripted reader, no physical-pad acceptance\n";
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"team-paper")
        {
            team_paper_matrix(app, levels, canvas, tuning, presentation, output);
            std::cout << "PASS team-paper-only actual pages; scripted reader, no physical-pad acceptance\n";
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"join-paper")
        {
            join_paper_matrix(app, levels, canvas, tuning, presentation, output);
            std::cout << "PASS join-paper-only actual pages; scripted reader, no physical-pad acceptance\n";
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"join-stage-motion")
        {
            join_stage_motion_matrix(app, levels, canvas, tuning, presentation, output);
            std::cout << "PASS join-stage-motion-only actual pages; scripted backend, no physical-pad acceptance\n";
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"setup-motion")
        {
            setup_motion_matrix(app, levels, canvas, tuning, presentation, output);
            std::cout << "PASS setup-motion-only actual pages; scripted backend, no physical-pad acceptance\n";
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"team-setup")
        {
            team_setup_matrix(app, levels, tuning, presentation, output);
            std::cout << "PASS team-setup-only actual menu hardware check; scripted backend, no physical-pad acceptance\n";
            return 0;
        }
#endif
        if (menu_motion_only)
        {
            menu_motion_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS menu-motion-only hardware check; other painter matrices were not run\n";
            return 0;
        }
        if (menu_text_only)
        {
            menu_text_matrix(app, levels, canvas, tuning, presentation, output);
            std::cout << "PASS menu-text-only hardware check; other painter matrices were not run\n";
            return 0;
        }
        if (weapon_catalog_only)
        {
            weapon_catalog_matrix(app, levels, tuning, presentation, output);
            std::cout << "PASS weapon-catalog-only hardware check; other painter matrices were not run\n";
            return 0;
        }
        if (stick_settings_only)
        {
            stick_settings_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS stick-settings-only hardware check; other painter matrices were not run\n";
            return 0;
        }
        if (controls_menu_only)
        {
            controls_menu_matrix(app, levels, canvas, tuning, presentation, content, output);
            stick_settings_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS controls-menu-only hardware check; other painter matrices were not run\n";
            return 0;
        }
        if (settings_menu_only)
        {
            settings_menu_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS settings-menu-only hardware check; other painter matrices were not run\n";
            return 0;
        }
        if (main_menu_only)
        {
            main_menu_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS main-menu-only hardware check; other painter matrices were not run\n";
            return 0;
        }
        if (results_menu_only)
        {
            results_menu_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS results-menu-only hardware check; other painter matrices were not run\n";
            return 0;
        }
        if (pause_menu_only)
        {
            pause_menu_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS pause-menu-only hardware check; other painter matrices were not run\n";
            return 0;
        }
        if (end_menu_only)
        {
            end_menu_matrix(app, canvas, tuning, presentation, output);
            series_repick_checks(app, levels, canvas, tuning, presentation, output);
            std::cout << "PASS end-menu-only hardware check; other painter matrices were not run\n";
            return 0;
        }
        if (practice_replay_only)
        {
            practice_replay_checks(app, levels, tuning, presentation, output);
            return 0;
        }
        if (resume_only)
        {
            resume_matrix(app, levels, tuning, presentation, content, output);
            std::cout << "PASS resume-only actual GameLevel hardware check\n";
            return 0;
        }
        if (hud_only)
        {
            hud_matrix(app, builder, canvas, presentation, output);
            std::cout << "PASS HUD-only visual check; other painter matrices were not run\n";
            return 0;
        }
        if (supply_only)
        {
            supply_art_checks(app, builder, canvas, tuning, presentation, preferences, output);
            std::cout << "PASS supply-only visual check; other painter matrices were not run\n";
            return 0;
        }
        if (closing_only)
        {
            closing_matrix(app, builder, canvas, tuning, presentation, preferences, output);
            require(records.good(), "Capture record write failed");
            std::cout << "PASS closing-only visual check; other painter matrices were not run\n";
            return 0;
        }
        if (countdown_only)
        {
            countdown_matrix(app, builder, canvas, tuning, presentation, preferences, output);
            require(records.good(), "Capture record write failed");
            std::cout << "PASS countdown-only visual check; other painter matrices were not run\n";
            return 0;
        }
        if (monitor_only)
        {
            monitor_matrix(app, builder, canvas, tuning, presentation, preferences, content, output);
            require(records.good(), "Capture record write failed");
            std::cout << "PASS monitor-only visual check; other painter matrices were not run\n";
            return 0;
        }
        if (identity_only)
        {
            identity_matrix(app, builder, levels, tuning, presentation, preferences, content, output);
            require(records.good(), "Capture record write failed");
            std::cout << "PASS identity-only visual check; other painter matrices were not run\n";
            return 0;
        }
        if (quiet_only)
        {
            quiet_matrix(app, builder, canvas, presentation, output, records);
            wet_tool_matrix(app, builder, canvas, presentation, preferences, output, records);
            require(records.good(), "Capture record write failed");
            std::cout << "PASS quiet-only visual check; other painter matrices were not run\n";
            return 0;
        }
        // Base captures below keep their normal movement effects. Disable them
        // only while checking action-body frame differences in this matrix.
        drift_matrix(app, levels, canvas, builder, preferences, output);
        presentation.set_movement_feedback_enabled(false);
        for (const bool right : { true, false })
        {
            for (const auto& tool : std::array<std::pair<WeaponType, const char*>, 5>{{
                { WeaponType::sprayer, "sprayer" }, { WeaponType::sniper, "sniper" },
                { WeaponType::roller, "roller" }, { WeaponType::mister, "mister" },
                { WeaponType::bucket, "bucket" } }})
            {
                tool_matrix(app, builder, canvas, output, records, 1,
                    TeamColourPair::blue_orange, right, tool.first, tool.second, presentation);
            }
        }
        for (const TeamColourPair palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            tool_matrix(app, builder, canvas, output, records, 4, palette, true,
                WeaponType::none, "mixed", presentation);
            tool_matrix(app, builder, canvas, output, records, 4, palette, false,
                WeaponType::roller, "roller", presentation);
        }
        for (const bool right : { true, false })
        {
            action_matrix(app, builder, canvas, tuning, presentation, output, records, 1,
                TeamColourPair::blue_orange, right);
        }
        for (const TeamColourPair palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            action_matrix(app, builder, canvas, tuning, presentation, output, records, 4, palette,
                palette != TeamColourPair::blue_orange);
        }
        combat_matrix(app, builder, canvas, tuning, presentation, output, records);
        weapon_feedback_matrix(app, builder, canvas, presentation, output, records);
        blocked_matrix(app, builder, canvas, presentation, output);
        contact_matrix(app, builder, canvas, tuning, presentation, output, records);
        quiet_matrix(app, builder, canvas, presentation, output, records);
        wet_tool_matrix(app, builder, canvas, presentation, preferences, output, records);
        reduced_effects_matrix(app, builder, canvas, presentation, preferences, output, records);
        shake_matrix(app, builder, tuning, presentation, preferences, output);
        identity_matrix(app, builder, levels, tuning, presentation, preferences, content, output);
        monitor_matrix(app, builder, canvas, tuning, presentation, preferences, content, output);
        countdown_matrix(app, builder, canvas, tuning, presentation, preferences, output);
        closing_matrix(app, builder, canvas, tuning, presentation, preferences, output);
        hud_matrix(app, builder, canvas, presentation, output, false);
        supply_art_checks(app, builder, canvas, tuning, presentation, preferences, output);
        {
            // Depletion pays hundreds of shots without wall-clock audio
            // updates. A silent bank keeps this renderer check independent of
            // the hardware one-shot voice pool, as in CPU acceptance.
            AudioResources marker_audio;
            marker_audio.add_sound_bank("paint_exe", SoundBank::silent());
            LevelBuilder marker_builder(app.viewport_manager(), app.render_resources(),
                &marker_audio, &preferences, &levels, &tuning, &presentation,
                app.resolution_manager(), app.thread_pool(), app.partitioner());
            marker_matrix(app, marker_builder, canvas, tuning, preferences, output);
        }
        presentation.set_movement_feedback_enabled(true);
        for (int count : { 1, 4 })
        {
            auto level = build(app, builder, count, TeamColourPair::blue_orange);
            auto inputs = neutral_inputs();
            settle(*level, inputs);
            for (int phase = 0; phase < 8; ++phase)
            {
                const bool right = phase < 4;
                for (auto& input : inputs)
                {
                    input.x_movement = right ? 1.0f : -1.0f;
                }
                for (int tick = 0; tick < 8; ++tick)
                {
                    const float angle = PI * (-0.25f + 0.5f * static_cast<float>(phase % 4) +
                        0.5f * static_cast<float>(tick + 1) / 8.0f);
                    for (auto& input : inputs)
                    {
                        input.shoot_angle = std::atan2(std::sin(angle), std::cos(angle));
                        input.shoot_direction = Vector2F::unit_vec_from_angle(input.shoot_angle);
                    }
                    level->update(acceptance::slot_seats(*level, inputs), dt);
                }
                for (int view = 0; view < count; ++view)
                {
                    require(level->player(view).state() == PlayerState::alive &&
                        level->player(view).facing_right() == right,
                        "scripted moving aim sweep did not preserve a live facing");
                }
                for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
                {
                    if (size.x != 1280 && phase != 1 && phase != 5) { continue; }
                    resize(app, *level, size);
                    const std::string stem = std::to_string(count) + "p-blue_orange-" +
                        (right ? "right" : "left") + "-q" + std::to_string(phase % 4 + 1) + "-" +
                        std::to_string(size.x) + "x" + std::to_string(size.y);
                    capture(app, *level, canvas, output, stem, records);
                }
            }
            resize(app, *level, { 1280, 720 });
            level->suspend();
            capture(app, *level, canvas, output, std::to_string(count) + "p-paused", records);
            level->resume();
        }
        for (const TeamColourPair palette : { TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            auto level = build(app, builder, 4, palette);
            const auto inputs = neutral_inputs();
            settle(*level, inputs);
            resize(app, *level, { 1280, 720 });
            capture(app, *level, canvas, output, std::string("4p-") + TeamColourTools::palette_key(palette), records);
        }
        {
            auto teammates = build(app, builder, 4, TeamColourPair::blue_orange, false, true);
            auto teammate_inputs = neutral_inputs();
            settle(*teammates, teammate_inputs);
            resize(app, *teammates, { 1280, 720 });
            for (bool right : { true, false })
            {
                const float angle = right ? PI * 0.25f : PI * 0.75f;
                for (auto& input : teammate_inputs)
                {
                    input.x_movement = right ? 1.0f : -1.0f;
                    input.shoot_angle = angle;
                    input.shoot_direction = Vector2F::unit_vec_from_angle(angle);
                }
                for (int tick = 0; tick < 8; ++tick) { teammates->update(acceptance::slot_seats(*teammates, teammate_inputs), dt); }
                for (int view = 0; view < 4; ++view)
                {
                    require(teammates->player(view).team() == PlayerTeam::a &&
                        teammates->player(view).state() == PlayerState::alive &&
                        teammates->player(view).facing_right() == right,
                        "same-team roster did not preserve four live teammate facings");
                }
                capture(app, *teammates, canvas, output,
                    right ? "4p-same-team-right" : "4p-same-team-left", records);
            }
        }
        auto level = build(app, builder, 1, TeamColourPair::blue_orange, true);
        const auto inputs = neutral_inputs();
        settle(*level, inputs);
        resize(app, *level, { 1280, 720 });
        capture(app, *level, canvas, output, "1p-roller", records);
        Player& player = const_cast<Player&>(level->player(0));
        const auto before_pose = observation(*level);
        player.set_pose({ Vector2F(4, 0), 0.15f, false, false });
        capture(app, *level, canvas, output, "1p-story-tool-free-squash", records);
        require(before_pose == observation(*level), "story pose changed simulation state");
        player.set_pose({});
        require(level->kill_player(0), "could not request elimination");
        level->update(acceptance::slot_seats(*level, inputs), dt);
        require(player.state() == PlayerState::dead, "elimination did not enter death");
        const Camera whole_camera = player.camera();
        player.set_pose({ Vector2F::ZERO, 0, false, true });
        const Frame early_whole = isolated(&player, player,
            app.viewport_manager()->player_viewport(0), *app.renderer(), canvas);
        player.set_pose({});
        for (int tick = 0; tick < 60 && (player.animation_state() == PlayerAnimationState::elimination ||
            player.combat_feedback().elimination_remaining() > 0.0f); ++tick)
        {
            level->update(acceptance::slot_seats(*level, inputs), dt);
        }
        require(player.animation_state() == PlayerAnimationState::idle &&
            player.combat_feedback().elimination_remaining() == 0.0f,
            "elimination pose/burst did not finish before the outline redraw");
        const Camera live_camera = player.camera();
        player.set_camera(whole_camera);
        player.set_pose({ Vector2F::ZERO, 0, false, true });
        const Frame settled_whole = isolated(&player, player,
            app.viewport_manager()->player_viewport(0), *app.renderer(), canvas);
        require(early_whole.pixels == settled_whole.pixels,
            "whole-dead story figure inherited an elimination frame instead of its idle figure");
        player.set_pose({});
        player.set_camera(live_camera);
        capture(app, *level, canvas, output, "1p-death", records, false);
        const float outline_at = (presentation.respawn_redraw().draw_from +
            presentation.respawn_redraw().fill_at) * 0.5f;
        while (player.state() == PlayerState::dead && player.respawn_progress() < outline_at) { level->update(acceptance::slot_seats(*level, inputs), dt); }
        require(player.state() == PlayerState::dead, "outline phase missed the respawn wait");
        capture(app, *level, canvas, output, "1p-redraw-outline", records);
        player.set_pose({ Vector2F::ZERO, 0.0f, false, true });
        capture(app, *level, canvas, output, "1p-story-dead-whole", records);
        player.set_pose({});
        while (player.state() == PlayerState::dead && player.respawn_progress() < presentation.respawn_redraw().fill_at)
        {
            level->update(acceptance::slot_seats(*level, inputs), dt);
        }
        require(player.state() == PlayerState::dead, "filled phase missed the respawn wait");
        capture(app, *level, canvas, output, "1p-redraw-filled", records);
        for (int tick = 0; tick < 600 && player.state() == PlayerState::dead; ++tick) { level->update(acceptance::slot_seats(*level, inputs), dt); }
        require(player.state() == PlayerState::alive, "redraw did not respawn within 600 ticks");
        capture(app, *level, canvas, output, "1p-respawn", records);
        resume_matrix(app, levels, tuning, presentation, content, output);
        end_menu_matrix(app, canvas, tuning, presentation, output);
        series_repick_checks(app, levels, canvas, tuning, presentation, output);
        pause_menu_matrix(app, canvas, tuning, presentation, output);
        results_menu_matrix(app, canvas, tuning, presentation, output);
        settings_menu_matrix(app, canvas, tuning, presentation, output);
        controls_menu_matrix(app, levels, canvas, tuning, presentation, content, output);
        stick_settings_matrix(app, canvas, tuning, presentation, output);
        menu_text_matrix(app, levels, canvas, tuning, presentation, output);
        weapon_catalog_matrix(app, levels, tuning, presentation, output);
        menu_motion_matrix(app, canvas, tuning, presentation, output);
        main_menu_matrix(app, canvas, tuning, presentation, output);
        require(records.good(), "capture record write failed");
        std::cout << "PASS real builder roster; 1P/4P 720p/1080p/QHD; three palettes; "
            "five action clips and five tool mechanisms, distinct pinned body/tool rasters; "
            "paid fire/hold/release/recovery, hose/bucket travel, roller lift/travel; "
            "moving aim quadrants and both facings; four same-team figures; frozen motion/redraw; "
            "raster culling bounds; death/outline/fill/respawn; tool-free and whole story poses\n";
        return 0;
    });
}
