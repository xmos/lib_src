# Copyright 2024 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

"""
Test verify the operation of both SSRC and ASRC (which belong to the multi-rate HiFi SRC collection)
"""

import pytest
import subprocess
from src_test_utils import vcd2wav
import numpy as np
from scipy.io import wavfile
from scipy.signal import stft
import matplotlib.pyplot as plt

def run_dut(bin_name, timeout=60):
    cmd = f"xrun --xscope-file trace {bin_name}"
    output = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=timeout)


@pytest.mark.prepare
def test_prepare():
    """ ......... """
    print("Building DUT")
    cmd = f"cd asrc_task_test && xmake -j"
    output = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=30)

@pytest.mark.main
def test_asrc_task():
    """ .... """
    print("Running DUT")
    run_dut("./asrc_task_test/bin/asrc_task_test.xe", timeout=60)
    print("Extracting WAV")
    vcd2wav("trace.vcd", 0, 1, 44100)

    print("Analysing WAV")
    sample_rate, data = wavfile.read("ch0-1-44100.wav")

    # Perform STFT
    frequencies, times, Zxx = stft(data, nperseg=4096, fs=sample_rate)

    SR_LIST = (44100, 48000, 88200, 96000, 176400, 192000)
    SR_LIST = (44100, 48000, 88200, 96000)
    input_freq = 1000
    expected_freqs = []
    for i in SR_LIST:
        for o in SR_LIST:
            freq = input_freq * i / o
            expected_freqs.append(freq)

    print(expected_freqs)

    # expected_freqs = expected_freqs * 2 * 2 * 7 # order * order * chans
    # expected_freqs = expected_freqs * 7
    expected_freqs = expected_freqs

    peaks = []
    for time in range(len(times)):
        peak = np.argmax(np.abs(Zxx)[:,time])
        peaks.append(frequencies[peak])


    expected_idx = 0
    peak_idx = 0
    for peak in peaks:
        expected = expected_freqs[expected_idx]
        # print(expected, peak)
        if np.isclose(expected, peak, rtol=0.05):
            print(f"found {expected} - {peak} @ peak_idx {peak_idx} of {len(peaks)} peaks")
            expected_idx += 1
            if expected_idx == len(expected_freqs):
                break
        peak_idx += 1


    print(f"expected_idx: {expected_idx} of {len(expected_freqs)}")

    # Plot the result
    plt.pcolormesh(times, frequencies, np.abs(Zxx))
    plt.title('STFT Magnitude')
    plt.ylabel('Frequency [Hz]')
    plt.xlabel('Time [sec]')
    plt.yscale('log')
    plt.ylim(500, 5000)
    plt.show()
