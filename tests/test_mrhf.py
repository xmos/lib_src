# Copyright 2023 XMOS LIMITED.
# This Software is subject to the terms of the XCORE VocalFusion Licence.

"""
Test to check and explore the behaviour of the adaptive rate controller in usb_buffer.
It uses a simplified model of usb_buffer with the actual sw_pll model and is a good representation of real world behaviour.
"""

import pytest
from src_test_utils import prepare, build_firmware, build_host_app, compare_results, src_mrh_file_name_builder, run_dut


NUM_SAMPLES_TO_PROCESS = 16
SR_LIST = (44100, 48000, 88200, 96000, 176400, 192000)


@pytest.fixture(scope="session")
def host_app():
    return build_host_app(["ssrc"])

@pytest.fixture(scope="session")
def firmware():
    return build_firmware(["ssrc"])

@pytest.mark.prepare
@pytest.mark.parametrize("src_type", ["ssrc"])
def test_prepare(host_app, firmware, src_type):
    """ -- """
    prepare(host_app, firmware, src_type, NUM_SAMPLES_TO_PROCESS)
  

@pytest.mark.main
@pytest.mark.parametrize("sr_out", SR_LIST)
@pytest.mark.parametrize("sr_in", SR_LIST)
def test_ssrc(sr_in, sr_out):
    run_dut(sr_in, sr_out, "ssrc", NUM_SAMPLES_TO_PROCESS)
