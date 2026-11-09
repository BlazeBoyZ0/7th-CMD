#include "seventh/systems/knowledge/knowledge_updater.hpp"

namespace seventh::knowledge {

KnowledgeUpdater::KnowledgeUpdater(KnowledgeInterface& knowledge)
    : knowledge_(knowledge) {}

bool KnowledgeUpdater::apply_user_approved_update(
    const KnowledgeRecord& record, bool user_approved) {
    if (!user_approved) return false;
    return knowledge_.put(record, true);
}

} // namespace seventh::knowledge
