# C++ CMake Template

A modern C++ project template using CMake, GoogleTest, and TDD workflow.

## Using This Template

### GitHub Users

Click the **"Use this template"** button above, or use the GitHub CLI:

```bash
gh repo create my-project --template <username>/claude-cpp-template --clone
cd my-project
```

### GitLab / Other Platforms

```bash
# Clone the template
git clone --depth 1 <template-url> my-project
cd my-project

# Remove template history and reinitialize
rm -rf .git
git init
git add .
git commit -m "Initial commit from claude-cpp-template"

# Update the project name in CMakeLists.txt
# Change: project(claude_cpp_template ...) to project(my_project ...)
```

### After Creating Your Project

1. Update `project()` name in `CMakeLists.txt`
2. Rename `include/mylib/` and `src/` library files as needed
3. Update test file names in `tests/`

## Features

- **C++20** standard with Clang toolchain
- **CMake** build system with presets
- **GoogleTest** for unit testing (via FetchContent)
- **TDD workflow** with dedicated presets
- **Thread Sanitizer** for race condition detection
- **Address Sanitizer** for memory error detection
- **Code coverage** with llvm-cov
- **clang-format** for code formatting (Google style)
- **clang-tidy** for static analysis
- **VSCode integration** with C++ TestMate debugging

## Project Structure

```
├── include/mylib/    # Public headers
├── src/              # Library implementation
├── app/              # Application executable
├── tests/            # Unit tests
├── cmake/            # CMake modules
├── context/          # Reference documentation
│   ├── standards/    # Coding standards (linked to Claude Code)
│   ├── libraries/    # Project-specific library docs
│   └── project/      # Human reference docs
└── .claude/          # Claude Code configuration
```

## Requirements

- CMake 3.28+
- Clang 15+
- llvm-profdata, llvm-cov (for code coverage)

## Building

### Using CMake Presets

```bash
# Debug build
cmake --preset default
cmake --build build

# Release build
cmake --preset release
cmake --build build-release

# TDD with Thread Sanitizer (recommended for development)
cmake --preset tdd-tsan
cmake --build build-tdd-tsan

# TDD with Address Sanitizer + Coverage
cmake --preset tdd-asan
cmake --build build-tdd-asan
```

### Manual Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## Testing

```bash
# Run all tests
ctest --test-dir build --output-on-failure

# Run tests by label
ctest --test-dir build -L unit

# Run specific test
ctest --test-dir build -R "MyLibTest"
```

## Code Coverage

```bash
# Build with ASan preset (includes coverage)
cmake --preset tdd-asan
cmake --build build-tdd-asan

# Generate coverage report
cmake --build build-tdd-asan --target coverage

# View report (Linux)
xdg-open build-tdd-asan/coverage/index.html
```

## Code Quality

```bash
# Check formatting
clang-format --dry-run -Werror src/*.cpp include/mylib/*.hpp

# Apply formatting
clang-format -i src/*.cpp include/mylib/*.hpp

# Run static analysis
clang-tidy src/*.cpp -p build
```

## License

MIT License - see [LICENSE](LICENSE)
