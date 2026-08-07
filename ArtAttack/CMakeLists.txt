# xaudio2redist first: directxtk's config references Microsoft::XAudio2Redist
# but does not find it itself.
find_package(xaudio2redist CONFIG REQUIRED)
find_package(directxtk CONFIG REQUIRED)

# --- ArtAttack — the paint-shooter ---------------------------------------
add_executable(ArtAttack WIN32
    DebugText.cpp
    EndMenu.cpp
    EndMenuData.cpp
    Game.cpp
    GameData.cpp
    GameStates.cpp
    InterfaceGameplay.cpp
    Level.cpp
    LevelBuilder.cpp
    LevelEndInfo.cpp
    LevelLoadedInfo.cpp
    LevelObjectBuilder.cpp
    level_stage.cpp
    Main.cpp
    MainMenu.cpp
    MainMenuData.cpp
    MenuData.cpp
    MenuHighlight.cpp
    MenuInput.cpp
    MenuLevelSettings.cpp
    MenuPage.cpp
    PaintTile.cpp
    PauseMenu.cpp
    PauseMenuData.cpp
    Player.cpp
    PlayerBuilder.cpp
    PlayerInput.cpp
    Projectile.cpp
    ProjectileBall.cpp
    ProjectileBuilder.cpp
    ProjectileJet.cpp
    ProjectileMist.cpp
    ProjectileRolling.cpp
    ProjectileSpray.cpp
    ResultsMenu.cpp
    ResultsMenuData.cpp
    Save.cpp
    Structure.cpp
    StructurePaintable.cpp
    StructureRamp.cpp
    StructureRampLeft.cpp
    StructureRampRight.cpp
    TeamColour.cpp
    Weapon.cpp
    WeaponBucket.cpp
    WeaponBuilder.cpp
    WeaponDetails.cpp
    WeaponMister.cpp
    WeaponRoller.cpp
    WeaponSniper.cpp
    WeaponSprayer.cpp
    resource.rc
    settings.manifest
)
target_include_directories(ArtAttack PRIVATE
    "${CMAKE_SOURCE_DIR}/external/rapidjson/include"
)
target_link_libraries(ArtAttack PRIVATE
    ArtAttackEngine
    Microsoft::DirectXTK
    artattack_settings
)
target_precompile_headers(ArtAttack PRIVATE pch.h)

# The game resolves content relative to its working directory
# (directory_consts.h), so mirror the manifest and the content folders it names
# beside the built exe and it runs from anywhere.
#
# Its own target, not a POST_BUILD step on ArtAttack: a POST_BUILD only runs
# when the exe relinks, so editing or adding an asset without touching a source
# file never reached the build directory - the build said "no work to do" and
# ran against stale content. That now includes the manifest itself, which is
# edited to add an asset and never triggers a relink at all. This runs every
# build; the if_different copies skip what has not changed.
add_custom_target(artattack_content ALL)
add_custom_command(TARGET artattack_content POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/manifest.json"
        "$<TARGET_FILE_DIR:ArtAttack>/manifest.json"
    VERBATIM
)
foreach(content_dir textures levels fonts sounds)
    add_custom_command(TARGET artattack_content POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/${content_dir}"
            "$<TARGET_FILE_DIR:ArtAttack>/${content_dir}"
        VERBATIM
    )
endforeach()
add_dependencies(ArtAttack artattack_content)

add_custom_command(TARGET ArtAttack POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        "-Dsrc=${CMAKE_CURRENT_SOURCE_DIR}/save"
        "-Ddst_parent=$<TARGET_FILE_DIR:ArtAttack>"
        -P "${CMAKE_SOURCE_DIR}/cmake/seed_save.cmake"
    VERBATIM
)
