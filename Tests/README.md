# Test Suite

The Tests directory contains the engine's test suite, built with doctest. These tests verify that all systems work correctly and help catch regressions when making changes.

## Running Tests

After building the project, you can run tests from the command line:

**Windows:**
```powershell
.\Build\bin\Debug_x64\Tests\Tests.exe
```

**Linux/macOS:**
```bash
./Build/bin/Debug_x64/Tests/Tests
```

Tests will run automatically and report results. You'll see which tests passed, which failed, and any assertion messages from failures.

## Test Organization

Tests are organized by component. Each test file covers a specific system:

- `TestLog.cpp` - Logging system tests
- `TestResult.cpp` - Error handling tests
- `TestConfigurationManager.cpp` - Configuration system tests
- `TestAsyncIO.cpp` - File I/O tests
- `TestGLAD.cpp` - OpenGL loader tests
- And more...

Integration tests verify that multiple systems work together correctly.

## Writing Tests

When adding new features or fixing bugs, add tests. A good test:

- Has a clear, descriptive name
- Tests one specific thing
- Uses appropriate assertions (CHECK for non-critical, REQUIRE for critical)
- Is independent of other tests

Example:

```cpp
#include "doctest.h"
#include "MyComponent.h"

TEST_SUITE("MyComponent")
{
    TEST_CASE("Initializes with default values")
    {
        MyComponent component;
        CHECK(component.GetValue() == 0);
    }
    
    TEST_CASE("Handles invalid input gracefully")
    {
        MyComponent component;
        auto result = component.Process(-1);
        REQUIRE(result.IsFailure());
    }
}
```

## Test Options

doctest supports various command-line options for controlling test execution:

- `--success` - Show successful test output
- `--no-exitcode` - Don't return error code on failure
- `--reporters=xml` - Output results in XML format
- `--help` - Show all available options

Run tests with `--help` to see all options.

## Continuous Integration

Tests run automatically on every pull request via GitHub Actions. All tests must pass before code can be merged. This ensures the codebase stays stable and working.

If you're making changes, make sure tests pass locally before submitting a pull request. This saves time and keeps the CI pipeline green.
