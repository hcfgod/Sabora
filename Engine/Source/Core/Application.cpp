#include "pch.h"
#include "Application.h"
#include "SDLManager.h"
#include "EventManager.h"
#include "Input/Input.h"
#include "Log.h"
#include <SDL3/SDL.h>

namespace Sabora
{
    Application::Application(const ApplicationConfig& config) : m_Config(config)
    {
        // Initialize logging system first (needed for error reporting)
        Sabora::Log::Initialize();
        SB_CORE_INFO("Application created with name: {}", config.name);
    }

    Result<void> Application::Initialize()
    {
        SB_CORE_INFO("Initializing application systems...");

        // Initialize SDL
        auto sdlResult = SDLManager::Initialize(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
        if (sdlResult.IsFailure())
        {
            SB_CORE_ERROR("Failed to initialize SDL: {}", sdlResult.GetError().ToString());
            return Result<void>::Failure(sdlResult.GetError());
        }

        // Create window
        auto windowResult = Window::Create(m_Config.windowConfig);
        if (windowResult.IsFailure())
        {
            SB_CORE_ERROR("Failed to create window: {}", windowResult.GetError().ToString());
            SDLManager::Shutdown();
            return Result<void>::Failure(windowResult.GetError());
        }

        m_Window = std::move(windowResult).Value();
        m_Window->Show();

        // Setup EventManager with our dispatcher
        EventManager::Get().SetDispatcher(m_EventDispatcher);

        // Setup event handlers
        SetupEventHandlers();

        SB_CORE_INFO("Application initialization complete.");
        return Result<void>::Success();
    }

    void Application::Run()
    {
        // Ensure application is initialized before running
        if (!SDLManager::IsInitialized() || m_Window == nullptr)
        {
            SB_CORE_ERROR("Application::Run() called before successful initialization!");
            return;
        }

        m_Running = true;
        m_LastFrame = Clock::now();

        SB_CORE_INFO("Entering main application loop...");

        while (m_Running)
        {
            // Calculate delta time
            const auto now = Clock::now();
            const float deltaTime = std::chrono::duration<float>(now - m_LastFrame).count();
            m_LastFrame = now;

            // Begin frame for Input system (resets frame-specific state)
            Input::Get().BeginFrame();

            // Process events (this also updates Input system)
            // Event-based tracking is the primary method - events update Input state directly
            // Events correctly set both held state (IsKeyPressed) and frame-specific state (IsKeyDown/IsKeyUp)
            m_EventDispatcher.ProcessSDLEvents();

            // Update application
            OnUpdate(deltaTime);

            // Future: Render frame
        }

        SB_CORE_INFO("Exited main application loop.");
    }

    void Application::RequestClose()
    {
        m_Running = false;
        SB_CORE_INFO("Application close requested.");
    }

    void Application::SetupEventHandlers()
    {
        // Subscribe to window close events
        [[maybe_unused]] auto windowCloseSubId = m_EventDispatcher.Subscribe<WindowCloseEvent>([this](const WindowCloseEvent& event) 
        {
            WindowCloseEvent& mutableEvent = const_cast<WindowCloseEvent&>(event);
            OnWindowClose(mutableEvent);
            if (!mutableEvent.IsHandled())
            {
                RequestClose();
            }
        });
    }

    Application::~Application()
    {
        SB_CORE_INFO("Application shutting down...");

        // Window is automatically cleaned up via unique_ptr destructor
        m_Window.reset();

        // Shutdown SDL
        SDLManager::Shutdown();

        // Shutdown logging system last
        Sabora::Log::Shutdown();
    }

} // namespace Sabora