#include "seventh/engines/reasoning/rule_evaluator.hpp"

#include "seventh/engines/reasoning/truth_evaluator.hpp"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

namespace seventh::reasoning {
namespace {

struct ConditionEvaluation {
    TruthValue truth{TruthValue::Unknown};
    unsigned confidence{0};
    std::uint64_t latest_observation{0};
    bool has_support{false};
    bool budget_exhausted{false};
};

ConditionEvaluation evaluate_condition(const RuleCondition& condition,
                                       const std::vector<Fact>& facts,
                                       std::size_t& facts_examined,
                                       std::size_t max_facts_examined) {
    ConditionEvaluation result;
    bool found_property = false;
    bool found_equal = false;
    bool found_different = false;

    for (const auto& fact : facts) {
        if (facts_examined >= max_facts_examined) {
            result.budget_exhausted = true;
            return result;
        }
        ++facts_examined;

        if (!fact.fresh || fact.subject != condition.subject ||
            fact.property != condition.property) {
            continue;
        }

        found_property = true;
        if (fact.value == condition.value) {
            found_equal = true;
            result.has_support = true;
        } else {
            found_different = true;
            if (condition.operation == ConditionOperator::NotEqual) {
                result.has_support = true;
            }
        }

        if (result.has_support) {
            result.confidence = std::max(result.confidence, fact.confidence);
            result.latest_observation =
                std::max(result.latest_observation, fact.observed_at);
        }
        if (condition.operation == ConditionOperator::Exists) {
            result.has_support = true;
            result.confidence = std::max(result.confidence, fact.confidence);
            result.latest_observation =
                std::max(result.latest_observation, fact.observed_at);
        }
    }

    switch (condition.operation) {
    case ConditionOperator::Equal:
        if (!found_property) result.truth = TruthValue::Unknown;
        else if (found_equal && found_different) result.truth = TruthValue::Unknown;
        else result.truth = found_equal ? TruthValue::True : TruthValue::False;
        break;

    case ConditionOperator::NotEqual:
        if (!found_property) result.truth = TruthValue::Unknown;
        else if (found_equal && found_different) result.truth = TruthValue::Unknown;
        else result.truth = found_equal ? TruthValue::False : TruthValue::True;
        break;

    case ConditionOperator::Exists:
        result.truth = found_property ? TruthValue::True : TruthValue::Unknown;
        break;
    }

    return result;
}

bool same_fact(const Fact& left, const Fact& right) {
    return left.subject == right.subject &&
           left.property == right.property &&
           left.value == right.value;
}

} // namespace

RuleEvaluation RuleEvaluator::evaluate(const std::vector<Rule>& rules,
                                        const std::vector<Fact>& facts,
                                        std::size_t budget,
                                        std::size_t max_facts_examined) const {
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
                const auto evaluation = evaluate_condition(
                    condition, known, result.facts_examined,
                    max_facts_examined);
                if (evaluation.budget_exhausted) {
                    result.fact_budget_exhausted = true;
                    return result;
                }

                premises = TruthEvaluator::conjunction(premises, evaluation.truth);
                if (premises == TruthValue::False) break;

                if (evaluation.truth == TruthValue::True && evaluation.has_support) {
                    premise_confidence =
                        std::min(premise_confidence, evaluation.confidence);
                    latest_observation =
                        std::max(latest_observation, evaluation.latest_observation);
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
