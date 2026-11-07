#include "seventh/engines/reasoning/bounded_search.hpp"

namespace seventh::reasoning {

std::vector<Fact> BoundedSearch::collect_relevant(
    const FactQuery& query, const std::vector<Fact>& facts,
    std::size_t max_candidates) const {
    std::vector<Fact> matches;
    matches.reserve(max_candidates);

    for (const auto& fact : facts) {
        if (matches.size() >= max_candidates) break;
        if (!fact.fresh || fact.subject != query.subject ||
            fact.property != query.property) {
            continue;
        }

        // Keep both matching and non-matching values. Non-matching facts are
        // necessary evidence when evaluating an exact-value query as FALSE.
        matches.push_back(fact);
    }
    return matches;
}

} // namespace seventh::reasoning
