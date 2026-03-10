#!/usr/bin/env bash

HELPERS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${HELPERS_DIR}/helpers.sh"

PROJECT_ROOT="$(cd "${HELPERS_DIR}/../.." && pwd)"

function test_output_parser_extracts_counter_values() {
    local tmpfile
    tmpfile=$(mktemp)
    cat > "$tmpfile" << 'DART_OUTPUT'
Subscribing on "demo/counter"
Received: 1
Received: 2
Received: 3
DART_OUTPUT
    local result
    result=$(parse_dart_output "$tmpfile")
    rm -f "$tmpfile"
    assert_equals "1 2 3" "$result"
}

function test_incrementing_check_validates_monotonic_sequence() {
    check_incrementing 1 2 3 4 5 6 7 8
    assert_equals 0 $?
}

function test_incrementing_check_detects_non_monotonic() {
    local output
    output=$(check_incrementing 1 2 3 2 5 6 7 8 2>&1)
    assert_not_equals 0 $?
    assert_contains "not incrementing" "$output"
}

function test_script_uses_port_7500() {
    local port
    port=$(grep -oP 'PORT=\K[0-9]+' "${HELPERS_DIR}/test_peer_interop.sh" 2>/dev/null || echo "not found")
    assert_equals "7500" "$port"
}

function test_peer_interop_end_to_end() {
    # This test runs the actual interop script
    local output
    output=$(bash "${HELPERS_DIR}/test_peer_interop.sh" 2>&1)
    local exit_code=$?
    assert_equals 0 "$exit_code" "Script should exit 0 on success"
    assert_contains "PASS" "$output"
}
