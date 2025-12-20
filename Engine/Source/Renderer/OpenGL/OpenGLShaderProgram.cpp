#include "pch.h"
#include "OpenGLShaderProgram.h"
#include "Core/Log.h"
#include "Core/MainThreadDispatcher.h"
#include <glad/gl.h>

namespace Sabora
{
    //==========================================================================
    // Factory Method
    //==========================================================================

    Result<std::unique_ptr<OpenGLShaderProgram>> OpenGLShaderProgram::Create(
        std::vector<std::unique_ptr<OpenGLShader>> shaders)
    {
        if (shaders.empty())
        {
            return Result<std::unique_ptr<OpenGLShaderProgram>>::Failure(
                ErrorCode::CoreInvalidArgument,
                "Cannot create shader program with no shader stages"
            );
        }

        // Collect shader pointers for linking
        std::vector<OpenGLShader*> shaderPtrs;
        shaderPtrs.reserve(shaders.size());
        for (auto& shader : shaders)
        {
            if (!shader || !shader->IsValid())
            {
                return Result<std::unique_ptr<OpenGLShaderProgram>>::Failure(
                    ErrorCode::CoreInvalidArgument,
                    "Cannot create shader program with invalid shader stage"
                );
            }
            shaderPtrs.push_back(shader.get());
        }

        // Link program on main thread
        uint32_t programId = 0;
        bool linked = false;
        std::string linkError;
        ShaderReflection reflection; // Will be populated later if needed

        auto linkFunc = [&programId, &linked, &linkError, &shaderPtrs]() {
            auto result = LinkProgram(shaderPtrs);
            programId = std::get<0>(result);
            linked = std::get<1>(result);
            linkError = std::get<2>(result);
        };

        MainThreadDispatcher::Get().DispatchSync(linkFunc);

        if (!linked)
        {
            return Result<std::unique_ptr<OpenGLShaderProgram>>::Failure(
                ErrorCode::GraphicsShaderLinkFailed,
                linkError
            );
        }

        auto program = std::unique_ptr<OpenGLShaderProgram>(
            new OpenGLShaderProgram(programId, linked, linkError, reflection)
        );

        // Cache locations
        program->CacheLocations();

        return Result<std::unique_ptr<OpenGLShaderProgram>>::Success(std::move(program));
    }

    Result<std::unique_ptr<OpenGLShaderProgram>> OpenGLShaderProgram::CreateFromProgramId(
        uint32_t programId)
    {
        if (programId == 0)
        {
            return Result<std::unique_ptr<OpenGLShaderProgram>>::Failure(
                ErrorCode::CoreInvalidArgument,
                "Program ID is 0"
            );
        }

        // Verify program is valid and linked
        bool isValid = false;
        MainThreadDispatcher::Get().DispatchSync([programId, &isValid]() {
            GLint linkStatus = 0;
            glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);
            isValid = (linkStatus == GL_TRUE);
        });

        if (!isValid)
        {
            return Result<std::unique_ptr<OpenGLShaderProgram>>::Failure(
                ErrorCode::GraphicsShaderLinkFailed,
                "Program ID is not a valid linked program"
            );
        }

        ShaderReflection reflection; // Empty - can be populated later if needed
        auto program = std::unique_ptr<OpenGLShaderProgram>(
            new OpenGLShaderProgram(programId, true, "", reflection)
        );

        // Cache locations
        program->CacheLocations();

        return Result<std::unique_ptr<OpenGLShaderProgram>>::Success(std::move(program));
    }

    //==========================================================================
    // Constructor/Destructor
    //==========================================================================

    OpenGLShaderProgram::OpenGLShaderProgram(
        uint32_t programId,
        bool linked,
        const std::string& linkError,
        const ShaderReflection& reflection) noexcept
        : m_ProgramId(programId)
        , m_Linked(linked)
        , m_LinkError(linkError)
        , m_Reflection(reflection)
    {
    }

    OpenGLShaderProgram::~OpenGLShaderProgram()
    {
        if (m_ProgramId != 0)
        {
            // Clean up shader program on main thread
            // Use DispatchSync to ensure synchronous cleanup even during shutdown
            uint32_t programId = m_ProgramId;
            MainThreadDispatcher::Get().DispatchSync([programId]() {
                glDeleteProgram(programId);
            });
            m_ProgramId = 0;
        }
    }

    OpenGLShaderProgram::OpenGLShaderProgram(OpenGLShaderProgram&& other) noexcept
        : m_ProgramId(other.m_ProgramId)
        , m_Linked(other.m_Linked)
        , m_LinkError(std::move(other.m_LinkError))
        , m_Reflection(std::move(other.m_Reflection))
        , m_UniformLocations(std::move(other.m_UniformLocations))
        , m_AttributeLocations(std::move(other.m_AttributeLocations))
    {
        other.m_ProgramId = 0;
        other.m_Linked = false;
    }

    OpenGLShaderProgram& OpenGLShaderProgram::operator=(OpenGLShaderProgram&& other) noexcept
    {
        if (this != &other)
        {
            // Delete current program
            if (m_ProgramId != 0)
            {
                uint32_t programId = m_ProgramId;
                MainThreadDispatcher::Get().DispatchSync([programId]() {
                    glDeleteProgram(programId);
                });
            }

            // Move data
            m_ProgramId = other.m_ProgramId;
            m_Linked = other.m_Linked;
            m_LinkError = std::move(other.m_LinkError);
            m_Reflection = std::move(other.m_Reflection);
            m_UniformLocations = std::move(other.m_UniformLocations);
            m_AttributeLocations = std::move(other.m_AttributeLocations);

            // Reset other
            other.m_ProgramId = 0;
            other.m_Linked = false;
        }

        return *this;
    }

    //==========================================================================
    // Public Methods
    //==========================================================================

    int32_t OpenGLShaderProgram::GetUniformLocation(const std::string& name)
    {
        // Check cache first
        auto it = m_UniformLocations.find(name);
        if (it != m_UniformLocations.end())
        {
            return it->second;
        }

        // Query OpenGL (must be on main thread)
        int32_t location = -1;
        MainThreadDispatcher::Get().DispatchSync([this, &location, &name]() {
            location = glGetUniformLocation(m_ProgramId, name.c_str());
        });

        // Cache result (even if -1)
        m_UniformLocations[name] = location;

        return location;
    }

    int32_t OpenGLShaderProgram::GetAttributeLocation(const std::string& name)
    {
        // Check cache first
        auto it = m_AttributeLocations.find(name);
        if (it != m_AttributeLocations.end())
        {
            return it->second;
        }

        // Query OpenGL (must be on main thread)
        int32_t location = -1;
        MainThreadDispatcher::Get().DispatchSync([this, &location, &name]() {
            location = glGetAttribLocation(m_ProgramId, name.c_str());
        });

        // Cache result (even if -1)
        m_AttributeLocations[name] = location;

        return location;
    }

    void* OpenGLShaderProgram::GetNativeHandle() const
    {
        return reinterpret_cast<void*>(static_cast<uintptr_t>(m_ProgramId));
    }

    //==========================================================================
    // Helper Methods
    //==========================================================================

    std::tuple<uint32_t, bool, std::string> OpenGLShaderProgram::LinkProgram(
        const std::vector<OpenGLShader*>& shaders)
    {
        // Create program
        uint32_t programId = glCreateProgram();
        if (programId == 0)
        {
            GLenum error = glGetError();
            return std::make_tuple(0, false,
                fmt::format("Failed to create shader program: error code {}", error));
        }

        // Attach all shaders
        for (OpenGLShader* shader : shaders)
        {
            glAttachShader(programId, shader->GetShaderId());
        }

        // Link program
        glLinkProgram(programId);

        // Check link status
        GLint success = 0;
        glGetProgramiv(programId, GL_LINK_STATUS, &success);

        if (success == GL_FALSE)
        {
            // Get error log
            GLint logLength = 0;
            glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);

            std::string errorLog;
            if (logLength > 0)
            {
                errorLog.resize(static_cast<size_t>(logLength));
                glGetProgramInfoLog(programId, logLength, nullptr, errorLog.data());
                // Remove null terminator if present
                if (!errorLog.empty() && errorLog.back() == '\0')
                {
                    errorLog.pop_back();
                }
            }
            else
            {
                errorLog = "Shader program linking failed (no error log available)";
            }

            // Detach and delete program
            for (OpenGLShader* shader : shaders)
            {
                glDetachShader(programId, shader->GetShaderId());
            }
            glDeleteProgram(programId);

            return std::make_tuple(0, false, errorLog);
        }

        // Detach shaders (they can be deleted independently)
        for (OpenGLShader* shader : shaders)
        {
            glDetachShader(programId, shader->GetShaderId());
        }

        return std::make_tuple(programId, true, "");
    }

    void OpenGLShaderProgram::CacheLocations()
    {
        // This can be extended to cache all uniforms/attributes at link time
        // For now, we cache on-demand in GetUniformLocation/GetAttributeLocation
    }

} // namespace Sabora
