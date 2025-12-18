# Contributing to Sabora

Thank you for considering contributing to Sabora. Your help makes this project better for everyone.

## Getting Started

First, make sure you can build and run the project. See the main README for setup instructions. Once you can build successfully and all tests pass, you're ready to contribute.

## How to Contribute

### Reporting Issues

If you find a bug or have a suggestion, please open an issue. Include as much detail as possible:

- What you were trying to do
- What actually happened
- Steps to reproduce (if it's a bug)
- Your platform and compiler version

Clear, detailed reports help us fix issues faster.

### Making Changes

1. **Create a branch** - Use a descriptive name like `fix-window-resize-bug` or `add-audio-system`

2. **Make your changes** - Follow the existing code style and conventions

3. **Add tests** - If you're adding a feature, add tests for it. If you're fixing a bug, add a test that would have caught it

4. **Run tests** - Make sure all existing tests still pass and your new tests work

5. **Update documentation** - If you're changing behavior or adding features, update the relevant documentation

6. **Submit a pull request** - Include a clear description of what changed and why

## Code Style

The project follows consistent style to keep the codebase readable:

- **Files and folders**: PascalCase (Source/, Application.h, not src/, application.h)
- **Classes**: PascalCase (ConfigurationManager)
- **Functions**: PascalCase (Initialize, ReadTextFile)
- **Variables**: camelCase with m_ prefix for members (m_window, m_config)
- **Constants**: UPPER_SNAKE_CASE (SB_CORE_INFO)

Follow the existing code style in the file you're editing. Consistency is more important than any particular style choice.

## Writing Tests

Tests use doctest and live in the Tests directory. Each test file should:

- Include doctest.h
- Include the component being tested
- Use TEST_SUITE to group related tests
- Use TEST_CASE for individual test cases
- Use CHECK or REQUIRE for assertions

Tests should be clear and focused. A good test verifies one thing and has a descriptive name that explains what it's testing.

## Pull Request Process

When you submit a pull request:

1. **Describe your changes** - What did you change and why? What problem does it solve?

2. **Reference issues** - If your PR fixes an issue, reference it in the description

3. **Keep changes focused** - One PR should address one thing. If you're doing multiple unrelated changes, split them into separate PRs

4. **Ensure tests pass** - CI will run tests automatically, but make sure they pass locally first

5. **Be responsive** - If reviewers ask questions or request changes, respond promptly

## Development Workflow

The typical workflow looks like this:

1. Create a feature branch from main
2. Make your changes
3. Write or update tests
4. Run tests locally
5. Commit with clear messages
6. Push and create a pull request
7. Address any feedback
8. Once approved, your changes will be merged

## Code Review

All contributions go through code review. Reviewers will check for:

- Correctness - Does the code do what it's supposed to?
- Style - Does it follow project conventions?
- Tests - Are there adequate tests?
- Documentation - Is the code documented appropriately?

Don't take feedback personally. Code review is about making the codebase better, not criticizing your work. Everyone's code gets reviewed, including maintainers.

## Questions?

If you have questions about contributing, feel free to open an issue with the "question" label. We're happy to help you get started.

Thank you for contributing to Sabora.
