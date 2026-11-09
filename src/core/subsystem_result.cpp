#include "seventh/core/subsystem_result.hpp"

namespace seventh {

bool SubsystemResult::successful() const noexcept {
    return status == SubsystemStatus::Success;
}

} // namespace seventh
