#pragma once

// CRITICAL: Include chrono FIRST before any other headers to avoid Windows case-insensitivity conflicts
// On Windows, file system is case-insensitive, so Time.h might conflict with system time.h
// We must include chrono before any system headers that might pull in time.h
#include <chrono>

// Now include other headers
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace Sabora
{
    /**
     * @brief Unity-style Time class for convenient access to time-related information.
     * 
     * Provides static access to time data similar to Unity's Time class, making it
     * easy to access delta time, elapsed time, frame count, and time scale from anywhere.
     * 
     * Usage:
     * @code
     *   // In OnUpdate or anywhere in your code
     *   float speed = 5.0f * Time::GetDeltaTime();
     *   position += velocity * Time::GetDeltaTime();
     *   
     *   // Check elapsed time
     *   if (Time::GetTime() > 10.0f)
     *   {
     *       // 10 seconds have passed
     *   }
     *   
     *   // Pause/slow motion
     *   Time::SetTimeScale(0.5f); // Half speed
     *   Time::SetTimeScale(0.0f); // Paused
     * @endcode
     */
    class Time
    {
    public:
        /**
         * @brief Update the Time system with the current frame's delta time.
         * @param unscaledDeltaTime The unscaled delta time in seconds.
         * 
         * This should be called once per frame by the Application class.
         * Do not call this manually - it's handled automatically by the engine.
         */
        static void Update(float unscaledDeltaTime);

        /**
         * @brief Get the time in seconds it took to complete the last frame (scaled by timeScale).
         * @return Delta time in seconds, scaled by timeScale.
         * 
         * This is the time between the current and previous frame, multiplied by timeScale.
         * Use this for frame-rate independent movement and animations.
         * 
         * Example:
         *   position += velocity * Time::GetDeltaTime();
         */
        [[nodiscard]] static float GetDeltaTime() noexcept { return s_DeltaTime; }

        /**
         * @brief Get the time in seconds it took to complete the last frame (unscaled).
         * @return Unscaled delta time in seconds.
         * 
         * This is the actual time between frames, unaffected by timeScale.
         * Use this for UI updates, input handling, or anything that should
         * run at real-time speed regardless of timeScale.
         */
        [[nodiscard]] static float GetUnscaledDeltaTime() noexcept { return s_UnscaledDeltaTime; }

        /**
         * @brief Get the time in seconds since the application started (scaled by timeScale).
         * @return Time since start in seconds, scaled by timeScale.
         * 
         * This value is affected by timeScale. If timeScale is 0.5, this will
         * advance at half speed. If timeScale is 0, this will not advance.
         */
        [[nodiscard]] static float GetTime() noexcept { return s_Time; }

        /**
         * @brief Get the time in seconds since the application started (unscaled).
         * @return Unscaled time since start in seconds.
         * 
         * This value is not affected by timeScale. It always represents the
         * actual real-world time since the application started.
         */
        [[nodiscard]] static float GetUnscaledTime() noexcept { return s_UnscaledTime; }

        /**
         * @brief Get the time in seconds since the application started (unscaled, high precision).
         * @return Realtime since startup in seconds.
         * 
         * This is the same as GetUnscaledTime() but uses a high-precision clock.
         * Useful for profiling, benchmarking, or when you need maximum precision.
         */
        [[nodiscard]] static float GetRealtimeSinceStartup() noexcept { return s_RealtimeSinceStartup; }

        /**
         * @brief Get the current frame count.
         * @return The number of frames that have been processed since the application started.
         */
        [[nodiscard]] static uint64_t GetFrameCount() noexcept { return s_FrameCount; }

        /**
         * @brief Get the time scale factor.
         * @return The current time scale (1.0 = normal, 0.0 = paused, 2.0 = double speed).
         * 
         * The time scale affects GetDeltaTime() and GetTime(), but not
         * GetUnscaledDeltaTime() or GetUnscaledTime().
         */
        [[nodiscard]] static float GetTimeScale() noexcept { return s_TimeScale; }

        /**
         * @brief Set the time scale factor.
         * @param timeScale The new time scale (1.0 = normal, 0.0 = paused, 2.0 = double speed).
         * 
         * Setting timeScale to 0.0 effectively pauses the game (scaled time stops advancing).
         * Values greater than 1.0 speed up time, values less than 1.0 slow it down.
         * 
         * @note Negative values are clamped to 0.0.
         */
        static void SetTimeScale(float timeScale) noexcept;

        /**
         * @brief Get the fixed delta time for physics updates.
         * @return Fixed delta time in seconds.
         * 
         * This is typically used for physics simulations that require a fixed timestep.
         * Default is 1/60 seconds (60 FPS).
         */
        [[nodiscard]] static float GetFixedDeltaTime() noexcept { return s_FixedDeltaTime; }

        /**
         * @brief Set the fixed delta time for physics updates.
         * @param fixedDeltaTime The fixed delta time in seconds.
         * 
         * This should match your physics timestep. Common values:
         * - 1/60 (60 FPS)
         * - 1/50 (50 FPS)
         * - 1/30 (30 FPS)
         * 
         * @note Must be positive. Negative or zero values are clamped to a small positive value.
         */
        static void SetFixedDeltaTime(float fixedDeltaTime) noexcept;

        /**
         * @brief Get the maximum delta time per frame (for frame spike protection).
         * @return Maximum allowed delta time in seconds.
         * 
         * If a frame takes longer than this, deltaTime will be clamped to this value.
         * This prevents large time jumps that can break physics or animations.
         * Default is 0.1 seconds (10 FPS minimum).
         */
        [[nodiscard]] static float GetMaximumDeltaTime() noexcept { return s_MaximumDeltaTime; }

        /**
         * @brief Set the maximum delta time per frame.
         * @param maximumDeltaTime Maximum allowed delta time in seconds.
         * 
         * Use this to prevent frame spikes from causing issues. If a frame takes
         * longer than this value, deltaTime will be clamped.
         * 
         * @note Must be positive. Negative or zero values are clamped to a small positive value.
         */
        static void SetMaximumDeltaTime(float maximumDeltaTime) noexcept;

        /**
         * @brief Get the smooth delta time (averaged over recent frames).
         * @return Smoothed delta time in seconds.
         * 
         * This provides a smoothed version of deltaTime that reduces jitter
         * from frame time variations. Useful for UI animations or camera smoothing.
         */
        [[nodiscard]] static float GetSmoothDeltaTime() noexcept { return s_SmoothDeltaTime; }

        /**
         * @brief Reset the Time system to initial state.
         * 
         * Resets all time values to zero and frame count to 0.
         * Useful for testing or restarting time tracking.
         */
        static void Reset() noexcept;

    private:
        // Scaled time values (affected by timeScale)
        static float s_DeltaTime;
        static float s_Time;

        // Unscaled time values (not affected by timeScale)
        static float s_UnscaledDeltaTime;
        static float s_UnscaledTime;
        static float s_RealtimeSinceStartup;

        // Time scale and settings
        static float s_TimeScale;
        static float s_FixedDeltaTime;
        static float s_MaximumDeltaTime;

        // Frame tracking
        static uint64_t s_FrameCount;

        // Smooth delta time calculation
        static float s_SmoothDeltaTime;
        static constexpr size_t SMOOTH_DELTA_TIME_SAMPLES = 10;
        static float s_DeltaTimeHistory[SMOOTH_DELTA_TIME_SAMPLES];
        static size_t s_DeltaTimeHistoryIndex;

        // High-precision clock for realtime tracking
        // Use fully qualified name to avoid any potential conflicts
        using Clock = ::std::chrono::high_resolution_clock;
        static Clock::time_point s_StartTime;

        // Helper to clamp delta time
        static float ClampDeltaTime(float deltaTime) noexcept;
    };

} // namespace Sabora
