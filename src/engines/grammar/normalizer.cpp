#include "seventh/engines/grammar/normalizer.hpp"

#include <cctype>

namespace seventh::grammar {

std::string Normalizer::normalize(const std::string& input) const {
    std::string output;
    output.reserve(input.size());
    bool previous_was_space = true;
    char quote = '\0';

    for (std::size_t index = 0; index < input.size(); ++index) {
        const char raw = input[index];

        if (quote != '\0') {
            output += raw;
            if (raw == quote && (index == 0 || input[index - 1] != '\\')) {
                quote = '\0';
            }
            continue;
        }

        if (raw == '"' || raw == '\'') {
            output += raw;
            quote = raw;
            previous_was_space = false;
            continue;
        }

        const unsigned char character = static_cast<unsigned char>(raw);
        if (character == '\r' || character == '\n' ||
            character == '\t' || std::isspace(character)) {
            if (!previous_was_space) {
                output += ' ';
                previous_was_space = true;
            }
            if (character == '\r' && index + 1 < input.size() &&
                input[index + 1] == '\n') {
                ++index;
            }
            continue;
        }

        // Preserve case and punctuation: names, paths, and quoted values
        // must not be silently rewritten during lexical normalization.
        output += raw;
        previous_was_space = false;
    }

    if (!output.empty() && output.back() == ' ') {
        output.pop_back();
    }
    return output;
}

} // namespace seventh::grammar
