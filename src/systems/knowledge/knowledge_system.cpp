#include "seventh/systems/knowledge/knowledge_system.hpp"

#include "seventh/utilities/json_loader.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <utility>

namespace seventh::knowledge {
namespace {

std::string make_key(KnowledgeScope scope, const std::string& key) {
    return std::to_string(static_cast<int>(scope)) + ":" + key;
}

bool persistent_scope(KnowledgeScope scope) {
    return scope == KnowledgeScope::UserPreference ||
           scope == KnowledgeScope::Domain;
}

std::string scope_name(KnowledgeScope scope) {
    switch (scope) {
    case KnowledgeScope::UserPreference: return "user_preference";
    case KnowledgeScope::Domain: return "domain";
    case KnowledgeScope::TaskContext:
    case KnowledgeScope::LiveSystem:
        break;
    }
    return {};
}

std::optional<KnowledgeScope> parse_scope(const std::string& value) {
    if (value == "user_preference") return KnowledgeScope::UserPreference;
    if (value == "domain") return KnowledgeScope::Domain;
    return std::nullopt;
}

const utilities::JsonValue* member(
    const utilities::JsonValue::Object& object, const std::string& key) {
    const auto found = object.find(key);
    return found == object.end() ? nullptr : &found->second;
}

std::string escape_json(const std::string& value) {
    constexpr char hex[] = "0123456789abcdef";
    std::string output;
    output.reserve(value.size() + 8);

    for (const unsigned char ch : value) {
        switch (ch) {
        case '"': output += "\\\""; break;
        case '\\': output += "\\\\"; break;
        case '\b': output += "\\b"; break;
        case '\f': output += "\\f"; break;
        case '\n': output += "\\n"; break;
        case '\r': output += "\\r"; break;
        case '\t': output += "\\t"; break;
        default:
            if (ch < 0x20) {
                output += "\\u00";
                output.push_back(hex[(ch >> 4) & 0x0F]);
                output.push_back(hex[ch & 0x0F]);
            } else {
                output.push_back(static_cast<char>(ch));
            }
        }
    }
    return output;
}

} // namespace

std::optional<KnowledgeRecord> KnowledgeSystem::get(
    KnowledgeScope scope, const std::string& key) const {
    if (!persistent_scope(scope) || key.empty()) return std::nullopt;
    const auto found = records_.find(make_key(scope, key));
    if (found == records_.end()) return std::nullopt;
    return found->second;
}

bool KnowledgeSystem::put(const KnowledgeRecord& record, bool explicit_approval) {
    if (!persistent_scope(record.scope) || record.key.empty() ||
        !explicit_approval) {
        return false;
    }

    const auto storage_key = make_key(record.scope, record.key);
    const auto previous = records_.find(storage_key);
    const bool had_previous = previous != records_.end();
    KnowledgeRecord previous_record;
    if (had_previous) previous_record = previous->second;

    records_[storage_key] = record;
    if (!storage_path_.empty()) {
        std::string error;
        if (!save(error)) {
            if (had_previous) records_[storage_key] = std::move(previous_record);
            else records_.erase(storage_key);
            last_error_ = std::move(error);
            return false;
        }
    }

    last_error_.clear();
    return true;
}

bool KnowledgeSystem::erase(KnowledgeScope scope, const std::string& key,
                            bool explicit_approval) {
    if (!persistent_scope(scope) || key.empty() || !explicit_approval) return false;

    const auto storage_key = make_key(scope, key);
    const auto found = records_.find(storage_key);
    if (found == records_.end()) return false;

    KnowledgeRecord previous = found->second;
    records_.erase(found);
    if (!storage_path_.empty()) {
        std::string error;
        if (!save(error)) {
            records_[storage_key] = std::move(previous);
            last_error_ = std::move(error);
            return false;
        }
    }

    last_error_.clear();
    return true;
}

bool KnowledgeSystem::load_from_file(const std::string& path, std::string& error) {
    if (path.empty()) {
        error = "A knowledge storage path is required.";
        return false;
    }

    std::error_code fs_error;
    const bool exists = std::filesystem::exists(path, fs_error);
    if (fs_error) {
        error = "Unable to inspect the knowledge file: " + fs_error.message();
        return false;
    }

    if (!exists) {
        records_.clear();
        storage_path_ = path;
        last_error_.clear();
        error.clear();
        return true;
    }

    utilities::JsonLoader loader;
    auto parsed = loader.load_file(path, error);
    if (!parsed) return false;

    const auto* root = parsed->as_object();
    if (!root) {
        error = "The knowledge file root must be a JSON object.";
        return false;
    }

    const auto* version_value = member(*root, "version");
    const auto* version = version_value ? version_value->as_number() : nullptr;
    if (!version || *version != 1.0) {
        error = "The knowledge file has an unsupported or missing version.";
        return false;
    }

    const auto* records_value = member(*root, "records");
    const auto* records = records_value ? records_value->as_array() : nullptr;
    if (!records) {
        error = "The knowledge file must contain a records array.";
        return false;
    }

    std::map<std::string, KnowledgeRecord> loaded;
    for (std::size_t index = 0; index < records->size(); ++index) {
        const auto* object = (*records)[index].as_object();
        if (!object) {
            error = "Knowledge record " + std::to_string(index) +
                    " must be a JSON object.";
            return false;
        }

        const auto* scope_value = member(*object, "scope");
        const auto* key_value = member(*object, "key");
        const auto* data_value = member(*object, "value");
        const auto* source_value = member(*object, "source");

        const auto* scope_text = scope_value ? scope_value->as_string() : nullptr;
        const auto* key = key_value ? key_value->as_string() : nullptr;
        const auto* data = data_value ? data_value->as_string() : nullptr;
        const auto* source = source_value ? source_value->as_string() : nullptr;

        if (!scope_text || !key || key->empty() || !data ||
            (source_value && !source)) {
            error = "Knowledge record " + std::to_string(index) +
                    " has missing or invalid fields.";
            return false;
        }

        const auto scope = parse_scope(*scope_text);
        if (!scope) {
            error = "Knowledge record " + std::to_string(index) +
                    " uses a non-persistent or unknown scope.";
            return false;
        }

        KnowledgeRecord record;
        record.scope = *scope;
        record.key = *key;
        record.value = *data;
        record.source = source ? *source : std::string{};

        const auto storage_key = make_key(record.scope, record.key);
        if (!loaded.emplace(storage_key, std::move(record)).second) {
            error = "The knowledge file contains a duplicate scope/key record.";
            return false;
        }
    }

    records_ = std::move(loaded);
    storage_path_ = path;
    last_error_.clear();
    error.clear();
    return true;
}

bool KnowledgeSystem::save(std::string& error) const {
    if (storage_path_.empty()) {
        error = "Knowledge storage has not been configured.";
        return false;
    }

    std::error_code fs_error;
    const std::filesystem::path destination(storage_path_);
    const auto parent = destination.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, fs_error);
        if (fs_error) {
            error = "Unable to create the knowledge directory: " + fs_error.message();
            return false;
        }
    }

    auto temporary = destination;
    temporary += ".tmp";

    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output) {
        error = "Unable to create the temporary knowledge file.";
        return false;
    }

    output << "{\n  \"version\": 1,\n  \"records\": [";
    bool first = true;
    for (const auto& [key, record] : records_) {
        (void)key;
        if (!first) output << ',';
        first = false;
        output << "\n    {\n"
               << "      \"scope\": \"" << escape_json(scope_name(record.scope)) << "\",\n"
               << "      \"key\": \"" << escape_json(record.key) << "\",\n"
               << "      \"value\": \"" << escape_json(record.value) << "\",\n"
               << "      \"source\": \"" << escape_json(record.source) << "\"\n"
               << "    }";
    }
    if (!first) output << '\n';
    output << "  ]\n}\n";
    output.flush();
    if (!output) {
        output.close();
        std::filesystem::remove(temporary, fs_error);
        error = "Unable to write the knowledge file.";
        return false;
    }
    output.close();

    std::filesystem::permissions(
        temporary,
        std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
        std::filesystem::perm_options::replace, fs_error);
    if (fs_error) {
        std::filesystem::remove(temporary, fs_error);
        error = "Unable to restrict knowledge file permissions.";
        return false;
    }

    std::filesystem::rename(temporary, destination, fs_error);
    if (fs_error) {
        const auto rename_message = fs_error.message();
        std::error_code cleanup_error;
        std::filesystem::remove(temporary, cleanup_error);
        error = "Unable to replace the knowledge file: " + rename_message;
        return false;
    }

    error.clear();
    return true;
}

const std::string& KnowledgeSystem::storage_path() const noexcept {
    return storage_path_;
}

const std::string& KnowledgeSystem::last_error() const noexcept {
    return last_error_;
}

} // namespace seventh::knowledge
