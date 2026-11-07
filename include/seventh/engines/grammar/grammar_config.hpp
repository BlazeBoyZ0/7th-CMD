#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

namespace seventh::grammar {

struct GrammarConfig {
    std::set<std::string> actions;
    std::set<std::string> entities;
    std::map<std::string, std::string> synonyms;
    std::map<std::string, std::string> spelling_corrections;
    std::vector<std::string> pattern_ids;

    [[nodiscard]] static GrammarConfig defaults();
    [[nodiscard]] std::string canonical_term(const std::string& term) const;
};

class GrammarConfigLoader {
public:
    [[nodiscard]] bool load_directory(const std::string& directory,
                                      GrammarConfig& config,
                                      std::string& error) const;
};

} // namespace seventh::grammar
