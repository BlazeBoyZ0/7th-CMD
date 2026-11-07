#include "seventh/engines/grammar/grammar_engine.hpp"

#include "seventh/engines/grammar/ast_generator.hpp"
#include "seventh/engines/grammar/cir_generator.hpp"
#include "seventh/engines/grammar/grammar_parser.hpp"
#include "seventh/engines/grammar/semantic_analyzer.hpp"

namespace seventh::grammar {

CanonicalIntent GrammarEngine::interpret(const std::string& input) const {
    const auto tokens = input_processor_.process(input);
    GrammarParser parser;
    AstGenerator ast_generator;
    SemanticAnalyzer semantic_analyzer;
    CirGenerator cir_generator;

    const auto match = parser.parse(tokens);
    auto ast = ast_generator.generate(match);
    ast = semantic_analyzer.analyze(std::move(ast));
    return cir_generator.generate(ast, input);
}

} // namespace seventh::grammar
