# Copyright 2023 XMOS LIMITED.
# This Software is subject to the terms of the XCORE VocalFusion Licence.

"""
Test to check and explore the behaviour of the adaptive rate controller in usb_buffer.
It uses a simplified model of usb_buffer with the actual sw_pll model and is a good representation of real world behaviour.
"""

import pytest
from pathlib import Path
import subprocess
import re
from src_test_utils import build_firmware, build_host_app, compare_results, src_mrh_file_name_builder, run_dut


NUM_SAMPLES_TO_PROCESS = 16
SR_LIST = (44100, 48000, 88200, 96000, 176400, 192000)


@pytest.fixture(scope="session")
def host_app():
    return build_host_app(["ssrc"])

@pytest.fixture(scope="session")
def firmware():
    return build_firmware(["ssrc"])

@pytest.mark.prepare
def test_prepare(host_app, firmware):
    """ -- """

    src_type = "ssrc"
    fnb = src_mrh_file_name_builder()
    freqs = tuple(fnb.file_name_helper.keys())

    file_dir = Path(__file__).resolve().parent
    bin_path = file_dir / f"../build/tests/{src_type}_test/{src_type}_golden" 
    test_in_path = file_dir / "src_input"
    test_out_path = file_dir / "src_output"
    test_out_path.mkdir(exist_ok=True)

    for q, out_sr in zip(range(len(freqs)), freqs):
        for k, in_sr in zip(range(len(freqs)), freqs):
            print(f"Generating golden {in_sr}->{out_sr}")
            test_signal_0, test_signal_1 = fnb.get_in_signal_pair(in_sr)
            golden_signal_0, golden_signal_1 = fnb.get_out_signal_pair(in_sr, out_sr, "golden")

            cmd = f"{bin_path} -i{test_in_path/test_signal_0} -j{test_in_path/test_signal_1} -k{k}"
            cmd +=f" -o{test_out_path/golden_signal_0} -p{test_out_path/golden_signal_1} -q{q} -d0 -l{NUM_SAMPLES_TO_PROCESS} -n4"
            output = subprocess.run(cmd, input=" ", text=True, shell=True, capture_output=True) # pass an input as the binary expects to press any key at end 
            if output.returncode != 0:
                print(f"Error, stdout: {output.stdout}, stderr {output.stderr}")

@pytest.mark.main
@pytest.mark.parametrize("sr_out", SR_LIST)
@pytest.mark.parametrize("sr_in", SR_LIST)
def test_ssrc(sr_in, sr_out):
    run_dut(sr_in, sr_out, "ssrc", NUM_SAMPLES_TO_PROCESS)
