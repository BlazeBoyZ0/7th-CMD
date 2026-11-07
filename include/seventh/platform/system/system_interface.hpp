#pragma once

#include "seventh/platform/platform_types.hpp"

namespace seventh::platform {

class SystemInterface {
public:
    virtual ~SystemInterface() = default;
    [[nodiscard]] virtual SystemInformation information(
        std::string& error) const = 0;
};

} // namespace seventh::platform
