#include "seventh/engines/grammar/ast_generator.hpp"

namespace seventh::grammar {

AstNode AstGenerator::generate(const ParseMatch& match) const {
    AstNode node;
    node.operation = match.operation;
    node.entity_type = match.entity_type;
    node.tokens = match.tokens;
    node.valid = match.recognized;
    node.error = match.error;
    return node;
}

} // namespace seventh::grammar
