#include "seventh/utilities/path_utils.hpp"

#include <filesystem>

namespace seventh::utilities {
namespace fs = std::filesystem;

std::string normalize_path(const std::string& path) {
    return fs::path(path).lexically_normal().string();
}

bool is_absolute_path(const std::string& path) noexcept {
    return !path.empty() && fs::path(path).is_absolute();
}

bool path_is_within(const std::string& path, const std::string& root) {
    const auto target = fs::path(path).lexically_normal();
    const auto base = fs::path(root).lexically_normal();
    if (!target.is_absolute() || !base.is_absolute()) return false;

    auto target_part = target.begin();
    auto base_part = base.begin();
    for (; base_part != base.end() && target_part != target.end();
         ++base_part, ++target_part) {
        if (*base_part != *target_part) return false;
    }
    return base_part == base.end();
}

} // namespace seventh::utilities
