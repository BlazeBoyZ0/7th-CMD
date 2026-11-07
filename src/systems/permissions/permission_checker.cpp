#include "seventh/systems/permissions/permission_checker.hpp"

#include <filesystem>

namespace seventh::permissions {

bool PermissionChecker::path_is_allowed(
    const std::string& path,
    const capabilities::CapabilityDefinition& capability) const {
    if (path.empty()) return capability.allowed_roots.empty();
    if (capability.allowed_roots.empty()) return false;

    const auto target = std::filesystem::path(path).lexically_normal();
    if (!target.is_absolute()) return false;

    for (const auto& root_text : capability.allowed_roots) {
        const auto root = std::filesystem::path(root_text).lexically_normal();
        if (!root.is_absolute()) continue;

        auto target_part = target.begin();
        auto root_part = root.begin();
        for (; root_part != root.end() && target_part != target.end();
             ++root_part, ++target_part) {
            if (*root_part != *target_part) break;
        }
        if (root_part == root.end()) return true;
    }
    return false;
}

} // namespace seventh::permissions
