cmake_minimum_required(VERSION 3.26)

project(ColourWars VERSION 0.1.1 LANGUAGES CXX RC)

# REL#7: Release keeps symbols, so a crash in a published build can be read.
# Every target, the engine's included, compiles with /Zi as Debug already does;
# ColourWars links them into ColourWars.pdb (game/CMakeLists.txt). The ZIP never
# carries a PDB (cmake/release_package.cmake, scripts/check-release-package.ps1).
if(MSVC)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
        "$<$<CONFIG:Debug,RelWithDebInfo,Release>:ProgramDatabase>")
endif()

# Labrador defines the shared labrador_settings target before game/ uses it.
# Its tests, benchmarks and samples are excluded when PROJECT_IS_TOP_LEVEL is false.
add_subdirectory(external/labrador)

add_subdirectory(game)
add_subdirectory(editor)

include(cmake/release_package.cmake)
