#include "pch.h"
#include "OpenGLFramebuffer.h"
#include "Renderer/Resources/Texture.h"
#include "Core/Log.h"
#include "Core/MainThreadDispatcher.h"
#include <glad/gl.h>
#include <algorithm>
#include <cstdint>

// Static assertion: OpenGL handles are always 32-bit, even on 64-bit systems
// This ensures our casting from void* to uint32_t is safe
static_assert(sizeof(uintptr_t) >= sizeof(uint32_t), "uintptr_t must be at least as large as uint32_t");

namespace Sabora
{
    //==========================================================================
    // Factory Method
    //==========================================================================

    Result<std::unique_ptr<OpenGLFramebuffer>> OpenGLFramebuffer::Create(
        uint32_t width,
        uint32_t height,
        Texture* const* colorAttachments,
        uint32_t colorAttachmentCount,
        Texture* depthStencilAttachment)
    {
        if (width == 0 || height == 0)
        {
            return Result<std::unique_ptr<OpenGLFramebuffer>>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                "Framebuffer dimensions must be greater than 0"
            );
        }

        // Validate color attachments
        for (uint32_t i = 0; i < colorAttachmentCount; ++i)
        {
            if (!colorAttachments[i] || !colorAttachments[i]->IsValid())
            {
                return Result<std::unique_ptr<OpenGLFramebuffer>>::Failure(
                    ErrorCode::CoreInvalidArgument,
                    fmt::format("Color attachment {} is null or invalid", i)
                );
            }

            if (colorAttachments[i]->GetWidth() != width || colorAttachments[i]->GetHeight() != height)
            {
                return Result<std::unique_ptr<OpenGLFramebuffer>>::Failure(
                    ErrorCode::GraphicsInvalidOperation,
                    fmt::format("Color attachment {} size ({}, {}) does not match framebuffer size ({}, {})",
                        i, colorAttachments[i]->GetWidth(), colorAttachments[i]->GetHeight(), width, height)
                );
            }
        }

        // Validate depth/stencil attachment if provided
        if (depthStencilAttachment)
        {
            if (!depthStencilAttachment->IsValid())
            {
                return Result<std::unique_ptr<OpenGLFramebuffer>>::Failure(
                    ErrorCode::CoreInvalidArgument,
                    "Depth/stencil attachment is invalid"
                );
            }

            if (depthStencilAttachment->GetWidth() != width || depthStencilAttachment->GetHeight() != height)
            {
                return Result<std::unique_ptr<OpenGLFramebuffer>>::Failure(
                    ErrorCode::GraphicsInvalidOperation,
                    fmt::format("Depth/stencil attachment size ({}, {}) does not match framebuffer size ({}, {})",
                        depthStencilAttachment->GetWidth(), depthStencilAttachment->GetHeight(), width, height)
                );
            }
        }

        // Create framebuffer on main thread
        uint32_t framebufferId = 0;
        bool success = false;
        std::string errorMessage;

        auto createFunc = [&framebufferId, &success, &errorMessage, width, height, 
                          colorAttachments, colorAttachmentCount, depthStencilAttachment]() {
            // Generate framebuffer
            glGenFramebuffers(1, &framebufferId);

            GLenum error = glGetError();
            if (error != GL_NO_ERROR)
            {
                success = false;
                errorMessage = fmt::format("Failed to generate OpenGL framebuffer: error code {}", error);
                return;
            }

            // Bind framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);

            // Attach color textures
            std::vector<GLenum> drawBuffers;
            for (uint32_t i = 0; i < colorAttachmentCount; ++i)
            {
                Texture* texture = colorAttachments[i];
                void* nativeHandle = texture->GetNativeHandle();
                if (!nativeHandle)
                {
                    SB_CORE_ERROR("Texture::GetNativeHandle() returned null for color attachment {}", i);
                    continue;
                }
                
                // OpenGL handles are always 32-bit, even on 64-bit systems
                uintptr_t handleValue = reinterpret_cast<uintptr_t>(nativeHandle);
                if (handleValue > UINT32_MAX)
                {
                    SB_CORE_ERROR("OpenGL texture handle value exceeds uint32_t maximum: {}", handleValue);
                    continue;
                }
                
                uint32_t textureId = static_cast<uint32_t>(handleValue);

                // Determine texture target based on type
                uint32_t target = GL_TEXTURE_2D;
                switch (texture->GetType())
                {
                    case TextureType::Texture2D:
                        target = GL_TEXTURE_2D;
                        break;
                    case TextureType::TextureCube:
                        target = GL_TEXTURE_CUBE_MAP_POSITIVE_X; // Use first face
                        break;
                    default:
                        success = false;
                        errorMessage = fmt::format("Unsupported texture type for color attachment {}", i);
                        glDeleteFramebuffers(1, &framebufferId);
                        framebufferId = 0;
                        return;
                }

                glFramebufferTexture2D(
                    GL_FRAMEBUFFER,
                    GL_COLOR_ATTACHMENT0 + i,
                    target,
                    textureId,
                    0 // mip level
                );

                drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
            }

            // Set draw buffers
            if (!drawBuffers.empty())
            {
                glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());
            }
            else
            {
                // No color attachments - render to depth only
                glDrawBuffer(GL_NONE);
                glReadBuffer(GL_NONE);
            }

            // Attach depth/stencil texture if provided
            if (depthStencilAttachment)
            {
                void* nativeHandle = depthStencilAttachment->GetNativeHandle();
                if (!nativeHandle)
                {
                    SB_CORE_ERROR("Texture::GetNativeHandle() returned null for depth/stencil attachment");
                    return;
                }
                
                // OpenGL handles are always 32-bit, even on 64-bit systems
                uintptr_t handleValue = reinterpret_cast<uintptr_t>(nativeHandle);
                if (handleValue > UINT32_MAX)
                {
                    SB_CORE_ERROR("OpenGL texture handle value exceeds uint32_t maximum: {}", handleValue);
                    return;
                }
                
                uint32_t textureId = static_cast<uint32_t>(handleValue);

                uint32_t target = GL_TEXTURE_2D;
                switch (depthStencilAttachment->GetType())
                {
                    case TextureType::Texture2D:
                        target = GL_TEXTURE_2D;
                        break;
                    default:
                        success = false;
                        errorMessage = "Unsupported texture type for depth/stencil attachment";
                        glDeleteFramebuffers(1, &framebufferId);
                        framebufferId = 0;
                        return;
                }

                // Determine attachment point based on format
                TextureFormat format = depthStencilAttachment->GetFormat();
                GLenum attachment = GL_DEPTH_ATTACHMENT;
                
                if (format == TextureFormat::Depth24Stencil8 || format == TextureFormat::Depth32FStencil8)
                {
                    attachment = GL_DEPTH_STENCIL_ATTACHMENT;
                }
                else
                {
                    attachment = GL_DEPTH_ATTACHMENT;
                }

                glFramebufferTexture2D(
                    GL_FRAMEBUFFER,
                    attachment,
                    target,
                    textureId,
                    0 // mip level
                );
            }

            // Check framebuffer completeness
            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE)
            {
                success = false;
                const char* statusStr = "Unknown";
                switch (status)
                {
                    case GL_FRAMEBUFFER_UNDEFINED: statusStr = "GL_FRAMEBUFFER_UNDEFINED"; break;
                    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: statusStr = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"; break;
                    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: statusStr = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"; break;
                    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: statusStr = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"; break;
                    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: statusStr = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"; break;
                    case GL_FRAMEBUFFER_UNSUPPORTED: statusStr = "GL_FRAMEBUFFER_UNSUPPORTED"; break;
                    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: statusStr = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"; break;
                    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: statusStr = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"; break;
                }
                errorMessage = fmt::format("Framebuffer is not complete: {}", statusStr);
                glDeleteFramebuffers(1, &framebufferId);
                framebufferId = 0;
                return;
            }

            // Unbind framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            success = true;
        };

        // Execute on main thread
        MainThreadDispatcher::Get().DispatchSync(createFunc);

        if (!success)
        {
            return Result<std::unique_ptr<OpenGLFramebuffer>>::Failure(
                ErrorCode::GraphicsFramebufferCreationFailed,
                errorMessage
            );
        }

        // Store color attachments
        std::vector<Texture*> colorAttachmentsVec;
        colorAttachmentsVec.reserve(colorAttachmentCount);
        for (uint32_t i = 0; i < colorAttachmentCount; ++i)
        {
            colorAttachmentsVec.push_back(colorAttachments[i]);
        }

        auto framebuffer = std::unique_ptr<OpenGLFramebuffer>(
            new OpenGLFramebuffer(framebufferId, width, height, std::move(colorAttachmentsVec), depthStencilAttachment)
        );

        return Result<std::unique_ptr<OpenGLFramebuffer>>::Success(std::move(framebuffer));
    }

    //==========================================================================
    // Constructor/Destructor
    //==========================================================================

    OpenGLFramebuffer::OpenGLFramebuffer(
        uint32_t framebufferId,
        uint32_t width,
        uint32_t height,
        std::vector<Texture*> colorAttachments,
        Texture* depthStencilAttachment) noexcept
        : m_FramebufferId(framebufferId)
        , m_Width(width)
        , m_Height(height)
        , m_ColorAttachments(std::move(colorAttachments))
        , m_DepthStencilAttachment(depthStencilAttachment)
    {
    }

    OpenGLFramebuffer::~OpenGLFramebuffer()
    {
        if (m_FramebufferId != 0)
        {
            // Clean up framebuffer on main thread
            // Use DispatchSync to ensure synchronous cleanup even during shutdown
            uint32_t framebufferId = m_FramebufferId;
            MainThreadDispatcher::Get().DispatchSync([framebufferId]() {
                glDeleteFramebuffers(1, &framebufferId);
            });
            m_FramebufferId = 0;
        }
    }

    OpenGLFramebuffer::OpenGLFramebuffer(OpenGLFramebuffer&& other) noexcept
        : m_FramebufferId(other.m_FramebufferId)
        , m_Width(other.m_Width)
        , m_Height(other.m_Height)
        , m_ColorAttachments(std::move(other.m_ColorAttachments))
        , m_DepthStencilAttachment(other.m_DepthStencilAttachment)
    {
        other.m_FramebufferId = 0;
        other.m_Width = 0;
        other.m_Height = 0;
        other.m_DepthStencilAttachment = nullptr;
    }

    OpenGLFramebuffer& OpenGLFramebuffer::operator=(OpenGLFramebuffer&& other) noexcept
    {
        if (this != &other)
        {
            if (m_FramebufferId != 0)
            {
                uint32_t framebufferId = m_FramebufferId;
                MainThreadDispatcher::Get().DispatchSync([framebufferId]() {
                    glDeleteFramebuffers(1, &framebufferId);
                });
            }

            m_FramebufferId = other.m_FramebufferId;
            m_Width = other.m_Width;
            m_Height = other.m_Height;
            m_ColorAttachments = std::move(other.m_ColorAttachments);
            m_DepthStencilAttachment = other.m_DepthStencilAttachment;

            other.m_FramebufferId = 0;
            other.m_Width = 0;
            other.m_Height = 0;
            other.m_DepthStencilAttachment = nullptr;
        }

        return *this;
    }

    //==========================================================================
    // Framebuffer Interface Implementation
    //==========================================================================

    uint32_t OpenGLFramebuffer::GetColorAttachmentCount() const
    {
        return static_cast<uint32_t>(m_ColorAttachments.size());
    }

    Texture* OpenGLFramebuffer::GetColorAttachment(uint32_t index) const
    {
        if (index >= m_ColorAttachments.size())
        {
            return nullptr;
        }
        return m_ColorAttachments[index];
    }

    Texture* OpenGLFramebuffer::GetDepthStencilAttachment() const
    {
        return m_DepthStencilAttachment;
    }

    bool OpenGLFramebuffer::IsComplete() const
    {
        if (m_FramebufferId == 0)
        {
            return false;
        }

        bool isComplete = false;
        MainThreadDispatcher::Get().DispatchSync([this, &isComplete]() {
            glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferId);
            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            isComplete = (status == GL_FRAMEBUFFER_COMPLETE);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        });

        return isComplete;
    }

    void* OpenGLFramebuffer::GetNativeHandle() const
    {
        return reinterpret_cast<void*>(static_cast<uintptr_t>(m_FramebufferId));
    }

    bool OpenGLFramebuffer::IsValid() const
    {
        return m_FramebufferId != 0;
    }

    Result<void> OpenGLFramebuffer::Bind()
    {
        if (m_FramebufferId == 0)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                "Cannot bind invalid framebuffer"
            );
        }

        bool success = false;
        std::string errorMessage;

        auto bindFunc = [this, &success, &errorMessage]() {
            glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferId);

            GLenum error = glGetError();
            if (error != GL_NO_ERROR)
            {
                success = false;
                errorMessage = fmt::format("Failed to bind framebuffer: error code {}", error);
                return;
            }

            success = true;
        };

        MainThreadDispatcher::Get().DispatchSync(bindFunc);

        if (!success)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                errorMessage
            );
        }

        return Result<void>::Success();
    }

} // namespace Sabora
