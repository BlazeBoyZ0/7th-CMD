#include "seventh/engines/grammar/grammar_config.hpp"

#include "seventh/utilities/json_loader.hpp"

#include <filesystem>

namespace seventh::grammar {
namespace {

bool read_string(const utilities::JsonValue::Object& object,
                 const std::string& key, std::string& output) {
    const auto found = object.find(key);
    if (found == object.end() || !found->second.as_string()) return false;
    output = *found->second.as_string();
    return true;
}

} // namespace

GrammarConfig GrammarConfig::defaults() {
    GrammarConfig config;
    config.actions = {"create", "rename", "move", "copy", "delete", "search", "open"};
    config.entities = {"folder", "file", "application", "process", "item"};
    config.synonyms = {
        {"make", "create"}, {"duplicate", "copy"}, {"remove", "delete"},
        {"find", "search"}, {"launch", "open"}, {"start", "open"},
        {"directory", "folder"}, {"app", "application"}
    };
    return config;
}

std::string GrammarConfig::canonical_term(const std::string& term) const {
    const auto found = synonyms.find(term);
    return found == synonyms.end() ? term : found->second;
}

bool GrammarConfigLoader::load_directory(
    const std::string& directory, GrammarConfig& config, std::string& error) const {
    utilities::JsonLoader loader;
    const auto root = std::filesystem::path(directory);

    const auto vocabulary = loader.load_file((root / "vocabulary.json").string(), error);
    if (!vocabulary || !vocabulary->as_object()) return false;
    const auto entries = vocabulary->as_object()->find("entries");
    if (entries == vocabulary->as_object()->end() || !entries->second.as_array()) {
        error = "Grammar vocabulary is missing its entries array.";
        return false;
    }

    GrammarConfig loaded;
    for (const auto& entry : *entries->second.as_array()) {
        const auto* object = entry.as_object();
        if (!object) continue;
        std::string term;
        std::string category;
        if (!read_string(*object, "term", term) ||
            !read_string(*object, "category", category)) continue;
        if (category == "action") loaded.actions.insert(term);
        else if (category == "entity") loaded.entities.insert(term);
    }

    const auto synonyms = loader.load_file((root / "synonyms.json").string(), error);
    if (!synonyms || !synonyms->as_object()) return false;
    const auto groups = synonyms->as_object()->find("groups");
    if (groups == synonyms->as_object()->end() || !groups->second.as_array()) {
        error = "Grammar synonyms are missing their groups array.";
        return false;
    }

    for (const auto& group : *groups->second.as_array()) {
        const auto* object = group.as_object();
        if (!object) continue;
        std::string canonical;
        if (!read_string(*object, "canonical", canonical)) continue;
        const auto terms = object->find("terms");
        if (terms == object->end() || !terms->second.as_array()) continue;
        for (const auto& term : *terms->second.as_array()) {
            if (term.as_string()) loaded.synonyms[*term.as_string()] = canonical;
        }
    }

    if (loaded.actions.empty() || loaded.entities.empty()) {
        error = "Grammar vocabulary must define at least one action and one entity.";
        return false;
    }

    const auto patterns = loader.load_file((root / "patterns.json").string(), error);
    if (patterns && patterns->as_object()) {
        const auto items = patterns->as_object()->find("patterns");
        if (items != patterns->as_object()->end() && items->second.as_array()) {
            for (const auto& item : *items->second.as_array()) {
                if (!item.as_object()) continue;
                std::string id;
                if (read_string(*item.as_object(), "id", id)) {
                    loaded.pattern_ids.push_back(std::move(id));
                }
            }
        }
    }

    config = std::move(loaded);
    return true;
}

} // namespace seventh::grammar
