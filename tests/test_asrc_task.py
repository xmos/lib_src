# Copyright 2024 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

"""
Test verify the operation of both SSRC and ASRC (which belong to the multi-rate HiFi SRC collection)
"""

import pytest
import subprocess
from src_test_utils import vcd2wav, build_firmware
import numpy as np
from scipy.io import wavfile
from scipy.signal import stft, get_window, find_peaks
from scipy.fft import fft, fftfreq
import matplotlib.pyplot as plt
import itertools
import re
from thdncalculator import THDN_and_freq

SR_LIST = (44100, 48000, 88200, 96000, 176400, 192000)


def run_dut(bin_name, cmds, multi_tone=False, timeout=60):
    """
    Runs DUT with a set of frequency change commands.
    If multi-tone is set, then differening tones are sent
    to each channel otherwise we use a single tone for
    frequency change tests.
    """

    print("Running DUT")

    flattend_cmds = "1 " if multi_tone else "0 "
    flattend_cmds += ' '.join(str(x)for x in list(itertools.chain(*cmds)))

    cmd = f"xrun --id 0 --xscope-file trace --args {bin_name} {flattend_cmds}"
    print(f"running: {cmd}")
    output = subprocess.run(cmd, shell=True, capture_output=True, check=True, text=True, timeout=timeout)
    assert output.returncode == 0

    return output.stderr #xrun puts output on err for some reason


def analyse_wav(expected_freqs):
    print("Extracting WAV")
    vcd2wav("trace.vcd", 0, 1, 44100)

    print("Analysing WAV")
    sample_rate, data = wavfile.read("ch0-1-44100.wav")

    # Perform STFT
    frequencies, times, Zxx = stft(data, nperseg=4096, fs=sample_rate)

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
            print(f"found {expected:.4f}Hz - {peak:.4f}Hz @ FFT peak_idx {peak_idx} of {len(peaks)} peaks")
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
    plt.savefig("asrc_task_freq_plot.png")

    return True if expected_idx == len(expected_freqs) else False

def build_cmd_list_expected_f(input_srs, output_srs, chans, delay_ms, input_freq = (44100 / 44)):
    """Format is SR_IN, IN_CHANS, SR_OUT, POST_DELAY_MS """
    cmd_list = []
    expected_freqs = []

    for i in input_srs:
        for o in output_srs:
            cmd_list.append([i, chans, o, delay_ms])
            freq = input_freq * i / o
            expected_freqs.append(freq)

    return cmd_list, expected_freqs

@pytest.fixture(scope="module")
def build_xe():
    print("Building DUT")
    xe = build_firmware("test_asrc_task")
    xe_path = f"../../lib_src/build/tests/asrc_task_test/{xe}"

    return xe_path


def test_asrc_task_freq_matrix(build_xe):
    """
    Looks for a set of frequencies accordng to SR rate changes. We uses a sine table of len 44
    which produces a nominal 1kHz ish which gets scaled according to input SR.
    It does a full matrix of the input and output frequencies.
    """
    cmd_list, expected_freqs = build_cmd_list_expected_f(SR_LIST, SR_LIST, 4, 100)
    output = run_dut(build_xe, cmd_list, timeout=60)
    vcd2wav("trace.vcd", 0, 1, 44100)
    assert analyse_wav(expected_freqs)


def parse_output_for_changes(output, cmds):
    num_valid_configs = 0
    num_found_configs = 0
    for cmd in cmds:
        output_list = iter(output.split("\n"))
        i_sr = cmd[0]
        n_ch = cmd[1]
        o_sr = cmd[2]
        if i_sr == 0 or n_ch == 0 or o_sr == 0:
            continue
        num_valid_configs += 1

        found_sr = False
        found_ch = False
        while not found_sr or not found_ch:
            try:
                line = next(output_list)
            except StopIteration:
                break

            res = re.search(r'Input fs: (\d+) Output fs: (\d+)', line)
            if res:
                found_sr = True
                f_i_sr = int(res.group(1))
                f_o_sr = int(res.group(2))

            res = re.search(r'FIFO init channels: (\d+)', line)
            if res:
                found_ch = True
                f_n_ch = int(res.group(1))

        print(f"Found successful config: input_sr {f_i_sr} ({i_sr}), n_chan: {f_n_ch} ({n_ch}), output_sr {f_o_sr} ({o_sr})") 
        num_found_configs += 1

        assert i_sr == f_i_sr
        assert n_ch == f_n_ch
        assert o_sr == f_o_sr

    assert num_valid_configs == num_found_configs

def test_asrc_task_zero_transition(build_xe):
    """ This checks SR transitions through invalid ones (when something is zero) """

    delay_ms = 10
    cmd_list = [
                [0, 0, 0, delay_ms],
                [48000, 2, 0, delay_ms],
                [48000, 2, 48000, delay_ms],
                [48000, 0, 48000, delay_ms],    # ch count invalid
                [48000, 2, 48000, delay_ms],
                [48000, 2, 0, delay_ms],        # output sr invalid
                [48000, 2, 48000, delay_ms],
                [0, 2, 48000, delay_ms],        # input sr invalid
                [48000, 2, 48000, delay_ms],
                ]
    output = run_dut(build_xe, cmd_list, timeout=60)
    parse_output_for_changes(output, cmd_list)

def analyse_thd(wav_file, max_thd=-90):
    sample_rate, data = wavfile.read(wav_file)
    start_chop_s = 0.02
    data = (data[int(sample_rate*start_chop_s):] / ((1<<31) - 1)).astype(np.float64)
    thd, freq = THDN_and_freq(data, sample_rate)

    expected_freq = (44100 / 44) * sample_rate / 44100
    print(F"THD: {thd:.2f}, freq: {freq:.4f}, expected_freq: {expected_freq:.4f}")

    assert thd < max_thd
    assert np.isclose(freq, expected_freq, rtol = 0.0001)

def test_asrc_task_longish_run(build_xe):
    """
    This runs a slightly longer test at a single frequency and checks THDN
    """
    test_len_s = 10
    cmd_list = [[48000, 8, 48000, test_len_s * 1000]]
    output = run_dut(build_xe, cmd_list, timeout=60)
    parse_output_for_changes(output, cmd_list)
    vcd2wav("trace.vcd", 0, 1, 48000)
    analyse_thd("ch0-1-48000.wav")

def analyse_freq_multi_tone(wav_file):
    sample_rate, data = wavfile.read(wav_file)
    start_chop_s = 0.02

    for ch in range(8):
        ch_data = data[:,ch]
        ch_data = (ch_data[int(sample_rate*start_chop_s):] / ((1<<31) - 1)).astype(np.float64)
        thd, freq = THDN_and_freq(ch_data, sample_rate)

        expected_freq = (44100 / 44) * sample_rate / 44100 * (ch + 1)
        print(F"THD: {thd:.2f}, freq: {freq:.4f}, expected_freq: {expected_freq:.4f}")

        assert np.isclose(freq, expected_freq, rtol = 0.001) # Short test and sketchy sine for ch > 0 so larger RTOL

def test_asrc_task_channel_mapping(build_xe):
    """
    This runs a short test at a multiple frequencies and checks output channels have correct tones
    """
    test_len_s = 1
    test_sr = 44100
    cmd_list = [[test_sr, 8, test_sr, test_len_s * 1000]]
    output = run_dut(build_xe, cmd_list, multi_tone=True, timeout=60)
    vcd2wav("trace.vcd", 0, 8, test_sr)
    analyse_freq_multi_tone("ch0-8-44100.wav")

# For local test only
if __name__ == "__main__":
    analyse_thd("ch0-1-48000.wav")
