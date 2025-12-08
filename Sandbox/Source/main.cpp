#include <filesystem>

#include "Sabora.h"

int main() 
{
    using namespace Sabora;
    namespace fs = std::filesystem;

    // Load config and launch app
    ConfigurationManager cfg("Sandbox/Source/defaults.json", "Sandbox/Source/user.json");
    cfg.Initialize();
    const auto config = cfg.Get();

    ApplicationConfig appCfg{};
    // Set app name from config if available
    if (config.contains("app_name")) 
    {
        appCfg.name = config["app_name"];
    }

    Application app(appCfg);

    // Initialize application systems (SDL, etc.)
    auto initResult = app.Initialize();
    if (initResult.IsFailure())
    {
        SB_CORE_CRITICAL("Failed to initialize application: {}", initResult.GetError().ToString());
        return 1;
    }

    // Test the logging system
    SB_INFO("Sandbox application started successfully!");
    SB_CORE_INFO("Core system is running");
    SB_RENDERER_INFO("Renderer system initialized");
    SB_AUDIO_DEBUG("Audio system debug message");
    SB_PHYSICS_WARN("Physics system warning");
    SB_INPUT_ERROR("Input system error occurred");
    SB_SCENE_CRITICAL("Critical scene error!");
    
    // Test format-based logging
    SB_INFO("Application config: {}", appCfg.name);
    SB_CORE_DEBUG("Delta time: {:.3f}", 0.016f);
    SB_RENDERER_TRACE("Rendering frame {}", 1);

    // Test Result type error handling
    SB_INFO("Testing Result type error handling...");
    
    // Example of creating and using Results
    auto successResult = Result<int>::Success(42);
    if (successResult)
    {
        SB_INFO("Success result value: {}", successResult.Value());
    }

    auto failureResult = Result<int>::Failure(ErrorCode::CoreNotFound, "Test error message");
    if (failureResult.IsFailure())
    {
        SB_WARN("Failure result error: {}", failureResult.GetError().ToString());
    }

    // Run the main application loop
    app.Run();

    SB_INFO("Sandbox finished.");

    return 0;
}