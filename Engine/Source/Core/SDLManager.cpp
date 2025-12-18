#include "SDLManager.h"
#include "Log.h"
#include <SDL3/SDL.h>

namespace Sabora
{
    bool SDLManager::s_Initialized = false;
    uint32_t SDLManager::s_InitFlags = 0;

    Result<void> SDLManager::Initialize(uint32_t flags)
    {
        if (s_Initialized)
        {
            return Result<void>::Failure(
                ErrorCode::CoreAlreadyExists,
                "SDL is already initialized. Shutdown before re-initializing."
            );
        }

        // Disable GameInput to prevent crashes on some Windows configurations
        SDL_SetHint(SDL_HINT_WINDOWS_GAMEINPUT, "0");

        // Note: SDL3 uses 'bool' return type for SDL_Init unlike SDL2 which used 'int'
        // SDL_Init returns true on success, false on failure
        if (!SDL_Init(flags))
        {
            return Result<void>::Failure(
                ErrorCode::PlatformSDLError,
                fmt::format("SDL3 could not initialize! SDL_Error: {}", SDL_GetError())
            );
        }

        s_Initialized = true;
        s_InitFlags = flags;
        SB_CORE_INFO("SDL3 initialized successfully! Version: {}", SDL_GetRevision());

        return Result<void>::Success();
    }

    void SDLManager::Shutdown()
    {
        if (s_Initialized)
        {
            SB_CORE_INFO("Shutting down SDL3...");
            SDL_Quit();
            s_Initialized = false;
            s_InitFlags = 0;
        }
    }

    bool SDLManager::IsInitialized() noexcept
    {
        return s_Initialized;
    }

    const char* SDLManager::GetVersion() noexcept
    {
        if (!s_Initialized)
        {
            return "";
        }
        return SDL_GetRevision();
    }

} // namespace Sabora
