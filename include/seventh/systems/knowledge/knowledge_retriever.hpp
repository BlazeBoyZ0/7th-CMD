#pragma once

#include "seventh/systems/knowledge/knowledge_interface.hpp"

#include <optional>

namespace seventh::knowledge {

class KnowledgeRetriever {
public:
    explicit KnowledgeRetriever(const KnowledgeInterface& knowledge);
    [[nodiscard]] std::optional<KnowledgeRecord> retrieve(
        KnowledgeScope scope, const std::string& key) const;

private:
    const KnowledgeInterface& knowledge_;
};

} // namespace seventh::knowledge
