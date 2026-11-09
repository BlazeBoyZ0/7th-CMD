#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace seventh {

using TaskId = std::string;
using RequestId = std::string;
using SubsystemId = std::string;
using Requirement = std::string;
using StringList = std::vector<std::string>;

enum class TruthValue : std::uint8_t {
    False,
    Unknown,
    True
};

enum class TaskState : std::uint8_t {
    New,
    Understanding,
    Routing,
    Processing,
    WaitingForUser,
    WaitingForPermission,
    Recovery,
    Completed,
    Failed,
    Cancelled
};

} // namespace seventh
