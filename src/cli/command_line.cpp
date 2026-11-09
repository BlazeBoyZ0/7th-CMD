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

    if (argument == "--help" || argument == "--h" || argument == "-h") {
        return {CommandLineAction::ShowHelp, 0};
    }

    if (argument == "--version" || argument == "--v" || argument == "-v") {
        return {CommandLineAction::ShowVersion, 0};
    }

    std::cerr << "7th cmd: unknown option: " << argument << '\n';
    std::cerr << "Run '7thcmd --help' for usage.\n";
    return {CommandLineAction::Invalid, 2};
}

void print_help() {
    std::cout
        << "7th cmd - Help\n"
        << "========================================\n"
        << "ABOUT\n"
        << "  Lightweight, local natural-language PC task automation.\n"
        << "  No LLM, cloud API, or external AI service is required.\n\n"
        << "CAPABILITIES\n"
        << "  FILES AND FOLDERS\n"
        << "    Creating folders\n"
        << "    Opening folders and applications\n"
        << "    Renaming and moving supported files and folders\n"
        << "    Copying files\n"
        << "    Deleting files and empty folders with confirmation\n"
        << "    Searching within directories\n"
        << "    Counting words, lines, and characters in text files\n\n"
        << "  TASK INTELLIGENCE\n"
        << "    Understanding natural-language instructions\n"
        << "    Asking for missing information when needed\n"
        << "    Planning supported multi-step folder tasks\n"
        << "    Tracking execution outcomes and verification\n\n"
        << "  EXECUTION INSPECTION\n"
        << "    inspect task <task-id>\n"
        << "      Inspect one execution record.\n"
        << "    show last task\n"
        << "      Inspect the latest execution record.\n\n"
        << "APPLICATION CONTROLS\n"
        << "  --help, --h       Display this help menu.\n"
        << "  --version, --v    Display version information.\n"
        << "  exit              Close 7th cmd.\n\n"
        << "NOTES\n"
        << "  Execution records are kept in memory for this session only.\n"
        << "  The latest 100 records are retained.\n"
        << "  Capabilities listed here reflect the current v0.1 implementation.\n"
        << "========================================\n";
}

void print_version() {
    std::cout << "7th cmd " << SEVENTH_CMD_VERSION << '\n';
}

} // namespace seventh::cli
