#include "seventh/engines/execution/execution_engine.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>

class SuccessfulExecutor final : public seventh::execution::OperationExecutor {
public:
    std::string_view id() const noexcept override { return "test"; }
    bool supports(const seventh::execution::ExecutionRequest&) const override { return true; }
    seventh::execution::ExecutionResult execute(
        const seventh::execution::ExecutionRequest& request) override {
        seventh::execution::ExecutionResult result;
        result.status = seventh::execution::ExecutionStatus::Success;
        result.output_data["path"] = request.input_data.at("path");
        return result;
    }
};

int main() {
    using namespace seventh::execution;
    int failures = 0;
    const auto check = [&failures](bool condition, const char* message) {
        std::cout << (condition ? "[PASS] " : "[FAIL] ") << message << '\\n';
        if (!condition) ++failures;
    };

    ExecutionEngine engine;
    engine.register_executor(std::make_shared<SuccessfulExecutor>());

    ExecutionRequest request;
    request.task_id = "record-task-1";
    request.operation = "create_directory";
    request.input_data["path"] = "/tmp/example";
    request.cir_id = "cir-1";
    request.plan_id = "plan-1";
    request.plan_step_id = "step-1";

    const auto result = engine.execute(request);
    auto record = engine.record_of(request.task_id);
    check(result.status == ExecutionStatus::Success, "supported operation succeeds");
    check(engine.state_of(request.task_id) == ExecutionState::Completed,
          "successful execution reaches the completed tracker state");
    check(record.has_value(), "execution record is retained in memory");
    if (record) {
        check(record->cir_id == "cir-1" && record->plan_id == "plan-1",
              "record references CIR and plan");
        check(record->steps.size() == 1 && record->steps[0].step_id == "step-1",
              "record identifies the attempted plan step");
        check(record->steps[0].outcome == StepOutcome::Succeeded &&
                  record->steps[0].output_data.at("path") == "/tmp/example",
              "record stores actual outcome and executor output");
        check(record->steps[0].verification == VerificationOutcome::NotRun,
              "execution does not falsely claim orchestration verification");
    }

    ExecutionRequest duplicate = request;
    duplicate.input_data["path"] = "/tmp/overwritten";
    const auto duplicate_result = engine.execute(duplicate);
    const auto preserved = engine.record_of(request.task_id);
    check(duplicate_result.status == ExecutionStatus::Failure,
          "a duplicate task identifier is rejected");
    check(preserved && preserved->steps[0].output_data.at("path") == "/tmp/example",
          "a rejected duplicate cannot overwrite the original execution record");

    ExecutionRequest blocked;
    blocked.task_id = "record-task-2";
    blocked.operation = "delete_path";
    blocked.requires_authorization = true;
    const auto denied = engine.execute(blocked);
    const auto blocked_record = engine.record_of(blocked.task_id);
    check(denied.status == ExecutionStatus::NeedsPermission &&
              engine.state_of(blocked.task_id) == ExecutionState::Blocked &&
              blocked_record && blocked_record->steps[0].outcome == StepOutcome::Blocked,
          "authorization blocks are recorded and reflected in tracker state");

    ExecutionRequest verification_failure;
    verification_failure.task_id = "record-task-3";
    verification_failure.operation = "create_directory";
    verification_failure.input_data["path"] = "/tmp/unverified";
    check(engine.execute(verification_failure).status == ExecutionStatus::Success,
          "operation can succeed before postcondition verification");
    check(engine.set_verification(verification_failure.task_id, VerificationOutcome::Failed),
          "postcondition verification can be attached to its execution record");
    const auto failed_verification = engine.record_of(verification_failure.task_id);
    check(failed_verification &&
              failed_verification->status == ExecutionStatus::Failure &&
              failed_verification->steps[0].verification == VerificationOutcome::Failed,
          "failed verification is reflected in the final execution record");

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "All execution-record tests passed.\\n";
    return EXIT_SUCCESS;
}
