# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.2.0] - 2026-03-09

### Added
- ShmCounterPublisher class with RAII session, publisher, and SHM provider management
- Publish() method: 8-byte SHM alloc, int64 LE encoding via memcpy, zero-copy put
- Counter() accessor for current publish count
- CMake integration for zenoh-c (IMPORTED shared lib) and zenoh-cpp (header-only)
- RPATH configuration so executables find libzenohc.so without LD_LIBRARY_PATH
- CLI executable (counter_pub) with -k/--key, -e/--connect, -l/--listen, -i/--interval, -h/--help
- Signal handling (SIGINT/SIGTERM) for graceful shutdown
- Unit tests: construction with default/custom key, listen/connect endpoints
- Integration tests: loopback pub/sub, two-session TCP delivery, byte encoding verification

### Changed
- Renamed project from claude_cpp_template to zenoh_counter_cpp
- Replaced template mylib with counter library target

## [0.1.0] - 2026-02-01

### Added
- Initial C++ CMake template with TDD support
- Clang toolchain with sanitizer presets (tdd-tsan, tdd-asan)
- Code coverage support using llvm-cov
- VSCode C++ TestMate debug configuration
- Context directory structure (standards/libraries/project)
- Comprehensive coding standards and best practices documentation
