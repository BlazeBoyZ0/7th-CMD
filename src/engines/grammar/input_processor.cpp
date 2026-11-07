#include "seventh/engines/grammar/input_processor.hpp"

namespace seventh::grammar {

std::vector<std::string> InputProcessor::process(const std::string& input) const {
    return tokenizer_.tokenize(normalizer_.normalize(input));
}

} // namespace seventh::grammar
