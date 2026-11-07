#include "seventh/systems/permissions/permission_checker.hpp"

#include <filesystem>

namespace seventh::permissions {

bool PermissionChecker::path_is_allowed(
    const std::string& path,
    const capabilities::CapabilityDefinition& capability) const {
    if (path.empty()) return capability.allowed_roots.empty();
    if (capability.allowed_roots.empty()) return false;

    std::error_code error;
    const auto target = std::filesystem::weakly_canonical(path, error);
    if (error) return false;

    for (const auto& root_text : capability.allowed_roots) {
        error.clear();
        const auto root = std::filesystem::weakly_canonical(root_text, error);
        if (error) continue;

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
