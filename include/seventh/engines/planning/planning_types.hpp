#pragma once

#include "seventh/core/types.hpp"

#include <map>
#include <string>
#include <vector>

namespace seventh::planning {

struct TaskDraft {
    std::string operation;
    std::map<std::string, std::string> input_data;
    StringList dependencies;
};

struct PlanTask {
    std::string id;
    std::string operation;
    std::map<std::string, std::string> input_data;
    StringList dependencies;
    TaskState state{TaskState::New};
};

struct Plan {
    std::string id;
    std::string goal;
    std::vector<PlanTask> tasks;
    bool valid{false};
    StringList errors;
};

struct PlanningRequest {
    std::string goal;
    std::vector<TaskDraft> tasks;
};

} // namespace seventh::planning
