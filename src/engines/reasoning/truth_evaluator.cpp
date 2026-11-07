#include "seventh/engines/reasoning/truth_evaluator.hpp"

namespace seventh::reasoning {

TruthValue TruthEvaluator::negate(TruthValue value) noexcept {
    if (value == TruthValue::True) return TruthValue::False;
    if (value == TruthValue::False) return TruthValue::True;
    return TruthValue::Unknown;
}

TruthValue TruthEvaluator::conjunction(TruthValue left, TruthValue right) noexcept {
    if (left == TruthValue::False || right == TruthValue::False) return TruthValue::False;
    if (left == TruthValue::True && right == TruthValue::True) return TruthValue::True;
    return TruthValue::Unknown;
}

TruthValue TruthEvaluator::disjunction(TruthValue left, TruthValue right) noexcept {
    if (left == TruthValue::True || right == TruthValue::True) return TruthValue::True;
    if (left == TruthValue::False && right == TruthValue::False) return TruthValue::False;
    return TruthValue::Unknown;
}

} // namespace seventh::reasoning
