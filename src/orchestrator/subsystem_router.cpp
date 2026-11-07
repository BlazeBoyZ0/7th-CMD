#include "seventh/orchestrator/subsystem_router.hpp"

namespace seventh {

std::shared_ptr<Subsystem> SubsystemRouter::route(
    const std::vector<std::shared_ptr<Subsystem>>& subsystems,
    const SubsystemRequest& request) const {
    for (const auto& subsystem : subsystems) {
        if (subsystem && subsystem->can_handle(request)) return subsystem;
    }
    return {};
}

} // namespace seventh
