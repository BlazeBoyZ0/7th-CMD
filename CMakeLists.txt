cmake_minimum_required(VERSION 3.16)

project(SeventhCmd
    VERSION 0.1.0
    DESCRIPTION "Lightweight local command intelligence and computer automation"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SEVENTH_CMD_BUILD_TESTS "Build 7th cmd tests" ON)

add_executable(7thcmd
    src/main.cpp
    src/core/identifiers.cpp
    src/core/status.cpp
    src/core/subsystem.cpp
    src/core/subsystem_request.cpp
    src/core/subsystem_result.cpp
    src/core/types.cpp
    src/cli/command_line.cpp
    src/cli/input_handler.cpp
    src/cli/terminal_output.cpp
)

target_include_directories(7thcmd PRIVATE include)
target_compile_definitions(7thcmd PRIVATE SEVENTH_CMD_VERSION="${PROJECT_VERSION}")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(7thcmd PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS 7thcmd RUNTIME DESTINATION bin)

if(SEVENTH_CMD_BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()
