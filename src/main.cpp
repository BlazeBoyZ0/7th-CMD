#include "seventh/cli/command_line.hpp"
#include "seventh/cli/input_handler.hpp"
#include "seventh/cli/terminal_output.hpp"
#include "seventh/engines/grammar/grammar_config.hpp"
#include "seventh/engines/grammar/grammar_subsystem.hpp"
#include "seventh/orchestrator/orchestrator.hpp"
#include "seventh/utilities/logger.hpp"

#include <array>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

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

    seventh::Orchestrator orchestrator;
    orchestrator.register_subsystem(
        std::make_shared<seventh::grammar::GrammarSubsystem>(
            std::move(grammar_config)));
    seventh::cli::InputHandler input_handler;

    while (true) {
        std::cout << "7thcmd> " << std::flush;
        const auto input = input_handler.read_line();
        if (input.empty()) break;
        if (input == "exit" || input == "quit") break;

        auto outcome = orchestrator.execute(
            "interpret", {{"raw_input", input}});

        if (outcome.result.status == seventh::SubsystemStatus::Failure) {
            seventh::cli::print_error(outcome.result.error.empty()
                ? "The request could not be processed."
                : outcome.result.error);
            continue;
        }

        if (outcome.result.status == seventh::SubsystemStatus::NeedsInformation) {
            seventh::cli::print_error("I need more information:");
            for (const auto& missing : outcome.result.missing_information) {
                std::cout << "  Missing: " << missing << '\n';
            }
            continue;
        }

        const auto& data = outcome.result.result_data;
        std::cout << "Understood request:\n"
                  << "  Operation: " << data.at("operation") << '\n'
                  << "  Target type: " << data.at("entity_type") << '\n';
        if (!data.at("name").empty()) std::cout << "  Name: " << data.at("name") << '\n';
        if (!data.at("source").empty()) std::cout << "  Source: " << data.at("source") << '\n';
        if (!data.at("destination").empty()) std::cout << "  Destination: " << data.at("destination") << '\n';
        if (!data.at("query").empty()) std::cout << "  Query: " << data.at("query") << '\n';
        std::cout << "  Task: " << outcome.task_id << '\n'
                  << "  State: completed (interpretation only; no OS action executed)\n";
    }

    std::cout << "Goodbye.\n";
    return 0;
}
