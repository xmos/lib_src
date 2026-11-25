# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
"""Common base for the block-based voice SRC models."""
import numpy as np


class src_block:
    """
    Base class for a synchronous SRC that consumes ``n_in`` samples and
    produces ``n_out`` samples per call, like the C API.

    Subclasses implement ``reset_state()`` and ``process_block()``.
    """

    n_in = 1
    n_out = 1

    def __init__(self, taps):
        self.taps = np.asarray(taps, dtype=np.float64)
        self.reset_state()

    def reset_state(self):
        raise NotImplementedError

    def process_block(self, samples):
        """Process ``n_in`` float samples and return ``n_out`` float samples."""
        raise NotImplementedError

    def process(self, sig):
        """Process a whole signal. Its length must be a multiple of ``n_in``."""
        sig = np.asarray(sig, dtype=np.float64)
        assert len(sig) % self.n_in == 0, f"signal length must be a multiple of {self.n_in}"
        n_blocks = len(sig) // self.n_in
        out = np.zeros(n_blocks * self.n_out)
        for b in range(n_blocks):
            out[b * self.n_out:(b + 1) * self.n_out] = \
                self.process_block(sig[b * self.n_in:(b + 1) * self.n_in])
        return out


def push(state, sample):
    """Shift ``state`` right by one and insert ``sample`` at index 0 (newest first)."""
    state[1:] = state[:-1]
    state[0] = sample
