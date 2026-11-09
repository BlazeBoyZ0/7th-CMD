#include "seventh/engines/reasoning/bounded_search.hpp"
#include "seventh/engines/reasoning/fact_resolver.hpp"
#include "seventh/engines/reasoning/reasoning_engine.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, const std::string& description) {
    if (condition) {
        std::cout << "[PASS] " << description << '\n';
    } else {
        std::cerr << "[FAIL] " << description << '\n';
        ++failures;
    }
}

using namespace seventh;
using namespace seventh::reasoning;

Fact fact(std::string subject, std::string property, std::string value,
          std::string source = "test") {
    Fact result;
    result.subject = std::move(subject);
    result.property = std::move(property);
    result.value = std::move(value);
    result.source = std::move(source);
    return result;
}

void test_complete_search_metadata() {
    const FactQuery query{"file-a", "exists", "", false};
    const std::vector<Fact> facts{
        fact("file-a", "exists", "true"),
        fact("file-b", "exists", "true"),
        fact("file-a", "exists", "true", "second-source")
    };

    const auto result = BoundedSearch{}.collect_relevant(query, facts, 4, 8);
    expect(result.report.complete && !result.report.truncated(),
           "a search within both limits is marked complete");
    expect(result.report.examined == 3 && result.report.matched == 2 &&
               result.candidates.size() == 2,
           "search metadata records examined and relevant fact counts");
}

void test_candidate_limit_is_visible() {
    const FactQuery query{"file-a", "exists", "", false};
    const std::vector<Fact> facts{
        fact("file-a", "exists", "true"),
        fact("file-a", "exists", "true"),
        fact("file-a", "exists", "true")
    };

    const auto result = BoundedSearch{}.collect_relevant(query, facts, 2, 8);
    expect(result.report.truncated() && result.report.candidate_limit_reached,
           "an omitted relevant candidate marks the search incomplete");
    expect(result.candidates.size() == 2 && result.report.matched == 3,
           "the search reports the extra match that exceeded its candidate cap");
}

void test_scan_limit_is_visible() {
    const FactQuery query{"file-a", "exists", "", false};
    const std::vector<Fact> facts{
        fact("other", "exists", "true"),
        fact("file-a", "exists", "true"),
        fact("other-two", "exists", "true"),
        fact("file-a", "exists", "true")
    };

    const auto result = BoundedSearch{}.collect_relevant(query, facts, 8, 2);
    expect(result.report.truncated() && result.report.scan_limit_reached,
           "unexamined input beyond the scan cap marks the search incomplete");
    expect(result.report.examined == 2 && result.candidates.size() == 1,
           "the scan budget limits examined facts without discarding its metadata");
}

void test_zero_limits_are_safe() {
    const FactQuery query{"file-a", "exists", "", false};
    const std::vector<Fact> facts{fact("file-a", "exists", "true")};

    const auto no_candidates = BoundedSearch{}.collect_relevant(query, facts, 0, 8);
    expect(no_candidates.report.truncated() &&
               no_candidates.report.candidate_limit_reached &&
               no_candidates.candidates.empty(),
           "a zero candidate budget cannot silently return a complete empty result");

    const auto no_scan = BoundedSearch{}.collect_relevant(query, facts, 8, 0);
    expect(no_scan.report.truncated() && no_scan.report.scan_limit_reached &&
               no_scan.report.examined == 0,
           "a zero scan budget is explicitly incomplete");
}

void test_fact_resolution_evidence_polarity() {
    const FactQuery query{"file-a", "extension", ".cpp", true};
    const auto resolved = FactResolver{}.resolve(query, {
        fact("file-a", "extension", ".cpp", "filename-index"),
        fact("file-a", "extension", ".txt", "metadata-index")
    });

    expect(resolved.truth == TruthValue::Unknown && resolved.contradiction,
           "conflicting evidence resolves to UNKNOWN");
    expect(resolved.supporting.size() == 1 && resolved.opposing.size() == 1,
           "fact resolution separates supporting and opposing evidence");
    expect(resolved.matches.size() == 2,
           "fact resolution retains all matching-property evidence");
}

void test_complete_absence_and_negative_evidence() {
    ReasoningRequest request;
    request.query = {"missing-file", "exists", "true", true};

    const auto absent = ReasoningEngine{}.evaluate(request);
    expect(absent.truth == TruthValue::Unknown && absent.search.complete,
           "a complete search with no facts remains UNKNOWN rather than proving absence");

    request.facts.push_back(fact("missing-file", "exists", "false"));
    const auto negative = ReasoningEngine{}.evaluate(request);
    expect(negative.truth == TruthValue::False && !negative.search.truncated(),
           "complete explicit negative evidence can establish FALSE");
    expect(negative.opposing_evidence.size() == 1,
           "negative conclusions retain the fact that opposes the query");
}

void test_incomplete_search_never_proves_truth_or_absence() {
    ReasoningRequest request;
    request.query = {"target", "exists", "true", true};
    request.facts = {
        fact("target", "exists", "true"),
        fact("target", "exists", "false"),
        fact("target", "exists", "true")
    };
    request.max_candidates = 1;

    const auto candidate_limited = ReasoningEngine{}.evaluate(request);
    expect(candidate_limited.truth == TruthValue::Unknown &&
               candidate_limited.search.candidate_limit_reached &&
               candidate_limited.budget_exhausted,
           "candidate truncation prevents a definitive truth value");
    expect(!candidate_limited.evidence.empty(),
           "partial evidence remains available for diagnostics");

    request.max_candidates = 8;
    request.max_facts_examined = 1;
    const auto scan_limited = ReasoningEngine{}.evaluate(request);
    expect(scan_limited.truth == TruthValue::Unknown &&
               scan_limited.search.scan_limit_reached,
           "scan truncation prevents a definitive truth value even after a positive match");
}

void test_rule_budget_incompleteness() {
    ReasoningRequest request;
    request.query = {"target", "available", "true", true};
    request.facts = {fact("seed", "ready", "true")};

    Rule first;
    first.id = "derive-middle";
    first.premises.push_back(
        {"seed", "ready", "true", ConditionOperator::Equal});
    first.conclusion = fact("middle", "ready", "true");

    Rule second;
    second.id = "derive-target";
    second.premises.push_back(
        {"middle", "ready", "true", ConditionOperator::Equal});
    second.conclusion = fact("target", "available", "true");

    request.rules = {first, second};
    request.max_rule_evaluations = 1;

    const auto result = ReasoningEngine{}.evaluate(request);
    expect(result.truth == TruthValue::Unknown &&
               result.rule_budget_exhausted && result.budget_exhausted,
           "an incomplete rule pass cannot produce a definitive conclusion");
    expect(result.derived_facts.size() == 1,
           "facts derived before budget exhaustion remain inspectable");
}

void test_rule_budget_can_complete_exactly() {
    ReasoningRequest request;
    request.query = {"target", "available", "true", true};
    request.facts = {fact("seed", "ready", "true")};

    Rule rule;
    rule.id = "derive-target";
    rule.premises.push_back(
        {"seed", "ready", "true", ConditionOperator::Equal});
    rule.conclusion = fact("target", "available", "true");
    request.rules = {rule};
    request.max_rule_evaluations = 1;

    const auto result = ReasoningEngine{}.evaluate(request);
    expect(result.truth == TruthValue::True && !result.rule_budget_exhausted,
           "a rule set that finishes exactly at its budget is still complete");
    expect(result.supporting_evidence.size() == 1 &&
               result.supporting_evidence.front().source == "rule:derive-target",
           "derived evidence preserves its rule provenance");
}

} // namespace

int main() {
    test_complete_search_metadata();
    test_candidate_limit_is_visible();
    test_scan_limit_is_visible();
    test_zero_limits_are_safe();
    test_fact_resolution_evidence_polarity();
    test_complete_absence_and_negative_evidence();
    test_incomplete_search_never_proves_truth_or_absence();
    test_rule_budget_incompleteness();
    test_rule_budget_can_complete_exactly();

    if (failures != 0) {
        std::cerr << failures << " reasoning test(s) failed.\n";
        return EXIT_FAILURE;
    }

    std::cout << "All bounded reasoning tests passed.\n";
    return EXIT_SUCCESS;
}
