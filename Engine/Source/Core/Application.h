#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include "Result.h"

namespace Sabora 
{
    /**
     * @brief Configuration structure for initializing an Application.
     * 
     * Contains all the settings needed to create and configure
     * the main application instance.
     */
    struct ApplicationConfig 
    {
        std::string name = "Sabora Application";
    };

    /**
     * @brief RAII wrapper for SDL initialization and cleanup.
     * 
     * Ensures SDL is properly initialized in the constructor and
     * cleaned up in the destructor, following RAII principles.
     * This guarantees proper resource management even in the
     * presence of exceptions or early returns.
     */
    class SDLContext
    {
    public:
        /**
         * @brief Initialize SDL with the specified subsystems.
         * @param flags SDL initialization flags (e.g., SDL_INIT_VIDEO | SDL_INIT_AUDIO).
         * @return Result indicating success or failure with error details.
         */
        [[nodiscard]] static Result<std::unique_ptr<SDLContext>> Create(uint32_t flags);

        // Non-copyable, movable
        SDLContext(const SDLContext&) = delete;
        SDLContext& operator=(const SDLContext&) = delete;
        SDLContext(SDLContext&& other) noexcept;
        SDLContext& operator=(SDLContext&& other) noexcept;

        /**
         * @brief Destructor - calls SDL_Quit() if this instance owns SDL.
         */
        ~SDLContext();

        /**
         * @brief Check if SDL is currently initialized.
         * @return True if SDL is initialized and owned by this context.
         */
        [[nodiscard]] bool IsInitialized() const noexcept { return m_Initialized; }

    private:
        /**
         * @brief Private constructor - use Create() factory method.
         */
        explicit SDLContext() noexcept;

        bool m_Initialized = false;
    };

    /**
     * @brief Main application class for the Sabora Engine.
     * 
     * Handles the core application lifecycle including initialization,
     * the main loop, and shutdown. Uses RAII for resource management.
     */
    class Application
    {
    public:
        /**
         * @brief Construct an Application with the given configuration.
         * @param config Application configuration settings.
         * 
         * @note This constructor initializes the logging system but does NOT
         *       initialize SDL. Call Initialize() to set up platform systems.
         */
        explicit Application(const ApplicationConfig& config);

        /**
         * @brief Virtual destructor for proper cleanup in derived classes.
         */
        virtual ~Application();

        // Non-copyable
        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        /**
         * @brief Initialize the application and platform systems.
         * @return Result indicating success or failure with error details.
         * 
         * This method initializes SDL and other platform-specific systems.
         * Must be called before Run().
         */
        [[nodiscard]] Result<void> Initialize();

        /**
         * @brief Run the main application loop.
         * 
         * Executes the main loop until RequestClose() is called.
         * The loop handles events and updates the application state.
         */
        void Run();

        /**
         * @brief Request the application to close gracefully.
         * 
         * Sets the running flag to false, causing the main loop to exit
         * on the next iteration.
         */
        void RequestClose();

        /**
         * @brief Check if the application is currently running.
         * @return True if the main loop is active.
         */
        [[nodiscard]] bool IsRunning() const noexcept { return m_Running.load(); }

    private:
        /**
         * @brief Test libsndfile library integration.
         * 
         * Verifies that libsndfile is properly linked and can be used.
         * Called during initialization to ensure the library is available.
         */
        void TestLibsndfile();
        std::atomic<bool> m_Running{false};

        // RAII wrapper for SDL - ensures proper initialization and cleanup
        std::unique_ptr<SDLContext> m_SDLContext;

        // Timing for delta time calculations
        using Clock = std::chrono::high_resolution_clock;
        Clock::time_point m_LastFrame;
    };

} // namespace Sabora
