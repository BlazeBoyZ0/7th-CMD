#pragma once

#include "seventh/core/types.hpp"

#include <map>
#include <mutex>

namespace seventh {

class TaskStateManager {
public:
    bool create(const TaskId& task_id);
    [[nodiscard]] bool transition(const TaskId& task_id, TaskState next);
    [[nodiscard]] TaskState state_of(const TaskId& task_id) const;

private:
    mutable std::mutex mutex_;
    std::map<TaskId, TaskState> states_;
};

} // namespace seventh
