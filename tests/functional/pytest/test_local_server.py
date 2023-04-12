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


class ResultSingleLine:
    def __init__(self, rand_seed: int,
                 hex_expected: str = '', hex_received: str = '',
                 error=None):
        self.rand_seed: int = rand_seed
        self.hex_expected: str = hex_expected
        self.hex_received: str = hex_received
        self.error = error

    def __str__(self):
        return f'{{ rand_seed: {self.rand_seed},\n' \
               f"hex_expected: {self.hex_expected if self.hex_received != '' else 'failed_to_calculate'},\n" \
               f"hex_received: {self.hex_received if self.hex_received != '' else 'failed_to_receive'},\n" \
               f'error: {str(self.error)} }}'


def validate_single_line(server: socket, symbols: int, rand_seed: int) -> ResultSingleLine:
    import random

    result = ResultSingleLine(rand_seed)

    try:
        hasher = hashlib.sha256()
        rand = random.Random(rand_seed)
        while symbols != 0:
            to_send = min(symbols, rand.randint(1, 2048))
            chunk = ''.join(rand.choice(string.ascii_letters + string.digits) for _ in range(to_send))
            hasher.update(chunk.encode())
            symbols -= to_send
            if symbols == 0:
                chunk += '\n'
            server.sendall(chunk.encode())

        hex_received = server.recv(512).decode()
        hex_expected = hasher.hexdigest()

    except (socket.timeout, ConnectionError) as e:
        result.error = f'Request failed: {e}'
        return result
    except Exception as e:
        result.error = f'Unexpected error: {e}'
        return result

    result.hex_expected = hex_expected + '\n'
    result.hex_received = hex_received
    return result


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

    rand_seed = 815
    try:
        with socket.create_connection(('127.0.0.1', server_port), timeout=2) as sock:
            result = validate_single_line(sock, 10000, rand_seed)
            if result.error is not None or result.hex_received != result.hex_expected:
                pytest.fail(f'{str(result)}')
    except (socket.timeout, ConnectionError) as e:
        pytest.fail(f'Request failed: {e}, (rand_seed: {rand_seed})')
    finally:
        kill_server(server_process)

    assert server_process.returncode == 0, \
        f'failed to shutdown the server properly, return code: {server_process.returncode}'


def create_tcp_connection(server_port, rand_seed: int):
    try:
        with socket.create_connection(('127.0.0.1', server_port), timeout=2) as sock:
            return validate_single_line(sock, 10000, rand_seed)
    except (socket.timeout, ConnectionError) as e:
        return ResultSingleLine(rand_seed, error=e)


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
        # Create 100 concurrent TCP connections
        seeds = [i for i in range(1, 101)]
        from concurrent.futures import ThreadPoolExecutor
        with ThreadPoolExecutor(max_workers=100) as executor:
            futures = []
            for _, seed in enumerate(seeds):
                futures.append((seed, executor.submit(create_tcp_connection, server_port, seed)))

        failed_results = [ResultSingleLine(seed, exc) if exc else future.result() for _, (seed, future) in
                          enumerate(futures)
                          if (exc := future.exception()) or
                          future.result().error is not None or
                          future.result().hex_expected != future.result().hex_received]

        if len(failed_results) != 0:
            msg = f'Requests failed: {len(failed_results)}/{len(futures)}\n' + '\n'.join(
                [f'{str(res)}' for _, res in enumerate(failed_results)])
            pytest.fail(msg)

    finally:
        kill_server(server_process)

    assert server_process.returncode == 0, \
        f'failed to shutdown the server properly, return code: {server_process.returncode}'
