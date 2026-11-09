#include "seventh/engines/grammar/semantic_analyzer.hpp"

#include <algorithm>
#include <cctype>

namespace seventh::grammar {
namespace {

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

const SlotValue* find_slot(const AstNode& node, const char* name) {
    const auto found = node.slots.find(name);
    return found == node.slots.end() ? nullptr : &found->second;
}

} // namespace

AstNode SemanticAnalyzer::analyze(AstNode node) const {
    if (!node.valid) return node;

    if (node.operation == "create" && node.entity_type == "item") {
        node.missing_slots.push_back("entity_type");
    }

    // Turn an explicitly supplied destination into a typed semantic relation.
    // This records language meaning only; filesystem/entity resolution happens
    // later and must not be inferred from this grammar-level representation.
    if (node.operation == "create") {
        const auto* name = find_slot(node, "name");
        const auto* destination = find_slot(node, "destination");
        const auto* relation = find_slot(node, "destination_relation");

        if (name && !name->value.empty() &&
            destination && !destination->value.empty()) {
            const auto marker = relation ? lower(relation->value) : "in";
            std::string predicate = "located_in";
            if (marker == "on") predicate = "located_on";
            else if (marker == "inside" || marker == "within") {
                predicate = "contained_by";
            }

            const auto object = lower(destination->value);
            const bool pronoun = object == "it" || object == "that" ||
                                 object == "this" || object == "itt";

            node.relationships.push_back({
                name->value,
                predicate,
                destination->value,
                node.entity_type,
                "location_or_container",
                pronoun
            });
        }
    }

    return node;
}

} // namespace seventh::grammar
