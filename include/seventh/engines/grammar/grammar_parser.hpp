#pragma once

#include "seventh/engines/grammar/grammar_types.hpp"

#include <string>
#include <vector>

namespace seventh::grammar {

class GrammarParser {
public:
    [[nodiscard]] ParseMatch parse(const std::vector<std::string>& tokens) const;
};

} // namespace seventh::grammar
