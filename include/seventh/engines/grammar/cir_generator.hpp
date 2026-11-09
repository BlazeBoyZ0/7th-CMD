#pragma once

#include "seventh/engines/grammar/grammar_types.hpp"

#include <string>

namespace seventh::grammar {

class CirGenerator {
public:
    [[nodiscard]] CanonicalIntent generate(const AstNode& node,
                                           const std::string& raw_input) const;
};

} // namespace seventh::grammar
