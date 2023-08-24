# Copyright 2016-2023 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

"""
Test verify the operation of both SSRC and ASRC (which belong to the multi-rate HiFi SRC collection)
"""

import pytest
from src_test_utils import gen_golden, build_firmware, build_host_app, compare_results, src_mrh_file_name_builder, run_dut


NUM_SAMPLES_TO_PROCESS = 256
SR_LIST = (44100, 48000, 88200, 96000, 176400, 192000)
ASRC_DEVIATIONS = ("1.000000", "0.990099", "1.009999")


@pytest.mark.prepare
@pytest.mark.parametrize("src_type", ["ssrc", "asrc"])
def test_prepare(src_type):
    """ Builds firmware and host reference and generates the golden reference signals on the host """
    host_app = build_host_app(src_type)
    firmware = build_firmware("test_" + src_type)
    gen_golden(host_app, src_type, NUM_SAMPLES_TO_PROCESS, SR_LIST, SR_LIST, ASRC_DEVIATIONS if src_type == "asrc" else None)
  

@pytest.mark.main
@pytest.mark.parametrize("sr_out", SR_LIST)
@pytest.mark.parametrize("sr_in", SR_LIST)
def test_ssrc(sr_in, sr_out):
    """ Runs the signal through the simulator via an xcore test app and compares to golden ref """
    run_dut(sr_in, sr_out, "ssrc", NUM_SAMPLES_TO_PROCESS)


@pytest.mark.main
@pytest.mark.parametrize("sr_out", SR_LIST)
@pytest.mark.parametrize("sr_in", SR_LIST)
@pytest.mark.parametrize("fs_deviation", ASRC_DEVIATIONS)
def test_asrc(sr_in, sr_out, fs_deviation):
    """ Runs the signal through the simulator via an xcore test app and compares to golden ref """

    run_dut(sr_in, sr_out, "asrc", NUM_SAMPLES_TO_PROCESS, fs_deviation=fs_deviation)
