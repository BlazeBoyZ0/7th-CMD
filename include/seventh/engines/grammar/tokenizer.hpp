#pragma once

#include <string>
#include <vector>

namespace seventh::grammar {

class Tokenizer {
public:
    [[nodiscard]] std::vector<std::string> tokenize(const std::string& input) const;
};

} // namespace seventh::grammar
