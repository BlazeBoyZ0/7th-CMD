#include "seventh/engines/grammar/tokenizer.hpp"

#include <cctype>
#include <utility>

namespace seventh::grammar {
namespace {

bool is_clause_punctuation(char value) {
    return value == ',' || value == ';' || value == '!' ||
           value == '?' || value == ':';
}

std::string lowercase_ascii(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (const unsigned char character : value) {
        result += static_cast<char>(std::tolower(character));
    }
    return result;
}

} // namespace

std::vector<Token> Tokenizer::tokenize_detailed(const std::string& input) const {
    std::vector<Token> tokens;
    std::size_t position = 0;
    std::size_t sentence_index = 0;
    std::string pending_punctuation;

    while (position < input.size()) {
        const unsigned char current =
            static_cast<unsigned char>(input[position]);

        if (std::isspace(current)) {
            ++position;
            continue;
        }

        if (is_clause_punctuation(input[position]) || input[position] == '.') {
            const char punctuation = input[position];
            pending_punctuation += punctuation;
            if (!tokens.empty()) {
                tokens.back().punctuation_after += punctuation;
            }
            ++position;

            // Treat repeated dots and a terminal dot as sentence boundaries,
            // but preserve dots inside filenames such as main.cpp and test.kek.
            if (punctuation == '.') {
                while (position < input.size() && input[position] == '.') {
                    pending_punctuation += input[position++];
                }
                if (position < input.size() &&
                    !std::isspace(static_cast<unsigned char>(input[position]))) {
                    continue;
                }
            }
            if (punctuation == '.' || punctuation == ';' ||
                punctuation == '!' || punctuation == '?') {
                ++sentence_index;
            }
            continue;
        }

        Token token;
        token.sentence_index = sentence_index;
        token.punctuation_before = std::move(pending_punctuation);
        pending_punctuation.clear();

        const std::size_t token_start = position;
        const char quote = (input[position] == '"' || input[position] == '\'')
            ? input[position] : '\0';

        if (quote != '\0') {
            token.quoted = true;
            ++position;
            const std::size_t content_start = position;
            while (position < input.size() && input[position] != quote) {
                ++position;
            }
            token.text = input.substr(content_start, position - content_start);
            if (position < input.size()) {
                ++position;
            }
            token.begin = token_start;
            token.end = position;
        } else {
            const std::size_t content_start = position;
            while (position < input.size()) {
                const char character = input[position];
                if (std::isspace(static_cast<unsigned char>(character)) ||
                    is_clause_punctuation(character)) {
                    break;
                }
                if (character == '.' &&
                    (position + 1 == input.size() ||
                     std::isspace(static_cast<unsigned char>(input[position + 1])) ||
                     input[position + 1] == '.')) {
                    break;
                }
                if (character == '"' || character == '\'') {
                    break;
                }
                ++position;
            }

            if (position == content_start) {
                // An unmatched quote or isolated punctuation must not stall
                // tokenization; preserve it as a one-byte token.
                ++position;
                token.text = input.substr(content_start, 1);
            } else {
                token.text = input.substr(content_start, position - content_start);
            }
            token.begin = content_start;
            token.end = position;
        }

        token.normalized = lowercase_ascii(token.text);
        tokens.push_back(std::move(token));
    }

    if (!pending_punctuation.empty() && !tokens.empty()) {
        tokens.back().punctuation_after = std::move(pending_punctuation);
    }
    return tokens;
}

std::vector<std::string> Tokenizer::tokenize(const std::string& input) const {
    std::vector<std::string> result;
    const auto detailed = tokenize_detailed(input);
    result.reserve(detailed.size());
    for (const auto& token : detailed) {
        result.push_back(token.text);
    }
    return result;
}

} // namespace seventh::grammar
