#pragma once

#include <string>

namespace seventh::utilities {

[[nodiscard]] std::string normalize_path(const std::string& path);
[[nodiscard]] bool is_absolute_path(const std::string& path) noexcept;
[[nodiscard]] bool path_is_within(const std::string& path,
                                  const std::string& root);

} // namespace seventh::utilities
