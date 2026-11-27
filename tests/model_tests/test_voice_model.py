# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
"""
Tests for the voice SRC Python models (lib_src.voice). These run on the host
only. The C implementations are tested against these models in sim_tests.
"""
import numpy as np
import pytest

from lib_src.voice import design, ff3, rat


def resample_ref(x, taps, up, down, offset):
    """
    Reference rational resampler: zero-stuff by ``up``, filter with ``taps``
    scaled by ``up``, keep every ``down``-th sample starting at ``offset``.
    """
    u = np.zeros(len(x) * up)
    u[::up] = x
    y = up * np.convolve(u, taps)[:len(u)]
    return y[offset::down]


# (model class, taps, up, down, offset into the reference output)
MODELS = {
    "ff3_ds_96t": (ff3.ff3_ds, design.ff3_kaiser(96), 1, 3, 2),
    "ff3_us_96t": (ff3.ff3_us, design.ff3_kaiser(96), 3, 1, 0),
    "ff3_ds_72t": (ff3.ff3_ds, design.ff3_kaiser(72), 1, 3, 2),
    "ff3_us_72t": (ff3.ff3_us, design.ff3_kaiser(72), 3, 1, 0),
    "ff3_ds_legacy": (ff3.ff3_ds, design.ff3_legacy_remez(), 1, 3, 2),
    "ff3_us_legacy": (ff3.ff3_us, design.ff3_legacy_remez(), 3, 1, 0),
    "rat_2_3_ds": (rat.rat_2_3_ds, design.rat_kaiser(), 2, 3, 2),
    "rat_3_2_us": (rat.rat_3_2_us, design.rat_kaiser(), 3, 2, 0),
}


@pytest.mark.parametrize("num_taps", [72, 96])
def test_ff3_design_meets_spec(num_taps):
    design.check_response(design.ff3_kaiser(num_taps, check=False), design.FF3_FS,
                          design.FF3_RESPONSE_BOUNDS)


def test_rat_design_meets_spec():
    design.check_response(design.rat_kaiser(check=False), design.RAT_FS,
                          design.RAT_RESPONSE_BOUNDS)


@pytest.mark.parametrize("num_phases", [2, 3])
@pytest.mark.parametrize("reverse", [False, True])
def test_polyphase_roundtrip(num_phases, reverse):
    taps = np.arange(96, dtype=float)
    phases = design.polyphase(taps, num_phases, reverse)
    assert phases.shape == (num_phases, 96 // num_phases)
    if reverse:
        phases = phases[::-1]
    np.testing.assert_array_equal(phases.T.reshape(-1), taps)


@pytest.mark.parametrize("name", MODELS.keys())
def test_model_matches_reference(name):
    cls, taps, up, down, offset = MODELS[name]
    model = cls(taps)
    rng = np.random.default_rng(0)
    x = rng.uniform(-0.5, 0.5, model.n_in * 200)

    y = model.process(x)
    y_ref = resample_ref(x, taps, up, down, offset)

    assert len(y) == len(x) * model.n_out // model.n_in
    np.testing.assert_allclose(y, y_ref, rtol=0, atol=1e-12)


@pytest.mark.parametrize("name", MODELS.keys())
def test_model_reset_state(name):
    cls, taps, *_ = MODELS[name]
    model = cls(taps)
    x = np.random.default_rng(1).uniform(-0.5, 0.5, model.n_in * 50)
    y0 = model.process(x)
    model.reset_state()
    np.testing.assert_array_equal(model.process(x), y0)


@pytest.mark.parametrize("name", [n for n in MODELS if "legacy" not in n])
def test_model_dc_gain(name):
    cls, taps, *_ = MODELS[name]
    model = cls(taps)
    y = model.process(np.full(model.n_in * 100, 0.5))
    # once the filter has settled, every output phase must have unity gain
    settled = y[len(taps):]
    np.testing.assert_allclose(settled, 0.5, rtol=0, atol=0.5 * 1e-3)
