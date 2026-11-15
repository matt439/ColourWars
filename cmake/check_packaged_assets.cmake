# Read-only check of the extracted Release package against reviewed source.
cmake_minimum_required(VERSION 3.26)
if(NOT DEFINED PACKAGE_DIR OR NOT IS_DIRECTORY "${PACKAGE_DIR}")
    message(FATAL_ERROR "PACKAGE_DIR must name an extracted package")
endif()
include("${CMAKE_CURRENT_LIST_DIR}/runtime_assets.cmake")
function(require_equal source packaged label)
    if(NOT EXISTS "${packaged}" OR IS_DIRECTORY "${packaged}" OR IS_SYMLINK "${packaged}")
        message(FATAL_ERROR "Packaged asset missing or not a regular file: ${label}")
    endif()
    file(SHA256 "${source}" source_hash)
    file(SHA256 "${packaged}" packaged_hash)
    if(NOT source_hash STREQUAL packaged_hash)
        message(FATAL_ERROR "Packaged asset differs from reviewed source: ${label}")
    endif()
endfunction()
foreach(asset IN LISTS COLOURWARS_RUNTIME_ASSETS)
    require_equal("${CMAKE_CURRENT_LIST_DIR}/../game/content/${asset}"
        "${PACKAGE_DIR}/${asset}" "${asset}")
endforeach()
require_equal("${CMAKE_CURRENT_LIST_DIR}/../docs/HOW-TO-PLAY.txt"
    "${PACKAGE_DIR}/HOW-TO-PLAY.txt" "HOW-TO-PLAY.txt")
# These directories are exclusively runtime content. Reject stale/extra files
# while leaving root DLLs and packaged licences to their existing checks.
foreach(directory IN LISTS COLOURWARS_RUNTIME_DIRECTORIES)
    file(GLOB_RECURSE actual LIST_DIRECTORIES FALSE RELATIVE "${PACKAGE_DIR}"
        "${PACKAGE_DIR}/${directory}/*")
    foreach(asset IN LISTS actual)
        if(NOT asset IN_LIST COLOURWARS_RUNTIME_ASSETS)
            message(FATAL_ERROR "Unreviewed packaged runtime asset: ${asset}")
        endif()
    endforeach()
endforeach()
message(STATUS "Packaged runtime assets and player guide match reviewed source")
