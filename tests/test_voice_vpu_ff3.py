# Copyright 2023 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from scipy import signal
import numpy as np
from pathlib import Path
import subprocess
import sys, os
import pytest
from thdncalculator import THDN_and_freq
import src_test_utils

try:
    from fixed_factor_vpu_voice import src_ff3_fir_gen as gf
except ModuleNotFoundError:
    assert False, "Could not find src_ff3_fir_gen.py script"

test_dir = Path(__file__).parent
build_dir = test_dir / "../build/tests/vpu_ff3_test/"

TEST_NAME_72t = "test_src_vpu_72t"
TEST_NAME_96t = "test_src_vpu_96t"

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

def run_py(sig48k_int, taps, fc, xe_name):

    if xe_name == TEST_NAME_96t:
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

def build_c(taps_int, mixed_int, taps_per_phase, xe_name):
    coeffs_path = Path(__file__).resolve().parent / "vpu_ff3_test" / "autogen"
    coeffs_path.mkdir(exist_ok=True, parents=True)

    name = "test_src_96t" if "96" in xe_name else "test_src_72t"
    gf.generate_header_file(coeffs_path, num_taps_per_phase = taps_per_phase, name = name)
    gf.generate_c_file(coeffs_path, taps_int, mixed_int, num_taps_per_phase = taps_per_phase, name = name)
    src_test_utils.build_firmware(xe_name, extra_args=f"-D{xe_name}_build=ON")

def run_c(fc, xe_name):

    if xe_name == TEST_NAME_96t:
        bounds = [-75, -75]
        if fc == 7000: bounds[1] = -60
    else:
        bounds = [-60, -55]
        if fc == 7000: bounds[1] = -35

    app = f"xsim {build_dir}/{xe_name}.xe"
    stdout = subprocess.check_output(app, cwd = build_dir, shell = True)
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

@pytest.mark.parametrize(
    "xe_name", [
    TEST_NAME_72t,
    TEST_NAME_96t]
)
@pytest.mark.prepare
def test_src_vpu_ff3_prepare(xe_name):
    print(f"Preparing test name {xe_name}")

    if xe_name == TEST_NAME_96t:
        taps_per_phase = 32
    else:
        taps_per_phase = 24
    taps_fl, taps_int, mixed_int = gf.gen_coefs(num_taps_per_phase = taps_per_phase)
    build_c(taps_int, mixed_int, taps_per_phase, xe_name)

@pytest.mark.parametrize(
    "test_freq", [
        1000, 
        7000]
)
@pytest.mark.parametrize(
    "xe_name", [
    TEST_NAME_72t,
    TEST_NAME_96t]
)
@pytest.mark.main
def test_src_vpu_ff3(test_freq, xe_name):
    print(f"Test name {xe_name}")
    print(f"Testing {test_freq} Hz sinewave")

    if xe_name == TEST_NAME_96t:
        taps_per_phase = 32
    else:
        taps_per_phase = 24

    taps_fl, taps_int, mixed_int = gf.gen_coefs(num_taps_per_phase = taps_per_phase)
    sig_fl, sig_int = gen_sig(test_freq)
    run_py(sig_int, taps_int, test_freq, xe_name)
    run_c(test_freq, xe_name)
