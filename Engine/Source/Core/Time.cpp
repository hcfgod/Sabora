#include "pch.h"
#include "Time.h"

namespace Sabora
{
    // Static member initialization
    float Time::s_DeltaTime = 0.0f;
    float Time::s_Time = 0.0f;
    float Time::s_UnscaledDeltaTime = 0.0f;
    float Time::s_UnscaledTime = 0.0f;
    float Time::s_RealtimeSinceStartup = 0.0f;
    float Time::s_TimeScale = 1.0f;
    float Time::s_FixedDeltaTime = 1.0f / 60.0f; // 60 FPS default
    float Time::s_MaximumDeltaTime = 0.1f; // 10 FPS minimum
    uint64_t Time::s_FrameCount = 0;
    float Time::s_SmoothDeltaTime = 0.0f;
    float Time::s_DeltaTimeHistory[SMOOTH_DELTA_TIME_SAMPLES] = {};
    size_t Time::s_DeltaTimeHistoryIndex = 0;
    Time::Clock::time_point Time::s_StartTime = ::std::chrono::high_resolution_clock::now();

    void Time::Update(float unscaledDeltaTime)
    {
        // Clamp delta time to prevent frame spikes
        s_UnscaledDeltaTime = ClampDeltaTime(unscaledDeltaTime);

        // Update unscaled time (real time)
        s_UnscaledTime += s_UnscaledDeltaTime;

        // Update realtime since startup using high-precision clock
        const auto now = Clock::now();
        const auto duration = ::std::chrono::duration<float>(now - s_StartTime);
        s_RealtimeSinceStartup = duration.count();

        // Calculate scaled delta time (affected by timeScale)
        s_DeltaTime = s_UnscaledDeltaTime * s_TimeScale;

        // Update scaled time
        s_Time += s_DeltaTime;

        // Update smooth delta time (moving average)
        s_DeltaTimeHistory[s_DeltaTimeHistoryIndex] = s_DeltaTime;
        s_DeltaTimeHistoryIndex = (s_DeltaTimeHistoryIndex + 1) % SMOOTH_DELTA_TIME_SAMPLES;

        // Calculate average of recent delta times
        float sum = 0.0f;
        size_t validSamples = 0;
        for (size_t i = 0; i < SMOOTH_DELTA_TIME_SAMPLES; ++i)
        {
            if (s_DeltaTimeHistory[i] > 0.0f) // Only count initialized samples
            {
                sum += s_DeltaTimeHistory[i];
                validSamples++;
            }
        }
        s_SmoothDeltaTime = validSamples > 0 ? sum / static_cast<float>(validSamples) : s_DeltaTime;

        // Increment frame count
        s_FrameCount++;
    }

    void Time::SetTimeScale(float timeScale) noexcept
    {
        // Clamp to non-negative values (negative time doesn't make sense)
        s_TimeScale = std::max(0.0f, timeScale);
    }

    void Time::SetFixedDeltaTime(float fixedDeltaTime) noexcept
    {
        // Ensure positive value (minimum 1/1000 second)
        constexpr float MIN_FIXED_DELTA_TIME = 0.001f;
        s_FixedDeltaTime = std::max(MIN_FIXED_DELTA_TIME, fixedDeltaTime);
    }

    void Time::SetMaximumDeltaTime(float maximumDeltaTime) noexcept
    {
        // Ensure positive value (minimum 1/1000 second)
        constexpr float MIN_MAX_DELTA_TIME = 0.001f;
        s_MaximumDeltaTime = std::max(MIN_MAX_DELTA_TIME, maximumDeltaTime);
    }

    void Time::Reset() noexcept
    {
        s_DeltaTime = 0.0f;
        s_Time = 0.0f;
        s_UnscaledDeltaTime = 0.0f;
        s_UnscaledTime = 0.0f;
        s_RealtimeSinceStartup = 0.0f;
        s_FrameCount = 0;
        s_SmoothDeltaTime = 0.0f;
        s_DeltaTimeHistoryIndex = 0;

        // Reset history array
        for (size_t i = 0; i < SMOOTH_DELTA_TIME_SAMPLES; ++i)
        {
            s_DeltaTimeHistory[i] = 0.0f;
        }

        // Reset start time
        s_StartTime = ::std::chrono::high_resolution_clock::now();
    }

    float Time::ClampDeltaTime(float deltaTime) noexcept
    {
        // Clamp to maximum delta time to prevent frame spikes
        return std::min(deltaTime, s_MaximumDeltaTime);
    }

} // namespace Sabora
