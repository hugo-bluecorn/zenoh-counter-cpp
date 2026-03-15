# zenoh-counter-cpp

C++ SHM counter publisher using [zenoh-cpp](https://github.com/eclipse-zenoh/zenoh-cpp), proving cross-language interop with the Dart subscriber from [zenoh-counter-dart](https://github.com/hugo-bluecorn/zenoh-counter-dart).

## What This Is

Three CLI programs built on zenoh-cpp v1.7.2:

- **counter_pub** -- Allocates shared memory, writes an incrementing int64 counter, publishes via zero-copy
- **counter_sub** -- Subscribes to the counter topic, decodes and prints received values
- **counter_router** -- Runs a zenoh router for hub-and-spoke topology

```
counter_pub (C++)                    counter_sub (Dart or C++)
  |                                    |
  PosixShmProvider(65536)             subscriber.stream / callback
  |                                    |
  alloc_gc_defrag_blocking(8)         decode LE int64 from payload
  |                                    |
  memcpy(&counter, 8)                print "Received: 42"
  |
  pub.put(std::move(buf))  // zero-copy SHM
```

This is the second of three template repos:

| Repo | Purpose | Status |
|------|---------|--------|
| [zenoh-counter-dart](https://github.com/hugo-bluecorn/zenoh-counter-dart) | Pure Dart CLI, validates package:zenoh + SHM | v0.1.1 |
| **zenoh-counter-cpp** (this) | C++ SHM publisher, validates cross-language interop | v0.4.0 |
| [zenoh-counter-flutter](https://github.com/hugo-bluecorn/zenoh-counter-flutter) | Flutter app, validates desktop + Android UI | v0.7.0 |

## Quick Start

### Prerequisites

- CMake 3.28+, Clang 15+, Ninja
- Rust (stable, MSRV 1.75.0) -- for building zenoh-c
- [FVM](https://fvm.app/) -- only needed for cross-language interop tests

### Build zenoh-c

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

### Build counter programs

```bash
cmake --preset default
cmake --build build
```

### Run (Peer Direct -- No Router)

**Terminal 1 -- C++ Publisher:**
```bash
./build/app/counter_pub -l tcp/0.0.0.0:7447
```

**Terminal 2 -- C++ Subscriber:**
```bash
./build/app/counter_sub -e tcp/127.0.0.1:7447
```

### Cross-Language (C++ -> Dart)

**Terminal 1 -- C++ Publisher:**
```bash
./build/app/counter_pub -l tcp/0.0.0.0:7448
```

**Terminal 2 -- Dart Subscriber** (from [zenoh-counter-dart](https://github.com/hugo-bluecorn/zenoh-counter-dart)):
```bash
cd /path/to/zenoh-counter-dart
fvm dart run bin/counter_sub.dart -e tcp/127.0.0.1:7448
```

No `LD_LIBRARY_PATH` needed -- Dart build hooks resolve native libraries automatically.

## Topologies

| Topology | Publisher | Subscriber | Router |
|----------|-----------|------------|:------:|
| Peer direct | `-l tcp/0.0.0.0:7447` | `-e tcp/host:7447` | No |
| Via router | `-e tcp/router:7447` | `-e tcp/router:7447` | Yes |

**Via router:**
```bash
./build/app/counter_router -l tcp/0.0.0.0:7447   # Terminal 1
./build/app/counter_pub -e tcp/127.0.0.1:7447     # Terminal 2
./build/app/counter_sub -e tcp/127.0.0.1:7447     # Terminal 3
```

## CLI Flags

All programs mirror [zenoh-c](https://github.com/eclipse-zenoh/zenoh-c) CLI conventions.

**counter_pub:**

| Flag | Description | Default |
|------|-------------|---------|
| `-k, --key` | Key expression | `demo/counter` |
| `-e, --connect` | Connect endpoint (repeatable) | -- |
| `-l, --listen` | Listen endpoint (repeatable) | -- |
| `-i, --interval` | Publish interval (ms) | `1000` |

**counter_sub:**

| Flag | Description | Default |
|------|-------------|---------|
| `-k, --key` | Key expression | `demo/counter` |
| `-e, --connect` | Connect endpoint (repeatable) | -- |
| `-l, --listen` | Listen endpoint (repeatable) | -- |
| `-n, --count` | Exit after N messages | unlimited |

**counter_router:**

| Flag | Description | Default |
|------|-------------|---------|
| `-l, --listen` | Listen endpoint | `tcp/0.0.0.0:7447` |

## Counter Payload

- Format: raw little-endian int64 (8 bytes)
- Key expression: `demo/counter` (default)
- Binary-compatible with zenoh-counter-dart publisher

## Tests

20 C++ tests (GoogleTest) + 2 cross-language interop scripts:

```bash
# C++ unit and integration tests
ctest --test-dir build --output-on-failure

# Cross-language interop (requires zenoh-counter-dart sibling repo)
ctest --test-dir build -L interop
```

## Project Structure

```
app/
  main.cpp                  # Publisher CLI (arg parsing, signal handling, loop)
  counter_sub.cpp           # Subscriber CLI
  counter_router.cpp        # Router CLI
include/counter/
  publisher.hpp             # ShmCounterPublisher class
src/
  publisher.cpp             # ShmCounterPublisher implementation
tests/
  test_publisher.cpp        # SHM alloc, int64 encoding, publish tests
  test_subscriber.cpp       # Subscriber callback, value collection tests
  test_integration.cpp      # Loopback and topology tests
  integration/              # Cross-language bash scripts
ext/
  zenoh-c/                  # git submodule (v1.7.2)
  zenoh-cpp/                # git submodule (v1.7.2, header-only)
```

## License

Apache 2.0 -- see [LICENSE](LICENSE).
