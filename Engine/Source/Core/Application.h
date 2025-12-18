#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include "Result.h"
#include "Window.h"
#include "Event.h"

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
        WindowConfig windowConfig;
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
         *       initialize SDL or create a window. Call Initialize() to set up platform systems.
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
         * This method initializes SDL, creates a window, and sets up the event system.
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

        /**
         * @brief Get the application window.
         * @return Pointer to the window, or nullptr if not initialized.
         */
        [[nodiscard]] Window* GetWindow() const noexcept { return m_Window.get(); }

        /**
         * @brief Get the event dispatcher.
         * @return Reference to the event dispatcher.
         */
        [[nodiscard]] EventDispatcher& GetEventDispatcher() noexcept { return m_EventDispatcher; }

    protected:
        /**
         * @brief Called once per frame before event processing.
         * @param deltaTime Time elapsed since last frame in seconds.
         */
        virtual void OnUpdate([[maybe_unused]] float deltaTime) {}

        /**
         * @brief Called when a window close event is received.
         * @param event The window close event.
         */
        virtual void OnWindowClose([[maybe_unused]] WindowCloseEvent& event) {}

    private:
        void SetupEventHandlers();

        std::atomic<bool> m_Running{false};
        ApplicationConfig m_Config;

        // Window and event system
        std::unique_ptr<Window> m_Window;
        EventDispatcher m_EventDispatcher;

        // Timing for delta time calculations
        using Clock = std::chrono::high_resolution_clock;
        Clock::time_point m_LastFrame;
    };

} // namespace Sabora