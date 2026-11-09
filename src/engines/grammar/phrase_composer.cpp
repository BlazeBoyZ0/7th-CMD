#include "seventh/engines/grammar/phrase_composer.hpp"

#include <algorithm>
#include <cstddef>
#include <string_view>

namespace seventh::grammar {
namespace {

bool contains_any(const std::string& punctuation, std::string_view values) {
    return std::any_of(punctuation.begin(), punctuation.end(),
        [&](char value) { return values.find(value) != std::string_view::npos; });
}

bool is_article(const std::string& token) {
    return token == "a" || token == "an" || token == "the" || token == "new";
}

bool is_name_phrase(const std::vector<std::string>& clause) {
    return clause.size() >= 3 &&
        ((clause[0] == "name" && (clause[1] == "it" || clause[1] == "that")) ||
         (clause[0] == "call" && (clause[1] == "it" || clause[1] == "that")) ||
         (clause[0] == "name" && clause[1] == "it"));
}

bool is_location_phrase(const std::vector<std::string>& clause) {
    if (clause.size() < 3) return false;
    const bool verb = clause[0] == "put" || clause[0] == "place" ||
                      clause[0] == "keep" || clause[0] == "save" ||
                      clause[0] == "move";
    if (!verb) return false;
    std::size_t position = 1;
    if (clause[position] == "it" || clause[position] == "that" ||
        clause[position] == "this") ++position;
    return position < clause.size() &&
        (clause[position] == "on" || clause[position] == "in" ||
         clause[position] == "inside" || clause[position] == "within" ||
         clause[position] == "to") &&
        position + 1 < clause.size();
}

std::vector<std::vector<std::string>> split_clauses(
    const std::vector<Token>& detailed,
    const std::vector<std::string>& tokens) {
    std::vector<std::vector<std::string>> clauses;
    std::vector<std::string> current;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i] == "then" && !current.empty()) {
            clauses.push_back(std::move(current));
            current.clear();
            continue;
        }
        if (tokens[i] == "and" && !current.empty() && i + 1 < tokens.size()) {
            const auto& next = tokens[i + 1];
            if (next == "put" || next == "place" || next == "keep" ||
                next == "save" || next == "move" || next == "name" ||
                next == "call") {
                clauses.push_back(std::move(current));
                current.clear();
                continue;
            }
        }

        current.push_back(tokens[i]);
        if (i < detailed.size() &&
            contains_any(detailed[i].punctuation_after, ",;:!?.")) {
            clauses.push_back(std::move(current));
            current.clear();
        }
    }
    if (!current.empty()) clauses.push_back(std::move(current));
    return clauses;
}

} // namespace

std::vector<std::string> PhraseComposer::compose(
    const std::vector<Token>& detailed,
    const std::vector<std::string>& corrected_tokens,
    const GrammarConfig& config) const {
    if (detailed.empty() || corrected_tokens.empty() ||
        detailed.size() != corrected_tokens.size()) {
        return corrected_tokens;
    }

    auto clauses = split_clauses(detailed, corrected_tokens);
    if (clauses.size() < 2 || clauses.front().empty()) return corrected_tokens;

    auto& primary = clauses.front();
    const auto operation = config.canonical_term(primary.front());
    if (operation != "create") return corrected_tokens;

    bool has_folder_entity = false;
    for (const auto& token : primary) {
        if (config.canonical_term(token) == "folder") {
            has_folder_entity = true;
            break;
        }
    }
    if (!has_folder_entity) return corrected_tokens;

    std::vector<std::string> result = primary;
    bool has_name = std::find(result.begin(), result.end(), "named") != result.end();
    bool has_destination = false;
    for (const auto& token : result) {
        if (token == "on" || token == "in" || token == "inside" ||
            token == "within") {
            has_destination = true;
            break;
        }
    }

    for (std::size_t clause_index = 1; clause_index < clauses.size(); ++clause_index) {
        const auto& clause = clauses[clause_index];
        if (clause.empty()) continue;

        if (!has_name && is_name_phrase(clause)) {
            result.push_back("named");
            result.insert(result.end(), clause.begin() + 2, clause.end());
            has_name = true;
            continue;
        }

        if (!has_destination && is_location_phrase(clause)) {
            std::size_t position = 1;
            if (clause[position] == "it" || clause[position] == "that" ||
                clause[position] == "this") ++position;
            const auto marker = clause[position] == "to" ? "on" : clause[position];
            result.push_back(marker);
            result.insert(result.end(), clause.begin() +
                static_cast<std::ptrdiff_t>(position + 1), clause.end());
            has_destination = true;
        }
    }

    return result;
}

} // namespace seventh::grammar
