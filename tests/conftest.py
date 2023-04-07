import ipaddress
import pytest


def pytest_addoption(parser):
    parser.addoption(
        "--server_ip",
        action="store",
        help="IP address of the server to test",
    )
    parser.addoption(
        "--server_port",
        action="store",
        type=int,
        default=23,
        help="Port of the server to test",
    )
    parser.addoption(
        "--lines_count",
        action="store",
        type=int,
        default=1000,
        help="Number of lines to process",
    )


@pytest.fixture
def server_ip(request):
    return ipaddress.IPv4Address(request.config.getoption("--server_ip"))


@pytest.fixture
def server_port(request):
    return request.config.getoption("--server_port")


@pytest.fixture
def lines_count(request):
    return request.config.getoption("--lines_count")
