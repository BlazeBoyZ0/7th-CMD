#pragma once

#include "seventh/engines/grammar/normalizer.hpp"
#include "seventh/engines/grammar/tokenizer.hpp"

#include <string>
#include <vector>

namespace seventh::grammar {

class InputProcessor {
public:
    [[nodiscard]] std::vector<Token> process_detailed(const std::string& input) const;
    [[nodiscard]] std::vector<std::string> process(const std::string& input) const;

private:
    Normalizer normalizer_;
    Tokenizer tokenizer_;
};

} // namespace seventh::grammar
