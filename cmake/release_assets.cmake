# The complete redistributable runtime content list. Never install a content
# directory or the development output directory wholesale: both may contain
# the local commercial wave bank, source art and asset-building tools.
set(COLOURWARS_RELEASE_ASSETS
    manifest.json
    tuning.json
    presentation.json
    fonts/courier_new_bold_16.spritefont
    fonts/gill_sans_mt_bold_24.spritefont
    fonts/gill_sans_mt_bold_36.spritefont
    fonts/gill_sans_mt_bold_48.spritefont
    fonts/gill_sans_mt_bold_72.spritefont
    fonts/gill_sans_mt_bold_144.spritefont
    fonts/gill_sans_ultra_bold_144.spritefont
    textures/sprite_sheet_1.dds
    textures/sprite_sheet_1.json
    textures/menu_paint.dds
    textures/menu_paint.json
    levels/king_of_the_hill.json
    levels/turbulence.json
    levels/close_quarters.json
    sounds/sound_bank_1.json
)

# A manifest edit must be reconciled with the reviewed list above. The bank's
# definition is included for the silent fallback, but its .xwb never is.
set(content "${PROJECT_SOURCE_DIR}/game/content")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${content}/manifest.json")
file(READ "${content}/manifest.json" manifest)
set(expected manifest.json tuning.json presentation.json)
string(JSON group_count LENGTH "${manifest}" assets)
math(EXPR last_group "${group_count} - 1")
foreach(group RANGE 0 ${last_group})
    string(JSON kind GET "${manifest}" assets ${group} kind)
    string(JSON directory GET "${manifest}" assets ${group} directory)
    string(REGEX REPLACE "^\\./" "" directory "${directory}")
    if(kind STREQUAL "font")
        set(extensions spritefont)
    elseif(kind STREQUAL "sprite_sheet")
        set(extensions json dds)
    elseif(kind STREQUAL "level")
        set(extensions json)
    elseif(kind STREQUAL "sound_bank")
        string(JSON optional GET "${manifest}" assets ${group} optional)
        if(NOT optional)
            message(FATAL_ERROR "Release packages require optional sound banks")
        endif()
        set(extensions json)
    else()
        message(FATAL_ERROR "Review release assets for new manifest kind: ${kind}")
    endif()
    string(JSON name_count LENGTH "${manifest}" assets ${group} names)
    math(EXPR last_name "${name_count} - 1")
    foreach(index RANGE 0 ${last_name})
        string(JSON name GET "${manifest}" assets ${group} names ${index})
        foreach(extension IN LISTS extensions)
            list(APPEND expected "${directory}${name}.${extension}")
        endforeach()
    endforeach()
endforeach()
list(SORT expected)
set(actual "${COLOURWARS_RELEASE_ASSETS}")
list(SORT actual)
if(NOT actual STREQUAL expected)
    message(FATAL_ERROR "Manifest and cmake/release_assets.cmake disagree. Review the release asset list.")
endif()
foreach(asset IN LISTS COLOURWARS_RELEASE_ASSETS)
    if(NOT EXISTS "${content}/${asset}")
        message(FATAL_ERROR "Missing release asset: ${asset}")
    endif()
endforeach()
