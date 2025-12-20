#pragma once

#include "Core/Result.h"
#include <memory>
#include <thread>

// Forward declarations
struct SDL_Window;

namespace Sabora
{
    class Window;

    /**
     * @brief Graphics context abstraction for managing API-specific contexts.
     * 
     * RenderContext provides a thread-safe interface for managing graphics API
     * contexts (OpenGL context, Vulkan device, etc.). It handles context creation,
     * destruction, and thread-local context binding.
     * 
     * Thread Safety:
     * - Context creation/destruction must be done on the main thread
     * - Context binding uses thread-local storage for safety
     * - All operations are thread-safe
     */
    class RenderContext
    {
    public:
        virtual ~RenderContext() = default;

        /**
         * @brief Create a new render context.
         * @param window The window to create the context for.
         * @param shareContext Optional context to share resources with.
         * @return Result containing the created context, or an error.
         */
        [[nodiscard]] static Result<std::unique_ptr<RenderContext>> Create(
            Window* window,
            RenderContext* shareContext = nullptr
        );

        /**
         * @brief Make this context current on the calling thread.
         * @return Result indicating success or failure.
         * 
         * This binds the context to the current thread. The context will remain
         * current until another context is made current or this context is destroyed.
         */
        [[nodiscard]] virtual Result<void> MakeCurrent() = 0;

        /**
         * @brief Unbind this context from the current thread.
         * @return Result indicating success or failure.
         */
        [[nodiscard]] virtual Result<void> ReleaseCurrent() = 0;

        /**
         * @brief Check if this context is current on the calling thread.
         * @return True if the context is current on this thread.
         */
        [[nodiscard]] virtual bool IsCurrent() const = 0;

        /**
         * @brief Swap the front and back buffers (present the frame).
         * @return Result indicating success or failure.
         */
        [[nodiscard]] virtual Result<void> SwapBuffers() = 0;

        /**
         * @brief Get the underlying API-specific context handle.
         * @return Opaque pointer to the context handle (for advanced use).
         */
        [[nodiscard]] virtual void* GetNativeHandle() const = 0;

        /**
         * @brief Check if the context is valid.
         * @return True if the context is valid and can be used.
         */
        [[nodiscard]] virtual bool IsValid() const = 0;

    protected:
        RenderContext() = default;

        // Non-copyable, movable
        RenderContext(const RenderContext&) = delete;
        RenderContext& operator=(const RenderContext&) = delete;
        RenderContext(RenderContext&&) noexcept = default;
        RenderContext& operator=(RenderContext&&) noexcept = default;
    };

} // namespace Sabora
