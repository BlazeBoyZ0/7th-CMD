#pragma once

#include "seventh/core/types.hpp"

#include <map>
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

    [[nodiscard]] bool valid() const noexcept;
};

} // namespace seventh
