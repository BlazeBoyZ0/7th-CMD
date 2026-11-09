#pragma once

#include "seventh/engines/grammar/tokenizer.hpp"

#include <cstddef>
#include <map>
#include <set>
#include <string>

namespace seventh::grammar {

struct CorrectionResult {
    std::string value;
    std::size_t distance{0};
    bool changed{false};
    bool ambiguous{false};
};

// Conservative, vocabulary-bounded spelling correction. Callers must explicitly
// mark a token as grammar vocabulary; user data is never corrected implicitly.
class TypoCorrector {
public:
    [[nodiscard]] CorrectionResult correct(
        const Token& token,
        const std::set<std::string>& vocabulary,
        const std::map<std::string, std::string>& explicit_corrections = {},
        bool grammar_vocabulary_position = false) const;

    [[nodiscard]] static std::size_t damerau_levenshtein(
        const std::string& left, const std::string& right);
};

} // namespace seventh::grammar
