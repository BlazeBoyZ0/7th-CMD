#include "seventh/orchestrator/result_handler.hpp"

#include <utility>

namespace seventh {

SubsystemResult ResultHandler::normalize(
    SubsystemResult result, const RequestId& request_id) const {
    if (result.request_id.empty()) result.request_id = request_id;
    if (result.status == SubsystemStatus::Failure && result.error.empty()) {
        result.error = "The subsystem reported a failure without an explanation.";
    }
    return result;
}

} // namespace seventh
