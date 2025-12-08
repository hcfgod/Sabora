# Sabora Engine

A modern C++ game engine built with performance and ease of use in mind.

## Features

- **Modern C++20** - Leveraging the latest C++ features
- **Comprehensive Logging System** - Built on top of spdlog with game engine specific features
- **Cross-Platform Build System** - Using Premake5 for easy project generation
- **Modular Architecture** - Clean separation of concerns with well-defined interfaces

## Logging System

The Sabora Engine includes a powerful logging system built on top of spdlog, similar to the Cherno game engine series.

### Basic Usage

```cpp
#include "Sabora.h"

// Simple logging
SB_INFO("Application started");
SB_WARN("This is a warning");
SB_ERROR("An error occurred");

// Category-based logging
SB_CORE_INFO("Core system initialized");
SB_RENDERER_DEBUG("Rendering frame");
SB_PHYSICS_WARN("Physics simulation warning");
SB_AUDIO_ERROR("Audio system error");
```

### Log Levels

- `TRACE` - Detailed trace information
- `DEBUG` - Debug information
- `INFO` - General information
- `WARN` - Warning messages
- `ERROR` - Error messages
- `CRITICAL` - Critical errors

### Log Categories

- `CORE` - Core engine systems
- `RENDERER` - Graphics and rendering
- `AUDIO` - Audio systems
- `PHYSICS` - Physics simulation
- `INPUT` - Input handling
- `SCENE` - Scene management
- `SCRIPT` - Scripting systems
- `NETWORK` - Networking
- `EDITOR` - Editor tools
- `CLIENT` - Client application

### Format-based Logging

```cpp
// Format strings with arguments
SB_INFO("Player {} joined at position ({:.2f}, {:.2f})", playerName, x, y);
SB_CORE_DEBUG("Frame time: {:.3f}ms", frameTime);
```

## Building

### Windows

```powershell
# Using PowerShell
.\Setup.bat -Configuration Debug

# Or double-click build.bat
```

### Linux

```bash
./Setup_linux.sh --config Debug --platforms x64
```

### macOS

```bash
./Setup_macos.sh --config Debug --platforms x64
```

## Project Structure

```
Sabora/
├── Engine/           # Core engine code
│   ├── Source/      # Source files
│   └── Vendor/      # Third-party libraries
├── Sandbox/         # Test application
├── Scripts/         # Build scripts
└── Tools/           # Development tools
```

## Dependencies

- **spdlog** - Fast logging library
- **GLM** - Mathematics library
- **nlohmann/json** - JSON parsing
- **doctest** - Unit testing framework

## Documentation

- **[ARCHITECTURE.md](ARCHITECTURE.md)**: Detailed architecture documentation, design principles, and system organization
- **[CONTRIBUTING.md](CONTRIBUTING.md)**: Guidelines for contributing to the project

## License

[Add your license here]