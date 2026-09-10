# TCP Key-Value Server

A multithreaded TCP key-value server written in C++20 using Boost.Asio.

The server stores key-value pairs in memory and supports multiple concurrent clients.

## Features

- TCP networking with Boost.Asio
- Thread-per-client concurrency
- Shared in-memory storage
- Thread-safe access using `std::mutex`
- `SET`, `GET` and `DEL` commands
- Interactive TCP client
- Python load testing
- CMake build system

## Protocol

The server uses a simple line-based protocol.

```text
SET key value
GET key
DEL key
```

Example:

```text
SET language cpp
GET language
DEL language
```

## Architecture

The server uses synchronous Boost.Asio I/O.

The main thread accepts incoming connections and creates a separate
`std::thread` for every connected client.

```text
Client 1 ── Thread 1 ──┐
Client 2 ── Thread 2 ──┼── Shared storage
Client 3 ── Thread 3 ──┘
```

All sessions share the same `std::unordered_map`.

Access to the storage is protected by `std::mutex`.

The current thread-per-client implementation is used as a baseline for
future comparison with asynchronous Boost.Asio models.

## Build

Requirements:

- C++20 compiler
- Boost
- CMake
- Linux

Using CMake:

```bash
cmake -S . -B build
cmake --build build
```

## Run

Start the server:

```bash
./kv_server
```

Start the client in another terminal:

```bash
./kv_client
```

The server listens on:

```text
127.0.0.1:8080
```

Type `exit` to close the client.

## Load Test

The project includes a Python load test for running multiple TCP clients
concurrently.

Clients and their commands are described in `scenario.txt`:

```text
ID: 1
SET key1 value1
GET key1
DEL key1

ID: 2
SET key2 value2
GET key2
DEL key2
```

Each `ID` represents a separate client connection.

Run the benchmark:

```bash
./tests/load/run_load_test.sh
```

Example result:

```text
Clients:     100
Commands:    10000
Time:        0.552169 seconds
Throughput:  18110.40 commands/sec
Errors:      0
```

The same workload can be reused to compare different server implementations.

## Current Status

Current architecture:

```text
synchronous Boost.Asio
        +
thread per client
        +
shared in-memory storage
        +
std::mutex
```

Future versions may explore asynchronous I/O, worker pools and persistent
storage.