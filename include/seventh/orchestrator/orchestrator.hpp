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
#include "seventh/systems/knowledge/context_manager.hpp"
#include "seventh/systems/knowledge/knowledge_system.hpp"
#include "seventh/systems/recovery/error_manager.hpp"
#include "seventh/systems/recovery/recovery_manager.hpp"

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

    [[nodiscard]] TaskOutcome execute(
        const std::string& operation,
        std::map<std::string, std::string> input_data = {},
        std::map<std::string, std::string> context = {},
        StringList constraints = {});
    [[nodiscard]] TaskState state_of(const TaskId& task_id) const;
    [[nodiscard]] bool cancel(const TaskId& task_id);
    [[nodiscard]] bool resume(const TaskId& task_id,
                              bool permission_granted = false);
    [[nodiscard]] TaskOutcome continue_task(
        const TaskId& task_id, bool permission_granted = false);
    [[nodiscard]] std::optional<knowledge::ClarificationRequest> next_question(
        const TaskId& task_id) const;
    bool resolve_question(const TaskId& task_id, const std::string& field,
                          const std::string& answer);

    [[nodiscard]] knowledge::KnowledgeSystem& knowledge() noexcept;
    [[nodiscard]] const knowledge::KnowledgeSystem& knowledge() const noexcept;
    [[nodiscard]] const knowledge::TaskContext* context_of(
        const TaskId& task_id) const;
    bool clear_context(const TaskId& task_id);
    [[nodiscard]] const std::vector<recovery::ErrorRecord>& error_history() const noexcept;

private:
    struct PendingTask {
        SubsystemRequest request;
        std::shared_ptr<Subsystem> subsystem;
        std::size_t recovery_attempts{0};
    };

    void record_context(const TaskOutcome& outcome);
    void record_error(const TaskOutcome& outcome);
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
    knowledge::KnowledgeSystem knowledge_;
    knowledge::ContextManager contexts_;
    std::map<TaskId, PendingTask> pending_tasks_;
    recovery::ErrorManager error_manager_;
    recovery::RecoveryManager recovery_manager_;
};

} // namespace seventh
