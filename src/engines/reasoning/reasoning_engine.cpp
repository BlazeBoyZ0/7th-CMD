#include "seventh/engines/reasoning/reasoning_engine.hpp"

namespace seventh::reasoning {

ReasoningResult ReasoningEngine::evaluate(const ReasoningRequest& request) const {
    ReasoningResult result;
    const auto candidates = bounded_search_.collect_relevant(
        request.query, request.facts, request.facts.size());

    const auto direct = fact_resolver_.resolve(request.query, candidates);
    result.evidence = direct.matches;
    result.truth = direct.truth;

    const auto rules = rule_evaluator_.evaluate(
        request.rules, request.facts, request.max_rule_evaluations);
    result.rule_evaluations = rules.evaluations;
    result.budget_exhausted = rules.budget_exhausted;
    result.derived_facts = rules.derived;

    if (result.truth == TruthValue::Unknown && !result.derived_facts.empty()) {
        const auto derived = fact_resolver_.resolve(request.query, result.derived_facts);
        result.truth = derived.truth;
        result.evidence.insert(result.evidence.end(),
                               derived.matches.begin(), derived.matches.end());
    }

    const bool conflict = direct.contradiction ||
        contradiction_handler_.contradictory(result.evidence);
    if (conflict) {
        result.truth = TruthValue::Unknown;
        result.explanation = "Conflicting fresh facts prevent a reliable conclusion.";
    } else if (result.budget_exhausted && result.truth == TruthValue::Unknown) {
        result.explanation = "The reasoning work budget was exhausted before a conclusion.";
    } else if (result.truth == TruthValue::True) {
        result.explanation = "The available evidence supports the requested fact.";
    } else if (result.truth == TruthValue::False) {
        result.explanation = "The available evidence contradicts the requested fact.";
    }

    result.confidence = confidence_evaluator_.evaluate(result.evidence);
    uncertainty_handler_.explain_unknown(result, request.query);
    return result;
}

} // namespace seventh::reasoning
