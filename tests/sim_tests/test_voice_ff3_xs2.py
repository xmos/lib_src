# Copyright 2023-2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
"""
Round trip test for the deprecated fixed factor of 3 voice SRC (src_ds3_voice / src_us3_voice).

A 48 kHz sine is decimated to 16 kHz and interpolated back to 48 kHz by an xcore test app.
Both outputs must match a bit-exact Python model and meet THD+N and gain limits.
"""
from pathlib import Path
import subprocess
import tempfile
import shutil
import contextlib
import os

import numpy as np
from scipy import signal
import pytest
from thdncalculator import THDN_and_freq
from utils.src_test_utils import build_firmware_xcommon_cmake

from lib_src.voice import design

fsup = 48000
fsdown = 16000
num_phases = design.LEGACY_FF3V_NUM_PHASES
# Long enough for the AES17 THD+N method in audio_test_tools
sig_len_s = 1.0
# Skip the filter start-up before measuring the output level
gain_skip_s = 0.1
# The C output is bit-exact, so the gain only differs from the filter response by rounding
gain_tol_db = 0.01

# THD+N limits in dB for the (16 kHz, 48 kHz) outputs, keyed by test frequency, ~3 dB above measured.
# The Python model is bit-exact, so the same limits apply to it and to the C output.
# The 16 kHz output of a decimated pure tone is limited only by arithmetic precision;
# the 48 kHz output is limited by the ~-64 dB stopband of the filter (image rejection).
thdn_limits = {
    1000: (-175, -59),
    7000: (-175, -61),
}


def gen_sig(f, vol=0.8):
    time = np.arange(0, sig_len_s, 1 / fsup)
    sig_int = (vol * np.sin(2 * np.pi * f * time) * (2**31)).astype(np.int32)
    sig_int.tofile("sig_48k.bin")
    return sig_int


def output_stage(acc, comp, comp_q):
    """
    Final stage of the src_ds3_voice/src_us3_voice functions: take bits [62:31] of
    the 64-bit accumulator, multiply by comp and shift right by comp_q with saturation.
    """
    r = (acc >> 31).astype(np.int64)
    r = np.array([int(x) * comp for x in r], dtype=object)
    lim = 1 << (31 + comp_q)
    r = np.clip(r, -lim, lim - 1)
    return np.array([x >> comp_q for x in r], dtype=np.int32)


def model_ds3(sig48k, coefs):
    """Bit-exact model of the src_ds3_voice_add_sample/add_final_sample sequence."""
    acc = np.convolve(sig48k.astype(np.int64), coefs["coefs_debug"].astype(np.int64))
    # Each output is computed when the last of its three input samples arrives
    acc = acc[num_phases - 1:len(sig48k):num_phases]
    return output_stage(acc, coefs["comp_ds"], coefs["comp_q_ds"])


def model_us3(sig16k, coefs):
    """Bit-exact model of the src_us3_voice_input_sample/get_next_sample sequence."""
    stuffed = np.zeros(len(sig16k) * num_phases, np.int64)
    stuffed[0::num_phases] = sig16k
    acc = np.convolve(stuffed, coefs["coefs_debug"].astype(np.int64))[:len(stuffed)]
    return output_stage(acc, coefs["comp_us"], coefs["comp_q_us"])


def check_thdn(name, sig, fs, fc, limit):
    thdn, freq = THDN_and_freq(sig.astype(np.float64), fs, fund_freq=fc)
    print(f"{name} THDN: {thdn:.1f} dB, fc: {freq}")
    assert abs(freq - fc) < 1, f"{name}: center frequency {freq} Hz is not within 1 Hz of the expected {fc} Hz"
    assert thdn < limit, f"{name}: THDN {thdn:.1f} dB is higher than the threshold of {limit} dB"


def rms(x):
    return np.sqrt(np.mean(x.astype(np.float64) ** 2))


def check_gain(name, sig_out, fs, sig_in, expected_db):
    """
    Check the output level against the input, which verifies the comp_ds/comp_us gain
    compensation. The filter has ~0.7 dB passband ripple, so the expected gain is its
    response at the test frequency rather than 0 dB.
    """
    gain_db = 20 * np.log10(rms(sig_out[int(fs * gain_skip_s):]) / rms(sig_in[int(fsup * gain_skip_s):]))
    print(f"{name} gain: {gain_db:.4f} dB, expected: {expected_db:.4f} dB")
    assert abs(gain_db - expected_db) < gain_tol_db, \
        f"{name}: gain {gain_db:.4f} dB differs from the expected {expected_db:.4f} dB by more than {gain_tol_db} dB"


@contextlib.contextmanager
def tmp_dir(new_dir):
    curdir = os.getcwd()
    os.chdir(new_dir)
    try:
        yield
    finally:
        os.chdir(curdir)
        shutil.rmtree(new_dir)


def run_c(xe):
    output = subprocess.run(["xsim", str(xe)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    print(output.stdout)
    assert output.returncode == 0
    return np.fromfile("sig_c_16k.bin", dtype=np.int32), np.fromfile("sig_c_48k.bin", dtype=np.int32)


def check_bit_exact(c16k, c48k, py16k, py48k):
    np.testing.assert_array_equal(c16k, py16k, "16 kHz C output does not match the Python model")
    np.testing.assert_array_equal(c48k, py48k, "48 kHz C output does not match the Python model")


@pytest.mark.prepare
def test_voice_ff3_xs2_prepare():
    """ Build firmware """
    build_firmware_xcommon_cmake(Path(__file__).parent / "ff3_voice_test")


@pytest.mark.main
@pytest.mark.parametrize("test_freq", [1000, 7000])
@pytest.mark.parametrize("arch", ["xs2"])
def test_voice_ff3_xs2(test_freq, arch):
    """ Runs a sine through ds3 and back through us3 in the simulator and checks THD+N """
    file_dir = Path(__file__).parent
    xe = file_dir / "ff3_voice_test" / "bin" / arch / f"ff3_voice_test_{arch}.xe"
    working_dir = Path(tempfile.mkdtemp(prefix="test_voice_ff3_xs2", dir=file_dir))
    taps = design.ff3_legacy_remez()
    coefs = design.legacy_ff3v_coefs(taps)
    lim_16k, lim_48k = thdn_limits[test_freq]
    # Expected gain at the test frequency: one pass through the filter down, two for the round trip
    filter_db = 20 * np.log10(np.abs(signal.freqz(taps, worN=[test_freq], fs=fsup)[1][0]))

    with tmp_dir(working_dir):
        sig48k = gen_sig(test_freq)

        py16k = model_ds3(sig48k, coefs)
        py48k = model_us3(py16k, coefs)
        check_thdn("PY 16k", py16k, fsdown, test_freq, lim_16k)
        check_thdn("PY 48k", py48k, fsup, test_freq, lim_48k)
        check_gain("PY 16k", py16k, fsdown, sig48k, filter_db)
        check_gain("PY 48k", py48k, fsup, sig48k, 2 * filter_db)

        c16k, c48k = run_c(xe)
        check_thdn("C  16k", c16k, fsdown, test_freq, lim_16k)
        check_thdn("C  48k", c48k, fsup, test_freq, lim_48k)
        check_gain("C  16k", c16k, fsdown, sig48k, filter_db)
        check_gain("C  48k", c48k, fsup, sig48k, 2 * filter_db)

        check_bit_exact(c16k, c48k, py16k, py48k)


@pytest.mark.main
@pytest.mark.parametrize("arch", ["xs2"])
def test_voice_ff3_xs2_us_saturation(arch):
    """
    Exercises the output saturation of src_us3_voice. The filter gain at 1 kHz is ~+0.43 dB,
    so a 0.93 full scale input stays below full scale after ds3 but goes above it after us3.
    The clipped output is only checked against the bit-exact model.
    """
    test_freq = 1000
    vol = 0.93
    file_dir = Path(__file__).parent
    xe = file_dir / "ff3_voice_test" / "bin" / arch / f"ff3_voice_test_{arch}.xe"
    working_dir = Path(tempfile.mkdtemp(prefix="test_voice_ff3_xs2_sat", dir=file_dir))
    coefs = design.legacy_ff3v_coefs(design.ff3_legacy_remez())
    int32_max = np.iinfo(np.int32).max
    int32_min = np.iinfo(np.int32).min

    with tmp_dir(working_dir):
        sig48k = gen_sig(test_freq, vol)

        py16k = model_ds3(sig48k, coefs)
        py48k = model_us3(py16k, coefs)
        num_sat = np.count_nonzero((py48k == int32_max) | (py48k == int32_min))
        print(f"16 kHz peak: {np.max(np.abs(py16k.astype(np.int64))) / 2**31:.4f} FS, "
              f"48 kHz saturated samples: {num_sat}")
        # Make sure the test signal still saturates us3, and only us3
        assert np.max(np.abs(py16k.astype(np.int64))) < int32_max, "ds3 output saturated, lower the input level"
        assert num_sat > 0, "us3 output did not saturate, raise the input level"

        c16k, c48k = run_c(xe)
        check_bit_exact(c16k, c48k, py16k, py48k)

