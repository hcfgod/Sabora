#pragma once

#include "Renderer/Core/Renderer.h"
#include "Renderer/Core/RendererTypes.h"
#include "Renderer/Core/RendererCapabilities.h"
#include <memory>
#include <mutex>

namespace Sabora
{
    class Window;
    class OpenGLContext;

    /**
     * @brief OpenGL 4.6 Core Profile renderer implementation.
     * 
     * OpenGLRenderer provides a full implementation of the Renderer interface
     * using OpenGL 4.6 Core Profile. It manages OpenGL resources, state, and
     * rendering operations.
     * 
     * Thread Safety:
     * - Resource creation methods are thread-safe and use MainThreadDispatcher
     * - Rendering operations must be called from the main thread
     * - State queries are thread-safe
     */
    class OpenGLRenderer : public Renderer
    {
    public:
        OpenGLRenderer();
        ~OpenGLRenderer() override;

        // Non-copyable, movable
        OpenGLRenderer(const OpenGLRenderer&) = delete;
        OpenGLRenderer& operator=(const OpenGLRenderer&) = delete;
        OpenGLRenderer(OpenGLRenderer&&) noexcept = default;
        OpenGLRenderer& operator=(OpenGLRenderer&&) noexcept = default;

        //==========================================================================
        // Initialization and Lifecycle
        //==========================================================================

        [[nodiscard]] Result<void> Initialize(Window* window) override;
        void Shutdown() override;
        [[nodiscard]] RendererAPI GetAPI() const override { return RendererAPI::OpenGL; }
        [[nodiscard]] RenderContext* GetContext() const override;

        //==========================================================================
        // Frame Management
        //==========================================================================

        [[nodiscard]] Result<void> BeginFrame() override;
        [[nodiscard]] Result<void> EndFrame() override;

        //==========================================================================
        // Viewport and Scissor
        //==========================================================================

        [[nodiscard]] Result<void> SetViewport(const Viewport& viewport) override;
        [[nodiscard]] Result<void> SetScissor(const ScissorRect& scissor) override;

        //==========================================================================
        // Clear Operations
        //==========================================================================

        [[nodiscard]] Result<void> Clear(
            ClearFlags flags,
            const ClearColor& color = ClearColor{},
            const ClearDepthStencil& depthStencil = ClearDepthStencil{}
        ) override;

        //==========================================================================
        // Resource Creation
        //==========================================================================

        [[nodiscard]] Result<std::unique_ptr<Buffer>> CreateBuffer(
            BufferType type,
            size_t size,
            BufferUsage usage,
            const void* data = nullptr
        ) override;

        [[nodiscard]] Result<std::unique_ptr<Texture>> CreateTexture(
            TextureType type,
            TextureFormat format,
            uint32_t width,
            uint32_t height,
            uint32_t depth,
            uint32_t mipLevels,
            TextureUsage usage,
            const void* data = nullptr
        ) override;

        [[nodiscard]] Result<std::unique_ptr<Shader>> CreateShader(
            ShaderStage stage,
            const std::string& source,
            const std::string& entryPoint = "main"
        ) override;

        [[nodiscard]] Result<std::unique_ptr<PipelineState>> CreatePipelineState(
            Shader* vertexShader,
            Shader* fragmentShader,
            const VertexLayout& vertexLayout,
            PrimitiveTopology topology
        ) override;

        [[nodiscard]] Result<std::unique_ptr<Framebuffer>> CreateFramebuffer(
            uint32_t width,
            uint32_t height,
            Texture* const* colorAttachments,
            uint32_t colorAttachmentCount,
            Texture* depthStencilAttachment = nullptr
        ) override;

        //==========================================================================
        // Rendering Operations
        //==========================================================================

        [[nodiscard]] Result<void> SetFramebuffer(Framebuffer* framebuffer) override;
        [[nodiscard]] Result<void> SetPipelineState(PipelineState* pipelineState) override;
        [[nodiscard]] Result<void> SetVertexBuffer(Buffer* buffer, size_t offset = 0) override;
        [[nodiscard]] Result<void> SetIndexBuffer(Buffer* buffer, size_t offset = 0) override;
        [[nodiscard]] Result<void> Draw(
            uint32_t vertexCount,
            uint32_t instanceCount = 1,
            uint32_t firstVertex = 0,
            uint32_t firstInstance = 0
        ) override;
        [[nodiscard]] Result<void> DrawIndexed(
            uint32_t indexCount,
            uint32_t instanceCount = 1,
            uint32_t firstIndex = 0,
            int32_t vertexOffset = 0,
            uint32_t firstInstance = 0
        ) override;

        //==========================================================================
        // Capabilities
        //==========================================================================

        /**
         * @brief Get renderer capabilities.
         * @return Reference to capabilities structure.
         */
        [[nodiscard]] const RendererCapabilities& GetCapabilities() const noexcept { return m_Capabilities; }

        /**
         * @brief Convert PrimitiveTopology enum to OpenGL primitive type.
         * @param topology The primitive topology to convert.
         * @return OpenGL primitive type constant (e.g., GL_TRIANGLES, GL_LINES).
         * 
         * This helper function eliminates code duplication between Draw() and DrawIndexed().
         * Made public for unit testing purposes.
         */
        [[nodiscard]] static uint32_t GetGLPrimitiveType(PrimitiveTopology topology);

    private:
        /**
         * @brief Initialize renderer capabilities by querying OpenGL.
         */
        void InitializeCapabilities();

        /**
         * @brief Ensure OpenGL context is current on this thread.
         * @return Result indicating success or failure.
         */
        [[nodiscard]] Result<void> EnsureContextCurrent();

        std::unique_ptr<OpenGLContext> m_Context;
        Window* m_Window = nullptr;
        RendererCapabilities m_Capabilities;
        bool m_Initialized = false;

        // Current state
        Viewport m_CurrentViewport;
        ScissorRect m_CurrentScissor;
        bool m_ViewportDirty = true;
        bool m_ScissorDirty = true;
        
        PipelineState* m_CurrentPipelineState = nullptr;  ///< Current bound pipeline state
        Framebuffer* m_CurrentFramebuffer = nullptr;      ///< Current bound framebuffer (nullptr = default)
        Buffer* m_CurrentVertexBuffer = nullptr;          ///< Current bound vertex buffer
        Buffer* m_CurrentIndexBuffer = nullptr;           ///< Current bound index buffer
    };

} // namespace Sabora
