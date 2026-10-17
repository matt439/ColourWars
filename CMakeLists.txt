cmake_minimum_required(VERSION 3.26)

project(ColourWars VERSION 0.1.0 LANGUAGES CXX RC)

# Labrador defines the shared labrador_settings target before game/ uses it.
# Its tests, benchmarks and samples are excluded when PROJECT_IS_TOP_LEVEL is false.
add_subdirectory(external/labrador)

add_subdirectory(game)
add_subdirectory(editor)

include(cmake/release_package.cmake)
