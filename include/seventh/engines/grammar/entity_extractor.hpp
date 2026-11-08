#pragma once
#include "seventh/engines/grammar/grammar_config.hpp"
#include <string>
#include <vector>
namespace seventh::grammar {
struct ExtractedEntity { std::string type; std::size_t token_index{0}; bool found{false}; };
class EntityExtractor {
public:
    [[nodiscard]] ExtractedEntity extract(const std::vector<std::string>& tokens, const GrammarConfig& config, std::size_t start = 1) const;
};
} // namespace seventh::grammar
