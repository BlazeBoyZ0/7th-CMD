#include "seventh/engines/reasoning/bounded_search.hpp"

namespace seventh::reasoning {

std::vector<Fact> BoundedSearch::collect_relevant(
    const FactQuery& query, const std::vector<Fact>& facts,
    std::size_t max_candidates) const {
    std::vector<Fact> matches;
    for (const auto& fact : facts) {
        if (matches.size() >= max_candidates) break;
        if (!fact.fresh || fact.subject != query.subject ||
            fact.property != query.property) continue;
        matches.push_back(fact);
    }
    return matches;
}

} // namespace seventh::reasoning
