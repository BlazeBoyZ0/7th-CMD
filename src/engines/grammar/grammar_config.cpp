#include "seventh/engines/grammar/grammar_config.hpp"

#include "seventh/utilities/json_loader.hpp"

#include <algorithm>
#include <cctype>
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

std::string lower(std::string value) {
    for (char& character : value) {
        character = static_cast<char>(
            std::tolower(static_cast<unsigned char>(character)));
    }
    return value;
}

} // namespace

GrammarConfig GrammarConfig::defaults() {
    GrammarConfig config;
    config.actions = {"create", "rename", "move", "copy", "delete", "search", "open", "count"};
    config.entities = {"folder", "file", "application", "process", "item"};
    config.synonyms = {
        {"make", "create"}, {"duplicate", "copy"}, {"remove", "delete"},
        {"find", "search"}, {"launch", "open"}, {"start", "open"},
        {"directory", "folder"}, {"app", "application"}
    };
    config.spelling_corrections = {
        {"creat", "create"}, {"foler", "folder"},
        {"lnes", "lines"}, {"muxh", "much"}
    };
    config.patterns = {
        {"create_named_entity","create","folder",{"named"},{},{},{},{"on","in","inside","within"}},
        {"rename_entity","rename","file",{},{},{"to"},{},{}},
        {"move_entity","move","",{},{},{"to"},{},{}},
        {"copy_entity","copy","",{},{},{"to"},{},{}},
        {"delete_entity","delete","",{},{},{},{},{}},
        {"search_entity","search","",{}, {}, {}, {"named","for"},{"in","on","inside","within"}},
        {"open_application","open","application",{},{},{},{},{}}
    };
    return config;
}

std::string GrammarConfig::canonical_term(const std::string& term) const {
    const auto normalized = lower(term);
    const auto synonym = synonyms.find(normalized);
    if (synonym != synonyms.end()) return synonym->second;
    const auto correction = spelling_corrections.find(normalized);
    return correction == spelling_corrections.end() ? normalized : correction->second;
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

    GrammarConfig loaded = GrammarConfig::defaults();
    loaded.actions.clear();
    loaded.entities.clear();
    loaded.synonyms.clear();
    loaded.spelling_corrections.clear();
    loaded.pattern_ids.clear();
    for (const auto& entry : *entries->second.as_array()) {
        const auto* object = entry.as_object();
        if (!object) continue;
        std::string term;
        std::string category;
        if (!read_string(*object, "term", term) ||
            !read_string(*object, "category", category)) continue;
        if (category == "action") loaded.actions.insert(lower(term));
        else if (category == "entity") loaded.entities.insert(lower(term));
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
            if (term.as_string()) {
                loaded.synonyms[lower(*term.as_string())] = lower(canonical);
            }
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
        const auto* object = pattern.as_object();
        if (!object) continue;
        IntentPattern definition;
        if (!read_string(*object, "id", definition.id) ||
            !read_string(*object, "operation", definition.operation)) continue;
        read_string(*object, "entity_type", definition.entity_type);
        const auto read_markers = [&](const char* key, std::vector<std::string>& output) {
            const auto found = object->find(key);
            if (found == object->end() || !found->second.as_array()) return;
            for (const auto& item : *found->second.as_array()) {
                if (item.as_string()) output.push_back(lower(*item.as_string()));
            }
        };
        read_markers("target_markers", definition.target_markers);
        read_markers("source_markers", definition.source_markers);
        read_markers("destination_markers", definition.destination_markers);
        read_markers("query_markers", definition.query_markers);
        read_markers("location_markers", definition.location_markers);
        loaded.pattern_ids.push_back(definition.id);
        loaded.patterns.push_back(std::move(definition));
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
            loaded.spelling_corrections[lower(from)] = lower(to);
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
