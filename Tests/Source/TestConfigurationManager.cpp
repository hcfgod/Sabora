/**
 * @file TestConfigurationManager.cpp
 * @brief Unit tests for ConfigurationManager functionality.
 */

#include "doctest.h"
#include "Core/ConfigurationManager.h"
#include "Core/AsyncIO.h"

#include <filesystem>
#include <fstream>

using namespace Sabora;
namespace fs = std::filesystem;

TEST_SUITE("ConfigurationManager")
{
    TEST_CASE("Initialize - Loads default config when file exists")
    {
        const fs::path defaultConfig = "test_default.json";
        const std::string defaultContent = R"({"window": {"width": 1920, "height": 1080}})";

        // Create default config file
        {
            std::ofstream out(defaultConfig);
            REQUIRE(out.is_open());
            out << defaultContent;
        }

        ConfigurationManager config(defaultConfig, "");
        bool initialized = config.Initialize();
        CHECK(initialized);

        auto merged = config.Get();
        CHECK(merged["window"]["width"] == 1920);
        CHECK(merged["window"]["height"] == 1080);

        // Cleanup
        AsyncIO::RemoveFile(defaultConfig);
    }

    TEST_CASE("Initialize - Handles missing files gracefully")
    {
        ConfigurationManager config("non_existent_default.json", "non_existent_user.json");
        bool initialized = config.Initialize();
        // Should not fail, just return false
        CHECK_FALSE(initialized);

        auto merged = config.Get();
        CHECK(merged.is_object());
        CHECK(merged.empty());
    }

    TEST_CASE("Get - Merges default and user overrides")
    {
        const fs::path defaultConfig = "test_default_merge.json";
        const fs::path userConfig = "test_user_merge.json";
        
        const std::string defaultContent = R"({
            "window": {"width": 1920, "height": 1080, "fullscreen": false},
            "audio": {"volume": 0.5}
        })";
        
        const std::string userContent = R"({
            "window": {"width": 2560, "fullscreen": true}
        })";

        // Create config files
        {
            std::ofstream out(defaultConfig);
            REQUIRE(out.is_open());
            out << defaultContent;
        }
        {
            std::ofstream out(userConfig);
            REQUIRE(out.is_open());
            out << userContent;
        }

        ConfigurationManager config(defaultConfig, userConfig);
        REQUIRE(config.Initialize());

        auto merged = config.Get();
        // User override should take precedence
        CHECK(merged["window"]["width"] == 2560);
        CHECK(merged["window"]["fullscreen"] == true);
        // Default value should be preserved
        CHECK(merged["window"]["height"] == 1080);
        // Non-overridden section should remain
        CHECK(merged["audio"]["volume"] == 0.5);

        // Cleanup
        AsyncIO::RemoveFile(defaultConfig);
        AsyncIO::RemoveFile(userConfig);
    }

    TEST_CASE("SetValue - Sets value using JSON pointer")
    {
        ConfigurationManager config("", "");
        config.Initialize();

        config.SetValue("/window/width", 2560);
        config.SetValue("/audio/enabled", true);

        auto merged = config.Get();
        CHECK(merged["window"]["width"] == 2560);
        CHECK(merged["audio"]["enabled"] == true);
    }

    TEST_CASE("SetValue - Handles invalid JSON pointer gracefully")
    {
        ConfigurationManager config("", "");
        config.Initialize();

        // Invalid pointer (doesn't start with /)
        config.SetValue("invalid_pointer", 42);
        
        // Should not crash, but value won't be set
        auto merged = config.Get();
        // The invalid pointer won't be in the config
        CHECK(merged.find("invalid_pointer") == merged.end());
    }

    TEST_CASE("EraseValue - Removes value from user overrides")
    {
        ConfigurationManager config("", "");
        config.Initialize();

        // Set a value
        config.SetValue("/test/key", "value");
        auto merged1 = config.Get();
        CHECK(merged1["test"]["key"] == "value");

        // Erase it
        config.EraseValue("/test/key");
        auto merged2 = config.Get();
        CHECK_FALSE(merged2["test"].contains("key"));
    }

    TEST_CASE("SaveUserOverrides - Saves to file")
    {
        const fs::path userConfig = "test_save_user.json";
        ConfigurationManager config("", userConfig);
        config.Initialize();

        config.SetValue("/test/value", 42);
        auto result = config.SaveUserOverrides();
        REQUIRE(result.IsSuccess());
        CHECK(AsyncIO::FileExists(userConfig));

        // Verify file content
        auto readResult = AsyncIO::ReadJsonFile(userConfig);
        REQUIRE(readResult.IsSuccess());
        CHECK(readResult.Value()["test"]["value"] == 42);

        // Cleanup
        AsyncIO::RemoveFile(userConfig);
    }

    TEST_CASE("MergeJson - Deep merges nested objects")
    {
        nlohmann::json base = nlohmann::json::parse(R"({
            "config": {
                "window": {"width": 1920, "height": 1080},
                "audio": {"volume": 0.5}
            }
        })");

        nlohmann::json overrides = nlohmann::json::parse(R"({
            "config": {
                "window": {"width": 2560}
            }
        })");

        auto merged = ConfigurationManager::MergeJsonPublic(base, overrides);
        CHECK(merged["config"]["window"]["width"] == 2560);
        CHECK(merged["config"]["window"]["height"] == 1080); // Preserved from base
        CHECK(merged["config"]["audio"]["volume"] == 0.5); // Preserved from base
    }
}

