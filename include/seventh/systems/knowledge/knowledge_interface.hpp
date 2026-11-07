#pragma once

#include "seventh/systems/knowledge/knowledge_types.hpp"

#include <optional>

namespace seventh::knowledge {

class KnowledgeInterface {
public:
    virtual ~KnowledgeInterface() = default;
    [[nodiscard]] virtual std::optional<KnowledgeRecord> get(
        KnowledgeScope scope, const std::string& key) const = 0;
    virtual bool put(const KnowledgeRecord& record, bool explicit_approval) = 0;
};

} // namespace seventh::knowledge
