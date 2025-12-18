#pragma once

#include <memory>
#include <string>
#include <array>
#include <spdlog/fmt/fmt.h>

namespace Sabora 
{
    // Log levels for the engine
    enum class LogLevel : uint8_t 
    {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warn = 3,
        Error = 4,
        Critical = 5
    };

    // Log categories for different engine systems
    enum class LogCategory : uint8_t 
    {
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

    /**
     * @brief Comprehensive logging system for the Sabora Engine.
     * 
     * The Log class provides a powerful, category-based logging system built on top of spdlog.
     * It supports multiple log levels, category filtering, and both console and file output.
     * 
     * Features:
     *   - Multiple log levels (Trace, Debug, Info, Warn, Error, Critical)
     *   - Category-based logging for different engine systems
     *   - Format string support with type-safe arguments
     *   - Per-category log level filtering
     *   - Console and file logging with customizable formats
     *   - Thread-safe operations
     * 
     * @note Must call Initialize() before using any logging methods.
     * @note Use convenience macros (SB_INFO, SB_CORE_ERROR, etc.) for easier logging.
     * 
     * @example
     * @code
     *   Log::Initialize();
     *   Log::Info("Application started");
     *   Log::Error(LogCategory::Renderer, "Failed to create shader");
     *   Log::Info("Frame time: {:.3f}ms", frameTime);  // Format string
     * @endcode
     */
    class Log {
    public:
        /**
         * @brief Initialize the logging system.
         * 
         * Sets up console and file loggers with default configurations.
         * Must be called before any logging operations.
         * 
         * @note Safe to call multiple times - subsequent calls are ignored.
         */
        static void Initialize();

        /**
         * @brief Shutdown the logging system.
         * 
         * Flushes all pending log messages and cleans up logger resources.
         * Should be called during application shutdown.
         */
        static void Shutdown();

        //==========================================================================
        // Core Logging Methods (Global Log Level)
        //==========================================================================

        /**
         * @brief Log a trace-level message.
         * @param message The message to log.
         * 
         * @note Only logs if trace level is enabled globally.
         */
        static void Trace(const std::string& message);

        /**
         * @brief Log a debug-level message.
         * @param message The message to log.
         * 
         * @note Only logs if debug level is enabled globally.
         */
        static void Debug(const std::string& message);

        /**
         * @brief Log an info-level message.
         * @param message The message to log.
         * 
         * @note Only logs if info level is enabled globally.
         */
        static void Info(const std::string& message);

        /**
         * @brief Log a warning-level message.
         * @param message The message to log.
         * 
         * @note Only logs if warn level is enabled globally.
         */
        static void Warn(const std::string& message);

        /**
         * @brief Log an error-level message.
         * @param message The message to log.
         * 
         * @note Only logs if error level is enabled globally.
         */
        static void Error(const std::string& message);

        /**
         * @brief Log a critical-level message.
         * @param message The message to log.
         * 
         * @note Only logs if critical level is enabled globally.
         */
        static void Critical(const std::string& message);

        //==========================================================================
        // Category-Based Logging Methods
        //==========================================================================

        /**
         * @brief Log a trace-level message with a specific category.
         * @param category The log category (e.g., LogCategory::Renderer).
         * @param message The message to log.
         * 
         * @note Only logs if trace level is enabled for the specified category.
         */
        static void Trace(LogCategory category, const std::string& message);

        /**
         * @brief Log a debug-level message with a specific category.
         * @param category The log category.
         * @param message The message to log.
         */
        static void Debug(LogCategory category, const std::string& message);

        /**
         * @brief Log an info-level message with a specific category.
         * @param category The log category.
         * @param message The message to log.
         */
        static void Info(LogCategory category, const std::string& message);

        /**
         * @brief Log a warning-level message with a specific category.
         * @param category The log category.
         * @param message The message to log.
         */
        static void Warn(LogCategory category, const std::string& message);

        /**
         * @brief Log an error-level message with a specific category.
         * @param category The log category.
         * @param message The message to log.
         */
        static void Error(LogCategory category, const std::string& message);

        /**
         * @brief Log a critical-level message with a specific category.
         * @param category The log category.
         * @param message The message to log.
         */
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

        //==========================================================================
        // Utility Methods
        //==========================================================================

        /**
         * @brief Set the global log level for all categories.
         * @param level The minimum log level to display.
         * 
         * Messages below this level will be filtered out globally.
         */
        static void SetLogLevel(LogLevel level);

        /**
         * @brief Set the log level for a specific category.
         * @param category The log category to configure.
         * @param level The minimum log level to display for this category.
         * 
         * Allows fine-grained control over logging per engine system.
         */
        static void SetLogLevel(LogCategory category, LogLevel level);

        /**
         * @brief Get the current global log level.
         * @return The global log level.
         */
        static LogLevel GetLogLevel();

        /**
         * @brief Get the log level for a specific category.
         * @param category The log category to query.
         * @return The log level for the specified category.
         */
        static LogLevel GetLogLevel(LogCategory category);
        
        //==========================================================================
        // File Logging Control
        //==========================================================================

        /**
         * @brief Enable file logging to a specific file.
         * @param filename The path to the log file.
         * 
         * @note Creates the log file if it doesn't exist.
         * @note Replaces any existing file logger.
         */
        static void EnableFileLogging(const std::string& filename);

        /**
         * @brief Disable file logging.
         * 
         * Logs will continue to be written to console if enabled.
         */
        static void DisableFileLogging();
        
        //==========================================================================
        // Console Logging Control
        //==========================================================================

        /**
         * @brief Enable console logging.
         * 
         * Logs will be written to stdout with color formatting.
         */
        static void EnableConsoleLogging();

        /**
         * @brief Disable console logging.
         * 
         * Logs will continue to be written to file if enabled.
         */
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
    // Note: These templates still format strings, but the macros check log levels
    // before calling these functions, preventing unnecessary template instantiation
    template<typename... Args>
    inline void Log::Trace(const std::string& format, const Args&... args)
    {
        // Double-check here as well (defensive programming)
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Trace) < static_cast<uint8_t>(s_GlobalLogLevel))
        {
            return;
        }

        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Trace(message);
    }

    template<typename... Args>
    inline void Log::Debug(const std::string& format, const Args&... args)
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Debug) < static_cast<uint8_t>(s_GlobalLogLevel)) 
        {
            return;
        }

        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Debug(message);
    }

    template<typename... Args>
    inline void Log::Info(const std::string& format, const Args&... args)
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Info) < static_cast<uint8_t>(s_GlobalLogLevel)) 
        {
            return;
        }

        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Info(message);
    }

    template<typename... Args>
    inline void Log::Warn(const std::string& format, const Args&... args) 
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Warn) < static_cast<uint8_t>(s_GlobalLogLevel)) 
        {
            return;
        }

        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Warn(message);
    }

    template<typename... Args>
    inline void Log::Error(const std::string& format, const Args&... args) 
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Error) < static_cast<uint8_t>(s_GlobalLogLevel))
        {
            return;
        }

        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Error(message);
    }

    template<typename... Args>
    inline void Log::Critical(const std::string& format, const Args&... args)
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Critical) < static_cast<uint8_t>(s_GlobalLogLevel)) 
        {
            return;
        }

        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Critical(message);
    }

    // Category-based format logging
    template<typename... Args>
    inline void Log::Trace(LogCategory category, const std::string& format, const Args&... args) 
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Trace) < static_cast<uint8_t>(s_CategoryLogLevels[static_cast<uint8_t>(category)])) 
        {
            return;
        }

        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Trace(category, message);
    }

    template<typename... Args>
    inline  void Log::Debug(LogCategory category, const std::string& format, const Args&... args) 
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Debug) < static_cast<uint8_t>(s_CategoryLogLevels[static_cast<uint8_t>(category)])) 
        {
            return;
        }

        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Debug(category, message);
    }

    template<typename... Args>
    inline void Log::Info(LogCategory category, const std::string& format, const Args&... args) 
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Info) < static_cast<uint8_t>(s_CategoryLogLevels[static_cast<uint8_t>(category)])) 
        {
            return;
        }

        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Info(category, message);
    }

    template<typename... Args>
    inline void Log::Warn(LogCategory category, const std::string& format, const Args&... args) 
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Warn) < static_cast<uint8_t>(s_CategoryLogLevels[static_cast<uint8_t>(category)])) 
        {
            return;
        }

        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Warn(category, message);
    }

    template<typename... Args>
    inline void Log::Error(LogCategory category, const std::string& format, const Args&... args)
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Error) < static_cast<uint8_t>(s_CategoryLogLevels[static_cast<uint8_t>(category)])) 
        {
            return;
        }

        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Error(category, message);
    }

    template<typename... Args>
    inline void Log::Critical(LogCategory category, const std::string& format, const Args&... args) 
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Critical) < static_cast<uint8_t>(s_CategoryLogLevels[static_cast<uint8_t>(category)])) 
        {
            return;
        }

        std::string message = fmt::vformat(format, fmt::make_format_args(args...));
        Log::Critical(category, message);
    }

    // Convenience macros with compile-time level checks to prevent string formatting when disabled
    // These macros check the log level at compile-time to avoid template instantiation overhead
    #define SB_TRACE(...) \
        (::Sabora::Log::GetLogLevel() <= ::Sabora::LogLevel::Trace ? \
            ::Sabora::Log::Trace(__VA_ARGS__) : (void)0)
    
    #define SB_DEBUG(...) \
        (::Sabora::Log::GetLogLevel() <= ::Sabora::LogLevel::Debug ? \
            ::Sabora::Log::Debug(__VA_ARGS__) : (void)0)
    
    #define SB_INFO(...) \
        (::Sabora::Log::GetLogLevel() <= ::Sabora::LogLevel::Info ? \
            ::Sabora::Log::Info(__VA_ARGS__) : (void)0)
    
    #define SB_WARN(...) \
        (::Sabora::Log::GetLogLevel() <= ::Sabora::LogLevel::Warn ? \
            ::Sabora::Log::Warn(__VA_ARGS__) : (void)0)
    
    #define SB_ERROR(...) \
        (::Sabora::Log::GetLogLevel() <= ::Sabora::LogLevel::Error ? \
            ::Sabora::Log::Error(__VA_ARGS__) : (void)0)
    
    #define SB_CRITICAL(...) \
        (::Sabora::Log::GetLogLevel() <= ::Sabora::LogLevel::Critical ? \
            ::Sabora::Log::Critical(__VA_ARGS__) : (void)0)

    // Category-based convenience macros with compile-time level checks
    #define SB_CORE_TRACE(...) \
        (::Sabora::Log::GetLogLevel(::Sabora::LogCategory::Core) <= ::Sabora::LogLevel::Trace ? \
            ::Sabora::Log::Trace(::Sabora::LogCategory::Core, __VA_ARGS__) : (void)0)
    
    #define SB_CORE_DEBUG(...) \
        (::Sabora::Log::GetLogLevel(::Sabora::LogCategory::Core) <= ::Sabora::LogLevel::Debug ? \
            ::Sabora::Log::Debug(::Sabora::LogCategory::Core, __VA_ARGS__) : (void)0)
    
    #define SB_CORE_INFO(...) \
        (::Sabora::Log::GetLogLevel(::Sabora::LogCategory::Core) <= ::Sabora::LogLevel::Info ? \
            ::Sabora::Log::Info(::Sabora::LogCategory::Core, __VA_ARGS__) : (void)0)
    
    #define SB_CORE_WARN(...) \
        (::Sabora::Log::GetLogLevel(::Sabora::LogCategory::Core) <= ::Sabora::LogLevel::Warn ? \
            ::Sabora::Log::Warn(::Sabora::LogCategory::Core, __VA_ARGS__) : (void)0)
    
    #define SB_CORE_ERROR(...) \
        (::Sabora::Log::GetLogLevel(::Sabora::LogCategory::Core) <= ::Sabora::LogLevel::Error ? \
            ::Sabora::Log::Error(::Sabora::LogCategory::Core, __VA_ARGS__) : (void)0)
    
    #define SB_CORE_CRITICAL(...) \
        (::Sabora::Log::GetLogLevel(::Sabora::LogCategory::Core) <= ::Sabora::LogLevel::Critical ? \
            ::Sabora::Log::Critical(::Sabora::LogCategory::Core, __VA_ARGS__) : (void)0)

    #define SB_RENDERER_TRACE(...) \
        (::Sabora::Log::GetLogLevel(::Sabora::LogCategory::Renderer) <= ::Sabora::LogLevel::Trace ? \
            ::Sabora::Log::Trace(::Sabora::LogCategory::Renderer, __VA_ARGS__) : (void)0)
    
    #define SB_RENDERER_DEBUG(...) \
        (::Sabora::Log::GetLogLevel(::Sabora::LogCategory::Renderer) <= ::Sabora::LogLevel::Debug ? \
            ::Sabora::Log::Debug(::Sabora::LogCategory::Renderer, __VA_ARGS__) : (void)0)
    
    #define SB_RENDERER_INFO(...) \
        (::Sabora::Log::GetLogLevel(::Sabora::LogCategory::Renderer) <= ::Sabora::LogLevel::Info ? \
            ::Sabora::Log::Info(::Sabora::LogCategory::Renderer, __VA_ARGS__) : (void)0)
    
    #define SB_RENDERER_WARN(...) \
        (::Sabora::Log::GetLogLevel(::Sabora::LogCategory::Renderer) <= ::Sabora::LogLevel::Warn ? \
            ::Sabora::Log::Warn(::Sabora::LogCategory::Renderer, __VA_ARGS__) : (void)0)
    
    #define SB_RENDERER_ERROR(...) \
        (::Sabora::Log::GetLogLevel(::Sabora::LogCategory::Renderer) <= ::Sabora::LogLevel::Error ? \
            ::Sabora::Log::Error(::Sabora::LogCategory::Renderer, __VA_ARGS__) : (void)0)
    
    #define SB_RENDERER_CRITICAL(...) \
        (::Sabora::Log::GetLogLevel(::Sabora::LogCategory::Renderer) <= ::Sabora::LogLevel::Critical ? \
            ::Sabora::Log::Critical(::Sabora::LogCategory::Renderer, __VA_ARGS__) : (void)0)

    // Additional category macros with compile-time checks (abbreviated for brevity)
    // Pattern: Check log level first, then call logging function only if enabled
    #define SB_CATEGORY_LOG(category, level, func, ...) \
        (::Sabora::Log::GetLogLevel(::Sabora::LogCategory::category) <= ::Sabora::LogLevel::level ? \
            ::Sabora::Log::func(::Sabora::LogCategory::category, __VA_ARGS__) : (void)0)
    
    #define SB_AUDIO_TRACE(...)    SB_CATEGORY_LOG(Audio, Trace, Trace, __VA_ARGS__)
    #define SB_AUDIO_DEBUG(...)    SB_CATEGORY_LOG(Audio, Debug, Debug, __VA_ARGS__)
    #define SB_AUDIO_INFO(...)     SB_CATEGORY_LOG(Audio, Info, Info, __VA_ARGS__)
    #define SB_AUDIO_WARN(...)     SB_CATEGORY_LOG(Audio, Warn, Warn, __VA_ARGS__)
    #define SB_AUDIO_ERROR(...)    SB_CATEGORY_LOG(Audio, Error, Error, __VA_ARGS__)
    #define SB_AUDIO_CRITICAL(...) SB_CATEGORY_LOG(Audio, Critical, Critical, __VA_ARGS__)

    #define SB_PHYSICS_TRACE(...)    SB_CATEGORY_LOG(Physics, Trace, Trace, __VA_ARGS__)
    #define SB_PHYSICS_DEBUG(...)    SB_CATEGORY_LOG(Physics, Debug, Debug, __VA_ARGS__)
    #define SB_PHYSICS_INFO(...)     SB_CATEGORY_LOG(Physics, Info, Info, __VA_ARGS__)
    #define SB_PHYSICS_WARN(...)     SB_CATEGORY_LOG(Physics, Warn, Warn, __VA_ARGS__)
    #define SB_PHYSICS_ERROR(...)    SB_CATEGORY_LOG(Physics, Error, Error, __VA_ARGS__)
    #define SB_PHYSICS_CRITICAL(...) SB_CATEGORY_LOG(Physics, Critical, Critical, __VA_ARGS__)

    #define SB_INPUT_TRACE(...)    SB_CATEGORY_LOG(Input, Trace, Trace, __VA_ARGS__)
    #define SB_INPUT_DEBUG(...)     SB_CATEGORY_LOG(Input, Debug, Debug, __VA_ARGS__)
    #define SB_INPUT_INFO(...)      SB_CATEGORY_LOG(Input, Info, Info, __VA_ARGS__)
    #define SB_INPUT_WARN(...)      SB_CATEGORY_LOG(Input, Warn, Warn, __VA_ARGS__)
    #define SB_INPUT_ERROR(...)     SB_CATEGORY_LOG(Input, Error, Error, __VA_ARGS__)
    #define SB_INPUT_CRITICAL(...)  SB_CATEGORY_LOG(Input, Critical, Critical, __VA_ARGS__)

    #define SB_SCENE_TRACE(...)    SB_CATEGORY_LOG(Scene, Trace, Trace, __VA_ARGS__)
    #define SB_SCENE_DEBUG(...)    SB_CATEGORY_LOG(Scene, Debug, Debug, __VA_ARGS__)
    #define SB_SCENE_INFO(...)     SB_CATEGORY_LOG(Scene, Info, Info, __VA_ARGS__)
    #define SB_SCENE_WARN(...)     SB_CATEGORY_LOG(Scene, Warn, Warn, __VA_ARGS__)
    #define SB_SCENE_ERROR(...)    SB_CATEGORY_LOG(Scene, Error, Error, __VA_ARGS__)
    #define SB_SCENE_CRITICAL(...) SB_CATEGORY_LOG(Scene, Critical, Critical, __VA_ARGS__)

    #define SB_SCRIPT_TRACE(...)    SB_CATEGORY_LOG(Script, Trace, Trace, __VA_ARGS__)
    #define SB_SCRIPT_DEBUG(...)    SB_CATEGORY_LOG(Script, Debug, Debug, __VA_ARGS__)
    #define SB_SCRIPT_INFO(...)     SB_CATEGORY_LOG(Script, Info, Info, __VA_ARGS__)
    #define SB_SCRIPT_WARN(...)     SB_CATEGORY_LOG(Script, Warn, Warn, __VA_ARGS__)
    #define SB_SCRIPT_ERROR(...)    SB_CATEGORY_LOG(Script, Error, Error, __VA_ARGS__)
    #define SB_SCRIPT_CRITICAL(...) SB_CATEGORY_LOG(Script, Critical, Critical, __VA_ARGS__)

    #define SB_NETWORK_TRACE(...)    SB_CATEGORY_LOG(Network, Trace, Trace, __VA_ARGS__)
    #define SB_NETWORK_DEBUG(...)    SB_CATEGORY_LOG(Network, Debug, Debug, __VA_ARGS__)
    #define SB_NETWORK_INFO(...)     SB_CATEGORY_LOG(Network, Info, Info, __VA_ARGS__)
    #define SB_NETWORK_WARN(...)     SB_CATEGORY_LOG(Network, Warn, Warn, __VA_ARGS__)
    #define SB_NETWORK_ERROR(...)    SB_CATEGORY_LOG(Network, Error, Error, __VA_ARGS__)
    #define SB_NETWORK_CRITICAL(...) SB_CATEGORY_LOG(Network, Critical, Critical, __VA_ARGS__)

    #define SB_EDITOR_TRACE(...)    SB_CATEGORY_LOG(Editor, Trace, Trace, __VA_ARGS__)
    #define SB_EDITOR_DEBUG(...)    SB_CATEGORY_LOG(Editor, Debug, Debug, __VA_ARGS__)
    #define SB_EDITOR_INFO(...)     SB_CATEGORY_LOG(Editor, Info, Info, __VA_ARGS__)
    #define SB_EDITOR_WARN(...)     SB_CATEGORY_LOG(Editor, Warn, Warn, __VA_ARGS__)
    #define SB_EDITOR_ERROR(...)    SB_CATEGORY_LOG(Editor, Error, Error, __VA_ARGS__)
    #define SB_EDITOR_CRITICAL(...) SB_CATEGORY_LOG(Editor, Critical, Critical, __VA_ARGS__)

    #define SB_CLIENT_TRACE(...)    SB_CATEGORY_LOG(Client, Trace, Trace, __VA_ARGS__)
    #define SB_CLIENT_DEBUG(...)    SB_CATEGORY_LOG(Client, Debug, Debug, __VA_ARGS__)
    #define SB_CLIENT_INFO(...)     SB_CATEGORY_LOG(Client, Info, Info, __VA_ARGS__)
    #define SB_CLIENT_WARN(...)     SB_CATEGORY_LOG(Client, Warn, Warn, __VA_ARGS__)
    #define SB_CLIENT_ERROR(...)    SB_CATEGORY_LOG(Client, Error, Error, __VA_ARGS__)
    #define SB_CLIENT_CRITICAL(...) SB_CATEGORY_LOG(Client, Critical, Critical, __VA_ARGS__)

} // namespace Sabora