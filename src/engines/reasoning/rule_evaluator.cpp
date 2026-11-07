#include "seventh/engines/reasoning/rule_evaluator.hpp"

#include "seventh/engines/reasoning/truth_evaluator.hpp"

#include <algorithm>
#include <utility>

namespace seventh::reasoning {
namespace {

TruthValue evaluate_condition(const RuleCondition& condition,
                              const std::vector<Fact>& facts) {
    bool found_property = false;
    bool found_equal = false;
    bool found_different = false;

    for (const auto& fact : facts) {
        if (!fact.fresh || fact.subject != condition.subject ||
            fact.property != condition.property) {
            continue;
        }

        found_property = true;
        if (fact.value == condition.value) {
            found_equal = true;
        } else {
            found_different = true;
        }
    }

    switch (condition.operation) {
    case ConditionOperator::Equal:
        if (!found_property) return TruthValue::Unknown;
        if (found_equal && found_different) return TruthValue::Unknown;
        return found_equal ? TruthValue::True : TruthValue::False;

    case ConditionOperator::NotEqual:
        if (!found_property) return TruthValue::Unknown;
        if (found_equal && found_different) return TruthValue::Unknown;
        return found_equal ? TruthValue::False : TruthValue::True;

    case ConditionOperator::Exists:
        return found_property ? TruthValue::True : TruthValue::Unknown;
    }

    return TruthValue::Unknown;
}

bool same_fact(const Fact& left, const Fact& right) {
    return left.subject == right.subject &&
           left.property == right.property &&
           left.value == right.value;
}

} // namespace

RuleEvaluation RuleEvaluator::evaluate(const std::vector<Rule>& rules,
                                        const std::vector<Fact>& facts,
                                        std::size_t budget) const {
    RuleEvaluation result;
    std::vector<Fact> known = facts;

    // Revisit rules when a new conclusion is added, allowing bounded,
    // deterministic multi-step deductions without recursive rule execution.
    bool added_fact = true;
    while (added_fact) {
        added_fact = false;

        for (const auto& rule : rules) {
            if (result.evaluations >= budget) {
                result.budget_exhausted = true;
                return result;
            }
            ++result.evaluations;

            if (rule.premises.empty() ||
                rule.conclusion.subject.empty() ||
                rule.conclusion.property.empty()) {
                continue;
            }

            TruthValue premises = TruthValue::True;
            for (const auto& condition : rule.premises) {
                premises = TruthEvaluator::conjunction(
                    premises, evaluate_condition(condition, known));
                if (premises == TruthValue::False) break;
            }

            if (premises != TruthValue::True) continue;

            Fact conclusion = rule.conclusion;
            if (conclusion.source.empty()) {
                conclusion.source = rule.id.empty() ? "rule:anonymous"
                                                    : "rule:" + rule.id;
            }

            const auto duplicate = std::find_if(
                known.begin(), known.end(),
                [&](const Fact& existing) {
                    return same_fact(existing, conclusion);
                });
            if (duplicate != known.end()) continue;

            known.push_back(conclusion);
            result.derived.push_back(std::move(conclusion));
            added_fact = true;
        }
    }

    return result;
}

} // namespace seventh::reasoning
