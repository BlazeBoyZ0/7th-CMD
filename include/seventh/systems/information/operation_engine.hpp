#pragma once

#include "seventh/systems/information/information_types.hpp"

namespace seventh::information {

class OperationEngine {
public:
    [[nodiscard]] DataResult execute(const DataRequest& request,
                                     const std::vector<DataRecord>& records) const;
};

} // namespace seventh::information
