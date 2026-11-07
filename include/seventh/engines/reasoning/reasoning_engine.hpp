#pragma once

#include "seventh/engines/reasoning/bounded_search.hpp"
#include "seventh/engines/reasoning/confidence_evaluator.hpp"
#include "seventh/engines/reasoning/contradiction_handler.hpp"
#include "seventh/engines/reasoning/fact_resolver.hpp"
#include "seventh/engines/reasoning/reasoning_types.hpp"
#include "seventh/engines/reasoning/rule_loader.hpp"

#include <string>
#include <vector>
#include "seventh/engines/reasoning/rule_evaluator.hpp"
#include "seventh/engines/reasoning/uncertainty_handler.hpp"

namespace seventh::reasoning {

class ReasoningEngine {
public:
    [[nodiscard]] ReasoningResult evaluate(const ReasoningRequest& request) const;
    bool load_rules_from_file(const std::string& path, std::string& error,
                              bool merge = false);

private:
    BoundedSearch bounded_search_;
    FactResolver fact_resolver_;
    RuleEvaluator rule_evaluator_;
    ContradictionHandler contradiction_handler_;
    ConfidenceEvaluator confidence_evaluator_;
    UncertaintyHandler uncertainty_handler_;
    RuleLoader rule_loader_;
    std::vector<Rule> configured_rules_;
};

} // namespace seventh::reasoning
