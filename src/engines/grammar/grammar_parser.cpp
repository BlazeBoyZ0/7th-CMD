#include "seventh/engines/grammar/grammar_parser.hpp"

#include <algorithm>
#include <cctype>
#include <set>

namespace seventh::grammar {
namespace {

std::string lower(std::string value) {
    for (char& character : value) {
        character = static_cast<char>(
            std::tolower(static_cast<unsigned char>(character)));
    }
    return value;
}

bool is_count_metric(const std::string& token) {
    static const std::set<std::string> metrics{
        "word", "words", "line", "lines", "character", "characters",
        "byte", "bytes"
    };
    return metrics.contains(token);
}

std::vector<std::string> normalize_count_question(
    const std::vector<std::string>& tokens) {
    if (tokens.size() < 4) return {};

    std::vector<std::string> lowered;
    lowered.reserve(tokens.size());
    for (const auto& token : tokens) lowered.push_back(lower(token));

    const bool how_question = lowered[0] == "how" &&
        (lowered[1] == "many" || lowered[1] == "much");
    const bool what_question = lowered[0] == "what" &&
        lowered.size() > 3 && lowered[1] == "is";
    if (!how_question && !what_question) return {};

    std::size_t metric_index = lowered.size();
    for (std::size_t index = how_question ? 2 : 2;
         index < lowered.size() && index < 6; ++index) {
        auto metric = lowered[index];
        if (metric == "word") metric = "words";
        if (metric == "line") metric = "lines";
        if (metric == "character") metric = "characters";
        if (metric == "byte") metric = "bytes";
        if (is_count_metric(metric)) {
            metric_index = index;
            lowered[index] = metric;
            break;
        }
    }
    if (metric_index == lowered.size()) return {};

    std::size_t file_index = lowered.size();
    for (const auto* separator : {"in", "of", "from"}) {
        const auto found = std::find(lowered.begin() +
            static_cast<std::ptrdiff_t>(metric_index + 1),
            lowered.end(), separator);
        if (found != lowered.end()) {
            file_index = std::min(file_index,
                static_cast<std::size_t>(found - lowered.begin()));
        }
    }
    if (file_index == lowered.size() || file_index + 1 >= tokens.size()) return {};

    std::vector<std::string> canonical{
        "count", lowered[metric_index], "file"
    };
    canonical.insert(canonical.end(), tokens.begin() +
        static_cast<std::ptrdiff_t>(file_index + 1), tokens.end());
    return canonical;
}

} // namespace

ParseMatch GrammarParser::parse(const std::vector<std::string>& tokens) const {
    return parse(tokens, GrammarConfig::defaults());
}

ParseMatch GrammarParser::parse(const std::vector<std::string>& tokens,
                                const GrammarConfig& config) const {
    ParseMatch match;
    match.tokens = normalize_count_question(tokens);
    if (match.tokens.empty()) match.tokens = tokens;

    if (match.tokens.empty()) {
        match.error = "The instruction is empty.";
        return match;
    }

    // Apply spelling corrections only to grammar vocabulary positions. Entity
    // names and file/folder paths are left untouched.
    for (std::size_t index = 0; index < match.tokens.size(); ++index) {
        const auto word = lower(match.tokens[index]);
        const auto canonical = config.canonical_term(word);
        if (index == 0 || config.entities.contains(canonical) ||
            is_count_metric(canonical)) {
            match.tokens[index] = canonical;
        }
    }

    const auto original_action = lower(match.tokens.front());
    const auto action = config.canonical_term(original_action);
    if (!config.actions.contains(action)) {
        match.error = "I do not recognize that command yet.";
        return match;
    }
    match.operation = action;
    match.tokens.front() = action;

    for (std::size_t index = 1; index < match.tokens.size(); ++index) {
        const auto canonical = config.canonical_term(lower(match.tokens[index]));
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
