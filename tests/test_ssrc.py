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
import shutil
from src_test_utils import build_firmware, build_host_app, compare_results, src_mrh_file_name_builder


NUM_SAMPLES_TO_PROCESS = 16


def run_dut(in_sr, out_sr, src_type):
    file_dir = Path(__file__).resolve().parent
    tmp_dir = file_dir / "tmp" / src_type / f"{in_sr}_{out_sr}"
    tmp_dir.mkdir(exist_ok=True, parents=True)

    # When running parallel, make copies of files so we don't encounter issues using xdist
    bin_name = f"{src_type}_test.xe"
    bin_path = file_dir / f"../build/tests/{src_type}_test" / bin_name
    shutil.copy(bin_path, tmp_dir / bin_name)

    test_in_path = file_dir / "src_input"
    test_out_path = file_dir / "src_output"
    test_out_path.mkdir(exist_ok=True)

    fnb = src_mrh_file_name_builder()

    test_signal_0, test_signal_1 = fnb.get_in_signal_pair(in_sr)
    shutil.copy(test_in_path / test_signal_0, tmp_dir / test_signal_0)
    shutil.copy(test_in_path / test_signal_1, tmp_dir / test_signal_1)

    dut_signal_0, dut_signal_1 = fnb.get_out_signal_pair(in_sr, out_sr, "dut")

    
    cmd = f"xsim --args {str(tmp_dir / bin_name)}"
    cmd += f" -i {tmp_dir / test_signal_0} {tmp_dir / test_signal_1}"
    cmd += f" -o {tmp_dir / dut_signal_0} {tmp_dir / dut_signal_1}"
    cmd += f" -f {in_sr} -g {out_sr} -n {NUM_SAMPLES_TO_PROCESS}"
    print(f"Generating dut {in_sr}->{out_sr}")
    output = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    if output.returncode != 0:
        assert 0, f"Error, stdout: {output.stdout}, sterr: {output.stderr}, running: {cmd}"

    assert(compare_results(tmp_dir / test_signal_0, tmp_dir / dut_signal_0))
    assert(compare_results(tmp_dir / test_signal_1, tmp_dir / dut_signal_1))

    shutil.copy(tmp_dir / test_signal_0, test_out_path / test_signal_0)
    shutil.copy(tmp_dir / test_signal_1, test_out_path / test_signal_1)

    max_mips = 0
    for line in output.stdout.split("\n"):
        found = re.search(r".*Process time per chan ticks=(\d+).*", line)
        if found:
            process_time_ticks =int(found.group(1))
            timer_hz = 100e6
            cpu_hz = 600e6
            num_threads = 5
            instr_per_sample = process_time_ticks * cpu_hz / timer_hz / num_threads
            mips = instr_per_sample * in_sr / 1e6
            max_mips = mips if mips > max_mips else max_mips

    with open(tmp_dir / f"max_mips.txt", "wt") as mf:
        mf.write(f"{in_sr}->{out_sr}:{max_mips}")
    print(f"max_mips for {in_sr}->{out_sr}: {max_mips}")

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
    bin_path = file_dir / f"../build/tests/{src_type}_test/ssrc_golden" 
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
@pytest.mark.parametrize("sr_out", (44100, 48000, 88200, 96000, 176400, 192000))
@pytest.mark.parametrize("sr_in", (44100, 48000, 88200, 96000, 176400, 192000))
def test_ssrc(sr_in, sr_out):
    run_dut(sr_in, sr_out, "ssrc")
