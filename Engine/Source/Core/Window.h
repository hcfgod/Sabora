#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include "Result.h"

// Forward declaration of SDL_Window
struct SDL_Window;

namespace Sabora
{
    /**
     * @brief Configuration for window creation.
     */
    struct WindowConfig
    {
        std::string title = "Sabora Window";
        int32_t width = 1280;
        int32_t height = 720;
        bool fullscreen = false;
        bool resizable = true;
        bool borderless = false;
        bool highDPI = true;
    };

    /**
     * @brief Represents a platform window managed by SDL3.
     * 
     * Window provides a high-level interface for creating and managing
     * application windows. It handles window creation, destruction, and
     * provides access to window properties.
     * 
     * Usage:
     * @code
     *   WindowConfig config;
     *   config.title = "My Game";
     *   config.width = 1920;
     *   config.height = 1080;
     *   
     *   auto windowResult = Window::Create(config);
     *   if (windowResult.IsFailure()) {
     *       // Handle error
     *   }
     *   
     *   auto window = std::move(windowResult).Value();
     *   window->Show();
     * @endcode
     */
    class Window
    {
    public:
        /**
         * @brief Create a new window with the specified configuration.
         * @param config Window configuration settings.
         * @return Result containing a unique_ptr to the Window, or an error.
         */
        [[nodiscard]] static Result<std::unique_ptr<Window>> Create(const WindowConfig& config);

        /**
         * @brief Destructor - closes and destroys the window.
         */
        ~Window();

        // Non-copyable, movable
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&& other) noexcept;
        Window& operator=(Window&& other) noexcept;

        /**
         * @brief Show the window.
         */
        void Show();

        /**
         * @brief Hide the window.
         */
        void Hide();

        /**
         * @brief Check if the window is currently visible.
         * @return True if the window is visible.
         */
        [[nodiscard]] bool IsVisible() const;

        /**
         * @brief Get the window width.
         * @return Window width in pixels.
         */
        [[nodiscard]] int32_t GetWidth() const;

        /**
         * @brief Get the window height.
         * @return Window height in pixels.
         */
        [[nodiscard]] int32_t GetHeight() const;

        /**
         * @brief Get the window title.
         * @return Window title string.
         */
        [[nodiscard]] const std::string& GetTitle() const;

        /**
         * @brief Set the window title.
         * @param title New window title.
         */
        void SetTitle(const std::string& title);

        /**
         * @brief Get the underlying SDL_Window pointer.
         * @return Raw SDL_Window pointer (for low-level SDL operations).
         * 
         * @warning This is an escape hatch for advanced SDL operations.
         *          Prefer using Window methods when possible.
         */
        [[nodiscard]] SDL_Window* GetSDLWindow() const noexcept { return m_Window; }

        /**
         * @brief Check if the window is valid.
         * @return True if the window handle is valid.
         */
        [[nodiscard]] bool IsValid() const noexcept { return m_Window != nullptr; }

    private:
        /**
         * @brief Private constructor - use Create() factory method.
         * @param window SDL window handle.
         * @param config Window configuration.
         */
        explicit Window(SDL_Window* window, const WindowConfig& config) noexcept;

        SDL_Window* m_Window = nullptr;
        WindowConfig m_Config;
    };

} // namespace Sabora
