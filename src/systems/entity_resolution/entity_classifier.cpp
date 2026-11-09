#include "seventh/systems/entity_resolution/entity_classifier.hpp"

#include <cctype>
#include <sstream>
#include <string>

namespace seventh::entity_resolution {
namespace {

EntityType type_for_token(const std::string& token) {
    if (token == "folder" || token == "folders" ||
        token == "directory" || token == "directories") {
        return EntityType::Folder;
    }
    if (token == "file" || token == "files") return EntityType::File;
    if (token == "process" || token == "processes") return EntityType::Process;
    if (token == "website" || token == "websites" ||
        token == "site" || token == "sites") {
        return EntityType::Website;
    }
    if (token == "app" || token == "apps" ||
        token == "application" || token == "applications") {
        return EntityType::Application;
    }
    return EntityType::Unknown;
}

} // namespace

EntityType EntityClassifier::classify(const std::string& query) const {
    std::istringstream input(query);
    std::string token;
    EntityType detected = EntityType::Unknown;

    while (input >> token) {
        while (!token.empty() &&
               !std::isalnum(static_cast<unsigned char>(token.front()))) {
            token.erase(token.begin());
        }
        while (!token.empty() &&
               !std::isalnum(static_cast<unsigned char>(token.back()))) {
            token.pop_back();
        }

        for (char& ch : token) {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }

        const auto current = type_for_token(token);
        if (current == EntityType::Unknown) continue;
        if (detected != EntityType::Unknown && detected != current) {
            return EntityType::Unknown;
        }
        detected = current;
    }

    return detected;
}

} // namespace seventh::entity_resolution
