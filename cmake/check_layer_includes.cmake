# Holds game/ to the layering facts docs/architecture.md lists under
# "Dependency direction" (AGENTS.md, "Architecture"):
#
#   - nothing in objects/, ai/ or carried/ includes a states/ header;
#   - carried/ includes nothing in the game but the precompiled header and
#     its own headers;
#   - ai/ reaches objects/ only through the value headers and the
#     paintable_object.h interface named in AI_OBJECTS_ALLOWED below;
#   - objects/, ai/ and carried/ never name GameData in code (comments may);
#   - outside game/debug/, the developer tools are reached only through the
#     three seam headers and dev_tools.h.
#
# Run with -DGAME_DIR=<repo>/game.
if(NOT DEFINED GAME_DIR)
    message(FATAL_ERROR "check_layer_includes: GAME_DIR is not set")
endif()
if(NOT IS_DIRECTORY "${GAME_DIR}/objects" OR NOT IS_DIRECTORY "${GAME_DIR}/ai"
   OR NOT IS_DIRECTORY "${GAME_DIR}/carried" OR NOT IS_DIRECTORY "${GAME_DIR}/debug")
    message(FATAL_ERROR "check_layer_includes: ${GAME_DIR} is not the game folder")
endif()

# The objects/ headers ai/ may include: values and the paintable interface,
# never Level, Player, Weapon or a gate (docs/architecture.md, BotWorld).
set(AI_OBJECTS_ALLOWED
    aim_facing.h
    paintable_object.h
    player_input_data.h
    player_team.h
    sub_type.h
    weapon_type.h
)
# The game/debug/ headers code outside game/debug/ may include.
set(DEBUG_SEAM
    debug_director.h
    debug_targets.h
    debug_overlay_draw.h
    dev_tools.h
)

set(offenders "")
set(include_regex "^[ \t]*#[ \t]*include[ \t]*\"([^\"]+)\"")

function(report file rule line)
    file(RELATIVE_PATH shown "${GAME_DIR}/.." "${file}")
    string(STRIP "${line}" line)
    set(offenders "${offenders}\n    ${shown}: ${rule}\n      ${line}" PARENT_SCOPE)
endfunction()

# The quoted includes of one file, as "<path>|<line>" entries.
function(quoted_includes file out)
    file(STRINGS "${file}" lines REGEX "${include_regex}")
    set(result "")
    foreach(line IN LISTS lines)
        if(line MATCHES "${include_regex}")
            list(APPEND result "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    set(${out} "${result}" PARENT_SCOPE)
endfunction()

foreach(layer IN ITEMS objects ai carried)
    file(GLOB_RECURSE sources "${GAME_DIR}/${layer}/*.h" "${GAME_DIR}/${layer}/*.cpp")
    foreach(file IN LISTS sources)
        quoted_includes("${file}" includes)
        foreach(path IN LISTS includes)
            set(line "#include \"${path}\"")
            if(path MATCHES "^game/states/")
                report("${file}" "${layer}/ includes a states/ header" "${line}")
            endif()
            if(layer STREQUAL "carried" AND path MATCHES "^game/"
               AND NOT path STREQUAL "game/pch.h"
               AND NOT path MATCHES "^game/carried/")
                report("${file}" "carried/ includes the game beyond pch.h" "${line}")
            endif()
            if(layer STREQUAL "ai" AND path MATCHES "^game/objects/(.+)$")
                list(FIND AI_OBJECTS_ALLOWED "${CMAKE_MATCH_1}" allowed)
                if(allowed EQUAL -1)
                    report("${file}"
                        "ai/ includes an objects/ header outside its value-header allowlist"
                        "${line}")
                endif()
            endif()
        endforeach()

        # GameData in code: drop block and line comments, then look for the
        # whole identifier.
        file(READ "${file}" text)
        string(FIND "${text}" "GameData" any)
        if(any EQUAL -1)
            continue()
        endif()
        set(code "")
        while(TRUE)
            string(FIND "${text}" "/*" open)
            if(open EQUAL -1)
                string(APPEND code "${text}")
                break()
            endif()
            string(SUBSTRING "${text}" 0 ${open} before)
            string(APPEND code "${before}")
            math(EXPR after_open "${open} + 2")
            string(SUBSTRING "${text}" ${after_open} -1 text)
            string(FIND "${text}" "*/" close)
            if(close EQUAL -1)
                break()
            endif()
            math(EXPR after_close "${close} + 2")
            string(SUBSTRING "${text}" ${after_close} -1 text)
        endwhile()
        string(REGEX REPLACE "//[^\n]*" "" code "${code}")
        if(code MATCHES "(^|[^A-Za-z0-9_])GameData([^A-Za-z0-9_]|$)")
            report("${file}" "${layer}/ names GameData" "GameData")
        endif()
    endforeach()
endforeach()

file(GLOB_RECURSE game_sources "${GAME_DIR}/*.h" "${GAME_DIR}/*.cpp")
foreach(file IN LISTS game_sources)
    file(RELATIVE_PATH relative "${GAME_DIR}" "${file}")
    if(relative MATCHES "^debug/")
        continue()
    endif()
    quoted_includes("${file}" includes)
    foreach(path IN LISTS includes)
        if(path MATCHES "^game/debug/(.+)$")
            list(FIND DEBUG_SEAM "${CMAKE_MATCH_1}" allowed)
            if(allowed EQUAL -1)
                report("${file}"
                    "reaches game/debug/ outside its seam headers"
                    "#include \"${path}\"")
            endif()
        endif()
    endforeach()
endforeach()

if(offenders)
    message(FATAL_ERROR
        "game/ breaks a layering rule (docs/architecture.md, \"Dependency "
        "direction\"):${offenders}")
endif()
