#pragma once

#include "Core/Result.h"
#include "Renderer/Core/Renderer.h"
#include "Renderer/Core/RendererTypes.h"
#include <memory>
#include <mutex>

namespace Sabora
{
    class Window;

    /**
     * @brief Singleton manager for renderer lifecycle and API selection.
     * 
     * RendererManager provides global access to the renderer instance and handles
     * renderer creation, API selection, and lifecycle management. It automatically
     * falls back to available APIs if the preferred API is unavailable.
     * 
     * Thread Safety:
     * - All methods are thread-safe
     * - Renderer creation/initialization uses MainThreadDispatcher for safety
     * 
     * Usage:
     * @code
     *   // Initialize renderer with preferred API
     *   auto result = RendererManager::Get().Initialize(window, RendererAPI::OpenGL);
     *   if (result.IsFailure()) {
     *       // Handle error
     *   }
     *   
     *   // Get renderer instance
     *   auto* renderer = RendererManager::Get().GetRenderer();
     *   
     *   // Shutdown
     *   RendererManager::Get().Shutdown();
     * @endcode
     */
    class RendererManager
    {
    public:
        /**
         * @brief Get the singleton instance of RendererManager.
         * @return Reference to the RendererManager instance.
         */
        [[nodiscard]] static RendererManager& Get()
        {
            static RendererManager instance;
            return instance;
        }

        /**
         * @brief Initialize the renderer with the specified API.
         * @param window The window to render to.
         * @param preferredAPI The preferred graphics API (OpenGL, Vulkan, etc.).
         * @return Result indicating success or failure.
         * 
         * This will attempt to create a renderer with the preferred API. If that
         * fails, it will automatically try fallback APIs in order of preference.
         * 
         * Thread Safety: This method is thread-safe but should typically be called
         * from the main thread during application initialization.
         */
        [[nodiscard]] Result<void> Initialize(Window* window, RendererAPI preferredAPI = RendererAPI::OpenGL);

        /**
         * @brief Shutdown the renderer and cleanup all resources.
         * 
         * This should be called before application shutdown. All renderer resources
         * will be automatically cleaned up.
         * 
         * Thread Safety: This method is thread-safe but should typically be called
         * from the main thread.
         */
        void Shutdown();

        /**
         * @brief Get the current renderer instance.
         * @return Pointer to the renderer, or nullptr if not initialized.
         * 
         * Thread Safety: This method is thread-safe.
         */
        [[nodiscard]] Renderer* GetRenderer() const noexcept;

        /**
         * @brief Get the current renderer API.
         * @return The renderer API, or RendererAPI::None if not initialized.
         * 
         * Thread Safety: This method is thread-safe.
         */
        [[nodiscard]] RendererAPI GetAPI() const noexcept;

        /**
         * @brief Check if the renderer is initialized.
         * @return True if the renderer is initialized and ready to use.
         * 
         * Thread Safety: This method is thread-safe.
         */
        [[nodiscard]] bool IsInitialized() const noexcept;

        /**
         * @brief Check if a specific API is available on this system.
         * @param api The API to check.
         * @return True if the API is available.
         * 
         * Thread Safety: This method is thread-safe.
         */
        [[nodiscard]] static bool IsAPIAvailable(RendererAPI api);

    private:
        RendererManager() = default;
        ~RendererManager() = default;

        // Non-copyable, non-movable
        RendererManager(const RendererManager&) = delete;
        RendererManager& operator=(const RendererManager&) = delete;
        RendererManager(RendererManager&&) = delete;
        RendererManager& operator=(RendererManager&&) = delete;

        /**
         * @brief Create a renderer instance for the specified API.
         * @param api The API to create a renderer for.
         * @return Result containing the created renderer, or an error.
         */
        [[nodiscard]] Result<std::unique_ptr<Renderer>> CreateRenderer(RendererAPI api);

        /**
         * @brief Get the fallback API order for a preferred API.
         * @param preferredAPI The preferred API.
         * @return Vector of APIs to try in order.
         */
        [[nodiscard]] static std::vector<RendererAPI> GetFallbackAPIs(RendererAPI preferredAPI);

        mutable std::mutex m_Mutex;
        std::unique_ptr<Renderer> m_Renderer;
        RendererAPI m_CurrentAPI = RendererAPI::None;
        bool m_Initialized = false;
    };

} // namespace Sabora
