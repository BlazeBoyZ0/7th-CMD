#pragma once
#include <cstddef>
#include <map>
#include <string>
#include <vector>
namespace seventh::grammar {
struct SlotValue {
    std::string value;
    bool explicitly_provided{true};
    std::size_t token_begin{0};
    std::size_t token_end{0};
};
struct ParseMatch {
    std::string operation, entity_type, pattern_id, error;
    std::vector<std::string> tokens, missing_slots;
    std::map<std::string, SlotValue> slots;
    bool recognized{false}, complete{false}, ambiguous{false};
};
struct AstNode {
    std::string operation, entity_type, pattern_id, error;
    std::vector<std::string> tokens, missing_slots;
    std::map<std::string, SlotValue> slots;
    bool valid{false};
};
struct CanonicalIntent {
    unsigned schema_version{1};
    std::string id, operation, entity_type, name, source, destination, query, raw_input, pattern_id;
    std::vector<std::string> missing_information;
    std::map<std::string, SlotValue> slots;
    bool valid{false};
    std::string error;
};
} // namespace seventh::grammar
