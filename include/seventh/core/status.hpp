#pragma once

#include <string_view>

namespace seventh {

enum class SubsystemStatus {
    Success,
    NeedsInformation,
    NeedsPermission,
    NewRequirements,
    Uncertain,
    Failure
};

[[nodiscard]] std::string_view to_string(SubsystemStatus status) noexcept;

} // namespace seventh
