#include "seventh/engines/grammar/grammar_parser.hpp"

#include <algorithm>
#include <cctype>

namespace seventh::grammar {
namespace {

std::string lower(std::string value) {
    for (char& character : value) {
        character = static_cast<char>(
            std::tolower(static_cast<unsigned char>(character)));
    }
    return value;
}

} // namespace

ParseMatch GrammarParser::parse(const std::vector<std::string>& tokens) const {
    return parse(tokens, GrammarConfig::defaults());
}

ParseMatch GrammarParser::parse(const std::vector<std::string>& tokens,
                                const GrammarConfig& config) const {
    ParseMatch match;
    match.tokens = tokens;

    if (tokens.empty()) {
        match.error = "The instruction is empty.";
        return match;
    }

    const auto original_action = lower(tokens.front());
    const auto action = config.canonical_term(original_action);
    if (!config.actions.contains(action)) {
        match.error = "I do not recognize that command yet.";
        return match;
    }
    match.operation = action;
    match.tokens.front() = action;

    for (std::size_t index = 1; index < tokens.size(); ++index) {
        const auto canonical = config.canonical_term(lower(tokens[index]));
        if (config.entities.contains(canonical)) {
            match.entity_type = canonical;
            match.tokens[index] = canonical;
            break;
        }
    }

    if (match.entity_type.empty()) {
        match.entity_type = match.operation == "open" ? "application" : "item";
    }

    match.recognized = true;
    return match;
}

} // namespace seventh::grammar
