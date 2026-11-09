#include "seventh/engines/grammar/grammar_engine.hpp"
#include "seventh/engines/grammar/ast_generator.hpp"
#include "seventh/engines/grammar/cir_generator.hpp"
#include "seventh/engines/grammar/grammar_parser.hpp"
#include "seventh/engines/grammar/semantic_analyzer.hpp"
#include "seventh/engines/grammar/phrase_composer.hpp"
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

bool is_courtesy_word(const std::string& token) {
    static const std::set<std::string> words{
        "hey", "hi", "hello", "please", "kindly", "dude"
    };
    return words.contains(token);
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

        std::size_t command_index = 0;
        while (command_index < detailed.size() &&
               is_courtesy_word(detailed[command_index].normalized)) {
            ++command_index;
        }

        const auto action = command_index < detailed.size()
            ? corrector.correct(detailed[command_index], action_vocabulary,
                                config_.spelling_corrections, true)
            : CorrectionResult{};
        if (action.changed) tokens[command_index] = action.value;

        // Only the first entity slot after the recognized command and its
        // articles/modifiers is eligible. Later tokens are user data.
        std::size_t entity_index = command_index + 1;
        while (entity_index < detailed.size() &&
               is_article_or_modifier(detailed[entity_index].normalized)) {
            ++entity_index;
        }
        if (entity_index < detailed.size()) {
            const auto entity = corrector.correct(
                detailed[entity_index], entity_vocabulary,
                config_.spelling_corrections, true);
            if (entity.changed) tokens[entity_index] = entity.value;
        }
    }

    const auto composed = PhraseComposer{}.compose(detailed, tokens, config_);
    const auto match = GrammarParser{}.parse(composed, config_);
    auto ast = SemanticAnalyzer{}.analyze(AstGenerator{}.generate(match));
    return CirGenerator{}.generate(ast, input);
}

} // namespace seventh::grammar
