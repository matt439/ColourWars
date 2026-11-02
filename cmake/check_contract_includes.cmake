# Holds game/contracts/ to its rule (AGENTS.md, "The engine-free contract
# headers"): a header there includes only the standard library and the other
# headers in that folder, because the editor compiles them without the
# engine's include path. Run with -DCONTRACTS_DIR=<repo>/game/contracts.
if(NOT DEFINED CONTRACTS_DIR)
    message(FATAL_ERROR "check_contract_includes: CONTRACTS_DIR is not set")
endif()

file(GLOB contract_headers "${CONTRACTS_DIR}/*.h")
if(NOT contract_headers)
    message(FATAL_ERROR "check_contract_includes: no headers in ${CONTRACTS_DIR}")
endif()

set(offenders "")
foreach(header IN LISTS contract_headers)
    file(STRINGS "${header}" includes REGEX "^[ \t]*#[ \t]*include")
    foreach(line IN LISTS includes)
        if(line MATCHES "^[ \t]*#[ \t]*include[ \t]*<([a-z_0-9]+)>")
            # A standard-library header: a bare name with no folder or extension.
            continue()
        endif()
        if(line MATCHES "^[ \t]*#[ \t]*include[ \t]*\"game/contracts/([A-Za-z0-9_]+\.h)\"")
            if(EXISTS "${CONTRACTS_DIR}/${CMAKE_MATCH_1}")
                continue()
            endif()
        endif()
        file(RELATIVE_PATH shown "${CONTRACTS_DIR}/../.." "${header}")
        string(STRIP "${line}" line)
        string(APPEND offenders "\n    ${shown}\n      ${line}")
    endforeach()
endforeach()

if(offenders)
    message(FATAL_ERROR
        "game/contracts/ headers may include only the standard library and each "
        "other (AGENTS.md, \"The engine-free contract headers\"):${offenders}")
endif()
