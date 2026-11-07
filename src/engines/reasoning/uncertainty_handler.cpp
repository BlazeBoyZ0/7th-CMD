#include "seventh/engines/reasoning/uncertainty_handler.hpp"

namespace seventh::reasoning {

void UncertaintyHandler::explain_unknown(ReasoningResult& result,
                                          const FactQuery& query) const {
    if (result.truth != TruthValue::Unknown) return;
    if (result.explanation.empty()) {
        result.explanation = "The available evidence does not establish the requested fact.";
    }
    if (!query.subject.empty()) result.unresolved_dependencies.push_back(query.subject);
    if (!query.property.empty()) result.unresolved_dependencies.push_back(query.property);
}

} // namespace seventh::reasoning
