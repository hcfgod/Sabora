#include "pch.h"
#include "OpenGLShader.h"
#include "Core/Log.h"
#include "Core/MainThreadDispatcher.h"
#include <glad/gl.h>
#include <sstream>

namespace Sabora
{
    //==========================================================================
    // Factory Method
    //==========================================================================

    Result<std::unique_ptr<OpenGLShader>> OpenGLShader::Create(
        ShaderStage stage,
        const std::string& source)
    {
        if (source.empty())
        {
            return Result<std::unique_ptr<OpenGLShader>>::Failure(
                ErrorCode::CoreInvalidArgument,
                "Shader source is empty"
            );
        }

        // Compile shader on main thread
        uint32_t shaderId = 0;
        bool compiled = false;
        std::string compileError;

        auto compileFunc = [&shaderId, &compiled, &compileError, stage, source]() {
            auto result = CompileShader(stage, source);
            shaderId = std::get<0>(result);
            compiled = std::get<1>(result);
            compileError = std::get<2>(result);
        };

        MainThreadDispatcher::Get().DispatchSync(compileFunc);

        if (!compiled)
        {
            return Result<std::unique_ptr<OpenGLShader>>::Failure(
                ErrorCode::GraphicsShaderCompilationFailed,
                compileError
            );
        }

        auto shader = std::unique_ptr<OpenGLShader>(
            new OpenGLShader(shaderId, stage, source, compiled, compileError)
        );

        return Result<std::unique_ptr<OpenGLShader>>::Success(std::move(shader));
    }

    //==========================================================================
    // Constructor/Destructor
    //==========================================================================

    OpenGLShader::OpenGLShader(
        uint32_t shaderId,
        ShaderStage stage,
        const std::string& source,
        bool compiled,
        const std::string& compileError) noexcept
        : m_ShaderId(shaderId)
        , m_Stage(stage)
        , m_Source(source)
        , m_Compiled(compiled)
        , m_CompileError(compileError)
    {
    }

    OpenGLShader::~OpenGLShader()
    {
        if (m_ShaderId != 0)
        {
            // Clean up shader on main thread
            // Use DispatchSync to ensure synchronous cleanup even during shutdown
            uint32_t shaderId = m_ShaderId;
            MainThreadDispatcher::Get().DispatchSync([shaderId]() {
                glDeleteShader(shaderId);
            });
            m_ShaderId = 0;
        }
    }

    OpenGLShader::OpenGLShader(OpenGLShader&& other) noexcept
        : m_ShaderId(other.m_ShaderId)
        , m_Stage(other.m_Stage)
        , m_Source(std::move(other.m_Source))
        , m_Compiled(other.m_Compiled)
        , m_CompileError(std::move(other.m_CompileError))
    {
        other.m_ShaderId = 0;
        other.m_Compiled = false;
    }

    OpenGLShader& OpenGLShader::operator=(OpenGLShader&& other) noexcept
    {
        if (this != &other)
        {
            // Delete current shader
            if (m_ShaderId != 0)
            {
                uint32_t shaderId = m_ShaderId;
                MainThreadDispatcher::Get().DispatchSync([shaderId]() {
                    glDeleteShader(shaderId);
                });
            }

            // Move data
            m_ShaderId = other.m_ShaderId;
            m_Stage = other.m_Stage;
            m_Source = std::move(other.m_Source);
            m_Compiled = other.m_Compiled;
            m_CompileError = std::move(other.m_CompileError);

            // Reset other
            other.m_ShaderId = 0;
            other.m_Compiled = false;
        }

        return *this;
    }

    //==========================================================================
    // Shader Interface Implementation
    //==========================================================================

    void* OpenGLShader::GetNativeHandle() const
    {
        return reinterpret_cast<void*>(static_cast<uintptr_t>(m_ShaderId));
    }

    //==========================================================================
    // Helper Methods
    //==========================================================================

    std::tuple<uint32_t, bool, std::string> OpenGLShader::CompileShader(
        ShaderStage stage,
        const std::string& source)
    {
        uint32_t shaderType = GetGLShaderType(stage);
        if (shaderType == 0)
        {
            return std::make_tuple(0, false, "Invalid shader stage");
        }

        // Create shader
        uint32_t shaderId = glCreateShader(shaderType);
        if (shaderId == 0)
        {
            GLenum error = glGetError();
            return std::make_tuple(0, false, 
                fmt::format("Failed to create shader: error code {}", error));
        }

        // Compile shader
        const char* sourcePtr = source.c_str();
        GLint sourceLength = static_cast<GLint>(source.length());
        glShaderSource(shaderId, 1, &sourcePtr, &sourceLength);
        glCompileShader(shaderId);

        // Check compilation status
        GLint success = 0;
        glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);

        if (success == GL_FALSE)
        {
            // Get error log
            GLint logLength = 0;
            glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);

            std::string errorLog;
            if (logLength > 0)
            {
                errorLog.resize(static_cast<size_t>(logLength));
                glGetShaderInfoLog(shaderId, logLength, nullptr, errorLog.data());
                // Remove null terminator if present
                if (!errorLog.empty() && errorLog.back() == '\0')
                {
                    errorLog.pop_back();
                }
            }
            else
            {
                errorLog = "Shader compilation failed (no error log available)";
            }

            // Delete shader
            glDeleteShader(shaderId);

            return std::make_tuple(0, false, errorLog);
        }

        return std::make_tuple(shaderId, true, "");
    }

    uint32_t OpenGLShader::GetGLShaderType(ShaderStage stage)
    {
        switch (stage)
        {
            case ShaderStage::Vertex:                  return GL_VERTEX_SHADER;
            case ShaderStage::Fragment:                 return GL_FRAGMENT_SHADER;
            case ShaderStage::Geometry:                 return GL_GEOMETRY_SHADER;
            case ShaderStage::Compute:                  return GL_COMPUTE_SHADER;
            case ShaderStage::TessellationControl:     return GL_TESS_CONTROL_SHADER;
            case ShaderStage::TessellationEvaluation:   return GL_TESS_EVALUATION_SHADER;
            default:                                    return 0;
        }
    }

} // namespace Sabora
