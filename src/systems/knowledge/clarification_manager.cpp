#include "seventh/systems/knowledge/clarification_manager.hpp"

#include <algorithm>

namespace seventh::knowledge {

void ClarificationManager::request(const ClarificationRequest& request) {
    if (request.task_id.empty() || request.required_field.empty()) return;
    auto& requests = pending_[request.task_id];
    const auto duplicate = std::find_if(requests.begin(), requests.end(),
        [&](const ClarificationRequest& existing) {
            return existing.required_field == request.required_field;
        });
    if (duplicate == requests.end()) requests.push_back(request);
}

std::optional<ClarificationRequest> ClarificationManager::next(
    const std::string& task_id) const {
    const auto found = pending_.find(task_id);
    if (found == pending_.end() || found->second.empty()) return std::nullopt;
    return found->second.front();
}

bool ClarificationManager::resolve(const std::string& task_id,
                                    const std::string& required_field) {
    const auto found = pending_.find(task_id);
    if (found == pending_.end()) return false;
    auto& requests = found->second;
    const auto previous_size = requests.size();
    requests.erase(std::remove_if(requests.begin(), requests.end(),
        [&](const ClarificationRequest& request) {
            return request.required_field == required_field;
        }), requests.end());
    const bool removed = requests.size() != previous_size;
    if (requests.empty()) pending_.erase(found);
    return removed;
}

void ClarificationManager::cancel_task(const std::string& task_id) {
    pending_.erase(task_id);
}

} // namespace seventh::knowledge
