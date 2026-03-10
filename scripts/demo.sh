#!/usr/bin/env bash
#
# demo.sh -- Start counter_router + counter_pub + counter_sub together.
# Extra arguments pass through to counter_sub (e.g., ./scripts/demo.sh -n 5).
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

ROUTER="$PROJECT_DIR/build/app/counter_router"
PUB="$PROJECT_DIR/build/app/counter_pub"
SUB="$PROJECT_DIR/build/app/counter_sub"

PORT=7450
ROUTER_PID=""
PUB_PID=""

cleanup() {
    if [[ -n "$PUB_PID" ]]; then
        kill "$PUB_PID" 2>/dev/null || true
    fi
    if [[ -n "$ROUTER_PID" ]]; then
        kill "$ROUTER_PID" 2>/dev/null || true
    fi
    wait 2>/dev/null || true
}

trap cleanup EXIT INT TERM

# Check that all binaries exist
for bin in "$ROUTER" "$PUB" "$SUB"; do
    if [[ ! -x "$bin" ]]; then
        echo "Error: $bin not found. Run 'cmake --build build' first." >&2
        exit 1
    fi
done

# Start router
"$ROUTER" -l "tcp/0.0.0.0:$PORT" &
ROUTER_PID=$!
sleep 1

# Start publisher
"$PUB" -e "tcp/127.0.0.1:$PORT" -i 1000 &
PUB_PID=$!
sleep 1

# Run subscriber in foreground (extra args pass through)
"$SUB" -e "tcp/127.0.0.1:$PORT" "$@"
