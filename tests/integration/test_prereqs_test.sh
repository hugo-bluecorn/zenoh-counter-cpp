#!/usr/bin/env bash

# Source the helpers relative to this file
HELPERS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${HELPERS_DIR}/helpers.sh"

# Override paths for testing
PROJECT_ROOT="$(cd "${HELPERS_DIR}/../.." && pwd)"

function test_all_prerequisites_present_returns_success() {
    # Given: real binaries exist, Dart repo exists, fvm available
    local output
    output=$(check_prereqs \
        "${PROJECT_ROOT}/build/app/counter_pub" \
        "${PROJECT_ROOT}/../zenoh-counter-dart" 2>&1)
    assert_equals 0 $?
    assert_contains "Prerequisites: OK" "$output"
}

function test_missing_binary_prints_error_and_fails() {
    # Given: binary does not exist
    local output
    output=$(check_prereqs \
        "/nonexistent/counter_pub" \
        "${PROJECT_ROOT}/../zenoh-counter-dart" 2>&1)
    assert_not_equals 0 $?
    assert_contains "counter_pub" "$output"
}

function test_missing_dart_repo_prints_error_and_fails() {
    local output
    output=$(check_prereqs \
        "${PROJECT_ROOT}/build/app/counter_pub" \
        "/nonexistent/zenoh-counter-dart" 2>&1)
    assert_not_equals 0 $?
    assert_contains "zenoh-counter-dart" "$output"
}

function test_missing_fvm_prints_error_and_fails() {
    # Override PATH to hide fvm
    local output
    output=$(PATH="/usr/bin:/bin" check_prereqs \
        "${PROJECT_ROOT}/build/app/counter_pub" \
        "${PROJECT_ROOT}/../zenoh-counter-dart" 2>&1)
    assert_not_equals 0 $?
    assert_contains "fvm" "$output"
}

function test_router_prereq_validates_counter_router_binary() {
    # check_prereqs with router binary that doesn't exist
    local output
    output=$(check_prereqs \
        "${PROJECT_ROOT}/build/app/counter_pub" \
        "${PROJECT_ROOT}/../zenoh-counter-dart" \
        "/nonexistent/counter_router" 2>&1)
    assert_not_equals 0 $?
    assert_contains "counter_router" "$output"
}
