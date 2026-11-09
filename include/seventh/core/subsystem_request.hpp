#pragma once

#include "seventh/core/types.hpp"

#include <atomic>
#include <map>
#include <memory>
#include <string>

namespace seventh {

struct SubsystemRequest {
    RequestId request_id;
    TaskId task_id;
    std::string operation;
    std::map<std::string, std::string> input_data;
    std::map<std::string, std::string> context;
    StringList constraints;
    std::map<std::string, std::string> metadata;
    std::shared_ptr<std::atomic_bool> cancellation_requested;

    [[nodiscard]] bool valid() const noexcept;
};

} // namespace seventh
