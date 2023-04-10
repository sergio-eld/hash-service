import hashlib
import os
import signal
import socket
import string
import threading
import traceback

import pytest
from pathlib import Path
import subprocess
import time


def validate_single_line(server: socket, symbols: int):
    import random
    try:
        hasher = hashlib.sha256()
        while symbols != 0:
            to_send = min(symbols, random.randint(1, 2048))
            chunk = ''.join(random.choice(string.ascii_letters + string.digits) for _ in range(to_send))
            hasher.update(chunk.encode())
            symbols -= to_send
            if symbols == 0:
                chunk += '\n'
            server.sendall(chunk.encode())

        hex_received = server.recv(512).decode()
        hex_expected = hasher.hexdigest()

    except (socket.timeout, ConnectionError) as e:
        pytest.fail(f'Request failed: {e}')
    except Exception as e:
        traceback.print_exc()
        pytest.fail(f'Unexpected error: {e}')

    if not hex_received.endswith('\n'):
        pytest.fail(f'Invalid string format received: {hex_received}')
    assert hex_expected == hex_received.rstrip('\n'), 'Invalid string received'


# TODO: fix for windows
def kill_server(server_process):
    try:
        if os.name == 'nt':
            os.kill(server_process.pid, signal.CTRL_C_EVENT)
        else:
            server_process.send_signal(signal.SIGINT)
        server_process.wait(timeout=2)
    except (OSError, subprocess.TimeoutExpired):
        server_process.kill()
        pytest.fail("Failed to properly shutdown the server")


def test_local_server_run_single_connection(local_server: Path, server_port: int):
    server_process = subprocess.Popen(
        [local_server, str(server_port)]
    )

    # Wait for the process to start up
    for _ in range(2):
        code = server_process.poll()
        if code is not None:
            pytest.fail(f"Server process failed to start up properly, returned: {code}")
        time.sleep(1)

    try:
        with socket.create_connection(('127.0.0.1', server_port), timeout=2) as sock:
            validate_single_line(sock, 10000)
    except (socket.timeout, ConnectionError) as e:
        pytest.fail(f'Request failed: {e}')
    finally:
        kill_server(server_process)

    assert server_process.returncode == 0


def create_tcp_connection(server_port):
    try:
        with socket.create_connection(('127.0.0.1', server_port), timeout=2) as sock:
            validate_single_line(sock, 10000)
            return 'OK'
    except (socket.timeout, ConnectionError):
        return 'FAILED'


def test_local_server_multiple_connections(local_server: Path, server_port: int):
    server_process = subprocess.Popen(
        [local_server, str(server_port)]
    )

    # Wait for the process to start up
    for _ in range(2):
        code = server_process.poll()
        if code is not None:
            pytest.fail(f"Server process failed to start up properly, returned: {code}")
        time.sleep(1)

    try:
        from concurrent.futures import ThreadPoolExecutor
        # Create 100 concurrent TCP connections
        with ThreadPoolExecutor(max_workers=100) as executor:
            futures = []
            for i in range(100):
                futures.append(executor.submit(create_tcp_connection, server_port))
            for future in futures:
                assert future.result() == 'OK'

    except (socket.timeout, ConnectionError) as e:
        pytest.fail(f'Request failed: {e}')

    finally:
        kill_server(server_process)

    assert server_process.returncode == 0
