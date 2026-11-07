#pragma once

#include "seventh/core/types.hpp"
#include "seventh/systems/knowledge/clarification_manager.hpp"

namespace seventh {

class InteractionCoordinator {
public:
    void request_missing_information(const TaskId& task_id,
                                     const StringList& missing_information);
    [[nodiscard]] std::optional<knowledge::ClarificationRequest> next_question(
        const TaskId& task_id) const;
    bool resolve(const TaskId& task_id, const std::string& field);
    void cancel(const TaskId& task_id);

private:
    knowledge::ClarificationManager clarifications_;
};

} // namespace seventh
