#!/usr/bin/env bash

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

SERVER="$PROJECT_ROOT/build/kv_server"
LOAD_TEST="$SCRIPT_DIR/load_by_file.py"

SERVER_PID=""

cleanup() {
    if [[ -n "$SERVER_PID" ]] && kill -0 "$SERVER_PID" 2>/dev/null; then
        echo "Stopping server..."
        kill "$SERVER_PID"
        wait "$SERVER_PID" 2>/dev/null
    fi
}

trap cleanup EXIT INT TERM


if [[ ! -x "$SERVER" ]]; then
    echo "Error: kv_server not found or not executable:"
    echo "$SERVER"
    exit 1
fi

echo "Starting server..."

"$SERVER" &
SERVER_PID=$!

sleep 0.5

if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    echo "Error: server failed to start"
    exit 1
fi

echo "Server started with PID $SERVER_PID"
echo

python3 "$LOAD_TEST"
TEST_RESULT=$?

echo

if [[ $TEST_RESULT -eq 0 ]]; then
    echo "Load test finished successfully"
else
    echo "Load test failed with code $TEST_RESULT"
fi

exit "$TEST_RESULT"