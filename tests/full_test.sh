#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD_DIR="$PROJECT_ROOT/build"
SERVER="$BUILD_DIR/kv_server"
INTEGRATION_DIR="$PROJECT_ROOT/tests/integration_tests"
LOAD_TEST="$PROJECT_ROOT/tests/load/run_load_test.sh"

SERVER_PID=""

BUILD_EXISTED=false
if [[ -d "$BUILD_DIR" ]]; then
    BUILD_EXISTED=true
fi

stop_server() {
    if [[ -n "$SERVER_PID" ]] && kill -0 "$SERVER_PID" 2>/dev/null; then
        echo "Stopping server..."
        kill "$SERVER_PID"
        wait "$SERVER_PID" 2>/dev/null || true
    fi

    SERVER_PID=""
}

cleanup() {
    stop_server

    if [[ "$BUILD_EXISTED" == false ]] && [[ -d "$BUILD_DIR" ]]; then
        echo "Removing build directory..."
        rm -rf "$BUILD_DIR"
    fi
}

trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

cd "$PROJECT_ROOT"

echo "===== BUILD ====="
./scripts/build.sh

echo
echo "===== UNIT TESTS ====="
./scripts/run_tests.sh

echo
echo "===== INTEGRATION TESTS ====="

"$SERVER" &
SERVER_PID=$!

sleep 0.5

if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    echo "Error: server failed to start"
    exit 1
fi

for test in "$INTEGRATION_DIR"/*.py; do
    echo "Running $(basename "$test")..."
    python3 "$test"
done

stop_server

echo
echo "===== LOAD TEST ====="
"$LOAD_TEST"

echo
echo "===== ALL TESTS PASSED ====="