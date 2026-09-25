#!/usr/bin/env bash

set -euo pipefail

HOST="127.0.0.1"
PORT=8080

CLIENTS=100
REQUESTS=1000

GET_PERCENT=80
SET_PERCENT=15
DEL_PERCENT=5

KEYSPACE=10000
SEED=42

echo "===== GO BENCHMARK ====="
echo "Server:          $HOST:$PORT"
echo "Clients:         $CLIENTS"
echo "Requests/client: $REQUESTS"
echo "Total requests:  $((CLIENTS * REQUESTS))"
echo "Mix:             GET $GET_PERCENT% / SET $SET_PERCENT% / DEL $DEL_PERCENT%"
echo "Keyspace:        $KEYSPACE"
echo "Seed:            $SEED"
echo

go run benchmarks/go_benchmark.go \
    -host "$HOST" \
    -port "$PORT" \
    -clients "$CLIENTS" \
    -requests "$REQUESTS" \
    -get "$GET_PERCENT" \
    -set "$SET_PERCENT" \
    -del "$DEL_PERCENT" \
    -keyspace "$KEYSPACE" \
    -seed "$SEED"