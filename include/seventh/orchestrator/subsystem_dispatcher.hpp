#pragma once

#include "seventh/core/subsystem.hpp"

namespace seventh {

class SubsystemDispatcher {
public:
    [[nodiscard]] SubsystemResult dispatch(
        Subsystem& subsystem, const SubsystemRequest& request) const;
};

} // namespace seventh
