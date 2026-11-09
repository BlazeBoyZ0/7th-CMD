#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace seventh::utilities {

[[nodiscard]] std::string trim(std::string value);
[[nodiscard]] std::string lowercase(std::string value);
[[nodiscard]] bool equals_ignore_case(std::string_view left,
                                      std::string_view right) noexcept;
[[nodiscard]] std::vector<std::string> split(std::string_view value, char delimiter);
[[nodiscard]] std::string join(const std::vector<std::string>& values,
                               std::string_view separator);

} // namespace seventh::utilities
