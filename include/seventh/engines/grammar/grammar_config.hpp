#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>
namespace seventh::grammar {
struct IntentPattern {
    std::string id, operation, entity_type;
    std::vector<std::string> target_markers, source_markers, destination_markers, query_markers, location_markers;
};
struct GrammarConfig {
    std::set<std::string> actions, entities;
    std::map<std::string, std::string> synonyms, spelling_corrections;
    std::vector<std::string> pattern_ids;
    std::vector<IntentPattern> patterns;
    [[nodiscard]] static GrammarConfig defaults();
    [[nodiscard]] std::string canonical_term(const std::string& term) const;
};
class GrammarConfigLoader {
public:
    [[nodiscard]] bool load_directory(const std::string& directory, GrammarConfig& config, std::string& error) const;
};
} // namespace seventh::grammar
