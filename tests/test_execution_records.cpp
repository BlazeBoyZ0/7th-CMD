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
        if (request.operation == "search_files") {
            result.output_data["count"] = "3";
            result.output_data["displayed"] = "3";
            result.output_data["truncated"] = "false";
            result.output_data["matches"] = "alpha.txt | /tmp/search/alpha.txt";
        } else {
            result.output_data["path"] = request.input_data.at("path");
        }
        return result;
    }
};

int main() {
    using namespace seventh::execution;
    int failures = 0;
    const auto check = [&failures](bool condition, const char* message) {
        std::cout << (condition ? "[PASS] " : "[FAIL] ") << message << '\n';
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

    ExecutionRequest verification_success;
    verification_success.task_id = "record-task-verified";
    verification_success.operation = "create_directory";
    verification_success.input_data["path"] = "/tmp/verified";
    check(engine.execute(verification_success).status == ExecutionStatus::Success,
          "operation succeeds before successful postcondition verification");
    check(engine.set_verification(verification_success.task_id, VerificationOutcome::Passed),
          "successful postcondition verification can be attached to its record");
    const auto passed_verification = engine.record_of(verification_success.task_id);
    check(passed_verification &&
              passed_verification->status == ExecutionStatus::Success &&
              passed_verification->steps[0].verification == VerificationOutcome::Passed,
          "successful verification preserves the successful execution status");

    ExecutionRequest search_request;
    search_request.task_id = "record-search-1";
    search_request.operation = "search_files";
    search_request.input_data["path"] = "/tmp/search";
    search_request.input_data["query"] = "alpha";
    search_request.cir_id = "search-cir-1";
    search_request.plan_id = "search-plan-1";
    search_request.plan_step_id = "search-step-1";
    const auto search_result = engine.execute(search_request);
    const auto search_record = engine.record_of(search_request.task_id);
    check(search_result.status == ExecutionStatus::Success && search_record,
          "search operation is executed and receives an execution record");
    if (search_record) {
        check(search_record->cir_id == "search-cir-1" &&
                  search_record->plan_id == "search-plan-1" &&
                  search_record->steps[0].operation == "search_files",
              "search record retains CIR, plan, and operation traceability");
        check(search_record->steps[0].output_data.at("count") == "3",
              "search record stores the actual search result summary");
    }

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

    check(engine.record_terminal_failure(
              "preflight-failure", "create", "Target already exists.", "cir-preflight"),
          "a pre-execution command failure can be retained");
    const auto preflight_failure = engine.record_of("preflight-failure");
    check(preflight_failure &&
              preflight_failure->status == ExecutionStatus::Failure &&
              preflight_failure->cir_id == "cir-preflight" &&
              preflight_failure->steps.size() == 1 &&
              preflight_failure->steps[0].outcome == StepOutcome::Failed &&
              preflight_failure->steps[0].error == "Target already exists.",
          "pre-execution failure records preserve task, CIR, outcome, and reason");
    check(!engine.record_terminal_failure(
              "preflight-failure", "create", "Overwrite attempt.", "cir-other"),
          "a later preflight failure cannot overwrite an existing record");

    const auto latest_before_bound = engine.latest_record();
    check(latest_before_bound && latest_before_bound->task_id == "preflight-failure",
          "latest_record returns the most recently retained execution");
    check(!engine.record_of("missing-task").has_value(),
          "unknown task identifiers do not produce execution records");

    for (int index = 0; index < 101; ++index) {
        ExecutionRequest bounded;
        bounded.task_id = "bounded-task-" + std::to_string(index);
        bounded.operation = "create_directory";
        bounded.input_data["path"] = "/tmp/bounded";
        (void)engine.execute(bounded);
    }
    check(!engine.record_of("record-task-1").has_value(),
          "record retention evicts the oldest entry after reaching its bound");
    const auto latest_bounded = engine.latest_record();
    check(latest_bounded && latest_bounded->task_id == "bounded-task-100",
          "latest_record remains correct after bounded-history eviction");

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "All execution-record tests passed.\n";
    return EXIT_SUCCESS;
}
