#pragma once

#include "seventh/engines/execution/execution_types.hpp"

#include <map>
#include <string>

namespace seventh::execution {

class ExecutionTracker {
public:
    [[nodiscard]] bool begin(const std::string& task_id);
    [[nodiscard]] bool complete(const std::string& task_id);
    [[nodiscard]] bool block(const std::string& task_id);
    [[nodiscard]] bool fail(const std::string& task_id);
    [[nodiscard]] ExecutionState state_of(const std::string& task_id) const;

private:
    std::map<std::string, ExecutionState> states_;
};

} // namespace seventh::execution
