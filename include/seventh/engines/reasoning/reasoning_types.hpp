#pragma once

#include "seventh/core/types.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace seventh::reasoning {

struct Fact {
    std::string subject;
    std::string property;
    std::string value;
    std::string source;
    std::uint64_t observed_at{0};
    unsigned confidence{100};
    bool fresh{true};
};

enum class ConditionOperator {
    Equal,
    NotEqual,
    Exists
};

struct RuleCondition {
    std::string subject;
    std::string property;
    std::string value;
    ConditionOperator operation{ConditionOperator::Equal};
};

struct Rule {
    std::string id;
    std::vector<RuleCondition> premises;
    Fact conclusion;
};

struct FactQuery {
    std::string subject;
    std::string property;
    std::string value;
    bool match_value{false};
};

struct SearchReport {
    std::size_t examined{0};
    std::size_t matched{0};
    std::size_t candidate_limit{0};
    std::size_t scan_limit{0};
    bool candidate_limit_reached{false};
    bool scan_limit_reached{false};
    bool complete{true};

    [[nodiscard]] bool truncated() const noexcept {
        return !complete;
    }
};

struct BoundedSearchResult {
    std::vector<Fact> candidates;
    SearchReport report;
};

struct ReasoningRequest {
    FactQuery query;
    std::vector<Fact> facts;
    std::vector<Rule> rules;
    std::size_t max_candidates{256};
    std::size_t max_rule_evaluations{128};
    std::size_t max_facts_examined{1024};
    std::size_t max_rule_facts_examined{4096};
};

struct ReasoningResult {
    TruthValue truth{TruthValue::Unknown};
    unsigned confidence{0};
    std::vector<Fact> evidence;
    std::vector<Fact> supporting_evidence;
    std::vector<Fact> opposing_evidence;
    std::vector<Fact> derived_facts;
    StringList unresolved_dependencies;
    std::string explanation;
    SearchReport search;
    std::size_t rule_evaluations{0};
    std::size_t rule_facts_examined{0};
    bool rule_budget_exhausted{false};
    bool rule_fact_budget_exhausted{false};
    bool budget_exhausted{false};
};

} // namespace seventh::reasoning
