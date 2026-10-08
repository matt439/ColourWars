# Build a sound bank's .xwb from its .wav sources, when they are there.
# The private audio is not redistributed (see NOTICE). The manifest marks the
# bank optional, so a fresh clone uses the engine's silent fallback.
#
# Run with:
#   cmake -DBANK_DIR=<dir> -DBANK_NAME=<name> -P cmake/build_wave_bank.cmake

if(NOT DEFINED BANK_DIR OR NOT DEFINED BANK_NAME)
    message(FATAL_ERROR "build_wave_bank.cmake needs BANK_DIR and BANK_NAME")
endif()

set(definition "${BANK_DIR}/${BANK_NAME}.json")
set(sources "${BANK_DIR}/${BANK_NAME}")
set(output "${BANK_DIR}/${BANK_NAME}.xwb")
set(tool "${sources}/XWBTool.exe")

# With missing inputs, reuse a bank only if its definition is older.
# Otherwise remove the generated bank; staging removes its old copy too.
function(use_optional_bank reason)
    if(EXISTS "${output}" AND NOT "${definition}" IS_NEWER_THAN "${output}")
        message(STATUS "wave bank: ${reason} Using the existing ${BANK_NAME}.xwb.")
        return()
    endif()
    if(EXISTS "${output}")
        file(REMOVE "${output}")
        if(EXISTS "${output}")
            message(FATAL_ERROR "wave bank: could not remove stale '${output}'")
        endif()
    endif()
    message(STATUS "wave bank: ${reason} The game will run without audio.")
endfunction()

if(NOT EXISTS "${definition}")
    return()
endif()

# Use the runtime definition's wave list so the bank passes startup validation.
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
    # Missing private audio is expected, so report status rather than a warning.
    use_optional_bank("skipping ${BANK_NAME} - ${missing_count} of ${wave_count} source waves are not in ${sources}.")
    return()
endif()

if(NOT EXISTS "${tool}")
    use_optional_bank("skipping ${BANK_NAME} - XWBTool.exe is not in ${sources}.")
    return()
endif()

# This build-time script tracks input timestamps itself.
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
