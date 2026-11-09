#pragma once

#include "seventh/platform/platform_types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace seventh::platform {

class ProcessInterface {
public:
    virtual ~ProcessInterface() = default;
    [[nodiscard]] virtual std::vector<ProcessEntry> list(
        std::string& error) const = 0;
    [[nodiscard]] virtual PlatformResult terminate(std::uint64_t process_id) = 0;
};

} // namespace seventh::platform
