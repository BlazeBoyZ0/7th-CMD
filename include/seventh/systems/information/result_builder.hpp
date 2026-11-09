#pragma once

#include "seventh/systems/information/information_types.hpp"

namespace seventh::information {

class ResultBuilder {
public:
    void build_summary(DataResult& result, const std::string& operation) const;
};

} // namespace seventh::information
