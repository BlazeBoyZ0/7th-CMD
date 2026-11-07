#include "seventh/orchestrator/subsystem_dispatcher.hpp"

#include <exception>

namespace seventh {

SubsystemResult SubsystemDispatcher::dispatch(
    Subsystem& subsystem, const SubsystemRequest& request) const {
    try {
        return subsystem.handle(request);
    } catch (const std::exception& error) {
        SubsystemResult result;
        result.request_id = request.request_id;
        result.status = SubsystemStatus::Failure;
        result.error = std::string("Subsystem exception: ") + error.what();
        return result;
    } catch (...) {
        SubsystemResult result;
        result.request_id = request.request_id;
        result.status = SubsystemStatus::Failure;
        result.error = "Subsystem failed with an unknown exception.";
        return result;
    }
}

} // namespace seventh
