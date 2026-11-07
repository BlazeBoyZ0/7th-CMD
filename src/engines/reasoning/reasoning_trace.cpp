#include "seventh/engines/reasoning/reasoning_trace.hpp"

#include <utility>

namespace seventh::reasoning {

void ReasoningTrace::add(std::string event) {
    if (!event.empty()) events_.push_back(std::move(event));
}

const std::vector<std::string>& ReasoningTrace::events() const noexcept {
    return events_;
}

void ReasoningTrace::clear() noexcept {
    events_.clear();
}

} // namespace seventh::reasoning
