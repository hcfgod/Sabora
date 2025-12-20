#pragma once

#include "Core/Result.h"
#include "Renderer/Core/RenderContext.h"
#include "Renderer/Core/RendererTypes.h"
#include "Renderer/Resources/VertexLayout.h"
#include "Renderer/Resources/Buffer.h"
#include "Renderer/Resources/Texture.h"
#include "Renderer/Resources/Shader.h"
#include "Renderer/Resources/PipelineState.h"
#include "Renderer/Resources/Framebuffer.h"
#include <memory>
#include <string>

// Forward declarations
namespace Sabora
{
    class Window;
}

namespace Sabora
{
    /**
     * @brief Abstract renderer interface for graphics API abstraction.
     * 
     * Renderer provides a high-level interface for rendering operations that
     * is independent of the underlying graphics API. All API-specific implementations
     * (OpenGL, Vulkan, DirectX 12, Metal) inherit from this base class.
     * 
     * Thread Safety:
     * - Resource creation methods are thread-safe and use MainThreadDispatcher
     * - Rendering operations (BeginFrame, EndFrame, Draw, etc.) must be called from main thread
     * - Query operations (GetCapabilities, etc.) are thread-safe
     * 
     * Error Handling:
     * - All methods return Result<T> for explicit error handling
     * - Errors include source location information for debugging
     */
    class Renderer
    {
    public:
        virtual ~Renderer() = default;

        //==========================================================================
        // Initialization and Lifecycle
        //==========================================================================

        /**
         * @brief Initialize the renderer with a window.
         * @param window The window to render to.
         * @return Result indicating success or failure.
         */
        [[nodiscard]] virtual Result<void> Initialize(Window* window) = 0;

        /**
         * @brief Shutdown the renderer and cleanup all resources.
         * 
         * This should be called before destroying the renderer. All resources
         * created by this renderer will be automatically cleaned up.
         */
        virtual void Shutdown() = 0;

        /**
         * @brief Get the renderer API type.
         * @return The renderer API (OpenGL, Vulkan, etc.).
         */
        [[nodiscard]] virtual RendererAPI GetAPI() const = 0;

        /**
         * @brief Get the render context.
         * @return Pointer to the render context, or nullptr if not initialized.
         */
        [[nodiscard]] virtual RenderContext* GetContext() const = 0;

        //==========================================================================
        // Frame Management
        //==========================================================================

        /**
         * @brief Begin a new frame for rendering.
         * @return Result indicating success or failure.
         * 
         * This must be called before any rendering operations. It prepares
         * the renderer for a new frame and may perform operations like acquiring
         * the swapchain image.
         */
        [[nodiscard]] virtual Result<void> BeginFrame() = 0;

        /**
         * @brief End the current frame and present it.
         * @return Result indicating success or failure.
         * 
         * This must be called after all rendering operations for the frame
         * are complete. It presents the rendered frame to the screen.
         */
        [[nodiscard]] virtual Result<void> EndFrame() = 0;

        //==========================================================================
        // Viewport and Scissor
        //==========================================================================

        /**
         * @brief Set the viewport dimensions.
         * @param viewport The viewport configuration.
         * @return Result indicating success or failure.
         */
        [[nodiscard]] virtual Result<void> SetViewport(const Viewport& viewport) = 0;

        /**
         * @brief Set the scissor rectangle.
         * @param scissor The scissor rectangle.
         * @return Result indicating success or failure.
         */
        [[nodiscard]] virtual Result<void> SetScissor(const ScissorRect& scissor) = 0;

        //==========================================================================
        // Clear Operations
        //==========================================================================

        /**
         * @brief Clear the current render target(s).
         * @param flags Which buffers to clear (color, depth, stencil).
         * @param color Clear color value (used if flags includes Color).
         * @param depthStencil Clear depth/stencil value (used if flags includes Depth/Stencil).
         * @return Result indicating success or failure.
         */
        [[nodiscard]] virtual Result<void> Clear(
            ClearFlags flags,
            const ClearColor& color = ClearColor{},
            const ClearDepthStencil& depthStencil = ClearDepthStencil{}
        ) = 0;

        //==========================================================================
        // Resource Creation
        //==========================================================================

        /**
         * @brief Create a GPU buffer.
         * @param type Buffer type (vertex, index, uniform, etc.).
         * @param size Buffer size in bytes.
         * @param usage Buffer usage hint (static, dynamic, stream).
         * @param data Optional initial data (nullptr for uninitialized buffer).
         * @return Result containing the created buffer, or an error.
         * 
         * Thread Safety: This method is thread-safe and queues the actual creation
         * on the main thread via MainThreadDispatcher.
         */
        [[nodiscard]] virtual Result<std::unique_ptr<Buffer>> CreateBuffer(
            BufferType type,
            size_t size,
            BufferUsage usage,
            const void* data = nullptr
        ) = 0;

        /**
         * @brief Create a texture.
         * @param type Texture type (2D, 3D, Cube, etc.).
         * @param format Texture pixel format.
         * @param width Texture width.
         * @param height Texture height (1 for 1D textures).
         * @param depth Texture depth (1 for 2D textures).
         * @param mipLevels Number of mipmap levels (0 for automatic).
         * @param usage Texture usage flags.
         * @param data Optional initial pixel data.
         * @return Result containing the created texture, or an error.
         * 
         * Thread Safety: This method is thread-safe and queues the actual creation
         * on the main thread via MainThreadDispatcher.
         */
        [[nodiscard]] virtual Result<std::unique_ptr<Texture>> CreateTexture(
            TextureType type,
            TextureFormat format,
            uint32_t width,
            uint32_t height,
            uint32_t depth,
            uint32_t mipLevels,
            TextureUsage usage,
            const void* data = nullptr
        ) = 0;

        /**
         * @brief Create a shader from source code.
         * @param stage Shader stage (vertex, fragment, etc.).
         * @param source Shader source code.
         * @param entryPoint Entry point name (for HLSL/Metal, "main" for GLSL).
         * @return Result containing the created shader, or an error.
         * 
         * Thread Safety: This method is thread-safe and queues the actual creation
         * on the main thread via MainThreadDispatcher.
         */
        [[nodiscard]] virtual Result<std::unique_ptr<Shader>> CreateShader(
            ShaderStage stage,
            const std::string& source,
            const std::string& entryPoint = "main"
        ) = 0;

        /**
         * @brief Create a render pipeline state.
         * @param vertexShader Vertex shader (required).
         * @param fragmentShader Fragment shader (optional, nullptr for compute pipelines).
         * @param vertexLayout Vertex attribute layout.
         * @param topology Primitive topology.
         * @return Result containing the created pipeline state, or an error.
         * 
         * Thread Safety: This method is thread-safe and queues the actual creation
         * on the main thread via MainThreadDispatcher.
         */
        [[nodiscard]] virtual Result<std::unique_ptr<PipelineState>> CreatePipelineState(
            Shader* vertexShader,
            Shader* fragmentShader,
            const VertexLayout& vertexLayout,
            PrimitiveTopology topology
        ) = 0;

        /**
         * @brief Create a framebuffer.
         * @param width Framebuffer width.
         * @param height Framebuffer height.
         * @param colorAttachments Array of color texture attachments (nullptr for no color).
         * @param colorAttachmentCount Number of color attachments.
         * @param depthStencilAttachment Depth/stencil texture attachment (nullptr for none).
         * @return Result containing the created framebuffer, or an error.
         * 
         * Thread Safety: This method is thread-safe and queues the actual creation
         * on the main thread via MainThreadDispatcher.
         */
        [[nodiscard]] virtual Result<std::unique_ptr<Framebuffer>> CreateFramebuffer(
            uint32_t width,
            uint32_t height,
            Texture* const* colorAttachments,
            uint32_t colorAttachmentCount,
            Texture* depthStencilAttachment = nullptr
        ) = 0;

        //==========================================================================
        // Rendering Operations
        //==========================================================================

        /**
         * @brief Set the active framebuffer.
         * @param framebuffer Framebuffer to bind (nullptr for default framebuffer).
         * @return Result indicating success or failure.
         */
        [[nodiscard]] virtual Result<void> SetFramebuffer(Framebuffer* framebuffer) = 0;

        /**
         * @brief Set the active pipeline state.
         * @param pipelineState Pipeline state to bind.
         * @return Result indicating success or failure.
         */
        [[nodiscard]] virtual Result<void> SetPipelineState(PipelineState* pipelineState) = 0;

        /**
         * @brief Set the vertex buffer for drawing.
         * @param buffer Vertex buffer to bind (nullptr to unbind).
         * @param offset Offset in bytes from the start of the buffer (0 for start).
         * @return Result indicating success or failure.
         * 
         * This binds a vertex buffer for use in subsequent draw calls.
         * The buffer must be compatible with the current pipeline state's vertex layout.
         */
        [[nodiscard]] virtual Result<void> SetVertexBuffer(Buffer* buffer, size_t offset = 0) = 0;

        /**
         * @brief Set the index buffer for drawing.
         * @param buffer Index buffer to bind (nullptr to unbind).
         * @param offset Offset in bytes from the start of the buffer (0 for start).
         * @return Result indicating success or failure.
         * 
         * This binds an index buffer for use in subsequent DrawIndexed calls.
         */
        [[nodiscard]] virtual Result<void> SetIndexBuffer(Buffer* buffer, size_t offset = 0) = 0;

        /**
         * @brief Draw primitives.
         * @param vertexCount Number of vertices to draw.
         * @param instanceCount Number of instances to draw (1 for non-instanced).
         * @param firstVertex Index of first vertex.
         * @param firstInstance Index of first instance (0 for non-instanced).
         * @return Result indicating success or failure.
         */
        [[nodiscard]] virtual Result<void> Draw(
            uint32_t vertexCount,
            uint32_t instanceCount = 1,
            uint32_t firstVertex = 0,
            uint32_t firstInstance = 0
        ) = 0;

        /**
         * @brief Draw indexed primitives.
         * @param indexCount Number of indices to draw.
         * @param instanceCount Number of instances to draw (1 for non-instanced).
         * @param firstIndex Index of first index.
         * @param vertexOffset Offset to add to vertex indices.
         * @param firstInstance Index of first instance (0 for non-instanced).
         * @return Result indicating success or failure.
         */
        [[nodiscard]] virtual Result<void> DrawIndexed(
            uint32_t indexCount,
            uint32_t instanceCount = 1,
            uint32_t firstIndex = 0,
            int32_t vertexOffset = 0,
            uint32_t firstInstance = 0
        ) = 0;

    protected:
        Renderer() = default;

        // Non-copyable, movable
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&) noexcept = default;
        Renderer& operator=(Renderer&&) noexcept = default;
    };

} // namespace Sabora
