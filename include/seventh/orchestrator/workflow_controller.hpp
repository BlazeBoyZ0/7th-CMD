#pragma once

#include "seventh/core/subsystem_result.hpp"

namespace seventh {

class WorkflowController {
public:
    [[nodiscard]] TaskState next_state(const SubsystemResult& result) const noexcept;
};

} // namespace seventh
