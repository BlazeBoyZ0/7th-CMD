#pragma once

#include "seventh/engines/reasoning/reasoning_types.hpp"

#include <vector>

namespace seventh::reasoning {

class ConfidenceEvaluator {
public:
    [[nodiscard]] unsigned evaluate(const std::vector<Fact>& evidence) const noexcept;
};

} // namespace seventh::reasoning
