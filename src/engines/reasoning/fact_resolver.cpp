#include "seventh/engines/reasoning/fact_resolver.hpp"

namespace seventh::reasoning {

FactResolution FactResolver::resolve(const FactQuery& query,
                                      const std::vector<Fact>& facts) const {
    FactResolution result;
    bool saw_match = false;
    bool saw_non_match = false;

    for (const auto& fact : facts) {
        if (!fact.fresh || fact.subject != query.subject ||
            fact.property != query.property) {
            continue;
        }

        // Preserve all relevant evidence, including facts that disprove the
        // requested value. The caller can then inspect conflicts as well.
        result.matches.push_back(fact);

        if (!query.match_value || fact.value == query.value) {
            saw_match = true;
        } else {
            saw_non_match = true;
        }
    }

    result.contradiction = saw_match && saw_non_match;
    if (result.contradiction) {
        result.truth = TruthValue::Unknown;
    } else if (saw_match) {
        result.truth = TruthValue::True;
    } else if (saw_non_match) {
        result.truth = TruthValue::False;
    }
    return result;
}

} // namespace seventh::reasoning
