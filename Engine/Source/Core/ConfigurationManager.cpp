#include "ConfigurationManager.h"
#include "AsyncIO.h"

#include <stdexcept>

namespace fs = std::filesystem;

namespace Sabora::Engine {

ConfigurationManager::ConfigurationManager(fs::path defaultConfigPath,
                                           fs::path userConfigPath)
    : m_defaultConfigPath(std::move(defaultConfigPath)),
      m_userConfigPath(std::move(userConfigPath)) {}

bool ConfigurationManager::initialize() {
    std::scoped_lock lock(m_mutex);
    bool loaded = false;
    if (!m_defaultConfigPath.empty() && AsyncIO::fileExists(m_defaultConfigPath)) {
        try {
            m_defaultConfig = AsyncIO::readJsonFile(m_defaultConfigPath);
            loaded = true;
        } catch (...) {
            // Keep empty on failure
        }
    }
    if (!m_userConfigPath.empty() && AsyncIO::fileExists(m_userConfigPath)) {
        try {
            m_userOverrides = AsyncIO::readJsonFile(m_userConfigPath);
            loaded = true;
        } catch (...) {
            // Keep empty on failure
        }
    }
    return loaded;
}

nlohmann::json ConfigurationManager::get() const {
    std::scoped_lock lock(m_mutex);
    return mergeJson(m_defaultConfig, m_userOverrides);
}

nlohmann::json ConfigurationManager::getDefaults() const {
    std::scoped_lock lock(m_mutex);
    return m_defaultConfig;
}

nlohmann::json ConfigurationManager::getUserOverrides() const {
    std::scoped_lock lock(m_mutex);
    return m_userOverrides;
}

void ConfigurationManager::set(const nlohmann::json& fullConfig) {
    std::scoped_lock lock(m_mutex);
    // When setting full config directly, treat it as defaults with no overrides
    m_defaultConfig = fullConfig;
    m_userOverrides = nlohmann::json::object();
}

void ConfigurationManager::setValue(const std::string& jsonPointer, const nlohmann::json& value) {
    std::scoped_lock lock(m_mutex);
    nlohmann::json::json_pointer ptr(jsonPointer);
    m_userOverrides[ptr] = value;
}

void ConfigurationManager::eraseValue(const std::string& jsonPointer) {
    std::scoped_lock lock(m_mutex);
    nlohmann::json::json_pointer ptr(jsonPointer);

    auto eraseFrom = [](nlohmann::json& target, const nlohmann::json::json_pointer& p) -> bool {
        if (!target.contains(p)) {
            return false;
        }
        if (p.empty()) {
            // Reset entire document
            target = nullptr;
            return true;
        }
        const auto parentPtr = p.parent_pointer();
        auto& parent = target.at(parentPtr);
        const std::string last = p.back();
        if (parent.is_object()) {
            return parent.erase(last) > 0;
        }
        if (parent.is_array()) {
            try {
                const std::size_t idx = static_cast<std::size_t>(std::stoull(last));
                if (idx < parent.size()) {
                    parent.erase(idx);
                    return true;
                }
            } catch (...) {
                return false;
            }
        }
        return false;
    };

    if (eraseFrom(m_userOverrides, ptr)) {
        return;
    }
    (void)eraseFrom(m_defaultConfig, ptr);
}

void ConfigurationManager::saveDefaults(bool pretty) {
    std::scoped_lock lock(m_mutex);
    if (m_defaultConfigPath.empty()) {
        throw std::runtime_error("Default config path is not set");
    }
    AsyncIO::writeJsonFile(m_defaultConfigPath, m_defaultConfig, pretty, /*createDirs*/ true);
}

void ConfigurationManager::saveUserOverrides(bool pretty) {
    std::scoped_lock lock(m_mutex);
    if (m_userConfigPath.empty()) {
        throw std::runtime_error("User config path is not set");
    }
    AsyncIO::writeJsonFile(m_userConfigPath, m_userOverrides, pretty, /*createDirs*/ true);
}

void ConfigurationManager::setDefaultConfigPath(fs::path path) {
    std::scoped_lock lock(m_mutex);
    m_defaultConfigPath = std::move(path);
}

void ConfigurationManager::setUserConfigPath(fs::path path) {
    std::scoped_lock lock(m_mutex);
    m_userConfigPath = std::move(path);
}

fs::path ConfigurationManager::getDefaultConfigPath() const {
    std::scoped_lock lock(m_mutex);
    return m_defaultConfigPath;
}

fs::path ConfigurationManager::getUserConfigPath() const {
    std::scoped_lock lock(m_mutex);
    return m_userConfigPath;
}

nlohmann::json ConfigurationManager::mergeJson(const nlohmann::json& baseConfig,
                                               const nlohmann::json& overrides) {
    if (!baseConfig.is_object()) {
        return overrides.is_null() ? nlohmann::json::object() : overrides;
    }

    nlohmann::json result = baseConfig;
    if (!overrides.is_object()) {
        return overrides.is_null() ? result : overrides;
    }

    for (auto it = overrides.begin(); it != overrides.end(); ++it) {
        const std::string& key = it.key();
        const nlohmann::json& val = it.value();
        if (result.contains(key) && result[key].is_object() && val.is_object()) {
            result[key] = mergeJson(result[key], val);
        } else {
            result[key] = val;
        }
    }
    return result;
}

} // namespace Sabora::Engine