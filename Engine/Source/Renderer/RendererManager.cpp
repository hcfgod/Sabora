#include "pch.h"
#include "Renderer/RendererManager.h"
#include "Core/Window.h"
#include "Core/Log.h"
#include "Renderer/OpenGL/OpenGLRenderer.h"

namespace Sabora
{
    Result<void> RendererManager::Initialize(Window* window, RendererAPI preferredAPI)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if (m_Initialized)
        {
            SB_CORE_WARN("RendererManager::Initialize() called but renderer is already initialized");
            return Result<void>::Success();
        }

        if (!window || !window->IsValid())
        {
            return Result<void>::Failure(
                ErrorCode::CoreNullPointer,
                "Window is null or invalid"
            );
        }

        // Get fallback API list
        auto fallbackAPIs = GetFallbackAPIs(preferredAPI);

        // Try each API in order
        for (RendererAPI api : fallbackAPIs)
        {
            if (!IsAPIAvailable(api))
            {
                SB_CORE_DEBUG("API {} is not available, skipping", RendererAPIToString(api));
                continue;
            }

            SB_CORE_INFO("Attempting to create renderer with API: {}", RendererAPIToString(api));

            auto rendererResult = CreateRenderer(api);
            if (rendererResult.IsFailure())
            {
                SB_CORE_WARN("Failed to create renderer with API {}: {}", 
                    RendererAPIToString(api), 
                    rendererResult.GetError().ToString());
                continue;
            }

            auto renderer = std::move(rendererResult).Value();

            // Initialize the renderer
            auto initResult = renderer->Initialize(window);
            if (initResult.IsFailure())
            {
                SB_CORE_WARN("Failed to initialize renderer with API {}: {}", 
                    RendererAPIToString(api), 
                    initResult.GetError().ToString());
                continue;
            }

            // Success!
            m_Renderer = std::move(renderer);
            m_CurrentAPI = api;
            m_Initialized = true;

            SB_CORE_INFO("Renderer initialized successfully with API: {}", RendererAPIToString(api));
            return Result<void>::Success();
        }

        // All APIs failed
        return Result<void>::Failure(
            ErrorCode::GraphicsDeviceCreationFailed,
            fmt::format("Failed to create renderer with any available API. Tried: {}", 
                [&fallbackAPIs]() {
                    std::string result;
                    for (size_t i = 0; i < fallbackAPIs.size(); ++i) {
                        if (i > 0) result += ", ";
                        result += RendererAPIToString(fallbackAPIs[i]);
                    }
                    return result;
                }())
        );
    }

    void RendererManager::Shutdown()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if (!m_Initialized)
        {
            return;
        }

        if (m_Renderer)
        {
            m_Renderer->Shutdown();
            m_Renderer.reset();
        }

        m_CurrentAPI = RendererAPI::None;
        m_Initialized = false;

        SB_CORE_INFO("Renderer shutdown complete");
    }

    Renderer* RendererManager::GetRenderer() const noexcept
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Renderer.get();
    }

    RendererAPI RendererManager::GetAPI() const noexcept
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_CurrentAPI;
    }

    bool RendererManager::IsInitialized() const noexcept
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Initialized;
    }

    bool RendererManager::IsAPIAvailable(RendererAPI api)
    {
        switch (api)
        {
            case RendererAPI::OpenGL:
                // OpenGL is available if we can load GLAD
                // For now, assume it's available (will be checked during context creation)
                return true;

            case RendererAPI::Vulkan:
                // TODO: Check for Vulkan availability
                return false;

            case RendererAPI::DirectX12:
                // TODO: Check for DirectX 12 availability (Windows only)
                return false;

            case RendererAPI::Metal:
                // TODO: Check for Metal availability (macOS/iOS only)
                return false;

            case RendererAPI::None:
            default:
                return false;
        }
    }

    Result<std::unique_ptr<Renderer>> RendererManager::CreateRenderer(RendererAPI api)
    {
        switch (api)
        {
            case RendererAPI::OpenGL:
                return Result<std::unique_ptr<Renderer>>::Success(
                    std::make_unique<OpenGLRenderer>()
                );

            case RendererAPI::Vulkan:
                // TODO: Implement Vulkan renderer
                return Result<std::unique_ptr<Renderer>>::Failure(
                    ErrorCode::CoreNotImplemented,
                    "Vulkan renderer not yet implemented"
                );

            case RendererAPI::DirectX12:
                // TODO: Implement DirectX 12 renderer
                return Result<std::unique_ptr<Renderer>>::Failure(
                    ErrorCode::CoreNotImplemented,
                    "DirectX 12 renderer not yet implemented"
                );

            case RendererAPI::Metal:
                // TODO: Implement Metal renderer
                return Result<std::unique_ptr<Renderer>>::Failure(
                    ErrorCode::CoreNotImplemented,
                    "Metal renderer not yet implemented"
                );

            case RendererAPI::None:
            default:
                return Result<std::unique_ptr<Renderer>>::Failure(
                    ErrorCode::CoreInvalidArgument,
                    "Invalid renderer API"
                );
        }
    }

    std::vector<RendererAPI> RendererManager::GetFallbackAPIs(RendererAPI preferredAPI)
    {
        std::vector<RendererAPI> fallbacks;

        switch (preferredAPI)
        {
            case RendererAPI::OpenGL:
                fallbacks = { RendererAPI::OpenGL, RendererAPI::Vulkan, RendererAPI::DirectX12, RendererAPI::Metal };
                break;

            case RendererAPI::Vulkan:
                fallbacks = { RendererAPI::Vulkan, RendererAPI::OpenGL, RendererAPI::DirectX12, RendererAPI::Metal };
                break;

            case RendererAPI::DirectX12:
                fallbacks = { RendererAPI::DirectX12, RendererAPI::Vulkan, RendererAPI::OpenGL };
                break;

            case RendererAPI::Metal:
                fallbacks = { RendererAPI::Metal, RendererAPI::OpenGL, RendererAPI::Vulkan };
                break;

            default:
                // Default fallback order
                fallbacks = { RendererAPI::OpenGL, RendererAPI::Vulkan, RendererAPI::DirectX12, RendererAPI::Metal };
                break;
        }

        return fallbacks;
    }

} // namespace Sabora
