#pragma once

#include <string>

namespace seventh::cli {

class InputHandler {
public:
    [[nodiscard]] std::string read_line();
};

} // namespace seventh::cli
