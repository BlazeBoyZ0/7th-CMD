#include "seventh/engines/grammar/ast_generator.hpp"
namespace seventh::grammar {
AstNode AstGenerator::generate(const ParseMatch& match) const {
    AstNode node; node.operation=match.operation; node.entity_type=match.entity_type;
    node.pattern_id=match.pattern_id; node.tokens=match.tokens; node.slots=match.slots;
    node.missing_slots=match.missing_slots; node.valid=match.recognized; node.error=match.error;
    return node;
}
} // namespace seventh::grammar
