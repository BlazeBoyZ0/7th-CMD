#include "seventh/engines/grammar/grammar_parser.hpp"

#include <array>
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

bool contains(const std::vector<std::string>& tokens, const std::string& word) {
    for (const auto& token : tokens) {
        if (lower(token) == word) return true;
    }
    return false;
}

std::string identify_entity(const std::vector<std::string>& tokens) {
    constexpr std::array<const char*, 7> entities{
        "folder", "directory", "file", "application", "app", "process", "item"
    };
    for (const auto* entity : entities) {
        if (contains(tokens, entity)) {
            if (std::string(entity) == "directory") return "folder";
            if (std::string(entity) == "app") return "application";
            return entity;
        }
    }
    return {};
}

} // namespace

ParseMatch GrammarParser::parse(const std::vector<std::string>& tokens) const {
    ParseMatch match;
    match.tokens = tokens;

    if (tokens.empty()) {
        match.error = "The instruction is empty.";
        return match;
    }

    const auto first = lower(tokens.front());
    if (first == "create" || first == "make") {
        match.operation = "create";
    } else if (first == "rename") {
        match.operation = "rename";
    } else if (first == "move") {
        match.operation = "move";
    } else if (first == "copy" || first == "duplicate") {
        match.operation = "copy";
    } else if (first == "delete" || first == "remove") {
        match.operation = "delete";
    } else if (first == "search" || first == "find") {
        match.operation = "search";
    } else if (first == "open" || first == "launch" || first == "start") {
        match.operation = "open";
    } else {
        match.error = "I do not recognize that command yet.";
        return match;
    }

    match.entity_type = identify_entity(tokens);
    if (match.entity_type.empty()) {
        match.entity_type = match.operation == "open" ? "application" : "item";
    }

    match.recognized = true;
    return match;
}

} // namespace seventh::grammar
