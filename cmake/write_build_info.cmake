# Refresh revision and dirty-state metadata on every build.
find_package(Git QUIET)
function(read_revision directory prefix)
    set(revision "source-archive")
    set(suffix "")
    if(GIT_FOUND AND EXISTS "${directory}/.git")
        execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${directory}" rev-parse HEAD
            OUTPUT_VARIABLE sha OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET RESULT_VARIABLE result)
        if(result EQUAL 0)
            set(revision "${sha}")
            execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${directory}"
                status --porcelain --untracked-files=normal
                OUTPUT_VARIABLE changes ERROR_QUIET RESULT_VARIABLE result)
            if(NOT result EQUAL 0 OR NOT changes STREQUAL "")
                set(suffix "-dirty")
            endif()
        endif()
    endif()
    set(short "${revision}")
    if(NOT revision STREQUAL "source-archive")
        string(SUBSTRING "${revision}" 0 12 short)
    endif()
    set(${prefix}_full "${revision}${suffix}" PARENT_SCOPE)
    set(${prefix}_short "${short}${suffix}" PARENT_SCOPE)
endfunction()

read_revision("${SOURCE_DIR}" game)
read_revision("${SOURCE_DIR}/external/labrador" engine)
set(BUILD_LABEL "Build ${game_short} / ${CONFIG}")
set(BUILD_DETAILS "Version: ${VERSION}\nColourWars: ${game_full}\nLabrador: ${engine_full}\nConfiguration: ${CONFIG}\nRender: ${RENDER_BACKEND}\nAudio: ${AUDIO_BACKEND}\n")
file(MAKE_DIRECTORY "${OUTPUT_DIR}/game")
configure_file("${SOURCE_DIR}/cmake/build_info.h.in"
    "${OUTPUT_DIR}/game/build_info.h" @ONLY)
configure_file("${SOURCE_DIR}/cmake/build-info.txt.in"
    "${OUTPUT_DIR}/build-info.txt" @ONLY)
