#include "Profiler.h"
#include "Log.h"
#include <cmath>
#include <iomanip>
#include <sstream>

namespace Sabora
{
    // Static member initialization
    std::mutex Profiler::s_Mutex;
    std::unordered_map<std::string, Profiler::MeasurementData> Profiler::s_Measurements;
    bool Profiler::s_Initialized = false;

    void Profiler::Initialize()
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        if (!s_Initialized)
        {
            s_Initialized = true;
        }
    }

    void Profiler::Shutdown()
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        if (s_Initialized)
        {
            PrintStatsLocked(); // Call version that doesn't lock
            s_Measurements.clear();
            s_Initialized = false;
        }
    }

    HighResolutionTime Profiler::GetHighResolutionTime() noexcept
    {
        return std::chrono::high_resolution_clock::now();
    }

    double Profiler::ToMilliseconds(Duration duration) noexcept
    {
        return std::chrono::duration_cast<DurationMs>(duration).count();
    }

    double Profiler::ToMicroseconds(Duration duration) noexcept
    {
        return std::chrono::duration_cast<DurationUs>(duration).count();
    }

    double Profiler::ToNanoseconds(Duration duration) noexcept
    {
        return std::chrono::duration_cast<DurationNs>(duration).count();
    }

    void Profiler::RecordMeasurement(const std::string& name, Duration duration)
    {
        if (!s_Initialized)
        {
            return;
        }

        const double ms = ToMilliseconds(duration);

        std::lock_guard<std::mutex> lock(s_Mutex);
        auto& data = s_Measurements[name];
        data.Measurements.push_back(ms);
        data.TotalMs += ms;
        data.Count++;
    }

    PerformanceStats Profiler::GetStats(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        return GetStatsLocked(name);
    }

    PerformanceStats Profiler::GetStatsLocked(const std::string& name)
    {
        // Assumes mutex is already locked
        auto it = s_Measurements.find(name);
        if (it == s_Measurements.end() || it->second.Measurements.empty())
        {
            return PerformanceStats{name};
        }

        const auto& data = it->second;
        PerformanceStats stats;
        stats.Name = name;
        stats.Count = data.Count;
        stats.TotalMs = data.TotalMs;

        if (!data.Measurements.empty())
        {
            auto measurements = data.Measurements; // Copy for sorting
            std::sort(measurements.begin(), measurements.end());
            
            stats.MinMs = measurements.front();
            stats.MaxMs = measurements.back();
            stats.AvgMs = data.TotalMs / static_cast<double>(data.Count);

            // Calculate standard deviation
            const double mean = stats.AvgMs;
            double variance = 0.0;
            for (double value : measurements)
            {
                const double diff = value - mean;
                variance += diff * diff;
            }
            stats.StdDevMs = std::sqrt(variance / static_cast<double>(measurements.size()));
        }

        return stats;
    }

    std::vector<PerformanceStats> Profiler::GetAllStats()
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        
        std::vector<PerformanceStats> stats;
        stats.reserve(s_Measurements.size());

        for (const auto& [name, data] : s_Measurements)
        {
            stats.push_back(GetStatsLocked(name));
        }

        return stats;
    }

    void Profiler::ClearAll()
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        s_Measurements.clear();
    }

    void Profiler::Clear(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        s_Measurements.erase(name);
    }

    void Profiler::PrintStats()
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        PrintStatsLocked();
    }

    void Profiler::PrintStatsLocked()
    {
        // Assumes mutex is already locked
        if (s_Measurements.empty())
        {
            SB_INFO("No performance measurements recorded.");
            return;
        }

        SB_INFO("=== Performance Statistics ===");
        
        for (const auto& [name, data] : s_Measurements)
        {
            PrintStatsLocked(name);
        }
    }

    void Profiler::PrintStats(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        PrintStatsLocked(name);
    }

    void Profiler::PrintStatsLocked(const std::string& name)
    {
        // Assumes mutex is already locked
        const auto stats = GetStatsLocked(name);
        
        if (stats.Count == 0)
        {
            SB_INFO("No measurements found for: {}", name);
            return;
        }

        std::stringstream ss;
        ss << std::fixed << std::setprecision(3);
        ss << "[" << stats.Name << "] "
           << "Count: " << stats.Count << ", "
           << "Total: " << stats.TotalMs << "ms, "
           << "Avg: " << stats.AvgMs << "ms, "
           << "Min: " << stats.MinMs << "ms, "
           << "Max: " << stats.MaxMs << "ms, "
           << "StdDev: " << stats.StdDevMs << "ms";
        
        SB_INFO(ss.str());
    }

    // ScopedTimer implementation
    ScopedTimer::ScopedTimer(const std::string& name)
        : m_Name(name)
        , m_StartTime(Profiler::GetHighResolutionTime())
        , m_Stopped(false)
    {
    }

    ScopedTimer::~ScopedTimer()
    {
        if (!m_Stopped)
        {
            Stop();
        }
    }

    void ScopedTimer::Stop()
    {
        if (m_Stopped)
        {
            return;
        }

        const auto endTime = Profiler::GetHighResolutionTime();
        const auto duration = endTime - m_StartTime;
        Profiler::RecordMeasurement(m_Name, duration);
        m_Stopped = true;
    }

} // namespace Sabora