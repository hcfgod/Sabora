#pragma once

#include "Renderer/Core/RenderContext.h"
#include <SDL3/SDL.h>
#include <thread>
#include <mutex>

// Forward declarations
struct SDL_Window;

namespace Sabora
{
    class Window;

    /**
     * @brief OpenGL context implementation using SDL.
     * 
     * OpenGLContext manages an OpenGL rendering context created through SDL.
     * It handles context creation, destruction, and thread-local context binding.
     * 
     * Thread Safety:
     * - Context creation/destruction must be done on the main thread
     * - Context binding uses thread-local storage for safety
     * - All operations are thread-safe
     */
    class OpenGLContext : public RenderContext
    {
    public:
        /**
         * @brief Create a new OpenGL context.
         * @param window The window to create the context for.
         * @param shareContext Optional context to share resources with.
         * @return Result containing the created context, or an error.
         */
        [[nodiscard]] static Result<std::unique_ptr<OpenGLContext>> Create(
            Window* window,
            RenderContext* shareContext = nullptr
        );

        ~OpenGLContext() override;

        // Non-copyable, movable
        OpenGLContext(const OpenGLContext&) = delete;
        OpenGLContext& operator=(const OpenGLContext&) = delete;
        OpenGLContext(OpenGLContext&& other) noexcept;
        OpenGLContext& operator=(OpenGLContext&& other) noexcept;

        [[nodiscard]] Result<void> MakeCurrent() override;
        [[nodiscard]] Result<void> ReleaseCurrent() override;
        [[nodiscard]] bool IsCurrent() const override;
        [[nodiscard]] Result<void> SwapBuffers() override;
        [[nodiscard]] void* GetNativeHandle() const override;
        [[nodiscard]] bool IsValid() const override;

        /**
         * @brief Get the OpenGL version major number.
         * @return OpenGL major version.
         */
        [[nodiscard]] int32_t GetMajorVersion() const noexcept { return m_MajorVersion; }

        /**
         * @brief Get the OpenGL version minor number.
         * @return OpenGL minor version.
         */
        [[nodiscard]] int32_t GetMinorVersion() const noexcept { return m_MinorVersion; }

        /**
         * @brief Check if GLAD was successfully loaded.
         * @return True if GLAD functions are loaded.
         */
        [[nodiscard]] bool IsGLADLoaded() const noexcept { return m_GLADLoaded; }

    private:
        /**
         * @brief Private constructor - use Create() factory method.
         * @param window SDL window handle.
         * @param glContext SDL OpenGL context handle.
         */
        OpenGLContext(SDL_Window* window, SDL_GLContext glContext);

        /**
         * @brief Load OpenGL functions using GLAD.
         * @return Result indicating success or failure.
         */
        [[nodiscard]] Result<void> LoadGLAD();

        SDL_Window* m_Window = nullptr;
        SDL_GLContext m_GLContext = nullptr;
        int32_t m_MajorVersion = 0;
        int32_t m_MinorVersion = 0;
        bool m_GLADLoaded = false;

        // Thread-local storage for tracking current context per thread
        static thread_local OpenGLContext* s_CurrentContext;
    };

} // namespace Sabora
