#pragma once

#include "seventh/systems/knowledge/task_context.hpp"

#include <map>
#include <optional>

namespace seventh::knowledge {

class ContextManager {
public:
    TaskContext& create(const TaskId& task_id);
    [[nodiscard]] TaskContext* find(const TaskId& task_id);
    [[nodiscard]] const TaskContext* find(const TaskId& task_id) const;
    bool erase(const TaskId& task_id);

private:
    std::map<TaskId, TaskContext> contexts_;
};

} // namespace seventh::knowledge
