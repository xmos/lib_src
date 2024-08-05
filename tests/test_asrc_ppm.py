# Copyright 2024 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

"""
Test verify the operation of both SSRC and ASRC (which belong to the multi-rate HiFi SRC collection)
"""

import pytest
import subprocess
from src_test_utils import vcd2wav, build_firmware_xccm
import numpy as np
from scipy.io import wavfile
import matplotlib.pyplot as plt
import itertools
from thdncalculator import THDN_and_freq

SR_LIST = (44100, 48000, 88200, 96000, 176400, 192000)


def run_dut(bin_name, cmds, timeout=60):
    """
    Runs DUT with a set of frequency commands. See asrc_task_test_ppm.xc for expected format
    """
    print("Running DUT")
    print(cmds)
    flattend_cmds = ' '.join(str(x)for x in list(itertools.chain(*cmds)))

    cmd = f"xrun --id 0 --xscope-file trace --args {bin_name} {flattend_cmds}"
    print(f"running: {cmd}") # so we can try locally
    output = subprocess.run(cmd, shell=True, capture_output=True, check=True, text=True, timeout=timeout)
    assert output.returncode == 0

    return output.stderr #xrun puts output on err for some reason


@pytest.fixture(scope="module")
def build_xe():
    print("Building DUT")
    xe = build_firmware_xccm("asrc_task_test_ppm")

    return xe


def analyse_thd(wav_file, max_thd=None):
    sample_rate, data = wavfile.read(wav_file)
    start_chop_s = 0.02
    data = (data[int(sample_rate*start_chop_s):] / ((1<<31) - 1)).astype(np.float64)
    thd, freq = THDN_and_freq(data, sample_rate)

    expected_freq = (44100 / 44) * sample_rate / 44100
    print(F"THD: {thd:.2f}, freq: {freq:.4f}, expected_freq: {expected_freq:.4f}")

    if max_thd is not None:
        assert thd < max_thd
    assert np.isclose(freq, expected_freq, rtol = 0.0001)

    return thd

def apply_ppm(freq, ppm):
    freq += freq * (ppm / 1e6)
    return int(freq + 0.5)

def plot_phase(data, name):
    data = np.array(data)
    xpoints = np.arange(0, data.size)
    ypoints = np.array(data)

    plt.plot(xpoints, ypoints)
    plt.savefig(name + ".png")

def test_asrc_range(build_xe):
    """
    This runs a slightly longer test at a single frequency and checks THDN
    """

    pytest.skip() # WORK IN PROGRESS!

    test_len_s = 10
    fifo_len = 100
    in_freq = 48000
    out_freq = 48000
    ppm = 10
    cmd_list = [[in_freq, out_freq, apply_ppm(out_freq, ppm), test_len_s, fifo_len]]
    output = run_dut(build_xe, cmd_list, timeout=test_len_s + 5)
    print(output)

    chan_start = 0
    chan_stop = 1
    data = vcd2wav("trace.vcd", chan_start, chan_stop, 48000)
    thd = analyse_thd(f"ch{chan_start}-{chan_stop}-48000.wav")
    print(thd)
    plot_phase(data[chan_stop], "plot")



# For local test only
if __name__ == "__main__":
    analyse_thd("ch0-1-48000.wav")
