#include "seventh/engines/grammar/semantic_analyzer.hpp"

namespace seventh::grammar {

AstNode SemanticAnalyzer::analyze(AstNode node) const {
    if (!node.valid) return node;

    if (node.tokens.size() < 2) {
        node.valid = false;
        node.error = "The instruction is missing its target.";
        return node;
    }

    if ((node.operation == "rename" || node.operation == "move" ||
         node.operation == "copy") &&
        node.tokens.size() < 4) {
        node.valid = false;
        node.error = "This operation needs a source and a destination.";
        return node;
    }

    if (node.operation == "create" && node.entity_type == "item") {
        node.valid = false;
        node.error = "Please specify what to create, such as a folder or file.";
    }

    return node;
}

} // namespace seventh::grammar
