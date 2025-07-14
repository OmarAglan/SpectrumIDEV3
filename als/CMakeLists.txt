# Alif Language Server (ALS) - Main CMake Configuration
cmake_minimum_required(VERSION 3.20)

# Project definition
project(AlifLanguageServer 
    VERSION 1.0.0 
    LANGUAGES CXX
    DESCRIPTION "High-performance Language Server for the Alif programming language"
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Global compiler options
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG=1")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG=1")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG=1")

# Platform-specific configurations
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Compiler-specific options
if(MSVC)
    # MSVC specific options
    add_compile_options(
        /W4          # Warning level 4
        /WX          # Treat warnings as errors
        /permissive- # Disable non-conforming code
        /Zc:__cplusplus # Enable correct __cplusplus macro
    )
    # Disable specific MSVC warnings
    add_compile_options(/wd4251 /wd4275) # DLL interface warnings
else()
    # GCC/Clang options
    add_compile_options(
        -Wall -Wextra -Wpedantic -Werror
        -Wno-unused-parameter
        -Wno-missing-field-initializers
    )
    
    # Additional GCC/Clang warnings
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-Wlogical-op -Wduplicated-cond)
    endif()
    
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(-Wno-c++98-compat -Wno-c++98-compat-pedantic)
    endif()
endif()

# Find required packages
find_package(PkgConfig QUIET)

# Try to find nlohmann_json
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found via find_package, trying pkg-config")
    if(PkgConfig_FOUND)
        pkg_check_modules(nlohmann_json QUIET nlohmann_json)
    endif()
    
    if(NOT nlohmann_json_FOUND)
        message(STATUS "nlohmann_json not found, will use bundled version")
        set(NLOHMANN_JSON_BUNDLED TRUE)
    endif()
endif()

# Try to find spdlog
find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
    message(STATUS "spdlog not found via find_package, trying pkg-config")
    if(PkgConfig_FOUND)
        pkg_check_modules(spdlog QUIET spdlog)
    endif()
    
    if(NOT spdlog_FOUND)
        message(STATUS "spdlog not found, will use bundled version")
        set(SPDLOG_BUNDLED TRUE)
    endif()
endif()

# Try to find fmt
find_package(fmt QUIET)
if(NOT fmt_FOUND)
    message(STATUS "fmt not found via find_package, trying pkg-config")
    if(PkgConfig_FOUND)
        pkg_check_modules(fmt QUIET fmt)
    endif()
    
    if(NOT fmt_FOUND)
        message(STATUS "fmt not found, will use bundled version")
        set(FMT_BUNDLED TRUE)
    endif()
endif()

# Handle bundled dependencies
if(NLOHMANN_JSON_BUNDLED OR SPDLOG_BUNDLED OR FMT_BUNDLED)
    add_subdirectory(third_party)
endif()

# Include directories
include_directories(src)
include_directories(include)

# Source files
set(ALS_SOURCES
    src/main.cpp
    src/core/LspServer.cpp
    src/core/JsonRpcProtocol.cpp
    src/core/RequestDispatcher.cpp
    src/core/ThreadPool.cpp
    src/core/ServerConfig.cpp
    src/core/Utils.cpp
    src/logging/Logger.cpp
    src/analysis/Token.cpp
    src/analysis/Lexer.cpp
    src/features/CompletionProvider.cpp
)

set(ALS_HEADERS
    include/als/core/LspServer.h
    include/als/core/ServerConfig.h
    include/als/logging/Logger.h
    src/analysis/Token.h
    src/analysis/Lexer.h
    src/features/CompletionProvider.h
)

# Create main executable
add_executable(als ${ALS_SOURCES} ${ALS_HEADERS})

# Set target properties
set_target_properties(als PROPERTIES
    OUTPUT_NAME "alif-language-server"
    VERSION ${PROJECT_VERSION}
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

# Link libraries
if(nlohmann_json_FOUND)
    target_link_libraries(als PRIVATE nlohmann_json::nlohmann_json)
elseif(NLOHMANN_JSON_BUNDLED)
    target_link_libraries(als PRIVATE nlohmann_json_bundled)
endif()

if(spdlog_FOUND)
    target_link_libraries(als PRIVATE spdlog::spdlog)
elseif(SPDLOG_BUNDLED)
    target_link_libraries(als PRIVATE spdlog_bundled)
endif()

if(fmt_FOUND)
    target_link_libraries(als PRIVATE fmt::fmt)
elseif(FMT_BUNDLED)
    target_link_libraries(als PRIVATE fmt_bundled)
endif()

# Platform-specific libraries
if(WIN32)
    target_link_libraries(als PRIVATE ws2_32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(als PRIVATE pthread)
endif()

# Include directories for target
target_include_directories(als PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Compile definitions
target_compile_definitions(als PRIVATE
    ALS_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    ALS_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    ALS_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    ALS_VERSION_STRING="${PROJECT_VERSION}"
)

# Compiler-specific settings for main target
if(MSVC)
    target_compile_options(als PRIVATE
        /W4                     # High warning level
        /WX                     # Treat warnings as errors
        /permissive-            # Disable non-conforming code
        /Zc:__cplusplus         # Enable correct __cplusplus macro
        /utf-8                  # Source and execution character sets are UTF-8
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(als PRIVATE
        -Wall -Wextra -Wpedantic    # Enable comprehensive warnings
        -Werror                     # Treat warnings as errors
        -Wno-unused-parameter       # Disable unused parameter warnings for now
    )
endif()

# Debug-specific definitions
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(als PRIVATE ALS_DEBUG=1)
endif()

# Enable testing
option(ALS_BUILD_TESTS "Build ALS tests" ON)
if(ALS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Enable benchmarks
option(ALS_BUILD_BENCHMARKS "Build ALS benchmarks" OFF)
if(ALS_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Installation configuration
include(GNUInstallDirs)

install(TARGETS als
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT Runtime
)

install(DIRECTORY include/als
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT Development
)

# Package configuration
set(CPACK_PACKAGE_NAME "AlifLanguageServer")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-performance Language Server for Alif")
set(CPACK_PACKAGE_VENDOR "Spectrum IDE Team")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
elseif(APPLE)
    set(CPACK_GENERATOR "ZIP;DragNDrop")
else()
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
endif()

include(CPack)

# Print configuration summary
message(STATUS "")
message(STATUS "=== Alif Language Server Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Dependencies:")
message(STATUS "  nlohmann_json: ${nlohmann_json_FOUND}")
message(STATUS "  spdlog: ${spdlog_FOUND}")
message(STATUS "  fmt: ${fmt_FOUND}")
message(STATUS "")
message(STATUS "Options:")
message(STATUS "  Build tests: ${ALS_BUILD_TESTS}")
message(STATUS "  Build benchmarks: ${ALS_BUILD_BENCHMARKS}")
message(STATUS "==========================================")
message(STATUS "")
