#pragma once

#include <optional>
#include <string_view>

namespace seventh::information {

class TypeSystem {
public:
    [[nodiscard]] static std::optional<double> number(std::string_view value) noexcept;
    [[nodiscard]] static std::optional<bool> boolean(std::string_view value) noexcept;
};

} // namespace seventh::information
