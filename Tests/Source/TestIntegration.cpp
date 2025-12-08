/**
 * @file TestIntegration.cpp
 * @brief Integration tests for Sabora Engine systems.
 * 
 * These tests verify that multiple systems work together correctly.
 */

#include "doctest.h"
#include "Sabora.h"
#include "Core/AsyncIO.h"
#include "Core/ConfigurationManager.h"
#include "Core/Log.h"
#include "Core/Profiler.h"

#include <filesystem>

using namespace Sabora;
#include <fstream>

namespace fs = std::filesystem;

TEST_SUITE("Integration")
{
    TEST_CASE("Full configuration workflow")
    {
        // Initialize systems
        Log::Initialize();
        Profiler::Initialize();

        const fs::path defaultConfig = "test_integration_default.json";
        const fs::path userConfig = "test_integration_user.json";

        // Create default configuration
        {
            std::ofstream out(defaultConfig);
            REQUIRE(out.is_open());
            out << R"({
                "app": {"name": "Sabora", "version": "1.0.0"},
                "window": {"width": 1920, "height": 1080}
            })";
        }

        // Create user overrides
        {
            std::ofstream out(userConfig);
            REQUIRE(out.is_open());
            out << R"({
                "window": {"width": 2560}
            })";
        }

        // Test full workflow
        ConfigurationManager config(defaultConfig, userConfig);
        REQUIRE(config.Initialize());

        // Modify configuration
        config.SetValue("/window/fullscreen", true);
        config.SetValue("/audio/volume", 0.75);

        // Save user overrides
        auto saveResult = config.SaveUserOverrides();
        REQUIRE(saveResult.IsSuccess());

        // Verify saved file
        auto readResult = AsyncIO::ReadJsonFile(userConfig);
        REQUIRE(readResult.IsSuccess());
        const auto& saved = readResult.Value();
        CHECK(saved["window"]["width"] == 2560);
        CHECK(saved["window"]["fullscreen"] == true);
        CHECK(saved["audio"]["volume"] == 0.75);

        // Cleanup
        AsyncIO::RemoveFile(defaultConfig);
        AsyncIO::RemoveFile(userConfig);
        Log::Shutdown();
        Profiler::Shutdown();
    }

    TEST_CASE("Error handling chain")
    {
        // Test that errors propagate correctly through Result chains
        auto result = AsyncIO::ReadTextFile("non_existent_file.txt")
            .AndThen([](const std::string& content) {
                // This should not be called
                return Result<int>::Success(static_cast<int>(content.length()));
            })
            .OrElse([](const Error& err) {
                // This should be called
                CHECK(err.Code() == ErrorCode::FileReadError);
                return Result<int>::Success(0);
            });

        REQUIRE(result.IsSuccess());
        CHECK(result.Value() == 0);
    }

    TEST_CASE("Performance profiling with file operations")
    {
        Profiler::Initialize();
        Profiler::ClearAll();

        const fs::path testFile = "test_perf.txt";
        const std::string content(1000, 'A'); // 1KB of data

        // Profile file write
        {
            ScopedTimer timer("FileWrite");
            auto result = AsyncIO::WriteTextFile(testFile, content, true);
            REQUIRE(result.IsSuccess());
        }

        // Profile file read
        {
            ScopedTimer timer("FileRead");
            auto result = AsyncIO::ReadTextFile(testFile);
            REQUIRE(result.IsSuccess());
        }

        // Verify measurements were recorded
        auto writeStats = Profiler::GetStats("FileWrite");
        auto readStats = Profiler::GetStats("FileRead");
        
        CHECK(writeStats.Count == 1);
        CHECK(readStats.Count == 1);
        CHECK(writeStats.TotalMs > 0);
        CHECK(readStats.TotalMs > 0);

        // Cleanup
        AsyncIO::RemoveFile(testFile);
        Profiler::Shutdown();
    }

    TEST_CASE("Logging with configuration")
    {
        Log::Initialize();
        
        // Set log level
        Log::SetLogLevel(LogLevel::Info);
        Log::SetLogLevel(LogCategory::Core, LogLevel::Debug);

        // Log messages (should not crash)
        SB_INFO("Integration test info message");
        SB_CORE_DEBUG("Integration test debug message");
        SB_WARN("Integration test warning");

        Log::Shutdown();
    }
}

