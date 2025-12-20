#include "pch.h"
#include "Application.h"
#include "SDLManager.h"
#include "EventManager.h"
#include "Input/Input.h"
#include "GameTime.h"
#include "MainThreadDispatcher.h"
#include "Assets/AssetManager.h"
#include "Renderer/RendererManager.h"
#include "Renderer/Events/RendererEvents.h"
#include "Renderer/Core/RendererTypes.h"
#include "Renderer/Shaders/ShaderLoader.h"
#include "Renderer/Textures/TextureLoader.h"
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

        // Set default renderer API if not specified (must be done before window creation)
        if (m_Config.windowConfig.preferredRendererAPI == RendererAPI::None)
        {
            m_Config.windowConfig.preferredRendererAPI = RendererAPI::OpenGL; // Default to OpenGL
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

        // Initialize AssetManager with event dispatcher
        AssetManager::Get().Initialize({}, &m_EventDispatcher);

        // Initialize renderer (use preferred API from window config, which was set above if needed)
        auto rendererResult = RendererManager::Get().Initialize(m_Window.get(), m_Config.windowConfig.preferredRendererAPI);
        if (rendererResult.IsFailure())
        {
            SB_CORE_ERROR("Failed to initialize renderer: {}", rendererResult.GetError().ToString());
            // Don't fail initialization - renderer is optional for some applications
            // But dispatch error event
            RendererErrorEvent errorEvent(rendererResult.GetError());
            m_EventDispatcher.Dispatch(errorEvent);
        }
        else
        {
            // Register renderer-specific asset loaders
            RegisterRendererLoaders();

            // Set initial viewport to match window size
            auto* renderer = RendererManager::Get().GetRenderer();
            if (renderer && m_Window)
            {
                Viewport viewport;
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width = static_cast<float>(m_Window->GetWidth());
                viewport.height = static_cast<float>(m_Window->GetHeight());
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                
                auto viewportResult = renderer->SetViewport(viewport);
                if (viewportResult.IsFailure())
                {
                    SB_CORE_WARN("Failed to set initial viewport: {}", viewportResult.GetError().ToString());
                }
            }

            // Dispatch renderer initialized event
            RendererInitializedEvent initEvent(RendererManager::Get().GetAPI());
            m_EventDispatcher.Dispatch(initEvent);
            SB_CORE_INFO("Renderer initialized successfully");
        }

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
            const float unscaledDeltaTime = std::chrono::duration<float>(now - m_LastFrame).count();
            m_LastFrame = now;

            // Update Time system (must be done before OnUpdate so Time is available)
            Time::Update(unscaledDeltaTime);

            // Begin frame for Input system (resets frame-specific state)
            Input::Get().BeginFrame();

            // Process events (this also updates Input system)
            // Event-based tracking is the primary method - events update Input state directly
            // Events correctly set both held state (IsKeyPressed) and frame-specific state (IsKeyDown/IsKeyUp)
            m_EventDispatcher.ProcessSDLEvents();

            // Process main thread dispatcher queue (for renderer and other main-thread work)
            // This allows other threads to queue work that must run on the main thread
            MainThreadDispatcher::Get().ProcessQueue();

            // Update AssetManager (process loading queue, check for hot reloads)
            AssetManager::Get().Update();

            // Begin render frame
            auto* renderer = RendererManager::Get().GetRenderer();
            if (renderer)
            {
                auto beginResult = renderer->BeginFrame();
                if (beginResult.IsFailure())
                {
                    SB_RENDERER_ERROR("Failed to begin frame: {}", beginResult.GetError().ToString());
                    RendererErrorEvent errorEvent(beginResult.GetError());
                    m_EventDispatcher.Dispatch(errorEvent);
                }
            }

            // Update application (still pass deltaTime for backward compatibility)
            // Users can also use Time::GetDeltaTime() or Time::GetUnscaledDeltaTime() inside OnUpdate
            OnUpdate(Time::GetDeltaTime());

            // End render frame and present
            if (renderer)
            {
                auto endResult = renderer->EndFrame();
                if (endResult.IsFailure())
                {
                    SB_RENDERER_ERROR("Failed to end frame: {}", endResult.GetError().ToString());
                    RendererErrorEvent errorEvent(endResult.GetError());
                    m_EventDispatcher.Dispatch(errorEvent);
                }
            }
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

        // Subscribe to window resize events - automatically update viewport
        [[maybe_unused]] auto windowResizeSubId = m_EventDispatcher.Subscribe<WindowResizeEvent>([this](const WindowResizeEvent& event) 
        {
            // Automatically update viewport to match window size
            auto* renderer = GetRenderer();
            if (renderer && m_Window)
            {
                Viewport viewport;
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width = static_cast<float>(event.GetWidth());
                viewport.height = static_cast<float>(event.GetHeight());
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                
                auto result = renderer->SetViewport(viewport);
                if (result.IsFailure())
                {
                    SB_CORE_WARN("Failed to update viewport on window resize: {}", result.GetError().ToString());
                }
            }
        });
    }

    void Application::RegisterRendererLoaders()
    {
        // Register shader loader
        AssetManager::Get().RegisterLoader<OpenGLShaderProgram>(
            std::make_unique<ShaderLoader>(330) // Target GLSL 330
        );
        SB_CORE_INFO("Registered ShaderLoader with AssetManager");

        // Register texture loader
        AssetManager::Get().RegisterLoader<OpenGLTexture>(
            std::make_unique<TextureLoader>(true) // Generate mipmaps by default
        );
        SB_CORE_INFO("Registered TextureLoader with AssetManager");
    }

    Renderer* Application::GetRenderer() const noexcept
    {
        return RendererManager::Get().GetRenderer();
    }

    Application::~Application()
    {
        SB_CORE_INFO("Application shutting down...");

        // Dispatch renderer shutdown event
        if (RendererManager::Get().IsInitialized())
        {
            RendererShutdownEvent shutdownEvent;
            m_EventDispatcher.Dispatch(shutdownEvent);
        }

        // Shutdown renderer
        RendererManager::Get().Shutdown();

        // Shutdown AssetManager
        AssetManager::Get().Shutdown();

        // Window is automatically cleaned up via unique_ptr destructor
        m_Window.reset();

        // Shutdown SDL
        SDLManager::Shutdown();

        // Shutdown logging system last
        Sabora::Log::Shutdown();
    }

} // namespace Sabora