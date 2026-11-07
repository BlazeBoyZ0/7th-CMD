#include "seventh/engines/grammar/tokenizer.hpp"

#include <cctype>

namespace seventh::grammar {

std::vector<std::string> Tokenizer::tokenize(const std::string& input) const {
    std::vector<std::string> tokens;
    std::string current;
    char quote = '\0';

    for (const char character : input) {
        if (quote != '\0') {
            if (character == quote) {
                quote = '\0';
            } else {
                current += character;
            }
            continue;
        }

        if (character == '"' || character == '\'') {
            quote = character;
        } else if (std::isspace(static_cast<unsigned char>(character))) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += character;
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

} // namespace seventh::grammar
