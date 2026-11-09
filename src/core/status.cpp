#include "seventh/core/status.hpp"

namespace seventh {

std::string_view to_string(const SubsystemStatus status) noexcept {
    switch (status) {
    case SubsystemStatus::Success: return "SUCCESS";
    case SubsystemStatus::NeedsInformation: return "NEEDS_INFORMATION";
    case SubsystemStatus::NeedsPermission: return "NEEDS_PERMISSION";
    case SubsystemStatus::NewRequirements: return "NEW_REQUIREMENTS";
    case SubsystemStatus::Uncertain: return "UNCERTAIN";
    case SubsystemStatus::Failure: return "FAILURE";
    }
    return "UNKNOWN_STATUS";
}

} // namespace seventh
