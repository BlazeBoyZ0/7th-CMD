#include "seventh/platform/linux/linux_system.hpp"

#include <fstream>
#include <sys/utsname.h>

namespace seventh::platform::linux {

SystemInformation LinuxSystem::information(std::string& error) const {
    SystemInformation info;
    struct utsname system_name {};
    if (::uname(&system_name) == 0) {
        info.operating_system = system_name.sysname;
        info.architecture = system_name.machine;
    } else {
        error = "Unable to read kernel information.";
    }

    std::ifstream memory("/proc/meminfo");
    if (!memory) {
        if (!error.empty()) error += " ";
        error += "Unable to read /proc/meminfo.";
        return info;
    }

    std::string key;
    std::uint64_t value = 0;
    std::string unit;
    while (memory >> key >> value >> unit) {
        if (key == "MemTotal:") info.total_memory_bytes = value * 1024;
        else if (key == "MemAvailable:") info.available_memory_bytes = value * 1024;
        if (info.total_memory_bytes && info.available_memory_bytes) break;
    }
    return info;
}

} // namespace seventh::platform::linux
