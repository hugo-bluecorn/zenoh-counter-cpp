# Issue 001: SHM Counter Publisher

## Summary

Implement a C++ SHM counter publisher that publishes incrementing int64 values
via shared memory on `demo/counter`. This is the core (and only) feature of the
zenoh-counter-cpp project.

## Scope

### In Scope

1. **CMake build system** -- Integrate zenoh-c (imported shared lib) and
   zenoh-cpp (header-only) from `ext/` submodules. Update root CMakeLists.txt,
   src/CMakeLists.txt, app/CMakeLists.txt, tests/CMakeLists.txt. Define
   `ZENOHCXX_ZENOHC`. Set RPATH for runtime discovery. Require SHM + unstable
   API flags.

2. **ShmCounterPublisher class** (header/source separation)
   - `include/counter/publisher.hpp` -- class declaration
   - `src/publisher.cpp` -- implementation
   - Owns: Session, Publisher, PosixShmProvider
   - Method: `Publish()` -- alloc 8 bytes SHM, write int64 LE, put(move)
   - Method: `Counter()` -- return current counter value
   - Constructor takes: key expression, connect endpoints, listen endpoints
   - RAII: destructor closes publisher/session/provider

3. **CLI executable** (`app/main.cpp`)
   - Parse flags: `-k/--key`, `-e/--connect`, `-l/--listen`, `-i/--interval`, `-h/--help`
   - Signal handling: SIGINT/SIGTERM -> graceful shutdown
   - Publish loop: call `Publish()`, sleep for interval, repeat while running
   - Print counter value on each publish

4. **GoogleTest tests** (`tests/test_publisher.cpp`)
   - Unit: int64 encoding (little-endian, 8 bytes)
   - Integration: SHM alloc + publish via two sessions in same process
   - Integration: verify subscriber receives correct int64 value

### Out of Scope

- State machine (play/pause/stop/reset) -- this is MVP, just increment
- Command subscriber / state publisher -- no bidirectional control
- Non-SHM publish path -- SHM always-on (degrades transparently cross-machine)
- Subscriber -- the Dart project provides that
- Android cross-compilation -- desktop Linux only for now

## Counter Protocol

Must match `zenoh-counter-dart/lib/counter_codec.dart`:

```dart
// Dart encode
ByteData(8)..setInt64(0, value, Endian.little) -> Uint8List

// Dart decode
ByteData.sublistView(bytes).getInt64(0, Endian.little) -> int
```

C++ equivalent:
```cpp
int64_t counter = 42;
std::memcpy(buf.data(), &counter, sizeof(int64_t));
// Platform is little-endian (x86_64) -- no byte swap needed
```

## zenoh-cpp SHM API

From `ext/zenoh-cpp/examples/zenohc/z_pub_shm.cxx`:

```cpp
PosixShmProvider provider(MemoryLayout(65536, AllocAlignment({2})));
auto alloc_result = provider.alloc_gc_defrag_blocking(8, AllocAlignment({0}));
ZShmMut &&buf = std::get<ZShmMut>(std::move(alloc_result));
std::memcpy(buf.data(), &counter, sizeof(int64_t));
pub.put(std::move(buf));
```

## Acceptance Criteria

- [ ] `cmake --preset default && cmake --build build` succeeds
- [ ] `ctest --test-dir build --output-on-failure` passes all tests
- [ ] `./build/counter_pub` runs, publishes incrementing int64 via SHM
- [ ] `./build/counter_pub -k demo/test -i 500 -l tcp/0.0.0.0:7448` works
- [ ] Dart subscriber receives correct int64 values (manual cross-language test)
- [ ] `clang-format --dry-run -Werror` passes
- [ ] No clang-tidy warnings

## Dependencies

- ext/zenoh-c v1.7.2 (git submodule, must be built with SHM)
- ext/zenoh-cpp v1.7.2 (git submodule, header-only)
- GoogleTest v1.15.2 (via FetchContent, already in template)

## References

- zenoh-cpp SHM example: `ext/zenoh-cpp/examples/zenohc/z_pub_shm.cxx`
- zenoh-c SHM example: `ext/zenoh-c/examples/z_pub_shm.c`
- Dart counter publisher: `zenoh-counter-dart/lib/counter_pub.dart`
- Dart counter codec: `zenoh-counter-dart/lib/counter_codec.dart`
- xplr C++ app (reference, not direct port): `dart_zenoh_xplr/apps/cpp_app/`
