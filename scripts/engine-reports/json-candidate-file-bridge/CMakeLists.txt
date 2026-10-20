cmake_minimum_required(VERSION 3.24)
project(json_candidate_file_bridge LANGUAGES CXX)

set(LABRADOR_SOURCE_DIR "" CACHE PATH "Labrador checkout root at the revision under review")
set(RAPIDJSON_INCLUDE_DIR "" CACHE PATH "Directory containing rapidjson/document.h")
if(NOT EXISTS "${LABRADOR_SOURCE_DIR}/engine/assets/json.cpp" OR
   NOT EXISTS "${LABRADOR_SOURCE_DIR}/engine/core/file_path.cpp")
    message(FATAL_ERROR "Set LABRADOR_SOURCE_DIR to the Labrador checkout root")
endif()
if(NOT EXISTS "${RAPIDJSON_INCLUDE_DIR}/rapidjson/document.h")
    message(FATAL_ERROR "Set RAPIDJSON_INCLUDE_DIR to a RapidJSON include directory")
endif()

add_executable(json_candidate_file_bridge
    json_candidate_file_bridge.cpp
    "${LABRADOR_SOURCE_DIR}/engine/assets/json.cpp"
    "${LABRADOR_SOURCE_DIR}/engine/core/file_path.cpp")
target_compile_features(json_candidate_file_bridge PRIVATE cxx_std_20)
target_include_directories(json_candidate_file_bridge PRIVATE "${LABRADOR_SOURCE_DIR}")
target_include_directories(json_candidate_file_bridge SYSTEM PRIVATE "${RAPIDJSON_INCLUDE_DIR}")
if(MSVC)
    target_compile_options(json_candidate_file_bridge PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
else()
    target_compile_options(json_candidate_file_bridge PRIVATE -Wall -Wextra -Werror)
endif()
