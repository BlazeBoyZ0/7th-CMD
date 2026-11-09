#include "seventh/engines/grammar/normalizer.hpp"

#include <cctype>

namespace seventh::grammar {

std::string Normalizer::normalize(const std::string& input) const {
    std::string output;
    output.reserve(input.size());
    bool previous_was_space = true;

    for (const unsigned char character : input) {
        if (std::isspace(character)) {
            if (!previous_was_space) {
                output += ' ';
                previous_was_space = true;
            }
            continue;
        }

        // Preserve letter case: names and paths must retain the user's spelling.
        output += static_cast<char>(character);
        previous_was_space = false;
    }

    if (!output.empty() && output.back() == ' ') {
        output.pop_back();
    }
    return output;
}

} // namespace seventh::grammar
