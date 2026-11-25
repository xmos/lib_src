# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
"""
Float models of the rational factor 3/2 voice SRCs (src_rat_2_3_96t_ds and
src_rat_3_2_96t_us).

The models follow the structure of ``src_poly.h``: a single newest-first state
shared by all phases, and ``phases[p][t] = taps[t * num_phases + p]``.
"""
import numpy as np

from . import design
from .model import src_block, push


class rat_2_3_ds(src_block):
    """Resample by 2/3 (48 -> 32 kHz). Three samples in, two samples out per block."""

    n_in = 3
    n_out = 2

    def __init__(self, taps=None):
        if taps is None:
            taps = design.rat_kaiser()
        self.coefs = design.polyphase(taps, design.RAT_NUM_PHASES_DS)
        super().__init__(taps)

    def reset_state(self):
        self.state = np.zeros(self.coefs.shape[1])

    def process_block(self, samples):
        # The gain of 2 restores the energy lost by zero-stuffing.
        push(self.state, samples[0])
        push(self.state, samples[1])
        out0 = np.dot(self.state, self.coefs[0])
        push(self.state, samples[2])
        out1 = np.dot(self.state, self.coefs[1])
        return np.array([out0, out1]) * design.RAT_NUM_PHASES_DS


class rat_3_2_us(src_block):
    """Resample by 3/2 (32 -> 48 kHz). Two samples in, three samples out per block."""

    n_in = 2
    n_out = 3

    def __init__(self, taps=None):
        if taps is None:
            taps = design.rat_kaiser()
        self.coefs = design.polyphase(taps, design.RAT_NUM_PHASES_US)
        super().__init__(taps)

    def reset_state(self):
        self.state = np.zeros(self.coefs.shape[1])

    def process_block(self, samples):
        # The gain of 3 restores the energy lost by zero-stuffing.
        push(self.state, samples[0])
        out0 = np.dot(self.state, self.coefs[0])
        out1 = np.dot(self.state, self.coefs[2])
        push(self.state, samples[1])
        out2 = np.dot(self.state, self.coefs[1])
        return np.array([out0, out1, out2]) * design.RAT_NUM_PHASES_US
