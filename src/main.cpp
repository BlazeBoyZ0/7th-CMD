#include "seventh/cli/command_line.hpp"
#include "seventh/cli/input_handler.hpp"
#include "seventh/cli/terminal_output.hpp"
#include "seventh/engines/grammar/grammar_engine.hpp"

#include <iostream>
#include <string>

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

    seventh::cli::print_startup_message();
    std::cout << "Type an instruction, or 'exit' to quit.\n";

    seventh::cli::InputHandler input_handler;
    seventh::grammar::GrammarEngine grammar_engine;

    while (true) {
        std::cout << "7thcmd> " << std::flush;
        const auto input = input_handler.read_line();
        if (input.empty()) break;
        if (input == "exit" || input == "quit") break;

        const auto intent = grammar_engine.interpret(input);
        if (!intent.valid) {
            seventh::cli::print_error(intent.error.empty()
                ? "I could not interpret that instruction."
                : intent.error);
            for (const auto& missing : intent.missing_information) {
                std::cout << "  Missing: " << missing << '\n';
            }
            continue;
        }

        std::cout << "Understood request:\n"
                  << "  Operation: " << intent.operation << '\n'
                  << "  Target type: " << intent.entity_type << '\n';
        if (!intent.name.empty()) std::cout << "  Name: " << intent.name << '\n';
        if (!intent.source.empty()) std::cout << "  Source: " << intent.source << '\n';
        if (!intent.destination.empty()) std::cout << "  Destination: " << intent.destination << '\n';
        if (!intent.query.empty()) std::cout << "  Query: " << intent.query << '\n';
        std::cout << "  Status: interpreted; execution is not connected yet.\n";
    }

    std::cout << "Goodbye.\n";
    return 0;
}
