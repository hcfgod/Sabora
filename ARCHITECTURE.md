# Sabora Engine Architecture

This document provides an overview of the Sabora Engine architecture, design principles, and system organization.

## Table of Contents

1. [Overview](#overview)
2. [Design Principles](#design-principles)
3. [Core Systems](#core-systems)
4. [Module Organization](#module-organization)
5. [Build System](#build-system)
6. [Dependencies](#dependencies)

## Overview

Sabora is a modern C++ game engine designed with performance, maintainability, and extensibility in mind. The engine follows a modular architecture where core systems are separated into distinct modules with well-defined interfaces.

### Key Characteristics

- **Modern C++20**: Leverages latest C++ features for type safety and performance
- **RAII-based Resource Management**: Automatic resource cleanup through smart pointers and RAII patterns
- **Explicit Error Handling**: Uses Result<T> type for error handling instead of exceptions
- **Thread-Safe Core Systems**: Critical systems use mutexes for thread safety
- **Layered Configuration**: Default + user override configuration system
- **Comprehensive Logging**: Category-based logging with multiple output targets

## Design Principles

### 1. Resource Management

The engine follows RAII (Resource Acquisition Is Initialization) principles:

- **Smart Pointers**: Use `std::unique_ptr` and `std::shared_ptr` for automatic memory management
- **RAII Wrappers**: Platform resources (like SDL) are wrapped in RAII classes
- **Automatic Cleanup**: Destructors handle all resource cleanup

**Example**: `SDLContext` ensures SDL is properly initialized and cleaned up:

```cpp
class SDLContext {
    ~SDLContext() {
        if (m_Initialized) {
            SDL_Quit();
        }
    }
};
```

### 2. Error Handling

The engine uses a `Result<T>` type for explicit error handling consistently throughout:

- **No Exceptions**: All error handling uses `Result<T>` - no exceptions are thrown
- **Type-Safe Errors**: Error codes are strongly typed with categories
- **Error Propagation**: Chainable operations with `AndThen()`, `Map()`, `OrElse()`
- **Source Location Tracking**: Errors capture where they occurred for debugging
- **Consistent API**: All I/O operations, initialization, and critical paths return `Result<T>`

**Example**:
```cpp
Result<void> Initialize() {
    auto sdlResult = SDLContext::Create(SDL_INIT_VIDEO);
    if (sdlResult.IsFailure()) {
        return Result<void>::Failure(sdlResult.GetError());
    }
    m_SDLContext = std::move(sdlResult).Value();
    return Result<void>::Success();
}

// File I/O also uses Result<T>
auto fileResult = AsyncIO::ReadTextFile("config.txt");
if (fileResult.IsSuccess()) {
    auto contents = fileResult.Value();
    // Use contents...
}
```

### 3. Thread Safety

Core systems are designed for thread-safe access:

- **Mutex Protection**: Shared state is protected with `std::mutex` or `std::scoped_lock`
- **Immutable Access**: Read operations return copies when possible
- **Lock-Free Where Possible**: Atomic operations for simple flags

**Example**: `ConfigurationManager` uses mutexes for thread-safe access:

```cpp
nlohmann::json ConfigurationManager::Get() const {
    std::scoped_lock lock(m_mutex);
    return MergeJson(m_defaultConfig, m_userOverrides);
}
```

### 4. Configuration Management

Layered configuration system:

- **Default Config**: Read-only defaults (typically in application directory)
- **User Overrides**: User-specific settings (typically in user data directory)
- **Deep Merging**: Nested objects are recursively merged
- **JSON Pointer Access**: Use JSON pointers for precise value access

**Example**:
```cpp
ConfigurationManager config("defaults.json", "user.json");
config.Initialize();
config.SetValue("/window/width", 1920);  // User override
auto merged = config.Get();  // Merged view
```

## Core Systems

### Application

The `Application` class manages the engine lifecycle:

- **Initialization**: Sets up logging, SDL, and platform systems
- **Main Loop**: Handles events, updates, and rendering
- **Shutdown**: Cleanup in reverse order of initialization

**Lifecycle**:
1. Constructor: Initialize logging
2. `Initialize()`: Set up platform systems (SDL, etc.)
3. `Run()`: Main application loop
4. Destructor: Cleanup all resources

### Logging System

Built on spdlog with engine-specific features:

- **Categories**: Separate log streams for different systems (Core, Renderer, Audio, etc.)
- **Levels**: Trace, Debug, Info, Warn, Error, Critical
- **Multiple Outputs**: Console (with colors) and file logging
- **Per-Category Filtering**: Set different log levels per category

**Usage**:
```cpp
SB_CORE_INFO("Engine initialized");
SB_RENDERER_DEBUG("Rendering frame {}", frameNumber);
SB_PHYSICS_WARN("Low physics timestep");
```

### Configuration Manager

Thread-safe configuration management:

- **Layered Configs**: Default + user override merging
- **JSON-Based**: Uses nlohmann/json for parsing and storage
- **JSON Pointer API**: Precise value access and modification
- **Automatic Persistence**: Save/load configuration files

### AsyncIO

File I/O utilities with async support:

- **Synchronous Operations**: Blocking file read/write
- **Asynchronous Operations**: Non-blocking with `std::future`
- **JSON Support**: Built-in JSON parsing and serialization
- **Filesystem Helpers**: Directory creation, file listing, existence checks

### Result Type

Type-safe error handling system:

- **Result<T>**: Holds either a value or an error
- **Error Codes**: Categorized error codes (Core, Platform, Graphics, etc.)
- **Error Details**: Code, message, and source location
- **Functional Operations**: `Map()`, `AndThen()`, `OrElse()` for chaining

```
Application
    ├── Log
    ├── Result
    └── SDL (via SDLContext)

ConfigurationManager
    ├── AsyncIO
    └── nlohmann/json

AsyncIO
    └── std::filesystem

Log
    └── spdlog
```

### Naming Conventions

- **Files**: PascalCase for headers/source files (e.g., `Application.h`)
- **Classes**: PascalCase (e.g., `ConfigurationManager`)
- **Functions**: PascalCase (e.g., `Initialize()`, `ReadTextFile()`)
- **Variables**: camelCase with prefix for members (e.g., `m_mutex`, `m_defaultConfig`)
- **Constants**: UPPER_SNAKE_CASE (e.g., `SB_CORE_INFO`)

## Build System

The engine uses **Premake5** for cross-platform build generation:

### Supported Platforms

- **Windows**: Visual Studio projects (.vcxproj, .sln)
- **Linux**: Makefiles or other generators
- **macOS**: Xcode projects

### Build Configuration

- **Debug**: Development builds with symbols and assertions
- **Release**: Optimized production builds

### Project Structure

- **Engine**: Static library containing core engine code
- **Sandbox**: Executable application for testing engine features

## Dependencies

### Core Dependencies

- **spdlog**: Fast, header-only logging library
- **GLM**: OpenGL Mathematics library for vector/matrix operations
- **nlohmann/json**: Modern C++ JSON library
- **SDL3**: Cross-platform multimedia library (windowing, input, audio)

### Development Dependencies

- **doctest**: Lightweight unit testing framework
- **Premake5**: Build system generator

### Dependency Management

Dependencies are included as git submodules or vendor libraries in `Engine/Vendor/`. This ensures:

- **Version Control**: Specific versions are tracked
- **No External Builds**: Dependencies are built with the engine
- **Cross-Platform**: Works on all supported platforms

## Future Architecture Considerations

### Planned Systems

1. **Rendering System**: Graphics API abstraction (Vulkan/DirectX/OpenGL)
2. **Scene Management**: Entity-Component-System (ECS) architecture
3. **Resource Management**: Asset loading and caching system
4. **Input System**: Unified input handling across platforms
5. **Audio System**: 3D audio and sound effect management
6. **Physics System**: Integration with physics engine
7. **Scripting System**: Lua or similar scripting support

### Extension Points

The architecture is designed to support:

- **Plugin System**: Loadable modules for engine extensions
- **Custom Renderers**: Pluggable rendering backends
- **Asset Importers**: Extensible asset loading pipeline
- **Editor Integration**: Tools for game development

## Performance Considerations

### Design Decisions

1. **Header-Only Libraries**: Where possible, use header-only libraries (spdlog, GLM, nlohmann/json) to reduce build times
2. **Move Semantics**: Prefer move over copy for large objects
3. **RAII**: Zero-cost abstractions for resource management
4. **No Virtual Calls in Hot Paths**: Minimize virtual function calls in performance-critical code
5. **Explicit Error Handling**: Avoid exception overhead in critical paths

### Memory Management

- **Stack Allocation**: Prefer stack allocation for small, short-lived objects
- **Smart Pointers**: Use `unique_ptr` for exclusive ownership, `shared_ptr` for shared ownership
- **No Raw Pointers**: Avoid raw pointers for ownership - use smart pointers or references

## Threading Model

### Current State

- **Main Thread**: Application loop, rendering, and most engine systems
- **Async I/O**: File operations can run on background threads via `AsyncIO`
- **Thread-Safe Core**: Configuration and logging are thread-safe

### Future Considerations

- **Job System**: Multi-threaded task execution for parallel processing
- **Render Thread**: Separate thread for rendering operations
- **Asset Loading Thread**: Background asset loading and processing

## Conclusion

The Sabora Engine architecture prioritizes:

1. **Maintainability**: Clear separation of concerns, well-documented interfaces
2. **Performance**: Zero-cost abstractions, explicit error handling
3. **Extensibility**: Modular design allows easy addition of new systems
4. **Reliability**: RAII, explicit error handling, thread-safe core systems

This architecture provides a solid foundation for building a modern game engine while maintaining code quality and performance.

