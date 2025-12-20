#pragma once

#include "Core/Result.h"
#include "Renderer/Core/RendererTypes.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace Sabora
{
    class Texture;

    /**
     * @brief Abstract framebuffer interface.
     * 
     * Framebuffer represents a render target that can have multiple color
     * attachments and a depth/stencil attachment. It provides a unified
     * interface for framebuffer operations across different APIs.
     * 
     * Thread Safety:
     * - Framebuffer creation/destruction is thread-safe (uses MainThreadDispatcher)
     * - Framebuffer operations should be done from the main thread
     */
    class Framebuffer
    {
    public:
        virtual ~Framebuffer() = default;

        /**
         * @brief Get the framebuffer width.
         * @return Width in pixels.
         */
        [[nodiscard]] virtual uint32_t GetWidth() const = 0;

        /**
         * @brief Get the framebuffer height.
         * @return Height in pixels.
         */
        [[nodiscard]] virtual uint32_t GetHeight() const = 0;

        /**
         * @brief Get the number of color attachments.
         * @return Number of color attachments.
         */
        [[nodiscard]] virtual uint32_t GetColorAttachmentCount() const = 0;

        /**
         * @brief Get a color attachment.
         * @param index Attachment index (0-based).
         * @return Pointer to the texture, or nullptr if index is invalid.
         */
        [[nodiscard]] virtual Texture* GetColorAttachment(uint32_t index) const = 0;

        /**
         * @brief Get the depth/stencil attachment.
         * @return Pointer to the depth/stencil texture, or nullptr if none.
         */
        [[nodiscard]] virtual Texture* GetDepthStencilAttachment() const = 0;

        /**
         * @brief Check if the framebuffer is complete and ready to use.
         * @return True if the framebuffer is complete.
         */
        [[nodiscard]] virtual bool IsComplete() const = 0;

        /**
         * @brief Get the native API handle (for advanced use).
         * @return Opaque pointer to the native framebuffer handle.
         */
        [[nodiscard]] virtual void* GetNativeHandle() const = 0;

        /**
         * @brief Check if the framebuffer is valid.
         * @return True if the framebuffer is valid and can be used.
         */
        [[nodiscard]] virtual bool IsValid() const = 0;

    protected:
        Framebuffer() = default;

        // Non-copyable, movable
        Framebuffer(const Framebuffer&) = delete;
        Framebuffer& operator=(const Framebuffer&) = delete;
        Framebuffer(Framebuffer&&) noexcept = default;
        Framebuffer& operator=(Framebuffer&&) noexcept = default;
    };

} // namespace Sabora
