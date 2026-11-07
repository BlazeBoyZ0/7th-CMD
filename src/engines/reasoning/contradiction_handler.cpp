#include "seventh/engines/reasoning/contradiction_handler.hpp"

namespace seventh::reasoning {

bool ContradictionHandler::contradictory(const std::vector<Fact>& facts) const noexcept {
    for (std::size_t left = 0; left < facts.size(); ++left) {
        if (!facts[left].fresh) continue;
        for (std::size_t right = left + 1; right < facts.size(); ++right) {
            if (facts[right].fresh &&
                facts[left].subject == facts[right].subject &&
                facts[left].property == facts[right].property &&
                facts[left].value != facts[right].value) {
                return true;
            }
        }
    }
    return false;
}

} // namespace seventh::reasoning
