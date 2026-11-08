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

std::string plural_metric(std::string metric) {
    if (metric == "word") return "words";
    if (metric == "line") return "lines";
    if (metric == "character") return "characters";
    if (metric == "byte") return "bytes";
    return metric;
}

std::vector<std::string> normalize_count_instruction(
    const std::vector<std::string>& tokens) {
    if (tokens.size() < 4) return {};

    std::vector<std::string> lowered;
    lowered.reserve(tokens.size());
    for (const auto& token : tokens) lowered.push_back(lower(token));

    const bool how_question = lowered[0] == "how" &&
        (lowered[1] == "many" || lowered[1] == "much");
    const bool what_question = lowered[0] == "what" &&
        lowered[1] == "is";
    const bool count_command = lowered[0] == "count";
    if (!how_question && !what_question && !count_command) return {};

    std::size_t metric_index = lowered.size();
    for (std::size_t index = count_command ? 1 : 2;
         index < lowered.size() && index < 7; ++index) {
        const auto metric = plural_metric(lowered[index]);
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
    auto prepared = tokens;
    if (!prepared.empty()) {
        const auto first = lower(prepared.front());
        if (first == "how" || first == "what") {
            for (std::size_t index = 1; index < prepared.size() && index < 5; ++index) {
                prepared[index] = config.canonical_term(prepared[index]);
            }
        } else if (first == "count" && prepared.size() > 1) {
            prepared[1] = config.canonical_term(prepared[1]);
        }
    }

    match.tokens = normalize_count_instruction(prepared);
    if (match.tokens.empty()) match.tokens = std::move(prepared);

    if (match.tokens.empty()) {
        match.error = "The instruction is empty.";
        return match;
    }

    // Correct only command vocabulary, entity types, and count metrics.
    // User-supplied names and paths remain exactly as entered.
    for (std::size_t index = 0; index < match.tokens.size(); ++index) {
        const auto canonical = config.canonical_term(match.tokens[index]);
        if (index == 0 || config.entities.contains(canonical) ||
            is_count_metric(canonical)) {
            match.tokens[index] = canonical;
        }
    }

    const auto action = config.canonical_term(match.tokens.front());
    if (!config.actions.contains(action)) {
        match.error = "I do not recognize that command yet.";
        return match;
    }
    match.operation = action;
    match.tokens.front() = action;

    for (std::size_t index = 1; index < match.tokens.size(); ++index) {
        const auto canonical = config.canonical_term(match.tokens[index]);
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
