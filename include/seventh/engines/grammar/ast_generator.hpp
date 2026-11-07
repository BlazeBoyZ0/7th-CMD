#pragma once

#include "seventh/engines/grammar/grammar_types.hpp"

namespace seventh::grammar {

class AstGenerator {
public:
    [[nodiscard]] AstNode generate(const ParseMatch& match) const;
};

} // namespace seventh::grammar
