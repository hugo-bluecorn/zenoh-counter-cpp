#!/usr/bin/env bash

HELPERS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${HELPERS_DIR}/helpers.sh"

function test_cleanup_kills_all_tracked_pids() {
    # Given: Three background processes
    PIDS=()
    sleep 60 &
    PIDS+=($!)
    sleep 60 &
    PIDS+=($!)
    sleep 60 &
    PIDS+=($!)

    # When: cleanup is called
    cleanup

    # Then: all processes are dead
    sleep 0.2  # Give OS time to reap
    local alive=0
    for pid in "${PIDS[@]}"; do
        if kill -0 "$pid" 2>/dev/null; then
            ((alive++))
        fi
    done
    assert_equals 0 "$alive"
}

function test_cleanup_handles_already_dead_processes() {
    # Given: A PID that already exited
    PIDS=()
    sleep 0 &
    PIDS+=($!)
    wait "${PIDS[0]}" 2>/dev/null  # Wait for it to die

    # When: cleanup is called (should not error)
    cleanup &>/dev/null
    local exit_code=$?

    # Then: no error
    assert_equals 0 "$exit_code"
}

function test_cleanup_with_no_tracked_pids() {
    # Given: empty PID array
    PIDS=()

    # When: cleanup is called
    cleanup &>/dev/null
    local exit_code=$?

    # Then: returns 0, no errors
    assert_equals 0 "$exit_code"
}
