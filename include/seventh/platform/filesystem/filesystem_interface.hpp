#pragma once

#include "seventh/platform/platform_types.hpp"

#include <optional>
#include <string>
#include <vector>

namespace seventh::permissions { struct AuthorizedRoot; }

namespace seventh::platform {

class FilesystemInterface {
public:
    virtual ~FilesystemInterface() = default;
    [[nodiscard]] virtual bool exists(const std::string& path) const = 0;
    [[nodiscard]] virtual std::vector<FileEntry> list(
        const std::string& directory, std::string& error) const = 0;
    [[nodiscard]] virtual std::optional<std::string> read_text(
        const std::string& path, std::string& error) const = 0;
    [[nodiscard]] virtual PlatformResult create_directory(
        const std::string& path) = 0;
    [[nodiscard]] virtual PlatformResult rename(
        const std::string& source, const std::string& destination) = 0;
    [[nodiscard]] virtual PlatformResult copy(
        const std::string& source, const std::string& destination) = 0;
    [[nodiscard]] virtual PlatformResult remove(
        const std::string& path) = 0;

    [[nodiscard]] virtual PlatformResult create_directory_authorized(
        const std::string&, const permissions::AuthorizedRoot&) {
        return {false, "This platform does not provide descriptor-based directory creation."};
    }
    [[nodiscard]] virtual PlatformResult rename_authorized(
        const std::string&, const std::string&,
        const permissions::AuthorizedRoot&,
        const permissions::AuthorizedRoot&) {
        return {false, "This platform does not provide descriptor-based rename."};
    }
    [[nodiscard]] virtual PlatformResult copy_authorized(
        const std::string&, const std::string&,
        const permissions::AuthorizedRoot&,
        const permissions::AuthorizedRoot&) {
        return {false, "This platform does not provide descriptor-based copy."};
    }
    [[nodiscard]] virtual PlatformResult remove_authorized(
        const std::string&, const permissions::AuthorizedRoot&) {
        return {false, "This platform does not provide descriptor-based removal."};
    }
    [[nodiscard]] virtual std::optional<std::string> read_text_authorized(
        const std::string&, const permissions::AuthorizedRoot&, std::string& error) const {
        error = "This platform does not provide descriptor-based text reading.";
        return std::nullopt;
    }
};

} // namespace seventh::platform
