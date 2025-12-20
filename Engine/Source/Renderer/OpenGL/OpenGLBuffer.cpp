#include "pch.h"
#include "OpenGLBuffer.h"
#include "Core/Log.h"
#include "Core/MainThreadDispatcher.h"
#include "Core/Result.h"
#include <glad/gl.h>
#include <algorithm>

namespace Sabora
{
    //==========================================================================
    // Factory Method
    //==========================================================================

    Result<std::unique_ptr<OpenGLBuffer>> OpenGLBuffer::Create(
        BufferType type,
        size_t size,
        BufferUsage usage,
        const void* data)
    {
        if (size == 0)
        {
            return Result<std::unique_ptr<OpenGLBuffer>>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                "Cannot create buffer with size 0"
            );
        }

        // Check if we're on the main thread
        // If not, queue the creation on the main thread
        uint32_t bufferId = 0;
        bool creationSuccess = false;
        std::string errorMessage;

        auto createFunc = [&bufferId, &creationSuccess, &errorMessage, type, size, usage, data]() {
            // Generate buffer
            glGenBuffers(1, &bufferId);

            // Check for OpenGL errors
            GLenum error = glGetError();
            if (error != GL_NO_ERROR)
            {
                creationSuccess = false;
                errorMessage = fmt::format("Failed to generate OpenGL buffer: error code {}", error);
                return;
            }

            // Get target and usage
            uint32_t target = 0;
            switch (type)
            {
                case BufferType::Vertex:    target = GL_ARRAY_BUFFER; break;
                case BufferType::Index:     target = GL_ELEMENT_ARRAY_BUFFER; break;
                case BufferType::Uniform:   target = GL_UNIFORM_BUFFER; break;
                case BufferType::Storage:   target = GL_SHADER_STORAGE_BUFFER; break;
                case BufferType::Indirect: target = GL_DRAW_INDIRECT_BUFFER; break;
                default:
                    creationSuccess = false;
                    errorMessage = "Invalid buffer type";
                    return;
            }

            uint32_t glUsage = 0;
            switch (usage)
            {
                case BufferUsage::Static:  glUsage = GL_STATIC_DRAW; break;
                case BufferUsage::Dynamic: glUsage = GL_DYNAMIC_DRAW; break;
                case BufferUsage::Stream:  glUsage = GL_STREAM_DRAW; break;
                default:
                    creationSuccess = false;
                    errorMessage = "Invalid buffer usage";
                    return;
            }

            // Bind and create buffer
            glBindBuffer(target, bufferId);
            glBufferData(target, static_cast<GLsizeiptr>(size), data, glUsage);

            // Check for errors
            error = glGetError();
            if (error != GL_NO_ERROR)
            {
                creationSuccess = false;
                errorMessage = fmt::format("Failed to create OpenGL buffer data: error code {}", error);
                // Clean up buffer
                glDeleteBuffers(1, &bufferId);
                bufferId = 0;
                return;
            }

            creationSuccess = true;
        };

        // Execute on main thread
        MainThreadDispatcher::Get().DispatchSync(createFunc);

        if (!creationSuccess)
        {
            return Result<std::unique_ptr<OpenGLBuffer>>::Failure(
                ErrorCode::GraphicsBufferCreationFailed,
                errorMessage
            );
        }

        // Create buffer object
        auto buffer = std::unique_ptr<OpenGLBuffer>(new OpenGLBuffer(bufferId, type, size, usage));
        return Result<std::unique_ptr<OpenGLBuffer>>::Success(std::move(buffer));
    }

    //==========================================================================
    // Constructor/Destructor
    //==========================================================================

    OpenGLBuffer::OpenGLBuffer(
        uint32_t bufferId,
        BufferType type,
        size_t size,
        BufferUsage usage) noexcept
        : m_BufferId(bufferId)
        , m_Type(type)
        , m_Size(size)
        , m_Usage(usage)
        , m_Mapped(false)
    {
    }

    OpenGLBuffer::~OpenGLBuffer()
    {
        if (m_BufferId != 0)
        {
            // Queue deletion on main thread
            uint32_t bufferId = m_BufferId;
            MainThreadDispatcher::Get().Dispatch([bufferId]() {
                glDeleteBuffers(1, &bufferId);
            });
            m_BufferId = 0;
        }
    }

    OpenGLBuffer::OpenGLBuffer(OpenGLBuffer&& other) noexcept
        : m_BufferId(other.m_BufferId)
        , m_Type(other.m_Type)
        , m_Size(other.m_Size)
        , m_Usage(other.m_Usage)
        , m_Mapped(other.m_Mapped)
    {
        other.m_BufferId = 0;
        other.m_Size = 0;
        other.m_Mapped = false;
    }

    OpenGLBuffer& OpenGLBuffer::operator=(OpenGLBuffer&& other) noexcept
    {
        if (this != &other)
        {
            // Delete current buffer
            if (m_BufferId != 0)
            {
                uint32_t bufferId = m_BufferId;
                MainThreadDispatcher::Get().Dispatch([bufferId]() {
                    glDeleteBuffers(1, &bufferId);
                });
            }

            // Move data
            m_BufferId = other.m_BufferId;
            m_Type = other.m_Type;
            m_Size = other.m_Size;
            m_Usage = other.m_Usage;
            m_Mapped = other.m_Mapped;

            // Reset other
            other.m_BufferId = 0;
            other.m_Size = 0;
            other.m_Mapped = false;
        }

        return *this;
    }

    //==========================================================================
    // Buffer Interface Implementation
    //==========================================================================

    Result<void> OpenGLBuffer::UpdateData(
        const void* data,
        size_t size,
        size_t offset)
    {
        if (data == nullptr)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                "UpdateData: data pointer is null"
            );
        }

        if (size == 0)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                "UpdateData: size is 0"
            );
        }

        if (offset + size > m_Size)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                fmt::format("UpdateData: offset ({}) + size ({}) exceeds buffer size ({})", offset, size, m_Size)
            );
        }

        if (m_Mapped)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                "UpdateData: cannot update buffer while it is mapped"
            );
        }

        bool success = false;
        std::string errorMessage;

        auto updateFunc = [this, &success, &errorMessage, data, size, offset]() {
            uint32_t target = GetGLTarget();
            glBindBuffer(target, m_BufferId);
            glBufferSubData(target, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);

            GLenum error = glGetError();
            if (error != GL_NO_ERROR)
            {
                success = false;
                errorMessage = fmt::format("Failed to update buffer data: error code {}", error);
                return;
            }

            success = true;
        };

        // Execute on main thread
        MainThreadDispatcher::Get().DispatchSync(updateFunc);

        if (!success)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                errorMessage
            );
        }

        return Result<void>::Success();
    }

    Result<void*> OpenGLBuffer::Map(MemoryAccess access)
    {
        if (m_Mapped)
        {
            return Result<void*>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                "Buffer is already mapped"
            );
        }

        void* mappedPtr = nullptr;
        bool success = false;
        std::string errorMessage;

        auto mapFunc = [this, &mappedPtr, &success, &errorMessage, access]() {
            uint32_t target = GetGLTarget();
            uint32_t glAccess = GetGLAccess(access);

            glBindBuffer(target, m_BufferId);
            mappedPtr = glMapBuffer(target, glAccess);

            if (mappedPtr == nullptr)
            {
                GLenum error = glGetError();
                success = false;
                errorMessage = fmt::format("Failed to map buffer: error code {}", error);
                return;
            }

            success = true;
        };

        // Execute on main thread
        MainThreadDispatcher::Get().DispatchSync(mapFunc);

        if (!success)
        {
            return Result<void*>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                errorMessage
            );
        }

        m_Mapped = true;
        return Result<void*>::Success(mappedPtr);
    }

    Result<void> OpenGLBuffer::Unmap()
    {
        if (!m_Mapped)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                "Buffer is not mapped"
            );
        }

        bool success = false;
        std::string errorMessage;

        auto unmapFunc = [this, &success, &errorMessage]() {
            uint32_t target = GetGLTarget();
            glBindBuffer(target, m_BufferId);
            GLboolean result = glUnmapBuffer(target);

            if (result == GL_FALSE)
            {
                GLenum error = glGetError();
                success = false;
                errorMessage = fmt::format("Failed to unmap buffer: error code {}", error);
                return;
            }

            success = true;
        };

        // Execute on main thread
        MainThreadDispatcher::Get().DispatchSync(unmapFunc);

        if (!success)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                errorMessage
            );
        }

        m_Mapped = false;
        return Result<void>::Success();
    }

    void* OpenGLBuffer::GetNativeHandle() const
    {
        return reinterpret_cast<void*>(static_cast<uintptr_t>(m_BufferId));
    }

    bool OpenGLBuffer::IsValid() const
    {
        return m_BufferId != 0;
    }

    //==========================================================================
    // Helper Methods
    //==========================================================================

    uint32_t OpenGLBuffer::GetGLTarget() const
    {
        switch (m_Type)
        {
            case BufferType::Vertex:    return GL_ARRAY_BUFFER;
            case BufferType::Index:     return GL_ELEMENT_ARRAY_BUFFER;
            case BufferType::Uniform:   return GL_UNIFORM_BUFFER;
            case BufferType::Storage:   return GL_SHADER_STORAGE_BUFFER;
            case BufferType::Indirect: return GL_DRAW_INDIRECT_BUFFER;
            default:                    return 0;
        }
    }

    uint32_t OpenGLBuffer::GetGLUsage() const
    {
        switch (m_Usage)
        {
            case BufferUsage::Static:  return GL_STATIC_DRAW;
            case BufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
            case BufferUsage::Stream:  return GL_STREAM_DRAW;
            default:                   return GL_STATIC_DRAW;
        }
    }

    uint32_t OpenGLBuffer::GetGLAccess(MemoryAccess access) const
    {
        switch (access)
        {
            case MemoryAccess::Read:      return GL_READ_ONLY;
            case MemoryAccess::Write:     return GL_WRITE_ONLY;
            case MemoryAccess::ReadWrite: return GL_READ_WRITE;
            default:                     return GL_READ_WRITE;
        }
    }

} // namespace Sabora
