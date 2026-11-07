#pragma once

#include "seventh/engines/grammar/grammar_types.hpp"

namespace seventh::grammar {

class SemanticAnalyzer {
public:
    [[nodiscard]] AstNode analyze(AstNode node) const;
};

} // namespace seventh::grammar
