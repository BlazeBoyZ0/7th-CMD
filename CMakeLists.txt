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
    src/engines/grammar/tokenizer.cpp
    src/engines/grammar/normalizer.cpp
    src/engines/grammar/input_processor.cpp
    src/engines/grammar/grammar_parser.cpp
    src/engines/grammar/ast_generator.cpp
    src/engines/grammar/semantic_analyzer.cpp
    src/engines/grammar/cir_generator.cpp
    src/engines/grammar/grammar_engine.cpp
    src/engines/grammar/grammar_subsystem.cpp
    src/engines/reasoning/bounded_search.cpp
    src/engines/reasoning/confidence_evaluator.cpp
    src/engines/reasoning/contradiction_handler.cpp
    src/engines/reasoning/fact_resolver.cpp
    src/engines/reasoning/reasoning_engine.cpp
    src/engines/reasoning/rule_evaluator.cpp
    src/engines/reasoning/truth_evaluator.cpp
    src/engines/reasoning/uncertainty_handler.cpp
    src/engines/planning/dependency_analyzer.cpp
    src/engines/planning/goal_decomposer.cpp
    src/engines/planning/operation_orderer.cpp
    src/engines/planning/plan_optimizer.cpp
    src/engines/planning/plan_state_manager.cpp
    src/engines/planning/plan_validator.cpp
    src/engines/planning/planning_engine.cpp
    src/engines/planning/task_breakdown.cpp
    src/orchestrator/orchestrator.cpp
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
