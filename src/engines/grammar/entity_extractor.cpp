#include "seventh/engines/grammar/entity_extractor.hpp"
namespace seventh::grammar {
ExtractedEntity EntityExtractor::extract(const std::vector<std::string>& tokens, const GrammarConfig& config, std::size_t start) const {
    for (std::size_t i = start; i < tokens.size(); ++i) {
        const auto term = config.canonical_term(tokens[i]);
        if (config.entities.contains(term)) return {term, i, true};
    }
    return {};
}
} // namespace seventh::grammar
