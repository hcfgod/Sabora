#include "pch.h"
#include "Log.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/formatter.h>
#include <iostream>
#include <array>
#include <memory>

namespace Sabora 
{
    // Static member initialization
    bool Log::s_Initialized = false;
    LogLevel Log::s_GlobalLogLevel = LogLevel::Info;
    std::array<LogLevel, 10> Log::s_CategoryLogLevels = {};

    // spdlog logger instances
    static std::shared_ptr<spdlog::logger> s_ConsoleLogger;
    static std::shared_ptr<spdlog::logger> s_FileLogger;
    static std::shared_ptr<spdlog::logger> s_CombinedLogger;

    void Log::Initialize()
    {
        if (s_Initialized) 
        {
            return;
        }

        // Initialize category log levels to global level
        s_CategoryLogLevels.fill(s_GlobalLogLevel);

        try 
        {
            // Create console logger with colors
            s_ConsoleLogger = spdlog::stdout_color_mt("SaboraConsole");
            s_ConsoleLogger->set_level(spdlog::level::trace);
            
            // Set custom pattern for console output
            auto console_formatter = std::make_unique<spdlog::pattern_formatter>(
                "[%H:%M:%S.%e] [%^%l%$] [%t] %v"
            );
            s_ConsoleLogger->set_formatter(std::move(console_formatter));

            // Create file logger
            s_FileLogger = spdlog::basic_logger_mt("SaboraFile", "logs/sabora.log");
            s_FileLogger->set_level(spdlog::level::trace);
            
            // Set custom pattern for file output
            auto file_formatter = std::make_unique<spdlog::pattern_formatter>(
                "[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v"
            );
            s_FileLogger->set_formatter(std::move(file_formatter));

            // Create combined logger that writes to both console and file
            std::vector<spdlog::sink_ptr> sinks;
            sinks.push_back(s_ConsoleLogger->sinks()[0]);
            sinks.push_back(s_FileLogger->sinks()[0]);
            
            s_CombinedLogger = std::make_shared<spdlog::logger>("SaboraCombined", sinks.begin(), sinks.end());
            s_CombinedLogger->set_level(spdlog::level::trace);

            // Set as default logger
            spdlog::set_default_logger(s_CombinedLogger);

            // Log initialization
            s_CombinedLogger->info("Logging system initialized successfully");
            s_CombinedLogger->info("Global log level: {}", GetLevelString(s_GlobalLogLevel));
            
            s_Initialized = true;
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        }
    }

    void Log::Shutdown() 
    {
        if (!s_Initialized)
        {
            return;
        }

        if (s_CombinedLogger)
        {
            s_CombinedLogger->info("Shutting down logging system");
            s_CombinedLogger->flush();
        }

        spdlog::shutdown();
        s_Initialized = false;
    }

    // Core logging methods
    void Log::Trace(const std::string& message) 
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Trace) < static_cast<uint8_t>(s_GlobalLogLevel))
        {
            return;
        }
        s_CombinedLogger->trace(message);
    }

    void Log::Debug(const std::string& message)
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Debug) < static_cast<uint8_t>(s_GlobalLogLevel)) 
        {
            return;
        }
        s_CombinedLogger->debug(message);
    }

    void Log::Info(const std::string& message) 
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Info) < static_cast<uint8_t>(s_GlobalLogLevel)) 
        {
            return;
        }
        s_CombinedLogger->info(message);
    }

    void Log::Warn(const std::string& message) 
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Warn) < static_cast<uint8_t>(s_GlobalLogLevel)) 
        {
            return;
        }
        s_CombinedLogger->warn(message);
    }

    void Log::Error(const std::string& message) 
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Error) < static_cast<uint8_t>(s_GlobalLogLevel)) 
        {
            return;
        }
        s_CombinedLogger->error(message);
    }

    void Log::Critical(const std::string& message) 
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Critical) < static_cast<uint8_t>(s_GlobalLogLevel))
        {
            return;
        }
        s_CombinedLogger->critical(message);
    }

    // Category-based logging methods
    void Log::Trace(LogCategory category, const std::string& message) 
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Trace) < static_cast<uint8_t>(s_CategoryLogLevels[static_cast<uint8_t>(category)])) 
        {
            return;
        }
        std::string formattedMessage = FormatLogMessage(LogLevel::Trace, category, message);
        s_CombinedLogger->trace(formattedMessage);
    }

    void Log::Debug(LogCategory category, const std::string& message) 
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Debug) < static_cast<uint8_t>(s_CategoryLogLevels[static_cast<uint8_t>(category)])) 
        {
            return;
        }
        std::string formattedMessage = FormatLogMessage(LogLevel::Debug, category, message);
        s_CombinedLogger->debug(formattedMessage);
    }

    void Log::Info(LogCategory category, const std::string& message) 
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Info) < static_cast<uint8_t>(s_CategoryLogLevels[static_cast<uint8_t>(category)])) 
        {
            return;
        }
        std::string formattedMessage = FormatLogMessage(LogLevel::Info, category, message);
        s_CombinedLogger->info(formattedMessage);
    }

    void Log::Warn(LogCategory category, const std::string& message) 
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Warn) < static_cast<uint8_t>(s_CategoryLogLevels[static_cast<uint8_t>(category)])) 
        {
            return;
        }
        std::string formattedMessage = FormatLogMessage(LogLevel::Warn, category, message);
        s_CombinedLogger->warn(formattedMessage);
    }

    void Log::Error(LogCategory category, const std::string& message) 
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Error) < static_cast<uint8_t>(s_CategoryLogLevels[static_cast<uint8_t>(category)]))
        {
            return;
        }
        std::string formattedMessage = FormatLogMessage(LogLevel::Error, category, message);
        s_CombinedLogger->error(formattedMessage);
    }

    void Log::Critical(LogCategory category, const std::string& message)
    {
        if (!s_Initialized || static_cast<uint8_t>(LogLevel::Critical) < static_cast<uint8_t>(s_CategoryLogLevels[static_cast<uint8_t>(category)])) 
        {
            return;
        }
        std::string formattedMessage = FormatLogMessage(LogLevel::Critical, category, message);
        s_CombinedLogger->critical(formattedMessage);
    }

    // Utility methods
    void Log::SetLogLevel(LogLevel level) 
    {
        s_GlobalLogLevel = level;
        if (s_CombinedLogger) 
        {
            s_CombinedLogger->set_level(static_cast<spdlog::level::level_enum>(level));
        }
    }

    void Log::SetLogLevel(LogCategory category, LogLevel level)
    {
        s_CategoryLogLevels[static_cast<uint8_t>(category)] = level;
    }

    LogLevel Log::GetLogLevel()
    {
        return s_GlobalLogLevel;
    }

    LogLevel Log::GetLogLevel(LogCategory category) 
    {
        return s_CategoryLogLevels[static_cast<uint8_t>(category)];
    }

    void Log::EnableFileLogging(const std::string& filename)
    {
        if (!s_Initialized) 
        {
            return;
        }

        try 
        {
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename);
            s_FileLogger = std::make_shared<spdlog::logger>("SaboraFile", file_sink);
            s_FileLogger->set_level(spdlog::level::trace);
            
            // Update combined logger
            std::vector<spdlog::sink_ptr> sinks;
            sinks.push_back(s_ConsoleLogger->sinks()[0]);
            sinks.push_back(s_FileLogger->sinks()[0]);
            
            s_CombinedLogger = std::make_shared<spdlog::logger>("SaboraCombined", sinks.begin(), sinks.end());
            s_CombinedLogger->set_level(spdlog::level::trace);
            
            spdlog::set_default_logger(s_CombinedLogger);
            s_CombinedLogger->info("File logging enabled: {}", filename);
        }
        catch (const spdlog::spdlog_ex& ex) 
        {
            if (s_CombinedLogger) 
            {
                s_CombinedLogger->error("Failed to enable file logging: {}", ex.what());
            }
        }
    }

    void Log::DisableFileLogging()
    {
        if (!s_Initialized) 
        {
            return;
        }

        // Remove file sink from combined logger
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(s_ConsoleLogger->sinks()[0]);
        
        s_CombinedLogger = std::make_shared<spdlog::logger>("SaboraCombined", sinks.begin(), sinks.end());
        s_CombinedLogger->set_level(spdlog::level::trace);
        
        spdlog::set_default_logger(s_CombinedLogger);
        s_CombinedLogger->info("File logging disabled");
    }

    void Log::EnableConsoleLogging() 
    {
        if (!s_Initialized) 
        {
            return;
        }

        if (s_CombinedLogger) 
        {
            s_CombinedLogger->info("Console logging enabled");
        }
    }

    void Log::DisableConsoleLogging()
    {
        if (!s_Initialized) 
        {
            return;
        }

        // Remove console sink from combined logger
        std::vector<spdlog::sink_ptr> sinks;
        if (s_FileLogger) 
        {
            sinks.push_back(s_FileLogger->sinks()[0]);
        }
        
        s_CombinedLogger = std::make_shared<spdlog::logger>("SaboraCombined", sinks.begin(), sinks.end());
        s_CombinedLogger->set_level(spdlog::level::trace);
        
        spdlog::set_default_logger(s_CombinedLogger);
        s_CombinedLogger->info("Console logging disabled");
    }

    // Private helper methods
    std::string Log::GetCategoryString(LogCategory category)
    {
        switch (category)
        {
            case LogCategory::Core:     return "CORE";
            case LogCategory::Renderer: return "RENDERER";
            case LogCategory::Audio:    return "AUDIO";
            case LogCategory::Physics:  return "PHYSICS";
            case LogCategory::Input:    return "INPUT";
            case LogCategory::Scene:    return "SCENE";
            case LogCategory::Script:   return "SCRIPT";
            case LogCategory::Network:  return "NETWORK";
            case LogCategory::Editor:   return "EDITOR";
            case LogCategory::Client:   return "CLIENT";
            default:                    return "UNKNOWN";
        }
    }

    std::string Log::GetLevelString(LogLevel level) 
    {
        switch (level) 
        {
            case LogLevel::Trace:   return "TRACE";
            case LogLevel::Debug:   return "DEBUG";
            case LogLevel::Info:    return "INFO";
            case LogLevel::Warn:    return "WARN";
            case LogLevel::Error:   return "ERROR";
            case LogLevel::Critical: return "CRITICAL";
            default:                return "UNKNOWN";
        }
    }

    std::string Log::FormatLogMessage(LogLevel /*level*/, LogCategory category, const std::string& message)
    {
        return "[" + GetCategoryString(category) + "] " + message;
    }

} // namespace Sabora