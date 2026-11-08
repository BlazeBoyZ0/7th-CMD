#include "seventh/systems/permissions/permission_checker.hpp"

#include <filesystem>
#include <fcntl.h>
#include <unistd.h>

namespace seventh::permissions {
namespace fs = std::filesystem;

AuthorizedRoot::~AuthorizedRoot() {
    if (descriptor >= 0) ::close(descriptor);
}

bool PermissionChecker::open_authorized_root(
    const std::string& path,
    const capabilities::CapabilityDefinition& capability,
    std::shared_ptr<const AuthorizedRoot>& authorized_root) const {
    authorized_root.reset();
    if (path.empty()) return capability.allowed_roots.empty();
    if (capability.allowed_roots.empty()) return false;

    std::error_code error;
    const auto target = fs::absolute(fs::path(path), error).lexically_normal();
    if (error) return false;

    for (const auto& root_text : capability.allowed_roots) {
        error.clear();
        const auto root = fs::weakly_canonical(root_text, error);
        if (error || !root.is_absolute()) continue;

        auto target_part = target.begin();
        auto root_part = root.begin();
        for (; root_part != root.end() && target_part != target.end();
             ++root_part, ++target_part) {
            if (*root_part != *target_part) break;
        }
        if (root_part != root.end()) continue;

        const int descriptor = ::open(
            root.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        if (descriptor < 0) continue;

        authorized_root = std::make_shared<AuthorizedRoot>(root.string(), descriptor);
        return true;
    }
    return false;
}

} // namespace seventh::permissions
