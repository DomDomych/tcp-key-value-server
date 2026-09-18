# TCP Key-Value Server

An asynchronous TCP key-value server written in C++20 using Boost.Asio.

The server uses PostgreSQL for persistent key-value storage and an in-memory LRU cache to reduce database access for frequently requested values.

## Features

* TCP networking with Boost.Asio
* Asynchronous accept, read and write operations
* Multiple worker threads running a shared `io_context`
* Persistent storage with PostgreSQL
* PostgreSQL integration using `libpqxx`
* In-memory LRU cache
* Thread-safe shared storage access
* `SET`, `GET` and `DEL` commands
* Interactive TCP client
* GoogleTest-based tests
* Python load testing
* CMake build system

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

The server uses asynchronous Boost.Asio networking.

Incoming connections are accepted with `async_accept()`.
Each connected client is represented by a `Session`, which performs asynchronous reads and writes.

Several worker threads run the same `boost::asio::io_context` and execute ready completion handlers.

```text
Client 1 ── Session 1 ──┐
Client 2 ── Session 2 ──┼── io_context ── Worker threads
Client 3 ── Session 3 ──┘
                            |
                            v
                         Storage
                         /     \
                        /       \
                 LRU cache    PostgreSQL
                 (memory)     (persistent)
```

Threads are not assigned permanently to individual clients.

While a client is waiting for network I/O, no worker thread is blocked waiting for that connection. When an asynchronous operation completes, one of the threads running `io_context.run()` executes its completion handler.

All sessions share the same `Storage` instance.

The storage layer combines an in-memory LRU cache with PostgreSQL. Frequently accessed values can be returned directly from the cache, while cache misses fall back to PostgreSQL.

The cache has a fixed capacity and evicts the least recently used entries when the capacity is reached.

`SET` and `DEL` operations keep the persistent storage and cache state consistent.

Network operations are asynchronous. PostgreSQL operations are currently synchronous and access to the shared database connection is synchronized between worker threads.

## Storage Flow

For a `GET` request:

```text
GET key
   |
   v
LRU cache
   |
   +── HIT ──────────────> return value
   |
   └── MISS
        |
        v
    PostgreSQL
        |
        v
    update cache
        |
        v
    return value
```

PostgreSQL acts as the persistent source of data, while the LRU cache reduces repeated database queries for frequently accessed keys.

## Build

Requirements:

* Linux
* C++20 compiler
* Boost
* CMake
* PostgreSQL
* libpqxx
* GoogleTest
* Python 3

Using CMake:

```bash
cmake -S . -B build
cmake --build build
```

A running PostgreSQL instance and the key-value table are required for database-backed server operations and storage tests.

## Run

Start the server:

```bash
./scripts/run_server.sh
```

Start the client in another terminal:

```bash
./scripts/run_client.sh
```

The server listens on:

```text
127.0.0.1:8080
```

Type `exit` to close the client.

## Load Test

The project includes a Python load test for running multiple TCP clients concurrently.

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

The benchmark reports:

```text
Clients:     ...
Commands:    ...
Time:        ... seconds
Throughput:  ... commands/sec
Errors:      ...
```

The same workload can be reused to measure the effect of architectural changes and compare different server implementations.

## Current Status

Current architecture:

```text
asynchronous Boost.Asio networking
        +
shared io_context
        +
multiple worker threads
        +
shared Storage layer
        +
LRU in-memory cache
        +
persistent PostgreSQL storage
        +
thread-safe access
```

The networking layer is asynchronous and can handle multiple client connections concurrently without dedicating one thread to each connection.

The storage layer provides persistent PostgreSQL-backed data while using an LRU cache to accelerate repeated reads.
