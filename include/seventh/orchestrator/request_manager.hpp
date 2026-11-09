#pragma once

#include "seventh/core/subsystem_request.hpp"

namespace seventh {

class RequestManager {
public:
    [[nodiscard]] SubsystemRequest create(
        const std::string& operation,
        std::map<std::string, std::string> input_data,
        std::map<std::string, std::string> context,
        StringList constraints) const;
};

} // namespace seventh
