#include "seventh/cli/command_line.hpp"
#include "seventh/cli/input_handler.hpp"
#include "seventh/cli/terminal_output.hpp"
#include "seventh/engines/grammar/grammar_config.hpp"
#include "seventh/engines/grammar/grammar_subsystem.hpp"
#include "seventh/orchestrator/command_subsystem.hpp"
#include "seventh/orchestrator/orchestrator.hpp"
#include "seventh/platform/linux/linux_platform.hpp"
#include "seventh/systems/response/response_generator.hpp"
#include "seventh/utilities/logger.hpp"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

int main(int argc, char* argv[]) {
    const auto result = seventh::cli::parse_command_line(argc, argv);

    switch (result.action) {
    case seventh::cli::CommandLineAction::ShowHelp:
        seventh::cli::print_help();
        return result.exit_code;
    case seventh::cli::CommandLineAction::ShowVersion:
        seventh::cli::print_version();
        return result.exit_code;
    case seventh::cli::CommandLineAction::Invalid:
        return result.exit_code;
    case seventh::cli::CommandLineAction::Start:
        break;
    }

    seventh::utilities::Logger logger;
    auto grammar_config = seventh::grammar::GrammarConfig::defaults();
    std::string config_error;
    bool loaded = false;
    constexpr std::array<const char*, 3> config_directories{
        "config/grammar",
        "/usr/local/share/7thcmd/config/grammar",
        "/usr/share/7thcmd/config/grammar"
    };
    for (const auto* directory : config_directories) {
        if (seventh::grammar::GrammarConfigLoader{}.load_directory(
                directory, grammar_config, config_error)) {
            loaded = true;
            break;
        }
    }
    if (!loaded) {
        logger.write(seventh::utilities::LogLevel::Warning,
                     "Using built-in grammar defaults: " + config_error);
    }

    seventh::cli::print_startup_message();
    std::cout << "Type an instruction, or 'exit' to quit.\n";

    seventh::platform::linux_backend::LinuxPlatform platform;
    seventh::Orchestrator orchestrator;
    const std::vector<std::filesystem::path> packaged_subsystem_profiles{
        "config/subsystem_profiles.json",
        "/usr/local/share/7thcmd/config/subsystem_profiles.json",
        "/usr/share/7thcmd/config/subsystem_profiles.json"
    };
    for (const auto& path : packaged_subsystem_profiles) {
        std::error_code path_error;
        if (!std::filesystem::is_regular_file(path, path_error) || path_error) continue;
        std::string profile_error;
        if (!orchestrator.load_subsystem_profiles(path.string(), profile_error)) {
            logger.write(seventh::utilities::LogLevel::Warning,
                         "Subsystem profiles were not loaded: " + profile_error);
        }
        break;
    }
    if (const char* home = std::getenv("HOME")) {
        const auto user_profiles =
            std::filesystem::path(home) / ".config/7thcmd/subsystem_profiles.json";
        std::error_code path_error;
        if (std::filesystem::is_regular_file(user_profiles, path_error) && !path_error) {
            std::string profile_error;
            if (!orchestrator.load_subsystem_profiles(
                    user_profiles.string(), profile_error, true)) {
                logger.write(seventh::utilities::LogLevel::Warning,
                             "User subsystem profiles were not loaded: " + profile_error);
            }
        }
    }
    if (const char* home = std::getenv("HOME")) {
        const auto knowledge_path =
            (std::filesystem::path(home) / ".config/7thcmd/knowledge.json").string();
        std::string knowledge_error;
        if (!orchestrator.knowledge().load_from_file(
                knowledge_path, knowledge_error)) {
            logger.write(seventh::utilities::LogLevel::Warning,
                         "Knowledge file was not loaded: " + knowledge_error);
        }
    }
    orchestrator.register_subsystem(
        std::make_shared<seventh::grammar::GrammarSubsystem>(grammar_config));

    auto command_subsystem =
        std::make_shared<seventh::CommandSubsystem>(
            platform, grammar_config, &orchestrator.knowledge());
    const std::vector<std::filesystem::path> packaged_capability_files{
        "config/capabilities.json",
        "/usr/local/share/7thcmd/config/capabilities.json",
        "/usr/share/7thcmd/config/capabilities.json"
    };
    for (const auto& path : packaged_capability_files) {
        std::error_code path_error;
        if (!std::filesystem::is_regular_file(path, path_error) || path_error) continue;
        std::string capability_error;
        if (!command_subsystem->load_capabilities(path.string(), capability_error)) {
            logger.write(seventh::utilities::LogLevel::Warning,
                         "Capability configuration was not loaded: " + capability_error);
        }
        break;
    }
    if (const char* home = std::getenv("HOME")) {
        const auto user_capabilities =
            std::filesystem::path(home) / ".config/7thcmd/capabilities.json";
        std::error_code path_error;
        if (std::filesystem::is_regular_file(user_capabilities, path_error) && !path_error) {
            std::string capability_error;
            if (!command_subsystem->load_capabilities(
                    user_capabilities.string(), capability_error)) {
                logger.write(seventh::utilities::LogLevel::Warning,
                             "User capability overrides were not loaded: " + capability_error);
            }
        }
    }
    const std::vector<std::filesystem::path> packaged_rule_files{
        "config/knowledge/rules/rules.json",
        "/usr/local/share/7thcmd/config/knowledge/rules/rules.json",
        "/usr/share/7thcmd/config/knowledge/rules/rules.json"
    };
    for (const auto& path : packaged_rule_files) {
        std::error_code path_error;
        if (!std::filesystem::is_regular_file(path, path_error) || path_error) continue;
        std::string rule_error;
        if (!command_subsystem->load_reasoning_rules(path.string(), rule_error)) {
            logger.write(seventh::utilities::LogLevel::Warning,
                         "Reasoning rules were not loaded: " + rule_error);
        }
        break;
    }
    if (const char* home = std::getenv("HOME")) {
        const auto user_rules =
            std::filesystem::path(home) / ".config/7thcmd/rules.json";
        std::error_code path_error;
        if (std::filesystem::is_regular_file(user_rules, path_error) && !path_error) {
            std::string rule_error;
            if (!command_subsystem->load_reasoning_rules(
                    user_rules.string(), rule_error, true)) {
                logger.write(seventh::utilities::LogLevel::Warning,
                             "User reasoning rules were not loaded: " + rule_error);
            }
        }
    }
    orchestrator.register_subsystem(std::move(command_subsystem));
    seventh::response::ResponseGenerator response_generator;
    seventh::cli::InputHandler input_handler;

    while (true) {
        std::cout << "7thcmd> " << std::flush;
        const auto input = input_handler.read_line();
        if (input.empty()) break;
        if (input == "exit" || input == "quit") break;

        auto outcome = orchestrator.execute(
            "command", {{"raw_input", input}});
        const auto cancel_task = [&orchestrator](seventh::TaskOutcome& task,
                                                const std::string& message) {
            if (!orchestrator.cancel(task.task_id)) {
                task.state = orchestrator.state_of(task.task_id);
                task.result.error = "The task could not be cancelled.";
                return false;
            }
            task.state = orchestrator.state_of(task.task_id);
            task.result.error = message;
            return true;
        };
        bool exit_requested = false;
        while (true) {
            if (outcome.result.status == seventh::SubsystemStatus::NeedsInformation) {
                bool cancelled = false;
                while (const auto question = orchestrator.next_question(outcome.task_id)) {
                    const auto options = outcome.result.result_data.find("choice_options");
                    if (options != outcome.result.result_data.end() &&
                        !options->second.empty()) {
                        std::cout << "Candidates:\n" << options->second << '\n';
                    }

                    if (!outcome.result.error.empty()) {
                        std::cout << seventh::cli::escape_terminal_text(outcome.result.error) << '\n';
                    }
                    std::cout << seventh::cli::escape_terminal_text(question->question)
                              << " (or 'cancel'): " << std::flush;
                    const auto answer = input_handler.read_line();
                    if (answer.empty() || answer == "cancel" || answer == "CANCEL" ||
                        answer == "Cancel") {
                        if (cancel_task(outcome,
                                "The task was cancelled while waiting for clarification.")) {
                            cancelled = true;
                            break;
                        }
                        std::cout << outcome.result.error << '\n';
                        continue;
                    }
                    if (answer == "exit" || answer == "EXIT" || answer == "Exit") {
                        if (cancel_task(outcome,
                                "The task was cancelled while waiting for clarification.")) {
                            cancelled = true;
                            exit_requested = true;
                            break;
                        }
                        std::cout << outcome.result.error << '\n';
                        continue;
                    }
                    if (!orchestrator.resolve_question(
                            outcome.task_id, question->required_field, answer)) {
                        std::cout << "That answer could not be applied. Try again.\n";
                    }
                }
                if (cancelled) break;
                outcome = orchestrator.continue_task(outcome.task_id);
                continue;
            }

            if (outcome.result.status == seventh::SubsystemStatus::NeedsPermission &&
                outcome.result.result_data.contains("confirmation_action") &&
                outcome.result.result_data.at("confirmation_action") == "delete") {
                std::cout << "Confirm deletion of "
                          << seventh::cli::escape_terminal_text(
                                 outcome.result.result_data.at("path"))
                          << "? Type yes to continue, anything else to cancel: " << std::flush;
                const auto confirmation = input_handler.read_line();
                if (confirmation != "yes" && confirmation != "YES" &&
                    confirmation != "Yes") {
                    if (cancel_task(outcome, "Deletion cancelled.")) break;
                    std::cout << outcome.result.error << '\n';
                    continue;
                }
                outcome = orchestrator.continue_task(outcome.task_id, true);
                continue;
            }
            break;
        }
        if (exit_requested) break;

        seventh::response::ResponseData response;
        response.task_id = outcome.task_id;
        response.state = outcome.state;
        response.operation = "command";
        response.error = outcome.result.error;
        response.missing_information = outcome.result.missing_information;

        for (const auto* key : {"operation", "entity_type", "name",
                                "destination", "path", "verified", "directory", "query", "count", "metric", "value", "bytes", "paths", "completed_steps", "total_steps", "displayed", "truncated", "matches", "source"}) {
            const auto found = outcome.result.result_data.find(key);
            if (found != outcome.result.result_data.end() && !found->second.empty()) {
                response.details[key] = found->second;
            }
        }

        std::cout << response_generator.generate(response) << '\n';
    }

    std::cout << "Goodbye.\n";
    return 0;
}
