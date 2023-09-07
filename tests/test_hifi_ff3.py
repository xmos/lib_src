# Copyright 2023 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

"""
Test verify the operation of both SSRC and ASRC (which belong to the multi-rate HiFi SRC collection)
"""

import pytest
from src_test_utils import gen_golden, build_firmware, build_host_app, compare_results, src_mrh_file_name_builder, run_dut

NUM_SAMPLES_TO_PROCESS = 256

@pytest.mark.prepare
@pytest.mark.parametrize("src_type", ["ds3", "os3"])
def test_prepare(src_type):
    """ Builds firmware and host reference and generates the golden reference signals on the host """
    host_app = build_host_app(src_type)
    firmware = build_firmware("test_" + src_type)
    sr_in = 16000 if src_type == "os3" else 48000
    sr_out = 48000 if src_type == "os3" else 16000
    gen_golden(host_app, src_type, NUM_SAMPLES_TO_PROCESS, [sr_in], [sr_out], None)
  

@pytest.mark.main
@pytest.mark.parametrize("src_type", ["ds3", "os3"])
def test_hifi_ff3(src_type):
    """ Runs the signal through the simulator via an xcore test app and compares to golden ref """
    sr_in = 16000 if src_type == "os3" else 48000
    sr_out = 48000 if src_type == "os3" else 16000
    run_dut(sr_in, sr_out, src_type, NUM_SAMPLES_TO_PROCESS)
