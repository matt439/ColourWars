add_library(colourwars_level_document STATIC level_document.cpp level_json.cpp blockout_aids.cpp)
target_include_directories(colourwars_level_document PUBLIC "${PROJECT_SOURCE_DIR}")
target_include_directories(colourwars_level_document SYSTEM PRIVATE
    "${PROJECT_SOURCE_DIR}/external/rapidjson/include")
target_link_libraries(colourwars_level_document PUBLIC labrador_settings)

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
    "${PROJECT_SOURCE_DIR}/scripts/acceptance/level_editor.cpp")
target_link_libraries(colourwars_editor_acceptance PRIVATE colourwars_level_document)
target_include_directories(colourwars_editor_acceptance SYSTEM PRIVATE
    "${PROJECT_SOURCE_DIR}/external/rapidjson/include")
