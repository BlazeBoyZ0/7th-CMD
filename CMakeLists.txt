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
    src/core/constants.cpp
    src/core/errors.cpp
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
    src/engines/grammar/typo_corrector.cpp
    src/engines/grammar/grammar_parser.cpp
    src/engines/grammar/entity_extractor.cpp
    src/engines/grammar/grammar_config.cpp
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
    src/engines/reasoning/rule_loader.cpp
    src/engines/reasoning/reasoning_trace.cpp
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
    src/engines/execution/action_dispatcher.cpp
    src/engines/execution/execution_engine.cpp
    src/engines/execution/execution_tracker.cpp
    src/engines/execution/operation_executor.cpp
    src/systems/knowledge/clarification_manager.cpp
    src/systems/knowledge/context_manager.cpp
    src/systems/knowledge/knowledge_interface.cpp
    src/systems/knowledge/knowledge_retriever.cpp
    src/systems/knowledge/knowledge_system.cpp
    src/systems/knowledge/knowledge_updater.cpp
    src/systems/knowledge/task_context.cpp
    src/systems/information/data_acquisition.cpp
    src/systems/information/data_normalizer.cpp
    src/systems/information/data_parser.cpp
    src/systems/information/information_processor.cpp
    src/systems/information/operation_engine.cpp
    src/systems/information/provider_registry.cpp
    src/systems/information/platform_data_providers.cpp
    src/systems/information/result_builder.cpp
    src/systems/information/type_system.cpp
    src/systems/entity_resolution/ambiguity_detector.cpp
    src/systems/entity_resolution/candidate_discovery.cpp
    src/systems/entity_resolution/context_resolver.cpp
    src/systems/entity_resolution/entity_classifier.cpp
    src/systems/entity_resolution/entity_matcher.cpp
    src/systems/entity_resolution/entity_resolver.cpp
    src/systems/capabilities/capability_definition.cpp
    src/systems/capabilities/capability_registry.cpp
    src/systems/permissions/authorization_handler.cpp
    src/systems/permissions/permission_checker.cpp
    src/systems/permissions/permission_manager.cpp
    src/systems/recovery/error_manager.cpp
    src/systems/recovery/recovery_manager.cpp
    src/systems/verification/result_verifier.cpp
    src/systems/verification/verification_system.cpp
    src/systems/response/response_formatter.cpp
    src/systems/response/response_generator.cpp
    src/platform/linux/applications/linux_applications.cpp
    src/platform/linux/filesystem/linux_filesystem.cpp
    src/platform/linux/linux_platform.cpp
    src/platform/linux/linux_operation_executor.cpp
    src/platform/linux/processes/linux_processes.cpp
    src/platform/linux/system/linux_system.cpp
    src/orchestrator/cancellation_handler.cpp
    src/orchestrator/command_subsystem.cpp
    src/orchestrator/interaction_coordinator.cpp
    src/orchestrator/orchestrator.cpp
    src/orchestrator/pause_resume_manager.cpp
    src/orchestrator/request_manager.cpp
    src/orchestrator/result_handler.cpp
    src/orchestrator/subsystem_dispatcher.cpp
    src/orchestrator/subsystem_router.cpp
    src/orchestrator/task_state_manager.cpp
    src/orchestrator/workflow_controller.cpp
    src/utilities/json_loader.cpp
    src/utilities/logger.cpp
    src/utilities/path_utils.cpp
    src/utilities/string_utils.cpp
)

target_include_directories(7thcmd PRIVATE include)
target_compile_definitions(7thcmd PRIVATE SEVENTH_CMD_VERSION="${PROJECT_VERSION}")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(7thcmd PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS 7thcmd RUNTIME DESTINATION bin)
install(DIRECTORY config/ DESTINATION share/7thcmd/config)

if(SEVENTH_CMD_BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

# Debian package naming keeps the version in the requested v1-0-0 style.
string(REPLACE "." "-" SEVENTH_CMD_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "7thcmd")
set(CPACK_PACKAGE_VENDOR "7th cmd")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "7thcmd-v${SEVENTH_CMD_PACKAGE_VERSION}")
set(CPACK_DEBIAN_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}.deb")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "7th cmd maintainers")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libstdc++6, libgcc-s1")
include(CPack)
