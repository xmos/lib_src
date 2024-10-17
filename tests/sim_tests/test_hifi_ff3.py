# Copyright 2023-2024 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

"""
Test verify the operation of both SSRC and ASRC (which belong to the multi-rate HiFi SRC collection)
"""

import pytest
from utils.src_test_utils import gen_golden, run_dut, build_firmware_xcommon_cmake, build_host_app_xcommon_cmake
from pathlib import Path

NUM_SAMPLES_TO_PROCESS = 256

@pytest.mark.prepare
@pytest.mark.parametrize("src_type", ["ds3", "os3"])
def test_prepare(src_type):
    """ Builds firmware and host reference and generates the golden reference signals on the host """
    host_app = build_host_app_xcommon_cmake(Path(__file__).parent / f"{src_type}_test")
    build_firmware_xcommon_cmake(Path(__file__).parent / f"{src_type}_test")
    sr_in = 16000 if src_type == "os3" else 48000
    sr_out = 48000 if src_type == "os3" else 16000
    gen_golden(host_app, src_type, NUM_SAMPLES_TO_PROCESS, [sr_in], [sr_out], None)


@pytest.mark.main
@pytest.mark.parametrize("src_type", ["ds3", "os3"])
def test_hifi_ff3(src_type):
    """ Runs the signal through the simulator via an xcore test app and compares to golden ref """
    file_path = Path(__file__).parent
    xe = file_path / f"{src_type}_test" / "bin" / f"{src_type}_test.xe"
    sr_in = 16000 if src_type == "os3" else 48000
    sr_out = 48000 if src_type == "os3" else 16000
    run_dut(xe, sr_in, sr_out, src_type, NUM_SAMPLES_TO_PROCESS)
