#pragma once

#include "seventh/core/subsystem.hpp"

#include <memory>
#include <vector>

namespace seventh {

class SubsystemRouter {
public:
    [[nodiscard]] std::shared_ptr<Subsystem> route(
        const std::vector<std::shared_ptr<Subsystem>>& subsystems,
        const SubsystemRequest& request) const;
};

} // namespace seventh
