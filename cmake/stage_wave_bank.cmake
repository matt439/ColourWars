# The optional generated bank is checked at build time, after its recipe.
if(NOT DEFINED BANK_FILE OR NOT DEFINED STAGE_DIR)
    message(FATAL_ERROR "stage_wave_bank.cmake needs BANK_FILE and STAGE_DIR")
endif()
get_filename_component(bank_name "${BANK_FILE}" NAME)
set(staged_bank "${STAGE_DIR}/${bank_name}")
if(EXISTS "${BANK_FILE}")
    file(MAKE_DIRECTORY "${STAGE_DIR}")
    file(COPY_FILE "${BANK_FILE}" "${staged_bank}" ONLY_IF_DIFFERENT)
elseif(EXISTS "${staged_bank}")
    # A removed or invalidated source bank must also disappear from the
    # runtime mirror, or an incremental build keeps loading the old audio.
    file(REMOVE "${staged_bank}")
    if(EXISTS "${staged_bank}")
        message(FATAL_ERROR "wave bank: could not remove stale '${staged_bank}'")
    endif()
endif()
