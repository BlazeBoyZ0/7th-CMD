#include "seventh/engines/grammar/grammar_engine.hpp"
#include "seventh/engines/grammar/ast_generator.hpp"
#include "seventh/engines/grammar/cir_generator.hpp"
#include "seventh/engines/grammar/grammar_parser.hpp"
#include "seventh/engines/grammar/semantic_analyzer.hpp"
#include "seventh/engines/grammar/typo_corrector.hpp"

#include <set>
#include <utility>

namespace seventh::grammar {
namespace {

std::set<std::string> vocabulary_with_aliases(
    const std::set<std::string>& canonical_terms,
    const std::map<std::string, std::string>& synonyms) {
    auto vocabulary = canonical_terms;
    for (const auto& [alias, canonical] : synonyms) {
        if (canonical_terms.contains(canonical)) vocabulary.insert(alias);
    }
    return vocabulary;
}

bool is_article_or_modifier(const std::string& token) {
    return token == "a" || token == "an" || token == "the" || token == "new";
}

} // namespace

GrammarEngine::GrammarEngine(GrammarConfig config) : config_(std::move(config)) {}

CanonicalIntent GrammarEngine::interpret(const std::string& input) const {
    const auto detailed = input_processor_.process_detailed(input);
    std::vector<std::string> tokens;
    tokens.reserve(detailed.size());
    for (const auto& token : detailed) tokens.push_back(token.text);

    if (!detailed.empty()) {
        const TypoCorrector corrector;
        const auto action_vocabulary =
            vocabulary_with_aliases(config_.actions, config_.synonyms);
        const auto entity_vocabulary =
            vocabulary_with_aliases(config_.entities, config_.synonyms);

        const auto action = corrector.correct(
            detailed.front(), action_vocabulary,
            config_.spelling_corrections, true);
        if (action.changed) tokens.front() = action.value;

        // Only the first entity slot after articles/modifiers is eligible.
        // Later tokens may be filenames, folder names, paths, or search data.
        std::size_t entity_index = 1;
        while (entity_index < detailed.size() &&
               is_article_or_modifier(tokens[entity_index])) {
            ++entity_index;
        }
        if (entity_index < detailed.size()) {
            const auto entity = corrector.correct(
                detailed[entity_index], entity_vocabulary,
                config_.spelling_corrections, true);
            if (entity.changed) tokens[entity_index] = entity.value;
        }
    }

    const auto match = GrammarParser{}.parse(tokens, config_);
    auto ast = SemanticAnalyzer{}.analyze(AstGenerator{}.generate(match));
    return CirGenerator{}.generate(ast, input);
}

} // namespace seventh::grammar
