import pytest
from pytest_embedded_idf.dut import IdfDut

def test_running(dut) -> None:
    """Test that the applilcation is running."""
    dut.expect('app_main: ESP32-cam_AI is running')

