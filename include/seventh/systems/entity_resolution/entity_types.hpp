#pragma once

#include <string>
#include <vector>

namespace seventh::entity_resolution {

enum class EntityType {
    Unknown,
    Application,
    File,
    Folder,
    Process,
    Website
};

enum class ResolutionStatus {
    Found,
    Ambiguous,
    NotFound
};

struct EntityCandidate {
    std::string id;
    std::string name;
    EntityType type{EntityType::Unknown};
    std::string location;
    std::string source;
    bool available{true};
    int score{0};
};

struct ResolutionRequest {
    std::string query;
    EntityType expected_type{EntityType::Unknown};
    std::string context_hint;
    std::vector<EntityCandidate> candidates;
};

struct ResolutionResult {
    ResolutionStatus status{ResolutionStatus::NotFound};
    EntityCandidate selected;
    std::vector<EntityCandidate> candidates;
    std::string explanation;
};

} // namespace seventh::entity_resolution
