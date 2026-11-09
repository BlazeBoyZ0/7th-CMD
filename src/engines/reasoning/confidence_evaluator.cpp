#include "seventh/engines/reasoning/confidence_evaluator.hpp"

namespace seventh::reasoning {

unsigned ConfidenceEvaluator::evaluate(const std::vector<Fact>& evidence) const noexcept {
    if (evidence.empty()) return 0;
    unsigned total = 0;
    for (const auto& fact : evidence) {
        total += fact.confidence > 100 ? 100 : fact.confidence;
    }
    return total / static_cast<unsigned>(evidence.size());
}

} // namespace seventh::reasoning
