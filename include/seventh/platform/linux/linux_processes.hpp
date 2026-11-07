#pragma once

#include "seventh/platform/processes/process_interface.hpp"

namespace seventh::platform::linux {

class LinuxProcesses final : public ProcessInterface {
public:
    [[nodiscard]] std::vector<ProcessEntry> list(
        std::string& error) const override;
    [[nodiscard]] PlatformResult terminate(std::uint64_t process_id) override;
};

} // namespace seventh::platform::linux
