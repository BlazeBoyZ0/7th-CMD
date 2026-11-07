add_executable(7thcmd_tests
    test_core_engines.cpp
    ../src/engines/grammar/ast_generator.cpp
    ../src/engines/grammar/cir_generator.cpp
    ../src/engines/grammar/grammar_engine.cpp
    ../src/engines/grammar/grammar_parser.cpp
    ../src/engines/grammar/input_processor.cpp
    ../src/engines/grammar/normalizer.cpp
    ../src/engines/grammar/semantic_analyzer.cpp
    ../src/engines/grammar/tokenizer.cpp
    ../src/engines/reasoning/truth_evaluator.cpp
    ../src/engines/reasoning/rule_evaluator.cpp
    ../src/engines/planning/dependency_analyzer.cpp
    ../src/engines/planning/goal_decomposer.cpp
    ../src/engines/planning/operation_orderer.cpp
    ../src/engines/planning/plan_optimizer.cpp
    ../src/engines/planning/plan_validator.cpp
    ../src/engines/planning/planning_engine.cpp
    ../src/engines/planning/task_breakdown.cpp
    ../src/core/identifiers.cpp
    ../src/systems/permissions/permission_manager.cpp
    ../src/systems/permissions/permission_checker.cpp
    ../src/systems/permissions/authorization_handler.cpp
)

target_include_directories(7thcmd_tests PRIVATE ../include)
target_compile_features(7thcmd_tests PRIVATE cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(7thcmd_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME 7thcmd_core_engines COMMAND 7thcmd_tests)
