#pragma once

namespace seventh::cli {

enum class CommandLineAction {
    Start,
    ShowHelp,
    ShowVersion,
    Invalid
};

struct CommandLineResult {
    CommandLineAction action{CommandLineAction::Start};
    int exit_code{0};
};

[[nodiscard]] CommandLineResult parse_command_line(int argc, char* argv[]);
void print_help();
void print_version();

} // namespace seventh::cli
