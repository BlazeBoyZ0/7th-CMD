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
        result.matches.push_back(fact);
        if (!query.match_value || fact.value == query.value) saw_true = true;
        else saw_false = true;
    }

    result.contradiction = query.match_value && saw_true && saw_false;
    if (result.contradiction) result.truth = TruthValue::Unknown;
    else if (saw_true) result.truth = TruthValue::True;
    else if (saw_false) result.truth = TruthValue::False;
    return result;
}

} // namespace seventh::reasoning
