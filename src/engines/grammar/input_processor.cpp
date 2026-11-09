#include "seventh/engines/grammar/input_processor.hpp"

namespace seventh::grammar {

std::vector<Token> InputProcessor::process_detailed(
    const std::string& input) const {
    return tokenizer_.tokenize_detailed(normalizer_.normalize(input));
}

std::vector<std::string> InputProcessor::process(const std::string& input) const {
    std::vector<std::string> result;
    const auto detailed = process_detailed(input);
    result.reserve(detailed.size());
    for (const auto& token : detailed) {
        result.push_back(token.text);
    }
    return result;
}

} // namespace seventh::grammar
