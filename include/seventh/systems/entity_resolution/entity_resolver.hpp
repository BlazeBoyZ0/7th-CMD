#pragma once

#include "seventh/systems/entity_resolution/ambiguity_detector.hpp"
#include "seventh/systems/entity_resolution/candidate_discovery.hpp"
#include "seventh/systems/entity_resolution/context_resolver.hpp"
#include "seventh/systems/entity_resolution/entity_classifier.hpp"
#include "seventh/systems/entity_resolution/entity_matcher.hpp"

namespace seventh::entity_resolution {

class EntityResolver {
public:
    [[nodiscard]] ResolutionResult resolve(ResolutionRequest request) const;

private:
    CandidateDiscovery discovery_;
    EntityClassifier classifier_;
    EntityMatcher matcher_;
    ContextResolver context_resolver_;
    AmbiguityDetector ambiguity_detector_;
};

} // namespace seventh::entity_resolution
