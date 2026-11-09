#pragma once

#include "seventh/systems/knowledge/knowledge_types.hpp"

#include <map>
#include <optional>
#include <vector>

namespace seventh::knowledge {

class ClarificationManager {
public:
    void request(const ClarificationRequest& request);
    [[nodiscard]] std::optional<ClarificationRequest> next(
        const std::string& task_id) const;
    bool resolve(const std::string& task_id, const std::string& required_field);
    void cancel_task(const std::string& task_id);

private:
    std::map<std::string, std::vector<ClarificationRequest>> pending_;
};

} // namespace seventh::knowledge
