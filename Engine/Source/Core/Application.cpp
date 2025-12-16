#include "Application.h"
#include "Log.h"
#include <SDL3/SDL.h>

// Audio library includes for verification
#include <sndfile.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <vorbis/vorbisenc.h>
#include <AL/alc.h>
#include <AL/al.h>

namespace Sabora 
{
    //==========================================================================
    // SDLContext Implementation
    //==========================================================================

    SDLContext::SDLContext() noexcept
        : m_Initialized(false)
    {
    }

    SDLContext::SDLContext(SDLContext&& other) noexcept
        : m_Initialized(other.m_Initialized)
    {
        other.m_Initialized = false;
    }

    SDLContext& SDLContext::operator=(SDLContext&& other) noexcept
    {
        if (this != &other)
        {
            // Clean up current state if initialized
            if (m_Initialized)
            {
                SDL_Quit();
            }

            m_Initialized = other.m_Initialized;
            other.m_Initialized = false;
        }
        return *this;
    }

    SDLContext::~SDLContext()
    {
        if (m_Initialized)
        {
            SB_CORE_INFO("Shutting down SDL3...");
            SDL_Quit();
            m_Initialized = false;
        }
    }

    Result<std::unique_ptr<SDLContext>> SDLContext::Create(uint32_t flags)
    {
        // Create the context (private constructor access via factory)
        auto context = std::unique_ptr<SDLContext>(new SDLContext());

        // Disable GameInput to prevent crashes on some Windows configurations
        SDL_SetHint(SDL_HINT_WINDOWS_GAMEINPUT, "0");

        // Note: SDL3 uses 'bool' return type for SDL_Init unlike SDL2 which used 'int'
        if (!SDL_Init(flags))
        {
            return Result<std::unique_ptr<SDLContext>>::Failure(
                ErrorCode::PlatformSDLError,
                fmt::format("SDL3 could not initialize! SDL_Error: {}", SDL_GetError())
            );
        }

        context->m_Initialized = true;
        SB_CORE_INFO("SDL3 initialized successfully! Version: {}", SDL_GetRevision());

        return Result<std::unique_ptr<SDLContext>>::Success(std::move(context));
    }

    //==========================================================================
    // Application Implementation
    //==========================================================================

    Application::Application(const ApplicationConfig& config) 
    {
        // Initialize logging system first (needed for error reporting)
        Sabora::Log::Initialize();
        SB_CORE_INFO("Application created with name: {}", config.name);
    }

    Result<void> Application::Initialize()
    {
        SB_CORE_INFO("Initializing application systems...");

        // Initialize SDL using RAII wrapper
        auto sdlResult = SDLContext::Create(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
        if (sdlResult.IsFailure())
        {
            SB_CORE_ERROR("Failed to initialize SDL: {}", sdlResult.GetError().ToString());
            return Result<void>::Failure(sdlResult.GetError());
        }

        m_SDLContext = std::move(sdlResult).Value();

        // Verify audio libraries are properly linked and functional
        SB_CORE_INFO("Verifying audio library integration...");

        // Test libsndfile - get library version
        const char* sndfileVersion = sf_version_string();
        if (sndfileVersion != nullptr)
        {
            SB_CORE_INFO("libsndfile version: {}", sndfileVersion);
        }
        else
        {
            SB_CORE_WARN("Could not retrieve libsndfile version");
        }

        // Test libogg - verify library is linked
        ogg_stream_state testStream;
        int oggResult = ogg_stream_init(&testStream, 1);
        if (oggResult == 0)
        {
            SB_CORE_INFO("libogg initialized successfully (serialno: {})", testStream.serialno);
            ogg_stream_clear(&testStream);
        }
        else
        {
            SB_CORE_WARN("libogg stream initialization failed");
        }

        // Test libvorbis - verify library is linked
        vorbis_info vorbisInfo;
        vorbis_info_init(&vorbisInfo);
        SB_CORE_INFO("libvorbis initialized successfully");
        vorbis_info_clear(&vorbisInfo);

        // Test OpenAL Soft - verify library is linked
        ALCdevice* alDevice = alcOpenDevice(nullptr);
        if (alDevice != nullptr)
        {
            ALCcontext* alContext = alcCreateContext(alDevice, nullptr);
            if (alContext != nullptr)
            {
                if (alcMakeContextCurrent(alContext) == ALC_TRUE)
                {
                    const char* alVersion = alGetString(AL_VERSION);
                    const char* alRenderer = alGetString(AL_RENDERER);
                    if (alVersion != nullptr && alRenderer != nullptr)
                    {
                        SB_CORE_INFO("OpenAL Soft initialized - Version: {}, Renderer: {}", alVersion, alRenderer);
                    }
                    alcMakeContextCurrent(nullptr);
                }
                alcDestroyContext(alContext);
            }
            alcCloseDevice(alDevice);
        }
        else
        {
            SB_CORE_WARN("OpenAL Soft: No audio device available (this is acceptable in headless environments)");
        }

        SB_CORE_INFO("Application initialization complete.");
        return Result<void>::Success();
    }

    void Application::Run() 
    {
        // Ensure application is initialized before running
        if (!m_SDLContext || !m_SDLContext->IsInitialized())
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
            [[maybe_unused]] const float deltaTime = 
                std::chrono::duration<float>(now - m_LastFrame).count();
            m_LastFrame = now;

            // Process SDL events
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_EVENT_QUIT)
                {
                    RequestClose();
                }
            }

            // Future: Update game systems with deltaTime
            // Future: Render frame
        }

        SB_CORE_INFO("Exited main application loop.");
    }

    void Application::RequestClose() 
    {
        m_Running = false;
        SB_CORE_INFO("Application close requested.");
    }

    Application::~Application() 
    {
        SB_CORE_INFO("Application shutting down...");

        // SDLContext is automatically cleaned up via unique_ptr destructor (RAII)
        // The destructor will call SDL_Quit() if SDL was initialized

        // Shutdown logging system last
        Sabora::Log::Shutdown();
    }

} // namespace Sabora