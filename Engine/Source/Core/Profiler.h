#pragma once

/**
 * @file Profiler.h
 * @brief Performance profiling infrastructure for the Sabora Engine.
 * 
 * This module provides tools for measuring and benchmarking code performance,
 * including scoped timers, performance counters, and benchmark utilities.
 * 
 * Features:
 *   - Scoped performance timers with automatic reporting
 *   - High-resolution timing using std::chrono
 *   - Performance counter tracking
 *   - Benchmark utilities for repeated measurements
 *   - Thread-safe profiling operations
 * 
 * Usage Examples:
 * @code
 *   // Scoped timer
 *   {
 *       ScopedTimer timer("MyOperation");
 *       // ... code to measure ...
 *   } // Timer automatically reports duration
 * 
 *   // Manual timing
 *   auto start = Profiler::GetHighResolutionTime();
 *   // ... code ...
 *   auto duration = Profiler::GetHighResolutionTime() - start;
 *   Profiler::RecordMeasurement("Operation", duration);
 * 
 *   // Benchmark
 *   auto results = Profiler::Benchmark([]() {
 *       // ... code to benchmark ...
 *   }, 1000); // Run 1000 iterations
 * @endcode
 */

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <algorithm>
#include <numeric>
#include <cstdint>

namespace Sabora
{
    /**
     * @brief High-resolution time point type for precise measurements.
     */
    using HighResolutionTime = std::chrono::high_resolution_clock::time_point;

    /**
     * @brief Duration type for time measurements.
     */
    using Duration = std::chrono::high_resolution_clock::duration;

    /**
     * @brief Duration in milliseconds (double precision for sub-millisecond accuracy).
     */
    using DurationMs = std::chrono::duration<double, std::milli>;

    /**
     * @brief Duration in microseconds (double precision).
     */
    using DurationUs = std::chrono::duration<double, std::micro>;

    /**
     * @brief Duration in nanoseconds (double precision).
     */
    using DurationNs = std::chrono::duration<double, std::nano>;

    /**
     * @brief Performance measurement statistics.
     */
    struct PerformanceStats
    {
        std::string Name;
        double MinMs = 0.0;
        double MaxMs = 0.0;
        double AvgMs = 0.0;
        double TotalMs = 0.0;
        std::uint64_t Count = 0;
        double StdDevMs = 0.0;
    };

    /**
     * @brief Benchmark results from repeated measurements.
     */
    struct BenchmarkResults
    {
        std::string Name;
        std::uint64_t Iterations = 0;
        double TotalTimeMs = 0.0;
        double AverageTimeMs = 0.0;
        double MinTimeMs = 0.0;
        double MaxTimeMs = 0.0;
        double StdDevMs = 0.0;
        double Throughput = 0.0; // Operations per second
    };

    /**
     * @brief Performance profiling system for measuring code execution time.
     * 
     * The Profiler class provides a comprehensive set of tools for performance
     * measurement and benchmarking. All operations are thread-safe.
     */
    class Profiler
    {
    public:
        /**
         * @brief Initialize the profiling system.
         * 
         * Should be called once at application startup.
         */
        static void Initialize();

        /**
         * @brief Shutdown the profiling system and generate final reports.
         */
        static void Shutdown();

        /**
         * @brief Get the current high-resolution time point.
         * @return Current time point.
         */
        static HighResolutionTime GetHighResolutionTime() noexcept;

        /**
         * @brief Convert a duration to milliseconds.
         * @param duration The duration to convert.
         * @return Duration in milliseconds.
         */
        static double ToMilliseconds(Duration duration) noexcept;

        /**
         * @brief Convert a duration to microseconds.
         * @param duration The duration to convert.
         * @return Duration in microseconds.
         */
        static double ToMicroseconds(Duration duration) noexcept;

        /**
         * @brief Convert a duration to nanoseconds.
         * @param duration The duration to convert.
         * @return Duration in nanoseconds.
         */
        static double ToNanoseconds(Duration duration) noexcept;

        /**
         * @brief Record a single performance measurement.
         * @param name The name/identifier of the measurement.
         * @param duration The duration to record.
         */
        static void RecordMeasurement(const std::string& name, Duration duration);

        /**
         * @brief Get performance statistics for a named measurement.
         * @param name The name of the measurement.
         * @return Performance statistics, or empty stats if not found.
         */
        static PerformanceStats GetStats(const std::string& name);

        /**
         * @brief Get all recorded performance statistics.
         * @return Vector of all performance statistics.
         */
        static std::vector<PerformanceStats> GetAllStats();

        /**
         * @brief Clear all recorded measurements.
         */
        static void ClearAll();

        /**
         * @brief Clear measurements for a specific name.
         * @param name The name of the measurements to clear.
         */
        static void Clear(const std::string& name);

        /**
         * @brief Run a benchmark of a function.
         * @tparam Func Function type to benchmark.
         * @param func The function to benchmark.
         * @param iterations Number of iterations to run.
         * @param name Optional name for the benchmark.
         * @return Benchmark results.
         */
        template<typename Func>
        static BenchmarkResults Benchmark(Func&& func, std::uint64_t iterations, const std::string& name = "");

        /**
         * @brief Print all performance statistics to console.
         */
        static void PrintStats();

        /**
         * @brief Print statistics for a specific measurement.
         * @param name The name of the measurement.
         */
        static void PrintStats(const std::string& name);

    private:
        // Internal versions that assume mutex is already locked
        static void PrintStatsLocked();
        static void PrintStatsLocked(const std::string& name);
        static PerformanceStats GetStatsLocked(const std::string& name);

    private:
        struct MeasurementData
        {
            std::vector<double> Measurements; // In milliseconds
            std::uint64_t Count = 0;
            double TotalMs = 0.0;
        };

        static std::mutex s_Mutex;
        static std::unordered_map<std::string, MeasurementData> s_Measurements;
        static bool s_Initialized;
    };

    /**
     * @brief RAII scoped timer for automatic performance measurement.
     * 
     * Automatically measures the duration of a code block and records it
     * when the timer goes out of scope.
     * 
     * @example
     * @code
     *   {
     *       ScopedTimer timer("MyFunction");
     *       // ... code to measure ...
     *   } // Timer automatically records duration here
     * @endcode
     */
    class ScopedTimer
    {
    public:
        /**
         * @brief Construct a scoped timer with a name.
         * @param name The name/identifier for this measurement.
         */
        explicit ScopedTimer(const std::string& name);

        /**
         * @brief Destructor - automatically records the measurement.
         */
        ~ScopedTimer();

        // Non-copyable
        ScopedTimer(const ScopedTimer&) = delete;
        ScopedTimer& operator=(const ScopedTimer&) = delete;

        // Movable
        ScopedTimer(ScopedTimer&&) noexcept = default;
        ScopedTimer& operator=(ScopedTimer&&) noexcept = default;

        /**
         * @brief Stop the timer early and record the measurement.
         */
        void Stop();

    private:
        std::string m_Name;
        HighResolutionTime m_StartTime;
        bool m_Stopped = false;
    };

    // Template implementation
    template<typename Func>
    BenchmarkResults Profiler::Benchmark(Func&& func, std::uint64_t iterations, const std::string& name)
    {
        BenchmarkResults results;
        results.Name = name.empty() ? "Benchmark" : name;
        results.Iterations = iterations;

        if (iterations == 0)
        {
            return results;
        }

        std::vector<double> measurements;
        measurements.reserve(iterations);

        // Warm-up run (optional, but helps with cache effects)
        if (iterations > 1)
        {
            func();
        }

        // Run benchmark
        const auto totalStart = GetHighResolutionTime();
        for (std::uint64_t i = 0; i < iterations; ++i)
        {
            const auto iterStart = GetHighResolutionTime();
            func();
            const auto iterEnd = GetHighResolutionTime();
            const double iterMs = ToMilliseconds(iterEnd - iterStart);
            measurements.push_back(iterMs);
        }
        const auto totalEnd = GetHighResolutionTime();

        results.TotalTimeMs = ToMilliseconds(totalEnd - totalStart);

        // Calculate statistics
        if (!measurements.empty())
        {
            std::sort(measurements.begin(), measurements.end());
            results.MinTimeMs = measurements.front();
            results.MaxTimeMs = measurements.back();
            results.AverageTimeMs = results.TotalTimeMs / static_cast<double>(iterations);

            // Calculate standard deviation
            const double mean = results.AverageTimeMs;
            double variance = 0.0;
            for (double value : measurements)
            {
                const double diff = value - mean;
                variance += diff * diff;
            }
            results.StdDevMs = std::sqrt(variance / static_cast<double>(iterations));

            // Calculate throughput (operations per second)
            if (results.AverageTimeMs > 0.0)
            {
                results.Throughput = 1000.0 / results.AverageTimeMs;
            }
        }

        return results;
    }

} // namespace Sabora

