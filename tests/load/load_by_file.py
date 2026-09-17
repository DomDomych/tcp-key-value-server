import math
import random
import socket
import threading
import time
from collections import Counter
from pathlib import Path


SCENARIO_FILE = Path(__file__).with_name("scenario.txt")
BENCHMARK_PREFIX = "__benchmark__"


def read_config(filename):
    config = {}

    with open(filename, "r", encoding="utf-8") as file:
        for line_number, line in enumerate(file, start=1):
            line = line.strip()

            if not line or line.startswith("#"):
                continue

            if "=" not in line:
                raise ValueError(
                    f"Wrong config line {line_number}: {line}"
                )

            key, value = line.split("=", 1)
            config[key.strip()] = value.strip()

    required = {
        "host",
        "port",
        "clients",
        "commands_per_client",
        "get",
        "set",
        "del",
        "keyspace",
        "seed",
        "warmup_commands_per_client",
        "cleanup",
    }

    missing = required - config.keys()
    if missing:
        raise ValueError(
            "Missing config keys: " + ", ".join(sorted(missing))
        )

    parsed = {
        "host": config["host"],
        "port": int(config["port"]),
        "clients": int(config["clients"]),
        "commands_per_client": int(config["commands_per_client"]),
        "get": int(config["get"]),
        "set": int(config["set"]),
        "del": int(config["del"]),
        "keyspace": int(config["keyspace"]),
        "seed": int(config["seed"]),
        "warmup_commands_per_client": int(
            config["warmup_commands_per_client"]
        ),
        "cleanup": config["cleanup"].lower()
        in {"1", "true", "yes", "on"},
    }

    if parsed["clients"] <= 0:
        raise ValueError("clients must be > 0")

    if parsed["commands_per_client"] <= 0:
        raise ValueError("commands_per_client must be > 0")

    if parsed["keyspace"] <= 0:
        raise ValueError("keyspace must be > 0")

    if parsed["warmup_commands_per_client"] < 0:
        raise ValueError("warmup_commands_per_client must be >= 0")

    if parsed["get"] + parsed["set"] + parsed["del"] != 100:
        raise ValueError("get + set + del must equal 100")

    return parsed


def receive_response(sock, buffer):
    while True:
        newline_pos = buffer.find(b"\n")

        if newline_pos != -1:
            response = buffer[:newline_pos]
            del buffer[:newline_pos + 1]
            return response.decode("utf-8")

        chunk = sock.recv(4096)

        if not chunk:
            raise ConnectionError("Server closed connection")

        buffer.extend(chunk)


def send_command(sock, buffer, command):
    sock.sendall((command + "\n").encode("utf-8"))
    return receive_response(sock, buffer)


def percentile(sorted_values, percent):
    if not sorted_values:
        return 0.0

    index = math.ceil((percent / 100.0) * len(sorted_values)) - 1
    index = max(0, min(index, len(sorted_values) - 1))
    return sorted_values[index]


def key_for(run_id, index):
    return f"{BENCHMARK_PREFIX}:{run_id}:key:{index}"


def populate(config, run_id):
    print("Populating benchmark keyspace...")

    with socket.create_connection(
        (config["host"], config["port"])
    ) as sock:
        buffer = bytearray()

        for index in range(config["keyspace"]):
            key = key_for(run_id, index)
            value = f"value_{index}"
            send_command(sock, buffer, f"SET {key} {value}")

    print(f"Populated {config['keyspace']} keys.")


def cleanup(config, run_id):
    print("Cleaning benchmark keys...")

    with socket.create_connection(
        (config["host"], config["port"])
    ) as sock:
        buffer = bytearray()

        for index in range(config["keyspace"]):
            key = key_for(run_id, index)
            send_command(sock, buffer, f"DEL {key}")

    print("Cleanup complete.")


def random_operation(rng, config):
    value = rng.randrange(100)

    if value < config["get"]:
        return "GET"

    if value < config["get"] + config["set"]:
        return "SET"

    return "DEL"


def build_command(rng, config, run_id, operation):
    index = rng.randrange(config["keyspace"])
    key = key_for(run_id, index)

    if operation == "GET":
        return f"GET {key}"

    if operation == "SET":
        value = f"value_{rng.randrange(1_000_000)}"
        return f"SET {key} {value}"

    return f"DEL {key}"


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
            (config["host"], config["port"])
        ) as sock:
            buffer = bytearray()

            ready_barrier.wait()
            start_event.wait()

            for _ in range(command_count):
                operation = random_operation(rng, config)
                command = build_command(
                    rng, config, run_id, operation
                )

                start = time.perf_counter_ns()

                try:
                    send_command(sock, buffer, command)
                except Exception as error:
                    local_errors.append(str(error))
                    break

                end = time.perf_counter_ns()

                local_operations[operation] += 1

                if collect_latency:
                    local_latencies.append(
                        (end - start) / 1_000_000.0
                    )

    except Exception as error:
        local_errors.append(str(error))

    with results_lock:
        results["latencies"].extend(local_latencies)
        results["operations"].update(local_operations)

        if local_errors:
            results["errors"].append(
                (client_id, local_errors)
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
        )

        threads.append(thread)
        thread.start()

    ready_barrier.wait()

    print(f"{phase_name}: all clients connected.")

    start_time = time.perf_counter()
    start_event.set()

    for thread in threads:
        thread.join()

    elapsed = time.perf_counter() - start_time

    return results, elapsed


def print_results(config, results, elapsed):
    latencies = sorted(results["latencies"])
    operations = results["operations"]

    successful = sum(operations.values())
    expected = (
        config["clients"]
        * config["commands_per_client"]
    )

    print()
    print("===== RESULT =====")
    print(f"Clients:       {config['clients']}")
    print(f"Expected:      {expected}")
    print(f"Successful:    {successful}")
    print(f"Time:          {elapsed:.6f} seconds")

    if elapsed > 0:
        print(
            f"Throughput:    {successful / elapsed:.2f} commands/sec"
        )

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
        print(
            f"  p50:         {percentile(latencies, 50):.3f} ms"
        )
        print(
            f"  p95:         {percentile(latencies, 95):.3f} ms"
        )
        print(
            f"  p99:         {percentile(latencies, 99):.3f} ms"
        )
        print(f"  max:         {latencies[-1]:.3f} ms")

    if results["errors"]:
        print()
        print("===== ERRORS =====")

        for client_id, errors in results["errors"]:
            for error in errors:
                print(f"Client {client_id}: {error}")

        print(f"Failed clients: {len(results['errors'])}")


def main():
    try:
        config = read_config(SCENARIO_FILE)
    except Exception as error:
        print(f"Config error: {error}")
        raise SystemExit(1)

    run_id = f"{int(time.time())}_{config['seed']}"

    print(f"Server:        {config['host']}:{config['port']}")
    print(f"Clients:       {config['clients']}")
    print(
        f"Commands/client: {config['commands_per_client']}"
    )
    print(
        "Mix:           "
        f"GET {config['get']}% / "
        f"SET {config['set']}% / "
        f"DEL {config['del']}%"
    )
    print(f"Keyspace:      {config['keyspace']}")
    print(f"Seed:          {config['seed']}")
    print(f"Run ID:        {run_id}")
    print()

    exit_code = 0

    try:
        populate(config, run_id)

        if config["warmup_commands_per_client"] > 0:
            print()
            print("Starting warmup...")

            warmup_results, warmup_elapsed = run_parallel_phase(
                config=config,
                run_id=run_id,
                command_count=config[
                    "warmup_commands_per_client"
                ],
                collect_latency=False,
                phase_name="Warmup",
            )

            print(
                f"Warmup finished in {warmup_elapsed:.3f} s"
            )

            if warmup_results["errors"]:
                print("Warmup failed.")
                exit_code = 1
                return

        print()
        print("Starting benchmark...")

        results, elapsed = run_parallel_phase(
            config=config,
            run_id=run_id,
            command_count=config[
                "commands_per_client"
            ],
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

    raise SystemExit(exit_code)


if __name__ == "__main__":
    main()