import pytest
from pytest_embedded_idf.dut import IdfDut

def test_running(dut) -> None:
    """Test that the applilcation is running."""
    dut.expect('main: esp32cam_snap is running')

