#pragma once

#include "seventh/engines/reasoning/reasoning_types.hpp"

#include <cstddef>
#include <vector>

namespace seventh::reasoning {

struct RuleEvaluation {
    std::vector<Fact> derived;
    std::size_t evaluations{0};
    std::size_t facts_examined{0};
    bool budget_exhausted{false};
    bool fact_budget_exhausted{false};
};

class RuleEvaluator {
public:
    [[nodiscard]] RuleEvaluation evaluate(const std::vector<Rule>& rules,
                                           const std::vector<Fact>& facts,
                                           std::size_t budget,
                                           std::size_t max_facts_examined = 4096) const;
};

} // namespace seventh::reasoning
