# CLAUDE.md

This file provides guidance to Claude Code when working with this repository.

## Project Overview

C++ SHM counter publisher using zenoh-cpp, proving cross-language interop with the Dart subscriber from zenoh-counter-dart. This is the second of three template repos:

1. **zenoh-counter-dart** (COMPLETE) -- Pure Dart CLI, validates package:zenoh + SHM
2. **zenoh-counter-cpp** (this repo) -- C++ SHM publisher, validates cross-language interop
3. **zenoh-counter-flutter** (THIRD) -- Flutter app, validates mobile + desktop UI

The counter is the vehicle; the real deliverable is a reusable pattern for C++ + zenoh-cpp + SHM applications.

## Project Structure

```
zenoh-counter-cpp/
  app/
    main.cpp                  # CLI entry point (arg parsing, signal handling, publish loop)
  include/counter/
    publisher.hpp             # ShmCounterPublisher class declaration
  src/
    publisher.cpp             # ShmCounterPublisher implementation
  tests/
    test_publisher.cpp        # GoogleTest: SHM alloc, int64 encoding, publish
  ext/
    zenoh-c/                  # git submodule (v1.7.2)
    zenoh-cpp/                # git submodule (v1.7.2, header-only)
  context/
    roles/                    # CA/CP/CI session prompts
    standards/                # C++ coding standards (from template)
    libraries/                # Library-specific docs (zenoh-cpp reference)
  docs/
    lessons-learned.md        # Living doc, populated during development
  cmake/
    Coverage.cmake            # Code coverage support
  CMakeLists.txt
  CMakePresets.json
  CLAUDE.md
  README.md
```

## Dependencies

### zenoh-cpp (header-only, via git submodule)

zenoh-cpp v1.7.2 wraps zenoh-c with a modern C++ API. Header-only -- no compilation needed for zenoh-cpp itself.

```
ext/zenoh-c/      # v1.7.2 -- must be built (Rust + CMake)
ext/zenoh-cpp/    # v1.7.2 -- header-only, just include paths
```

### zenoh-c (native library prerequisite)

zenoh-c must be built with SHM and unstable API support:

```bash
cmake -S ext/zenoh-c -B ext/zenoh-c/build -G Ninja \
  -DCMAKE_C_COMPILER=/usr/bin/clang \
  -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=TRUE \
  -DZENOHC_BUILD_IN_SOURCE_TREE=TRUE \
  -DZENOHC_BUILD_WITH_SHARED_MEMORY=TRUE \
  -DZENOHC_BUILD_WITH_UNSTABLE_API=TRUE

RUSTUP_TOOLCHAIN=stable cmake --build ext/zenoh-c/build --config Release
```

Build artifacts:
- Shared library: `ext/zenoh-c/target/release/libzenohc.so`
- Headers: `ext/zenoh-c/include/`

### zenoh-dart reference (companion project)

The zenoh-counter-dart project at `/home/hugo-bluecorn/bluecorn/CSR/git/zenoh-counter-dart/` contains the Dart subscriber that receives from this C++ publisher. The counter protocol must match:

| What | Value |
|------|-------|
| Key expression | `demo/counter` (default) |
| Payload format | Raw little-endian int64 (8 bytes) |
| Transport | SHM zero-copy (same machine) or standard (cross-machine) |

## Build Commands

```bash
# Debug build (default)
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

## Testing

```bash
# Run all tests
ctest --test-dir build --output-on-failure

# Run tests by label
ctest --test-dir build -L unit

# Run specific test pattern
ctest --test-dir build -R "Publisher"
```

## Running

```bash
# Run the counter publisher (default: demo/counter, 1000ms interval)
./build/counter_pub

# With custom key and interval
./build/counter_pub -k demo/myapp/counter -i 500

# Connect to a router
./build/counter_pub -e tcp/127.0.0.1:7447

# Listen for peer connections
./build/counter_pub -l tcp/0.0.0.0:7447
```

### CLI Flags (mirror zenoh-c conventions)

- `-k, --key <KEYEXPR>` -- key expression (default: `demo/counter`)
- `-e, --connect <ENDPOINT>` -- connect to endpoint (optional, repeatable)
- `-l, --listen <ENDPOINT>` -- listen on endpoint (optional, repeatable)
- `-i, --interval <MS>` -- publish interval in milliseconds (default: 1000)
- `-h, --help` -- show usage

### Cross-Language Interop Test

Run the C++ publisher and Dart subscriber together:

```bash
# Terminal 1: C++ SHM publisher (this repo)
./build/counter_pub -l tcp/0.0.0.0:7448

# Terminal 2: Dart subscriber (zenoh-counter-dart repo)
cd /path/to/zenoh-counter-dart
fvm dart run bin/counter_sub.dart -e tcp/127.0.0.1:7448
```

## Architecture

### Data Flow

```
counter_pub (C++)                    counter_sub (Dart)
  |                                    |
  PosixShmProvider(65536)             Session.declareSubscriber('demo/counter')
  |                                    |
  alloc_gc_defrag_blocking(8)         subscriber.stream.listen((sample) {
  |                                     sample.payloadBytes -> int64
  memcpy(buf.data(), &counter, 8)     })
  |
  pub.put(std::move(buf))  // zero-copy SHM
```

### Counter Protocol

- Format: raw little-endian int64 (8 bytes)
- Published on key expression `demo/counter` (configurable via `-k`)
- SHM zero-copy path: alloc -> memcpy -> put(move)
- Receiver (Dart) transparently receives SHM data via standard subscriber

### Topologies Supported

| Topology | Publisher | Subscriber | Router |
|----------|-----------|------------|--------|
| Peer direct | `-l tcp/0.0.0.0:7447` | `-e tcp/localhost:7447` | no |
| Via router | `-e tcp/router:7447` | `-e tcp/router:7447` | yes |

### No State Machine

This is a simple incrementing publisher. No play/pause/stop/reset. Just:
1. Open session
2. Create SHM provider
3. Declare publisher
4. Loop: alloc -> write int64 -> put -> sleep
5. Signal handler for graceful shutdown

## Style Guide

Follows Google C++ Style Guide (enforced via `.clang-format` and `.clang-tidy`):

| Element | Style | Example |
|---------|-------|---------|
| Files | `snake_case` | `publisher.cpp` |
| Classes | `PascalCase` | `ShmCounterPublisher` |
| Functions | `PascalCase` | `Publish()` |
| Variables | `snake_case` | `buffer_size` |
| Members | `snake_case_` | `counter_` |
| Constants | `kPascalCase` | `kDefaultKey` |

## Code Quality

```bash
# Check formatting
clang-format --dry-run -Werror src/*.cpp include/counter/*.hpp

# Apply formatting
clang-format -i src/*.cpp include/counter/*.hpp app/*.cpp tests/*.cpp

# Static analysis
clang-tidy src/*.cpp -p build
```

## Commit Scope Naming

Use the primary module as `<scope>` in commit messages:
- `feat(publisher): ...` for SHM publisher logic
- `test(publisher): ...` for publisher tests
- `build: ...` for CMake, submodules, presets
- `docs: ...` for documentation changes
- `chore: ...` for config, cleanup

## Session Roles

This project uses a three-session workflow:

| Session | Role | Scope |
|---------|------|-------|
| **CA** | Architect / Reviewer | Decisions, reviews, memory |
| **CP** | Planner | Slice decomposition, TDD plans |
| **CI** | Implementer | Code, tests, releases |

Role prompts are in `context/roles/`. Each session reads its role doc before starting.

## TDD Workflow Plugin

This project uses the **tdd-workflow** Claude Code plugin for structured
test-driven development.

### Available Commands

- **`/tdd-plan <feature description>`** -- Create a TDD implementation plan
- **`/tdd-implement`** -- Start or resume TDD implementation for pending slices
- **`/tdd-release`** -- Finalize and release a completed TDD feature

### Session State

If `.tdd-progress.md` exists at the project root, a TDD session is in progress.
Read it to understand the current state before making changes.

### Testing Constraints

- Tests require zenoh-c built with SHM support (see Dependencies)
- RPATH is set in CMake so executables find libzenohc.so automatically
- No mocking of zenoh -- tests call real zenoh-cpp through libzenohc.so
- Two-session testing: use explicit TCP listen/connect with unique ports per test group
- SHM alloc can fail -- always check the result variant

### zenoh-cpp API Reference (v1.7.2)

Key classes from `zenoh.hxx`:

- `Session` -- `Session::open(config)`, `declare_publisher()`, `declare_subscriber()`
- `Publisher` -- `put(Bytes)`, `put(ZShmMut)` (SHM zero-copy)
- `PosixShmProvider` -- `PosixShmProvider(MemoryLayout)`, `alloc_gc_defrag_blocking(size, alignment)`
- `ZShmMut` -- mutable SHM buffer, `data()` returns pointer, moved into `put()`
- `Config` -- `Config::create_default()`, `insert_json5(key, value)`
- `KeyExpr` -- key expression construction
- `Bytes` -- payload container (from raw bytes or string)
- `Sample` -- received data with `get_payload()`, `get_keyexpr()`
- `ZException` -- error type

### Session Directives

When /tdd-plan completes, always show the FULL plan text produced by the planner
agent -- every slice with Given/When/Then, acceptance criteria, and dependencies.
Never summarize or abbreviate the plan output.
