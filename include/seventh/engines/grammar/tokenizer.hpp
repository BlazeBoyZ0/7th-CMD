#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace seventh::grammar {

// A lexical token with source positions and punctuation context.
// begin/end are byte offsets into the normalized input, with end exclusive.
struct Token {
    std::string text;
    std::string normalized;
    std::size_t begin{0};
    std::size_t end{0};
    std::size_t sentence_index{0};
    std::string punctuation_before;
    std::string punctuation_after;
    bool quoted{false};
};

class Tokenizer {
public:
    [[nodiscard]] std::vector<Token> tokenize_detailed(const std::string& input) const;
    [[nodiscard]] std::vector<std::string> tokenize(const std::string& input) const;
};

} // namespace seventh::grammar
