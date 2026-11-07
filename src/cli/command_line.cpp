#include "seventh/cli/command_line.hpp"

#include <iostream>
#include <string_view>

#ifndef SEVENTH_CMD_VERSION
#define SEVENTH_CMD_VERSION "0.1.0"
#endif

namespace seventh::cli {

CommandLineResult parse_command_line(const int argc, char* argv[]) {
    if (argc <= 1) {
        return {CommandLineAction::Start, 0};
    }

    const std::string_view argument{argv[1]};

    if (argument == "--help" || argument == "-h") {
        return {CommandLineAction::ShowHelp, 0};
    }

    if (argument == "--version" || argument == "-v") {
        return {CommandLineAction::ShowVersion, 0};
    }

    std::cerr << "7th cmd: unknown option: " << argument << '\n';
    std::cerr << "Run '7thcmd --help' for usage.\n";
    return {CommandLineAction::Invalid, 2};
}

void print_help() {
    std::cout
        << "7th cmd - local command intelligence\n\n"
        << "Usage: 7thcmd [option]\n\n"
        << "Options:\n"
        << "  -h, --help       Show this help message\n"
        << "  -v, --version    Show version information\n";
}

void print_version() {
    std::cout << "7th cmd " << SEVENTH_CMD_VERSION << '\n';
}

} // namespace seventh::cli
