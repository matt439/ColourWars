# xaudio2redist first: directxtk's config references Microsoft::XAudio2Redist
# but does not find it itself.
find_package(xaudio2redist CONFIG REQUIRED)
find_package(directxtk CONFIG REQUIRED)

# --- ArtAttackGame — the paint-shooter, first client of the engine --------
#
# An ordinary C++ application that links two static libraries. There is no
# project file format and no code generation: this file is the whole of it
# (ARCHITECTURE.md, A game project).
add_executable(ArtAttackGame WIN32
    game_data.cpp
    main.cpp
    save.cpp

    objects/debug_text.cpp
    objects/interface_gameplay.cpp
    objects/level.cpp
    objects/level_builder.cpp
    objects/level_end_info.cpp
    objects/level_loaded_info.cpp
    objects/level_object_builder.cpp
    objects/paint_tile.cpp
    objects/player.cpp
    objects/player_builder.cpp
    objects/player_input.cpp
    objects/projectile.cpp
    objects/projectile_ball.cpp
    objects/projectile_builder.cpp
    objects/projectile_jet.cpp
    objects/projectile_mist.cpp
    objects/projectile_rolling.cpp
    objects/projectile_spray.cpp
    objects/structure.cpp
    objects/structure_paintable.cpp
    objects/team_colour.cpp
    objects/weapon.cpp
    objects/weapon_bucket.cpp
    objects/weapon_builder.cpp
    objects/weapon_details.cpp
    objects/weapon_mister.cpp
    objects/weapon_roller.cpp
    objects/weapon_sniper.cpp
    objects/weapon_sprayer.cpp

    states/end_menu.cpp
    states/end_menu_data.cpp
    states/game_states.cpp
    states/main_menu.cpp
    states/main_menu_data.cpp
    states/menu_data.cpp
    states/menu_input.cpp
    states/menu_level_settings.cpp
    states/menu_page.cpp
    states/pause_menu.cpp
    states/pause_menu_data.cpp
    states/results_menu.cpp
    states/results_menu_data.cpp

    resource.rc
    settings.manifest
)
target_include_directories(ArtAttackGame PRIVATE
    "${CMAKE_SOURCE_DIR}/external/rapidjson/include"
)
target_link_libraries(ArtAttackGame PRIVATE
    ArtAttackEngine
    Microsoft::DirectXTK
    artattack_settings
)
target_precompile_headers(ArtAttackGame PRIVATE pch.h)

# The game resolves content relative to its working directory
# (directory_consts.h), so mirror content/ beside the built exe and it runs
# from anywhere. The manifest sits at the top of it and names the rest.
#
# Its own target, not a POST_BUILD step on ArtAttackGame: a POST_BUILD only
# runs when the exe relinks, so editing or adding an asset without touching a
# source file never reached the build directory - the build said "no work to
# do" and ran against stale content. That includes the manifest itself, which
# is edited to add an asset and never triggers a relink at all. This runs every
# build; the if_different copies skip what has not changed.
add_custom_target(artattack_content ALL)
add_custom_command(TARGET artattack_content POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/content/manifest.json"
        "$<TARGET_FILE_DIR:ArtAttackGame>/manifest.json"
    VERBATIM
)
foreach(content_dir textures levels fonts sounds)
    add_custom_command(TARGET artattack_content POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/content/${content_dir}"
            "$<TARGET_FILE_DIR:ArtAttackGame>/${content_dir}"
        VERBATIM
    )
endforeach()
add_dependencies(ArtAttackGame artattack_content)

add_custom_command(TARGET ArtAttackGame POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        "-Dsrc=${CMAKE_CURRENT_SOURCE_DIR}/content/save"
        "-Ddst_parent=$<TARGET_FILE_DIR:ArtAttackGame>"
        -P "${CMAKE_SOURCE_DIR}/cmake/seed_save.cmake"
    VERBATIM
)
