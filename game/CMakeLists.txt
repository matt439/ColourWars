# xaudio2redist first: directxtk's config references Microsoft::XAudio2Redist
# but does not find it itself.
find_package(xaudio2redist CONFIG REQUIRED)
find_package(directxtk CONFIG REQUIRED)

add_executable(ColourWars WIN32
    content_validation.cpp
    directory_consts.cpp
    drawable_text.cpp
    game_data.cpp
    main.cpp
    presentation.cpp
    save.cpp
    session.cpp
    tuning.cpp

    debug/bind_commands.cpp
    debug/bot.cpp
    debug/bot_driver.cpp
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

    objects/collision_object_type.cpp
    objects/combat_feedback.cpp
    objects/crude_drawing.cpp
    objects/debug_text.cpp
    objects/figure_acting.cpp
    objects/interface_gameplay.cpp
    objects/level.cpp
    objects/level_builder.cpp
    objects/level_end_info.cpp
    objects/level_loaded_info.cpp
    objects/level_object_builder.cpp
    objects/level_intro.cpp
    objects/level_opening.cpp
    objects/monitor.cpp
    objects/movement_audio.cpp
    objects/movement_feedback.cpp
    objects/paint_audio.cpp
    objects/paint_tile.cpp
    objects/player.cpp
    objects/player_builder.cpp
    objects/player_camera.cpp
    objects/player_input.cpp
    objects/projectile.cpp
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
    objects/team_colour.cpp
    objects/weapon.cpp
    objects/weapon_bucket.cpp
    objects/weapon_builder.cpp
    objects/weapon_details.cpp
    objects/weapon_roller.cpp
    objects/weapon_sniper.cpp

    states/connection_audio.cpp
    states/controls_settings_menu.cpp
    states/end_menu.cpp
    states/game_states.cpp
    states/main_menu.cpp
    states/main_menu_credits.cpp
    states/main_menu_options.cpp
    states/main_menu_players.cpp
    states/main_menu_stage_select.cpp
    states/main_menu_team_select.cpp
    states/main_menu_weapon_select.cpp
    states/menu_context.cpp
    states/menu_input.cpp
    states/menu_level_settings.cpp
    states/menu_page.cpp
    states/menu_panel.cpp
    states/option_row.cpp
    states/pause_menu.cpp
    states/prompt_bar.cpp
    states/results_menu.cpp
    states/stage_preview.cpp

    resource.rc
    settings.manifest
)
# Resolve game includes from this repository's root.
target_include_directories(ColourWars PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

# save.cpp uses RapidJSON to write saves; Labrador does not expose it.
# SYSTEM keeps third-party headers outside the game's /W4 /WX checks.
target_include_directories(ColourWars SYSTEM PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/rapidjson/include"
)
target_link_libraries(ColourWars PRIVATE
    LabradorEngine
    Microsoft::DirectXTK
    labrador_settings
)
target_precompile_headers(ColourWars PRIVATE pch.h)

add_custom_target(colourwars_build_info
    COMMAND "${CMAKE_COMMAND}"
        "-DSOURCE_DIR=${PROJECT_SOURCE_DIR}"
        "-DOUTPUT_DIR=${PROJECT_BINARY_DIR}/generated"
        "-DCONFIG=$<CONFIG>"
        "-DRENDER_BACKEND=${LABRADOR_RENDER_BACKEND}"
        "-DAUDIO_BACKEND=${LABRADOR_AUDIO_BACKEND}"
        -P "${PROJECT_SOURCE_DIR}/cmake/write_build_info.cmake"
    BYPRODUCTS "${PROJECT_BINARY_DIR}/generated/game/build_info.h"
        "${PROJECT_BINARY_DIR}/generated/build-info.txt"
    VERBATIM
)
add_dependencies(ColourWars colourwars_build_info)

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
target_include_directories(ColourWars PRIVATE "${PROJECT_BINARY_DIR}/generated")

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
