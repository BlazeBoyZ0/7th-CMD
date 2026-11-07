#include "seventh/cli/command_line.hpp"
#include "seventh/cli/terminal_output.hpp"

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
        seventh::cli::print_startup_message();
        return result.exit_code;
    }

    return 1;
}
