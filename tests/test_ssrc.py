# Copyright 2016-2023 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
# Copyright 2023 XMOS LIMITED.
# This Software is subject to the terms of the XCORE VocalFusion Licence.

"""
Test to check and explore the behaviour of the adaptive rate controller in usb_buffer.
It uses a simplified model of usb_buffer with the actual sw_pll model and is a good representation of real world behaviour.
"""

import pytest
from pathlib import Path
import matplotlib.pyplot as plt
import numpy as np
import contextlib, os
import subprocess
import re
import shutil
import csv

NUM_SAMPLES_TO_PROCESS = 128


def graph_it(filename, freqs, y_axis_name, horiz_line=None, title_suffix=None):
    plt.clf()

    # Plot the data
    x_axis = [x / sof_freq for x in range(len(freqs))]
    y_axis = freqs

    plt.plot(x_axis, y_axis)
    if horiz_line is not None:
        plt.axhline(y=horiz_line, color='r', linestyle='-')
    title = 'PI controller'
    title +=  "" if title_suffix is None else f"{title_suffix}"
    plt.title(title)
    plt.xlabel('Seconds')
    plt.ylabel(y_axis_name)
    filename = filename if title_suffix is None else filename.replace(".", f"{title_suffix}.").replace(" ", "_")
    plt.savefig(filename, dpi=300)
    # plt.show()

def plot_histogram(filename, data):
    plt.clf()
    plt.hist(data)
    plt.title("Final frequency PPM at end of simulation")
    plt.xlabel("PLL Frequency PPM")
    plt.ylabel("Occurance")
    plt.savefig(filename, dpi=300)

supported_sr = (44100, 48000, 88200, 96000, 176400, 192000)

class src_mrh_file_name_builder:
    """Helper to build the input/output/golden filenames from various input output sample rares"""

    signal_types = {"pure_sine": "s1k_0dB", "inter_modulation": "im10k11k_m6dB"}
    file_name_helper = {44100: "44", 48000: "48", 88200: "88", 96000: "96", 176400: "176", 192000: "192"}

    def test_signal(self, input_sr, signal_type):
        file_name = self.signal_types[signal_type] + "_" + self.file_name_helper[input_sr] + ".dat"
        return file_name

    def output_signal(self, input_sr, output_sr, signal_type):
        file_name = self.signal_types[signal_type] + "_" + self.file_name_helper[input_sr] + "_" + self.file_name_helper[output_sr]
        return file_name

    def get_in_signal_pair(self, input_sr):
        ch0 = self.test_signal(input_sr, "pure_sine")
        ch1 = self.test_signal(input_sr, "inter_modulation")
        return (ch0, ch1)

    def get_out_signal_pair(self, input_sr, output_sr, extension):
        ch0 = self.output_signal(input_sr, output_sr, "pure_sine") + f".{extension}"
        ch1 = self.output_signal(input_sr, output_sr, "inter_modulation") + f".{extension}"
        return (ch0, ch1)

def compare_results(golden_file, dut_file, isclose=False):
    golden = np.loadtxt(golden_file, dtype=np.int32)
    dut = np.loadtxt(golden_file, dtype=np.int32)

    same = True
    if(isclose):
        same = np.all_close(golden, dut, rtol=2)
    else:
        same = np.array_equal(golden, dut)

    return same

def run_dut(in_sr, out_sr):
    file_dir = Path(__file__).resolve().parent
    tmp_dir = file_dir / "tmp" / f"{in_sr}_{out_sr}"
    tmp_dir.mkdir(exist_ok=True, parents=True)

    # When running parallel, make copies of files so we don't encounter issues
    bin_name = "ssrc_test.xe"
    bin_path = file_dir / "../build/tests/ssrc_test" / bin_name
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
def build_host_app():
    file_dir = Path(__file__).resolve().parent
    bin_path = file_dir / "ssrc_test/model" 
    subprocess.run("make", cwd=str(bin_path))

@pytest.fixture(scope="session")
def test_build_firmware():
    file_dir = Path(__file__).resolve().parent
    build_path = file_dir / "../build" 
    build_path.mkdir(exist_ok=True)
    subprocess.run("cmake --toolchain ../xmos_cmake_toolchain/xs3a.cmake  ..", shell=True, cwd=str(build_path))
    subprocess.run("make -j ssrc_test", shell=True, cwd=str(build_path))

@pytest.mark.prepare
def test_prepare(build_host_app, test_build_firmware):
    """ -- """

    fnb = src_mrh_file_name_builder()
    freqs = tuple(fnb.file_name_helper.keys())

    file_dir = Path(__file__).resolve().parent
    bin_path = file_dir / "ssrc_test/model/ssrc_golden" 
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
                print(f"Error, stdout: {output.stdout}")

@pytest.mark.main
@pytest.mark.parametrize("sr_out", (44100, 48000, 88200, 96000, 176400, 192000))
@pytest.mark.parametrize("sr_in", (44100, 48000, 88200, 96000, 176400, 192000))
def test_ssrc(sr_in, sr_out):
    run_dut(sr_in, sr_out)
