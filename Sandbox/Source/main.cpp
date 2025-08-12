#include <iostream>
#include <filesystem>

#include "Sabora.h"

int main() 
{
    using namespace Sabora::Engine;
    namespace fs = std::filesystem;

    // Load config and launch app
    ConfigurationManager cfg("Sandbox/Source/defaults.json", "Sandbox/Source/user.json");
    cfg.initialize();
    const auto config = cfg.get();

    ApplicationConfig appCfg{};
    // Set app name from config if available
    if (config.contains("app_name")) {
        appCfg.name = config["app_name"];
    }

    Sabora::Engine::Application app(appCfg);

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

    app.run();

    std::cout << "Sandbox finished." << std::endl;
    return 0;
}