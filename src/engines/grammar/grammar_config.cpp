#include "seventh/engines/grammar/grammar_config.hpp"

#include "seventh/utilities/json_loader.hpp"

#include <filesystem>
#include <utility>

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
    const auto synonym = synonyms.find(term);
    if (synonym != synonyms.end()) return synonym->second;
    const auto correction = spelling_corrections.find(term);
    return correction == spelling_corrections.end() ? term : correction->second;
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

    const auto patterns = loader.load_file((root / "patterns.json").string(), error);
    if (!patterns || !patterns->as_object()) return false;
    const auto pattern_list = patterns->as_object()->find("patterns");
    if (pattern_list == patterns->as_object()->end() ||
        !pattern_list->second.as_array()) {
        error = "Grammar patterns are missing their patterns array.";
        return false;
    }
    for (const auto& pattern : *pattern_list->second.as_array()) {
        if (!pattern.as_object()) continue;
        std::string id;
        if (read_string(*pattern.as_object(), "id", id)) {
            loaded.pattern_ids.push_back(std::move(id));
        }
    }

    const auto spelling = loader.load_file((root / "spelling.json").string(), error);
    if (!spelling || !spelling->as_object()) return false;
    const auto corrections = spelling->as_object()->find("corrections");
    if (corrections == spelling->as_object()->end() ||
        !corrections->second.as_array()) {
        error = "Grammar spelling data is missing its corrections array.";
        return false;
    }
    for (const auto& correction : *corrections->second.as_array()) {
        if (!correction.as_object()) continue;
        std::string from;
        std::string to;
        if (read_string(*correction.as_object(), "from", from) &&
            read_string(*correction.as_object(), "to", to)) {
            loaded.spelling_corrections[std::move(from)] = std::move(to);
        }
    }

    if (loaded.actions.empty() || loaded.entities.empty()) {
        error = "Grammar vocabulary must define at least one action and one entity.";
        return false;
    }

    config = std::move(loaded);
    error.clear();
    return true;
}

} // namespace seventh::grammar
