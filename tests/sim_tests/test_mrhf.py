# Copyright 2016-2024 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

"""
Test verify the operation of both SSRC and ASRC (which belong to the multi-rate HiFi SRC collection)
"""

import pytest
from utils.src_test_utils import gen_golden, run_dut, build_firmware_xcommon_cmake, build_host_app_xcommon_cmake
from pathlib import Path


NUM_SAMPLES_TO_PROCESS = 256
SR_LIST = (44100, 48000, 88200, 96000, 176400, 192000)
ASRC_DEVIATIONS = ("1.000000", "0.990099", "1.009999")


@pytest.mark.prepare
@pytest.mark.parametrize("src_type", ["ssrc", "asrc"])
def test_prepare(src_type):
    """ Builds firmware and host reference and generates the golden reference signals on the host """
    host_app = build_host_app_xcommon_cmake(Path(__file__).parent / f"{src_type}_test")
    build_firmware_xcommon_cmake(Path(__file__).parent / f"{src_type}_test")
    gen_golden(host_app, src_type, NUM_SAMPLES_TO_PROCESS, SR_LIST, SR_LIST, ASRC_DEVIATIONS if src_type == "asrc" else None)


@pytest.mark.main
@pytest.mark.parametrize("sr_out", SR_LIST)
@pytest.mark.parametrize("sr_in", SR_LIST)
def test_ssrc(sr_in, sr_out):
    """ Runs the signal through the simulator via an xcore test app and compares to golden ref """
    file_path = Path(__file__).parent
    xe = file_path / "ssrc_test" / "bin" / "ssrc_test.xe"
    run_dut(xe, sr_in, sr_out, "ssrc", NUM_SAMPLES_TO_PROCESS, compare_mode='abs_diff')


@pytest.mark.main
@pytest.mark.parametrize("sr_out", SR_LIST)
@pytest.mark.parametrize("sr_in", SR_LIST)
@pytest.mark.parametrize("fs_deviation", ASRC_DEVIATIONS)
def test_asrc(sr_in, sr_out, fs_deviation):
    """ Runs the signal through the simulator via an xcore test app and compares to golden ref """
    file_path = Path(__file__).parent
    xe = file_path / "asrc_test" / "bin" / "asrc_test.xe"
    run_dut(xe, sr_in, sr_out, "asrc", NUM_SAMPLES_TO_PROCESS, fs_deviation=fs_deviation, compare_mode='abs_diff')
