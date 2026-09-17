cmake_minimum_required(VERSION 3.26)

# ColourWars — a split-screen paint-shooter, and the first client of the
# Labrador engine. The engine is a submodule at external/labrador; this
# repository is an ordinary application that links it.
project(ColourWars LANGUAGES CXX RC)

# The engine first, and not only because game/ links it: it defines
# labrador_settings, the /W4 /WX interface target that every real target in
# both repositories compiles against. There is deliberately no copy of it
# here. Two definitions across two repositories is exactly the second place
# that target exists to prevent - one of them would drift, and the difference
# would be a warning level (Labrador's ARCHITECTURE.md, The build).
#
# PROJECT_IS_TOP_LEVEL is false for it from here, so this brings in the engine
# library and nothing else: no doctest, no ten test targets, no sample.
add_subdirectory(external/labrador)

add_subdirectory(game)

include(cmake/release_package.cmake)
