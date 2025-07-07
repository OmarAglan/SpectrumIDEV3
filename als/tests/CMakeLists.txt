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

# Test executable
set(TEST_SOURCES
    test_main.cpp
)

# Create test executable
add_executable(als_tests ${TEST_SOURCES})

# Link with main library (when available)
# target_link_libraries(als_tests PRIVATE als_lib)

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
    
    set_tests_properties(BasicTestRunner PROPERTIES
        TIMEOUT 30
        LABELS "basic"
    )
endif()

# Set C++ standard for tests
target_compile_features(als_tests PRIVATE cxx_std_23)

# Add include directories
target_include_directories(als_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Compiler-specific settings
if(MSVC)
    target_compile_options(als_tests PRIVATE
        /W4
        /WX  # Treat warnings as errors
    )
else()
    target_compile_options(als_tests PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror  # Treat warnings as errors
    )
endif()

# Add test data directory
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test_data")

# Copy test configuration files
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/test_config.json.in"
    "${CMAKE_CURRENT_BINARY_DIR}/test_config.json"
    @ONLY
)

message(STATUS "ALS tests configured successfully")
