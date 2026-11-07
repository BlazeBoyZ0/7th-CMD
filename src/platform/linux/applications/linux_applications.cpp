#include "seventh/platform/linux/linux_applications.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <spawn.h>
#include <string>
#include <utility>
#include <vector>

extern char** environ;

namespace seventh::platform::linux_backend {
namespace fs = std::filesystem;
namespace {

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r");
    return value.substr(first, last - first + 1);
}

void scan_directory(const fs::path& directory,
                    std::vector<ApplicationEntry>& apps) {
    std::error_code ec;
    if (!fs::is_directory(directory, ec) || ec) return;
    for (const auto& entry : fs::directory_iterator(directory, ec)) {
        if (ec) break;
        if (!entry.is_regular_file(ec) || entry.path().extension() != ".desktop") continue;

        std::ifstream input(entry.path());
        if (!input) continue;
        ApplicationEntry app;
        app.id = entry.path().stem().string();
        app.desktop_file = entry.path().string();
        bool hidden = false;
        std::string line;
        while (std::getline(input, line)) {
            if (line.starts_with("Name=") && app.name.empty()) app.name = line.substr(5);
            else if (line.starts_with("Exec=") && app.executable.empty()) {
                app.executable = trim(line.substr(5));
            } else if (line == "NoDisplay=true" || line == "Hidden=true") {
                hidden = true;
            }
        }
        if (!hidden && !app.name.empty() && !app.executable.empty()) {
            apps.push_back(std::move(app));
        }
    }
}

} // namespace

std::vector<ApplicationEntry> LinuxApplications::list(std::string& error) const {
    std::vector<ApplicationEntry> apps;
    scan_directory("/usr/share/applications", apps);
    scan_directory("/usr/local/share/applications", apps);
    if (const char* home = std::getenv("HOME")) {
        scan_directory(fs::path(home) / ".local/share/applications", apps);
    }
    if (apps.empty()) error = "No readable desktop application entries were found.";
    std::sort(apps.begin(), apps.end(),
        [](const auto& left, const auto& right) { return left.name < right.name; });
    return apps;
}

std::optional<ApplicationEntry> LinuxApplications::find(
    const std::string& name) const {
    std::string error;
    const auto apps = list(error);
    const auto wanted = lower(name);
    for (const auto& app : apps) {
        if (lower(app.name) == wanted || lower(app.id) == wanted) return app;
    }
    return std::nullopt;
}

PlatformResult LinuxApplications::launch(const ApplicationEntry& application) {
    if (application.id.empty()) return {false, "A desktop application ID is required."};
    pid_t child = 0;
    char* const arguments[] = {
        const_cast<char*>("gtk-launch"),
        const_cast<char*>(application.id.c_str()),
        nullptr
    };
    const int status = posix_spawnp(&child, "gtk-launch", nullptr, nullptr,
                                    arguments, environ);
    if (status != 0) {
        return {false, "Unable to start gtk-launch (error " +
                       std::to_string(status) + ")."};
    }
    return {true, {}};
}

} // namespace seventh::platform::linux_backend
