#pragma once

#include <string>

namespace seventh::grammar {

class Normalizer {
public:
    [[nodiscard]] std::string normalize(const std::string& input) const;
};

} // namespace seventh::grammar
