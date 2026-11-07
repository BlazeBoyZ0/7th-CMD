#pragma once

#include "seventh/engines/reasoning/bounded_search.hpp"
#include "seventh/engines/reasoning/confidence_evaluator.hpp"
#include "seventh/engines/reasoning/contradiction_handler.hpp"
#include "seventh/engines/reasoning/fact_resolver.hpp"
#include "seventh/engines/reasoning/reasoning_types.hpp"
#include "seventh/engines/reasoning/rule_evaluator.hpp"
#include "seventh/engines/reasoning/uncertainty_handler.hpp"

namespace seventh::reasoning {

class ReasoningEngine {
public:
    [[nodiscard]] ReasoningResult evaluate(const ReasoningRequest& request) const;

private:
    BoundedSearch bounded_search_;
    FactResolver fact_resolver_;
    RuleEvaluator rule_evaluator_;
    ContradictionHandler contradiction_handler_;
    ConfidenceEvaluator confidence_evaluator_;
    UncertaintyHandler uncertainty_handler_;
};

} // namespace seventh::reasoning
