#pragma once

#include "seventh/engines/reasoning/reasoning_types.hpp"

namespace seventh::reasoning {

class UncertaintyHandler {
public:
    void explain_unknown(ReasoningResult& result, const FactQuery& query) const;
};

} // namespace seventh::reasoning
