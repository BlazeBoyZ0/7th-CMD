#include "seventh/engines/reasoning/reasoning_engine.hpp"

#include <algorithm>
#include <utility>

namespace seventh::reasoning {

bool ReasoningEngine::load_rules_from_file(
    const std::string& path, std::string& error, bool merge) {
    std::vector<Rule> loaded;
    if (!rule_loader_.load_file(path, loaded, error)) return false;

    if (!merge) {
        configured_rules_ = std::move(loaded);
    } else {
        for (auto& rule : loaded) {
            const auto existing = std::find_if(
                configured_rules_.begin(), configured_rules_.end(),
                [&](const Rule& candidate) { return candidate.id == rule.id; });
            if (existing == configured_rules_.end()) {
                configured_rules_.push_back(std::move(rule));
            } else {
                *existing = std::move(rule);
            }
        }
    }
    error.clear();
    return true;
}

ReasoningResult ReasoningEngine::evaluate(const ReasoningRequest& request) const {
    ReasoningResult result;

    const auto candidates = bounded_search_.collect_relevant(
        request.query, request.facts, request.max_candidates);
    const auto direct = fact_resolver_.resolve(request.query, candidates);

    auto active_rules = configured_rules_;
    active_rules.insert(active_rules.end(), request.rules.begin(), request.rules.end());
    const auto rules = rule_evaluator_.evaluate(
        active_rules, request.facts, request.max_rule_evaluations);
    result.rule_evaluations = rules.evaluations;
    result.budget_exhausted = rules.budget_exhausted;
    result.derived_facts = rules.derived;

    FactResolution derived;
    if (!result.derived_facts.empty()) {
        derived = fact_resolver_.resolve(request.query, result.derived_facts);
    }

    result.evidence = direct.matches;
    result.evidence.insert(result.evidence.end(),
                           derived.matches.begin(), derived.matches.end());

    const bool opposing_conclusions =
        (direct.truth == TruthValue::True && derived.truth == TruthValue::False) ||
        (direct.truth == TruthValue::False && derived.truth == TruthValue::True);

    const bool conflict = direct.contradiction || derived.contradiction ||
        opposing_conclusions || contradiction_handler_.contradictory(result.evidence);

    if (conflict) {
        result.truth = TruthValue::Unknown;
        result.explanation = "Conflicting fresh facts prevent a reliable conclusion.";
    } else if (direct.truth != TruthValue::Unknown) {
        result.truth = direct.truth;
    } else {
        result.truth = derived.truth;
    }

    if (result.truth == TruthValue::Unknown && result.budget_exhausted) {
        result.explanation = "The reasoning work budget was exhausted before a conclusion.";
    } else if (result.truth == TruthValue::True && result.explanation.empty()) {
        result.explanation = "The available evidence supports the requested fact.";
    } else if (result.truth == TruthValue::False && result.explanation.empty()) {
        result.explanation = "The available evidence contradicts the requested fact.";
    }

    result.confidence = confidence_evaluator_.evaluate(result.evidence);
    uncertainty_handler_.explain_unknown(result, request.query);
    return result;
}

} // namespace seventh::reasoning
