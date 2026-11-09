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
    bool erase(KnowledgeScope scope, const std::string& key,
               bool explicit_approval = false);

    // Loads only persistent user-preference and domain records. Missing files
    // are treated as an empty store; live OS and task-context data are excluded.
    bool load_from_file(const std::string& path, std::string& error);
    bool save(std::string& error) const;

    [[nodiscard]] const std::string& storage_path() const noexcept;
    [[nodiscard]] const std::string& last_error() const noexcept;

private:
    std::map<std::string, KnowledgeRecord> records_;
    std::string storage_path_;
    std::string last_error_;
};

} // namespace seventh::knowledge
