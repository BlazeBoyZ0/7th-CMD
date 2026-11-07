#include "seventh/systems/knowledge/knowledge_system.hpp"

namespace seventh::knowledge {
namespace {

std::string make_key(KnowledgeScope scope, const std::string& key) {
    return std::to_string(static_cast<int>(scope)) + ":" + key;
}

bool requires_approval(KnowledgeScope scope) {
    return scope == KnowledgeScope::UserPreference ||
           scope == KnowledgeScope::Domain;
}

} // namespace

std::optional<KnowledgeRecord> KnowledgeSystem::get(
    KnowledgeScope scope, const std::string& key) const {
    const auto found = records_.find(make_key(scope, key));
    if (found == records_.end()) return std::nullopt;
    return found->second;
}

bool KnowledgeSystem::put(const KnowledgeRecord& record, bool explicit_approval) {
    if (record.key.empty() || record.scope == KnowledgeScope::LiveSystem ||
        record.scope == KnowledgeScope::TaskContext) {
        return false;
    }
    if (requires_approval(record.scope) && !explicit_approval) return false;
    records_[make_key(record.scope, record.key)] = record;
    return true;
}

bool KnowledgeSystem::erase(KnowledgeScope scope, const std::string& key) {
    return records_.erase(make_key(scope, key)) != 0;
}

} // namespace seventh::knowledge
