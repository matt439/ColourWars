cmake_minimum_required(VERSION 3.26)
project(RuntimeStagingExhibit LANGUAGES CXX)

# This fixture consumes existing libraries. It never adds/builds the engine,
# its samples, tests or backends, and introduces no install/package rules.
if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR CMAKE_CONFIGURATION_TYPES)
    message(FATAL_ERROR "This preparation fixture uses single-config x64 MSVC")
endif()
if(NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release)$")
    message(FATAL_ERROR "Choose Debug or Release to match the prebuilt libraries")
endif()
foreach(required LABRADOR_ROOT LABRADOR_LIBRARY MATH_LIBRARY)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "Set ${required} to an existing source/library path")
    endif()
endforeach()
if(NOT EXISTS "${LABRADOR_ROOT}/engine/app/content_root.h"
    OR NOT EXISTS "${LABRADOR_LIBRARY}" OR NOT EXISTS "${MATH_LIBRARY}")
    message(FATAL_ERROR "The engine header and both prebuilt libraries must exist")
endif()

add_executable(RuntimeStagingExhibit runtime_staging.cpp)
target_compile_features(RuntimeStagingExhibit PRIVATE cxx_std_20)
target_compile_options(RuntimeStagingExhibit PRIVATE
    /EHsc /W4 /WX /permissive- /Zc:__cplusplus)
target_compile_definitions(RuntimeStagingExhibit PRIVATE
    "$<$<CONFIG:Debug>:_DEBUG>")
set_target_properties(RuntimeStagingExhibit PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/run")
target_include_directories(RuntimeStagingExhibit PRIVATE "${LABRADOR_ROOT}")
target_link_libraries(RuntimeStagingExhibit PRIVATE
    "${LABRADOR_LIBRARY}" "${MATH_LIBRARY}")

# The caller supplies this finite asset policy. The mechanical part below is
# the workaround being exhibited: it must run even if the exe does not relink.
add_custom_target(stage_runtime_assets ALL
    COMMAND "${CMAKE_COMMAND}"
        "-DCONTENT_DIR=${CMAKE_CURRENT_SOURCE_DIR}/content"
        "-DBUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}"
        "-DSTAGE_DIR=$<TARGET_FILE_DIR:RuntimeStagingExhibit>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/stage.cmake"
    VERBATIM)
add_dependencies(RuntimeStagingExhibit stage_runtime_assets)

# A wrong file at cwd makes executable-relative loading observable. This is
# private generated fixture data, separate from the executable's run/ tree.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/unrelated-cwd/runtime")
file(CONFIGURE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/unrelated-cwd/runtime/required.txt"
    CONTENT "cwd-decoy\n"
    NEWLINE_STYLE UNIX)
