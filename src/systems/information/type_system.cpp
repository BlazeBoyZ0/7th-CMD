#include "seventh/systems/information/type_system.hpp"

#include <charconv>
#include <string>

namespace seventh::information {

std::optional<double> TypeSystem::number(std::string_view value) noexcept {
    try {
        std::size_t consumed = 0;
        const std::string text(value);
        const double parsed = std::stod(text, &consumed);
        if (consumed != text.size()) return std::nullopt;
        return parsed;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<bool> TypeSystem::boolean(std::string_view value) noexcept {
    if (value == "true" || value == "1" || value == "yes") return true;
    if (value == "false" || value == "0" || value == "no") return false;
    return std::nullopt;
}

} // namespace seventh::information
