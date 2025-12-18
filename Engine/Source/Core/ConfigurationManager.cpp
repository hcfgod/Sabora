#include "pch.h"
#include "ConfigurationManager.h"
#include "AsyncIO.h"
#include "Result.h"

namespace fs = std::filesystem;

namespace Sabora
{
    ConfigurationManager::ConfigurationManager(fs::path defaultConfigPath, fs::path userConfigPath) : m_defaultConfigPath(std::move(defaultConfigPath)), m_userConfigPath(std::move(userConfigPath)) {}

    bool ConfigurationManager::Initialize() 
    {
        std::scoped_lock lock(m_mutex);
        bool loaded = false;
        
        // Try to load default configuration file
        if (!m_defaultConfigPath.empty() && AsyncIO::FileExists(m_defaultConfigPath)) 
        {
            auto result = AsyncIO::ReadJsonFile(m_defaultConfigPath);
            if (result.IsSuccess())
            {
                m_defaultConfig = std::move(result).Value();
                loaded = true;
            }
            // Keep empty on failure - missing or invalid config files are not errors
        }

        // Try to load user override configuration file
        if (!m_userConfigPath.empty() && AsyncIO::FileExists(m_userConfigPath)) 
        {
            auto result = AsyncIO::ReadJsonFile(m_userConfigPath);
            if (result.IsSuccess())
            {
                m_userOverrides = std::move(result).Value();
                loaded = true;
            }
            // Keep empty on failure - missing or invalid config files are not errors
        }

        return loaded;
    }

    nlohmann::json ConfigurationManager::Get() const 
    {
        std::scoped_lock lock(m_mutex);
        return MergeJson(m_defaultConfig, m_userOverrides);
    }

    nlohmann::json ConfigurationManager::GetDefaults() const
    {
        std::scoped_lock lock(m_mutex);
        return m_defaultConfig;
    }

    nlohmann::json ConfigurationManager::GetUserOverrides() const 
    {
        std::scoped_lock lock(m_mutex);
        return m_userOverrides;
    }

    void ConfigurationManager::Set(const nlohmann::json& fullConfig)
    {
        std::scoped_lock lock(m_mutex);
        // When setting full config directly, treat it as defaults with no overrides
        m_defaultConfig = fullConfig;
        m_userOverrides = nlohmann::json::object();
    }

    void ConfigurationManager::SetValue(const std::string& jsonPointer, const nlohmann::json& value) 
    {
        std::scoped_lock lock(m_mutex);
        
        // Validate JSON pointer format
        if (jsonPointer.empty() || jsonPointer[0] != '/')
        {
            // Invalid JSON pointer format - silently ignore or could throw/log
            return;
        }
        
        try
        {
        nlohmann::json::json_pointer ptr(jsonPointer);
        m_userOverrides[ptr] = value;
        }
        catch (const nlohmann::json::exception&)
        {
            // Invalid JSON pointer - silently ignore
            // In production, you might want to log this
        }
    }

    void ConfigurationManager::EraseValue(const std::string& jsonPointer)
    {
        std::scoped_lock lock(m_mutex);
        
        // Validate JSON pointer format
        if (jsonPointer.empty() || jsonPointer[0] != '/')
        {
            // Invalid JSON pointer format - silently ignore
            return;
        }
        
        nlohmann::json::json_pointer ptr;
        try
        {
            ptr = nlohmann::json::json_pointer(jsonPointer);
        }
        catch (const nlohmann::json::exception&)
        {
            // Invalid JSON pointer - silently ignore
            return;
        }

        // Lambda function to erase a value from a JSON object/array at a given pointer path.
        // This handles the complexity of erasing from nested JSON structures, supporting both
        // object keys and array indices. The function returns true if the value was found and erased.
        auto eraseFrom = [](nlohmann::json& target, const nlohmann::json::json_pointer& p) -> bool
        {
            // First check if the pointer path exists in the target JSON
            if (!target.contains(p)) 
            {
                return false;
            }

            // Special case: empty pointer means erase the entire document
            if (p.empty()) 
            {
                target = nullptr;
                return true;
            }

            // Get the parent container and the last segment of the path
            // For example, if pointer is "/config/window/width", parent is "/config/window"
            // and last is "width"
            const auto parentPtr = p.parent_pointer();
            auto& parent = target.at(parentPtr);
            const std::string last = p.back();
            
            // Handle object case: erase the key from the parent object
            if (parent.is_object()) 
            {
                // erase() returns the number of elements removed (0 or 1 for objects)
                return parent.erase(last) > 0;
            }

            // Handle array case: erase the element at the specified index
            if (parent.is_array()) 
            {
                try
                {
                    // Convert the last segment to an array index
                    // JSON pointer uses string representation of indices (e.g., "0", "1", "2")
                    const unsigned long long idxValue = std::stoull(last);
                    
                    // Bounds checking: ensure index doesn't overflow size_t
                    if (idxValue > std::numeric_limits<std::size_t>::max())
                    {
                        return false;
                    }
                    
                    const std::size_t idx = static_cast<std::size_t>(idxValue);
                    
                    // Bounds checking: ensure index is within array bounds
                    if (idx < parent.size())
                    {
                        // Erase the element at the specified index
                        parent.erase(parent.begin() + static_cast<std::ptrdiff_t>(idx));
                        return true;
                    }
                } 
                catch (const std::out_of_range&)
                {
                    // Index string is out of range for conversion
                    return false;
                }
                catch (const std::invalid_argument&)
                {
                    // Index string is not a valid number
                    return false;
                } 
                catch (...)
                {
                    // If conversion fails for any other reason, return false
                    return false;
                }
            }
            
            // If parent is neither object nor array, we can't erase from it
            return false;
        };

        // Priority: try to erase from user overrides first
        // If the value exists in user overrides, removing it will allow the default
        // value to show through in the merged view
        if (eraseFrom(m_userOverrides, ptr)) 
        {
            return;
        }

        // If not found in user overrides, try to erase from default config
        // This allows removing default values entirely (they won't appear in merged view)
        (void)eraseFrom(m_defaultConfig, ptr);
    }

    Result<void> ConfigurationManager::SaveDefaults(bool pretty) 
    {
        std::scoped_lock lock(m_mutex);

        if (m_defaultConfigPath.empty()) 
        {
            return Result<void>::Failure(
                ErrorCode::CoreInvalidState,
                "Default config path is not set"
            );
        }

        return AsyncIO::WriteJsonFile(m_defaultConfigPath, m_defaultConfig, pretty, /*createDirs*/ true);
    }

    Result<void> ConfigurationManager::SaveUserOverrides(bool pretty) 
    {
        std::scoped_lock lock(m_mutex);

        if (m_userConfigPath.empty())
        {
            return Result<void>::Failure(
                ErrorCode::CoreInvalidState,
                "User config path is not set"
            );
        }

        return AsyncIO::WriteJsonFile(m_userConfigPath, m_userOverrides, pretty, /*createDirs*/ true);
    }

    void ConfigurationManager::SetDefaultConfigPath(fs::path path) 
    {
        std::scoped_lock lock(m_mutex);
        m_defaultConfigPath = std::move(path);
    }

    void ConfigurationManager::SetUserConfigPath(fs::path path)
    {
        std::scoped_lock lock(m_mutex);
        m_userConfigPath = std::move(path);
    }

    fs::path ConfigurationManager::GetDefaultConfigPath() const
    {
        std::scoped_lock lock(m_mutex);
        return m_defaultConfigPath;
    }

    fs::path ConfigurationManager::GetUserConfigPath() const 
    {
        std::scoped_lock lock(m_mutex);
        return m_userConfigPath;
    }

    nlohmann::json ConfigurationManager::MergeJson(const nlohmann::json& baseConfig, const nlohmann::json& overrides) 
    {
        // If base config is not an object, return overrides (or empty object if overrides is null)
        if (!baseConfig.is_object()) 
        {
            return overrides.is_null() ? nlohmann::json::object() : overrides;
        }

        // If overrides is empty or not an object, return base config directly (avoid copy)
        if (!overrides.is_object() || overrides.empty())
        {
            return overrides.is_null() ? baseConfig : overrides;
        }

        // Optimization: Use move semantics when possible
        // Start with base config, but try to avoid full copy if overrides is small
        nlohmann::json result = baseConfig;

        // Deep merge: iterate through all override keys and merge them into the result
        // This modifies result in-place to avoid additional copies
        for (auto it = overrides.begin(); it != overrides.end(); ++it)
        {
            const std::string& key = it.key();
            const nlohmann::json& val = it.value();

            // Recursive merge: if both base and override values are objects,
            // recursively merge them instead of replacing the entire object
            // This allows partial overrides of nested configurations
            if (result.contains(key) && result[key].is_object() && val.is_object()) 
            {
                // Recursive merge - this creates a new merged object
                result[key] = MergeJson(result[key], val);
            } 
            else 
            {
                // Simple override: replace the base value with the override value
                // This handles both new keys and replacement of existing non-object values
                // Use move if val is a temporary, otherwise copy
                result[key] = val;
            }
        }

        return result;
    }

} // namespace Sabora