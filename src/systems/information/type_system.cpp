#include "seventh/systems/information/type_system.hpp"

#include <charconv>
#include <cmath>
#include <string>

namespace seventh::information {

std::optional<double> TypeSystem::number(std::string_view value) noexcept {
    if (value.empty()) return std::nullopt;

    double parsed = 0.0;
    const auto result = std::from_chars(
        value.data(), value.data() + value.size(), parsed,
        std::chars_format::general);

    if (result.ec != std::errc{} ||
        result.ptr != value.data() + value.size() ||
        !std::isfinite(parsed)) {
        return std::nullopt;
    }
    return parsed;
}

std::optional<bool> TypeSystem::boolean(std::string_view value) noexcept {
    std::string normalized(value);
    for (char& ch : normalized) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }

    if (normalized == "true" || normalized == "1" || normalized == "yes") {
        return true;
    }
    if (normalized == "false" || normalized == "0" || normalized == "no") {
        return false;
    }
    return std::nullopt;
}

} // namespace seventh::information
