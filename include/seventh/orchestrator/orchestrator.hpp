#pragma once

#include "seventh/orchestrator/cancellation_handler.hpp"
#include "seventh/orchestrator/interaction_coordinator.hpp"
#include "seventh/orchestrator/pause_resume_manager.hpp"
#include "seventh/orchestrator/request_manager.hpp"
#include "seventh/orchestrator/result_handler.hpp"
#include "seventh/orchestrator/subsystem_dispatcher.hpp"
#include "seventh/orchestrator/subsystem_router.hpp"
#include "seventh/orchestrator/task_state_manager.hpp"
#include "seventh/orchestrator/workflow_controller.hpp"

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

    [[nodiscard]] TaskOutcome execute(
        const std::string& operation,
        std::map<std::string, std::string> input_data = {},
        std::map<std::string, std::string> context = {},
        StringList constraints = {});
    [[nodiscard]] TaskState state_of(const TaskId& task_id) const;
    [[nodiscard]] bool cancel(const TaskId& task_id);
    [[nodiscard]] bool resume(const TaskId& task_id,
                              bool permission_granted = false);
    [[nodiscard]] std::optional<knowledge::ClarificationRequest> next_question(
        const TaskId& task_id) const;
    bool resolve_question(const TaskId& task_id, const std::string& field);

private:
    std::vector<std::shared_ptr<Subsystem>> subsystems_;
    RequestManager request_manager_;
    SubsystemRouter router_;
    SubsystemDispatcher dispatcher_;
    TaskStateManager task_states_;
    WorkflowController workflow_;
    ResultHandler result_handler_;
    InteractionCoordinator interactions_;
    CancellationHandler cancellation_;
    PauseResumeManager pause_resume_;
};

} // namespace seventh
