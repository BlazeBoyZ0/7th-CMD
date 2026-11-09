#pragma once

#include "seventh/core/types.hpp"

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace seventh::planning {

inline constexpr std::size_t default_max_plan_tasks = 256;
inline constexpr std::size_t default_max_dependency_edges = 4096;

struct TaskDraft {
    std::string id;
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
    std::size_t max_tasks{default_max_plan_tasks};
    std::size_t max_dependency_edges{default_max_dependency_edges};
};

struct PlanningRequest {
    std::string goal;
    std::vector<TaskDraft> tasks;
    std::size_t max_tasks{default_max_plan_tasks};
    std::size_t max_dependency_edges{default_max_dependency_edges};
};

} // namespace seventh::planning
