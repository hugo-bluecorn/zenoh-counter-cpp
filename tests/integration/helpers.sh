#!/usr/bin/env bash
# Shared helpers for integration test scripts

# check_prereqs <pub_binary> <dart_repo> [<extra_binary>...]
# Validates that all required binaries, the Dart repo, and fvm exist.
# Prints errors to stderr and returns 1 on failure, or prints
# "Prerequisites: OK" to stdout and returns 0 on success.
check_prereqs() {
    local pub_binary="$1"
    local dart_repo="$2"
    shift 2
    local extra_binaries=("$@")
    local has_error=0

    # Check publisher binary
    if [[ ! -x "$pub_binary" ]]; then
        echo "ERROR: Binary not found: $pub_binary" >&2
        has_error=1
    fi

    # Check extra binaries (e.g., counter_router)
    for bin in "${extra_binaries[@]}"; do
        if [[ ! -x "$bin" ]]; then
            echo "ERROR: Binary not found: $bin" >&2
            has_error=1
        fi
    done

    # Check Dart repo
    if [[ ! -d "$dart_repo" ]]; then
        echo "ERROR: Dart repo not found: $dart_repo" >&2
        has_error=1
    fi

    # Check fvm
    if ! command -v fvm &>/dev/null; then
        echo "ERROR: fvm not found in PATH" >&2
        has_error=1
    fi

    if [[ $has_error -eq 1 ]]; then
        return 1
    fi

    echo "Prerequisites: OK"
    return 0
}

# Process tracking and cleanup
PIDS=()

cleanup() {
    for pid in "${PIDS[@]}"; do
        kill "$pid" 2>/dev/null || true
    done
    for pid in "${PIDS[@]}"; do
        wait "$pid" 2>/dev/null || true
    done
    return 0
}

# parse_dart_output <output_file>
# Extracts integer values from "Received: N" lines
# Prints space-separated values to stdout
parse_dart_output() {
    return 1  # Not implemented
}

# check_incrementing <value1> <value2> ...
# Validates monotonically increasing sequence
# Returns 0 if valid, 1 otherwise (with error to stderr)
check_incrementing() {
    return 1  # Not implemented
}
