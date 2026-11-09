#pragma once

#include "seventh/engines/reasoning/reasoning_types.hpp"

#include <vector>

namespace seventh::reasoning {

class ContradictionHandler {
public:
    [[nodiscard]] bool contradictory(const std::vector<Fact>& facts) const noexcept;
};

} // namespace seventh::reasoning
