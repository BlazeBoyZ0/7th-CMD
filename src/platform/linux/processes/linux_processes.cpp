#include "seventh/platform/linux/linux_processes.hpp"

#include <csignal>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace seventh::platform::linux {
namespace fs = std::filesystem;

std::vector<ProcessEntry> LinuxProcesses::list(std::string& error) const {
    std::vector<ProcessEntry> processes;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator("/proc", ec)) {
        if (ec) break;
        const auto name = entry.path().filename().string();
        if (name.empty() || name.find_first_not_of("0123456789") != std::string::npos) continue;

        ProcessEntry process;
        try {
            process.id = std::stoull(name);
        } catch (...) {
            continue;
        }

        std::ifstream comm(entry.path() / "comm");
        std::getline(comm, process.name);
        std::error_code link_error;
        const auto executable = fs::read_symlink(entry.path() / "exe", link_error);
        if (!link_error) process.executable = executable.string();
        if (!process.name.empty()) processes.push_back(std::move(process));
    }
    if (ec) error = ec.message();
    return processes;
}

PlatformResult LinuxProcesses::terminate(std::uint64_t process_id) {
    if (process_id <= 1 || process_id == static_cast<std::uint64_t>(::getpid())) {
        return {false, "Refusing to terminate a protected process ID."};
    }
    const auto process_path = fs::path("/proc") / std::to_string(process_id);
    struct stat_info { };
    (void)process_path;
    if (::kill(static_cast<pid_t>(process_id), SIGTERM) != 0) {
        return {false, "Unable to send SIGTERM to the requested process."};
    }
    return {true, {}};
}

} // namespace seventh::platform::linux
