# Copyright 2023 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from scipy import signal
import numpy as np
from pathlib import Path
import subprocess
import sys, os
import pytest
import soundfile as sf
from thdncalculator import THDN_and_freq
import src_test_utils


try:
    from fixed_factor_vpu_voice import src_rat_fir_gen as gf
except ModuleNotFoundError:
    assert False, "Could not find src_rat_fir_gen.py script in module fixed_factor_vpu_voice"

test_dir = Path(__file__).parent
build_dir = test_dir / "../build/tests/vpu_rat_test/"

def assert_thdn_and_fc(thdn, fc, thdn_ex, fc_ex):
    assert abs(fc - fc_ex) < 1, f"center frequency {fc} Hz is not whithin the allowed range from the exepcted {fc_ex} Hz"
    assert thdn < thdn_ex, f"THDN {thdn} is higher then the threshold of {thdn_ex} dB"

def get_sig(frequency):
    fs = 48000
    time = np.arange(0, 0.5, 1/fs)
    #print(len(time))
    sig_fl = 0.9 * np.sin(2 * np.pi * frequency * time)
    #sig_fl = 0.9 * signal.sawtooth(2 * np.pi * frequency * time)
    sig_int = (sig_fl * (2 ** 31)).astype(np.int32)
    name = f"sig_48k"
    sig_int.tofile(build_dir /  str(name + ".bin"))
    thdn, freq = THDN_and_freq(sig_int.astype(np.float64), fs)
    #print(f"NP 48k THDN: {thdn}, fc: {freq}")

    return sig_fl, sig_int

def conv_simple(h, x):
    acc = 0
    for p in range(len(h)):
        acc += h[p] * x[p]
    
    return acc

def upsample(sig32k_fl, orig_taps, poly_taps, fc_ex):
    fact_up = 3
    fact_down = 2
    sig48k = np.zeros(len(sig32k_fl) * fact_up // fact_down)
    
    if True:
        buff = np.zeros(len(sig32k_fl) * fact_up)
        buff[0::fact_up] = sig32k_fl
        buff = signal.convolve(buff, orig_taps, "same", "direct") * fact_up
        sig48k = buff[0::fact_down]
    else:
        state = np.zeros(32)
        for i in range(len(sig32k_fl) // 2):
            state = np.roll(state, 1)
            state[0] = sig32k_fl[i * 2]
            sig48k[i * 3] = conv_simple(poly_taps[0], state) * fact_up
            sig48k[i * 3 + 1] = conv_simple(poly_taps[2], state) * fact_up
            state = np.roll(state, 1)
            state[0] = sig32k_fl[i * 2 + 1]
            sig48k[i * 3 + 2] = conv_simple(poly_taps[1], state) * fact_up

    thdn, freq = THDN_and_freq(sig48k.astype(np.float64), 48000)
    print(f"PY 48k THDN: {thdn}, fc: {freq}")
    assert_thdn_and_fc(thdn, freq, -60, fc_ex)
    return sig48k

def downsample(sig48k_fl, orig_taps, poly_taps, fc_ex):
    fact_up = 2
    fact_down = 3
    sig32k = np.zeros(len(sig48k_fl) * fact_up // fact_down)

    if True:
        buff = np.zeros(len(sig48k_fl) * fact_up)
        buff[0::2] = sig48k_fl
        buff = signal.convolve(buff, orig_taps, "same", "direct") * fact_up
        sig32k = buff[0::3]
    else:
        state = np.zeros(48)
        for i in range(len(sig48k_fl) // 3):
            state = np.roll(state, 2)
            state[0] = sig48k_fl[i * 3 + 1]
            state[1] = sig48k_fl[i * 3]
            sig32k[i * 2] = conv_simple(poly_taps[0], state) * fact_up
            state = np.roll(state, 1)
            state[0] = sig48k_fl[i * 3 + 2]
            sig32k[i * 2 + 1] = conv_simple(poly_taps[1], state) * fact_up

    thdn, freq = THDN_and_freq(sig32k.astype(np.float64), 32000)
    print(f"PY 32k THDN: {thdn}, fc: {freq}")
    assert_thdn_and_fc(thdn, freq, -70, fc_ex)
    return sig32k

def build_c(poly_ds_int, poly_us_int):
    coeffs_path = Path(__file__).resolve().parent / "vpu_rat_test" / "autogen"
    coeffs_path.mkdir(exist_ok=True, parents=True)

    gf.generate_header_file(coeffs_path)
    gf.generate_c_file(coeffs_path, poly_ds_int, poly_us_int)

    src_test_utils.build_firmware("test_src_vpu_rat")

def run_c(fc_ex):

    app = "xsim --args " + str(build_dir) + "/test_src_vpu_rat.xe"
    stdout = subprocess.check_output(app, cwd = build_dir, shell = True)
    #print("run msg\n", stdout)

    sig_bin = build_dir / "sig_c_32k.bin"
    assert sig_bin.is_file(), "could not find output bin"
    sig32k_int = np.fromfile(sig_bin, dtype=np.int32)

    sf.write(build_dir / "sig_c_32k.wav", sig32k_int, 32000, "PCM_32")
    thdn, freq = THDN_and_freq(sig32k_int.astype(np.float64), 32000)
    print(f"C 32k THDN: {thdn}, fc: {freq}")
    assert_thdn_and_fc(thdn, freq, -70, fc_ex)

    sig_bin = build_dir / "sig_c_48k.bin"
    assert sig_bin.is_file(), "could not find output bin"
    sig48k_int = np.fromfile(sig_bin, dtype=np.int32)

    sf.write(build_dir / "sig_c_48k.wav", sig48k_int, 48000, "PCM_32")
    thdn, freq = THDN_and_freq(sig48k_int.astype(np.float64), 48000)
    print(f"C 48k THDN: {thdn}, fc: {freq}")
    assert_thdn_and_fc(thdn, freq, -60, fc_ex)

@pytest.mark.prepare
def test_src_vpu_rat_prepare():
    print(f"Preparing rat test")
    total_num_taps = 8 * 2 * 3 * 2 # 8 for vpu, 2 and 3 for ds and us phases adds up to 96 taps
    taps, poly_ds, poly_ds_int, poly_us, poly_us_int = gf.gen_coefs(total_num_taps)
    build_c(poly_ds_int, poly_us_int)


@pytest.mark.parametrize(
    "test_freq", [
        100, 14000
        ]
)
@pytest.mark.main
def test_src_vpu_rat(test_freq):
    print(f"Testing {test_freq} Hz sinewave")
    total_num_taps = 8 * 2 * 3 * 2 # 8 for vpu, 2 and 3 for ds and us phases adds up to 96 taps
    taps, poly_ds, poly_ds_int, poly_us, poly_us_int = gf.gen_coefs(total_num_taps)
    sig_fl, sig_int = get_sig(test_freq)
    sig32k = downsample(sig_fl, taps, poly_ds, test_freq)
    upsample(sig32k, taps, poly_us, test_freq)
    run_c(test_freq)
