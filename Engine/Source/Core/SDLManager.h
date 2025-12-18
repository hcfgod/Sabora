#pragma once

#include <memory>
#include <cstdint>
#include "Result.h"

namespace Sabora
{
    /**
     * @brief Manages SDL3 initialization and lifecycle.
     * 
     * SDLManager is responsible for initializing SDL subsystems and ensuring
     * proper cleanup. It follows RAII principles and provides a singleton-like
     * interface for accessing SDL functionality throughout the application.
     * 
     * Usage:
     * @code
     *   auto managerResult = SDLManager::Initialize(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
     *   if (managerResult.IsFailure()) {
     *       // Handle error
     *   }
     *   
     *   // Use SDL throughout application...
     *   
     *   SDLManager::Shutdown(); // Called automatically on destruction
     * @endcode
     */
    class SDLManager
    {
    public:
        /**
         * @brief Initialize SDL with the specified subsystems.
         * @param flags SDL initialization flags (e.g., SDL_INIT_VIDEO | SDL_INIT_AUDIO).
         * @return Result indicating success or failure with error details.
         * 
         * This method should be called once at application startup before
         * using any SDL functionality. Subsequent calls will return an error
         * if SDL is already initialized.
         */
        [[nodiscard]] static Result<void> Initialize(uint32_t flags);

        /**
         * @brief Shutdown SDL and cleanup resources.
         * 
         * This method should be called once at application shutdown.
         * It's safe to call multiple times (idempotent).
         */
        static void Shutdown();

        /**
         * @brief Check if SDL is currently initialized.
         * @return True if SDL has been successfully initialized.
         */
        [[nodiscard]] static bool IsInitialized() noexcept;

        /**
         * @brief Get the SDL version string.
         * @return SDL revision string, or empty if not initialized.
         */
        [[nodiscard]] static const char* GetVersion() noexcept;

    private:
        SDLManager() = default;
        ~SDLManager() = default;

        // Non-copyable, non-movable
        SDLManager(const SDLManager&) = delete;
        SDLManager& operator=(const SDLManager&) = delete;
        SDLManager(SDLManager&&) = delete;
        SDLManager& operator=(SDLManager&&) = delete;

        static bool s_Initialized;
        static uint32_t s_InitFlags;
    };

} // namespace Sabora