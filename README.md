# TCP Key-Value Server

An asynchronous TCP key-value server written in C++20 using Boost.Asio.

The server stores key-value pairs through PostgreSQL,also using LRU-cache

## Features

* TCP networking with Boost.Asio
* Asynchronous accept, read and write operations
* Multiple worker threads running a shared `io_context`
* Shared in-memory storage
* Thread-safe access using `std::shared_mutex`
* `SET`, `GET` and `DEL` commands
* Interactive TCP client
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

The server uses asynchronous Boost.Asio I/O.

Incoming connections are accepted with `async_accept()`.
Each connected client is represented by a `Session`, which performs asynchronous reads and writes.

Several worker threads run the same `boost::asio::io_context` and execute ready completion handlers.

```text
Client 1 ── Session 1 ──┐
Client 2 ── Session 2 ──┼── io_context ── Worker threads
Client 3 ── Session 3 ──┘
                            |
                            └── Shared storage
```

Threads are not assigned permanently to individual clients.

While a client is waiting for network I/O, no worker thread is blocked waiting for that connection. When an asynchronous operation completes, one of the threads running `io_context.run()` executes its callback.

All sessions share the same in-memory storage.

Access to the storage is protected by `std::shared_mutex`, allowing multiple concurrent readers while writes require exclusive access.

## Build

Requirements:

* C++20 compiler
* Boost
* CMake
* Linux
* GoogleTest
* Python3

Using CMake:

```bash
cmake -S . -B build
cmake --build build
```

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
asynchronous Boost.Asio
        +
shared io_context
        +
multiple worker threads
        +
shared PostgreSQL
        +
LRU-cache
        +
std::shared_mutex
```
