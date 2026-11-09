#include "seventh/engines/reasoning/fact_resolver.hpp"

namespace seventh::reasoning {

FactResolution FactResolver::resolve(const FactQuery& query,
                                      const std::vector<Fact>& facts) const {
    FactResolution result;

    for (const auto& fact : facts) {
        if (!fact.fresh || fact.subject != query.subject ||
            fact.property != query.property) {
            continue;
        }

        result.matches.push_back(fact);

        const bool supports = !query.match_value || fact.value == query.value;
        if (supports) {
            result.supporting.push_back(fact);
        } else {
            result.opposing.push_back(fact);
        }
    }

    result.contradiction = !result.supporting.empty() &&
                           !result.opposing.empty();

    if (result.contradiction) {
        result.truth = TruthValue::Unknown;
    } else if (!result.supporting.empty()) {
        result.truth = TruthValue::True;
    } else if (!result.opposing.empty()) {
        result.truth = TruthValue::False;
    }

    return result;
}

} // namespace seventh::reasoning
