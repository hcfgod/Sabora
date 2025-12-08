# Contributing to Sabora Engine

Thank you for your interest in contributing to the Sabora Engine! This document provides guidelines and instructions for contributing to the project.

## Table of Contents

1. [Code of Conduct](#code-of-conduct)
2. [Getting Started](#getting-started)
3. [Development Workflow](#development-workflow)
4. [Coding Standards](#coding-standards)
5. [Commit Guidelines](#commit-guidelines)
6. [Pull Request Process](#pull-request-process)
7. [Documentation](#documentation)
8. [Testing](#testing)

## Code of Conduct

### Our Standards

- **Be Respectful**: Treat all contributors with respect and professionalism
- **Be Constructive**: Provide helpful feedback and suggestions
- **Be Patient**: Understand that everyone has different skill levels and experiences
- **Be Open**: Welcome new ideas and different approaches

### Unacceptable Behavior

- Harassment, discrimination, or offensive comments
- Personal attacks or trolling
- Publishing others' private information without permission
- Other conduct that could reasonably be considered inappropriate

## Getting Started

### Prerequisites

- **C++20 Compatible Compiler**: MSVC 2019+, GCC 10+, or Clang 12+
- **CMake 3.20+** or **Premake5**: For build system generation
- **Git**: For version control
- **Platform-Specific Tools**: See platform-specific setup instructions

### Setting Up Development Environment

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/yourusername/Sabora.git
   cd Sabora
   ```

2. **Initialize Submodules**:
   ```bash
   git submodule update --init --recursive
   ```

3. **Generate Build Files**:
   ```bash
   # Windows
   .\Setup.bat
   
   # Linux
   ./Setup_linux.sh
   
   # macOS
   ./Setup_macos.sh
   ```

4. **Build the Project**:
   ```bash
   # Windows (Visual Studio)
   # Open Build/Sabora.sln and build
   
   # Linux/macOS
   cd Build
   make
   ```

## Development Workflow

### Branch Strategy

- **main**: Stable, production-ready code
- **develop**: Integration branch for features
- **feature/**: Feature branches (e.g., `feature/rendering-system`)
- **bugfix/**: Bug fix branches (e.g., `bugfix/memory-leak`)
- **hotfix/**: Critical fixes for main branch

### Creating a Branch

```bash
# Create and switch to a new feature branch
git checkout -b feature/your-feature-name

# Or from develop
git checkout develop
git pull
git checkout -b feature/your-feature-name
```

### Making Changes

1. **Create a branch** for your changes
2. **Make your changes** following coding standards
3. **Test your changes** thoroughly
4. **Document your changes** (code comments, API docs, etc.)
5. **Commit with clear messages**
6. **Push and create a Pull Request**

## Coding Standards

### General Principles

1. **Professional AAA Developer Standards**: Code should be production-ready and maintainable
2. **No Abbreviations**: Use full, descriptive names (e.g., `Source` not `src`, `Configuration` not `config`)
3. **Clear Naming**: Names should be self-documenting
4. **Documentation**: Comment complex logic and document public APIs
5. **Consistency**: Follow existing code style and patterns

### Naming Conventions

#### Files and Directories

- **Directories**: PascalCase, no abbreviations (e.g., `Source`, `Configuration`, `Rendering`)
- **Header Files**: PascalCase with `.h` extension (e.g., `Application.h`, `ConfigurationManager.h`)
- **Source Files**: PascalCase with `.cpp` extension (e.g., `Application.cpp`)

#### Code Elements

- **Classes**: PascalCase (e.g., `Application`, `ConfigurationManager`)
- **Functions**: PascalCase (e.g., `Initialize()`, `ReadTextFile()`)
- **Variables**: camelCase
  - Member variables: `m_` prefix (e.g., `m_mutex`, `m_defaultConfig`)
  - Static variables: `s_` prefix (e.g., `s_Initialized`)
  - Parameters: camelCase (e.g., `configPath`, `createDirectories`)
- **Constants**: UPPER_SNAKE_CASE (e.g., `MAX_BUFFER_SIZE`)
- **Enums**: PascalCase for type, PascalCase for values (e.g., `LogLevel::Info`)

### Code Style

#### Indentation and Formatting

- **Indentation**: 4 spaces (no tabs)
- **Line Length**: Prefer 100 characters, but readability is more important
- **Braces**: Opening brace on same line for functions/classes, new line for control flow

```cpp
// Good
class Application {
public:
    void Initialize();
    
    if (condition) {
        DoSomething();
    }
};

// Also acceptable for control flow
if (condition)
{
    DoSomething();
}
```

#### Comments

- **File Headers**: Include purpose and author information
- **Function Documentation**: Use Doxygen-style comments for public APIs
- **Inline Comments**: Explain *why*, not *what* (the code should be self-explanatory)
- **Complex Logic**: Add comments for non-obvious algorithms or business logic

```cpp
/**
 * @brief Initialize the application and platform systems.
 * @return Result indicating success or failure with error details.
 * 
 * This method initializes SDL and other platform-specific systems.
 * Must be called before Run().
 */
Result<void> Initialize();

// Complex algorithm - explain the approach
// Use recursive merge to allow partial overrides of nested configurations
if (result.contains(key) && result[key].is_object() && val.is_object()) {
    result[key] = MergeJson(result[key], val);
}
```

#### Error Handling

- **Use Result<T>**: Prefer `Result<T>` over exceptions for error handling
- **Explicit Checks**: Always check `IsSuccess()` or `IsFailure()` before accessing values
- **Error Propagation**: Use `SB_TRY` macro or `AndThen()` for error propagation

```cpp
// Good
auto result = LoadFile("config.json");
if (result.IsFailure()) {
    SB_CORE_ERROR("Failed to load config: {}", result.GetError().ToString());
    return Result<void>::Failure(result.GetError());
}
auto config = result.Value();

// Also good - using SB_TRY
auto config = SB_TRY(LoadFile("config.json"));
```

#### Memory Management

- **Smart Pointers**: Prefer `std::unique_ptr` or `std::shared_ptr` over raw pointers
- **RAII**: Use RAII for all resource management
- **No Raw new/delete**: Avoid raw `new` and `delete` in favor of smart pointers

```cpp
// Good
std::unique_ptr<SDLContext> context = SDLContext::Create(flags);

// Avoid
SDLContext* context = new SDLContext();  // Don't do this
```

#### Const Correctness

- **Const Methods**: Mark methods as `const` when they don't modify state
- **Const References**: Use `const&` for parameters that aren't modified
- **Const Variables**: Use `const` for variables that don't change

```cpp
// Good
nlohmann::json Get() const;
void ProcessData(const std::string& data);

// Avoid
nlohmann::json Get();  // If it doesn't modify state
void ProcessData(std::string data);  // Unnecessary copy
```

### Header Organization

1. **Include Guards**: Use `#pragma once`
2. **Includes Order**:
   - Corresponding header (for .cpp files)
   - System headers (`<iostream>`, `<string>`, etc.)
   - Third-party headers (`<spdlog/...>`, `<SDL3/...>`, etc.)
   - Project headers (`"Log.h"`, `"Result.h"`, etc.)

```cpp
#pragma once

#include <filesystem>
#include <mutex>
#include <string>

#include <nlohmann/json.hpp>

#include "AsyncIO.h"
```

## Commit Guidelines

### Commit Message Format

Use clear, descriptive commit messages:

```
<type>: <subject>

<body (optional)>

<footer (optional)>
```

### Types

- **feat**: New feature
- **fix**: Bug fix
- **docs**: Documentation changes
- **style**: Code style changes (formatting, etc.)
- **refactor**: Code refactoring
- **test**: Adding or updating tests
- **chore**: Maintenance tasks

### Examples

```
feat: Add configuration manager with JSON support

Implements a thread-safe configuration manager that supports
layered configurations (default + user overrides) with deep
merging of nested JSON objects.

fix: Resolve memory leak in AsyncIO file operations

The file handle was not being properly closed in error cases.
Now using RAII wrapper to ensure cleanup.

docs: Add API documentation for Log class

Added comprehensive Doxygen-style comments for all public
methods in the Log class.
```

### Commit Best Practices

- **Atomic Commits**: Each commit should represent a single logical change
- **Clear Messages**: Write commit messages that explain *what* and *why*
- **Test Before Committing**: Ensure code compiles and tests pass
- **No WIP Commits**: Avoid committing work-in-progress code to shared branches

## Pull Request Process

### Before Submitting

1. **Update Documentation**: Ensure all public APIs are documented
2. **Add Tests**: Include tests for new functionality
3. **Check Style**: Ensure code follows project style guidelines
4. **Test Build**: Verify the project builds on your platform
5. **Update CHANGELOG**: Add entry describing your changes (if applicable)

### Creating a Pull Request

1. **Push Your Branch**:
   ```bash
   git push origin feature/your-feature-name
   ```

2. **Create PR on GitHub**: 
   - Use a clear, descriptive title
   - Fill out the PR template
   - Link related issues
   - Describe what changes were made and why

3. **PR Description Should Include**:
   - Summary of changes
   - Motivation/context
   - Testing performed
   - Screenshots (if UI changes)
   - Breaking changes (if any)

### Review Process

- **Automated Checks**: PRs must pass CI/CD checks (build, tests, linting)
- **Code Review**: At least one approval required before merging
- **Address Feedback**: Respond to review comments and make requested changes
- **Keep PRs Focused**: Avoid mixing unrelated changes in a single PR

### Merging

- **Squash and Merge**: PRs are typically squashed into a single commit
- **Rebase**: Keep your branch up to date with the target branch
- **Clean History**: Maintain a clean, linear git history

## Documentation

### Code Documentation

- **Public APIs**: All public classes, functions, and methods must have Doxygen comments
- **Complex Logic**: Add inline comments explaining non-obvious algorithms
- **Examples**: Include usage examples in documentation when helpful

### Documentation Files

- **README.md**: Project overview and quick start
- **ARCHITECTURE.md**: System architecture and design decisions
- **CONTRIBUTING.md**: This file - contribution guidelines
- **API Documentation**: Generated from code comments (future)

## Testing

### Writing Tests

- **Unit Tests**: Test individual functions and classes
- **Integration Tests**: Test system interactions
- **Use doctest**: The project uses doctest for testing

### Test Structure

```cpp
TEST_CASE("ConfigurationManager loads default config")
{
    ConfigurationManager config("test_defaults.json");
    REQUIRE(config.Initialize() == true);
    
    auto merged = config.Get();
    REQUIRE(merged.contains("window"));
}
```

### Running Tests

```bash
# Build tests
# Run test executable
./Build/bin/Debug_x64/EngineTests
```

## Questions?

If you have questions about contributing:

1. **Check Documentation**: Read through existing documentation first
2. **Search Issues**: Check if your question has been asked before
3. **Ask in Discussions**: Use GitHub Discussions for questions
4. **Open an Issue**: For bugs or feature requests

## Thank You!

Your contributions help make Sabora Engine better for everyone. We appreciate your time and effort!