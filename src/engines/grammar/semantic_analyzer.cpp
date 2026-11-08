#include "seventh/engines/grammar/semantic_analyzer.hpp"
namespace seventh::grammar {
AstNode SemanticAnalyzer::analyze(AstNode node) const {
    if(!node.valid)return node;
    if(node.operation=="create"&&node.entity_type=="item") {
        node.missing_slots.push_back("entity_type");
    }
    return node;
}
} // namespace seventh::grammar
