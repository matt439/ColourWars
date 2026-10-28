add_library(colourwars_level_document STATIC level_document.cpp level_json.cpp horizontal_authoring.cpp slab_authoring.cpp blockout_aids.cpp
    content_catalog.cpp stage_registration.cpp user_stage_save.cpp)
target_include_directories(colourwars_level_document PUBLIC "${PROJECT_SOURCE_DIR}")
target_include_directories(colourwars_level_document SYSTEM PRIVATE
    "${PROJECT_SOURCE_DIR}/external/rapidjson/include")
target_link_libraries(colourwars_level_document PUBLIC labrador_settings)

# Explicit opt-in source registration; never staged or installed with the game.
add_executable(ColourWarsRegisterStage EXCLUDE_FROM_ALL register_stage_main.cpp)
target_link_libraries(ColourWarsRegisterStage PRIVATE colourwars_level_document)

add_executable(ColourWarsEditor WIN32
    main.cpp
    editor_canvas.cpp
    editor_window.cpp)
target_link_libraries(ColourWarsEditor PRIVATE colourwars_level_document
    comctl32 comdlg32 shell32 gdi32 user32)
target_compile_definitions(ColourWarsEditor PRIVATE
    COLOURWARS_CONTENT_DIR=L"${PROJECT_SOURCE_DIR}/game/content"
    COLOURWARS_EDITOR_DIR=L"${PROJECT_SOURCE_DIR}/editor")

# Consumer checks are explicit, like scripts/acceptance's game checks; no CTest.
add_executable(colourwars_editor_acceptance EXCLUDE_FROM_ALL
    "${PROJECT_SOURCE_DIR}/scripts/acceptance/level_editor.cpp"
    "${PROJECT_SOURCE_DIR}/scripts/acceptance/level_editor/fixtures.cpp"
    "${PROJECT_SOURCE_DIR}/scripts/acceptance/level_editor/content_checks.cpp"
    "${PROJECT_SOURCE_DIR}/scripts/acceptance/level_editor/format_checks.cpp"
    "${PROJECT_SOURCE_DIR}/scripts/acceptance/level_editor/field_checks.cpp"
    "${PROJECT_SOURCE_DIR}/scripts/acceptance/level_editor/motion_checks.cpp"
    "${PROJECT_SOURCE_DIR}/scripts/acceptance/level_editor/drive_checks.cpp"
    "${PROJECT_SOURCE_DIR}/scripts/acceptance/level_editor/gate_checks.cpp"
    "${PROJECT_SOURCE_DIR}/scripts/acceptance/level_editor/slab_checks.cpp"
    "${PROJECT_SOURCE_DIR}/scripts/acceptance/level_editor/editing_checks.cpp"
    "${PROJECT_SOURCE_DIR}/scripts/acceptance/level_editor/save_guard_checks.cpp"
    "${PROJECT_SOURCE_DIR}/scripts/acceptance/level_editor/registration_checks.cpp"
    "${PROJECT_SOURCE_DIR}/scripts/acceptance/level_editor/user_stage_checks.cpp")
target_link_libraries(colourwars_editor_acceptance PRIVATE colourwars_level_document)
target_include_directories(colourwars_editor_acceptance SYSTEM PRIVATE
    "${PROJECT_SOURCE_DIR}/external/rapidjson/include")
