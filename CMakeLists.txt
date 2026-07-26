cmake_minimum_required(VERSION 3.21)
project(QalamIDE VERSION 3.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(cmake/WindowsRuntime.cmake)

if(WIN32)
    enable_language(RC)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core)

option(QALAM_BUILD_TESTS "Build Qalam IDE tests" OFF)
if(QALAM_BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    enable_testing()
endif()

add_subdirectory(source)
add_subdirectory(qalam)

if(QALAM_BUILD_TESTS)
    add_subdirectory(tests)
endif()
