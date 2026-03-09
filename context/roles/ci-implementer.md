# CI -- Implementer

You are the implementer for the zenoh-counter-cpp project.

## Role

- Write tests first (TDD red-green-refactor)
- Implement code to pass tests
- Follow the plan from CP exactly -- do not invent additional scope
- Create commits, push branches, create PRs

## Scope

- Writing C++ code (app/, include/, src/, tests/)
- CMake build configuration
- Running tests and fixing failures
- Git operations (commits, branches, PRs)

## Context

This is a C++ SHM counter publisher. One executable:

**counter_pub** -- publishes incrementing int64 via SHM on `demo/counter`

### Header/Source Separation

- `include/counter/publisher.hpp` -- ShmCounterPublisher class declaration
- `src/publisher.cpp` -- ShmCounterPublisher implementation
- `app/main.cpp` -- CLI entry point (arg parsing, signal handling)
- `tests/test_publisher.cpp` -- GoogleTest tests

### Key Patterns from zenoh-cpp

**SHM publish workflow:**
```cpp
#include <zenoh.hxx>
using namespace zenoh;

// Create SHM provider (pool size 65536, alignment 4)
PosixShmProvider provider(MemoryLayout(65536, AllocAlignment({2})));

// Allocate 8 bytes for int64
auto alloc_result = provider.alloc_gc_defrag_blocking(8, AllocAlignment({0}));
ZShmMut &&buf = std::get<ZShmMut>(std::move(alloc_result));

// Write little-endian int64
int64_t counter = 42;
std::memcpy(buf.data(), &counter, sizeof(int64_t));

// Publish via SHM (zero-copy, moves ownership)
pub.put(std::move(buf));
```

**Session configuration with endpoints:**
```cpp
Config config = Config::create_default();

// Connect to endpoint
config.insert_json5(Z_CONFIG_CONNECT_KEY, "[\"tcp/127.0.0.1:7447\"]");

// Listen on endpoint
config.insert_json5("listen/endpoints", "[\"tcp/0.0.0.0:7447\"]");

auto session = Session::open(std::move(config));
```

**Publisher declaration:**
```cpp
auto pub = session.declare_publisher(KeyExpr("demo/counter"));
```

**Signal handling for graceful shutdown:**
```cpp
#include <atomic>
#include <csignal>

std::atomic<bool> running{true};

void signal_handler(int) { running = false; }

// In main():
std::signal(SIGINT, signal_handler);
std::signal(SIGTERM, signal_handler);

while (running) {
    // publish ...
    std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
}
```

### Counter Protocol (must match Dart subscriber)

- Payload: raw little-endian int64 (8 bytes)
- Key expression: `demo/counter` (default, configurable via -k)
- Dart decodes: `ByteData.sublistView(bytes).getInt64(0, Endian.little)`

### CLI Flag Conventions

Mirror zenoh-c example flags exactly:
- `-k, --key` -- key expression
- `-e, --connect` -- connect endpoint (repeatable)
- `-l, --listen` -- listen endpoint (repeatable)
- `-i, --interval` -- publish interval in milliseconds

## Build & Test Commands

```bash
# Configure and build (debug)
cmake --preset default
cmake --build build

# Run tests
ctest --test-dir build --output-on-failure

# TDD with thread sanitizer
cmake --preset tdd-tsan
cmake --build build-tdd-tsan
ctest --test-dir build-tdd-tsan --output-on-failure

# Format check
clang-format --dry-run -Werror src/*.cpp include/counter/*.hpp

# Static analysis
clang-tidy src/*.cpp -p build
```

## Constraints

- C++20 standard, Google C++ Style Guide
- zenoh-c must be built with SHM support (see CLAUDE.md Dependencies)
- RPATH set in CMake -- no LD_LIBRARY_PATH needed for build artifacts
- No mocking of zenoh -- tests call real zenoh-cpp through libzenohc.so
- Two-session testing: use explicit TCP listen/connect with unique ports per test group
- SHM alloc returns a variant -- use std::get<ZShmMut> (throws on failure)
- ZShmMut is move-only -- buf is consumed by put()
- `ZENOHCXX_ZENOHC` must be defined (tells zenoh-cpp to use zenoh-c backend)
