cmake_minimum_required(VERSION 3.16)

project(SeventhCmd
    VERSION 0.1.0
    DESCRIPTION "Lightweight local command intelligence and computer automation"
    LANGUAGES CXX
)

# Project-wide language baseline. Keep compiler extensions disabled for portability.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SEVENTH_CMD_BUILD_TESTS "Build 7th cmd tests" ON)

# Source files are intentionally placeholders at scaffold stage.
# Executable and test targets will be introduced with the first working vertical slice.
if(SEVENTH_CMD_BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()
