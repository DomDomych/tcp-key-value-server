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
* GitHub Actions CI
* Docker and Docker Compose support

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

Network operations are asynchronous. PostgreSQL operations are currently synchronous, and access to the shared database connection is synchronized between worker threads.

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

## Docker

Docker is the recommended way to run the complete project.

Requirements:

* Docker
* Docker Compose

Create the environment file:

```bash
cp .env.example .env
```

Build and start the server with PostgreSQL:

```bash
docker compose up -d --build
```

Start an interactive client:

```bash
docker compose --profile tools run --rm client
```

Multiple clients can be started simultaneously by running the same command in different terminals. Each invocation creates a separate TCP connection.

The server is also available from the host at:

```text
127.0.0.1:8080
```

Check the running containers:

```bash
docker compose ps
```

View server logs:

```bash
docker compose logs -f server
```

Stop the project:

```bash
docker compose down
```

PostgreSQL data is stored in a Docker volume and remains available after the containers are stopped.

The `.env` file contains local configuration and is not committed. Use `.env.example` as a template.

## Native Build

Requirements:

* Linux
* C++20 compiler
* Boost
* CMake
* Git
* PostgreSQL
* PostgreSQL development files (`libpq-dev`)
* GoogleTest
* Python 3

Using CMake:

```bash
cmake -S . -B build
cmake --build build --parallel
```

A running PostgreSQL instance and the `kv_store` table are required for database-backed server operations and storage tests.

## Native Run

Start the server:

```bash
./scripts/run_server.sh
```

Start the client in another terminal:

```bash
./scripts/run_client.sh
```

By default, the server listens on port 8080. The local client connects to

```text
127.0.0.1:8080
```

Type `exit` to close the client.

## Tests

Configure and build the project with testing enabled:

```bash
cmake \
    -S . \
    -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTING=ON

cmake --build build --parallel
```

Run the tests:

```bash
./scripts/run_tests.sh
```

The test suite requires a running PostgreSQL instance with the `kv_store` table.

GitHub Actions automatically builds the project and runs the tests against PostgreSQL 16 on every push and pull request.

## Benchmark

The project includes a Python benchmark that creates multiple concurrent clients and generates a configurable mix of `GET`, `SET` and `DEL` requests.

Example:

```bash
./benchmarks/run_full.sh \
    100 \
    --get 80 \
    --set 15 \
    --del 5
```

This starts 100 clients with the following request distribution:

```text
GET 80%
SET 15%
DEL 5%
```

The percentages must add up to `100`.

The number of commands per client, keyspace size, random seed, warmup and cleanup settings are configured in:

```text
benchmarks/scenario_for_full.txt
```

The benchmark populates the keyspace, performs a warmup stage, executes the configured workload and optionally cleans up benchmark keys.

Reported metrics include:

```text
Successful commands
Execution time
Throughput
GET / SET / DEL counts
Average latency
p50 latency
p95 latency
p99 latency
Maximum latency
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

PostgreSQL operations are synchronous, and access to the shared database connection is synchronized between worker threads.