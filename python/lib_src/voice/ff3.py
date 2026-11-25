# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
"""
Float models of the fixed factor of 3 voice SRCs (src_ff3_96t_ds/us and the
72 tap variants).

The models follow the structure of ``src_poly.h``: the prototype filter is
split into three phases in reversed order (see ``design.polyphase``) and the
state is kept newest-sample-first.
"""
import numpy as np

from . import design
from .model import src_block, push


class ff3_ds(src_block):
    """Downsample by 3. Three samples in, one sample out per block."""

    n_in = 3
    n_out = 1

    def __init__(self, taps=None):
        if taps is None:
            taps = design.ff3_kaiser()
        self.num_phases = design.FF3_NUM_PHASES
        self.coefs = design.polyphase(taps, self.num_phases, reverse=True)
        super().__init__(taps)

    def reset_state(self):
        self.state = np.zeros(self.coefs.shape)

    def process_block(self, samples):
        acc = 0.0
        for p in range(self.num_phases):
            push(self.state[p], samples[p])
            acc += np.dot(self.state[p], self.coefs[p])
        return np.array([acc])


class ff3_us(src_block):
    """Upsample by 3. One sample in, three samples out per block."""

    n_in = 1
    n_out = 3

    def __init__(self, taps=None):
        if taps is None:
            taps = design.ff3_kaiser()
        self.num_phases = design.FF3_NUM_PHASES
        self.coefs = design.polyphase(taps, self.num_phases, reverse=True)
        super().__init__(taps)

    def reset_state(self):
        self.state = np.zeros(self.coefs.shape[1])

    def process_block(self, samples):
        push(self.state, samples[0])
        # Phases are stored in reversed order, so output m uses coefs[2 - m].
        # The gain of 3 restores the energy lost by zero-stuffing.
        return np.array([np.dot(self.state, self.coefs[self.num_phases - 1 - m])
                         for m in range(self.num_phases)]) * self.num_phases
