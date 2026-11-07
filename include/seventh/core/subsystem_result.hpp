#pragma once

#include "seventh/core/status.hpp"
#include "seventh/core/types.hpp"

#include <map>
#include <string>

namespace seventh {

struct SubsystemResult {
    RequestId request_id;
    SubsystemStatus status{SubsystemStatus::Failure};
    std::map<std::string, std::string> result_data;
    StringList new_requirements;
    StringList missing_information;
    std::string error;

    [[nodiscard]] bool successful() const noexcept;
};

} // namespace seventh
