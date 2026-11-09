#include "seventh/engines/grammar/phrase_composer.hpp"

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <set>

namespace seventh::grammar {
namespace {

bool contains_any(const std::string& punctuation, std::string_view values) {
    return std::any_of(punctuation.begin(), punctuation.end(),
        [&](char value) { return values.find(value) != std::string_view::npos; });
}

bool is_polite_word(const std::string& token) {
    static const std::set<std::string> words{
        "hey", "hi", "hello", "please", "kindly", "dude",
        "okay", "ok", "thanks", "thank", "you"
    };
    return words.contains(token);
}

bool is_polite_clause(const std::vector<std::string>& clause) {
    return !clause.empty() &&
        std::all_of(clause.begin(), clause.end(), is_polite_word);
}

bool is_reference_pronoun(const std::string& token) {
    return token == "it" || token == "that" || token == "this" ||
           token == "itt";
}

bool is_name_phrase(const std::vector<std::string>& clause) {
    return clause.size() >= 3 &&
        ((clause[0] == "name" || clause[0] == "call") &&
         is_reference_pronoun(clause[1]));
}

bool is_location_phrase(const std::vector<std::string>& clause) {
    if (clause.size() < 3) return false;
    const bool verb = clause[0] == "put" || clause[0] == "place" ||
                      clause[0] == "keep" || clause[0] == "save" ||
                      clause[0] == "move";
    if (!verb) return false;
    std::size_t position = 1;
    if (is_reference_pronoun(clause[position])) ++position;
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
        if (tokens[i] == "and" && i + 1 < tokens.size()) {
            const auto& next = tokens[i + 1];
            const bool follow_up = next == "put" || next == "place" ||
                next == "keep" || next == "save" || next == "move" ||
                next == "name" || next == "call";
            if (follow_up && current.empty()) continue;
            if (follow_up && !current.empty()) {
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
    if (clauses.empty()) return corrected_tokens;

    // Ignore only a small, explicit set of conversational courtesy words.
    // Unknown leading text is never silently discarded.
    std::size_t primary_index = clauses.size();
    for (std::size_t i = 0; i < clauses.size(); ++i) {
        auto& clause = clauses[i];
        while (!clause.empty() && is_polite_word(clause.front())) {
            clause.erase(clause.begin());
        }
        if (clause.empty()) continue;
        if (config.canonical_term(clause.front()) == "create") {
            primary_index = i;
            break;
        }
        if (!is_polite_clause(clause)) return corrected_tokens;
    }
    if (primary_index == clauses.size()) return corrected_tokens;

    auto& primary = clauses[primary_index];
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

    for (std::size_t clause_index = primary_index + 1;
         clause_index < clauses.size(); ++clause_index) {
        const auto& clause = clauses[clause_index];
        if (clause.empty() || is_polite_clause(clause)) continue;

        if (!has_name && is_name_phrase(clause)) {
            result.push_back("named");
            result.insert(result.end(), clause.begin() + 2, clause.end());
            has_name = true;
            continue;
        }

        if (!has_destination && is_location_phrase(clause)) {
            std::size_t position = 1;
            if (is_reference_pronoun(clause[position])) ++position;
            const std::string marker =
                clause[position] == "to" ? "on" : clause[position];
            result.push_back(marker);
            result.insert(result.end(), clause.begin() +
                static_cast<std::ptrdiff_t>(position + 1), clause.end());
            has_destination = true;
        }
    }
    return result;
}

} // namespace seventh::grammar
