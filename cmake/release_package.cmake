# This game's supported release is the ordinary Windows x64 D3D11/XAudio2
# build. Debug CRTs are not redistributable; reject packaging those builds.
install(CODE [[
    if(NOT CMAKE_INSTALL_CONFIG_NAME STREQUAL "Release")
        message(FATAL_ERROR "Package with the x64-release preset; Debug is not redistributable.")
    endif()
]])
# Also check what this build tree actually contains. Passing cpack -C Release
# to a Debug Ninja tree must not relabel a Debug executable as redistributable.
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_CONFIGURATION_TYPES)
    install(CODE "message(FATAL_ERROR \"Configure with the x64-release preset before packaging.\")")
endif()
if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8
    OR NOT LABRADOR_RENDER_BACKEND STREQUAL "d3d11"
    OR NOT LABRADOR_AUDIO_BACKEND STREQUAL "xaudio2")
    install(CODE "message(FATAL_ERROR \"Packaging supports MSVC x64, d3d11 and xaudio2 only.\")")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/release_assets.cmake")
install(TARGETS ColourWars RUNTIME DESTINATION .)
# Follow the imported targets, not whichever DLLs happen to be in game/.
install(FILES $<TARGET_RUNTIME_DLLS:ColourWars> DESTINATION .)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
set(CMAKE_INSTALL_DEBUG_LIBRARIES FALSE)
set(CMAKE_INSTALL_DEBUG_LIBRARIES_ONLY FALSE)
include(InstallRequiredSystemLibraries)
if(MSVC AND NOT CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
    message(FATAL_ERROR "MSVC redistributable runtime DLLs were not found")
endif()
install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION .)

foreach(asset IN LISTS COLOURWARS_RELEASE_ASSETS)
    get_filename_component(directory "${asset}" DIRECTORY)
    if(directory STREQUAL "")
        set(directory ".")
    endif()
    install(FILES "${PROJECT_SOURCE_DIR}/game/content/${asset}"
        DESTINATION "${directory}")
endforeach()
install(FILES "${PROJECT_BINARY_DIR}/generated/build-info.txt"
    "${PROJECT_SOURCE_DIR}/LICENSE" "${PROJECT_SOURCE_DIR}/NOTICE"
    "${PROJECT_SOURCE_DIR}/README.md" DESTINATION .)
install(FILES "${PROJECT_SOURCE_DIR}/external/labrador/LICENSE"
    DESTINATION licenses RENAME Labrador-LICENSE.txt)
install(FILES "${PROJECT_SOURCE_DIR}/external/labrador/NOTICE"
    DESTINATION licenses RENAME Labrador-NOTICE.txt)
install(FILES "${PROJECT_SOURCE_DIR}/external/rapidjson/license.txt"
    DESTINATION licenses RENAME rapidjson-LICENSE.txt)
install(FILES "${directxtk_DIR}/copyright"
    DESTINATION licenses RENAME DirectXTK-LICENSE.txt)
install(FILES "${directxmath_DIR}/copyright"
    DESTINATION licenses RENAME DirectXMath-LICENSE.txt)
install(FILES "${xaudio2redist_DIR}/copyright"
    DESTINATION licenses RENAME XAudio2Redist-LICENSE.txt)

set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_NAME ColourWars)
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_PACKAGE_FILE_NAME "ColourWars-windows-x64")
set(CPACK_PACKAGE_DIRECTORY "${PROJECT_BINARY_DIR}/packages")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
include(CPack)
