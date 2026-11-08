#pragma once

#include <string>
#include <string_view>

namespace seventh::cli {

[[nodiscard]] std::string escape_terminal_text(std::string_view text);
void print_startup_message();
void print_message(std::string_view message);
void print_error(std::string_view message);

} // namespace seventh::cli
