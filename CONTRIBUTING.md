# Contributing to Sabora

Thank you for your interest in contributing to Sabora! This document provides guidelines and instructions for contributing.

## Development Setup

See the main README.md for setup instructions.

## Testing

### Running Tests

All tests use doctest and are located in the `Tests/` directory.

**Windows:**
```powershell
Tools\Premake\premake5.exe vs2022
msbuild Build\Sabora.sln /p:Configuration=Debug /p:Platform=x64 /p:Project=Tests
Build\bin\Debug_x64\Tests\Tests.exe
```

**Linux/macOS:**
```bash
Tools/Premake/premake5 gmake2
cd Build && make Tests config=debug_x64
./bin/Debug_x64/Tests/Tests
```

### Writing Tests

1. Create test files in `Tests/Source/` with naming pattern `Test*.cpp`
2. Use doctest macros: `TEST_SUITE`, `TEST_CASE`, `CHECK`, `REQUIRE`
3. Tests are automatically discovered and run

Example:
```cpp
#include "doctest.h"
#include "MyComponent.h"

TEST_SUITE("MyComponent")
{
    TEST_CASE("Feature works")
    {
        MyComponent comp;
        CHECK(comp.DoSomething() == true);
    }
}
```

## Code Style

- Follow existing code style
- Use descriptive names (no abbreviations per project rules)
- Capitalize folder/file names (e.g., `Source/`, not `src/`)
- Document code with comments
- Keep code professional and maintainable

## Pull Request Process

1. Create a feature branch
2. Make your changes
3. Add/update tests
4. Ensure all tests pass
5. Submit PR with description

CI will automatically run tests on Windows, Linux, and macOS.
