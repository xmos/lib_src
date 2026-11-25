# Copyright 2023-2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
"""
Filter design, polyphase decomposition and quantisation for the voice SRCs.

Each prototype filter is designed at the higher (interpolated) sample rate and
has unity DC gain. The polyphase layouts returned here match the order in which
the C implementations index their coefficient arrays.
"""
import numpy as np
from scipy import signal

FF3_FS = 48000
FF3_NUM_PHASES = 3
FF3_TAPS_PER_PHASE = 32

RAT_FS = 96000
RAT_NUM_TAPS = 96
RAT_NUM_PHASES_DS = 2
RAT_NUM_PHASES_US = 3

LEGACY_FF3V_NUM_PHASES = 3
LEGACY_FF3V_TAPS_PER_PHASE = 24

# (frequency Hz, min dB or None, max dB) points the designed responses must meet
FF3_RESPONSE_BOUNDS = [
    (10500, None, -6),
    (7500, -2, 0.05),
    (6300, -0.05, 0.05),
    (5000, -0.05, 0.05),
    (200, -0.05, 0.05),
    (180, -0.05, 0.05),
    (100, -0.05, 0.05),
]

RAT_RESPONSE_BOUNDS = [
    (16000, None, -6),
    (15000, -2, 0.05),
    (14000, -0.05, 0.05),
    (12500, -0.05, 0.05),
    (5000, -0.05, 0.05),
    (200, -0.05, 0.05),
    (80, -0.05, 0.05),
]


def check_response(taps, fs, bounds):
    """
    Assert that the magnitude response of ``taps`` meets ``bounds``.

    Each bound is checked at the closest frequency bin of a 512 point
    ``scipy.signal.freqz``.
    """
    w, h = signal.freqz(taps)
    mag_db = 20 * np.log10(np.abs(h))
    freqs = 0.5 * fs * w / np.pi

    for freq, min_db, max_db in bounds:
        idx = np.abs(freqs - freq).argmin()
        assert mag_db[idx] < max_db, \
            f"{mag_db[idx]:.3f} dB at {freqs[idx]:.0f} Hz is above {max_db} dB"
        if min_db is not None:
            assert mag_db[idx] > min_db, \
                f"{mag_db[idx]:.3f} dB at {freqs[idx]:.0f} Hz is below {min_db} dB"


def ff3_kaiser(num_taps=FF3_NUM_PHASES * FF3_TAPS_PER_PHASE, check=True):
    """8 kHz low-pass prototype for the 48 <-> 16 kHz VPU SRC (Kaiser, beta 4)."""
    taps = signal.firwin2(num_taps, [0, 7600, 8400, 0.5 * FF3_FS], [1, 1, 0, 0],
                          window=("kaiser", 4), fs=FF3_FS)
    if check:
        check_response(taps, FF3_FS, FF3_RESPONSE_BOUNDS)
    return taps


def ff3_legacy_remez(num_taps=LEGACY_FF3V_NUM_PHASES * LEGACY_FF3V_TAPS_PER_PHASE):
    """
    8 kHz low-pass prototype used by the deprecated src_ds3_voice/src_us3_voice
    functions (Remez, 72 taps). Kept to compare against the newer designs.
    """
    return signal.remez(num_taps, [0, 7300, 8700, 0.5 * FF3_FS], [1, 0], weight=[.008, 1],
                        fs=FF3_FS)


def rat_kaiser(num_taps=RAT_NUM_TAPS, check=True):
    """16 kHz low-pass prototype for the 48 <-> 32 kHz VPU SRC (Kaiser, beta 3.2)."""
    taps = signal.firwin2(num_taps, [0, 15000, 17000, 0.5 * RAT_FS], [1, 1, 0, 0],
                          window=("kaiser", 3.2), fs=RAT_FS)
    if check:
        check_response(taps, RAT_FS, RAT_RESPONSE_BOUNDS)
    return taps


def polyphase(taps, num_phases, reverse=False):
    """
    Split ``taps`` into ``num_phases`` sub-filters, ``phases[p][t] = taps[t * num_phases + p]``.

    With ``reverse=True`` the phase order is reversed, i.e.
    ``phases[p][t] = taps[t * num_phases + (num_phases - 1 - p)]``. This is the
    layout used by the fixed factor of 3 implementations.
    """
    taps = np.asarray(taps)
    assert len(taps) % num_phases == 0, "number of taps must be a multiple of num_phases"
    phases = taps.reshape(-1, num_phases).T
    if reverse:
        phases = phases[::-1]
    return np.ascontiguousarray(phases)


def to_fixed(x, q):
    """Quantise to int32 with ``q`` fractional bits, rounding toward zero."""
    return (np.asarray(x) * 2.0 ** q).astype(np.int32)


def ff3_coefs_q30(taps, num_phases=FF3_NUM_PHASES):
    """Q30 polyphase coefficients for the VPU ff3 SRC, ``[num_phases][taps_per_phase]``."""
    # The coefficients have always been rounded to float32 before quantising.
    # This is kept so the generated tables do not change.
    phases = polyphase(taps, num_phases, reverse=True).astype(np.float32)
    return to_fixed(phases, 30)


def rat_coefs_q30(taps, num_phases):
    """Q30 polyphase coefficients for the VPU rational factor SRC."""
    return to_fixed(polyphase(taps, num_phases), 30)


def legacy_ff3v_coefs(taps, num_phases=LEGACY_FF3V_NUM_PHASES):
    """
    Coefficients and output compensation for the deprecated ff3v functions.

    The taps are normalised by sum(|taps|) so the accumulator can never exceed
    full scale. The output is then scaled back up by ``comp_*`` and shifted
    right by ``comp_q_*`` with saturation.

    Returns a dict with ``coefs_debug`` (flat Q31), ``coefs``
    (``[num_phases][taps_per_phase]`` Q31, reversed phase order) and the
    ``comp_ds``, ``comp_q_ds``, ``comp_us`` and ``comp_q_us`` constants.
    """
    pass_band_atten = np.sum(np.abs(taps))
    upsampling_atten = num_phases
    scaled = np.asarray(taps) / pass_band_atten

    q_ds = 30 - int(np.log2(pass_band_atten) + 0.5)
    q_us = 30 - int(np.log2(pass_band_atten * upsampling_atten) + 0.5)

    def q31(x):
        return np.array([int(c * (2**31 - 1)) for c in np.ravel(x)], np.int64).reshape(np.shape(x))

    return {
        "coefs_debug": q31(scaled),
        "coefs": q31(polyphase(scaled, num_phases, reverse=True)),
        "comp_q_ds": q_ds,
        "comp_ds": int(((2**q_ds) - 1) * pass_band_atten),
        "comp_q_us": q_us,
        "comp_us": int(((2**q_us) - 1) * pass_band_atten * upsampling_atten),
    }
