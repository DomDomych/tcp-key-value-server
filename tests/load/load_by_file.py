import socket
import threading
import time
from pathlib import Path


HOST = "127.0.0.1"
PORT = 8080

SCENARIO_FILE = Path(__file__).with_name("scenario.txt")


def read_scenario(filename):
    clients = {}
    current_id = None

    with open(filename, "r", encoding="utf-8") as file:
        for line_number, line in enumerate(file, start=1):
            line = line.strip()

            if not line:
                continue

            if line.startswith("#"):
                continue

            if line.startswith("ID:"):
                try:
                    current_id = int(line.split(":", 1)[1].strip())
                except ValueError:
                    raise ValueError(
                        f"Wrong client ID at line {line_number}: {line}"
                    )

                if current_id in clients:
                    raise ValueError(
                        f"Duplicate client ID: {current_id}"
                    )

                clients[current_id] = []
                continue

            if current_id is None:
                raise ValueError(
                    f"Command before first ID at line {line_number}: {line}"
                )

            clients[current_id].append(line)

    if not clients:
        raise ValueError("Scenario contains no clients")

    return clients


def receive_response(sock):
    data = bytearray()

    while True:
        chunk = sock.recv(1)

        if not chunk:
            raise ConnectionError("Server closed connection")

        if chunk == b"\n":
            break

        data.extend(chunk)

    return data.decode("utf-8")


def run_client(
    client_id,
    commands,
    ready_barrier,
    start_event,
    errors,
    errors_lock
):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.connect((HOST, PORT))

            # Сообщаем, что этот клиент уже подключился.
            ready_barrier.wait()

            # Ждем общего старта теста.
            start_event.wait()

            for command in commands:
                sock.sendall((command + "\n").encode("utf-8"))

                response = receive_response(sock)

                # Если захочешь смотреть ответы каждого клиента:
                # print(f"[Client {client_id}] {command} -> {response}")

    except Exception as error:
        with errors_lock:
            errors.append((client_id, str(error)))


def main():
    try:
        clients = read_scenario(SCENARIO_FILE)
    except Exception as error:
        print(f"Scenario error: {error}")
        return

    clients_count = len(clients)

    total_commands = sum(
        len(commands)
        for commands in clients.values()
    )

    ready_barrier = threading.Barrier(clients_count + 1)
    start_event = threading.Event()

    threads = []

    errors = []
    errors_lock = threading.Lock()

    print(f"Server: {HOST}:{PORT}")
    print(f"Clients: {clients_count}")
    print(f"Commands: {total_commands}")
    print("Connecting clients...")

    for client_id, commands in clients.items():
        thread = threading.Thread(
            target=run_client,
            args=(
                client_id,
                commands,
                ready_barrier,
                start_event,
                errors,
                errors_lock
            )
        )

        threads.append(thread)
        thread.start()

    # Ждем, пока все клиенты подключатся к серверу.
    ready_barrier.wait()

    print("All clients connected.")
    print("Starting load test...")

    start_time = time.perf_counter()

    # Одновременно разрешаем всем клиентам отправлять команды.
    start_event.set()

    for thread in threads:
        thread.join()

    end_time = time.perf_counter()

    elapsed = end_time - start_time

    print()
    print("===== RESULT =====")
    print(f"Clients:     {clients_count}")
    print(f"Commands:    {total_commands}")
    print(f"Time:        {elapsed:.6f} seconds")

    if elapsed > 0:
        print(
            f"Throughput:  {total_commands / elapsed:.2f} commands/sec"
        )

    if errors:
        print()
        print("===== ERRORS =====")

        for client_id, error in errors:
            print(f"Client {client_id}: {error}")

        print(f"Failed clients: {len(errors)}")
        raise SystemExit(1)
    else:
        print("Errors:      0")


if __name__ == "__main__":
    main()