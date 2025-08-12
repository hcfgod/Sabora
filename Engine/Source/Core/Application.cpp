#include "Application.h"
#include "Log.h"
#include <SDL3/SDL.h>
#include <iostream>

namespace Sabora::Engine {

Application::Application(const ApplicationConfig& config) {
	// Initialize logging system
	Sabora::Log::Init();
	SB_CORE_INFO("Application initialized with config: {}", config.name);
}

void Application::run() {
    m_running = true;
    m_lastFrame = clock::now();

    // Disable GameInput to prevent crashes on Some windows
    SDL_SetHint(SDL_HINT_WINDOWS_GAMEINPUT, "0");
    
    // Test SDL3 initialization
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        std::cerr << "SDL3 could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }

    std::cout << "SDL3 initialized successfully! Version: " << SDL_GetRevision() << std::endl;

    while (m_running) {

        const auto now = clock::now();
        const float dt = std::chrono::duration<float>(now - m_lastFrame).count();
        m_lastFrame = now;

        if (m_update) {
            m_update(dt);
        }
    }
}

void Application::requestClose() {
    m_running = false;
    SB_CORE_INFO("Application close requested");
}

Application::~Application() {
    // Cleanup SDL3
    SDL_Quit();

    // Shutdown logging system
    Sabora::Log::Shutdown();
}

} // namespace Sabora::Engine