#include "seventh/core/subsystem_request.hpp"

namespace seventh {

bool SubsystemRequest::valid() const noexcept {
    return !request_id.empty() && !task_id.empty() && !operation.empty();
}

} // namespace seventh
