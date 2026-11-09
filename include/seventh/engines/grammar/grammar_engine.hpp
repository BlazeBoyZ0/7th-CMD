#pragma once

#include "seventh/engines/grammar/grammar_config.hpp"
#include "seventh/engines/grammar/grammar_types.hpp"
#include "seventh/engines/grammar/input_processor.hpp"

#include <string>

namespace seventh::grammar {

class GrammarEngine {
public:
    explicit GrammarEngine(GrammarConfig config = GrammarConfig::defaults());
    [[nodiscard]] CanonicalIntent interpret(const std::string& input) const;

private:
    InputProcessor input_processor_;
    GrammarConfig config_;
};

} // namespace seventh::grammar
