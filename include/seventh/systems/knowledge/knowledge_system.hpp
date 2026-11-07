#pragma once

#include "seventh/systems/knowledge/knowledge_interface.hpp"

#include <map>
#include <optional>
#include <string>

namespace seventh::knowledge {

class KnowledgeSystem final : public KnowledgeInterface {
public:
    [[nodiscard]] std::optional<KnowledgeRecord> get(
        KnowledgeScope scope, const std::string& key) const override;
    bool put(const KnowledgeRecord& record, bool explicit_approval) override;
    bool erase(KnowledgeScope scope, const std::string& key);

private:
    std::map<std::string, KnowledgeRecord> records_;
};

} // namespace seventh::knowledge
