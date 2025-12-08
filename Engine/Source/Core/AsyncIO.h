#pragma once

#include <cstdint>
#include <filesystem>
#include <future>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "Result.h"

namespace Sabora
{
    /**
     * @brief Utility class for file I/O operations with both synchronous and asynchronous support.
     * 
     * AsyncIO provides a comprehensive set of file operations including:
     *   - Text and binary file reading/writing
     *   - JSON file parsing and serialization
     *   - Filesystem operations (existence checks, directory creation, file listing)
     *   - Asynchronous wrappers for all I/O operations
     * 
     * All operations are thread-safe and use Result<T> for explicit error handling.
     * Asynchronous operations return std::future<Result<T>> objects for non-blocking execution.
     * 
     * @note This is a utility class with only static methods - no instantiation required.
     */
    class AsyncIO final 
    {
    public:
        //==========================================================================
        // Constants and Limits
        //==========================================================================

        // Maximum file size limits (in bytes) to prevent memory exhaustion
        static constexpr std::size_t MaxTextFileSize = 100 * 1024 * 1024;      // 100 MB
        static constexpr std::size_t MaxBinaryFileSize = 500 * 1024 * 1024;   // 500 MB
        static constexpr std::size_t MaxJsonFileSize = 50 * 1024 * 1024;      // 50 MB
        static constexpr std::size_t MaxPathLength = 4096;                     // 4 KB path limit

        //==========================================================================
        // Path Validation and Sanitization
        //==========================================================================

        /**
         * @brief Validate and sanitize a file path for security.
         * @param path The path to validate.
         * @param allowAbsolute If true, allow absolute paths (default: false).
         * @return Result containing the sanitized path, or an error if invalid.
         * 
         * This function:
         *   - Checks for path traversal attacks (../)
         *   - Validates path length
         *   - Optionally restricts to relative paths
         *   - Normalizes the path
         */
        static Result<std::filesystem::path> ValidateAndSanitizePath(
            const std::filesystem::path& path,
            bool allowAbsolute = false
        );

        //==========================================================================
        // Filesystem Helper Methods
        //==========================================================================

        /**
         * @brief Check if a file exists at the given path.
         * @param path The file path to check.
         * @return True if the file exists and is a regular file, false otherwise.
         * 
         * @note This method never throws - errors are silently ignored.
         */
        static bool FileExists(const std::filesystem::path& path) noexcept;

        /**
         * @brief Remove a file from the filesystem.
         * @param path The file path to remove.
         * @return True if the file was removed or didn't exist, false on error.
         * 
         * @note This method never throws - errors are silently ignored.
         */
        static bool RemoveFile(const std::filesystem::path& path) noexcept;

        /**
         * @brief Create all parent directories for a given file path.
         * @param path The file path whose parent directories should be created.
         * @return True if directories were created or already exist, false on error.
         * 
         * @note This method never throws - errors are silently ignored.
         * @note If the path has no filename component, the path itself is treated as a directory.
         */
        static bool CreateDirectoriesFor(const std::filesystem::path& path) noexcept;

        /**
         * @brief List all files in a directory.
         * @param directory The directory to list files from.
         * @param recursive If true, recursively search subdirectories.
         * @return Vector of file paths found in the directory.
         * 
         * @note Returns an empty vector if the directory doesn't exist or isn't a directory.
         * @note Only regular files are included (directories and special files are excluded).
         */
        static std::vector<std::filesystem::path> ListFiles(const std::filesystem::path& directory,
                                                            bool recursive);

        //==========================================================================
        // Synchronous File I/O Methods
        //==========================================================================

        /**
         * @brief Read a text file into a string.
         * @param path The file path to read from.
         * @return Result containing the file contents as a string, or an error.
         * 
         * @note The file is read in binary mode to preserve line endings.
         */
        static Result<std::string> ReadTextFile(const std::filesystem::path& path);

        /**
         * @brief Read a binary file into a byte vector.
         * @param path The file path to read from.
         * @return Result containing the file contents as a vector of bytes, or an error.
         */
        static Result<std::vector<std::uint8_t>> ReadBinaryFile(const std::filesystem::path& path);

        /**
         * @brief Write a string to a text file.
         * @param path The file path to write to.
         * @param contents The string contents to write.
         * @param createDirectories If true, create parent directories if they don't exist (default: true).
         * @return Result<void> indicating success or failure.
         * 
         * @note The file is written in binary mode and truncates existing files.
         */
        static Result<void> WriteTextFile(const std::filesystem::path& path,
                                          std::string_view contents,
                                          bool createDirectories = true);

        /**
         * @brief Write binary data to a file.
         * @param path The file path to write to.
         * @param data The binary data to write.
         * @param createDirectories If true, create parent directories if they don't exist (default: true).
         * @return Result<void> indicating success or failure.
         * 
         * @note The file is written in binary mode and truncates existing files.
         */
        static Result<void> WriteBinaryFile(const std::filesystem::path& path,
                                            const std::vector<std::uint8_t>& data,
                                            bool createDirectories = true);

        //==========================================================================
        // JSON File I/O Methods
        //==========================================================================

        /**
         * @brief Read and parse a JSON file.
         * @param path The file path to read from.
         * @param validateStructure If true, validate that JSON is an object (default: false).
         * @return Result containing the parsed JSON object, or an error.
         * 
         * @note Returns FileReadError if the file cannot be opened or read.
         * @note Returns FileInvalidFormat if the JSON is invalid.
         * @note Returns ValidationFailed if validateStructure is true and JSON is not an object.
         */
        static Result<nlohmann::json> ReadJsonFile(
            const std::filesystem::path& path,
            bool validateStructure = false
        );

        /**
         * @brief Write a JSON object to a file.
         * @param path The file path to write to.
         * @param json The JSON object to write.
         * @param pretty If true, format JSON with indentation (default: true).
         * @param createDirectories If true, create parent directories if they don't exist (default: true).
         * @return Result<void> indicating success or failure.
         */
        static Result<void> WriteJsonFile(const std::filesystem::path& path,
                                          const nlohmann::json& json,
                                          bool pretty = true,
                                          bool createDirectories = true);

        //==========================================================================
        // Asynchronous File I/O Methods
        //==========================================================================

        /**
         * @brief Asynchronously read a text file.
         * @param path The file path to read from (moved into the async task).
         * @return Future containing a Result with the file contents or an error.
         * 
         * @note The path is copied/moved into the async task, so it's safe to pass temporary paths.
         */
        static std::future<Result<std::string>> ReadTextFileAsync(std::filesystem::path path);

        /**
         * @brief Asynchronously read a binary file.
         * @param path The file path to read from (moved into the async task).
         * @return Future containing a Result with the file contents or an error.
         */
        static std::future<Result<std::vector<std::uint8_t>>> ReadBinaryFileAsync(std::filesystem::path path);

        /**
         * @brief Asynchronously write a text file.
         * @param path The file path to write to (moved into the async task).
         * @param contents The string contents to write (moved into the async task).
         * @param createDirectories If true, create parent directories if they don't exist (default: true).
         * @return Future containing a Result<void> that completes when the write operation finishes.
         */
        static std::future<Result<void>> WriteTextFileAsync(std::filesystem::path path,
                                                             std::string contents,
                                                             bool createDirectories = true);

        /**
         * @brief Asynchronously write a binary file.
         * @param path The file path to write to (moved into the async task).
         * @param data The binary data to write (moved into the async task).
         * @param createDirectories If true, create parent directories if they don't exist (default: true).
         * @return Future containing a Result<void> that completes when the write operation finishes.
         */
        static std::future<Result<void>> WriteBinaryFileAsync(std::filesystem::path path,
                                                              std::vector<std::uint8_t> data,
                                                              bool createDirectories = true);

        /**
         * @brief Asynchronously read and parse a JSON file.
         * @param path The file path to read from (moved into the async task).
         * @return Future containing a Result with the parsed JSON object or an error.
         */
        static std::future<Result<nlohmann::json>> ReadJsonFileAsync(std::filesystem::path path);

        /**
         * @brief Asynchronously write a JSON object to a file.
         * @param path The file path to write to (moved into the async task).
         * @param json The JSON object to write (moved into the async task).
         * @param pretty If true, format JSON with indentation (default: true).
         * @param createDirectories If true, create parent directories if they don't exist (default: true).
         * @return Future containing a Result<void> that completes when the write operation finishes.
         */
        static std::future<Result<void>> WriteJsonFileAsync(std::filesystem::path path,
                                                             nlohmann::json json,
                                                             bool pretty = true,
                                                             bool createDirectories = true);
    };

} // namespace Sabora