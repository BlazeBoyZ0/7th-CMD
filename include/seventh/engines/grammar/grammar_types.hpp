#pragma once

#include <string>
#include <vector>

namespace seventh::grammar {

struct ParseMatch {
    std::string operation;
    std::string entity_type;
    std::vector<std::string> tokens;
    bool recognized{false};
    std::string error;
};

struct AstNode {
    std::string operation;
    std::string entity_type;
    std::vector<std::string> tokens;
    bool valid{false};
    std::string error;
};

struct CanonicalIntent {
    std::string operation;
    std::string entity_type;
    std::string name;
    std::string source;
    std::string destination;
    std::string query;
    std::string raw_input;
    std::vector<std::string> missing_information;
    bool valid{false};
    std::string error;
};

} // namespace seventh::grammar
