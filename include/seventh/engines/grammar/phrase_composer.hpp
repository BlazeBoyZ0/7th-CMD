#pragma once

#include "seventh/engines/grammar/grammar_config.hpp"
#include "seventh/engines/grammar/tokenizer.hpp"

#include <string>
#include <vector>

namespace seventh::grammar {

// Composes short, explicitly recognized follow-up phrases into the existing
// single-intent token shape. Unknown clauses are preserved rather than guessed.
class PhraseComposer {
public:
    [[nodiscard]] std::vector<std::string> compose(
        const std::vector<Token>& detailed,
        const std::vector<std::string>& corrected_tokens,
        const GrammarConfig& config) const;
};

} // namespace seventh::grammar
