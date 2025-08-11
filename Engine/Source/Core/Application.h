#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>

namespace Sabora::Engine {

struct ApplicationConfig {
    std::string name = "Sabora Application";
    //WindowDesc window;
};

class Application {
public:
    using UpdateCallback = std::function<void(float)>;     // dt in seconds

    explicit Application(const ApplicationConfig& config);
    virtual ~Application();

    void run();
    void requestClose();

private:
    std::atomic<bool> m_running{false};
    UpdateCallback m_update;

    // Timing
    using clock = std::chrono::high_resolution_clock;
    clock::time_point m_lastFrame;
};

} // namespace Sabora::Engine