#pragma once

#include "seventh/core/subsystem.hpp"
#include "seventh/core/types.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace seventh {

struct TaskOutcome {
    TaskId task_id;
    TaskState state{TaskState::New};
    SubsystemResult result;
    std::string subsystem_id;
};

class Orchestrator {
public:
    void register_subsystem(std::shared_ptr<Subsystem> subsystem);

    [[nodiscard]] TaskOutcome execute(const std::string& operation,
                                      std::map<std::string, std::string> input_data = {},
                                      std::map<std::string, std::string> context = {},
                                      StringList constraints = {});
    [[nodiscard]] TaskState state_of(const TaskId& task_id) const;

private:
    [[nodiscard]] bool transition(const TaskId& task_id, TaskState next);
    [[nodiscard]] std::shared_ptr<Subsystem> select_subsystem(
        const SubsystemRequest& request) const;

    std::vector<std::shared_ptr<Subsystem>> subsystems_;
    std::map<TaskId, TaskState> task_states_;
};

} // namespace seventh
