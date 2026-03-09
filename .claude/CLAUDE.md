# zenoh-counter-cpp - Claude Code Configuration

## Project Overview

C++ SHM counter publisher using zenoh-cpp v1.7.2. Publishes incrementing
little-endian int64 on `demo/counter` via shared memory. Companion to
zenoh-counter-dart (Dart subscriber).

## Development Rules

### Mandatory Requirements

1. **TDD is mandatory** - All new features must follow Red-Green-Refactor
2. **Google C++ Style Guide** - Enforced via clang-format and clang-tidy
3. **Feature branches** - Never commit directly to main
4. **CHANGELOG updates** - Required before every release
5. **Tests must pass** - No commits with failing tests

### Build Commands

```bash
# Configure and build
cmake --preset default && cmake --build build

# Run tests
ctest --test-dir build --output-on-failure

# TDD build with thread sanitizer (race detection)
cmake --preset tdd-tsan && cmake --build build-tdd-tsan

# TDD build with address sanitizer + coverage
cmake --preset tdd-asan && cmake --build build-tdd-asan

# Generate coverage report
cmake --build build-tdd-asan --target coverage
```

### Code Quality

```bash
# Format code
clang-format -i src/*.cpp include/counter/*.hpp app/*.cpp tests/*.cpp

# Check formatting
clang-format --dry-run -Werror src/*.cpp include/counter/*.hpp

# Static analysis
clang-tidy -p build src/*.cpp
```

## Key Documentation

@context/standards/cpp-coding-standards.md
@context/standards/cpp-tdd.md
@context/standards/cpp-tooling.md
@context/standards/cpp-commits.md
@context/standards/cpp-versioning.md
@context/standards/cpp-changelog.md
@context/standards/version-control.md
@context/standards/cmake-best-practices.md
@context/standards/googletest-reference.md

## Project Structure

```
.
├── include/counter/   # Public headers (ShmCounterPublisher)
├── src/               # Library implementation
├── app/               # CLI executable (main.cpp)
├── tests/             # Unit + integration tests (GoogleTest)
├── cmake/             # CMake modules (Coverage.cmake)
├── context/           # Reference documentation
│   ├── roles/         # CA/CP/CI session prompts
│   ├── standards/     # Coding standards & guidelines
│   ├── libraries/     # Project-specific library docs
│   └── project/       # Project-specific docs (human reference)
├── ext/               # External submodules
│   ├── zenoh-c/       # v1.7.2 (must be built with SHM support)
│   └── zenoh-cpp/     # v1.7.2 (header-only)
├── issues/            # Issue specifications
└── .claude/           # Claude Code configuration
```

## Quick Reference

### Naming Conventions (Google Style)

| Element | Style | Example |
|---------|-------|---------|
| Files | `snake_case` | `publisher.cpp` |
| Classes | `PascalCase` | `ShmCounterPublisher` |
| Functions | `PascalCase` | `Publish()` |
| Variables | `snake_case` | `buffer_size` |
| Members | `snake_case_` | `counter_` |
| Constants | `kPascalCase` | `kDefaultKey` |

### Commit Types

| Type | Use |
|------|-----|
| `feat` | New feature |
| `fix` | Bug fix |
| `refactor` | Restructure without behavior change |
| `test` | Add/fix tests (existing code) |
| `docs` | Documentation only |
| `build` | CMake, dependencies, submodules |
| `chore` | Config, cleanup |

### TDD Phases

1. **RED** - Write failing test, commit
2. **GREEN** - Minimal implementation, commit
3. **REFACTOR** - Improve quality, commit

## Git Workflow

1. Create feature branch: `git checkout -b feature/<name>`
2. Follow TDD workflow
3. Update CHANGELOG.md
4. Create PR to main
5. Squash and merge
