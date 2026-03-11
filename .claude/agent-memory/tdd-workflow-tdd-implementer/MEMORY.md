# TDD Implementer Memory

## zenoh-cpp API Patterns
- `Publisher` is NOT a template class (no `Publisher<void>`), just `Publisher`
- `Session`, `Publisher`, `PosixShmProvider` are move-only (inherit from `Owned<>`)
- No default constructors for zenoh types -- use pimpl with `std::unique_ptr<Impl>`
- Impl struct takes all zenoh types by value in constructor, uses `std::move()`
- `Config::create_default()`, `Session::open(std::move(config))`
- `session.declare_publisher(KeyExpr(key_expr))`
- `PosixShmProvider(MemoryLayout(65536, AllocAlignment({2})))`
- Endpoint config: `config.insert_json5(Z_CONFIG_CONNECT_KEY, "[\"tcp/...\"]")`
- SHM alloc: `shm_provider.alloc_gc_defrag_blocking(size, AllocAlignment({0}))` returns variant
- Extract buffer: `std::get<ZShmMut>(std::move(result))`, then `buf.data()` for pointer
- Zero-copy publish: `publisher.put(std::move(buf))`
- Subscriber callback signature: `void(Sample& sample)` (non-const ref!)
- `session.declare_subscriber(KeyExpr(...), callback, closures::none)` returns `Subscriber<void>`
- Payload access: `sample.get_payload().as_vector()` returns `std::vector<uint8_t>`

## Build & Test
- Build: `cmake --preset default && cmake --build build`
- Test: `ctest --test-dir build --output-on-failure`
- Each test takes ~0.5s (zenoh session open overhead)
- zenoh connect to non-existent endpoint does NOT throw (lazy connection)

## Loopback Test Pattern (Slice 3+4+Sub)
- Use explicit TCP listen/connect for reliable pub/sub loopback tests
- Publisher: `ShmCounterPublisher("demo/counter", {}, {"tcp/0.0.0.0:<port>"})`
- Subscriber: separate Session via `ConnectSession(port)` helper
- Use unique ports per test group: Slice 2 (7450-7451), Slice 3 (7452-7454), Slice 4 (7460-7462), Sub (7480-7483)
- Sleep 500ms after subscriber setup for session discovery
- `Int64Collector` struct: thread-safe value collection with `Callback()` and `WaitFor(count)`
- `ConnectSession(port)`: creates a Session connecting to localhost on given port
- Each loopback test takes ~1.6s total

## Subprocess Testing Pattern
- Use `COUNTER_SUB_BIN` compile definition from CMake (`$<TARGET_FILE:counter_sub>`) for absolute path
- `popen()` + `pclose()` with `timeout` wrapper for process tests
- Check `WEXITSTATUS(status)` for exit code (124 = timeout)
- Run publisher in background thread, give subprocess 1s to start before publishing
- CTest runs tests from the build directory, NOT the project root -- relative paths break

## Project Structure (after Slice 4)
- Library: `counter_lib` (alias `counter::counter`)
- Header: `include/counter/publisher.hpp`
- Source: `src/publisher.cpp`
- Tests: `tests/test_publisher.cpp` -> executable `publisher_tests`
- App: `app/main.cpp` -> executable `counter_pub`
- Old mylib files removed

## TDD RED Phase Pattern
- Write test file with all test cases
- Create header with class declaration
- Create .cpp with constructor that throws `std::runtime_error("Not implemented")`
- Tests compile and fail due to throw = confirmed RED

## bashunit Patterns (Integration Tests)
- Test file naming: `test_*_test.sh` (bashunit convention)
- Source helpers relative to test file: `HELPERS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"`
- bashunit location: `~/.local/bin/bashunit`
- bashunit version: 0.33.0
- Assertions: `assert_equals`, `assert_not_equals`, `assert_contains`
- Capture stdout+stderr: `output=$(func args 2>&1)` then check `$?` and `$output`
- Override PATH to hide commands: `PATH="/usr/bin:/bin" command_here`
- shellcheck: `-S warning` level, ignore SC2034 for stub variables

## Dart Subscriber Integration
- Dart subscriber resolves native libraries automatically via build hooks (no `LD_LIBRARY_PATH` needed)
- Dart repo: `../zenoh-counter-dart` (hyphen), zenoh_dart SDK: `../zenoh_dart` (underscore)
- No `dart` directly in PATH, use `fvm dart run` from the Dart repo directory
- Dart sub output format: `Received: <int>` (one per line)
- Dart sub runs indefinitely (no `-n` flag), must be killed after collecting values
- `grep -c` with `|| true` safer than `|| echo "0"` (avoids multiline capture)
- Port 7500 for peer direct interop, bump for other topologies

## GoogleTest Patterns Used
- `EXPECT_NO_THROW({ ... })` for construction tests
- `EXPECT_EQ(obj.Method(), value)` inside EXPECT_NO_THROW blocks
- Namespace wrapping: `namespace counter { namespace { TEST(...) } }`
