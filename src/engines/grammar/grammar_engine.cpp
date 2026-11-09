#include "seventh/engines/grammar/grammar_engine.hpp"
#include "seventh/engines/grammar/ast_generator.hpp"
#include "seventh/engines/grammar/cir_generator.hpp"
#include "seventh/engines/grammar/grammar_parser.hpp"
#include "seventh/engines/grammar/semantic_analyzer.hpp"
#include <utility>
namespace seventh::grammar {
GrammarEngine::GrammarEngine(GrammarConfig config):config_(std::move(config)){}
CanonicalIntent GrammarEngine::interpret(const std::string& input) const {
    const auto tokens=input_processor_.process(input);
    const auto match=GrammarParser{}.parse(tokens,config_);
    auto ast=SemanticAnalyzer{}.analyze(AstGenerator{}.generate(match));
    return CirGenerator{}.generate(ast,input);
}
} // namespace seventh::grammar
