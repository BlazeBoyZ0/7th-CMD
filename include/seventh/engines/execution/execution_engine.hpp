#pragma once
#include "seventh/engines/execution/action_dispatcher.hpp"
#include "seventh/engines/execution/execution_tracker.hpp"
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>
namespace seventh::execution {
class ExecutionEngine {
public:
 void register_executor(std::shared_ptr<OperationExecutor> executor);
 [[nodiscard]] ExecutionResult execute(const ExecutionRequest& request);
 [[nodiscard]] bool record_terminal_failure(const std::string& task_id, const std::string& operation, const std::string& error, const std::string& cir_id = {});
 [[nodiscard]] ExecutionState state_of(const std::string& task_id) const;
 [[nodiscard]] std::optional<ExecutionRecord> record_of(const std::string& task_id) const;
 [[nodiscard]] std::optional<ExecutionRecord> latest_record() const;
 [[nodiscard]] bool set_verification(const std::string& task_id, VerificationOutcome outcome);
private:
 void retain_record(ExecutionRecord record);
 static constexpr std::size_t max_records_{100};
 ActionDispatcher dispatcher_;
 ExecutionTracker tracker_;
 std::map<std::string,ExecutionRecord> records_;
 std::vector<std::string> record_order_;
};
}