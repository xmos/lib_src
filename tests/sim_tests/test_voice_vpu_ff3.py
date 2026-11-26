# Copyright 2023-2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from scipy import signal
import numpy as np
from pathlib import Path
import subprocess
import pytest
from thdncalculator import THDN_and_freq
from utils.src_test_utils import build_firmware_xcommon_cmake
import tempfile
import shutil
import contextlib
import os

from lib_src.voice import design

fsup = 48000
fsdown = 16000
# Long enough for the AES17 THD+N method in audio_test_tools: at 16 kHz this
# gives more than one 8192 sample Welch segment, and leaves plenty of signal
# after the notch filter settling time is discarded.
sig_len_s = 1.0

# THD+N limits in dB for (16 kHz, 48 kHz) outputs, keyed by (num_taps, test_freq).
# Shared by the Python reference and the C implementation, ~3 dB above measured.
# The 16 kHz output of a decimated pure tone is limited only by arithmetic
# precision (~-172..-190 dB); the 48 kHz output is limited by image rejection.
thdn_limits = {
    (96, 1000): (-160, -79),
    (96, 7000): (-160, -66),
    (72, 1000): (-160, -73),
    (72, 7000): (-160, -38),
}

def assert_thdn(thdn, thdn_ex):
    assert thdn < thdn_ex, f"THDN {thdn} is higher then the threshold of {thdn_ex} dB"

def gen_sig(f):
    time = np.arange(0, sig_len_s, 1/fsup)
    vol = 0.8
    sig_fl = vol * np.sin(2 * np.pi * f * time)
    sig_int = (sig_fl * (2**31)).astype(np.int32)
    print("num samples: ", len(sig_int))

    name = f"sig_{str(48) if fsup == 48000.0 else str(16)}k"
    sig_int.tofile(str(name + ".bin"))
    thdn, freq = THDN_and_freq(sig_int.astype(np.float64), 48000, fund_freq=f)
    print(f"NP 48k THDN: {thdn}, fc: {freq}")

    return sig_fl, sig_int

def conv_simple(h, x):
    acc = 0
    for p in range(len(h)):
        acc += h[p] * x[p]

    return acc

def run_py(sig48k_int, taps, fc, num_taps):

    bounds = thdn_limits[(num_taps, fc)]

    sig16k_int = np.zeros(len(sig48k_int) // 3, np.int32)
    if True:
        sig16k_int64 = np.zeros(len(sig48k_int) // 3, np.int64)
        sig16k_int64 = signal.convolve(sig48k_int.astype(np.int64), taps.astype(np.int64), "full", "direct")[:len(sig48k_int)]
        sig16k_int = (sig16k_int64[0::3] * 2 ** (-31)).astype(np.int32)
    else:
        state = np.zeros(mixed_taps.shape, np.int64)
        for i in range(len(sig16k_int)):
            state[0] = np.roll(state[0], 1)
            state[0][0] = sig48k_int[i * 3]
            acc = conv_simple(mixed_taps[0], state[0])
            state[1] = np.roll(state[1], 1)
            state[1][0] = sig48k_int[i * 3 + 1]
            acc += conv_simple(mixed_taps[1], state[1])
            state[2] = np.roll(state[2], 1)
            state[2][0] = sig48k_int[i * 3 + 2]
            acc += conv_simple(mixed_taps[2], state[2])

            sig16k_int[i] = (acc * 2 ** (-30)).astype(np.int32)

    thdn, freq = THDN_and_freq(sig16k_int.astype(np.float64), fsdown, fund_freq=fc)
    print(f"PY 16k THDN: {thdn}, fc: {freq}")
    assert_thdn(thdn, bounds[0])

    sig48k_int = np.zeros(len(sig48k_int), np.int32)
    if True:
        sig48k_int64 = np.zeros(len(sig48k_int), np.int64)
        sig48k_int64[0::3] = sig16k_int.astype(np.int64)
        sig48k_int64 = signal.convolve(sig48k_int64.astype(np.int64), taps.astype(np.int64), "full", "direct")[:len(sig48k_int64)]
        sig48k_int = (sig48k_int64 * 3 * 2 ** (-31)).astype(np.int32)
    else:
        state = np.zeros(mixed_taps.shape[1], np.int64)
        for i in range(len(sig48k_int) // 3):
            state = np.roll(state, 1)
            state[0] = sig16k_int[i]
            sig48k_int[i * 3] = (conv_simple(mixed_taps[2], state) * 3 * 2 ** (-30)).astype(np.int32)
            sig48k_int[i * 3 + 1] = (conv_simple(mixed_taps[1], state) * 3 * 2 ** (-30)).astype(np.int32)
            sig48k_int[i * 3 + 2] = (conv_simple(mixed_taps[0], state) * 3 * 2 ** (-30)).astype(np.int32)

    thdn, freq = THDN_and_freq(sig48k_int.astype(np.float64), fsup, fund_freq=fc)
    print(f"PY 48k THDN: {thdn}, fc: {freq}")
    assert_thdn(thdn, bounds[1])


def run_c(fc, xe_name, num_taps):

    bounds = thdn_limits[(num_taps, fc)]

    cmd = f"xsim {xe_name}"
    stdout = subprocess.check_output(cmd.split())
    #print("run msg\n", stdout)

    sig_bin = Path("sig_c_16k.bin")
    assert sig_bin.is_file(), "could not find sig_c_16k.bin"
    sig16k_int = np.fromfile(sig_bin, dtype=np.int32)

    sig_bin = Path("sig_c_48k.bin")
    assert sig_bin.is_file(), "could not find sig_c_48k.bin"
    sig48k_int = np.fromfile(sig_bin, dtype=np.int32)

    thdn, freq = THDN_and_freq(sig16k_int.astype(np.float64), fsdown, fund_freq=fc)
    print(f"C  16k THDN: {thdn}, fc: {freq}")
    assert_thdn(thdn, bounds[0])


    thdn, freq = THDN_and_freq(sig48k_int.astype(np.float64), fsup, fund_freq=fc)
    print(f"C  48k THDN: {thdn}, fc: {freq}")
    assert_thdn(thdn, bounds[1])

@contextlib.contextmanager
def tmp_dir(new_dir):
    curdir = os.getcwd()
    os.chdir(new_dir)
    try:
        yield
    finally:
        os.chdir(curdir)
        shutil.rmtree(new_dir)

@pytest.mark.parametrize("num_taps", [96, 72])
@pytest.mark.prepare
def test_src_vpu_ff3_prepare(num_taps):
    print(f"Building for num_taps {num_taps}")
    build_firmware_xcommon_cmake(Path(__file__).parent / "vpu_ff3_test", config=f"{num_taps}t")

@pytest.mark.parametrize(
    "test_freq", [
        1000,
        7000]
)
@pytest.mark.parametrize("num_taps", [96, 72])
@pytest.mark.main
def test_src_vpu_ff3(test_freq, num_taps):
    print(f"Test name vpu_ff3_test_{num_taps}t.xe")
    print(f"Testing {test_freq} Hz sinewave")

    file_path = Path(__file__).parent
    xe = file_path / "vpu_ff3_test" / "bin" / f"{num_taps}t"/ f"vpu_ff3_test_{num_taps}t.xe"
    working_dir = Path(tempfile.mkdtemp(prefix="test_src_vpu_ff3", dir=file_path))

    with tmp_dir(working_dir):
        taps_int = design.to_fixed(design.ff3_kaiser(num_taps), 31)
        _, sig_int = gen_sig(test_freq)
        run_py(sig_int, taps_int, test_freq, num_taps)
        run_c(test_freq, xe, num_taps)
