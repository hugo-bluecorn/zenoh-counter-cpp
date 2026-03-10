cmake_minimum_required(VERSION 3.28)

# Use new timestamp behavior for FetchContent downloads
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

project(zenoh_counter_cpp VERSION 0.4.0 LANGUAGES CXX)

# C++20 standard (GoogleTest requires C++17 minimum)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clang-tidy and IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_TESTING "Build tests" ON)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

# Include custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Coverage configuration
if(ENABLE_COVERAGE)
    include(Coverage)
endif()

# ---------------------------------------------------------------------------
# zenoh-c (shared library, must be pre-built)
# ---------------------------------------------------------------------------
set(ZENOHC_LIB "${CMAKE_SOURCE_DIR}/ext/zenoh-c/target/release/libzenohc.so")
if(NOT EXISTS "${ZENOHC_LIB}")
    message(FATAL_ERROR
        "zenoh-c shared library not found at:\n"
        "  ${ZENOHC_LIB}\n"
        "Build zenoh-c first. See CLAUDE.md for instructions."
    )
endif()

add_library(zenohc SHARED IMPORTED GLOBAL)
set_target_properties(zenohc PROPERTIES
    IMPORTED_LOCATION "${ZENOHC_LIB}"
    IMPORTED_SONAME "libzenohc.so"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/ext/zenoh-c/include"
)

# ---------------------------------------------------------------------------
# zenoh-cpp (header-only, wraps zenoh-c)
# ---------------------------------------------------------------------------
add_library(zenohcpp INTERFACE)
target_include_directories(zenohcpp
    INTERFACE
        "${CMAKE_SOURCE_DIR}/ext/zenoh-cpp/include"
)
target_compile_definitions(zenohcpp
    INTERFACE
        ZENOHCXX_ZENOHC
)
target_link_libraries(zenohcpp INTERFACE zenohc)

# ---------------------------------------------------------------------------
# RPATH so executables find libzenohc.so at runtime
# ---------------------------------------------------------------------------
set(CMAKE_BUILD_RPATH "${CMAKE_SOURCE_DIR}/ext/zenoh-c/target/release")
set(CMAKE_INSTALL_RPATH "${CMAKE_SOURCE_DIR}/ext/zenoh-c/target/release")

# Library
add_subdirectory(src)

# Executable
add_subdirectory(app)

# Testing
if(BUILD_TESTING)
    enable_testing()
    include(FetchContent)
    # GoogleTest v1.15.2 (2024-07-31)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/b514bdc898e2951020cbdca1304b75f5950d1f59.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_subdirectory(tests)
endif()
