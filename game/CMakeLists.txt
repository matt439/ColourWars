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
    directory_consts.cpp
    drawable_text.cpp
    game_data.cpp
    main.cpp
    presentation.cpp
    save.cpp
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

    states/end_menu.cpp
    states/game_states.cpp
    states/main_menu.cpp
    states/menu_context.cpp
    states/menu_input.cpp
    states/menu_level_settings.cpp
    states/menu_page.cpp
    states/pause_menu.cpp
    states/results_menu.cpp

    resource.rc
    settings.manifest
)
# This repository's own include root, and it has to be stated here.
#
# Includes are written from the repository root (CONVENTIONS), so main.cpp
# opens with #include "game/pch.h". That used to resolve for free: the engine
# published the old repository root as a PUBLIC include directory, and that
# root held game/. After the split the engine publishes its own root, which
# does not - which is the entire point of the split arriving, and it lands
# here because this is the half that moved.
target_include_directories(ArtAttackGame PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

# The game asks for rapidjson itself rather than receiving it through the
# engine, which no longer passes it on. It is asked for on account of the save
# file, which the game *writes*: the engine's assets module only reads, and its
# JsonValue deliberately keeps rapidjson out of a client's translation units,
# so the level files need no copy of their own. game/save.cpp is the one file
# here that includes it. SYSTEM, because a third party's headers are not
# measured against /W4 /WX.
target_include_directories(ArtAttackGame SYSTEM PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/rapidjson/include"
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

# The wave bank, when its sources are there. cmake/build_wave_bank.cmake says
# why the built bank is not committed and what a clone without it gets: the
# manifest marks the bank optional, so the game runs in silence rather than
# throwing on a file it was never going to have. Before the copy below, so a
# bank *rebuilt* this run is the one that gets copied. A bank built here for
# the very first time is staged by the next build instead: the copy list below
# is enumerated before this command runs, so a file that did not exist yet is
# not on it, and the glob puts it there next time round.
add_custom_command(TARGET artattack_content PRE_BUILD
    COMMAND "${CMAKE_COMMAND}"
        "-DBANK_DIR=${CMAKE_CURRENT_SOURCE_DIR}/content/sounds"
        -DBANK_NAME=sound_bank_1
        -P "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/build_wave_bank.cmake"
    VERBATIM
)
foreach(content_file manifest.json tuning.json presentation.json)
    add_custom_command(TARGET artattack_content POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/content/${content_file}"
            "$<TARGET_FILE_DIR:ArtAttackGame>/${content_file}"
        VERBATIM
    )
endforeach()
# The mirror stages what the game loads, which is not all of content/. Two
# kinds of thing are left behind, and cmake -E has no exclude filter, so the
# entries are enumerated one by one rather than copied a directory at a time:
#
#   - sounds/sound_bank_1/, the source waves. 148 MB, and four of them are
#     commercial tracks that are not this repository's to redistribute
#     (NOTICE). What the game opens is sounds/sound_bank_1.xwb *beside* that
#     folder, so nothing inside it is wanted at runtime - and staging it meant
#     that zipping the output directory to hand someone a playable build handed
#     them the tracks with it.
#   - The .exe content tools (XWBTool, MakeSpriteFont, texconv). They make
#     assets; they are not assets.
#
# Both also cost every build, not just a release: this target is a dependency
# of the exe, so everything listed here is byte-compared before the first .cpp
# compiles, on no-op builds too.
#
# CONFIGURE_DEPENDS keeps the property this target exists for. The glob is
# re-checked at the start of every build, so an asset added to content/ still
# reaches the build directory without anyone touching a source file; it just
# re-runs CMake first. Only the top level is globbed - a subdirectory is copied
# whole, so what is inside one needs no enumeration.
foreach(content_dir textures levels fonts sounds)
    file(GLOB content_entries CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/content/${content_dir}/*")
    add_custom_command(TARGET artattack_content POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_FILE_DIR:ArtAttackGame>/${content_dir}"
        VERBATIM
    )
    foreach(content_entry IN LISTS content_entries)
        get_filename_component(entry_name "${content_entry}" NAME)
        # The folder named for the bank, not sound_bank_1.xwb or .json beside it.
        if("${entry_name}" STREQUAL "sound_bank_1"
            OR "${entry_name}" MATCHES "\\.exe$")
            continue()
        endif()
        if(IS_DIRECTORY "${content_entry}")
            set(copy_command copy_directory_if_different)
        else()
            set(copy_command copy_if_different)
        endif()
        add_custom_command(TARGET artattack_content POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E ${copy_command}
                "${content_entry}"
                "$<TARGET_FILE_DIR:ArtAttackGame>/${content_dir}/${entry_name}"
            VERBATIM
        )
    endforeach()
endforeach()
add_dependencies(ArtAttackGame artattack_content)

# Nothing seeds a save. There used to be a POST_BUILD step copying a tracked
# content/save/save_data.json beside the exe when none was there, and that file
# was generated user state that had been committed: it said 1920x1080 while the
# compiled default said 1280x720, so what a fresh build actually started at was
# decided by a file nobody was maintaining. Save now writes its own defaults on
# first run, which it always could.
