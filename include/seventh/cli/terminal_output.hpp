#pragma once

#include <string_view>

namespace seventh::cli {

void print_startup_message();
void print_message(std::string_view message);
void print_error(std::string_view message);

} // namespace seventh::cli
