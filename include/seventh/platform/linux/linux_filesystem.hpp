#pragma once

#include "seventh/platform/filesystem/filesystem_interface.hpp"

namespace seventh::platform::linux_backend {

class LinuxFilesystem final : public FilesystemInterface {
public:
    [[nodiscard]] bool exists(const std::string& path) const override;
    [[nodiscard]] std::vector<FileEntry> list(
        const std::string& directory, std::string& error) const override;
    [[nodiscard]] std::optional<std::string> read_text(
        const std::string& path, std::string& error) const override;
    [[nodiscard]] PlatformResult create_directory(const std::string& path) override;
    [[nodiscard]] PlatformResult rename(
        const std::string& source, const std::string& destination) override;
    [[nodiscard]] PlatformResult copy(
        const std::string& source, const std::string& destination) override;
    [[nodiscard]] PlatformResult remove(const std::string& path) override;
};

} // namespace seventh::platform::linux_backend
