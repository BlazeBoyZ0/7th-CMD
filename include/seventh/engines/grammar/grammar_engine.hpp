#pragma once

#include "seventh/engines/grammar/grammar_types.hpp"
#include "seventh/engines/grammar/input_processor.hpp"

#include <string>

namespace seventh::grammar {

class GrammarEngine {
public:
    [[nodiscard]] CanonicalIntent interpret(const std::string& input) const;

private:
    InputProcessor input_processor_;
};

} // namespace seventh::grammar
