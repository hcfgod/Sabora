#pragma once

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#include "Result.h"

namespace Sabora
{
    /**
     * @brief Thread-safe configuration manager using JSON files with layered configuration support.
     * 
     * The ConfigurationManager provides a robust system for managing application configuration
     * with support for default configurations and user-specific overrides. It uses a layered
     * approach where user overrides are merged on top of default values, allowing partial
     * configuration updates without losing default settings.
     * 
     * Features:
     *   - Thread-safe operations using mutex locks
     *   - Layered configuration (default + user overrides)
     *   - Deep merging of nested JSON objects
     *   - JSON pointer-based value access and modification
     *   - Automatic directory creation for config files
     * 
     * @example
     * @code
     *   ConfigurationManager config("defaults.json", "user.json");
     *   config.Initialize();
     *   
     *   // Get merged configuration (user overrides default)
     *   auto merged = config.Get();
     *   
     *   // Set a specific value using JSON pointer
     *   config.SetValue("/window/width", 1920);
     *   
     *   // Save user overrides
     *   config.SaveUserOverrides();
     * @endcode
     */
    class ConfigurationManager 
    {
    public:
        /**
         * @brief Construct a ConfigurationManager with optional config file paths.
         * @param defaultConfigPath Path to the default configuration file (read-only, typically in app directory).
         * @param userConfigPath Path to the user configuration file (read-write, typically in user data directory).
         * 
         * @note Paths can be set later using SetDefaultConfigPath() and SetUserConfigPath().
         */
        explicit ConfigurationManager(std::filesystem::path defaultConfigPath = {},
                                     std::filesystem::path userConfigPath = {});

        /**
         * @brief Initialize by reading existing configuration files if present.
         * @return True if any configuration file was successfully loaded, false otherwise.
         * 
         * This method attempts to load both default and user configuration files.
         * Missing files are not considered errors - they will simply result in empty
         * configurations that can be populated later.
         */
        bool Initialize();

        /**
         * @brief Get the merged configuration view (user overrides applied to defaults).
         * @return JSON object containing the merged configuration.
         * 
         * The merged view combines default and user configurations, with user values
         * taking precedence. Nested objects are deep-merged, allowing partial overrides.
         */
        nlohmann::json Get() const;

        /**
         * @brief Get the default configuration (without user overrides).
         * @return JSON object containing only the default configuration.
         */
        nlohmann::json GetDefaults() const;

        /**
         * @brief Get the user override configuration (without defaults).
         * @return JSON object containing only user-specific overrides.
         */
        nlohmann::json GetUserOverrides() const;

        /**
         * @brief Replace the entire default configuration.
         * @param fullConfig Complete JSON configuration to set as defaults.
         * 
         * @note This clears all user overrides. Use SetValue() for partial updates.
         */
        void Set(const nlohmann::json& fullConfig);

        /**
         * @brief Set a specific value in the configuration using a JSON pointer.
         * @param jsonPointer JSON pointer path (e.g., "/window/width" or "/settings/audio/volume").
         * @param value The value to set at the specified path.
         * 
         * @note Values set via this method are stored as user overrides and will
         *       override default values in the merged view.
         * 
         * @example
         * @code
         *   config.SetValue("/window/width", 1920);
         *   config.SetValue("/settings/audio/enabled", true);
         * @endcode
         */
        void SetValue(const std::string& jsonPointer, const nlohmann::json& value);

        /**
         * @brief Erase a value from the configuration using a JSON pointer.
         * @param jsonPointer JSON pointer path to the value to erase.
         * 
         * This method attempts to remove a value from user overrides first. If not found
         * in overrides, it attempts to remove from defaults. Erasing a value from defaults
         * means it will not appear in the merged view.
         * 
         * @note Supports erasing from both objects (by key) and arrays (by index).
         * 
         * @example
         * @code
         *   config.EraseValue("/window/fullscreen");  // Remove a key
         *   config.EraseValue("/items/0");            // Remove first array element
         * @endcode
         */
        void EraseValue(const std::string& jsonPointer);

        /**
         * @brief Save the default configuration to its file.
         * @param pretty If true, format JSON with indentation (default: true).
         * @return Result<void> indicating success or failure.
         * 
         * Creates parent directories automatically if they don't exist.
         * Returns an error if the default config path is not set or if the file cannot be written.
         */
        Result<void> SaveDefaults(bool pretty = true);

        /**
         * @brief Save the user override configuration to its file.
         * @param pretty If true, format JSON with indentation (default: true).
         * @return Result<void> indicating success or failure.
         * 
         * Creates parent directories automatically if they don't exist.
         * Returns an error if the user config path is not set or if the file cannot be written.
         */
        Result<void> SaveUserOverrides(bool pretty = true);

        /**
         * @brief Set the path for the default configuration file.
         * @param path File path for default configuration.
         */
        void SetDefaultConfigPath(std::filesystem::path path);

        /**
         * @brief Set the path for the user configuration file.
         * @param path File path for user configuration.
         */
        void SetUserConfigPath(std::filesystem::path path);

        /**
         * @brief Get the current default configuration file path.
         * @return The default configuration file path.
         */
        std::filesystem::path GetDefaultConfigPath() const;

        /**
         * @brief Get the current user configuration file path.
         * @return The user configuration file path.
         */
        std::filesystem::path GetUserConfigPath() const;

    private:
        mutable std::mutex m_mutex;
        std::filesystem::path m_defaultConfigPath;
        std::filesystem::path m_userConfigPath;
        nlohmann::json m_defaultConfig;   // may be empty
        nlohmann::json m_userOverrides;   // may be empty

        static nlohmann::json MergeJson(const nlohmann::json& baseConfig,
                                        const nlohmann::json& overrides);
    public:
        // Expose MergeJson for testing purposes only
        static nlohmann::json MergeJsonPublic(const nlohmann::json& baseConfig,
                                              const nlohmann::json& overrides)
        {
            return MergeJson(baseConfig, overrides);
        }
    };

} // namespace Sabora