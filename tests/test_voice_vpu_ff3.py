# Copyright 2023 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from scipy import signal
import numpy as np
from pathlib import Path
import subprocess
import pytest
from thdncalculator import THDN_and_freq
import src_test_utils

try:
    from fixed_factor_vpu_voice import src_ff3_fir_gen as gf
except ModuleNotFoundError:
    assert False, "Could not find src_ff3_fir_gen.py script"

test_dir = Path(__file__).parent
build_dir = test_dir

fsup = 48000
fsdown = 16000

def assert_thdn_and_fc(thdn, fc, thdn_ex, fc_ex):
    assert abs(fc - fc_ex) < 1, f"center frequency {fc} Hz is not whithin the allowed range from the exepcted {fc_ex} Hz"
    assert thdn < thdn_ex, f"THDN {thdn} is higher then the threshold of {thdn_ex} dB"

def gen_sig(f):
    length = 0.5
    time = np.arange(0, length, 1/fsup)
    vol = 0.8
    sig_fl = vol * np.sin(2 * np.pi * f * time)
    sig_int = (sig_fl * (2**31)).astype(np.int32)
    print("num samples: ", len(sig_int))

    name = f"sig_{str(48) if fsup == 48000.0 else str(16)}k"
    sig_int.tofile(build_dir /  str(name + ".bin"))
    thdn, freq = THDN_and_freq(sig_int.astype(np.float64), 48000)
    print(f"NP 48k THDN: {thdn}, fc: {freq}")

    return sig_fl, sig_int

def conv_simple(h, x):
    acc = 0
    for p in range(len(h)):
        acc += h[p] * x[p]

    return acc

def run_py(sig48k_int, taps, fc, num_taps):

    if num_taps == 96:
        bounds = [-75, -75]
        if fc == 7000: bounds[1] = -60
    else:
        bounds = [-75, -75]
        if fc == 7000: bounds[1] = -35

    sig16k_int = np.zeros(len(sig48k_int) // 3, np.int32)
    if True:
        sig16k_int64 = np.zeros(len(sig48k_int) // 3, np.int64)
        sig16k_int64 = signal.convolve(sig48k_int.astype(np.int64), taps.astype(np.int64), "same", "direct")
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

    thdn, freq = THDN_and_freq(sig16k_int.astype(np.float64), fsdown)
    print(f"PY 16k THDN: {thdn}, fc: {freq}")
    assert_thdn_and_fc(thdn, freq, bounds[0], fc)

    sig48k_int = np.zeros(len(sig48k_int), np.int32)
    if True:
        sig48k_int64 = np.zeros(len(sig48k_int), np.int64)
        sig48k_int64[0::3] = sig16k_int.astype(np.int64)
        sig48k_int64 = signal.convolve(sig48k_int64.astype(np.int64), taps.astype(np.int64), "same", "direct")
        sig48k_int = (sig48k_int64 * 3 * 2 ** (-31)).astype(np.int32)
    else:
        state = np.zeros(mixed_taps.shape[1], np.int64)
        for i in range(len(sig48k_int) // 3):
            state = np.roll(state, 1)
            state[0] = sig16k_int[i]
            sig48k_int[i * 3] = (conv_simple(mixed_taps[2], state) * 3 * 2 ** (-30)).astype(np.int32)
            sig48k_int[i * 3 + 1] = (conv_simple(mixed_taps[1], state) * 3 * 2 ** (-30)).astype(np.int32)
            sig48k_int[i * 3 + 2] = (conv_simple(mixed_taps[0], state) * 3 * 2 ** (-30)).astype(np.int32)

    thdn, freq = THDN_and_freq(sig48k_int.astype(np.float64), fsup)
    print(f"PY 48k THDN: {thdn}, fc: {freq}")
    assert_thdn_and_fc(thdn, freq, bounds[1], fc)


def run_c(fc, xe_name, num_taps):

    if num_taps == 96:
        bounds = [-75, -75]
        if fc == 7000: bounds[1] = -60
    else:
        bounds = [-60, -55]
        if fc == 7000: bounds[1] = -35

    cmd = f"xsim {xe_name}"
    stdout = subprocess.check_output(cmd.split(), cwd = build_dir)
    #print("run msg\n", stdout)

    sig_bin = build_dir / "sig_c_16k.bin"
    assert sig_bin.is_file(), "could not find sig_c_16k.bin"
    sig16k_int = np.fromfile(sig_bin, dtype=np.int32)

    thdn, freq = THDN_and_freq(sig16k_int.astype(np.float64), fsdown)
    print(f"C  16k THDN: {thdn}, fc: {freq}")
    assert_thdn_and_fc(thdn, freq, bounds[0], fc)

    sig_bin = build_dir / "sig_c_48k.bin"
    assert sig_bin.is_file(), "could not find sig_c_48k.bin"
    sig48k_int = np.fromfile(sig_bin, dtype=np.int32)

    thdn, freq = THDN_and_freq(sig48k_int.astype(np.float64), fsup)
    print(f"C  48k THDN: {thdn}, fc: {freq}")
    assert_thdn_and_fc(thdn, freq, bounds[1], fc)

@pytest.mark.parametrize("num_taps", [96, 72])
@pytest.mark.prepare
def test_src_vpu_ff3_prepare(num_taps):
    print(f"Building for num_taps {num_taps}")
    src_test_utils.build_firmware_xcommon_cmake("vpu_ff3_test", config=f"{num_taps}t")

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
    num_phases = 3

    taps_per_phase = int(num_taps / num_phases)

    _, taps_int, _ = gf.gen_coefs(num_taps_per_phase = taps_per_phase)
    _, sig_int = gen_sig(test_freq)
    run_py(sig_int, taps_int, test_freq, num_taps)
    run_c(test_freq, xe, num_taps)
