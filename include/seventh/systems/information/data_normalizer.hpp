#pragma once

#include "seventh/systems/information/information_types.hpp"

namespace seventh::information {

class DataNormalizer {
public:
    void normalize(std::vector<DataRecord>& records) const;
};

} // namespace seventh::information
