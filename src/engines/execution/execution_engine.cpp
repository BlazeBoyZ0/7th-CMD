#include "seventh/engines/execution/execution_engine.hpp"

#include <utility>

namespace seventh::execution {

void ExecutionEngine::register_executor(std::shared_ptr<OperationExecutor> executor) {
    dispatcher_.register_executor(std::move(executor));
}

ExecutionResult ExecutionEngine::execute(const ExecutionRequest& request) {
    ExecutionResult result;
    if (request.task_id.empty() || request.operation.empty()) {
        result.error = "Execution request is missing a task identifier or operation.";
        return result;
    }

    if (!tracker_.begin(request.task_id)) {
        result.error = "This task is already running or has reached a terminal execution state.";
        return result;
    }

    ExecutionRecord record;
    record.record_id = request.task_id;
    record.task_id = request.task_id;
    record.cir_id = request.cir_id;
    record.plan_id = request.plan_id;

    ExecutionStepRecord step;
    step.step_id = request.plan_step_id.empty()
        ? request.task_id : request.plan_step_id;
    step.operation = request.operation;
    step.input_data = request.input_data;

    if (request.requires_authorization && !request.authorization_granted) {
        result.status = ExecutionStatus::NeedsPermission;
        result.error = "This operation requires explicit authorization.";
        if (!tracker_.block(request.task_id)) {
            result.status = ExecutionStatus::Failure;
            result.error += " The tracker could not record the blocked state.";
            (void)tracker_.fail(request.task_id);
        }
        step.outcome = StepOutcome::Blocked;
        step.error = result.error;
        record.status = result.status;
        record.steps.push_back(std::move(step));
        retain_record(std::move(record));
        return result;
    }

    result = dispatcher_.dispatch(request);
    switch (result.status) {
    case ExecutionStatus::Success:
        if (!tracker_.complete(request.task_id)) {
            result.status = ExecutionStatus::Failure;
            result.error = "Execution succeeded, but its tracker state could not be completed.";
            (void)tracker_.fail(request.task_id);
        }
        break;
    case ExecutionStatus::NeedsPermission:
        if (!tracker_.block(request.task_id)) {
            result.status = ExecutionStatus::Failure;
            result.error = "Permission is required, but the tracker could not record the blocked state.";
            (void)tracker_.fail(request.task_id);
        }
        break;
    case ExecutionStatus::Unsupported:
    case ExecutionStatus::Failure:
        if (!tracker_.fail(request.task_id)) {
            result.error += " The tracker could not record the failed state.";
        }
        break;
    }

    step.outcome = result.status == ExecutionStatus::Success
        ? StepOutcome::Succeeded
        : (result.status == ExecutionStatus::NeedsPermission
            ? StepOutcome::Blocked : StepOutcome::Failed);
    step.output_data = result.output_data;
    step.error = result.error;
    record.status = result.status;
    record.steps.push_back(std::move(step));
    retain_record(std::move(record));
    return result;
}

bool ExecutionEngine::record_terminal_failure(
    const std::string& task_id, const std::string& operation,
    const std::string& error, const std::string& cir_id, bool blocked) {
    ExecutionStepRecord step;
    step.step_id = task_id;
    step.operation = operation.empty() ? "command" : operation;
    step.outcome = blocked ? StepOutcome::Blocked : StepOutcome::Failed;
    step.error = error;
    return record_terminal_failure(task_id, std::vector<ExecutionStepRecord>{std::move(step)},
                                   cir_id, {}, blocked);
}

bool ExecutionEngine::record_terminal_failure(
    const std::string& task_id, const std::vector<ExecutionStepRecord>& steps,
    const std::string& cir_id, const std::string& plan_id, bool blocked) {
    if (task_id.empty() || records_.contains(task_id) || steps.empty()) return false;
    ExecutionRecord record;
    record.record_id = task_id;
    record.task_id = task_id;
    record.cir_id = cir_id;
    record.plan_id = plan_id;
    record.status = blocked ? ExecutionStatus::NeedsPermission : ExecutionStatus::Failure;
    record.steps = steps;
    if (tracker_.begin(task_id)) {
        if (blocked) (void)tracker_.block(task_id);
        else (void)tracker_.fail(task_id);
    }
    retain_record(std::move(record));
    return true;
}


bool ExecutionEngine::record_aggregate(
    const std::string& task_id, const std::vector<std::string>& step_task_ids,
    const std::string& cir_id, const std::string& plan_id) {
    if (task_id.empty() || records_.contains(task_id) || step_task_ids.empty()) return false;

    ExecutionRecord aggregate;
    aggregate.record_id = task_id;
    aggregate.task_id = task_id;
    aggregate.cir_id = cir_id;
    aggregate.plan_id = plan_id;
    aggregate.status = ExecutionStatus::Success;

    // An aggregate is authoritative only when every requested child record
    // is present. Never silently turn a partial history into a successful task.
    std::vector<std::string> seen_step_task_ids;
    seen_step_task_ids.reserve(step_task_ids.size());
    for (const auto& step_task_id : step_task_ids) {
        if (step_task_id.empty() ||
            std::find(seen_step_task_ids.begin(), seen_step_task_ids.end(),
                      step_task_id) != seen_step_task_ids.end()) {
            return false;
        }
        seen_step_task_ids.push_back(step_task_id);

        const auto found = records_.find(step_task_id);
        if (found == records_.end() || found->second.steps.empty()) return false;

        aggregate.steps.insert(aggregate.steps.end(),
                               found->second.steps.begin(), found->second.steps.end());
        if (found->second.status == ExecutionStatus::NeedsPermission) {
            aggregate.status = ExecutionStatus::NeedsPermission;
        } else if (found->second.status != ExecutionStatus::Success &&
                   aggregate.status != ExecutionStatus::NeedsPermission) {
            aggregate.status = ExecutionStatus::Failure;
        }
    }
    if (aggregate.steps.empty() || !tracker_.begin(task_id)) return false;

    bool transitioned = false;
    if (aggregate.status == ExecutionStatus::Success) {
        transitioned = tracker_.complete(task_id);
    } else if (aggregate.status == ExecutionStatus::NeedsPermission) {
        transitioned = tracker_.block(task_id);
    } else {
        transitioned = tracker_.fail(task_id);
    }
    if (!transitioned) {
        (void)tracker_.fail(task_id);
        return false;
    }

    retain_record(std::move(aggregate));
    return true;
}

void ExecutionEngine::retain_record(ExecutionRecord record) {
    const auto task_id = record.task_id;
    records_.insert_or_assign(task_id, std::move(record));
    record_order_.push_back(task_id);
    while (record_order_.size() > max_records_) {
        records_.erase(record_order_.front());
        record_order_.erase(record_order_.begin());
    }
}

ExecutionState ExecutionEngine::state_of(const std::string& task_id) const {
    return tracker_.state_of(task_id);
}

std::optional<ExecutionRecord> ExecutionEngine::latest_record() const {
    if (record_order_.empty()) return std::nullopt;
    return record_of(record_order_.back());
}

std::optional<ExecutionRecord> ExecutionEngine::record_of(
    const std::string& task_id) const {
    const auto found = records_.find(task_id);
    if (found == records_.end()) return std::nullopt;
    return found->second;
}

bool ExecutionEngine::set_verification(
    const std::string& task_id, VerificationOutcome outcome) {
    const auto found = records_.find(task_id);
    if (found == records_.end() || found->second.steps.empty()) return false;
    found->second.steps.back().verification = outcome;
    if (outcome == VerificationOutcome::Failed) {
        found->second.status = ExecutionStatus::Failure;
    }
    return true;
}

} // namespace seventh::execution
