#pragma once

#include "seventh/systems/knowledge/knowledge_interface.hpp"

namespace seventh::knowledge {

class KnowledgeUpdater {
public:
    explicit KnowledgeUpdater(KnowledgeInterface& knowledge);
    bool apply_user_approved_update(const KnowledgeRecord& record,
                                    bool user_approved);

private:
    KnowledgeInterface& knowledge_;
};

} // namespace seventh::knowledge
