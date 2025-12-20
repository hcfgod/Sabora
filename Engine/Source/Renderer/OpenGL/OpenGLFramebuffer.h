#pragma once

#include "Renderer/Resources/Framebuffer.h"
#include <cstdint>
#include <vector>
#include <memory>

namespace Sabora
{
    class Texture;

    /**
     * @brief OpenGL implementation of the Framebuffer interface.
     * 
     * OpenGLFramebuffer provides OpenGL-specific framebuffer operations including
     * multiple color attachments and depth/stencil attachments.
     */
    class OpenGLFramebuffer : public Framebuffer
    {
    public:
        /**
         * @brief Create a framebuffer with attachments.
         * @param width Framebuffer width.
         * @param height Framebuffer height.
         * @param colorAttachments Array of color texture attachments.
         * @param colorAttachmentCount Number of color attachments.
         * @param depthStencilAttachment Depth/stencil texture attachment (optional).
         * @return Result containing the created framebuffer, or an error.
         */
        [[nodiscard]] static Result<std::unique_ptr<OpenGLFramebuffer>> Create(
            uint32_t width,
            uint32_t height,
            Texture* const* colorAttachments,
            uint32_t colorAttachmentCount,
            Texture* depthStencilAttachment = nullptr
        );

        /**
         * @brief Destructor - destroys the OpenGL framebuffer.
         */
        ~OpenGLFramebuffer() override;

        // Non-copyable, movable
        OpenGLFramebuffer(const OpenGLFramebuffer&) = delete;
        OpenGLFramebuffer& operator=(const OpenGLFramebuffer&) = delete;
        OpenGLFramebuffer(OpenGLFramebuffer&& other) noexcept;
        OpenGLFramebuffer& operator=(OpenGLFramebuffer&& other) noexcept;

        //==========================================================================
        // Framebuffer Interface Implementation
        //==========================================================================

        [[nodiscard]] uint32_t GetWidth() const override { return m_Width; }
        [[nodiscard]] uint32_t GetHeight() const override { return m_Height; }
        [[nodiscard]] uint32_t GetColorAttachmentCount() const override;
        [[nodiscard]] Texture* GetColorAttachment(uint32_t index) const override;
        [[nodiscard]] Texture* GetDepthStencilAttachment() const override;
        [[nodiscard]] bool IsComplete() const override;
        [[nodiscard]] void* GetNativeHandle() const override;
        [[nodiscard]] bool IsValid() const override;

        /**
         * @brief Bind this framebuffer for rendering (internal use by renderer).
         * @return Result indicating success or failure.
         */
        [[nodiscard]] Result<void> Bind();

    private:
        /**
         * @brief Private constructor - use Create() factory method.
         * @param framebufferId OpenGL framebuffer ID.
         * @param width Framebuffer width.
         * @param height Framebuffer height.
         * @param colorAttachments Color attachment textures (not owned).
         * @param depthStencilAttachment Depth/stencil attachment texture (not owned).
         */
        OpenGLFramebuffer(
            uint32_t framebufferId,
            uint32_t width,
            uint32_t height,
            std::vector<Texture*> colorAttachments,
            Texture* depthStencilAttachment
        ) noexcept;

        uint32_t m_FramebufferId = 0;              ///< OpenGL framebuffer ID
        uint32_t m_Width = 0;                      ///< Framebuffer width
        uint32_t m_Height = 0;                     ///< Framebuffer height
        std::vector<Texture*> m_ColorAttachments;  ///< Color attachment textures (not owned)
        Texture* m_DepthStencilAttachment = nullptr; ///< Depth/stencil attachment (not owned)
    };

} // namespace Sabora
