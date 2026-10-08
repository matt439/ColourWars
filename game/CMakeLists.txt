# xaudio2redist first: directxtk's config references Microsoft::XAudio2Redist
# but does not find it itself.
find_package(xaudio2redist CONFIG REQUIRED)
find_package(directxtk CONFIG REQUIRED)

add_executable(ColourWars WIN32
    directory_consts.cpp
    drawable_text.cpp
    game_data.cpp
    main.cpp
    presentation.cpp
    save.cpp
    session.cpp
    tuning.cpp

    objects/collision_object_type.cpp
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
    objects/weapon_roller.cpp
    objects/weapon_sniper.cpp

    states/controls_settings_menu.cpp
    states/end_menu.cpp
    states/game_states.cpp
    states/main_menu.cpp
    states/menu_context.cpp
    states/menu_input.cpp
    states/menu_level_settings.cpp
    states/menu_page.cpp
    states/option_row.cpp
    states/pause_menu.cpp
    states/prompt_bar.cpp
    states/results_menu.cpp

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
target_include_directories(ColourWars PRIVATE "${PROJECT_BINARY_DIR}/generated")

# Stage content beside the executable, where the game resolves it.
# Run every build so asset-only edits are copied without an executable relink;
# if_different skips unchanged files.
add_custom_target(colourwars_content ALL)

# Build the optional wave bank before staging it. Stage it explicitly because
# the configure-time glob cannot see a bank generated for the first time.
add_custom_command(TARGET colourwars_content PRE_BUILD
    COMMAND "${CMAKE_COMMAND}"
        "-DBANK_DIR=${CMAKE_CURRENT_SOURCE_DIR}/content/sounds"
        -DBANK_NAME=sound_bank_1
        -P "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/build_wave_bank.cmake"
    COMMAND "${CMAKE_COMMAND}"
        "-DBANK_FILE=${CMAKE_CURRENT_SOURCE_DIR}/content/sounds/sound_bank_1.xwb"
        "-DSTAGE_DIR=$<TARGET_FILE_DIR:ColourWars>/sounds"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/stage_wave_bank.cmake"
    VERBATIM
)
foreach(content_file manifest.json tuning.json presentation.json)
    add_custom_command(TARGET colourwars_content POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/content/${content_file}"
            "$<TARGET_FILE_DIR:ColourWars>/${content_file}"
        VERBATIM
    )
endforeach()
# Enumerate entries to exclude source waves and asset-building tools (see NOTICE).
# CONFIGURE_DEPENDS detects new top-level entries; subdirectories are copied whole.
foreach(content_dir textures levels fonts sounds)
    file(GLOB content_entries CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/content/${content_dir}/*")
    add_custom_command(TARGET colourwars_content POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_FILE_DIR:ColourWars>/${content_dir}"
        VERBATIM
    )
    foreach(content_entry IN LISTS content_entries)
        get_filename_component(entry_name "${content_entry}" NAME)
        # Source waves/tools stay behind; the generated bank is staged above.
        if("${entry_name}" STREQUAL "sound_bank_1"
            OR "${entry_name}" STREQUAL "sound_bank_1.xwb"
            OR "${entry_name}" MATCHES "\\.exe$")
            continue()
        endif()
        if(IS_DIRECTORY "${content_entry}")
            set(copy_command copy_directory_if_different)
        else()
            set(copy_command copy_if_different)
        endif()
        add_custom_command(TARGET colourwars_content POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E ${copy_command}
                "${content_entry}"
                "$<TARGET_FILE_DIR:ColourWars>/${content_dir}/${entry_name}"
            VERBATIM
        )
    endforeach()
endforeach()
add_dependencies(ColourWars colourwars_content)
