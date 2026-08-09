# Build a sound bank's .xwb from its .wav sources, when they are there.
#
# WHY THIS EXISTS RATHER THAN A COMMITTED .xwb. The paint-shooter's bank is
# 154 MB built and 148 MB in sources, which is past what GitHub accepts for a
# single file - and, more decisively, four of its waves are commercial music
# tracks that cannot be redistributed under this repository's licence. So the
# repository ships the recipe: the bank's definition, the tool, and this.
#
# WHAT A FRESH CLONE GETS. Nothing, and that is handled rather than broken:
# the manifest marks the bank optional, so the loader substitutes a silent one
# and says so on stderr (engine/assets/resource_loader.cpp). The game runs.
#
# Run with:
#   cmake -DBANK_DIR=<dir> -DBANK_NAME=<name> -P cmake/build_wave_bank.cmake
#
# It is idempotent and quiet: if the output is newer than every input, or the
# sources are not there, it does nothing.

if(NOT DEFINED BANK_DIR OR NOT DEFINED BANK_NAME)
    message(FATAL_ERROR "build_wave_bank.cmake needs BANK_DIR and BANK_NAME")
endif()

set(definition "${BANK_DIR}/${BANK_NAME}.json")
set(sources "${BANK_DIR}/${BANK_NAME}")
set(output "${BANK_DIR}/${BANK_NAME}.xwb")
set(tool "${sources}/XWBTool.exe")

if(NOT EXISTS "${definition}")
    return()
endif()

# The definition is the authority on which waves the bank holds, because it is
# the file the engine reads afterwards to resolve names. Taking the list from
# anywhere else would let the two drift, and a bank built from a different list
# than the one the game resolves against fails at the first play.
file(READ "${definition}" definition_text)
string(JSON wave_count ERROR_VARIABLE json_error LENGTH "${definition_text}" waves)
if(json_error)
    message(WARNING "wave bank: '${definition}' has no 'waves' array (${json_error})")
    return()
endif()

set(wave_files "")
set(missing "")
math(EXPR last "${wave_count} - 1")
foreach(index RANGE ${last})
    string(JSON wave GET "${definition_text}" waves ${index})
    set(wave_path "${sources}/${wave}.wav")
    if(EXISTS "${wave_path}")
        list(APPEND wave_files "${wave_path}")
    else()
        list(APPEND missing "${wave}.wav")
    endif()
endforeach()

if(missing)
    list(LENGTH missing missing_count)
    # Not a warning. A clone without the audio is the expected case, not a
    # broken one, and a build that shouts about it every time teaches people to
    # ignore the output.
    message(STATUS
        "wave bank: skipping ${BANK_NAME} - ${missing_count} of ${wave_count} "
        "source waves are not in ${sources}. The game will run without audio.")
    return()
endif()

if(NOT EXISTS "${tool}")
    message(STATUS
        "wave bank: skipping ${BANK_NAME} - XWBTool.exe is not in ${sources}.")
    return()
endif()

# Staleness, by hand, because this is a script and not a build graph.
set(rebuild TRUE)
if(EXISTS "${output}")
    set(rebuild FALSE)
    foreach(wave_file IN LISTS wave_files)
        if("${wave_file}" IS_NEWER_THAN "${output}")
            set(rebuild TRUE)
            break()
        endif()
    endforeach()
    if("${definition}" IS_NEWER_THAN "${output}")
        set(rebuild TRUE)
    endif()
endif()

if(NOT rebuild)
    return()
endif()

message(STATUS "wave bank: building ${BANK_NAME}.xwb from ${wave_count} waves")
execute_process(
    COMMAND "${tool}" -f -y -o "${output}" ${wave_files}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE tool_output
    ERROR_VARIABLE tool_output
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "wave bank: XWBTool failed for ${BANK_NAME} (${result})\n${tool_output}")
endif()
