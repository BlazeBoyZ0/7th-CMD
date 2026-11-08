#include "seventh/cli/terminal_output.hpp"

#include <iostream>

namespace seventh::cli {

void print_startup_message() {
    std::cout
        << "7th cmd v0.1 foundation build.\\n"
        << "Command mode is active; enter an instruction to begin.\\n"
        << "Use --help to see available options.\\n";
}

void print_message(const std::string_view message) {
    std::cout << message << '\\n';
}

void print_error(const std::string_view message) {
    std::cerr << "Error: " << message << '\\n';
}

} // namespace seventh::cli
