#include "pch.h"
#include "Renderer/OpenGL/OpenGLRenderer.h"
#include "Renderer/OpenGL/OpenGLContext.h"
#include "Renderer/OpenGL/OpenGLBuffer.h"
#include "Renderer/OpenGL/OpenGLTexture.h"
#include "Renderer/OpenGL/OpenGLShader.h"
#include "Renderer/OpenGL/OpenGLPipelineState.h"
#include "Renderer/OpenGL/OpenGLFramebuffer.h"
#include "Renderer/Resources/Buffer.h"
#include "Core/Window.h"
#include "Core/Log.h"
#include "Core/MainThreadDispatcher.h"
#include <glad/gl.h>
#include <cassert>
#include <cstdint>
#include <climits>

// Static assertion: OpenGL handles are always 32-bit, even on 64-bit systems
// This ensures our casting from void* to uint32_t is safe
static_assert(sizeof(uintptr_t) >= sizeof(uint32_t), "uintptr_t must be at least as large as uint32_t");

namespace Sabora
{
    // OpenGL version constants for feature detection
    namespace OpenGLVersionConstants
    {
        constexpr int32_t MIN_MAJOR_VERSION_FOR_COMPUTE = 4;
        constexpr int32_t MIN_MINOR_VERSION_FOR_COMPUTE = 3;
        constexpr int32_t MIN_MAJOR_VERSION_FOR_UNIFORM_BUFFERS = 3;
        constexpr int32_t MIN_MINOR_VERSION_FOR_UNIFORM_BUFFERS = 1;
    }

    OpenGLRenderer::OpenGLRenderer()
    {
    }

    OpenGLRenderer::~OpenGLRenderer()
    {
        Shutdown();
    }

    Result<void> OpenGLRenderer::Initialize(Window* window)
    {
        if (m_Initialized)
        {
            SB_CORE_WARN("OpenGLRenderer::Initialize() called but renderer is already initialized");
            return Result<void>::Success();
        }

        if (!window || !window->IsValid())
        {
            return Result<void>::Failure(
                ErrorCode::CoreNullPointer,
                "Window is null or invalid"
            );
        }

        m_Window = window;

        // Create OpenGL context
        auto contextResult = OpenGLContext::Create(window);
        if (contextResult.IsFailure())
        {
            return Result<void>::Failure(contextResult.GetError());
        }
        m_Context = std::move(contextResult).Value();

        // Make context current
        auto makeCurrentResult = m_Context->MakeCurrent();
        if (makeCurrentResult.IsFailure())
        {
            m_Context.reset();
            return Result<void>::Failure(makeCurrentResult.GetError());
        }

        // Initialize capabilities
        InitializeCapabilities();

        // Set initial viewport
        int32_t width = window->GetWidth();
        int32_t height = window->GetHeight();
        m_CurrentViewport = Viewport{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
        m_CurrentScissor = ScissorRect{ 0, 0, width, height };

        m_Initialized = true;

        SB_CORE_INFO("OpenGL renderer initialized successfully");
        SB_CORE_INFO("{}", m_Capabilities.ToString());

        return Result<void>::Success();
    }

    void OpenGLRenderer::Shutdown()
    {
        if (!m_Initialized)
        {
            return;
        }

        // Release context
        if (m_Context)
        {
            auto releaseResult = m_Context->ReleaseCurrent();
            if (releaseResult.IsFailure())
            {
                SB_CORE_WARN("Failed to release OpenGL context during shutdown: {}", 
                    releaseResult.GetError().ToString());
                // Continue with shutdown even if context release fails
            }
            m_Context.reset();
        }

        m_Window = nullptr;
        m_Initialized = false;

        SB_CORE_INFO("OpenGL renderer shutdown complete");
    }

    RenderContext* OpenGLRenderer::GetContext() const
    {
        return m_Context.get();
    }

    Result<void> OpenGLRenderer::BeginFrame()
    {
        if (!m_Initialized)
        {
            return Result<void>::Failure(
                ErrorCode::CoreInvalidState,
                "Renderer is not initialized"
            );
        }

        SB_TRY_VOID(EnsureContextCurrent());

        // Lazy state application: Viewport and scissor are marked as "dirty" when set,
        // and only applied to OpenGL at the start of each frame. This reduces redundant
        // OpenGL state changes when the same viewport/scissor is set multiple times.
        // The dirty flag system ensures we only call glViewport/glScissor when necessary.

        // Apply viewport if dirty
        // OpenGL viewport state persists until changed, so we only update it when needed.
        if (m_ViewportDirty)
        {
            glViewport(
                static_cast<GLint>(m_CurrentViewport.x),
                static_cast<GLint>(m_CurrentViewport.y),
                static_cast<GLsizei>(m_CurrentViewport.width),
                static_cast<GLsizei>(m_CurrentViewport.height)
            );
            // glDepthRange sets the depth buffer range for the viewport
            glDepthRange(m_CurrentViewport.minDepth, m_CurrentViewport.maxDepth);
            m_ViewportDirty = false;
        }

        // Apply scissor if dirty
        // Scissor test restricts rendering to a rectangular region. Like viewport,
        // OpenGL scissor state persists, so we only update when changed.
        if (m_ScissorDirty)
        {
            glScissor(
                m_CurrentScissor.x,
                m_CurrentScissor.y,
                m_CurrentScissor.width,
                m_CurrentScissor.height
            );
            m_ScissorDirty = false;
        }

        return Result<void>::Success();
    }

    Result<void> OpenGLRenderer::EndFrame()
    {
        if (!m_Initialized)
        {
            return Result<void>::Failure(
                ErrorCode::CoreInvalidState,
                "Renderer is not initialized"
            );
        }

        // Swap buffers
        return m_Context->SwapBuffers();
    }

    Result<void> OpenGLRenderer::SetViewport(const Viewport& viewport)
    {
        if (!m_Initialized)
        {
            return Result<void>::Failure(
                ErrorCode::CoreInvalidState,
                "Renderer is not initialized"
            );
        }

        // Store the viewport and mark it as dirty for lazy application in BeginFrame().
        // This defers the actual OpenGL call until the next frame, reducing redundant
        // state changes if SetViewport() is called multiple times per frame.
        m_CurrentViewport = viewport;
        m_ViewportDirty = true;

        return Result<void>::Success();
    }

    Result<void> OpenGLRenderer::SetScissor(const ScissorRect& scissor)
    {
        if (!m_Initialized)
        {
            return Result<void>::Failure(
                ErrorCode::CoreInvalidState,
                "Renderer is not initialized"
            );
        }

        // Store the scissor rect and mark it as dirty for lazy application in BeginFrame().
        // Similar to viewport, this defers OpenGL state changes to reduce overhead.
        m_CurrentScissor = scissor;
        m_ScissorDirty = true;

        return Result<void>::Success();
    }

    Result<void> OpenGLRenderer::Clear(
        ClearFlags flags,
        const ClearColor& color,
        const ClearDepthStencil& depthStencil)
    {
        if (!m_Initialized)
        {
            return Result<void>::Failure(
                ErrorCode::CoreInvalidState,
                "Renderer is not initialized"
            );
        }

        SB_TRY_VOID(EnsureContextCurrent());

        GLbitfield clearBits = 0;

        if ((flags & ClearFlags::Color) != ClearFlags::None)
        {
            clearBits |= GL_COLOR_BUFFER_BIT;
            glClearColor(color.r, color.g, color.b, color.a);
        }

        if ((flags & ClearFlags::Depth) != ClearFlags::None)
        {
            clearBits |= GL_DEPTH_BUFFER_BIT;
            glClearDepth(depthStencil.depth);
        }

        if ((flags & ClearFlags::Stencil) != ClearFlags::None)
        {
            clearBits |= GL_STENCIL_BUFFER_BIT;
            glClearStencil(depthStencil.stencil);
        }

        if (clearBits != 0)
        {
            glClear(clearBits);
        }

        return Result<void>::Success();
    }

    Result<std::unique_ptr<Buffer>> OpenGLRenderer::CreateBuffer(
        BufferType type,
        size_t size,
        BufferUsage usage,
        const void* data)
    {
        auto bufferResult = OpenGLBuffer::Create(type, size, usage, data);
        if (bufferResult.IsFailure())
        {
            return Result<std::unique_ptr<Buffer>>::Failure(bufferResult.GetError());
        }

        // Convert OpenGLBuffer to Buffer
        std::unique_ptr<Buffer> buffer = std::move(bufferResult).Value();
        return Result<std::unique_ptr<Buffer>>::Success(std::move(buffer));
    }

    Result<std::unique_ptr<Texture>> OpenGLRenderer::CreateTexture(
        TextureType type,
        TextureFormat format,
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        uint32_t mipLevels,
        TextureUsage usage,
        const void* data)
    {
        auto textureResult = OpenGLTexture::Create(type, format, width, height, depth, mipLevels, usage, data);
        if (textureResult.IsFailure())
        {
            return Result<std::unique_ptr<Texture>>::Failure(textureResult.GetError());
        }

        // Convert OpenGLTexture to Texture
        std::unique_ptr<Texture> texture = std::move(textureResult).Value();
        return Result<std::unique_ptr<Texture>>::Success(std::move(texture));
    }

    Result<std::unique_ptr<Shader>> OpenGLRenderer::CreateShader(
        ShaderStage stage,
        const std::string& source,
        const std::string& entryPoint)
    {
        // OpenGL uses "main" as entry point, ignore entryPoint parameter
        (void)entryPoint;

        auto shaderResult = OpenGLShader::Create(stage, source);
        if (shaderResult.IsFailure())
        {
            return Result<std::unique_ptr<Shader>>::Failure(shaderResult.GetError());
        }

        // Convert OpenGLShader to Shader
        std::unique_ptr<Shader> shader = std::move(shaderResult).Value();
        return Result<std::unique_ptr<Shader>>::Success(std::move(shader));
    }

    Result<std::unique_ptr<PipelineState>> OpenGLRenderer::CreatePipelineState(
        Shader* vertexShader,
        Shader* fragmentShader,
        const VertexLayout& vertexLayout,
        PrimitiveTopology topology)
    {
        auto pipelineResult = OpenGLPipelineState::Create(
            vertexShader,
            fragmentShader,
            vertexLayout,
            topology
        );

        if (pipelineResult.IsFailure())
        {
            return Result<std::unique_ptr<PipelineState>>::Failure(pipelineResult.GetError());
        }

        // Convert OpenGLPipelineState to PipelineState
        std::unique_ptr<PipelineState> pipeline = std::move(pipelineResult).Value();
        return Result<std::unique_ptr<PipelineState>>::Success(std::move(pipeline));
    }

    Result<std::unique_ptr<Framebuffer>> OpenGLRenderer::CreateFramebuffer(
        uint32_t width,
        uint32_t height,
        Texture* const* colorAttachments,
        uint32_t colorAttachmentCount,
        Texture* depthStencilAttachment)
    {
        auto framebufferResult = OpenGLFramebuffer::Create(
            width, height, colorAttachments, colorAttachmentCount, depthStencilAttachment
        );

        if (framebufferResult.IsFailure())
        {
            return Result<std::unique_ptr<Framebuffer>>::Failure(framebufferResult.GetError());
        }

        // Convert OpenGLFramebuffer to Framebuffer
        std::unique_ptr<Framebuffer> framebuffer = std::move(framebufferResult).Value();
        return Result<std::unique_ptr<Framebuffer>>::Success(std::move(framebuffer));
    }

    Result<void> OpenGLRenderer::SetFramebuffer(Framebuffer* framebuffer)
    {
        if (!m_Initialized)
        {
            return Result<void>::Failure(
                ErrorCode::CoreInvalidState,
                "Renderer is not initialized"
            );
        }

        SB_TRY_VOID(EnsureContextCurrent());

        bool success = false;
        std::string errorMessage;

        auto bindFunc = [this, framebuffer, &success, &errorMessage]() {
            if (framebuffer == nullptr)
            {
                // Bind default framebuffer (screen)
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                m_CurrentFramebuffer = nullptr;
                success = true;
                return;
            }

            // Cast to OpenGLFramebuffer
            OpenGLFramebuffer* glFramebuffer = dynamic_cast<OpenGLFramebuffer*>(framebuffer);
            if (!glFramebuffer || !glFramebuffer->IsValid())
            {
                success = false;
                errorMessage = "Framebuffer is not a valid OpenGLFramebuffer";
                return;
            }

            // Bind framebuffer
            auto bindResult = glFramebuffer->Bind();
            if (bindResult.IsFailure())
            {
                success = false;
                errorMessage = bindResult.GetError().ToString();
                return;
            }

            m_CurrentFramebuffer = framebuffer;
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

    Result<void> OpenGLRenderer::SetPipelineState(PipelineState* pipelineState)
    {
        if (!m_Initialized)
        {
            return Result<void>::Failure(
                ErrorCode::CoreInvalidState,
                "Renderer is not initialized"
            );
        }

        if (pipelineState == nullptr)
        {
            return Result<void>::Failure(
                ErrorCode::CoreInvalidArgument,
                "Pipeline state cannot be null"
            );
        }

        SB_TRY_VOID(EnsureContextCurrent());

        // Cast to OpenGLPipelineState
        OpenGLPipelineState* glPipeline = dynamic_cast<OpenGLPipelineState*>(pipelineState);
        if (!glPipeline || !glPipeline->IsValid())
        {
            return Result<void>::Failure(
                ErrorCode::CoreInvalidArgument,
                "Pipeline state is not a valid OpenGLPipelineState"
            );
        }

        // Bind pipeline state
        auto bindResult = glPipeline->Bind();
        if (bindResult.IsFailure())
        {
            return Result<void>::Failure(bindResult.GetError());
        }

        m_CurrentPipelineState = pipelineState;
        return Result<void>::Success();
    }

    Result<void> OpenGLRenderer::SetVertexBuffer(Buffer* buffer, size_t offset)
    {
        if (!m_Initialized)
        {
            return Result<void>::Failure(
                ErrorCode::CoreInvalidState,
                "Renderer is not initialized"
            );
        }

        SB_TRY_VOID(EnsureContextCurrent());

        // If no pipeline state is bound, we can't set vertex buffer
        if (m_CurrentPipelineState == nullptr)
        {
            return Result<void>::Failure(
                ErrorCode::CoreInvalidState,
                "No pipeline state is bound. Set a pipeline state before setting vertex buffer."
            );
        }

        // Cast to OpenGLBuffer
        OpenGLBuffer* glBuffer = buffer ? dynamic_cast<OpenGLBuffer*>(buffer) : nullptr;
        
        if (buffer != nullptr && glBuffer == nullptr)
        {
            return Result<void>::Failure(
                ErrorCode::CoreInvalidArgument,
                "Buffer is not a valid OpenGLBuffer"
            );
        }

        // Get vertex layout from pipeline state
        OpenGLPipelineState* glPipeline = dynamic_cast<OpenGLPipelineState*>(m_CurrentPipelineState);
        if (!glPipeline)
        {
            return Result<void>::Failure(
                ErrorCode::CoreInvalidState,
                "Current pipeline state is not a valid OpenGLPipelineState"
            );
        }

        const VertexLayout& vertexLayout = glPipeline->GetVertexLayout();
        const auto& attributes = vertexLayout.GetAttributes();
        uint32_t stride = vertexLayout.GetStride();
        uint32_t vaoId = glPipeline->GetVAO();

        // Validate offset calculations to prevent overflow
        // Check each attribute offset to ensure offset + attr.offset doesn't overflow
        for (const auto& attr : attributes)
        {
            // Check if offset + attr.offset would overflow size_t
            if (offset > SIZE_MAX - static_cast<size_t>(attr.offset))
            {
                return Result<void>::Failure(
                    ErrorCode::CoreInvalidArgument,
                    fmt::format("Vertex buffer offset overflow: offset ({}) + attr.offset ({}) exceeds maximum size_t value",
                        offset, attr.offset)
                );
            }
        }

        // Bind the vertex buffer and set up vertex attributes
        auto bindFunc = [glBuffer, offset, attributes, stride, vaoId]() 
        {
            // Bind VAO first (required for vertex attributes to be stored)
            if (vaoId != 0)
            {
                glBindVertexArray(vaoId);
            }
            
            if (glBuffer)
            {
                // OpenGL handles are always 32-bit, even on 64-bit systems
                // GetNativeHandle() returns a void* that contains a uint32_t value
                void* nativeHandle = glBuffer->GetNativeHandle();
                if (!nativeHandle)
                {
                    SB_CORE_ERROR("OpenGLBuffer::GetNativeHandle() returned null");
                    return;
                }
                
                uintptr_t handleValue = reinterpret_cast<uintptr_t>(nativeHandle);
                // Validate that the handle value fits in uint32_t (should always be true for OpenGL)
                if (handleValue > UINT32_MAX)
                {
                    SB_CORE_ERROR("OpenGL buffer handle value exceeds uint32_t maximum: {}", handleValue);
                    return;
                }
                
                uint32_t bufferId = static_cast<uint32_t>(handleValue);
                glBindBuffer(GL_ARRAY_BUFFER, bufferId);

                // Set up vertex attributes based on the vertex layout

                for (const auto& attr : attributes)
                {
                    glEnableVertexAttribArray(attr.location);

                    // Convert attribute type to OpenGL format
                    GLenum type = GL_FLOAT;
                    GLint size = 1;
                    GLboolean normalized = attr.normalized ? GL_TRUE : GL_FALSE;

                    switch (attr.type)
                    {
                        case VertexAttributeType::Float:   type = GL_FLOAT; size = 1; break;
                        case VertexAttributeType::Float2:  type = GL_FLOAT; size = 2; break;
                        case VertexAttributeType::Float3:  type = GL_FLOAT; size = 3; break;
                        case VertexAttributeType::Float4:  type = GL_FLOAT; size = 4; break;
                        case VertexAttributeType::Int:      type = GL_INT; size = 1; normalized = GL_FALSE; break;
                        case VertexAttributeType::Int2:    type = GL_INT; size = 2; normalized = GL_FALSE; break;
                        case VertexAttributeType::Int3:    type = GL_INT; size = 3; normalized = GL_FALSE; break;
                        case VertexAttributeType::Int4:    type = GL_INT; size = 4; normalized = GL_FALSE; break;
                        case VertexAttributeType::UInt:     type = GL_UNSIGNED_INT; size = 1; normalized = GL_FALSE; break;
                        case VertexAttributeType::UInt2:   type = GL_UNSIGNED_INT; size = 2; normalized = GL_FALSE; break;
                        case VertexAttributeType::UInt3:   type = GL_UNSIGNED_INT; size = 3; normalized = GL_FALSE; break;
                        case VertexAttributeType::UInt4:   type = GL_UNSIGNED_INT; size = 4; normalized = GL_FALSE; break;
                        case VertexAttributeType::Byte:    type = GL_BYTE; size = 1; break;
                        case VertexAttributeType::Byte2:   type = GL_BYTE; size = 2; break;
                        case VertexAttributeType::Byte4:   type = GL_BYTE; size = 4; break;
                        case VertexAttributeType::UByte:   type = GL_UNSIGNED_BYTE; size = 1; break;
                        case VertexAttributeType::UByte2:  type = GL_UNSIGNED_BYTE; size = 2; break;
                        case VertexAttributeType::UByte4:   type = GL_UNSIGNED_BYTE; size = 4; break;
                        case VertexAttributeType::Short:   type = GL_SHORT; size = 1; break;
                        case VertexAttributeType::Short2:  type = GL_SHORT; size = 2; break;
                        case VertexAttributeType::Short4:  type = GL_SHORT; size = 4; break;
                        case VertexAttributeType::UShort:  type = GL_UNSIGNED_SHORT; size = 1; break;
                        case VertexAttributeType::UShort2: type = GL_UNSIGNED_SHORT; size = 2; break;
                        case VertexAttributeType::UShort4: type = GL_UNSIGNED_SHORT; size = 4; break;
                        default: break;
                    }

                    glVertexAttribPointer(
                        attr.location,
                        size,
                        type,
                        normalized,
                        static_cast<GLsizei>(stride),
                        reinterpret_cast<const void*>(static_cast<uintptr_t>(offset + attr.offset))
                    );
                }
            }
            else
            {
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                // Disable all vertex attributes
                for (const auto& attr : attributes)
                {
                    glDisableVertexAttribArray(attr.location);
                }
            }
        };

        MainThreadDispatcher::Get().DispatchSync(bindFunc);

        // Store current vertex buffer
        m_CurrentVertexBuffer = buffer;

        return Result<void>::Success();
    }

    Result<void> OpenGLRenderer::SetIndexBuffer(Buffer* buffer, size_t offset)
    {
        if (!m_Initialized)
        {
            return Result<void>::Failure(
                ErrorCode::CoreInvalidState,
                "Renderer is not initialized"
            );
        }

        SB_TRY_VOID(EnsureContextCurrent());

        // Cast to OpenGLBuffer
        OpenGLBuffer* glBuffer = buffer ? dynamic_cast<OpenGLBuffer*>(buffer) : nullptr;
        
        if (buffer != nullptr && glBuffer == nullptr)
        {
            return Result<void>::Failure(
                ErrorCode::CoreInvalidArgument,
                "Buffer is not a valid OpenGLBuffer"
            );
        }

        // Bind the index buffer
        auto bindFunc = [glBuffer, offset]() {
            if (glBuffer)
            {
                // OpenGL handles are always 32-bit, even on 64-bit systems
                void* nativeHandle = glBuffer->GetNativeHandle();
                if (!nativeHandle)
                {
                    SB_CORE_ERROR("OpenGLBuffer::GetNativeHandle() returned null");
                    return;
                }
                
                uintptr_t handleValue = reinterpret_cast<uintptr_t>(nativeHandle);
                if (handleValue > UINT32_MAX)
                {
                    SB_CORE_ERROR("OpenGL buffer handle value exceeds uint32_t maximum: {}", handleValue);
                    return;
                }
                
                uint32_t bufferId = static_cast<uint32_t>(handleValue);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferId);
            }
            else
            {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            }
        };

        MainThreadDispatcher::Get().DispatchSync(bindFunc);

        // Store current index buffer
        m_CurrentIndexBuffer = buffer;

        return Result<void>::Success();
    }

    Result<void> OpenGLRenderer::Draw(
        uint32_t vertexCount,
        uint32_t instanceCount,
        uint32_t firstVertex,
        uint32_t firstInstance)
    {
        if (!m_Initialized)
        {
            return Result<void>::Failure(
                ErrorCode::CoreInvalidState,
                "Renderer is not initialized"
            );
        }

        if (m_CurrentPipelineState == nullptr)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                "No pipeline state is bound. Call SetPipelineState() first."
            );
        }

        SB_TRY_VOID(EnsureContextCurrent());

        // Get primitive type from pipeline
        OpenGLPipelineState* glPipeline = dynamic_cast<OpenGLPipelineState*>(m_CurrentPipelineState);
        if (!glPipeline)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                "Current pipeline state is not a valid OpenGLPipelineState"
            );
        }

        uint32_t primitiveType = 0;
        bool success = false;
        std::string errorMessage;

        auto drawFunc = [this, glPipeline, &primitiveType, &success, &errorMessage, 
                        vertexCount, instanceCount, firstVertex, firstInstance]() {
            // Ensure VAO is bound (required for drawing)
            uint32_t vaoId = glPipeline->GetVAO();
            if (vaoId == 0)
            {
                success = false;
                errorMessage = "VAO not created for pipeline state";
                return;
            }
            glBindVertexArray(vaoId);

            // Ensure shader program is bound
            if (glPipeline->GetShaderProgram())
            {
                glUseProgram(glPipeline->GetShaderProgram()->GetProgramId());
            }
            else
            {
                success = false;
                errorMessage = "Shader program is null";
                return;
            }

            // Ensure vertex buffer is bound (VAO remembers the binding, but be explicit)
            if (m_CurrentVertexBuffer)
            {
                OpenGLBuffer* glBuffer = dynamic_cast<OpenGLBuffer*>(m_CurrentVertexBuffer);
                if (glBuffer)
                {
                    // OpenGL handles are always 32-bit, even on 64-bit systems
                    void* nativeHandle = glBuffer->GetNativeHandle();
                    if (nativeHandle)
                    {
                        uintptr_t handleValue = reinterpret_cast<uintptr_t>(nativeHandle);
                        if (handleValue <= UINT32_MAX)
                        {
                            uint32_t bufferId = static_cast<uint32_t>(handleValue);
                            glBindBuffer(GL_ARRAY_BUFFER, bufferId);
                        }
                        else
                        {
                            SB_CORE_ERROR("OpenGL buffer handle value exceeds uint32_t maximum: {}", handleValue);
                        }
                    }
                }
            }

            // Get primitive type from pipeline topology using helper function
            // This eliminates code duplication between Draw() and DrawIndexed()
            PrimitiveTopology topology = glPipeline->GetTopology();
            primitiveType = GetGLPrimitiveType(topology);

            // Execute draw call with optimization based on instance parameters
            // OpenGL provides different draw functions optimized for different use cases:
            // - glDrawArrays: Simple non-instanced drawing (most common case)
            // - glDrawArraysInstanced: Instanced drawing starting from instance 0
            // - glDrawArraysInstancedBaseInstance: Instanced drawing with base instance offset
            // We choose the most efficient function based on the parameters to avoid
            // unnecessary overhead from base instance calculations when not needed.
            if (instanceCount == 1)
            {
                // Non-instanced draw: use the simplest and fastest path
                glDrawArrays(primitiveType, static_cast<GLint>(firstVertex), static_cast<GLsizei>(vertexCount));
            }
            else
            {
                // Instanced draw: choose function based on base instance
                if (firstInstance == 0)
                {
                    // No base instance offset: use standard instanced function
                    glDrawArraysInstanced(
                        primitiveType,
                        static_cast<GLint>(firstVertex),
                        static_cast<GLsizei>(vertexCount),
                        static_cast<GLsizei>(instanceCount)
                    );
                }
                else
                {
                    // Base instance offset required: use extended function
                    // glDrawArraysInstancedBaseInstance requires GL 4.2+ or ARB_base_instance extension
                    // This allows drawing a subset of instances from a larger instance buffer
                    glDrawArraysInstancedBaseInstance(
                        primitiveType,
                        static_cast<GLint>(firstVertex),
                        static_cast<GLsizei>(vertexCount),
                        static_cast<GLsizei>(instanceCount),
                        firstInstance
                    );
                }
            }

            GLenum error = glGetError();
            if (error != GL_NO_ERROR)
            {
                success = false;
                errorMessage = fmt::format("Draw call failed: error code {}", error);
                return;
            }

            success = true;
        };

        MainThreadDispatcher::Get().DispatchSync(drawFunc);

        if (!success)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                errorMessage
            );
        }

        return Result<void>::Success();
    }

    Result<void> OpenGLRenderer::DrawIndexed(
        uint32_t indexCount,
        uint32_t instanceCount,
        uint32_t firstIndex,
        int32_t vertexOffset,
        uint32_t firstInstance)
    {
        if (!m_Initialized)
        {
            return Result<void>::Failure(
                ErrorCode::CoreInvalidState,
                "Renderer is not initialized"
            );
        }

        if (m_CurrentPipelineState == nullptr)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                "No pipeline state is bound. Call SetPipelineState() first."
            );
        }

        if (m_CurrentIndexBuffer == nullptr)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                "No index buffer is bound. Call SetIndexBuffer() before DrawIndexed()."
            );
        }

        SB_TRY_VOID(EnsureContextCurrent());

        // Get primitive type from pipeline
        OpenGLPipelineState* glPipeline = dynamic_cast<OpenGLPipelineState*>(m_CurrentPipelineState);
        if (!glPipeline)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                "Current pipeline state is not a valid OpenGLPipelineState"
            );
        }

        uint32_t primitiveType = 0;
        bool success = false;
        std::string errorMessage;

        auto drawFunc = [this, glPipeline, &primitiveType, &success, &errorMessage,
                        indexCount, instanceCount, firstIndex, vertexOffset, firstInstance]() {
            // Get primitive type from pipeline topology using helper function
            PrimitiveTopology topology = glPipeline->GetTopology();
            primitiveType = GetGLPrimitiveType(topology);

            // Execute indexed draw call
            // Note: OpenGL 4.6 supports glDrawElementsBaseVertex and glDrawElementsInstancedBaseVertexBaseInstance
            if (instanceCount == 1)
            {
                if (vertexOffset == 0)
                {
                    glDrawElements(
                        primitiveType,
                        static_cast<GLsizei>(indexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const void*>(static_cast<uintptr_t>(firstIndex * sizeof(uint32_t)))
                    );
                }
                else
                {
                    glDrawElementsBaseVertex(
                        primitiveType,
                        static_cast<GLsizei>(indexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const void*>(static_cast<uintptr_t>(firstIndex * sizeof(uint32_t))),
                        vertexOffset
                    );
                }
            }
            else
            {
                // Use simpler function if base instance and vertex offset are 0
                if (firstInstance == 0 && vertexOffset == 0)
                {
                    glDrawElementsInstanced(
                        primitiveType,
                        static_cast<GLsizei>(indexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const void*>(static_cast<uintptr_t>(firstIndex * sizeof(uint32_t))),
                        static_cast<GLsizei>(instanceCount)
                    );
                }
                else if (firstInstance == 0)
                {
                    // Use glDrawElementsInstancedBaseVertex if available (GL 4.2+)
                    // Otherwise fall back to glDrawElementsInstancedBaseVertexBaseInstance
                    glDrawElementsInstancedBaseVertexBaseInstance(
                        primitiveType,
                        static_cast<GLsizei>(indexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const void*>(static_cast<uintptr_t>(firstIndex * sizeof(uint32_t))),
                        static_cast<GLsizei>(instanceCount),
                        vertexOffset,
                        0 // firstInstance is 0 in this branch
                    );
                }
                else
                {
                    // Full version with base instance and base vertex
                    glDrawElementsInstancedBaseVertexBaseInstance(
                        primitiveType,
                        static_cast<GLsizei>(indexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const void*>(static_cast<uintptr_t>(firstIndex * sizeof(uint32_t))),
                        static_cast<GLsizei>(instanceCount),
                        vertexOffset,
                        firstInstance
                    );
                }
            }

            GLenum error = glGetError();
            if (error != GL_NO_ERROR)
            {
                success = false;
                errorMessage = fmt::format("DrawIndexed call failed: error code {}", error);
                return;
            }

            success = true;
        };

        MainThreadDispatcher::Get().DispatchSync(drawFunc);

        if (!success)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                errorMessage
            );
        }

        return Result<void>::Success();
    }

    void OpenGLRenderer::InitializeCapabilities()
    {
        m_Capabilities.apiName = "OpenGL";
        m_Capabilities.apiVersion = fmt::format("{}.{}", m_Context->GetMajorVersion(), m_Context->GetMinorVersion());
        m_Capabilities.vendorName = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
        m_Capabilities.deviceName = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
        m_Capabilities.driverVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));

        // Query feature support
        GLint value = 0;
        
        // Shader support
        glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &value);
        m_Capabilities.supportsGeometryShaders = (value > 0);
        
        // Compute shaders (OpenGL 4.3+)
        m_Capabilities.supportsComputeShaders = (
            m_Context->GetMajorVersion() >= OpenGLVersionConstants::MIN_MAJOR_VERSION_FOR_COMPUTE &&
            m_Context->GetMinorVersion() >= OpenGLVersionConstants::MIN_MINOR_VERSION_FOR_COMPUTE
        );
        
        // Instancing
        m_Capabilities.supportsInstancing = true; // Always available in OpenGL 3.3+
        
        // Texture arrays
        m_Capabilities.supportsTextureArrays = true;
        m_Capabilities.supportsCubeMaps = true;
        m_Capabilities.supports3DTextures = true;
        
        // MSAA
        glGetIntegerv(GL_MAX_SAMPLES, &value);
        m_Capabilities.maxSamples = static_cast<uint32_t>(value);
        m_Capabilities.supportsMSAA = (value > 0);
        
        // Anisotropic filtering
        glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &value);
        m_Capabilities.maxTextureAnisotropy = static_cast<uint32_t>(value);
        m_Capabilities.supportsAnisotropicFiltering = (value > 1);
        
        // Multiple render targets
        glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &value);
        m_Capabilities.maxColorAttachments = static_cast<uint32_t>(value);
        m_Capabilities.supportsMultipleRenderTargets = (value > 1);
        
        // Texture limits
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);
        m_Capabilities.maxTextureSize = static_cast<uint32_t>(value);
        
        glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &value);
        m_Capabilities.max3DTextureSize = static_cast<uint32_t>(value);
        
        glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &value);
        m_Capabilities.maxCubeMapSize = static_cast<uint32_t>(value);
        
        glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &value);
        m_Capabilities.maxArrayTextureLayers = static_cast<uint32_t>(value);
        
        // Buffer limits (OpenGL 3.1+)
        // GL_MAX_UNIFORM_BLOCK_SIZE (0x8A30) is the same as GL_MAX_UNIFORM_BUFFER_SIZE
        if (m_Context->GetMajorVersion() > OpenGLVersionConstants::MIN_MAJOR_VERSION_FOR_UNIFORM_BUFFERS ||
            (m_Context->GetMajorVersion() == OpenGLVersionConstants::MIN_MAJOR_VERSION_FOR_UNIFORM_BUFFERS &&
             m_Context->GetMinorVersion() >= OpenGLVersionConstants::MIN_MINOR_VERSION_FOR_UNIFORM_BUFFERS))
        {
            glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &value);
            m_Capabilities.maxUniformBufferSize = static_cast<uint32_t>(value);
        }
        else
        {
            m_Capabilities.maxUniformBufferSize = 0;
        }
        
        // Vertex attributes
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &value);
        m_Capabilities.maxVertexAttributes = static_cast<uint32_t>(value);
        
        // Viewports
        glGetIntegerv(GL_MAX_VIEWPORTS, &value);
        m_Capabilities.maxViewports = static_cast<uint32_t>(value);
    }

    uint32_t OpenGLRenderer::GetGLPrimitiveType(PrimitiveTopology topology)
    {
        switch (topology)
        {
            case PrimitiveTopology::Points:                return GL_POINTS;
            case PrimitiveTopology::Lines:                  return GL_LINES;
            case PrimitiveTopology::LineStrip:              return GL_LINE_STRIP;
            case PrimitiveTopology::Triangles:              return GL_TRIANGLES;
            case PrimitiveTopology::TriangleStrip:          return GL_TRIANGLE_STRIP;
            case PrimitiveTopology::TriangleFan:             return GL_TRIANGLE_FAN;
            case PrimitiveTopology::LinesAdjacency:         return GL_LINES_ADJACENCY;
            case PrimitiveTopology::LineStripAdjacency:     return GL_LINE_STRIP_ADJACENCY;
            case PrimitiveTopology::TrianglesAdjacency:     return GL_TRIANGLES_ADJACENCY;
            case PrimitiveTopology::TriangleStripAdjacency: return GL_TRIANGLE_STRIP_ADJACENCY;
            case PrimitiveTopology::Patches:                return GL_PATCHES;
            default:                                        return GL_TRIANGLES;
        }
    }

    Result<void> OpenGLRenderer::EnsureContextCurrent()
    {
        if (!m_Context)
        {
            return Result<void>::Failure(
                ErrorCode::CoreInvalidState,
                "OpenGL context is null"
            );
        }

        if (!m_Context->IsCurrent())
        {
            return m_Context->MakeCurrent();
        }

        return Result<void>::Success();
    }

} // namespace Sabora
