#pragma once

#include <string>

namespace seventh::grammar {

class Normalizer {
public:
    // Normalizes spacing and line endings without changing case, punctuation,
    // quoted content, or filename characters.
    [[nodiscard]] std::string normalize(const std::string& input) const;
};

} // namespace seventh::grammar
