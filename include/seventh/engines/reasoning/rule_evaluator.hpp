#pragma once

#include "seventh/engines/reasoning/reasoning_types.hpp"

#include <cstddef>
#include <vector>

namespace seventh::reasoning {

struct RuleEvaluation {
    std::vector<Fact> derived;
    std::size_t evaluations{0};
    bool budget_exhausted{false};
};

class RuleEvaluator {
public:
    [[nodiscard]] RuleEvaluation evaluate(const std::vector<Rule>& rules,
                                           const std::vector<Fact>& facts,
                                           std::size_t budget) const;
};

} // namespace seventh::reasoning
