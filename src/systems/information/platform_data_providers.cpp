#include "seventh/systems/information/platform_data_providers.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <utility>

namespace seventh::information {
namespace {

class FilesystemDataProvider final : public DataProvider {
public:
    explicit FilesystemDataProvider(platform::PlatformInterface& platform)
        : platform_(platform) {}

    std::string_view id() const noexcept override { return "filesystem"; }

    bool supports(const DataRequest& request) const override {
        return request.provider_id == id() && !request.source_path.empty();
    }

    bool acquire(const DataRequest& request, std::vector<DataRecord>& records,
                 std::string& error) override {
        const auto entries = platform_.filesystem().list(request.source_path, error);
        if (!error.empty()) return false;

        records.clear();
        records.reserve(entries.size());
        for (const auto& entry : entries) {
            records.push_back({
                {"name", entry.name},
                {"path", entry.path},
                {"type", entry.directory ? "folder" : "file"},
                {"directory", entry.directory ? "true" : "false"},
                {"size", std::to_string(entry.size)},
                {"extension", std::filesystem::path(entry.name).extension().string()}
            });
        }
        return true;
    }

private:
    platform::PlatformInterface& platform_;
};

class DocumentDataProvider final : public DataProvider {
public:
    explicit DocumentDataProvider(platform::PlatformInterface& platform)
        : platform_(platform) {}

    std::string_view id() const noexcept override { return "document"; }

    bool supports(const DataRequest& request) const override {
        return request.provider_id == id() && !request.source_path.empty();
    }

    bool acquire(const DataRequest& request, std::vector<DataRecord>& records,
                 std::string& error) override {
        auto text = platform_.filesystem().read_text(request.source_path, error);
        if (!text) return false;
        if (text->find('\\0') != std::string::npos) {
            error = "The document provider refuses binary file content.";
            return false;
        }

        records.clear();
        records.push_back({
            {"path", request.source_path},
            {"text", *text},
            {"bytes", std::to_string(text->size())}
        });
        return true;
    }

private:
    platform::PlatformInterface& platform_;
};

class ApplicationDataProvider final : public DataProvider {
public:
    explicit ApplicationDataProvider(platform::PlatformInterface& platform)
        : platform_(platform) {}

    std::string_view id() const noexcept override { return "applications"; }

    bool supports(const DataRequest& request) const override {
        return request.provider_id == id();
    }

    bool acquire(const DataRequest&, std::vector<DataRecord>& records,
                 std::string& error) override {
        const auto applications = platform_.applications().list(error);
        if (!error.empty()) return false;

        records.clear();
        records.reserve(applications.size());
        for (const auto& app : applications) {
            records.push_back({
                {"id", app.id},
                {"name", app.name},
                {"executable", app.executable},
                {"desktop_file", app.desktop_file}
            });
        }
        return true;
    }

private:
    platform::PlatformInterface& platform_;
};

class ProcessDataProvider final : public DataProvider {
public:
    explicit ProcessDataProvider(platform::PlatformInterface& platform)
        : platform_(platform) {}

    std::string_view id() const noexcept override { return "processes"; }

    bool supports(const DataRequest& request) const override {
        return request.provider_id == id();
    }

    bool acquire(const DataRequest&, std::vector<DataRecord>& records,
                 std::string& error) override {
        const auto processes = platform_.processes().list(error);
        if (!error.empty()) return false;

        records.clear();
        records.reserve(processes.size());
        for (const auto& process : processes) {
            records.push_back({
                {"pid", std::to_string(process.id)},
                {"name", process.name},
                {"executable", process.executable}
            });
        }
        return true;
    }

private:
    platform::PlatformInterface& platform_;
};

class SystemInformationDataProvider final : public DataProvider {
public:
    explicit SystemInformationDataProvider(platform::PlatformInterface& platform)
        : platform_(platform) {}

    std::string_view id() const noexcept override { return "system"; }

    bool supports(const DataRequest& request) const override {
        return request.provider_id == id();
    }

    bool acquire(const DataRequest&, std::vector<DataRecord>& records,
                 std::string& error) override {
        const auto info = platform_.system().information(error);
        if (!error.empty()) return false;

        records.clear();
        records.push_back({
            {"operating_system", info.operating_system},
            {"architecture", info.architecture},
            {"total_memory_bytes", std::to_string(info.total_memory_bytes)},
            {"available_memory_bytes", std::to_string(info.available_memory_bytes)}
        });
        return true;
    }

private:
    platform::PlatformInterface& platform_;
};

} // namespace

void register_platform_data_providers(
    ProviderRegistry& registry, platform::PlatformInterface& platform) {
    registry.register_provider(std::make_shared<FilesystemDataProvider>(platform));
    registry.register_provider(std::make_shared<DocumentDataProvider>(platform));
    registry.register_provider(std::make_shared<ApplicationDataProvider>(platform));
    registry.register_provider(std::make_shared<ProcessDataProvider>(platform));
    registry.register_provider(std::make_shared<SystemInformationDataProvider>(platform));
}

} // namespace seventh::information
