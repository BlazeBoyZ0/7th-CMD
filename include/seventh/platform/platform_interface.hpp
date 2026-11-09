#pragma once

#include "seventh/platform/applications/application_interface.hpp"
#include "seventh/platform/filesystem/filesystem_interface.hpp"
#include "seventh/platform/processes/process_interface.hpp"
#include "seventh/platform/system/system_interface.hpp"

namespace seventh::platform {

class PlatformInterface {
public:
    virtual ~PlatformInterface() = default;
    [[nodiscard]] virtual const char* id() const noexcept = 0;
    [[nodiscard]] virtual FilesystemInterface& filesystem() noexcept = 0;
    [[nodiscard]] virtual ApplicationInterface& applications() noexcept = 0;
    [[nodiscard]] virtual ProcessInterface& processes() noexcept = 0;
    [[nodiscard]] virtual SystemInterface& system() noexcept = 0;
};

} // namespace seventh::platform
