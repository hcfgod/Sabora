#pragma once

#include <memory>
#include <string>
#include <array>
#include <spdlog/fmt/fmt.h>

namespace Sabora {

    // Log levels for the engine
    enum class LogLevel : uint8_t {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warn = 3,
        Error = 4,
        Critical = 5
    };

    // Log categories for different engine systems
    enum class LogCategory : uint8_t {
        Core = 0,
        Renderer = 1,
        Audio = 2,
        Physics = 3,
        Input = 4,
        Scene = 5,
        Script = 6,
        Network = 7,
        Editor = 8,
        Client = 9
    };

    class Log {
    public:
        static void Init();
        static void Shutdown();

        // Core logging methods
        static void Trace(const std::string& message);
        static void Debug(const std::string& message);
        static void Info(const std::string& message);
        static void Warn(const std::string& message);
        static void Error(const std::string& message);
        static void Critical(const std::string& message);

        // Category-based logging methods
        static void Trace(LogCategory category, const std::string& message);
        static void Debug(LogCategory category, const std::string& message);
        static void Info(LogCategory category, const std::string& message);
        static void Warn(LogCategory category, const std::string& message);
        static void Error(LogCategory category, const std::string& message);
        static void Critical(LogCategory category, const std::string& message);

        // Format-based logging methods
        template<typename... Args>
        static void Trace(const std::string& format, const Args&... args);
        
        template<typename... Args>
        static void Debug(const std::string& format, const Args&... args);
        
        template<typename... Args>
        static void Info(const std::string& format, const Args&... args);
        
        template<typename... Args>
        static void Warn(const std::string& format, const Args&... args);
        
        template<typename... Args>
        static void Error(const std::string& format, const Args&... args);
        
        template<typename... Args>
        static void Critical(const std::string& format, const Args&... args);

        // Category-based format logging
        template<typename... Args>
        static void Trace(LogCategory category, const std::string& format, const Args&... args);
        
        template<typename... Args>
        static void Debug(LogCategory category, const std::string& format, const Args&... args);
        
        template<typename... Args>
        static void Info(LogCategory category, const std::string& format, const Args&... args);
        
        template<typename... Args>
        static void Warn(LogCategory category, const std::string& format, const Args&... args);
        
        template<typename... Args>
        static void Error(LogCategory category, const std::string& format, const Args&... args);
        
        template<typename... Args>
        static void Critical(LogCategory category, const std::string& format, const Args&... args);

        // Utility methods
        static void SetLogLevel(LogLevel level);
        static void SetLogLevel(LogCategory category, LogLevel level);
        static LogLevel GetLogLevel();
        static LogLevel GetLogLevel(LogCategory category);
        
        // File logging
        static void EnableFileLogging(const std::string& filename);
        static void DisableFileLogging();
        
        // Console logging
        static void EnableConsoleLogging();
        static void DisableConsoleLogging();

    protected:
        static std::string GetCategoryString(LogCategory category);
        static std::string GetLevelString(LogLevel level);
        static std::string FormatLogMessage(LogLevel level, LogCategory category, const std::string& message);
        
        static bool s_Initialized;
        static LogLevel s_GlobalLogLevel;
        static std::array<LogLevel, 10> s_CategoryLogLevels;
    };

    // Template implementations
    template<typename... Args>
    void Log::Trace(const std::string& format, const Args&... args) {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Trace) < static_cast<uint8_t>(s_GlobalLogLevel)) {
            return;
        }
        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Trace(message);
    }

    template<typename... Args>
    void Log::Debug(const std::string& format, const Args&... args) {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Debug) < static_cast<uint8_t>(s_GlobalLogLevel)) {
            return;
        }
        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Debug(message);
    }

    template<typename... Args>
    void Log::Info(const std::string& format, const Args&... args) {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Info) < static_cast<uint8_t>(s_GlobalLogLevel)) {
            return;
        }
        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Info(message);
    }

    template<typename... Args>
    void Log::Warn(const std::string& format, const Args&... args) {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Warn) < static_cast<uint8_t>(s_GlobalLogLevel)) {
            return;
        }
        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Warn(message);
    }

    template<typename... Args>
    void Log::Error(const std::string& format, const Args&... args) {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Error) < static_cast<uint8_t>(s_GlobalLogLevel)) {
            return;
        }
        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Error(message);
    }

    template<typename... Args>
    void Log::Critical(const std::string& format, const Args&... args) {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Critical) < static_cast<uint8_t>(s_GlobalLogLevel)) {
            return;
        }
        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Critical(message);
    }

    // Category-based format logging
    template<typename... Args>
    void Log::Trace(LogCategory category, const std::string& format, const Args&... args) {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Trace) < static_cast<uint8_t>(s_CategoryLogLevels[static_cast<uint8_t>(category)])) {
            return;
        }
        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Trace(category, message);
    }

    template<typename... Args>
    void Log::Debug(LogCategory category, const std::string& format, const Args&... args) {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Debug) < static_cast<uint8_t>(s_CategoryLogLevels[static_cast<uint8_t>(category)])) {
            return;
        }
        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Debug(category, message);
    }

    template<typename... Args>
    void Log::Info(LogCategory category, const std::string& format, const Args&... args) {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Info) < static_cast<uint8_t>(s_CategoryLogLevels[static_cast<uint8_t>(category)])) {
            return;
        }
        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Info(category, message);
    }

    template<typename... Args>
    void Log::Warn(LogCategory category, const std::string& format, const Args&... args) {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Warn) < static_cast<uint8_t>(s_CategoryLogLevels[static_cast<uint8_t>(category)])) {
            return;
        }
        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Warn(category, message);
    }

    template<typename... Args>
    void Log::Error(LogCategory category, const std::string& format, const Args&... args) {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Error) < static_cast<uint8_t>(s_CategoryLogLevels[static_cast<uint8_t>(category)])) {
            return;
        }
        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Error(category, message);
    }

    template<typename... Args>
    void Log::Critical(LogCategory category, const std::string& format, const Args&... args) {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Critical) < static_cast<uint8_t>(s_CategoryLogLevels[static_cast<uint8_t>(category)])) {
            return;
        }
        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Critical(category, message);
    }

    // Convenience macros for easier logging
    #define SB_TRACE(...)    ::Sabora::Log::Trace(__VA_ARGS__)
    #define SB_DEBUG(...)    ::Sabora::Log::Debug(__VA_ARGS__)
    #define SB_INFO(...)     ::Sabora::Log::Info(__VA_ARGS__)
    #define SB_WARN(...)     ::Sabora::Log::Warn(__VA_ARGS__)
    #define SB_ERROR(...)    ::Sabora::Log::Error(__VA_ARGS__)
    #define SB_CRITICAL(...) ::Sabora::Log::Critical(__VA_ARGS__)

    // Category-based convenience macros
    #define SB_CORE_TRACE(...)    ::Sabora::Log::Trace(::Sabora::LogCategory::Core, __VA_ARGS__)
    #define SB_CORE_DEBUG(...)    ::Sabora::Log::Debug(::Sabora::LogCategory::Core, __VA_ARGS__)
    #define SB_CORE_INFO(...)     ::Sabora::Log::Info(::Sabora::LogCategory::Core, __VA_ARGS__)
    #define SB_CORE_WARN(...)     ::Sabora::Log::Warn(::Sabora::LogCategory::Core, __VA_ARGS__)
    #define SB_CORE_ERROR(...)    ::Sabora::Log::Error(::Sabora::LogCategory::Core, __VA_ARGS__)
    #define SB_CORE_CRITICAL(...) ::Sabora::Log::Critical(::Sabora::LogCategory::Core, __VA_ARGS__)

    #define SB_RENDERER_TRACE(...)    ::Sabora::Log::Trace(::Sabora::LogCategory::Renderer, __VA_ARGS__)
    #define SB_RENDERER_DEBUG(...)    ::Sabora::Log::Debug(::Sabora::LogCategory::Renderer, __VA_ARGS__)
    #define SB_RENDERER_INFO(...)     ::Sabora::Log::Info(::Sabora::LogCategory::Renderer, __VA_ARGS__)
    #define SB_RENDERER_WARN(...)     ::Sabora::Log::Warn(::Sabora::LogCategory::Renderer, __VA_ARGS__)
    #define SB_RENDERER_ERROR(...)    ::Sabora::Log::Error(::Sabora::LogCategory::Renderer, __VA_ARGS__)
    #define SB_RENDERER_CRITICAL(...) ::Sabora::Log::Critical(::Sabora::LogCategory::Renderer, __VA_ARGS__)

    #define SB_AUDIO_TRACE(...)    ::Sabora::Log::Trace(::Sabora::LogCategory::Audio, __VA_ARGS__)
    #define SB_AUDIO_DEBUG(...)    ::Sabora::Log::Debug(::Sabora::LogCategory::Audio, __VA_ARGS__)
    #define SB_AUDIO_INFO(...)     ::Sabora::Log::Info(::Sabora::LogCategory::Audio, __VA_ARGS__)
    #define SB_AUDIO_WARN(...)     ::Sabora::Log::Warn(::Sabora::LogCategory::Audio, __VA_ARGS__)
    #define SB_AUDIO_ERROR(...)    ::Sabora::Log::Error(::Sabora::LogCategory::Audio, __VA_ARGS__)
    #define SB_AUDIO_CRITICAL(...) ::Sabora::Log::Critical(::Sabora::LogCategory::Audio, __VA_ARGS__)

    #define SB_PHYSICS_TRACE(...)    ::Sabora::Log::Trace(::Sabora::LogCategory::Physics, __VA_ARGS__)
    #define SB_PHYSICS_DEBUG(...)    ::Sabora::Log::Debug(::Sabora::LogCategory::Physics, __VA_ARGS__)
    #define SB_PHYSICS_INFO(...)     ::Sabora::Log::Info(::Sabora::LogCategory::Physics, __VA_ARGS__)
    #define SB_PHYSICS_WARN(...)     ::Sabora::Log::Warn(::Sabora::LogCategory::Physics, __VA_ARGS__)
    #define SB_PHYSICS_ERROR(...)    ::Sabora::Log::Error(::Sabora::LogCategory::Physics, __VA_ARGS__)
    #define SB_PHYSICS_CRITICAL(...) ::Sabora::Log::Critical(::Sabora::LogCategory::Physics, __VA_ARGS__)

    #define SB_INPUT_TRACE(...)    ::Sabora::Log::Trace(::Sabora::LogCategory::Input, __VA_ARGS__)
    #define SB_INPUT_DEBUG(...)    ::Sabora::Log::Debug(::Sabora::LogCategory::Input, __VA_ARGS__)
    #define SB_INPUT_INFO(...)     ::Sabora::Log::Info(::Sabora::LogCategory::Input, __VA_ARGS__)
    #define SB_INPUT_WARN(...)     ::Sabora::Log::Warn(::Sabora::LogCategory::Input, __VA_ARGS__)
    #define SB_INPUT_ERROR(...)    ::Sabora::Log::Error(::Sabora::LogCategory::Input, __VA_ARGS__)
    #define SB_INPUT_CRITICAL(...) ::Sabora::Log::Critical(::Sabora::LogCategory::Input, __VA_ARGS__)

    #define SB_SCENE_TRACE(...)    ::Sabora::Log::Trace(::Sabora::LogCategory::Scene, __VA_ARGS__)
    #define SB_SCENE_DEBUG(...)    ::Sabora::Log::Debug(::Sabora::LogCategory::Scene, __VA_ARGS__)
    #define SB_SCENE_INFO(...)     ::Sabora::Log::Info(::Sabora::LogCategory::Scene, __VA_ARGS__)
    #define SB_SCENE_WARN(...)     ::Sabora::Log::Warn(::Sabora::LogCategory::Scene, __VA_ARGS__)
    #define SB_SCENE_ERROR(...)    ::Sabora::Log::Error(::Sabora::LogCategory::Scene, __VA_ARGS__)
    #define SB_SCENE_CRITICAL(...) ::Sabora::Log::Critical(::Sabora::LogCategory::Scene, __VA_ARGS__)

    #define SB_SCRIPT_TRACE(...)    ::Sabora::Log::Trace(::Sabora::LogCategory::Script, __VA_ARGS__)
    #define SB_SCRIPT_DEBUG(...)    ::Sabora::Log::Debug(::Sabora::LogCategory::Script, __VA_ARGS__)
    #define SB_SCRIPT_INFO(...)     ::Sabora::Log::Info(::Sabora::LogCategory::Script, __VA_ARGS__)
    #define SB_SCRIPT_WARN(...)     ::Sabora::Log::Warn(::Sabora::LogCategory::Script, __VA_ARGS__)
    #define SB_SCRIPT_ERROR(...)    ::Sabora::Log::Error(::Sabora::LogCategory::Script, __VA_ARGS__)
    #define SB_SCRIPT_CRITICAL(...) ::Sabora::Log::Critical(::Sabora::LogCategory::Script, __VA_ARGS__)

    #define SB_NETWORK_TRACE(...)    ::Sabora::Log::Trace(::Sabora::LogCategory::Network, __VA_ARGS__)
    #define SB_NETWORK_DEBUG(...)    ::Sabora::Log::Debug(::Sabora::LogCategory::Network, __VA_ARGS__)
    #define SB_NETWORK_INFO(...)     ::Sabora::Log::Info(::Sabora::LogCategory::Network, __VA_ARGS__)
    #define SB_NETWORK_WARN(...)     ::Sabora::Log::Warn(::Sabora::LogCategory::Network, __VA_ARGS__)
    #define SB_NETWORK_ERROR(...)    ::Sabora::Log::Error(::Sabora::LogCategory::Network, __VA_ARGS__)
    #define SB_NETWORK_CRITICAL(...) ::Sabora::Log::Critical(::Sabora::LogCategory::Network, __VA_ARGS__)

    #define SB_EDITOR_TRACE(...)    ::Sabora::Log::Trace(::Sabora::LogCategory::Editor, __VA_ARGS__)
    #define SB_EDITOR_DEBUG(...)    ::Sabora::Log::Debug(::Sabora::LogCategory::Editor, __VA_ARGS__)
    #define SB_EDITOR_INFO(...)     ::Sabora::Log::Info(::Sabora::LogCategory::Editor, __VA_ARGS__)
    #define SB_EDITOR_WARN(...)     ::Sabora::Log::Warn(::Sabora::LogCategory::Editor, __VA_ARGS__)
    #define SB_EDITOR_ERROR(...)    ::Sabora::Log::Error(::Sabora::LogCategory::Editor, __VA_ARGS__)
    #define SB_EDITOR_CRITICAL(...) ::Sabora::Log::Critical(::Sabora::LogCategory::Editor, __VA_ARGS__)

    #define SB_CLIENT_TRACE(...)    ::Sabora::Log::Trace(::Sabora::LogCategory::Client, __VA_ARGS__)
    #define SB_CLIENT_DEBUG(...)    ::Sabora::Log::Debug(::Sabora::LogCategory::Client, __VA_ARGS__)
    #define SB_CLIENT_INFO(...)     ::Sabora::Log::Info(::Sabora::LogCategory::Client, __VA_ARGS__)
    #define SB_CLIENT_WARN(...)     ::Sabora::Log::Warn(::Sabora::LogCategory::Client, __VA_ARGS__)
    #define SB_CLIENT_ERROR(...)    ::Sabora::Log::Error(::Sabora::LogCategory::Client, __VA_ARGS__)
    #define SB_CLIENT_CRITICAL(...) ::Sabora::Log::Critical(::Sabora::LogCategory::Client, __VA_ARGS__)

} // namespace Sabora
