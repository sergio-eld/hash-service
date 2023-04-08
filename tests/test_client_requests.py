import ipaddress
import hashlib
import socket
import traceback

import pytest


def test_predefined_line(server_ip: ipaddress.IPv4Address, server_port: int):
    try:
        request = 'oceanic 815'
        hasher = hashlib.sha256()
        hasher.update(request.encode())
        hex_expected = hasher.hexdigest()

        with socket.create_connection((str(server_ip), server_port), timeout=2) as sock:
            sock.sendall(request.join('\n').encode())
            data = sock.recv(512)
            hex_received = data.decode()

        if not hex_received.endswith('\n'):
            pytest.fail(f'Invalid string format received: {hex_received}')

        assert hex_expected == hex_received.rstrip('\n'), 'Invalid string received'

    except (socket.timeout, ConnectionError) as e:
        pytest.fail(f'Request failed: {e}')
    except Exception as e:
        traceback.print_exc()
        pytest.fail(f'Unexpected error: {e}')


def test_n_lines(server_ip: ipaddress.IPv4Address, server_port: int, lines_count: int = 1000):
    assert False, "this test is not implemented"
