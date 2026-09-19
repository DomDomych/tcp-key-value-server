#!/usr/bin/env python3

import argparse
import math
import random
import socket
import threading
import time
from collections import Counter
from pathlib import Path


BENCHMARK_PREFIX = "__benchmark__"
CONNECTION_TIMEOUT_SECONDS = 10
CLIENTS_READY_TIMEOUT_SECONDS = 30


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Run a concurrent benchmark against the KV server."
    )
    parser.add_argument(
        "scenario",
        type=Path,
        help="path to a scenario file containing benchmark settings",
    )
    parser.add_argument(
        "clients",
        type=int,
        help="number of concurrent clients",
    )
    parser.add_argument(
        "--get",
        dest="get_percent",
        type=int,
        required=True,
        help="percentage of GET requests",
    )
    parser.add_argument(
        "--set",
        dest="set_percent",
        type=int,
        required=True,
        help="percentage of SET requests",
    )
    parser.add_argument(
        "--del",
        dest="del_percent",
        type=int,
        required=True,
        help="percentage of DEL requests",
    )

    arguments = parser.parse_args()

    if arguments.clients <= 0:
        parser.error("clients must be greater than 0")

    return arguments


def parse_bool(value):
    normalized = value.strip().lower()

    if normalized in {"1", "true", "yes", "on"}:
        return True

    if normalized in {"0", "false", "no", "off"}:
        return False

    raise ValueError(f"cleanup must be a boolean, got: {value}")


def read_config(
    filename,
    clients,
    get_percent,
    set_percent,
    del_percent,
):
    raw_config = {}

    with filename.open("r", encoding="utf-8") as config_file:
        for line_number, line in enumerate(config_file, start=1):
            line = line.strip()

            if not line or line.startswith("#"):
                continue

            if "=" not in line:
                raise ValueError(
                    f"wrong config line {line_number}: {line}"
                )

            key, value = line.split("=", 1)
            key = key.strip().lower()
            value = value.strip()

            if not key or not value:
                raise ValueError(
                    f"wrong config line {line_number}: {line}"
                )

            raw_config[key] = value

    required_keys = {
        "host",
        "port",
        "commands_per_client",
        "keyspace",
        "seed",
        "warmup_commands_per_client",
        "cleanup",
    }

    missing_keys = required_keys - raw_config.keys()
    if missing_keys:
        missing = ", ".join(sorted(missing_keys))
        raise ValueError(f"missing config keys: {missing}")

    config = {
        "host": raw_config["host"],
        "port": int(raw_config["port"]),
        "clients": clients,
        "commands_per_client": int(raw_config["commands_per_client"]),
        "get": get_percent,
        "set": set_percent,
        "del": del_percent,
        "keyspace": int(raw_config["keyspace"]),
        "seed": int(raw_config["seed"]),
        "warmup_commands_per_client": int(
            raw_config["warmup_commands_per_client"]
        ),
        "cleanup": parse_bool(raw_config["cleanup"]),
    }

    if not 1 <= config["port"] <= 65535:
        raise ValueError("port must be between 1 and 65535")

    if config["commands_per_client"] <= 0:
        raise ValueError("commands_per_client must be greater than 0")

    if config["keyspace"] <= 0:
        raise ValueError("keyspace must be greater than 0")

    if config["warmup_commands_per_client"] < 0:
        raise ValueError(
            "warmup_commands_per_client must be greater than or equal to 0"
        )

    percentages = (config["get"], config["set"], config["del"])

    if any(percent < 0 or percent > 100 for percent in percentages):
        raise ValueError("GET, SET and DEL percentages must be from 0 to 100")

    if sum(percentages) != 100:
        raise ValueError("GET + SET + DEL percentages must equal 100")

    return config


def receive_response(sock, buffer):
    while True:
        newline_position = buffer.find(b"\n")

        if newline_position != -1:
            response = buffer[:newline_position]
            del buffer[: newline_position + 1]
            return response.decode("utf-8")

        chunk = sock.recv(4096)

        if not chunk:
            raise ConnectionError("server closed the connection")

        buffer.extend(chunk)


def send_command(sock, buffer, command):
    sock.sendall(f"{command}\n".encode("utf-8"))
    return receive_response(sock, buffer)


def percentile(sorted_values, percent):
    if not sorted_values:
        return 0.0

    index = math.ceil(percent / 100.0 * len(sorted_values)) - 1
    index = max(0, min(index, len(sorted_values) - 1))
    return sorted_values[index]


def key_for(run_id, index):
    return f"{BENCHMARK_PREFIX}:{run_id}:key:{index}"


def populate(config, run_id):
    print("Populating benchmark keyspace...")

    with socket.create_connection(
        (config["host"], config["port"]),
        timeout=CONNECTION_TIMEOUT_SECONDS,
    ) as sock:
        buffer = bytearray()

        for index in range(config["keyspace"]):
            key = key_for(run_id, index)
            response = send_command(sock, buffer, f"SET {key} value_{index}")

            if response != "OK":
                raise RuntimeError(
                    f"population failed for {key}: server returned {response!r}"
                )

    print(f"Populated {config['keyspace']} keys.")


def cleanup(config, run_id):
    print("Cleaning benchmark keys...")

    with socket.create_connection(
        (config["host"], config["port"]),
        timeout=CONNECTION_TIMEOUT_SECONDS,
    ) as sock:
        buffer = bytearray()

        for index in range(config["keyspace"]):
            key = key_for(run_id, index)
            send_command(sock, buffer, f"DEL {key}")

    print("Cleanup complete.")


def choose_operation(rng, config):
    random_percent = rng.randrange(100)

    if random_percent < config["get"]:
        return "GET"

    if random_percent < config["get"] + config["set"]:
        return "SET"

    return "DEL"


def build_command(rng, config, run_id, operation):
    key_index = rng.randrange(config["keyspace"])
    key = key_for(run_id, key_index)

    if operation == "GET":
        return f"GET {key}"

    if operation == "SET":
        value = f"value_{rng.randrange(1_000_000)}"
        return f"SET {key} {value}"

    return f"DEL {key}"


def save_client_results(
    client_id,
    local_latencies,
    local_operations,
    local_errors,
    results,
    results_lock,
):
    with results_lock:
        results["latencies"].extend(local_latencies)
        results["operations"].update(local_operations)

        if local_errors:
            results["errors"].append((client_id, local_errors))


def run_client(
    client_id,
    config,
    run_id,
    command_count,
    ready_barrier,
    start_event,
    collect_latency,
    results,
    results_lock,
):
    rng = random.Random(config["seed"] + client_id)
    local_latencies = []
    local_operations = Counter()
    local_errors = []

    try:
        with socket.create_connection(
            (config["host"], config["port"]),
            timeout=CONNECTION_TIMEOUT_SECONDS,
        ) as sock:
            buffer = bytearray()

            ready_barrier.wait(timeout=CLIENTS_READY_TIMEOUT_SECONDS)
            start_event.wait()

            for _ in range(command_count):
                operation = choose_operation(rng, config)
                command = build_command(rng, config, run_id, operation)
                started_at = time.perf_counter_ns()

                send_command(sock, buffer, command)

                finished_at = time.perf_counter_ns()
                local_operations[operation] += 1

                if collect_latency:
                    latency_ms = (finished_at - started_at) / 1_000_000.0
                    local_latencies.append(latency_ms)

    except threading.BrokenBarrierError:
        local_errors.append("not all clients became ready in time")
    except Exception as error:
        local_errors.append(str(error))
        ready_barrier.abort()

    save_client_results(
        client_id,
        local_latencies,
        local_operations,
        local_errors,
        results,
        results_lock,
    )


def run_parallel_phase(
    config,
    run_id,
    command_count,
    collect_latency,
    phase_name,
):
    clients_count = config["clients"]
    ready_barrier = threading.Barrier(clients_count + 1)
    start_event = threading.Event()
    results_lock = threading.Lock()
    results = {
        "latencies": [],
        "operations": Counter(),
        "errors": [],
    }
    threads = []

    for client_id in range(clients_count):
        thread = threading.Thread(
            target=run_client,
            args=(
                client_id,
                config,
                run_id,
                command_count,
                ready_barrier,
                start_event,
                collect_latency,
                results,
                results_lock,
            ),
            name=f"benchmark-client-{client_id}",
        )
        threads.append(thread)
        thread.start()

    try:
        ready_barrier.wait(timeout=CLIENTS_READY_TIMEOUT_SECONDS)
    except threading.BrokenBarrierError:
        start_event.set()

        for thread in threads:
            thread.join()

        if not results["errors"]:
            results["errors"].append(
                (-1, ["not all clients became ready in time"])
            )

        return results, 0.0

    print(f"{phase_name}: all {clients_count} clients connected.")

    started_at = time.perf_counter()
    start_event.set()

    for thread in threads:
        thread.join()

    elapsed = time.perf_counter() - started_at
    return results, elapsed


def print_results(config, results, elapsed):
    latencies = sorted(results["latencies"])
    operations = results["operations"]
    successful = sum(operations.values())
    expected = config["clients"] * config["commands_per_client"]

    print()
    print("===== RESULT =====")
    print(f"Clients:       {config['clients']}")
    print(f"Expected:      {expected}")
    print(f"Successful:    {successful}")
    print(f"Time:          {elapsed:.6f} seconds")

    if elapsed > 0:
        print(f"Throughput:    {successful / elapsed:.2f} commands/sec")

    print()
    print("Operations:")
    print(f"  GET:         {operations['GET']}")
    print(f"  SET:         {operations['SET']}")
    print(f"  DEL:         {operations['DEL']}")

    if latencies:
        average = sum(latencies) / len(latencies)

        print()
        print("Latency:")
        print(f"  avg:         {average:.3f} ms")
        print(f"  p50:         {percentile(latencies, 50):.3f} ms")
        print(f"  p95:         {percentile(latencies, 95):.3f} ms")
        print(f"  p99:         {percentile(latencies, 99):.3f} ms")
        print(f"  max:         {latencies[-1]:.3f} ms")

    if results["errors"]:
        print()
        print("===== ERRORS =====")

        for client_id, errors in results["errors"]:
            prefix = f"Client {client_id}" if client_id >= 0 else "Benchmark"

            for error in errors:
                print(f"{prefix}: {error}")

        print(f"Failed clients: {len(results['errors'])}")


def print_configuration(config, scenario):
    print(f"Scenario:      {scenario}")
    print(f"Server:        {config['host']}:{config['port']}")
    print(f"Clients:       {config['clients']}")
    print(f"Commands/client: {config['commands_per_client']}")
    print(
        "Mix:           "
        f"GET {config['get']}% / "
        f"SET {config['set']}% / "
        f"DEL {config['del']}%"
    )
    print(f"Keyspace:      {config['keyspace']}")
    print(f"Seed:          {config['seed']}")


def main():
    arguments = parse_arguments()

    try:
        config = read_config(
            filename=arguments.scenario,
            clients=arguments.clients,
            get_percent=arguments.get_percent,
            set_percent=arguments.set_percent,
            del_percent=arguments.del_percent,
        )
    except (OSError, ValueError) as error:
        print(f"Config error: {error}")
        return 1

    run_id = f"{int(time.time())}_{config['seed']}"
    print_configuration(config, arguments.scenario)
    print(f"Run ID:        {run_id}")

    exit_code = 0

    try:
        print()
        populate(config, run_id)

        if config["warmup_commands_per_client"] > 0:
            print()
            print("Starting warmup...")

            warmup_results, warmup_elapsed = run_parallel_phase(
                config=config,
                run_id=run_id,
                command_count=config["warmup_commands_per_client"],
                collect_latency=False,
                phase_name="Warmup",
            )

            print(f"Warmup finished in {warmup_elapsed:.3f} seconds.")

            if warmup_results["errors"]:
                raise RuntimeError("warmup failed")

        print()
        print("Starting benchmark...")

        results, elapsed = run_parallel_phase(
            config=config,
            run_id=run_id,
            command_count=config["commands_per_client"],
            collect_latency=True,
            phase_name="Benchmark",
        )

        print_results(config, results, elapsed)

        if results["errors"]:
            exit_code = 1

    except KeyboardInterrupt:
        print()
        print("Benchmark interrupted.")
        exit_code = 130
    except Exception as error:
        print(f"Benchmark failed: {error}")
        exit_code = 1
    finally:
        if config["cleanup"]:
            try:
                print()
                cleanup(config, run_id)
            except Exception as error:
                print(f"Cleanup failed: {error}")
                exit_code = 1

    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
