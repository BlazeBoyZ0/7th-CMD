#include "seventh/engines/reasoning/fact_resolver.hpp"

namespace seventh::reasoning {

FactResolution FactResolver::resolve(const FactQuery& query,
                                      const std::vector<Fact>& facts) const {
    FactResolution result;
    bool saw_true = false;
    bool saw_false = false;

    for (const auto& fact : facts) {
        if (!fact.fresh || fact.subject != query.subject ||
            fact.property != query.property) {
            continue;
        }
        if (query.match_value && fact.value != query.value) {
            saw_false = true;
            continue;
        }
        result.matches.push_back(fact);
        if (!query.match_value || fact.value == query.value) saw_true = true;
    }

    result.contradiction = saw_true && saw_false;
    if (result.contradiction) result.truth = TruthValue::Unknown;
    else if (saw_true) result.truth = TruthValue::True;
    else if (saw_false) result.truth = TruthValue::False;
    return result;
}

} // namespace seventh::reasoning
