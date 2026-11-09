#include "seventh/engines/reasoning/rule_evaluator.hpp"

#include "seventh/engines/reasoning/truth_evaluator.hpp"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

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

struct ConditionSupport {
    unsigned confidence{0};
    std::uint64_t latest_observation{0};
    bool found{false};
};

ConditionSupport supporting_evidence(const RuleCondition& condition,
                                     const std::vector<Fact>& facts) {
    ConditionSupport support;

    for (const auto& fact : facts) {
        if (!fact.fresh || fact.subject != condition.subject ||
            fact.property != condition.property) {
            continue;
        }

        bool supports = false;
        switch (condition.operation) {
        case ConditionOperator::Equal:
            supports = fact.value == condition.value;
            break;
        case ConditionOperator::NotEqual:
            supports = fact.value != condition.value;
            break;
        case ConditionOperator::Exists:
            supports = true;
            break;
        }

        if (!supports) continue;
        support.found = true;
        support.confidence = std::max(support.confidence, fact.confidence);
        support.latest_observation =
            std::max(support.latest_observation, fact.observed_at);
    }

    return support;
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
    std::vector<bool> fired(rules.size(), false);

    // Revisit unresolved rules only when another rule adds a fact. A rule
    // that has already fired cannot add anything new on a later pass.
    bool added_fact = true;
    while (added_fact) {
        added_fact = false;

        for (std::size_t index = 0; index < rules.size(); ++index) {
            if (fired[index]) continue;
            if (result.evaluations >= budget) {
                result.budget_exhausted = true;
                return result;
            }

            ++result.evaluations;
            const auto& rule = rules[index];

            if (rule.premises.empty() ||
                rule.conclusion.subject.empty() ||
                rule.conclusion.property.empty()) {
                fired[index] = true;
                continue;
            }

            TruthValue premises = TruthValue::True;
            unsigned premise_confidence = 100;
            std::uint64_t latest_observation = 0;

            for (const auto& condition : rule.premises) {
                const auto truth = evaluate_condition(condition, known);
                premises = TruthEvaluator::conjunction(premises, truth);
                if (premises == TruthValue::False) break;

                if (truth == TruthValue::True) {
                    const auto support = supporting_evidence(condition, known);
                    if (support.found) {
                        premise_confidence =
                            std::min(premise_confidence, support.confidence);
                        latest_observation =
                            std::max(latest_observation, support.latest_observation);
                    }
                }
            }

            if (premises != TruthValue::True) continue;

            Fact conclusion = rule.conclusion;
            conclusion.confidence =
                std::min(conclusion.confidence, premise_confidence);
            conclusion.observed_at =
                std::max(conclusion.observed_at, latest_observation);

            // A derived fact must identify the rule that produced it.
            // The conclusion template's source describes its declaration,
            // not the evidence provenance of this derived instance.
            conclusion.source = rule.id.empty() ? "rule:anonymous"
                                                : "rule:" + rule.id;

            const auto duplicate = std::find_if(
                known.begin(), known.end(),
                [&](const Fact& existing) {
                    return same_fact(existing, conclusion);
                });
            fired[index] = true;
            if (duplicate != known.end()) continue;

            known.push_back(conclusion);
            result.derived.push_back(std::move(conclusion));
            added_fact = true;
        }
    }

    return result;
}

} // namespace seventh::reasoning
