#include "seventh/cli/input_handler.hpp"

#include <iostream>
#include <utility>

namespace seventh::cli {

std::string InputHandler::read_line() {
    std::string line;
    if (!std::getline(std::cin, line)) {
        return {};
    }
    return line;
}

} // namespace seventh::cli
