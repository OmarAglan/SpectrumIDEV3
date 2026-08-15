if(NOT DEFINED QALAM_ROOT)
    message(FATAL_ERROR "QALAM_ROOT is required")
endif()

set(qalam_resource_file "${QALAM_ROOT}/qalam/resources.qrc")
if(NOT EXISTS "${qalam_resource_file}")
    message(FATAL_ERROR "Missing Qalam resource manifest: ${qalam_resource_file}")
endif()

file(READ "${qalam_resource_file}" qalam_resources)
file(GLOB_RECURSE qalam_icon_consumers LIST_DIRECTORIES false
    "${QALAM_ROOT}/source/*.cpp"
    "${QALAM_ROOT}/source/*.h"
    "${QALAM_ROOT}/qalam/*.cpp"
    "${QALAM_ROOT}/qalam/*.h")

set(qalam_icon_references)
foreach(source IN LISTS qalam_icon_consumers)
    file(READ "${source}" contents)
    string(REGEX MATCHALL
        ":/icons/resources/[A-Za-z0-9_.-]+"
        source_icon_references
        "${contents}")
    list(APPEND qalam_icon_references ${source_icon_references})
endforeach()
list(REMOVE_DUPLICATES qalam_icon_references)

foreach(reference IN LISTS qalam_icon_references)
    string(REPLACE ":/icons/" "" relative_path "${reference}")
    if(NOT EXISTS "${QALAM_ROOT}/qalam/${relative_path}")
        message(FATAL_ERROR
            "Qalam icon reference has no asset: ${reference}")
    endif()
    string(FIND "${qalam_resources}" "<file>${relative_path}</file>" registration_index)
    if(registration_index EQUAL -1)
        message(FATAL_ERROR
            "Qalam icon reference is not registered in resources.qrc: ${reference}")
    endif()
endforeach()

foreach(required_icon IN ITEMS replace.svg replace-all.svg stop.svg trash.svg)
    if(NOT EXISTS "${QALAM_ROOT}/qalam/resources/${required_icon}")
        message(FATAL_ERROR "Missing Qalam action icon: ${required_icon}")
    endif()
    string(FIND
        "${qalam_resources}"
        "<file>resources/${required_icon}</file>"
        registration_index)
    if(registration_index EQUAL -1)
        message(FATAL_ERROR
            "Qalam action icon is not registered: ${required_icon}")
    endif()
endforeach()

list(LENGTH qalam_icon_references qalam_icon_reference_count)
message(STATUS
    "Verified ${qalam_icon_reference_count} Qalam icon references and required action assets")
