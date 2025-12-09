#pragma once

/**
 * @file Result.h
 * @brief Professional error handling system for the Sabora Engine.
 * 
 * This module provides a comprehensive Result type for explicit error handling,
 * inspired by Rust's Result<T, E> and modern C++ error handling best practices.
 * 
 * Key Features:
 *   - Type-safe error handling without exceptions
 *   - Comprehensive error codes organized by category
 *   - Chainable operations with map(), andThen(), orElse()
 *   - Source location tracking for debugging
 *   - Integration with the logging system
 * 
 * Usage Examples:
 * @code
 *   // Returning a success value
 *   Result<int> ComputeValue() {
 *       return Result<int>::Success(42);
 *   }
 * 
 *   // Returning an error
 *   Result<FileHandle> OpenFile(const std::string& path) {
 *       if (!FileExists(path)) {
 *           return Result<FileHandle>::Failure(ErrorCode::FileNotFound, "File not found: " + path);
 *       }
 *       return Result<FileHandle>::Success(handle);
 *   }
 * 
 *   // Checking and using results
 *   auto result = OpenFile("config.json");
 *   if (result) {
 *       auto handle = result.Value();
 *       // Use handle...
 *   } else {
 *       SB_CORE_ERROR("Failed to open file: {}", result.GetError().Message());
 *   }
 * 
 *   // Chaining operations
 *   auto finalResult = OpenFile("data.bin")
 *       .AndThen([](FileHandle& h) { return ReadData(h); })
 *       .Map([](Data& d) { return ProcessData(d); });
 * @endcode
 */

#include <optional>
#include <string>
#include <variant>
#include <functional>
#include <type_traits>
#include <source_location>
#include <memory>
#include <spdlog/fmt/fmt.h>

namespace Sabora
{
    //==========================================================================
    // Error Categories - Broad classification of error types
    //==========================================================================

    /**
     * @brief High-level categories for grouping related error codes.
     * 
     * Error categories help identify which subsystem generated an error
     * and guide error handling strategies.
     */
    enum class ErrorCategory : uint16_t
    {
        None        = 0x0000,   ///< No error / success
        Core        = 0x0100,   ///< Core engine errors
        Platform    = 0x0200,   ///< Platform/OS level errors
        Graphics    = 0x0300,   ///< Rendering and graphics errors
        Audio       = 0x0400,   ///< Audio system errors
        Input       = 0x0500,   ///< Input handling errors
        FileSystem  = 0x0600,   ///< File and IO errors
        Network     = 0x0700,   ///< Networking errors
        Scripting   = 0x0800,   ///< Scripting engine errors
        Physics     = 0x0900,   ///< Physics simulation errors
        Memory      = 0x0A00,   ///< Memory allocation errors
        Validation  = 0x0B00,   ///< Validation and assertion errors
        External    = 0x0C00,   ///< Third-party library errors
    };

    //==========================================================================
    // Error Codes - Specific error identifiers
    //==========================================================================

    /**
     * @brief Comprehensive enumeration of all possible error codes in the engine.
     * 
     * Error codes are organized by category using the upper byte, allowing
     * easy identification of the error source. The lower byte provides
     * specific error identification within each category.
     */
    enum class ErrorCode : uint16_t
    {
        // Success (0x0000)
        Success                         = 0x0000,

        // Core Errors (0x01XX)
        CoreUnknown                     = 0x0100,
        CoreInitializationFailed        = 0x0101,
        CoreShutdownFailed              = 0x0102,
        CoreInvalidState                = 0x0103,
        CoreInvalidArgument             = 0x0104,
        CoreNullPointer                 = 0x0105,
        CoreOutOfRange                  = 0x0106,
        CoreNotImplemented              = 0x0107,
        CoreTimeout                     = 0x0108,
        CoreOperationCancelled          = 0x0109,
        CoreAlreadyExists               = 0x010A,
        CoreNotFound                    = 0x010B,
        CorePermissionDenied            = 0x010C,
        CoreResourceBusy                = 0x010D,
        CoreDependencyMissing           = 0x010E,

        // Platform Errors (0x02XX)
        PlatformUnknown                 = 0x0200,
        PlatformInitializationFailed    = 0x0201,
        PlatformWindowCreationFailed    = 0x0202,
        PlatformContextCreationFailed   = 0x0203,
        PlatformDisplayNotFound         = 0x0204,
        PlatformUnsupportedFeature      = 0x0205,
        PlatformDriverError             = 0x0206,
        PlatformSDLError                = 0x0207,

        // Graphics Errors (0x03XX)
        GraphicsUnknown                 = 0x0300,
        GraphicsDeviceCreationFailed    = 0x0301,
        GraphicsSwapchainCreationFailed = 0x0302,
        GraphicsShaderCompilationFailed = 0x0303,
        GraphicsPipelineCreationFailed  = 0x0304,
        GraphicsBufferCreationFailed    = 0x0305,
        GraphicsTextureCreationFailed   = 0x0306,
        GraphicsOutOfMemory             = 0x0307,
        GraphicsInvalidFormat           = 0x0308,
        GraphicsRenderPassFailed        = 0x0309,
        GraphicsCommandBufferError      = 0x030A,

        // Audio Errors (0x04XX)
        AudioUnknown                    = 0x0400,
        AudioDeviceCreationFailed       = 0x0401,
        AudioStreamCreationFailed       = 0x0402,
        AudioCodecNotSupported          = 0x0403,
        AudioBufferUnderrun             = 0x0404,
        AudioBufferOverrun              = 0x0405,
        AudioInvalidSampleRate          = 0x0406,
        AudioInvalidChannelCount        = 0x0407,

        // Input Errors (0x05XX)
        InputUnknown                    = 0x0500,
        InputDeviceNotFound             = 0x0501,
        InputDeviceDisconnected         = 0x0502,
        InputInvalidBinding             = 0x0503,
        InputMappingNotFound            = 0x0504,

        // FileSystem Errors (0x06XX)
        FileSystemUnknown               = 0x0600,
        FileNotFound                    = 0x0601,
        FileAccessDenied                = 0x0602,
        FileAlreadyExists               = 0x0603,
        FileReadError                   = 0x0604,
        FileWriteError                  = 0x0605,
        FileInvalidPath                 = 0x0606,
        FileCorrupted                   = 0x0607,
        FileTooLarge                    = 0x0608,
        FileInvalidFormat               = 0x0609,
        DirectoryNotFound               = 0x060A,
        DirectoryNotEmpty               = 0x060B,

        // Network Errors (0x07XX)
        NetworkUnknown                  = 0x0700,
        NetworkConnectionFailed         = 0x0701,
        NetworkConnectionRefused        = 0x0702,
        NetworkConnectionTimeout        = 0x0703,
        NetworkConnectionReset          = 0x0704,
        NetworkHostNotFound             = 0x0705,
        NetworkProtocolError            = 0x0706,
        NetworkSendFailed               = 0x0707,
        NetworkReceiveFailed            = 0x0708,
        NetworkSSLError                 = 0x0709,

        // Scripting Errors (0x08XX)
        ScriptingUnknown                = 0x0800,
        ScriptingSyntaxError            = 0x0801,
        ScriptingRuntimeError           = 0x0802,
        ScriptingTypeError              = 0x0803,
        ScriptingCompilationFailed      = 0x0804,
        ScriptingModuleNotFound         = 0x0805,
        ScriptingStackOverflow          = 0x0806,

        // Physics Errors (0x09XX)
        PhysicsUnknown                  = 0x0900,
        PhysicsWorldCreationFailed      = 0x0901,
        PhysicsBodyCreationFailed       = 0x0902,
        PhysicsShapeCreationFailed      = 0x0903,
        PhysicsConstraintError          = 0x0904,
        PhysicsSimulationError          = 0x0905,

        // Memory Errors (0x0AXX)
        MemoryUnknown                   = 0x0A00,
        MemoryAllocationFailed          = 0x0A01,
        MemoryDeallocationFailed        = 0x0A02,
        MemoryCorruption                = 0x0A03,
        MemoryLeak                      = 0x0A04,
        MemoryAlignmentError            = 0x0A05,
        MemoryOutOfBounds               = 0x0A06,

        // Validation Errors (0x0BXX)
        ValidationUnknown               = 0x0B00,
        ValidationFailed                = 0x0B01,
        ValidationPreconditionFailed    = 0x0B02,
        ValidationPostconditionFailed   = 0x0B03,
        ValidationInvariantViolated     = 0x0B04,
        ValidationSchemaError           = 0x0B05,

        // External Library Errors (0x0CXX)
        ExternalUnknown                 = 0x0C00,
        ExternalLibraryNotLoaded        = 0x0C01,
        ExternalSymbolNotFound          = 0x0C02,
        ExternalVersionMismatch         = 0x0C03,
        ExternalAPIError                = 0x0C04,
    };

    //==========================================================================
    // Error Utilities
    //==========================================================================

    /**
     * @brief Get the category of an error code.
     * @param code The error code to categorize.
     * @return The category of the error code.
     */
    constexpr ErrorCategory GetErrorCategory(ErrorCode code) noexcept
    {
        return static_cast<ErrorCategory>(static_cast<uint16_t>(code) & 0xFF00);
    }

    /**
     * @brief Check if an error code represents success.
     * @param code The error code to check.
     * @return True if the code represents success.
     */
    constexpr bool IsSuccess(ErrorCode code) noexcept
    {
        return code == ErrorCode::Success;
    }

    /**
     * @brief Get a human-readable string for an error category.
     * @param category The error category.
     * @return String representation of the category.
     */
    inline const char* GetCategoryName(ErrorCategory category) noexcept
    {
        switch (category)
        {
            case ErrorCategory::None:       return "None";
            case ErrorCategory::Core:       return "Core";
            case ErrorCategory::Platform:   return "Platform";
            case ErrorCategory::Graphics:   return "Graphics";
            case ErrorCategory::Audio:      return "Audio";
            case ErrorCategory::Input:      return "Input";
            case ErrorCategory::FileSystem: return "FileSystem";
            case ErrorCategory::Network:    return "Network";
            case ErrorCategory::Scripting:  return "Scripting";
            case ErrorCategory::Physics:    return "Physics";
            case ErrorCategory::Memory:     return "Memory";
            case ErrorCategory::Validation: return "Validation";
            case ErrorCategory::External:   return "External";
            default:                        return "Unknown";
        }
    }

    /**
     * @brief Get a human-readable string for an error code.
     * @param code The error code.
     * @return String representation of the error code.
     */
    inline const char* GetErrorCodeName(ErrorCode code) noexcept
    {
        switch (code)
        {
            // Success
            case ErrorCode::Success:                        return "Success";

            // Core
            case ErrorCode::CoreUnknown:                    return "CoreUnknown";
            case ErrorCode::CoreInitializationFailed:       return "CoreInitializationFailed";
            case ErrorCode::CoreShutdownFailed:             return "CoreShutdownFailed";
            case ErrorCode::CoreInvalidState:               return "CoreInvalidState";
            case ErrorCode::CoreInvalidArgument:            return "CoreInvalidArgument";
            case ErrorCode::CoreNullPointer:                return "CoreNullPointer";
            case ErrorCode::CoreOutOfRange:                 return "CoreOutOfRange";
            case ErrorCode::CoreNotImplemented:             return "CoreNotImplemented";
            case ErrorCode::CoreTimeout:                    return "CoreTimeout";
            case ErrorCode::CoreOperationCancelled:         return "CoreOperationCancelled";
            case ErrorCode::CoreAlreadyExists:              return "CoreAlreadyExists";
            case ErrorCode::CoreNotFound:                   return "CoreNotFound";
            case ErrorCode::CorePermissionDenied:           return "CorePermissionDenied";
            case ErrorCode::CoreResourceBusy:               return "CoreResourceBusy";
            case ErrorCode::CoreDependencyMissing:          return "CoreDependencyMissing";

            // Platform
            case ErrorCode::PlatformUnknown:                return "PlatformUnknown";
            case ErrorCode::PlatformInitializationFailed:   return "PlatformInitializationFailed";
            case ErrorCode::PlatformWindowCreationFailed:   return "PlatformWindowCreationFailed";
            case ErrorCode::PlatformContextCreationFailed:  return "PlatformContextCreationFailed";
            case ErrorCode::PlatformDisplayNotFound:        return "PlatformDisplayNotFound";
            case ErrorCode::PlatformUnsupportedFeature:     return "PlatformUnsupportedFeature";
            case ErrorCode::PlatformDriverError:            return "PlatformDriverError";
            case ErrorCode::PlatformSDLError:               return "PlatformSDLError";

            // Graphics
            case ErrorCode::GraphicsUnknown:                return "GraphicsUnknown";
            case ErrorCode::GraphicsDeviceCreationFailed:   return "GraphicsDeviceCreationFailed";
            case ErrorCode::GraphicsSwapchainCreationFailed: return "GraphicsSwapchainCreationFailed";
            case ErrorCode::GraphicsShaderCompilationFailed: return "GraphicsShaderCompilationFailed";
            case ErrorCode::GraphicsPipelineCreationFailed: return "GraphicsPipelineCreationFailed";
            case ErrorCode::GraphicsBufferCreationFailed:   return "GraphicsBufferCreationFailed";
            case ErrorCode::GraphicsTextureCreationFailed:  return "GraphicsTextureCreationFailed";
            case ErrorCode::GraphicsOutOfMemory:            return "GraphicsOutOfMemory";
            case ErrorCode::GraphicsInvalidFormat:          return "GraphicsInvalidFormat";
            case ErrorCode::GraphicsRenderPassFailed:       return "GraphicsRenderPassFailed";
            case ErrorCode::GraphicsCommandBufferError:     return "GraphicsCommandBufferError";

            // Audio
            case ErrorCode::AudioUnknown:                   return "AudioUnknown";
            case ErrorCode::AudioDeviceCreationFailed:      return "AudioDeviceCreationFailed";
            case ErrorCode::AudioStreamCreationFailed:      return "AudioStreamCreationFailed";
            case ErrorCode::AudioCodecNotSupported:         return "AudioCodecNotSupported";
            case ErrorCode::AudioBufferUnderrun:            return "AudioBufferUnderrun";
            case ErrorCode::AudioBufferOverrun:             return "AudioBufferOverrun";
            case ErrorCode::AudioInvalidSampleRate:         return "AudioInvalidSampleRate";
            case ErrorCode::AudioInvalidChannelCount:       return "AudioInvalidChannelCount";

            // Input
            case ErrorCode::InputUnknown:                   return "InputUnknown";
            case ErrorCode::InputDeviceNotFound:            return "InputDeviceNotFound";
            case ErrorCode::InputDeviceDisconnected:        return "InputDeviceDisconnected";
            case ErrorCode::InputInvalidBinding:            return "InputInvalidBinding";
            case ErrorCode::InputMappingNotFound:           return "InputMappingNotFound";

            // FileSystem
            case ErrorCode::FileSystemUnknown:              return "FileSystemUnknown";
            case ErrorCode::FileNotFound:                   return "FileNotFound";
            case ErrorCode::FileAccessDenied:               return "FileAccessDenied";
            case ErrorCode::FileAlreadyExists:              return "FileAlreadyExists";
            case ErrorCode::FileReadError:                  return "FileReadError";
            case ErrorCode::FileWriteError:                 return "FileWriteError";
            case ErrorCode::FileInvalidPath:                return "FileInvalidPath";
            case ErrorCode::FileCorrupted:                  return "FileCorrupted";
            case ErrorCode::FileTooLarge:                   return "FileTooLarge";
            case ErrorCode::FileInvalidFormat:              return "FileInvalidFormat";
            case ErrorCode::DirectoryNotFound:              return "DirectoryNotFound";
            case ErrorCode::DirectoryNotEmpty:              return "DirectoryNotEmpty";

            // Network
            case ErrorCode::NetworkUnknown:                 return "NetworkUnknown";
            case ErrorCode::NetworkConnectionFailed:        return "NetworkConnectionFailed";
            case ErrorCode::NetworkConnectionRefused:       return "NetworkConnectionRefused";
            case ErrorCode::NetworkConnectionTimeout:       return "NetworkConnectionTimeout";
            case ErrorCode::NetworkConnectionReset:         return "NetworkConnectionReset";
            case ErrorCode::NetworkHostNotFound:            return "NetworkHostNotFound";
            case ErrorCode::NetworkProtocolError:           return "NetworkProtocolError";
            case ErrorCode::NetworkSendFailed:              return "NetworkSendFailed";
            case ErrorCode::NetworkReceiveFailed:           return "NetworkReceiveFailed";
            case ErrorCode::NetworkSSLError:                return "NetworkSSLError";

            // Scripting
            case ErrorCode::ScriptingUnknown:               return "ScriptingUnknown";
            case ErrorCode::ScriptingSyntaxError:           return "ScriptingSyntaxError";
            case ErrorCode::ScriptingRuntimeError:          return "ScriptingRuntimeError";
            case ErrorCode::ScriptingTypeError:             return "ScriptingTypeError";
            case ErrorCode::ScriptingCompilationFailed:     return "ScriptingCompilationFailed";
            case ErrorCode::ScriptingModuleNotFound:        return "ScriptingModuleNotFound";
            case ErrorCode::ScriptingStackOverflow:         return "ScriptingStackOverflow";

            // Physics
            case ErrorCode::PhysicsUnknown:                 return "PhysicsUnknown";
            case ErrorCode::PhysicsWorldCreationFailed:     return "PhysicsWorldCreationFailed";
            case ErrorCode::PhysicsBodyCreationFailed:      return "PhysicsBodyCreationFailed";
            case ErrorCode::PhysicsShapeCreationFailed:     return "PhysicsShapeCreationFailed";
            case ErrorCode::PhysicsConstraintError:         return "PhysicsConstraintError";
            case ErrorCode::PhysicsSimulationError:         return "PhysicsSimulationError";

            // Memory
            case ErrorCode::MemoryUnknown:                  return "MemoryUnknown";
            case ErrorCode::MemoryAllocationFailed:         return "MemoryAllocationFailed";
            case ErrorCode::MemoryDeallocationFailed:       return "MemoryDeallocationFailed";
            case ErrorCode::MemoryCorruption:               return "MemoryCorruption";
            case ErrorCode::MemoryLeak:                     return "MemoryLeak";
            case ErrorCode::MemoryAlignmentError:           return "MemoryAlignmentError";
            case ErrorCode::MemoryOutOfBounds:              return "MemoryOutOfBounds";

            // Validation
            case ErrorCode::ValidationUnknown:              return "ValidationUnknown";
            case ErrorCode::ValidationFailed:               return "ValidationFailed";
            case ErrorCode::ValidationPreconditionFailed:   return "ValidationPreconditionFailed";
            case ErrorCode::ValidationPostconditionFailed:  return "ValidationPostconditionFailed";
            case ErrorCode::ValidationInvariantViolated:    return "ValidationInvariantViolated";
            case ErrorCode::ValidationSchemaError:          return "ValidationSchemaError";

            // External
            case ErrorCode::ExternalUnknown:                return "ExternalUnknown";
            case ErrorCode::ExternalLibraryNotLoaded:       return "ExternalLibraryNotLoaded";
            case ErrorCode::ExternalSymbolNotFound:         return "ExternalSymbolNotFound";
            case ErrorCode::ExternalVersionMismatch:        return "ExternalVersionMismatch";
            case ErrorCode::ExternalAPIError:               return "ExternalAPIError";

            default:                                        return "UnknownError";
        }
    }

    //==========================================================================
    // Error Class - Detailed error information
    //==========================================================================

    /**
     * @brief Represents a detailed error with code, message, and source location.
     * 
     * The Error class provides comprehensive error information including:
     *   - Error code for programmatic handling
     *   - Human-readable message for logging/display
     *   - Source location for debugging
     *   - Optional nested/inner error for error chains
     */
    class Error
    {
    public:
        /**
         * @brief Construct an error with code, message, and optional source location.
         * @param code The error code.
         * @param message Human-readable error message.
         * @param location Source location where the error occurred.
         */
        explicit Error(
            ErrorCode code,
            std::string message = "",
            std::source_location location = std::source_location::current()
        ) noexcept
            : m_Code(code)
            , m_Message(std::move(message))
            , m_Location(location)
        {
        }

        /**
         * @brief Construct an error with a nested inner error.
         * @param code The error code.
         * @param message Human-readable error message.
         * @param innerError The underlying error that caused this error.
         * @param location Source location where the error occurred.
         */
        Error(
            ErrorCode code,
            std::string message,
            std::shared_ptr<Error> innerError,
            std::source_location location = std::source_location::current()
        ) noexcept
            : m_Code(code)
            , m_Message(std::move(message))
            , m_InnerError(std::move(innerError))
            , m_Location(location)
        {
        }

        // Accessors
        [[nodiscard]] ErrorCode Code() const noexcept { return m_Code; }
        [[nodiscard]] ErrorCategory Category() const noexcept { return GetErrorCategory(m_Code); }
        [[nodiscard]] const std::string& Message() const noexcept { return m_Message; }
        [[nodiscard]] const std::source_location& Location() const noexcept { return m_Location; }
        [[nodiscard]] const std::shared_ptr<Error>& InnerError() const noexcept { return m_InnerError; }

        // String representations
        [[nodiscard]] const char* CodeName() const noexcept { return GetErrorCodeName(m_Code); }
        [[nodiscard]] const char* CategoryName() const noexcept { return GetCategoryName(Category()); }

        /**
         * @brief Get a fully formatted error string for logging.
         * @return Formatted string with all error details.
         */
        [[nodiscard]] std::string ToString() const
        {
            std::string result = fmt::format(
                "[{}::{}] {} (at {}:{})",
                CategoryName(),
                CodeName(),
                m_Message.empty() ? "(no message)" : m_Message,
                m_Location.file_name(),
                m_Location.line()
            );

            if (m_InnerError)
            {
                result += "\n  Caused by: " + m_InnerError->ToString();
            }

            return result;
        }

        /**
         * @brief Check if this error has an inner/nested error.
         * @return True if there is an inner error.
         */
        [[nodiscard]] bool HasInnerError() const noexcept { return m_InnerError != nullptr; }

        /**
         * @brief Wrap this error as an inner error of a new error.
         * @param code The new error code.
         * @param message The new error message.
         * @return A new Error with this error as the inner error.
         */
        [[nodiscard]] Error Wrap(
            ErrorCode code,
            std::string message,
            std::source_location location = std::source_location::current()
        ) const
        {
            return Error(code, std::move(message), std::make_shared<Error>(*this), location);
        }

    private:
        ErrorCode m_Code;
        std::string m_Message;
        std::shared_ptr<Error> m_InnerError;
        std::source_location m_Location;
    };

    //==========================================================================
    // Result Class - Type-safe error handling
    //==========================================================================

    /**
     * @brief A type-safe result type that holds either a value or an error.
     * 
     * Result<T> provides a clean way to handle operations that can fail without
     * using exceptions. It enforces explicit error handling and provides
     * functional-style operations for chaining.
     * 
     * @tparam T The type of the success value. Use void for operations with no return value.
     */
    template<typename T>
    class Result
    {
    public:
        using ValueType = T;

        // Constructors for success
        explicit Result(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
            : m_Data(std::move(value))
        {
        }

        // Constructors for failure
        explicit Result(Error error) noexcept
            : m_Data(std::move(error))
        {
        }

        // Factory methods for clearer intent
        /**
         * @brief Create a successful result with a value.
         * @param value The success value.
         * @return A Result containing the value.
         */
        [[nodiscard]] static Result Success(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
        {
            return Result(std::move(value));
        }

        /**
         * @brief Create a failed result with an error.
         * @param code The error code.
         * @param message Optional error message.
         * @param location Source location (auto-captured).
         * @return A Result containing the error.
         */
        [[nodiscard]] static Result Failure(
            ErrorCode code,
            std::string message = "",
            std::source_location location = std::source_location::current()
        )
        {
            return Result(Error(code, std::move(message), location));
        }

        /**
         * @brief Create a failed result from an existing Error object.
         * @param error The error object.
         * @return A Result containing the error.
         */
        [[nodiscard]] static Result Failure(Error error) noexcept
        {
            return Result(std::move(error));
        }

        // State queries
        [[nodiscard]] bool IsSuccess() const noexcept { return std::holds_alternative<T>(m_Data); }
        [[nodiscard]] bool IsFailure() const noexcept { return std::holds_alternative<Error>(m_Data); }
        [[nodiscard]] explicit operator bool() const noexcept { return IsSuccess(); }

        // Value access
        /**
         * @brief Get the success value. Undefined behavior if result is a failure.
         * @return Reference to the value.
         */
        [[nodiscard]] T& Value() & { return std::get<T>(m_Data); }
        [[nodiscard]] const T& Value() const& { return std::get<T>(m_Data); }
        [[nodiscard]] T&& Value() && { return std::get<T>(std::move(m_Data)); }

        /**
         * @brief Get the success value or a default if failure.
         * @param defaultValue Value to return if this is a failure.
         * @return The success value or the default.
         */
        [[nodiscard]] T ValueOr(T defaultValue) const& noexcept(std::is_nothrow_copy_constructible_v<T>)
        {
            return IsSuccess() ? Value() : std::move(defaultValue);
        }

        [[nodiscard]] T ValueOr(T defaultValue) && noexcept(std::is_nothrow_move_constructible_v<T>)
        {
            return IsSuccess() ? std::move(*this).Value() : std::move(defaultValue);
        }

        // Error access
        /**
         * @brief Get the error. Undefined behavior if result is a success.
         * @return Reference to the error.
         */
        [[nodiscard]] Error& GetError() & { return std::get<Error>(m_Data); }
        [[nodiscard]] const Error& GetError() const& { return std::get<Error>(m_Data); }
        [[nodiscard]] Error&& GetError() && { return std::get<Error>(std::move(m_Data)); }

        // Functional operations

        /**
         * @brief Transform the value if success, propagate error if failure.
         * @tparam Func Function type that takes T and returns U.
         * @param func The transformation function.
         * @return Result<U> with transformed value or propagated error.
         */
        template<typename Func>
        [[nodiscard]] auto Map(Func&& func) const& -> Result<std::invoke_result_t<Func, const T&>>
        {
            using U = std::invoke_result_t<Func, const T&>;
            if (IsSuccess())
            {
                return Result<U>::Success(std::invoke(std::forward<Func>(func), Value()));
            }
            return Result<U>::Failure(GetError());
        }

        template<typename Func>
        [[nodiscard]] auto Map(Func&& func) && -> Result<std::invoke_result_t<Func, T&&>>
        {
            using U = std::invoke_result_t<Func, T&&>;
            if (IsSuccess())
            {
                return Result<U>::Success(std::invoke(std::forward<Func>(func), std::move(*this).Value()));
            }
            return Result<U>::Failure(std::move(*this).GetError());
        }

        /**
         * @brief Chain another operation that returns a Result.
         * @tparam Func Function type that takes T and returns Result<U>.
         * @param func The chained operation.
         * @return Result<U> from func or propagated error.
         */
        template<typename Func>
        [[nodiscard]] auto AndThen(Func&& func) const& -> std::invoke_result_t<Func, const T&>
        {
            using ResultU = std::invoke_result_t<Func, const T&>;
            if (IsSuccess())
            {
                return std::invoke(std::forward<Func>(func), Value());
            }
            return ResultU::Failure(GetError());
        }

        template<typename Func>
        [[nodiscard]] auto AndThen(Func&& func) && -> std::invoke_result_t<Func, T&&>
        {
            using ResultU = std::invoke_result_t<Func, T&&>;
            if (IsSuccess())
            {
                return std::invoke(std::forward<Func>(func), std::move(*this).Value());
            }
            return ResultU::Failure(std::move(*this).GetError());
        }

        /**
         * @brief Provide an alternative value or recovery on failure.
         * @tparam Func Function type that takes Error and returns Result<T>.
         * @param func The recovery function.
         * @return This result if success, or result from func if failure.
         */
        template<typename Func>
        [[nodiscard]] Result OrElse(Func&& func) const&
        {
            if (IsSuccess())
            {
                return *this;
            }
            return std::invoke(std::forward<Func>(func), GetError());
        }

        template<typename Func>
        [[nodiscard]] Result OrElse(Func&& func) &&
        {
            if (IsSuccess())
            {
                return std::move(*this);
            }
            return std::invoke(std::forward<Func>(func), std::move(*this).GetError());
        }

        /**
         * @brief Execute a function on success without changing the result.
         * @param func Function to execute on success.
         * @return Reference to this result.
         */
        template<typename Func>
        const Result& OnSuccess(Func&& func) const&
        {
            if (IsSuccess())
            {
                std::invoke(std::forward<Func>(func), Value());
            }
            return *this;
        }

        /**
         * @brief Execute a function on failure without changing the result.
         * @param func Function to execute on failure.
         * @return Reference to this result.
         */
        template<typename Func>
        const Result& OnFailure(Func&& func) const&
        {
            if (IsFailure())
            {
                std::invoke(std::forward<Func>(func), GetError());
            }
            return *this;
        }

    private:
        std::variant<T, Error> m_Data;
    };

    //==========================================================================
    // Result<void> Specialization - For operations with no return value
    //==========================================================================

    /**
     * @brief Specialization of Result for void return types.
     * 
     * Used for operations that can fail but don't return a value on success.
     */
    template<>
    class Result<void>
    {
    public:
        using ValueType = void;

        // Default constructor creates success
        Result() noexcept : m_Error(std::nullopt) {}

        // Constructor for failure
        explicit Result(Error error) noexcept : m_Error(std::move(error)) {}

        // Factory methods
        [[nodiscard]] static Result Success() noexcept { return Result(); }

        [[nodiscard]] static Result Failure(
            ErrorCode code,
            std::string message = "",
            std::source_location location = std::source_location::current()
        )
        {
            return Result(Error(code, std::move(message), location));
        }

        [[nodiscard]] static Result Failure(Error error) noexcept
        {
            return Result(std::move(error));
        }

        // State queries
        [[nodiscard]] bool IsSuccess() const noexcept { return !m_Error.has_value(); }
        [[nodiscard]] bool IsFailure() const noexcept { return m_Error.has_value(); }
        [[nodiscard]] explicit operator bool() const noexcept { return IsSuccess(); }

        // Error access
        [[nodiscard]] Error& GetError() & { return m_Error.value(); }
        [[nodiscard]] const Error& GetError() const& { return m_Error.value(); }
        [[nodiscard]] Error&& GetError() && { return std::move(m_Error.value()); }

        // Functional operations

        /**
         * @brief Chain another void operation.
         */
        template<typename Func>
        [[nodiscard]] auto AndThen(Func&& func) const -> std::invoke_result_t<Func>
        {
            using ResultU = std::invoke_result_t<Func>;
            if (IsSuccess())
            {
                return std::invoke(std::forward<Func>(func));
            }
            return ResultU::Failure(GetError());
        }

        /**
         * @brief Provide recovery on failure.
         */
        template<typename Func>
        [[nodiscard]] Result OrElse(Func&& func) const&
        {
            if (IsSuccess())
            {
                return *this;
            }
            return std::invoke(std::forward<Func>(func), GetError());
        }

        /**
         * @brief Execute a function on success.
         */
        template<typename Func>
        const Result& OnSuccess(Func&& func) const&
        {
            if (IsSuccess())
            {
                std::invoke(std::forward<Func>(func));
            }
            return *this;
        }

        /**
         * @brief Execute a function on failure.
         */
        template<typename Func>
        const Result& OnFailure(Func&& func) const&
        {
            if (IsFailure())
            {
                std::invoke(std::forward<Func>(func), GetError());
            }
            return *this;
        }

    private:
        std::optional<Error> m_Error;
    };

    //==========================================================================
    // Convenience Macros
    //==========================================================================

    /**
     * @brief Macro to propagate errors early (similar to Rust's ? operator).
     * 
     * Usage:
     * @code
     *   Result<int> DoSomething() {
     *       auto value = SB_TRY(GetValue());  // Returns early if GetValue() fails
     *       return Result<int>::Success(value * 2);
     *   }
     * @endcode
     */
    #define SB_TRY(expression)                                                          \
        ({                                                                              \
            auto _result = (expression);                                                \
            if (_result.IsFailure()) {                                                  \
                return decltype(_result)::Failure(std::move(_result).GetError());       \
            }                                                                           \
            std::move(_result).Value();                                                 \
        })

    /**
     * @brief Macro to propagate void results early.
     */
    #define SB_TRY_VOID(expression)                                                     \
        do {                                                                            \
            auto _result = (expression);                                                \
            if (_result.IsFailure()) {                                                  \
                return ::Sabora::Result<void>::Failure(std::move(_result).GetError());  \
            }                                                                           \
        } while (false)

    /**
     * @brief Create a success result with automatic type deduction.
     */
    #define SB_SUCCESS(value) ::Sabora::Result<decltype(value)>::Success(value)

    /**
     * @brief Create a failure result with automatic source location capture.
     */
    #define SB_FAILURE(code, message)                                                   \
        ::Sabora::Result<void>::Failure(code, message, std::source_location::current())

    /**
     * @brief Assert a condition and return a failure if it doesn't hold.
     */
    #define SB_ENSURE(condition, code, message)                                         \
        do {                                                                            \
            if (!(condition)) {                                                         \
                return ::Sabora::Result<void>::Failure(                                 \
                    code, message, std::source_location::current());                    \
            }                                                                           \
        } while (false)

} // namespace Sabora

