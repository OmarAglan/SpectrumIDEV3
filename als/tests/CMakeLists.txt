# Test configuration for Alif Language Server
cmake_minimum_required(VERSION 3.20)

# Only build tests if requested
if(NOT ALS_BUILD_TESTS)
    return()
endif()

message(STATUS "Configuring ALS tests...")

# Find Google Test (optional)
find_package(GTest QUIET)

if(GTest_FOUND)
    message(STATUS "Google Test found, using full test suite")
    set(USE_GTEST ON)
else()
    message(STATUS "Google Test not found, using basic test runner")
    set(USE_GTEST OFF)
endif()

# Test executables
set(BASIC_TEST_SOURCES
    test_main.cpp
)

set(JSONRPC_TEST_SOURCES
    test_jsonrpc_protocol.cpp
)

set(LSP_TEST_SOURCES
    test_lsp_messages.cpp
)

set(PROTOCOL_TEST_SOURCES
    test_protocol_compliance.cpp
)

set(DEBUG_SOURCES
    debug_stream.cpp
)

# Add source files that tests depend on
set(ALS_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/core/JsonRpcProtocol.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ServerConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Utils.cpp
)

# Create test executables
add_executable(als_tests ${BASIC_TEST_SOURCES})
add_executable(als_jsonrpc_tests ${JSONRPC_TEST_SOURCES} ${ALS_CORE_SOURCES})
add_executable(als_lsp_tests ${LSP_TEST_SOURCES} ${ALS_CORE_SOURCES})
add_executable(als_protocol_tests ${PROTOCOL_TEST_SOURCES} ${ALS_CORE_SOURCES})
add_executable(debug_stream ${DEBUG_SOURCES})

# Function to link libraries to test targets
function(link_test_libraries TARGET_NAME)
    if(nlohmann_json_FOUND)
        target_link_libraries(${TARGET_NAME} PRIVATE nlohmann_json::nlohmann_json)
    elseif(NLOHMANN_JSON_BUNDLED)
        target_link_libraries(${TARGET_NAME} PRIVATE nlohmann_json_bundled)
    endif()

    if(spdlog_FOUND)
        target_link_libraries(${TARGET_NAME} PRIVATE spdlog::spdlog)
    elseif(SPDLOG_BUNDLED)
        target_link_libraries(${TARGET_NAME} PRIVATE spdlog_bundled)
    endif()

    if(fmt_FOUND)
        target_link_libraries(${TARGET_NAME} PRIVATE fmt::fmt)
    elseif(FMT_BUNDLED)
        target_link_libraries(${TARGET_NAME} PRIVATE fmt_bundled)
    endif()
endfunction()

# Link libraries to all test executables
link_test_libraries(als_jsonrpc_tests)
link_test_libraries(als_lsp_tests)
link_test_libraries(als_protocol_tests)

# Configure for Google Test if available
if(USE_GTEST)
    target_link_libraries(als_tests PRIVATE GTest::gtest GTest::gtest_main)
    target_compile_definitions(als_tests PRIVATE USE_GTEST)
    
    # Enable testing
    enable_testing()
    
    # Add test cases
    add_test(NAME BasicTests COMMAND als_tests)
    add_test(NAME ServerConfigTests COMMAND als_tests --gtest_filter="ServerConfigTest.*")
    add_test(NAME LspServerTests COMMAND als_tests --gtest_filter="LspServerTest.*")
    
    # Set test properties
    set_tests_properties(BasicTests PROPERTIES
        TIMEOUT 30
        LABELS "basic"
    )
    
    set_tests_properties(ServerConfigTests PROPERTIES
        TIMEOUT 10
        LABELS "config"
    )
    
    set_tests_properties(LspServerTests PROPERTIES
        TIMEOUT 20
        LABELS "server"
    )
else()
    # Basic test runner without Google Test
    enable_testing()
    add_test(NAME BasicTestRunner COMMAND als_tests)
    add_test(NAME JsonRpcProtocolTests COMMAND als_jsonrpc_tests)
    add_test(NAME LspMessageTests COMMAND als_lsp_tests)
    add_test(NAME ProtocolComplianceTests COMMAND als_protocol_tests)

    set_tests_properties(BasicTestRunner PROPERTIES
        TIMEOUT 30
        LABELS "basic"
    )

    set_tests_properties(JsonRpcProtocolTests PROPERTIES
        TIMEOUT 30
        LABELS "protocol"
    )

    set_tests_properties(LspMessageTests PROPERTIES
        TIMEOUT 30
        LABELS "lsp"
    )

    set_tests_properties(ProtocolComplianceTests PROPERTIES
        TIMEOUT 30
        LABELS "compliance"
    )
endif()

# Function to configure test targets
function(configure_test_target TARGET_NAME)
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_23)

    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    if(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE
            /W4
            /WX  # Treat warnings as errors
        )
    else()
        target_compile_options(${TARGET_NAME} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Werror  # Treat warnings as errors
        )
    endif()
endfunction()

# Configure all test targets
configure_test_target(als_tests)
configure_test_target(als_jsonrpc_tests)
configure_test_target(als_lsp_tests)
configure_test_target(als_protocol_tests)
configure_test_target(debug_stream)

# Add test data directory
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test_data")

# Copy test configuration files
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/test_config.json.in"
    "${CMAKE_CURRENT_BINARY_DIR}/test_config.json"
    @ONLY
)

message(STATUS "ALS tests configured successfully")
