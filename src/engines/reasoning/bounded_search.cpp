#include "seventh/engines/reasoning/bounded_search.hpp"

#include <algorithm>

namespace seventh::reasoning {

BoundedSearchResult BoundedSearch::collect_relevant(
    const FactQuery& query, const std::vector<Fact>& facts,
    std::size_t max_candidates, std::size_t max_facts_examined) const {
    BoundedSearchResult result;
    result.candidates.reserve(std::min(max_candidates, facts.size()));
    result.report.candidate_limit = max_candidates;
    result.report.scan_limit = max_facts_examined;

    for (const auto& fact : facts) {
        if (result.report.examined >= max_facts_examined) {
            result.report.scan_limit_reached = true;
            result.report.complete = false;
            break;
        }

        ++result.report.examined;
        if (!fact.fresh || fact.subject != query.subject ||
            fact.property != query.property) {
            continue;
        }

        ++result.report.matched;
        if (result.candidates.size() >= max_candidates) {
            // We found relevant evidence that could not be retained. The
            // caller must not treat the retained prefix as a complete search.
            result.report.candidate_limit_reached = true;
            result.report.complete = false;
            break;
        }

        // Keep both matching and non-matching values. Non-matching facts are
        // necessary evidence when evaluating an exact-value query as FALSE.
        result.candidates.push_back(fact);
    }

    return result;
}

} // namespace seventh::reasoning
