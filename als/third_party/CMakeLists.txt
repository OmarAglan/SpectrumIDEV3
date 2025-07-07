# Third-party dependencies for Alif Language Server
cmake_minimum_required(VERSION 3.20)

# Function to download and setup header-only libraries
function(setup_header_only_lib NAME URL COMMIT_HASH INCLUDE_DIR)
    set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}")
    
    if(NOT EXISTS "${LIB_DIR}")
        message(STATUS "Downloading ${NAME}...")
        
        # Create temporary directory for download
        set(TEMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/temp_${NAME}")
        file(MAKE_DIRECTORY "${TEMP_DIR}")
        
        # Download and extract
        set(ARCHIVE_FILE "${TEMP_DIR}/${NAME}.zip")
        file(DOWNLOAD 
            "${URL}/archive/${COMMIT_HASH}.zip"
            "${ARCHIVE_FILE}"
            SHOW_PROGRESS
        )
        
        # Extract archive
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf "${ARCHIVE_FILE}"
            WORKING_DIRECTORY "${TEMP_DIR}"
        )
        
        # Find extracted directory
        file(GLOB EXTRACTED_DIRS "${TEMP_DIR}/*")
        list(GET EXTRACTED_DIRS 0 EXTRACTED_DIR)
        
        # Move to final location
        file(RENAME "${EXTRACTED_DIR}" "${LIB_DIR}")
        
        # Cleanup
        file(REMOVE_RECURSE "${TEMP_DIR}")
        
        message(STATUS "${NAME} downloaded successfully")
    else()
        message(STATUS "${NAME} already exists, skipping download")
    endif()
    
    # Create interface library
    add_library(${NAME}_bundled INTERFACE)
    target_include_directories(${NAME}_bundled INTERFACE "${LIB_DIR}/${INCLUDE_DIR}")
endfunction()

# Setup nlohmann/json if needed
if(NLOHMANN_JSON_BUNDLED)
    setup_header_only_lib(
        nlohmann_json
        "https://github.com/nlohmann/json"
        "v3.11.3"
        "include"
    )
    
    # Add compile definitions for nlohmann/json
    target_compile_definitions(nlohmann_json_bundled INTERFACE
        JSON_USE_IMPLICIT_CONVERSIONS=0
    )
    
    message(STATUS "Using bundled nlohmann/json")
endif()

# Setup spdlog if needed
if(SPDLOG_BUNDLED)
    set(SPDLOG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/spdlog")
    
    if(NOT EXISTS "${SPDLOG_DIR}")
        message(STATUS "Downloading spdlog...")
        
        # Download spdlog
        set(TEMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/temp_spdlog")
        file(MAKE_DIRECTORY "${TEMP_DIR}")
        
        set(ARCHIVE_FILE "${TEMP_DIR}/spdlog.zip")
        file(DOWNLOAD 
            "https://github.com/gabime/spdlog/archive/v1.12.0.zip"
            "${ARCHIVE_FILE}"
            SHOW_PROGRESS
        )
        
        # Extract
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf "${ARCHIVE_FILE}"
            WORKING_DIRECTORY "${TEMP_DIR}"
        )
        
        # Move to final location
        file(GLOB EXTRACTED_DIRS "${TEMP_DIR}/*")
        list(GET EXTRACTED_DIRS 0 EXTRACTED_DIR)
        file(RENAME "${EXTRACTED_DIR}" "${SPDLOG_DIR}")
        
        # Cleanup
        file(REMOVE_RECURSE "${TEMP_DIR}")
        
        message(STATUS "spdlog downloaded successfully")
    endif()
    
    # Add spdlog subdirectory
    set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
    set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
    add_subdirectory("${SPDLOG_DIR}" EXCLUDE_FROM_ALL)

    # Disable warnings-as-errors for spdlog to prevent third-party warnings from breaking build
    if(TARGET spdlog)
        if(MSVC)
            target_compile_options(spdlog PRIVATE /WX-)
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(spdlog PRIVATE -Wno-error)
        endif()
    endif()

    # Create alias
    add_library(spdlog_bundled ALIAS spdlog)
    
    message(STATUS "Using bundled spdlog")
endif()

# Setup fmt if needed
if(FMT_BUNDLED)
    set(FMT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fmt")
    
    if(NOT EXISTS "${FMT_DIR}")
        message(STATUS "Downloading fmt...")
        
        # Download fmt
        set(TEMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/temp_fmt")
        file(MAKE_DIRECTORY "${TEMP_DIR}")
        
        set(ARCHIVE_FILE "${TEMP_DIR}/fmt.zip")
        file(DOWNLOAD 
            "https://github.com/fmtlib/fmt/archive/10.1.1.zip"
            "${ARCHIVE_FILE}"
            SHOW_PROGRESS
        )
        
        # Extract
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf "${ARCHIVE_FILE}"
            WORKING_DIRECTORY "${TEMP_DIR}"
        )
        
        # Move to final location
        file(GLOB EXTRACTED_DIRS "${TEMP_DIR}/*")
        list(GET EXTRACTED_DIRS 0 EXTRACTED_DIR)
        file(RENAME "${EXTRACTED_DIR}" "${FMT_DIR}")
        
        # Cleanup
        file(REMOVE_RECURSE "${TEMP_DIR}")
        
        message(STATUS "fmt downloaded successfully")
    endif()
    
    # Add fmt subdirectory
    set(FMT_DOC OFF CACHE BOOL "" FORCE)
    set(FMT_TEST OFF CACHE BOOL "" FORCE)
    add_subdirectory("${FMT_DIR}" EXCLUDE_FROM_ALL)

    # Disable warnings-as-errors for fmt to prevent third-party warnings from breaking build
    if(TARGET fmt)
        if(MSVC)
            target_compile_options(fmt PRIVATE /WX-)
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(fmt PRIVATE -Wno-error)
        endif()
    endif()

    # Create alias
    add_library(fmt_bundled ALIAS fmt)
    
    message(STATUS "Using bundled fmt")
endif()

# Create a README for third-party directory
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
"# Third-Party Dependencies

This directory contains bundled third-party libraries used by the Alif Language Server.

## Libraries

- **nlohmann/json**: JSON parsing and serialization
- **spdlog**: Fast C++ logging library  
- **fmt**: String formatting library

## Usage

These libraries are automatically downloaded and configured when not found on the system.
The build system will prefer system-installed versions when available.

## Licenses

Each library maintains its own license. Please refer to the individual library directories for license information.
")

message(STATUS "Third-party dependencies configured")
