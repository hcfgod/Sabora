#pragma once

#include "Application.h"
#include "Log.h"

// Forward declaration - must be provided by the application
extern Sabora::Application* CreateApplication();

/**
 * @brief Application entry point.
 * 
 * This function is provided by the engine. It handles the application lifecycle.
 * The application must provide a CreateApplication() function that returns an Application instance.
 */
int main()
{
    using namespace Sabora;
    
    // Create the application instance
    Application* app = CreateApplication();
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