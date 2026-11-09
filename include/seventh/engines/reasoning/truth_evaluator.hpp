#pragma once

#include "seventh/core/types.hpp"

namespace seventh::reasoning {

class TruthEvaluator {
public:
    [[nodiscard]] static TruthValue negate(TruthValue value) noexcept;
    [[nodiscard]] static TruthValue conjunction(TruthValue left, TruthValue right) noexcept;
    [[nodiscard]] static TruthValue disjunction(TruthValue left, TruthValue right) noexcept;
};

} // namespace seventh::reasoning
