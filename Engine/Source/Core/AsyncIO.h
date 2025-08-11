#pragma once

#include <cstdint>
#include <filesystem>
#include <future>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace Sabora::Engine {

class AsyncIO final {
public:
    // Filesystem helpers
    static bool fileExists(const std::filesystem::path& path) noexcept;
    static bool removeFile(const std::filesystem::path& path) noexcept;
    static bool createDirectoriesFor(const std::filesystem::path& path) noexcept;
    static std::vector<std::filesystem::path> listFiles(const std::filesystem::path& directory,
                                                        bool recursive);

    // Synchronous file I/O
    static std::string readTextFile(const std::filesystem::path& path);
    static std::vector<std::uint8_t> readBinaryFile(const std::filesystem::path& path);
    static void writeTextFile(const std::filesystem::path& path,
                              std::string_view contents,
                              bool createDirectories = true);
    static void writeBinaryFile(const std::filesystem::path& path,
                                const std::vector<std::uint8_t>& data,
                                bool createDirectories = true);

    // JSON I/O
    static nlohmann::json readJsonFile(const std::filesystem::path& path);
    static void writeJsonFile(const std::filesystem::path& path,
                              const nlohmann::json& json,
                              bool pretty = true,
                              bool createDirectories = true);

    // Asynchronous wrappers (values are copied/moved into tasks)
    static std::future<std::string> readTextFileAsync(std::filesystem::path path);
    static std::future<std::vector<std::uint8_t>> readBinaryFileAsync(std::filesystem::path path);
    static std::future<void> writeTextFileAsync(std::filesystem::path path,
                                                std::string contents,
                                                bool createDirectories = true);
    static std::future<void> writeBinaryFileAsync(std::filesystem::path path,
                                                  std::vector<std::uint8_t> data,
                                                  bool createDirectories = true);
    static std::future<nlohmann::json> readJsonFileAsync(std::filesystem::path path);
    static std::future<void> writeJsonFileAsync(std::filesystem::path path,
                                                nlohmann::json json,
                                                bool pretty = true,
                                                bool createDirectories = true);
};

} // namespace Sabora::Engine