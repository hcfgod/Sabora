# Sabora Engine Tests

This directory contains the test suite for the Sabora Engine using doctest.

## Structure

- `Source/main.cpp` - Test runner entry point
- `Source/Test*.cpp` - Unit tests for individual components
- `Source/TestIntegration.cpp` - Integration tests

## Running Tests

### Windows
```powershell
# Generate project files
Tools\Premake\premake5.exe vs2022

# Build tests
msbuild Build\Sabora.sln /p:Configuration=Debug /p:Platform=x64 /p:Project=Tests

# Run tests
Build\bin\Debug_x64\Tests\Tests.exe
```

### Linux
```bash
# Generate project files
Tools/Premake/premake5 gmake2

# Build tests
cd Build
make Tests config=debug_x64 -j$(nproc)

# Run tests
./bin/Debug_x64/Tests/Tests
```

### macOS
```bash
# Generate project files
Tools/Premake/premake5 gmake2

# Build tests
cd Build
make Tests config=debug_x64 -j$(sysctl -n hw.ncpu)

# Run tests
./bin/Debug_x64/Tests/Tests
```

## Test Options

doctest supports various command-line options:

- `--success` - Show successful test cases
- `--no-exitcode` - Don't return exit code based on test results
- `--reporters=xml` - Output results in XML format
- `--help` - Show all available options

## Writing Tests

Tests are organized by component. Each test file should:

1. Include `doctest/doctest.h`
2. Include the component being tested
3. Use `TEST_SUITE` to group related tests
4. Use `TEST_CASE` for individual test cases
5. Use `CHECK`, `REQUIRE`, etc. for assertions

Example:
```cpp
#include <doctest/doctest.h>
#include "MyComponent.h"

TEST_SUITE("MyComponent")
{
    TEST_CASE("Feature works correctly")
    {
        MyComponent component;
        CHECK(component.DoSomething() == true);
    }
}
```

