#include "Application.h"
#include "Log.h"

namespace Sabora::Engine {

Application::Application(const ApplicationConfig& config) {
	// Initialize logging system
	Sabora::Log::Init();
	SB_CORE_INFO("Application initialized with config: {}", config.name);
}

void Application::run() {
    m_running = true;
    m_lastFrame = clock::now();
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
    // Shutdown logging system
    Sabora::Log::Shutdown();
}

} // namespace Sabora::Engine