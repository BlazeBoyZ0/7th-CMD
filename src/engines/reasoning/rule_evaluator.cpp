#include "seventh/engines/reasoning/rule_evaluator.hpp"

#include <algorithm>

namespace seventh::reasoning {
namespace {

bool condition_matches(const RuleCondition& condition, const std::vector<Fact>& facts) {
    bool found_property = false;
    bool found_equal = false;
    for (const auto& fact : facts) {
        if (!fact.fresh || fact.subject != condition.subject ||
            fact.property != condition.property) continue;
        found_property = true;
        if (fact.value == condition.value) found_equal = true;
    }

    switch (condition.operation) {
    case ConditionOperator::Equal: return found_equal;
    case ConditionOperator::NotEqual: return found_property && !found_equal;
    case ConditionOperator::Exists: return found_property;
    }
    return false;
}

} // namespace

RuleEvaluation RuleEvaluator::evaluate(const std::vector<Rule>& rules,
                                        const std::vector<Fact>& facts,
                                        std::size_t budget) const {
    RuleEvaluation result;
    for (const auto& rule : rules) {
        if (result.evaluations >= budget) {
            result.budget_exhausted = true;
            break;
        }
        ++result.evaluations;
        if (rule.premises.empty()) continue;

        const bool satisfied = std::all_of(
            rule.premises.begin(), rule.premises.end(),
            [&facts](const RuleCondition& condition) {
                return condition_matches(condition, facts);
            });
        if (satisfied && !rule.conclusion.subject.empty() &&
            !rule.conclusion.property.empty()) {
            result.derived.push_back(rule.conclusion);
        }
    }
    return result;
}

} // namespace seventh::reasoning
