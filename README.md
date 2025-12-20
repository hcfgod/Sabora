# Sabora Engine

Sabora is a modern C++ game engine designed for developers who want performance, clarity, and control. Built with C++20 and a focus on clean architecture, it provides the foundation for creating games and interactive applications across Windows, Linux, and macOS.

## What Makes Sabora Different

Sabora prioritizes developer experience without sacrificing performance. Every system is designed with explicit error handling, comprehensive logging, and clear interfaces. You won't find magic happening behind the scenes—everything is transparent and controllable.

The engine uses modern C++ patterns throughout: RAII for resource management, Result types for error handling instead of exceptions, and a modular architecture that makes it easy to understand and extend.

## Getting Started

### Prerequisites

You'll need a C++20 compatible compiler and a few build tools. The setup scripts will help you install everything you need automatically.

**Windows:**
- Visual Studio 2022 or later (with C++ desktop development workload)
- PowerShell 5.1 or later

**Linux:**
- GCC 10+ or Clang 12+
- CMake 3.20+
- Git

**macOS:**
- Xcode 13+ (with command line tools)
- Homebrew (installed automatically if needed)

### Quick Start

Clone the repository and run the setup script for your platform:

**Windows:**
```powershell or command prompt
.\Setup.bat
```

**Linux:**
```bash
./Setup_linux.sh
```

**macOS:**
```bash
./Setup_macos.sh
```

The setup scripts will automatically install any missing dependencies, build the required libraries, and generate project files. Once complete, you can open the generated solution or project files in your IDE and start building.

### Building Your First Application

After setup, create a simple application. The engine handles `main()` for you—just provide your application class:

```cpp
#include "Sabora.h"
#include <SDL3/SDL.h>

class MyApp : public Sabora::Application
{
public:
    MyApp() : Application({ "My Game", Sabora::WindowConfig{ "My Game", 1280, 720 } })
    {
        // Subscribe to events using EventManager
        EventManager::Get().Subscribe<KeyEvent>(
            [this](const KeyEvent& e) {
                if (e.GetKey() == SDLK_ESCAPE && e.IsPressed())
                {
                    RequestClose();
                }
            }
        );
    }
    
    void OnUpdate(float deltaTime) override
    {
        // You can use the deltaTime parameter, or access Time directly (Unity-style)
        // Both approaches work:
        
        // Using the parameter (backward compatible)
        position += velocity * deltaTime;
        
        // Using Time class (Unity-style, more convenient)
        position += velocity * Time::GetDeltaTime();
        
        // Access other time information
        if (Time::GetTime() > 10.0f)
        {
            // 10 seconds have passed (scaled by timeScale)
        }
        
        // Pause/slow motion
        if (Input::Get().IsKeyDown(KeyCode::P))
        {
            Time::SetTimeScale(0.0f); // Pause
        }
        else if (Input::Get().IsKeyDown(KeyCode::S))
        {
            Time::SetTimeScale(0.5f); // Slow motion
        }
        else
        {
            Time::SetTimeScale(1.0f); // Normal speed
        }
    }
};

// Provide CreateApplication() - engine handles the rest
Sabora::Application* CreateApplication()
{
    return new MyApp();
}
```

That's it. No `main()` function needed—the engine provides it automatically. Just include `Sabora.h` and provide `CreateApplication()`.

## Core Systems

### Entry Point

The engine provides `main()` automatically. You don't write a `main()` function—just provide `CreateApplication()` and the engine handles the rest. This keeps your application code focused on game logic rather than boilerplate.

Simply include `Sabora.h` (which includes `EntryPoint.h`) and define `CreateApplication()`. The engine's `main()` will call it, initialize your application, run the main loop, and clean up automatically.

### Logging

Sabora includes a comprehensive logging system built on spdlog. Logs are organized by category, making it easy to filter and debug specific systems.

```cpp
SB_CORE_INFO("Engine initialized successfully");
SB_RENDERER_DEBUG("Rendered frame in {:.2f}ms", frameTime);
SB_PHYSICS_WARN("Physics timestep is too large");
```

Categories include Core, Renderer, Audio, Physics, Input, Scene, Script, Network, Editor, and Client. Each category supports all standard log levels from Trace to Critical.

### Error Handling

The engine uses a Result type for explicit error handling. No exceptions are thrown—every operation that can fail returns a Result that you can inspect and handle explicitly.

```cpp
auto windowResult = Window::Create(config);
if (windowResult.IsFailure())
{
    SB_CORE_ERROR("Window creation failed: {}", windowResult.GetError().ToString());
    return;
}
auto window = std::move(windowResult).Value();
```

### Window Management

Windows are created automatically when you initialize your Application. The Window class handles all platform-specific details while providing a clean, consistent interface. Access the window through your Application instance:

```cpp
auto* window = app.GetWindow();
window->SetTitle("New Title");
int32_t width = window->GetWidth();
```

Window configuration is provided when creating your Application:

```cpp
WindowConfig config;
config.title = "My Game";
config.width = 1920;
config.height = 1080;
config.resizable = true;

Application app({ "My Game", config });
```

### Event System

The event system provides type-safe event handling through the EventManager singleton. Subscribe to events from anywhere in your code. EventManager is automatically connected during Application initialization, so it's ready to use in your Application constructor:

```cpp
class MyApp : public Application
{
public:
    MyApp() : Application({ "My Game", WindowConfig{...} })
    {
        // EventManager is ready to use here
        EventManager::Get().Subscribe<WindowCloseEvent>([](const WindowCloseEvent& e) {
            // Handle window close
        });

        EventManager::Get().Subscribe<KeyEvent>([](const KeyEvent& e) {
            if (e.GetKey() == SDLK_ESCAPE && e.IsPressed())
            {
                // Handle escape key
            }
        });
    }
};

// Dispatch custom events from anywhere
MyCustomEvent event(data);
EventManager::Get().Dispatch(event);
```

Events are automatically processed each frame. SDL events are converted to engine events transparently, so you work with a consistent interface regardless of platform.

### Main Thread Dispatcher

The engine provides a thread-safe dispatcher for executing work on the main thread. This is essential for graphics APIs like OpenGL, which require all operations to be performed on the thread that created the context.

```cpp
// From any thread, queue work for the main thread
MainThreadDispatcher::Get().Dispatch([]() {
    // This will run on the main thread
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
});

// Synchronous dispatch (blocks until work is done)
MainThreadDispatcher::Get().DispatchSync([]() {
    // This will run on the main thread and block until complete
    GLuint texture = 0;
    glGenTextures(1, &texture);
    return texture;
});
```

The dispatcher is automatically processed each frame in the main loop, so queued work executes automatically.

### Time Management

The engine provides a Unity-style Time class for convenient access to time-related information. Access time data from anywhere in your code. All Time methods are thread-safe.

```cpp
// Get delta time (scaled by timeScale)
float speed = 5.0f * Time::GetDeltaTime();
position += velocity * Time::GetDeltaTime();

// Get unscaled delta time (real-time, unaffected by timeScale)
float uiAnimationSpeed = 10.0f * Time::GetUnscaledDeltaTime();

// Get elapsed time
if (Time::GetTime() > 10.0f)
{
    // 10 seconds have passed (scaled)
}

// Pause/slow motion
Time::SetTimeScale(0.0f);  // Pause
Time::SetTimeScale(0.5f);  // Half speed
Time::SetTimeScale(2.0f);  // Double speed
Time::SetTimeScale(1.0f);  // Normal speed

// Access other time information
uint64_t frameCount = Time::GetFrameCount();
float realtime = Time::GetRealtimeSinceStartup();
float smoothDelta = Time::GetSmoothDeltaTime(); // Smoothed for UI animations
```

The Time class provides:
- **GetDeltaTime()** - Scaled delta time (affected by timeScale)
- **GetUnscaledDeltaTime()** - Real delta time (unaffected by timeScale)
- **GetTime()** - Scaled elapsed time since start
- **GetUnscaledTime()** - Real elapsed time since start
- **GetRealtimeSinceStartup()** - High-precision realtime
- **GetFrameCount()** - Number of frames processed
- **GetTimeScale() / SetTimeScale()** - Control time speed (pause, slow-mo, etc.)
- **GetSmoothDeltaTime()** - Smoothed delta time for UI animations

### Asset System

The engine provides a comprehensive, type-safe asset management system with async loading, progress tracking, reference counting, caching, and automatic hot reloading.

**Key Features:**
- Type-safe asset handles with automatic reference counting
- Asynchronous loading with progress tracking
- Automatic caching (same path = same asset)
- Hot reloading (automatic file watching)
- Event-driven architecture (subscribe to loading events)
- Support for any asset type via loader interface

**Basic Usage:**

```cpp
// 1. Register a loader for your asset type
class ShaderLoader : public IAssetLoader<Shader> {
public:
    Result<std::unique_ptr<Shader>> Load(const std::filesystem::path& path) override {
        // Read and compile shader file
        auto shaderCode = AsyncIO::ReadTextFile(path);
        if (shaderCode.IsFailure()) {
            return Result<std::unique_ptr<Shader>>::Failure(shaderCode.GetError());
        }
        
        // Create and return shader
        return Result<std::unique_ptr<Shader>>::Success(
            std::make_unique<Shader>(shaderCode.Value())
        );
    }
    
    std::string GetAssetTypeName() const override { return "Shader"; }
    std::vector<std::string> GetSupportedExtensions() const override {
        return {".glsl", ".vert", ".frag"};
    }
};

// Register the loader
AssetManager::Get().RegisterLoader<Shader>(std::make_unique<ShaderLoader>());

// 2. Load assets asynchronously
AssetHandle<Shader> shader = AssetManager::Get().LoadAsync<Shader>("Shaders/basic.glsl");

// 3. Check if loaded (or subscribe to events)
if (shader.IsLoaded()) {
    auto* shaderPtr = shader.Get();
    // Use shader...
}

// 4. Track loading progress
float progress = AssetManager::Get().GetLoadingProgress();  // 0.0 to 1.0
size_t loadingCount = AssetManager::Get().GetLoadingAssetCount();

// 5. Listen for events
EventManager::Get().Subscribe<AllAssetsLoadedEvent>([](const AllAssetsLoadedEvent& e) {
    SB_CORE_INFO("All assets loaded! {} successful, {} failed", 
                 e.GetSuccessfulAssets(), e.GetFailedAssets());
});

// 6. Hot reloading is automatic - modify shader file and it reloads automatically
// (AssetReloadedEvent is dispatched when reload completes)
```

**Asset Handles:**

Asset handles are lightweight, copyable, and automatically manage reference counting:

```cpp
AssetHandle<Shader> shader1 = AssetManager::Get().LoadAsync<Shader>("Shaders/basic.glsl");

// Copying increments ref count
AssetHandle<Shader> shader2 = shader1;  // Ref count now 2

// Moving transfers ownership (no ref count change)
AssetHandle<Shader> shader3 = std::move(shader1);  // Ref count still 2

// When handles go out of scope, ref count decrements
// Asset is unloaded when ref count reaches 0 (via UnloadUnusedAssets())
```

**Synchronous Loading:**

For blocking loads (use sparingly):

```cpp
AssetHandle<Shader> shader = AssetManager::Get().LoadSync<Shader>("Shaders/basic.glsl");
// Blocks until loaded or failed
```

**Path Resolution:**

Asset paths can be relative (to asset root) or absolute:

```cpp
// Set asset root directory
AssetManager::Get().SetAssetRoot("Assets");

// Relative path (resolved to Assets/Shaders/basic.glsl)
AssetHandle<Shader> shader1 = AssetManager::Get().LoadAsync<Shader>("Shaders/basic.glsl");

// Absolute path (used as-is)
AssetHandle<Shader> shader2 = AssetManager::Get().LoadAsync<Shader>("/full/path/to/shader.glsl");
```

**Hot Reloading:**

Hot reloading is enabled by default and automatically watches all loaded assets:

```cpp
// Enable/disable hot reloading
AssetManager::Get().EnableHotReloading(true);

// Listen for reload events
EventManager::Get().Subscribe<AssetReloadedEvent>([](const AssetReloadedEvent& e) {
    if (e.IsSuccess()) {
        SB_CORE_INFO("Asset reloaded: {}", e.GetFilePath().string());
    }
});
```

**Asset Cleanup:**

Unload assets that are no longer referenced:

```cpp
// Remove assets with zero reference count
AssetManager::Get().UnloadUnusedAssets();

// Or clear everything (invalidates all handles)
AssetManager::Get().ClearCache();
```

### Configuration Management

Configuration files use JSON with a layered system: default values merged with user overrides. Thread-safe and easy to use.

```cpp
ConfigurationManager config("defaults.json", "user.json");
config.Initialize();
config.SetValue("/window/width", 1920);
auto merged = config.Get();
```

## Core Systems

**Asset Management:**
- Type-safe asset handles with reference counting
- Asynchronous loading with progress tracking
- Automatic caching and hot reloading
- Event-driven asset lifecycle

**Time Management:**
- Unity-style Time class (scaled/unscaled time, delta time, time scale)
- Frame spike protection
- Smooth delta time calculation
- Thread-safe access

**Input System:**
- Polling-based input (keyboard, mouse)
- Frame-specific state (IsKeyDown, IsKeyUp)
- Layout-independent scancode support
- Thread-safe queries

**Event System:**
- Type-safe event dispatching
- Observer pattern implementation
- SDL event integration
- Thread-safe subscriptions

**Main Thread Dispatcher:**
- Queue work for main thread execution
- Essential for graphics APIs (OpenGL/DirectX)
- Async and sync dispatch modes
- Thread-safe

## Integrated Libraries

Sabora includes a comprehensive set of libraries for game development:

**Graphics & Rendering:**
- GLAD (OpenGL 4.6 Core loader)
- GLM (mathematics library)
- Shaderc (shader compilation)
- SPIRV-Cross (shader reflection)
- MSDF-Atlas-Gen (signed distance field font rendering)
- stb_image (image loading)

**Audio:**
- OpenAL Soft (3D audio)
- libsndfile (audio file I/O)
- libogg, libvorbis (Ogg Vorbis support)
- libFLAC (FLAC support)
- libopus, libopusenc, opusfile (Opus codec support)
- minimp3 (MP3 decoding)

**Physics:**
- Jolt Physics (3D physics simulation)
- Box2D (2D physics)

**UI & Tools:**
- ImGui (immediate mode GUI)
- Freetype (font rendering)
- msdf-atlas-gen

**Platform:**
- SDL3 (windowing, input, audio)

**Utilities:**
- spdlog (logging)
- nlohmann/json (JSON parsing)
- doctest (unit testing)

All libraries are built from source and statically linked, ensuring consistent behavior across platforms.

## Testing

The engine includes a comprehensive test suite using doctest. Run tests to verify everything is working correctly:

```bash
# After building
./Build/bin/Debug_x64/Tests/Tests
```

Tests cover all core systems and library integrations. See the Tests directory for more information.

## Documentation

- **[ARCHITECTURE.md](ARCHITECTURE.md)** - Deep dive into the engine's design and architecture
- **[CONTRIBUTING.md](CONTRIBUTING.md)** - Guidelines for contributing to the project

## Thread Safety

The engine is designed with thread safety in mind:

**Thread-Safe Systems:**
- **Logging** - All logging operations are thread-safe (spdlog)
- **Configuration** - ConfigurationManager uses mutex locks for thread-safe access
- **Profiler** - All profiling operations are thread-safe
- **Time** - All Time getter methods are thread-safe
- **Input** - All Input query methods are thread-safe
- **EventDispatcher** - Subscriptions and dispatch are thread-safe
- **MainThreadDispatcher** - Thread-safe queue for main thread work

**Main Thread Only:**
- Window operations (SDL requires main thread)
- Event processing (SDL events must be processed on main thread)
- Rendering (graphics APIs require main thread)

**Multi-Threading Support:**
- Use `MainThreadDispatcher` to queue graphics operations from worker threads
- AsyncIO system for non-blocking file operations
- All query operations (Time, Input) are safe from any thread

## Development Philosophy

Sabora follows a few core principles:

**Explicit over Implicit** - No hidden behavior or magic. If something happens, you can see why and how.

**Performance by Design** - Zero-cost abstractions where possible. RAII ensures resources are managed efficiently without runtime overhead.

**Thread Safety by Default** - Core systems are thread-safe, allowing safe multi-threaded access.

**Developer Experience** - Clear APIs, comprehensive logging, and helpful error messages make development pleasant and productive.

**Modularity** - Systems are independent and can be understood in isolation. Dependencies are explicit and minimal.

## License

[Add your license here]

## Acknowledgments

Sabora builds on the excellent work of many open-source projects. Special thanks to all the library maintainers who make this possible.
