#!/usr/bin/env bash

HELPERS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${HELPERS_DIR}/helpers.sh"

function test_router_interop_end_to_end() {
    local output
    output=$(bash "${HELPERS_DIR}/test_router_interop.sh" 2>&1)
    local exit_code=$?
    assert_equals 0 "$exit_code" "Script should exit 0"
    assert_contains "PASS" "$output"
}

function test_router_script_starts_three_processes() {
    # Verify script starts router first, then publisher, then subscriber
    local script="${HELPERS_DIR}/test_router_interop.sh"
    local router_line pub_line sub_line
    router_line=$(grep -n 'counter_router' "$script" | grep -v '#' | head -1 | cut -d: -f1)
    pub_line=$(grep -n 'counter_pub' "$script" | grep -v '#' | head -1 | cut -d: -f1)
    sub_line=$(grep -n 'counter_sub.dart' "$script" | grep -v '#' | head -1 | cut -d: -f1)
    assert_not_empty "$router_line"
    assert_not_empty "$pub_line"
    assert_not_empty "$sub_line"
    # Router line should be before publisher, publisher before subscriber
    assert_less_than "$pub_line" "$router_line"
    assert_less_than "$sub_line" "$pub_line"
}

function test_router_script_uses_port_7501() {
    local port
    port=$(grep -oP 'PORT=\K[0-9]+' "${HELPERS_DIR}/test_router_interop.sh" 2>/dev/null || echo "not found")
    assert_equals "7501" "$port"
}

function test_router_script_cleans_up_processes() {
    # Run the script and verify no orphan processes after
    bash "${HELPERS_DIR}/test_router_interop.sh" &>/dev/null || true
    sleep 1
    # Check no counter_router or counter_pub processes with our port
    local orphans
    orphans=$(pgrep -f "tcp.*7501" 2>/dev/null | wc -l)
    assert_equals "0" "$orphans"
}

function test_router_script_fails_if_counter_router_missing() {
    # Temporarily rename counter_router to test prereq check
    local router_bin
    router_bin="$(cd "${HELPERS_DIR}/../.." && pwd)/build/app/counter_router"
    if [[ -f "$router_bin" ]]; then
        mv "$router_bin" "${router_bin}.bak"
        local output
        output=$(bash "${HELPERS_DIR}/test_router_interop.sh" 2>&1)
        local exit_code=$?
        mv "${router_bin}.bak" "$router_bin"
        assert_not_equals 0 "$exit_code"
        assert_contains "counter_router" "$output"
    else
        skip "counter_router not found"
    fi
}
