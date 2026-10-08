cmake_minimum_required(VERSION 3.26)

project(ColourWars LANGUAGES CXX RC)

# Labrador defines the shared labrador_settings target before game/ uses it.
# Its tests, benchmarks and samples are excluded when PROJECT_IS_TOP_LEVEL is false.
add_subdirectory(external/labrador)

add_subdirectory(game)

include(cmake/release_package.cmake)
