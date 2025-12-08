/**
 * @file TestProfiler.cpp
 * @brief Unit tests for Profiler functionality.
 */

#include "doctest.h"
#include "Profiler.h"

#include <thread>
#include <chrono>

TEST_SUITE("Profiler")
{
    TEST_CASE("Initialize - Sets up profiling system")
    {
        Profiler::Initialize();
        
        // Should be able to get time
        auto time = Profiler::GetHighResolutionTime();
        CHECK(time.time_since_epoch().count() > 0);
        
        Profiler::Shutdown();
    }

    TEST_CASE("ToMilliseconds - Converts duration correctly")
    {
        Profiler::Initialize();
        
        auto start = Profiler::GetHighResolutionTime();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        auto end = Profiler::GetHighResolutionTime();
        
        double ms = Profiler::ToMilliseconds(end - start);
        CHECK(ms >= 9.0); // Allow some tolerance
        CHECK(ms < 100.0); // Should be reasonable
        
        Profiler::Shutdown();
    }

    TEST_CASE("RecordMeasurement - Records performance data")
    {
        Profiler::Initialize();
        Profiler::ClearAll();
        
        auto start = Profiler::GetHighResolutionTime();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        auto end = Profiler::GetHighResolutionTime();
        
        Profiler::RecordMeasurement("TestOperation", end - start);
        
        auto stats = Profiler::GetStats("TestOperation");
        CHECK(stats.Count == 1);
        CHECK(stats.TotalMs > 0);
        
        Profiler::Shutdown();
    }

    TEST_CASE("ScopedTimer - Automatically records duration")
    {
        Profiler::Initialize();
        Profiler::ClearAll();
        
        {
            ScopedTimer timer("ScopedTest");
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        
        auto stats = Profiler::GetStats("ScopedTest");
        CHECK(stats.Count == 1);
        CHECK(stats.TotalMs > 0);
        
        Profiler::Shutdown();
    }

    TEST_CASE("Benchmark - Runs function multiple times")
    {
        Profiler::Initialize();
        
        int counter = 0;
        auto results = Profiler::Benchmark([&counter]() {
            counter++;
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }, 10, "BenchmarkTest");
        
        CHECK(results.Iterations == 10);
        CHECK(counter == 10);
        CHECK(results.TotalTimeMs > 0);
        CHECK(results.AverageTimeMs > 0);
        CHECK(results.MinTimeMs > 0);
        CHECK(results.MaxTimeMs >= results.MinTimeMs);
        
        Profiler::Shutdown();
    }

    TEST_CASE("GetAllStats - Returns all measurements")
    {
        Profiler::Initialize();
        Profiler::ClearAll();
        
        Profiler::RecordMeasurement("Test1", std::chrono::milliseconds(10));
        Profiler::RecordMeasurement("Test2", std::chrono::milliseconds(20));
        
        auto allStats = Profiler::GetAllStats();
        CHECK(allStats.size() == 2);
        
        Profiler::Shutdown();
    }

    TEST_CASE("Clear - Removes specific measurement")
    {
        Profiler::Initialize();
        Profiler::ClearAll();
        
        Profiler::RecordMeasurement("Test1", std::chrono::milliseconds(10));
        Profiler::RecordMeasurement("Test2", std::chrono::milliseconds(20));
        
        Profiler::Clear("Test1");
        
        auto stats1 = Profiler::GetStats("Test1");
        CHECK(stats1.Count == 0);
        
        auto stats2 = Profiler::GetStats("Test2");
        CHECK(stats2.Count == 1);
        
        Profiler::Shutdown();
    }
}

