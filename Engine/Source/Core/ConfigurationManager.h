#pragma once

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace Sabora::Engine {

// Thread-safe configuration loader/saver using JSON files.
// Supports layered configs (default + user override) and in-memory updates.
class ConfigurationManager {
public:
    explicit ConfigurationManager(std::filesystem::path defaultConfigPath = {},
                                  std::filesystem::path userConfigPath = {});

    // Initialize by reading existing files if present.
    // Returns true when any config file was successfully loaded.
    bool initialize();

    // Accessors
    nlohmann::json get() const;                // merged view (user overrides default)
    nlohmann::json getDefaults() const;        // default config
    nlohmann::json getUserOverrides() const;   // user override config

    // Mutation
    void set(const nlohmann::json& fullConfig);
    void setValue(const std::string& jsonPointer, const nlohmann::json& value);
    void eraseValue(const std::string& jsonPointer);

    // Persistence
    void saveDefaults(bool pretty = true);
    void saveUserOverrides(bool pretty = true);

    // Paths
    void setDefaultConfigPath(std::filesystem::path path);
    void setUserConfigPath(std::filesystem::path path);
    std::filesystem::path getDefaultConfigPath() const;
    std::filesystem::path getUserConfigPath() const;

private:
    mutable std::mutex m_mutex;
    std::filesystem::path m_defaultConfigPath;
    std::filesystem::path m_userConfigPath;
    nlohmann::json m_defaultConfig;   // may be empty
    nlohmann::json m_userOverrides;   // may be empty

    static nlohmann::json mergeJson(const nlohmann::json& baseConfig,
                                    const nlohmann::json& overrides);
};

} // namespace Sabora::Engine


