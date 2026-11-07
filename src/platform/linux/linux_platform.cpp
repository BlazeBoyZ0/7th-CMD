#include "seventh/platform/linux/linux_platform.hpp"

namespace seventh::platform::linux_backend {

const char* LinuxPlatform::id() const noexcept {
    return "linux";
}

FilesystemInterface& LinuxPlatform::filesystem() noexcept {
    return filesystem_;
}

ApplicationInterface& LinuxPlatform::applications() noexcept {
    return applications_;
}

ProcessInterface& LinuxPlatform::processes() noexcept {
    return processes_;
}

SystemInterface& LinuxPlatform::system() noexcept {
    return system_;
}

} // namespace seventh::platform::linux_backend
