#!/usr/bin/env bash

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

SERVER="$PROJECT_ROOT/build/kv_server"
BENCHMARK="$SCRIPT_DIR/full_benchmark.py"
SCENARIO="$SCRIPT_DIR/scenario_for_full.txt"

SERVER_PID=""

usage() {
    echo "Usage: $0 <clients> --get <percent> --set <percent> --del <percent>"
    echo
    echo "Example:"
    echo "  $0 100 --get 80 --set 15 --del 5"
}

cleanup() {
    if [[ -n "$SERVER_PID" ]] && kill -0 "$SERVER_PID" 2>/dev/null; then
        echo
        echo "Stopping server..."
        kill "$SERVER_PID"
        wait "$SERVER_PID" 2>/dev/null || true
    fi
}

trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

if [[ $# -lt 1 ]]; then
    usage
    exit 2
fi

CLIENTS="$1"
shift

if [[ ! "$CLIENTS" =~ ^[1-9][0-9]*$ ]]; then
    echo "Error: clients must be a positive integer"
    echo
    usage
    exit 2
fi

if [[ ! -x "$SERVER" ]]; then
    echo "Error: kv_server not found or not executable:"
    echo "  $SERVER"
    echo "Build the project before running the benchmark."
    exit 1
fi

if [[ ! -f "$BENCHMARK" ]]; then
    echo "Error: benchmark file not found:"
    echo "  $BENCHMARK"
    exit 1
fi

if [[ ! -f "$SCENARIO" ]]; then
    echo "Error: scenario file not found:"
    echo "  $SCENARIO"
    exit 1
fi

if ! command -v python3 >/dev/null 2>&1; then
    echo "Error: python3 is not installed or not available in PATH"
    exit 1
fi

echo "Starting server..."

"$SERVER" &
SERVER_PID=$!

sleep 0.5

if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    echo "Error: server failed to start"
    wait "$SERVER_PID" 2>/dev/null || true
    exit 1
fi

echo "Server started with PID $SERVER_PID"
echo

python3 "$BENCHMARK" "$SCENARIO" "$CLIENTS" "$@"
BENCHMARK_RESULT=$?

echo

if [[ $BENCHMARK_RESULT -eq 0 ]]; then
    echo "Benchmark finished successfully"
else
    echo "Benchmark failed with code $BENCHMARK_RESULT"
fi

exit "$BENCHMARK_RESULT"