#include "pch.h"
#include "Window.h"
#include "Renderer/Core/RendererTypes.h"
#include "Log.h"
#include <SDL3/SDL.h>

namespace Sabora
{
    Window::Window(SDL_Window* window, const WindowConfig& config) noexcept : m_Window(window), m_Config(config)
    {
    }

    Window::~Window()
    {
        if (m_Window != nullptr)
        {
            SB_CORE_INFO("Destroying window: {}", m_Config.title);
            SDL_DestroyWindow(m_Window);
            m_Window = nullptr;
        }
    }

    Window::Window(Window&& other) noexcept : m_Window(other.m_Window), m_Config(std::move(other.m_Config))
    {
        other.m_Window = nullptr;
    }

    Window& Window::operator=(Window&& other) noexcept
    {
        if (this != &other)
        {
            // Destroy current window if it exists
            if (m_Window != nullptr)
            {
                SDL_DestroyWindow(m_Window);
            }

            m_Window = other.m_Window;
            m_Config = std::move(other.m_Config);
            other.m_Window = nullptr;
        }

        return *this;
    }

    Result<std::unique_ptr<Window>> Window::Create(const WindowConfig& config)
    {
        // Build SDL window flags
        uint32_t flags = 0;

        if (config.fullscreen)
        {
            flags |= SDL_WINDOW_FULLSCREEN;
        }

        if (config.resizable)
        {
            flags |= SDL_WINDOW_RESIZABLE;
        }

        if (config.borderless)
        {
            flags |= SDL_WINDOW_BORDERLESS;
        }

        if (config.highDPI)
        {
            flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
        }

        // Add renderer-specific flags based on preferred API
        if (config.preferredRendererAPI == RendererAPI::OpenGL)
        {
            flags |= SDL_WINDOW_OPENGL;
        }
        // Future: Add flags for Vulkan, DirectX, Metal as needed
        // else if (config.preferredRendererAPI == RendererAPI::Vulkan)
        // {
        //     flags |= SDL_WINDOW_VULKAN;
        // }

        // Create the SDL window
        SDL_Window* sdlWindow = SDL_CreateWindow(
            config.title.c_str(),
            config.width,
            config.height,
            flags
        );

        if (sdlWindow == nullptr)
        {
            return Result<std::unique_ptr<Window>>::Failure
            (
                ErrorCode::PlatformWindowCreationFailed,
                fmt::format("Failed to create window: {}", SDL_GetError())
            );
        }

        auto window = std::unique_ptr<Window>(new Window(sdlWindow, config));
        SB_CORE_INFO("Window created: {} ({}x{})", config.title, config.width, config.height);

        return Result<std::unique_ptr<Window>>::Success(std::move(window));
    }

    void Window::Show()
    {
        if (m_Window != nullptr)
        {
            SDL_ShowWindow(m_Window);
        }
    }

    void Window::Hide()
    {
        if (m_Window != nullptr)
        {
            SDL_HideWindow(m_Window);
        }
    }

    bool Window::IsVisible() const
    {
        if (m_Window == nullptr)
        {
            return false;
        }

        return SDL_GetWindowFlags(m_Window) & SDL_WINDOW_HIDDEN ? false : true;
    }

    int32_t Window::GetWidth() const
    {
        if (m_Window == nullptr)
        {
            return 0;
        }

        int32_t width = 0;
        int32_t height = 0;
        SDL_GetWindowSize(m_Window, &width, &height);

        return width;
    }

    int32_t Window::GetHeight() const
    {
        if (m_Window == nullptr)
        {
            return 0;
        }

        int32_t width = 0;
        int32_t height = 0;
        SDL_GetWindowSize(m_Window, &width, &height);

        return height;
    }

    const std::string& Window::GetTitle() const
    {
        return m_Config.title;
    }

    void Window::SetTitle(const std::string& title)
    {
        if (m_Window != nullptr)
        {
            SDL_SetWindowTitle(m_Window, title.c_str());
            m_Config.title = title;
        }
    }

} // namespace Sabora