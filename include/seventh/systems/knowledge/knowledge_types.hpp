#pragma once

#include <string>

namespace seventh::knowledge {

enum class KnowledgeScope {
    UserPreference,
    Domain,
    TaskContext,
    LiveSystem
};

struct KnowledgeRecord {
    KnowledgeScope scope{KnowledgeScope::Domain};
    std::string key;
    std::string value;
    std::string source;
};

struct ClarificationRequest {
    std::string task_id;
    std::string question;
    std::string required_field;
};

} // namespace seventh::knowledge
