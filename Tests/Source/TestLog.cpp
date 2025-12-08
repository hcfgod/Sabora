/**
 * @file TestLog.cpp
 * @brief Unit tests for Log system functionality.
 */

#include "doctest.h"
#include "Core/Log.h"

using namespace Sabora;

TEST_SUITE("Log")
{
    TEST_CASE("Initialize - Sets up logging system")
    {
        Log::Initialize();
        CHECK(Log::GetLogLevel() >= LogLevel::Trace);
        
        // Should be able to log without crashing
        Log::Info("Test log message");
        
        Log::Shutdown();
    }

    TEST_CASE("SetLogLevel - Changes global log level")
    {
        Log::Initialize();
        
        Log::SetLogLevel(LogLevel::Warn);
        CHECK(Log::GetLogLevel() == LogLevel::Warn);
        
        Log::SetLogLevel(LogLevel::Info);
        CHECK(Log::GetLogLevel() == LogLevel::Info);
        
        Log::Shutdown();
    }

    TEST_CASE("SetLogLevel - Changes category log level")
    {
        Log::Initialize();
        
        Log::SetLogLevel(LogCategory::Renderer, LogLevel::Error);
        CHECK(Log::GetLogLevel(LogCategory::Renderer) == LogLevel::Error);
        
        // Other categories should be unaffected
        CHECK(Log::GetLogLevel(LogCategory::Core) == LogLevel::Info);
        
        Log::Shutdown();
    }

    TEST_CASE("Category logging - Respects category level")
    {
        Log::Initialize();
        
        Log::SetLogLevel(LogCategory::Audio, LogLevel::Error);
        
        // Should be able to log error level
        Log::Error(LogCategory::Audio, "Audio error test");
        
        // Debug should be filtered (but won't crash)
        Log::Debug(LogCategory::Audio, "This should be filtered");
        
        Log::Shutdown();
    }

    TEST_CASE("Format logging - Works with format strings")
    {
        Log::Initialize();
        
        // Should not crash with format strings
        Log::Info("Test format: {}", 42);
        Log::Info(LogCategory::Core, "Category format: {}", "test");
        
        Log::Shutdown();
    }
}

