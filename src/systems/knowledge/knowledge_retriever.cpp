#include "seventh/systems/knowledge/knowledge_retriever.hpp"

namespace seventh::knowledge {

KnowledgeRetriever::KnowledgeRetriever(const KnowledgeInterface& knowledge)
    : knowledge_(knowledge) {}

std::optional<KnowledgeRecord> KnowledgeRetriever::retrieve(
    KnowledgeScope scope, const std::string& key) const {
    if (scope == KnowledgeScope::LiveSystem ||
        scope == KnowledgeScope::TaskContext) return std::nullopt;
    return knowledge_.get(scope, key);
}

} // namespace seventh::knowledge
