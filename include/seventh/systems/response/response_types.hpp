#pragma once

#include "seventh/core/types.hpp"

#include <map>
#include <string>

namespace seventh::response {

struct ResponseData {
    TaskId task_id;
    TaskState state{TaskState::New};
    std::string operation;
    std::map<std::string, std::string> details;
    StringList missing_information;
    std::string error;
};

} // namespace seventh::response
