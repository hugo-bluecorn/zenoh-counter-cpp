#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
source "${SCRIPT_DIR}/helpers.sh"

# Configuration
PORT=7501
INTERVAL_MS=400
REQUIRED_VALUES=8
TIMEOUT_SECONDS=10
KEY_EXPR="demo/counter"

PUB_BIN="${PROJECT_ROOT}/build/app/counter_pub"
ROUTER_BIN="${PROJECT_ROOT}/build/app/counter_router"
DART_REPO="${PROJECT_ROOT}/../zenoh-counter-dart"

# Trap for cleanup
trap cleanup EXIT INT TERM

echo "=== Router Topology Interop Test ==="
echo "Topology: C++ router -l -> C++ pub -e -> Dart sub -e"
echo "Port: ${PORT}, Interval: ${INTERVAL_MS}ms, Required: ${REQUIRED_VALUES} values"
echo ""

# Check prerequisites (publisher, dart repo, and router binary)
check_prereqs "$PUB_BIN" "$DART_REPO" "$ROUTER_BIN"

# Start C++ router (listening)
"$ROUTER_BIN" -l "tcp/0.0.0.0:${PORT}" &
PIDS+=($!)
echo "Started counter_router (PID ${PIDS[-1]})"

# Wait for router to bind
sleep 1

# Start C++ publisher (connecting to router)
"$PUB_BIN" -k "$KEY_EXPR" -e "tcp/127.0.0.1:${PORT}" -i "$INTERVAL_MS" &
PIDS+=($!)
echo "Started publisher (PID ${PIDS[-1]})"

# Wait for publisher to connect
sleep 1

# Start Dart subscriber (connecting to router) with output capture
DART_OUTPUT=$(mktemp)
(cd "$DART_REPO" && fvm dart run bin/counter_sub.dart -e "tcp/127.0.0.1:${PORT}") > "$DART_OUTPUT" 2>&1 &
PIDS+=($!)
echo "Started Dart subscriber (PID ${PIDS[-1]})"

# Wait for values with timeout
echo ""
echo "Waiting up to ${TIMEOUT_SECONDS}s for ${REQUIRED_VALUES} values..."
ELAPSED=0
while [[ $ELAPSED -lt $TIMEOUT_SECONDS ]]; do
    sleep 1
    ELAPSED=$((ELAPSED + 1))
    # Count received values so far
    COUNT_SO_FAR="$(grep -c 'Received:' "$DART_OUTPUT" 2>/dev/null || true)"
    COUNT_SO_FAR="${COUNT_SO_FAR:-0}"
    echo "  ${ELAPSED}s: ${COUNT_SO_FAR} values received"
    if [[ "$COUNT_SO_FAR" -ge "$REQUIRED_VALUES" ]]; then
        break
    fi
done

echo ""

# Parse output
VALUES_STR=$(parse_dart_output "$DART_OUTPUT")
rm -f "$DART_OUTPUT"

if [[ -z "$VALUES_STR" ]]; then
    echo "FAIL: No values received from Dart subscriber"
    exit 1
fi

# shellcheck disable=SC2086
read -ra VALUES <<< "$VALUES_STR"
COUNT=${#VALUES[@]}

echo "Received ${COUNT} values: ${VALUES_STR}"

# Check results
if [[ $COUNT -lt $REQUIRED_VALUES ]]; then
    echo ""
    echo "FAIL: Expected at least ${REQUIRED_VALUES} values, got ${COUNT}"
    exit 1
fi

# Check incrementing
# shellcheck disable=SC2086
if ! check_incrementing ${VALUES_STR}; then
    echo ""
    echo "FAIL: Values are not monotonically increasing"
    exit 1
fi

echo ""
echo "PASS: Received ${COUNT} incrementing values via router topology"
exit 0
