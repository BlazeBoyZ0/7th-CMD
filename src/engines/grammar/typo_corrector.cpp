#include "seventh/engines/grammar/typo_corrector.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <vector>

namespace seventh::grammar {
namespace {

std::string lowercase_ascii(std::string value) {
    for (char& character : value) {
        character = static_cast<char>(
            std::tolower(static_cast<unsigned char>(character)));
    }
    return value;
}

bool is_ascii_word(const std::string& value) {
    if (value.size() < 3) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return (character >= 'a' && character <= 'z') ||
               (character >= 'A' && character <= 'Z');
    });
}

} // namespace

std::size_t TypoCorrector::damerau_levenshtein(
    const std::string& left, const std::string& right) {
    const std::size_t rows = left.size() + 1;
    const std::size_t columns = right.size() + 1;
    std::vector<std::vector<std::size_t>> distance(
        rows, std::vector<std::size_t>(columns, 0));

    for (std::size_t row = 0; row < rows; ++row) distance[row][0] = row;
    for (std::size_t column = 0; column < columns; ++column) {
        distance[0][column] = column;
    }

    for (std::size_t row = 1; row < rows; ++row) {
        for (std::size_t column = 1; column < columns; ++column) {
            const std::size_t substitution =
                left[row - 1] == right[column - 1] ? 0 : 1;
            distance[row][column] = std::min({
                distance[row - 1][column] + 1,
                distance[row][column - 1] + 1,
                distance[row - 1][column - 1] + substitution
            });

            if (row > 1 && column > 1 &&
                left[row - 1] == right[column - 2] &&
                left[row - 2] == right[column - 1]) {
                distance[row][column] = std::min(
                    distance[row][column],
                    distance[row - 2][column - 2] + 1);
            }
        }
    }
    return distance.back().back();
}

CorrectionResult TypoCorrector::correct(
    const Token& token,
    const std::set<std::string>& vocabulary,
    const std::map<std::string, std::string>& explicit_corrections,
    bool grammar_vocabulary_position) const {
    CorrectionResult result{token.text, 0, false, false};

    // Quoted values and tokens not explicitly identified as grammar vocabulary
    // are user data (names, paths, filenames, search terms, etc.).
    if (!grammar_vocabulary_position || token.quoted ||
        !is_ascii_word(token.text)) {
        return result;
    }

    const auto word = lowercase_ascii(token.text);
    const auto explicit_entry = explicit_corrections.find(word);
    if (explicit_entry != explicit_corrections.end()) {
        const auto target = lowercase_ascii(explicit_entry->second);
        if (vocabulary.contains(target)) {
            result.value = target;
            result.changed = result.value != word;
            return result;
        }
    }

    if (vocabulary.contains(word)) {
        result.value = word;
        return result;
    }

    // Keep short-word correction strict; allow one extra edit only for long
    // words where a two-character typo is less likely to be accidental data.
    const std::size_t maximum_distance = word.size() >= 8 ? 2 : 1;
    std::size_t best_distance = std::numeric_limits<std::size_t>::max();
    std::string best_candidate;
    bool tied = false;

    for (const auto& candidate_value : vocabulary) {
        const auto candidate = lowercase_ascii(candidate_value);
        if (!is_ascii_word(candidate)) continue;
        const auto length_difference = word.size() > candidate.size()
            ? word.size() - candidate.size()
            : candidate.size() - word.size();
        if (length_difference > maximum_distance) continue;

        const auto distance = damerau_levenshtein(word, candidate);
        if (distance == 0 || distance > maximum_distance) continue;

        if (distance < best_distance) {
            best_distance = distance;
            best_candidate = candidate;
            tied = false;
        } else if (distance == best_distance && candidate != best_candidate) {
            tied = true;
        }
    }

    if (best_candidate.empty()) return result;
    result.distance = best_distance;
    if (tied) {
        result.ambiguous = true;
        return result;
    }

    result.value = best_candidate;
    result.changed = true;
    return result;
}

} // namespace seventh::grammar
