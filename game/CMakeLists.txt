# Every game source except the entry point and the two platform adapters. The
# executable and the acceptance probes link this one library, so a probe sees
# the game exactly as the executable does without reaching into its objects.
add_library(colourwars_game STATIC
    ai/bot_random.cpp
    ai/match_bot.cpp
    ai/match_bot_ascent.cpp
    ai/match_bot_ballistics.cpp
    ai/match_bot_driver.cpp
    ai/match_bot_navigation.cpp
    ai/match_bot_ramp.cpp
    carried/animation_object.cpp
    carried/camera_tools.cpp
    carried/moving_object.cpp
    carried/sound_bank_object.cpp
    carried/text_drop_shadow.cpp
    carried/ui_text_drop_shadow.cpp
    action_bindings.cpp
    input_bindings.cpp
    player_profiles.cpp
    content_validation.cpp
    content_value.cpp
    controls_text.cpp
    directory_consts.cpp
    drawable_text.cpp
    fatal_report.cpp
    game_data.cpp
    last_match_setup.cpp
    match_records.cpp
    match_setup.cpp
    presentation.cpp
    presentation_menu_text.cpp
    presentation_read_match.cpp
    presentation_read_menu.cpp
    presentation_validation.cpp
    presentation_validation_assets.cpp
    presentation_validation_catalog.cpp
    presentation_validation_menu.cpp
    presentation_validation_metrics.cpp
    presentation_validation_results.cpp
    presentation_validation_roster.cpp
    presentation_validation_setup.cpp
    presentation_validation_stage.cpp
    presentation_validation_text.cpp
    round_tape.cpp
    rumble.cpp
    save.cpp
    save_profiles.cpp
    series_stage_rule.cpp
    session.cpp
    sound_groups.cpp
    stage_loading.cpp
    stage_scan_diagnostics.cpp
    presentation_validation_stage_scan.cpp
    text_wrap.cpp
    tuning.cpp
    tuning_validation.cpp
    tunnelling_limits.cpp

    debug/bind_commands.cpp
    debug/bot.cpp
    debug/bot_driver.cpp
    debug/debug_background.cpp
    debug/debug_chord.cpp
    debug/debug_clock.cpp
    debug/debug_command.cpp
    debug/debug_binds.cpp
    debug/debug_console_state.cpp
    debug/debug_console_view.cpp
    debug/debug_director.cpp
    debug/debug_director_hooks.cpp
    debug/debug_director_impl.cpp
    debug/debug_files.cpp
    debug/debug_keys.cpp
    debug/debug_log.cpp
    debug/debug_perf.cpp
    debug/debug_menu_model.cpp
    debug/debug_menu_page.cpp
    debug/debug_overlay_draw.cpp
    debug/debug_registry.cpp
    debug/debug_screenshot.cpp
    debug/debug_script.cpp
    debug/debug_script_runner.cpp
    debug/debug_status_text.cpp
    debug/debug_text_field.cpp
    debug/debug_tokenizer.cpp
    debug/debug_variable.cpp
    debug/debug_wait.cpp
    debug/game_commands.cpp
    debug/json_spans.cpp
    debug/match_fingerprint.cpp
    debug/match_plan.cpp
    debug/player_commands.cpp
    debug/replay.cpp
    debug/replay_commands.cpp
    debug/script_commands.cpp
    debug/tune_commands.cpp
    debug/tune_document.cpp
    debug/tune_scope.cpp
    debug/tune_target.cpp
    debug/view_commands.cpp

    objects/blocked_shot_feedback.cpp
    objects/collider.cpp
    objects/collision_object_type.cpp
    objects/combat_feedback.cpp
    objects/airbrush_shadow.cpp
    objects/drifting_visual.cpp
    objects/crude_drawing.cpp
    objects/countdown_art.cpp
    objects/reveal_art.cpp
    objects/debug_text.cpp
    objects/figure_acting.cpp
    objects/fixed_slab_geometry.cpp
    objects/fixed_slab_paintable.cpp
    objects/fixed_slab_shadow.cpp
    objects/interface_gameplay.cpp
    objects/level.cpp
    objects/level_horizontal_motion.cpp
    objects/level_audio.cpp
    objects/level_dev_tools.cpp
    objects/level_draw.cpp
    objects/level_builder.cpp
    objects/level_end_info.cpp
    objects/level_loaded_info.cpp
    objects/level_object_builder.cpp
    objects/level_intro.cpp
    objects/level_opening.cpp
    objects/level_practice.cpp
    objects/monitor.cpp
    objects/movement_audio.cpp
    objects/movement_feedback.cpp
    objects/paint_audio.cpp
    objects/paint_tile.cpp
    objects/player.cpp
    objects/player_figure.cpp
    objects/player_badge.cpp
    objects/teammate_marker.cpp
    objects/player_builder.cpp
    objects/player_camera.cpp
    objects/view_shake.cpp
    objects/player_input.cpp
    objects/projectile.cpp
    objects/projectile_contacts.cpp
    objects/projectile_ball.cpp
    objects/projectile_builder.cpp
    objects/projectile_jet.cpp
    objects/projectile_mist.cpp
    objects/projectile_rolling.cpp
    objects/projectile_spray.cpp
    objects/respawn_redraw.cpp
    objects/screen_cursor.cpp
    objects/silhouette.cpp
    objects/stage.cpp
    objects/structure.cpp
    objects/structure_paintable.cpp
    objects/horizontal_motion.cpp
    objects/translating_structure_paintable.cpp
    objects/team_gate_structure_paintable.cpp
    objects/translated_airbrush_shadow.cpp
    objects/terrain_material.cpp
    objects/terrain_material_drawings.cpp
    objects/team_colour.cpp
    objects/weapon.cpp
    objects/weapon_motion.cpp
    objects/weapon_feedback.cpp
    objects/idle_drip.cpp
    objects/weapon_bucket.cpp
    objects/weapon_builder.cpp
    objects/weapon_details.cpp
    objects/weapon_roller.cpp
    objects/weapon_sniper.cpp

    states/connection_audio.cpp
    states/display_confirmation.cpp
    states/display_settings_trial.cpp
    states/resume_audio.cpp
    states/screen_music.cpp
    states/results_audio.cpp
    states/resume_countdown.cpp
    states/controls_settings_menu.cpp
    states/keyboard_jump_capture.cpp
    states/keyboard_jump_menu.cpp
    states/pad_buttons_menu.cpp
    states/profiles_menu.cpp
    states/stick_settings_menu.cpp
    states/end_menu.cpp
    states/game_states.cpp
    states/game_level_dev_tools.cpp
    states/game_level_replay.cpp
    states/game_level_rumble.cpp
    states/main_menu.cpp
    states/main_menu_credits.cpp
    states/main_menu_exit.cpp
    states/main_menu_options.cpp
    states/main_menu_players.cpp
    states/main_menu_stage_select.cpp
    states/main_menu_team_select.cpp
    states/main_menu_weapon_select.cpp
    states/menu_context.cpp
    states/user_stage_refusals_menu.cpp
    states/menu_focus_motion.cpp
    states/menu_content_validation.cpp
    states/menu_input.cpp
    states/menu_page.cpp
    states/menu_panel.cpp
    states/option_row.cpp
    states/pause_menu.cpp
    states/player_label.cpp
    states/prompt_bar.cpp
    states/results_menu.cpp
    states/settings_panel_page.cpp
    states/stage_preview.cpp
    states/wrapped_text_drop_shadow.cpp
)
# Resolve game includes from this repository's root.
target_include_directories(colourwars_game PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

# save.cpp uses RapidJSON to write saves; Labrador does not expose it.
# SYSTEM keeps third-party headers outside the game's /W4 /WX checks.
target_include_directories(colourwars_game SYSTEM PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/rapidjson/include"
)
target_link_libraries(colourwars_game
    PUBLIC
        LabradorEngine
        comctl32
    PRIVATE
        labrador_settings
)
target_precompile_headers(colourwars_game PRIVATE pch.h)

# The platform adapters are libraries of their own, so a probe chooses each by
# linking it or supplying its own definitions: make_rumble_sink() for the motor
# adapter, DisplayAwake's default constructor for the display-idle request.
# Each implements a seam the game library declares, so it builds against that
# library's headers and precompiled header; the game library never names them.
add_library(colourwars_rumble_xinput STATIC rumble_xinput.cpp)
target_link_libraries(colourwars_rumble_xinput PRIVATE Xinput)
add_library(colourwars_display_awake STATIC display_awake.cpp)
foreach(adapter IN ITEMS colourwars_rumble_xinput colourwars_display_awake)
    target_link_libraries(${adapter} PRIVATE colourwars_game labrador_settings)
    target_precompile_headers(${adapter} REUSE_FROM colourwars_game)
endforeach()

add_executable(ColourWars WIN32
    main.cpp
    resource.rc
    settings.manifest
)
target_link_libraries(ColourWars PRIVATE
    colourwars_game
    colourwars_rumble_xinput
    colourwars_display_awake
    labrador_settings
)
target_precompile_headers(ColourWars REUSE_FROM colourwars_game)

add_custom_target(colourwars_build_info
    COMMAND "${CMAKE_COMMAND}"
        "-DSOURCE_DIR=${PROJECT_SOURCE_DIR}"
        "-DVERSION=${PROJECT_VERSION}"
        "-DOUTPUT_DIR=${PROJECT_BINARY_DIR}/generated"
        "-DCONFIG=$<CONFIG>"
        "-DRENDER_BACKEND=${LABRADOR_RENDER_BACKEND}"
        "-DAUDIO_BACKEND=${LABRADOR_AUDIO_BACKEND}"
        -P "${PROJECT_SOURCE_DIR}/cmake/write_build_info.cmake"
    BYPRODUCTS "${PROJECT_BINARY_DIR}/generated/game/build_info.h"
        "${PROJECT_BINARY_DIR}/generated/build-info.txt"
    VERBATIM
)
add_dependencies(colourwars_game colourwars_build_info)

# The Debug developer console's `reload` reads tuning and presentation from the
# source tree, so edits apply without a rebuild, `tune.save` writes edits back
# to it, and `exec` finds the shipped review scripts in scripts/debug. Generated
# at configure time and included only by Debug-only translation units
# (game/debug/game_commands.cpp, game/debug/tune_commands.cpp,
# game/debug/script_commands.cpp); Release never includes it, so its executable
# carries no source path.
file(CONFIGURE OUTPUT "${PROJECT_BINARY_DIR}/generated/game/source_paths.h"
    CONTENT "#pragma once

// Generated by game/CMakeLists.txt. Debug-only: see game/debug/game_commands.cpp,
// game/debug/tune_commands.cpp and game/debug/script_commands.cpp.
namespace source_paths
{
	inline constexpr char content_directory[] = R\"colourwars(${CMAKE_CURRENT_SOURCE_DIR}/content)colourwars\";
	inline constexpr char scripts_directory[] = R\"colourwars(${PROJECT_SOURCE_DIR}/scripts/debug)colourwars\";
}
"
    @ONLY)
# resource.rc stamps the executable's VERSIONINFO from project(VERSION) - the
# same number as CPACK_PACKAGE_VERSION. Version only, written at configure
# time: the revision stays in build_info.h and build-info.txt, so a commit does
# not recompile the resource.
file(CONFIGURE OUTPUT "${PROJECT_BINARY_DIR}/generated/game/version.rc.h"
    CONTENT "#pragma once

// Generated by game/CMakeLists.txt from project(VERSION); read by game/resource.rc.
#define COLOURWARS_VERSION_NUMBERS ${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0
#define COLOURWARS_VERSION_STRING \"${PROJECT_VERSION}\"
"
    @ONLY)
target_include_directories(colourwars_game PUBLIC "${PROJECT_BINARY_DIR}/generated")

# Stage the same reviewed assets as Release, pruning stale files only inside
# the build's four owned content directories. Root DLLs and other outputs stay.
# Run every build so asset-only edits/removals need no executable relink.
add_custom_target(colourwars_content ALL
    COMMAND "${CMAKE_COMMAND}"
        "-DCONTENT_DIR=${CMAKE_CURRENT_SOURCE_DIR}/content"
        "-DBUILD_DIR=${PROJECT_BINARY_DIR}"
        "-DSTAGE_DIR=$<TARGET_FILE_DIR:ColourWars>"
        -P "${PROJECT_SOURCE_DIR}/cmake/stage_runtime_assets.cmake"
    VERBATIM
)
add_dependencies(ColourWars colourwars_content)
