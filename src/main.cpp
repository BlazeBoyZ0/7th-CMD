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

    seventh::platform::linux_backend::LinuxPlatform platform;
    seventh::Orchestrator orchestrator;
    orchestrator.register_subsystem(
        std::make_shared<seventh::grammar::GrammarSubsystem>(grammar_config));
    orchestrator.register_subsystem(
        std::make_shared<seventh::CommandSubsystem>(platform));
    seventh::response::ResponseGenerator response_generator;
    seventh::cli::InputHandler input_handler;

    while (true) {
        std::cout << "7thcmd> " << std::flush;
        const auto input = input_handler.read_line();
        if (input.empty()) break;
        if (input == "exit" || input == "quit") break;

        const auto outcome = orchestrator.execute(
            "command", {{"raw_input", input}});

        seventh::response::ResponseData response;
        response.task_id = outcome.task_id;
        response.state = outcome.state;
        response.operation = "command";
        response.error = outcome.result.error;
        response.missing_information = outcome.result.missing_information;

        for (const auto* key : {"operation", "entity_type", "name",
                                "destination", "path", "verified"}) {
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
