#pragma once

#include "seventh/core/types.hpp"

#include <map>
#include <string>

namespace seventh::knowledge {

struct TaskContext {
    TaskId task_id;
    std::map<std::string, std::string> values;
    StringList unresolved_information;
};

} // namespace seventh::knowledge
