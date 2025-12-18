#include "pch.h"
#include "Sabora.h"
#include <SDL3/SDL.h>

using namespace Sabora;

/**
 * @brief Sandbox application for testing engine features.
 */
class SandboxApp : public Application
{
public:
    SandboxApp() : Application({ "Sabora Sandbox", WindowConfig{ "Sabora Sandbox", 1280, 720, false, true, false, true } })
    {
        // Subscribe to events using EventManager singleton
        [[maybe_unused]] auto closeSubId = EventManager::Get().Subscribe<WindowCloseEvent>
        (
            [this](const WindowCloseEvent& event) 
            {
                OnWindowClose(const_cast<WindowCloseEvent&>(event));
            }
        );

        [[maybe_unused]] auto keySubId = EventManager::Get().Subscribe<KeyEvent>
        (
            [this](const KeyEvent& event) 
            {
                // Example: Handle key events
                if (event.GetKey() == SDLK_ESCAPE && event.IsPressed())
                {
                    SB_CORE_INFO("Escape key pressed - closing application");
                    RequestClose();
                }
            }
        );
    }

    void OnUpdate(float deltaTime) override
    {
        // Example: Log frame time occasionally
        static float timeSinceLastLog = 0.0f;
        timeSinceLastLog += deltaTime;
        if (timeSinceLastLog >= 1.0f)
        {
            SB_CORE_DEBUG("Frame time: {:.3f}ms", deltaTime * 1000.0f);
            timeSinceLastLog = 0.0f;
        }
    }

    void OnWindowClose(WindowCloseEvent& event) override
    {
        SB_CORE_INFO("Window close requested");
        // You can mark the event as handled to prevent default close behavior
        // event.MarkHandled();
    }
};

/**
 * @brief Create the application instance.
 * 
 * This function is called by EntryPoint::Main() to create your application.
 * Simply return a new instance of your Application-derived class.
 */
Sabora::Application* Sabora::CreateApplication()
{
    return new SandboxApp();
}