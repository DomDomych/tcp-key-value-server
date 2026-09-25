#!/usr/bin/env bash

set -e

echo "=== C++ unit tests ==="
ctest --test-dir build --output-on-failure

echo
echo "=== Starting server ==="
./build/kv_server &
SERVER_PID=$!

cleanup() {
    echo
    echo "=== Stopping server ==="
    kill "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null || true
}

trap cleanup EXIT

echo "Waiting for server..."

for i in {1..20}; do
    if (echo > /dev/tcp/127.0.0.1/8080) 2>/dev/null; then
        echo "Server is ready."
        break
    fi

    if ! kill -0 "$SERVER_PID" 2>/dev/null; then
        echo "Server exited unexpectedly."
        exit 1
    fi

    sleep 0.25
done

if ! (echo > /dev/tcp/127.0.0.1/8080) 2>/dev/null; then
    echo "Server did not become ready."
    exit 1
fi

echo
echo "=== Go integration tests ==="
go test ./tests/integration_tests/... -v

echo
echo "=== All tests passed ===" 