#include "seventh/engines/grammar/cir_generator.hpp"

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

bool is_article(const std::string& token) {
    const auto value = lower(token);
    return value == "a" || value == "an" || value == "the";
}

std::size_t find_token(const std::vector<std::string>& tokens,
                       const std::string& value,
                       std::size_t start = 0) {
    for (std::size_t index = start; index < tokens.size(); ++index) {
        if (lower(tokens[index]) == value) return index;
    }
    return tokens.size();
}

std::string join(const std::vector<std::string>& tokens,
                 std::size_t begin, std::size_t end) {
    std::string value;
    for (std::size_t index = begin; index < end && index < tokens.size(); ++index) {
        if (!value.empty()) value += ' ';
        value += tokens[index];
    }
    return value;
}

} // namespace

CanonicalIntent CirGenerator::generate(const AstNode& node,
                                        const std::string& raw_input) const {
    CanonicalIntent intent;
    intent.operation = node.operation;
    intent.entity_type = node.entity_type;
    intent.raw_input = raw_input;

    if (!node.valid) {
        intent.error = node.error;
        return intent;
    }

    const auto& tokens = node.tokens;
    const auto entity = find_token(tokens, node.entity_type);
    std::size_t target_start = entity < tokens.size() ? entity + 1 : 1;
    if (target_start < tokens.size() && is_article(tokens[target_start])) {
        ++target_start;
    }

    if (node.operation == "create") {
        if (target_start < tokens.size() && lower(tokens[target_start]) == "named") {
            ++target_start;
        }
        auto end = find_token(tokens, "on", target_start);
        if (end == tokens.size()) end = find_token(tokens, "in", target_start);
        if (end == tokens.size()) end = tokens.size();
        intent.name = join(tokens, target_start, end);
        if (end < tokens.size() && end + 1 < tokens.size()) {
            intent.destination = join(tokens, end + 1, tokens.size());
        }
    } else if (node.operation == "rename" || node.operation == "move" ||
               node.operation == "copy") {
        const auto separator = find_token(tokens, "to", target_start);
        if (separator < tokens.size()) {
            intent.source = join(tokens, target_start, separator);
            intent.destination = join(tokens, separator + 1, tokens.size());
            intent.name = intent.source;
        }
    } else if (node.operation == "delete" || node.operation == "open") {
        intent.name = join(tokens, target_start, tokens.size());
    } else if (node.operation == "count") {
        intent.query = tokens.size() > 1 ? lower(tokens[1]) : std::string{};
        if (target_start < tokens.size() && lower(tokens[target_start]) == "named") {
            ++target_start;
        }
        intent.name = join(tokens, target_start, tokens.size());
    } else if (node.operation == "search") {
        auto query_start = target_start;
        const auto named = find_token(tokens, "named", target_start);
        if (named < tokens.size()) query_start = named + 1;
        else if (query_start < tokens.size() && lower(tokens[query_start]) == "for") {
            ++query_start;
        }

        auto location = find_token(tokens, "in", query_start);
        if (location == tokens.size()) location = tokens.size();
        intent.query = join(tokens, query_start, location);
        if (location < tokens.size() && location + 1 < tokens.size()) {
            intent.destination = join(tokens, location + 1, tokens.size());
        }
    }

    if (node.operation == "create" && intent.name.empty()) {
        intent.missing_information.emplace_back("name");
    }
    if ((node.operation == "rename" || node.operation == "move" ||
         node.operation == "copy") &&
        (intent.source.empty() || intent.destination.empty())) {
        intent.missing_information.emplace_back("source and destination");
    }
    if ((node.operation == "delete" || node.operation == "open") &&
        intent.name.empty()) {
        intent.missing_information.emplace_back("target");
    }
    if (node.operation == "count" && intent.name.empty()) {
        intent.missing_information.emplace_back("file");
    }

    intent.valid = intent.missing_information.empty();
    if (!intent.valid) intent.error = "The instruction is missing required information.";
    return intent;
}

} // namespace seventh::grammar
