#include "seventh/systems/entity_resolution/entity_classifier.hpp"

#include <algorithm>
#include <cctype>

namespace seventh::entity_resolution {
namespace {

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

} // namespace

EntityType EntityClassifier::classify(const std::string& query) const {
    const auto value = lower(query);
    if (value.find("folder") != std::string::npos ||
        value.find("directory") != std::string::npos) return EntityType::Folder;
    if (value.find("file") != std::string::npos) return EntityType::File;
    if (value.find("process") != std::string::npos) return EntityType::Process;
    if (value.find("website") != std::string::npos ||
        value.find("site") != std::string::npos) return EntityType::Website;
    if (value.find("app") != std::string::npos ||
        value.find("application") != std::string::npos) return EntityType::Application;
    return EntityType::Unknown;
}

} // namespace seventh::entity_resolution
