# The optional generated bank is checked at build time, after its recipe.
if(NOT DEFINED BANK_FILE OR NOT DEFINED STAGE_DIR)
    message(FATAL_ERROR "stage_wave_bank.cmake needs BANK_FILE and STAGE_DIR")
endif()
if(EXISTS "${BANK_FILE}")
    file(MAKE_DIRECTORY "${STAGE_DIR}")
    get_filename_component(bank_name "${BANK_FILE}" NAME)
    file(COPY_FILE "${BANK_FILE}" "${STAGE_DIR}/${bank_name}" ONLY_IF_DIFFERENT)
endif()
