#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>
namespace seventh::permissions { struct AuthorizedRoot; }
namespace seventh::execution {
enum class ExecutionStatus { Success, NeedsPermission, Unsupported, Failure };
enum class ExecutionState { Pending, Running, Completed, Blocked, Failed };
enum class StepOutcome { Attempted, Succeeded, Blocked, Failed };
enum class VerificationOutcome { NotRun, Passed, Failed };
struct ExecutionRequest {
 std::string task_id, operation;
 std::map<std::string,std::string> input_data;
 bool requires_authorization{false}, authorization_granted{false};
 std::shared_ptr<const permissions::AuthorizedRoot> authorized_root, authorized_destination_root;
 std::string cir_id, plan_id, plan_step_id;
};
struct ExecutionResult { ExecutionStatus status{ExecutionStatus::Failure}; std::map<std::string,std::string> output_data; std::string error; };
struct ExecutionStepRecord {
 std::string step_id, operation;
 StepOutcome outcome{StepOutcome::Attempted};
 VerificationOutcome verification{VerificationOutcome::NotRun};
 std::map<std::string,std::string> input_data, output_data;
 std::string error;
};
struct ExecutionRecord {
 std::string record_id, task_id, cir_id, plan_id;
 ExecutionStatus status{ExecutionStatus::Failure};
 std::vector<ExecutionStepRecord> steps;
};
}