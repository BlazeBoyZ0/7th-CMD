#pragma once

#include "seventh/platform/system/system_interface.hpp"

namespace seventh::platform::linux_backend {

class LinuxSystem final : public SystemInterface {
public:
    [[nodiscard]] SystemInformation information(std::string& error) const override;
};

} // namespace seventh::platform::linux_backend
