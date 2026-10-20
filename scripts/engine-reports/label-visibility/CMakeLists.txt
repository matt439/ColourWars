cmake_minimum_required(VERSION 3.24)
project(label_visibility_recording LANGUAGES CXX)

set(LABRADOR_SOURCE_DIR "" CACHE PATH "Labrador checkout root at the revision under review")
if(NOT EXISTS "${LABRADOR_SOURCE_DIR}/engine/render/null/recording.h")
    message(FATAL_ERROR "Set LABRADOR_SOURCE_DIR to the Labrador checkout root")
endif()

# A source-audited candidate closure, not a full engine target and not yet built.
# Keep the renderer/resource implementation entirely null; mixing a prebuilt
# D3D engine library with these files would violate the backend selection seam.
set(render_sources
    render/camera.cpp
    render/colour.cpp
    render/draw_object.cpp
    render/font.cpp
    render/label.cpp
    render/render_resources.cpp
    render/sprite_geometry.cpp
    render/text.cpp
    render/text_object.cpp
    render/texture_data.cpp
    render/viewport.cpp
    render/null/renderer.cpp
    render/null/render_resources.cpp
    render/null/texture_factory.cpp)
set(math_sources
    math/circle.cpp
    math/ericson_math.cpp
    math/inflate.cpp
    math/quad.cpp
    math/rectangle_rotated.cpp
    math/rectanglef.cpp
    math/rectanglei.cpp
    math/scalar.cpp
    math/segment.cpp
    math/triangle.cpp
    math/vector2f.cpp)

set(engine_sources)
foreach(relative_path IN LISTS render_sources math_sources)
    set(source_path "${LABRADOR_SOURCE_DIR}/engine/${relative_path}")
    if(NOT EXISTS "${source_path}")
        message(FATAL_ERROR "Missing checked engine source: ${source_path}")
    endif()
    list(APPEND engine_sources "${source_path}")
endforeach()

add_executable(label_visibility_recording
    label_visibility_recording.cpp
    ${engine_sources})
target_compile_features(label_visibility_recording PRIVATE cxx_std_20)
target_include_directories(label_visibility_recording PRIVATE "${LABRADOR_SOURCE_DIR}")
if(MSVC)
    target_compile_options(label_visibility_recording PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
else()
    target_compile_options(label_visibility_recording PRIVATE -Wall -Wextra -Werror)
endif()
