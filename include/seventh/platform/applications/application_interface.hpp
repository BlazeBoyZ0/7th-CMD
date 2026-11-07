#pragma once

#include "seventh/platform/platform_types.hpp"

#include <optional>
#include <string>
#include <vector>

namespace seventh::platform {

class ApplicationInterface {
public:
    virtual ~ApplicationInterface() = default;
    [[nodiscard]] virtual std::vector<ApplicationEntry> list(
        std::string& error) const = 0;
    [[nodiscard]] virtual std::optional<ApplicationEntry> find(
        const std::string& name) const = 0;
    [[nodiscard]] virtual PlatformResult launch(
        const ApplicationEntry& application) = 0;
};

} // namespace seventh::platform
