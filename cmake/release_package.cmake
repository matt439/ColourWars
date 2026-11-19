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
install(CODE "
    set(BANK_DIR \"${PROJECT_SOURCE_DIR}/game/content/sounds\")
    set(REQUIRE_BANK ON)
    include(\"${CMAKE_CURRENT_LIST_DIR}/check_wave_bank.cmake\")
")
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

foreach(asset IN LISTS COLOURWARS_RUNTIME_ASSETS)
    get_filename_component(directory "${asset}" DIRECTORY)
    if(directory STREQUAL "")
        set(directory ".")
    endif()
    install(FILES "${PROJECT_SOURCE_DIR}/game/content/${asset}"
        DESTINATION "${directory}")
endforeach()
install(FILES "${PROJECT_BINARY_DIR}/generated/build-info.txt" DESTINATION .)

function(colourwars_install_document source installed)
    cmake_path(GET installed PARENT_PATH directory)
    cmake_path(GET installed FILENAME filename)
    if(directory STREQUAL "")
        set(directory ".")
    endif()
    install(FILES "${source}" DESTINATION "${directory}" RENAME "${filename}")
endfunction()
function(colourwars_packaged_document source installed)
    colourwars_install_document("${PROJECT_SOURCE_DIR}/${source}" "${installed}")
endfunction()
function(colourwars_packaged_vcpkg_notice package installed)
    colourwars_install_document("${${package}_DIR}/copyright" "${installed}")
endfunction()
include("${CMAKE_CURRENT_LIST_DIR}/packaged_documents.cmake")

set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_NAME ColourWars)
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "ColourWars-${PROJECT_VERSION}-windows-x64")
set(CPACK_PACKAGE_DIRECTORY "${PROJECT_BINARY_DIR}/packages")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
include(CPack)
