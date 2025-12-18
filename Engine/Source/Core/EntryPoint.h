#include "Sabora.h"

/**
 * @brief Application entry point implementation.
 * 
 * This function is provided by the engine. It handles the application lifecycle.
 * The application must provide a CreateApplication() function that returns an Application instance.
 */
extern Sabora::Application* CreateApplication();

int main()
{
    using namespace Sabora;
    
    // Create the application instance
    Application* app = Sabora::CreateApplication();
    if (app == nullptr)
    {
        return 1;
    }

    // Initialize application systems
    auto initResult = app->Initialize();
    if (initResult.IsFailure())
    {
        SB_CORE_CRITICAL("Failed to initialize application: {}", initResult.GetError().ToString());
        delete app;
        return 1;
    }

    // Run the main loop
    app->Run();

    // Cleanup
    delete app;

    return 0;
}