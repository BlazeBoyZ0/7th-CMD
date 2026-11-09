#pragma once

#include "seventh/core/subsystem_result.hpp"

namespace seventh {

class ResultHandler {
public:
    [[nodiscard]] SubsystemResult normalize(
        SubsystemResult result, const RequestId& request_id) const;
};

} // namespace seventh
