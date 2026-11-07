#pragma once

#include "seventh/platform/platform_interface.hpp"
#include "seventh/platform/linux/linux_applications.hpp"
#include "seventh/platform/linux/linux_filesystem.hpp"
#include "seventh/platform/linux/linux_processes.hpp"
#include "seventh/platform/linux/linux_system.hpp"

namespace seventh::platform::linux {

class LinuxPlatform final : public PlatformInterface {
public:
    [[nodiscard]] const char* id() const noexcept override;
    [[nodiscard]] FilesystemInterface& filesystem() noexcept override;
    [[nodiscard]] ApplicationInterface& applications() noexcept override;
    [[nodiscard]] ProcessInterface& processes() noexcept override;
    [[nodiscard]] SystemInterface& system() noexcept override;

private:
    LinuxFilesystem filesystem_;
    LinuxApplications applications_;
    LinuxProcesses processes_;
    LinuxSystem system_;
};

} // namespace seventh::platform::linux
