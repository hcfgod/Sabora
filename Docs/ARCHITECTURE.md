# Sabora Engine Architecture

This document explains how Sabora is structured and why it's designed the way it is. If you're new to the codebase, this is a good place to start understanding the big picture.

## Design Philosophy

Sabora is built around a few core ideas that guide every design decision:

**Resource Safety** - Resources are managed automatically through RAII. You never manually free memory or close handles. When an object goes out of scope, its resources are cleaned up automatically.

**Explicit Error Handling** - Operations that can fail return a Result type. You check for success or failure explicitly. No exceptions are thrown, which means no hidden control flow and predictable performance.

**Thread Safety Where It Matters** - Core systems like logging and configuration are thread-safe. Most systems run on the main thread, but the architecture supports multi-threading where needed.

**Layered Configuration** - Default settings are provided, but users can override them. The system merges these layers automatically, so you always get sensible defaults with the ability to customize.

## Core Systems

### Application Lifecycle

The Application class manages the entire engine lifecycle. The engine provides `main()` automatically through EntryPoint.h—you only need to provide a `CreateApplication()` function that returns your Application instance.

When you create an Application, it initializes logging. When you call Initialize(), it sets up platform systems through SDLManager, creates a window, and connects the EventManager. Run() starts the main loop, and the destructor cleans everything up in reverse order.

This ensures resources are always cleaned up, even if something goes wrong. The RAII pattern means you can't forget to shut down SDL or close a window.

**Entry Point Pattern:**
```cpp
class MyApp : public Application { /* ... */ };

Application* CreateApplication() { return new MyApp(); }
// No main() needed - engine provides it!
```

### Logging

The logging system is built on spdlog but adds engine-specific features. Logs are organized by category (Core, Renderer, Audio, etc.) so you can filter what you see. Each category can have its own log level, so you might want verbose renderer logs but only warnings from the audio system.

Logs go to both the console (with colors) and files. The file logging is automatically rotated to prevent huge log files.

### Error Handling

Every operation that can fail returns a Result type. This is similar to Rust's Result or Haskell's Either type. A Result either contains a value (success) or an error (failure).

You can chain operations together:

```cpp
auto result = OpenFile("config.txt")
    .AndThen([](FileHandle& h) { return ReadData(h); })
    .Map([](Data& d) { return ProcessData(d); });
```

Errors include a code, a message, and the source location where they occurred. This makes debugging much easier than cryptic error codes or exception stack traces.

### Configuration

Configuration uses JSON files with a two-layer system. Default configuration provides sensible values, and user configuration overrides specific settings. The system merges these automatically, so you can override just the window width without redefining everything.

Configuration is thread-safe, so you can read settings from any thread. Changes are persisted automatically.

### Platform Management

**SDLManager** handles SDL initialization and lifecycle. It's a singleton-like static interface that ensures SDL is initialized once and cleaned up properly. You never interact with SDL directly—SDLManager wraps all SDL operations.

**Window Management** is handled by the Window class, which wraps SDL's window functionality with a cleaner interface. Windows are created automatically during Application initialization based on the WindowConfig you provide. The Window class handles creation, showing, hiding, and cleanup automatically.

Windows are created through a factory method that returns a Result, so you can handle creation failures gracefully. The Application class owns the window and provides access to it.

### Event System

The event system consists of three layers:

**Event Classes** - Base Event class with derived types for different event categories (WindowCloseEvent, KeyEvent, MouseButtonEvent, etc.). Events can be marked as handled to prevent further processing.

**EventDispatcher** - The core event routing system. It manages subscriptions and dispatches events to registered listeners. The Application class owns an EventDispatcher instance.

**EventManager** - A singleton wrapper around EventDispatcher that provides global, easy-to-use access to the event system. You can subscribe to and dispatch events from anywhere using `EventManager::Get()`.

SDL events are automatically converted to engine events each frame, so you work with a consistent interface regardless of the underlying platform. The EventManager is connected to the Application's EventDispatcher during initialization, so it's ready to use after `Application::Initialize()` succeeds.

## Resource Management

Sabora uses RAII (Resource Acquisition Is Initialization) throughout. This means resources are acquired in constructors and released in destructors. Smart pointers (unique_ptr and shared_ptr) manage memory automatically.

Platform resources are wrapped in manager classes:
- **SDLManager** handles SDL initialization and shutdown
- **Window** handles window creation and destruction
- **Application** coordinates all platform systems

You never call SDL_Quit() or SDL_DestroyWindow() directly—the wrapper classes do it automatically.

This approach eliminates entire classes of bugs: no double-free errors, no leaked resources, no forgetting to clean up.

## Error Handling Strategy

The engine uses Result types instead of exceptions for several reasons:

**Performance** - No exception overhead in the hot path. Error handling is explicit and fast.

**Predictability** - You can see all error paths in your code. No hidden control flow from exceptions.

**Debugging** - Errors include source location information, making it easy to find where problems occur.

**Composability** - Results can be chained together naturally, making complex operations readable.

Every I/O operation, initialization call, and critical system operation returns a Result. You check for success and handle failures explicitly.

## Threading Model

Most engine systems run on the main thread. The main application loop processes events, updates systems, and renders frames sequentially.

File I/O can run asynchronously through the AsyncIO system. This allows loading assets without blocking the main thread.

Core systems like logging and configuration are thread-safe, so you can use them from any thread safely. Most other systems assume single-threaded access and should only be used from the main thread.

Future versions may add a job system for parallel processing, but the current architecture focuses on correctness and simplicity.

## Build System

Sabora uses Premake5 to generate platform-specific project files. This means you can work in Visual Studio on Windows, Xcode on macOS, or Makefiles on Linux, all from the same source.

Dependencies are built from source and statically linked. This ensures consistent behavior across platforms and makes distribution easier. You don't need to worry about users having the right DLLs or shared libraries installed.

## Module Organization

The engine is organized into clear modules:

**Core** - Fundamental systems like logging, error handling, configuration, and application lifecycle.

**Platform** - Platform-specific code wrapped in clean interfaces:
- **SDLManager** - Singleton-like static interface for SDL initialization and lifecycle management
- **Window** - High-level window creation and management, wraps SDL window functionality
- **Event System** - Type-safe event handling with three layers:
  - `Event` base class and derived event types (WindowCloseEvent, KeyEvent, etc.)
  - `EventDispatcher` - Core routing system owned by Application
  - `EventManager` - Singleton wrapper providing global access to events
- **EntryPoint** - Header-only entry point that provides `main()` function

**Utilities** - Helper functions and classes used throughout the engine.

Each module has a clear responsibility and minimal dependencies on other modules. This makes the codebase easier to understand and modify.

### Entry Point Pattern

The engine uses a header-only entry point pattern. `EntryPoint.h` defines `main()` in the global namespace. When you include `Sabora.h`, you get `main()` automatically. You only need to provide `CreateApplication()`:

```cpp
Application* CreateApplication() { return new MyApp(); }
// main() is provided by EntryPoint.h
```

This pattern keeps application code simple and focused. The engine handles all the boilerplate of creating, initializing, running, and cleaning up the application.

## Naming Conventions

The project follows consistent naming to make code easier to read:

- **Files**: PascalCase (Application.h, not application.h)
- **Classes**: PascalCase (ConfigurationManager)
- **Functions**: PascalCase (Initialize, ReadTextFile)
- **Variables**: camelCase with m_ prefix for members (m_window, m_config)
- **Constants**: UPPER_SNAKE_CASE (SB_CORE_INFO)

These conventions are enforced throughout the codebase to maintain consistency.

## Future Directions

The current architecture provides a solid foundation. Planned additions include:

- Rendering system with graphics API abstraction
- Scene management using an Entity-Component-System
- Resource management for assets
- Input system unification
- Audio system with 3D positioning
- Physics integration
- Scripting support

The modular design makes it straightforward to add these systems without disrupting existing code.

## Performance Considerations

Performance is considered at every level:

- Header-only libraries reduce build times
- Move semantics avoid unnecessary copies
- RAII has zero runtime overhead
- Explicit error handling avoids exception costs
- Minimal virtual function calls in hot paths

The goal is to provide powerful abstractions without sacrificing performance. So you get the benefits of modern C++.

## Conclusion

Sabora's architecture prioritizes clarity, safety, and performance. Every design decision is made with these goals in mind. The result is an engine that's both powerful and understandable, making it easier to build great games.
