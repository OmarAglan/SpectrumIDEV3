if(NOT DEFINED QALAM_ROOT)
    message(FATAL_ERROR "QALAM_ROOT is required")
endif()

file(GLOB_RECURSE qalam_owned_sources LIST_DIRECTORIES false
    "${QALAM_ROOT}/source/*.cpp"
    "${QALAM_ROOT}/source/*.h"
    "${QALAM_ROOT}/qalam/*.cpp"
    "${QALAM_ROOT}/qalam/*.h")

foreach(source IN LISTS qalam_owned_sources)
    get_filename_component(name "${source}" NAME)
    if("${name}" MATCHES "^T[A-Z].*\\.(cpp|h)$")
        message(FATAL_ERROR
            "Legacy T-prefixed Qalam source file is forbidden: ${source}")
    endif()

    file(READ "${source}" contents)
    if("${contents}" MATCHES "(class|struct)[ \t\r\n]+T[A-Z]")
        message(FATAL_ERROR
            "Legacy T-prefixed Qalam type is forbidden: ${source}")
    endif()
    if("${contents}" MATCHES "#[ \t]*include[ \t]+\"T[A-Z]")
        message(FATAL_ERROR
            "Legacy T-prefixed Qalam include is forbidden: ${source}")
    endif()
endforeach()

message(STATUS "Qalam-owned source names use the Qalam prefix")
