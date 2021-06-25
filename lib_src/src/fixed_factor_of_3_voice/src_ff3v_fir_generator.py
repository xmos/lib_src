#!/usr/bin/env python
# Copyright 2016-2021 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import os.path
import numpy as np
from scipy import signal
import matplotlib.pyplot as plt


def plot_response(fs, w, h, title):
    plt.figure()
    plt.plot(0.5*fs*w/np.pi, 20*np.log10(np.abs(h)))

    plt.xlim(0, 0.5*fs)
    plt.grid(True)
    plt.xlabel('Frequency (Hz)')
    plt.ylabel('Gain (dB)')
    plt.title(title)
    plt.show()

def plot_response_passband(fs, w, h, title):
    plt.figure()
    plt.plot(0.5*fs*w/np.pi, 20*np.log10(np.abs(h)))
    plt.ylim(-1, 1)
    plt.xlim(0, 0.25*fs)
    plt.grid(True)
    plt.xlabel('Frequency (Hz)')
    plt.ylabel('Gain (dB)')
    plt.title(title)
    plt.show()

def generate_header_file(num_taps_per_phase, num_phases):
    header_template = """\
// Copyright (c) 2016-2021, XMOS Ltd, All rights reserved
//
// This file is generated using src_ff3v_fir_generator.py
//
// Do not modify it directly, changes may be overwritten!
//

#ifndef _SRC_FF3V_FIR_H_
#define _SRC_FF3V_FIR_H_

#include <stdint.h>

#define SRC_FF3V_FIR_NUM_PHASES (%(phases)s)
#define SRC_FF3V_FIR_TAPS_PER_PHASE (%(taps_per_phase)s)

extern const unsigned src_ff3v_fir_comp_q_ds;
extern const int32_t src_ff3v_fir_comp_ds;

extern const unsigned src_ff3v_fir_comp_q_us;
extern const int32_t src_ff3v_fir_comp_us;

extern int32_t src_ff3v_fir_coefs_debug[SRC_FF3V_FIR_NUM_PHASES * SRC_FF3V_FIR_TAPS_PER_PHASE];

#if defined(__XC__)
extern const int32_t (*src_ff3v_fir_coefs_xc)[SRC_FF3V_FIR_TAPS_PER_PHASE];
#define src_ff3v_fir_coefs src_ff3v_fir_coefs_xc
#else
extern const int32_t (*src_ff3v_fir_coefs_c)[SRC_FF3V_FIR_TAPS_PER_PHASE];
#define src_ff3v_fir_coefs src_ff3v_fir_coefs_c
#endif

#endif // _SRC_FF3V_FIR_H_
"""

    header_path = os.path.join(os.path.dirname(os.path.realpath(__file__)),
                               'src_ff3v_fir.h')
    with open(header_path, "w") as header_file:
        header_file.writelines(header_template % {'taps_per_phase':num_taps_per_phase,
                                                  'phases':num_phases})

def generate_xc_file(q_ds, q_us, comp_ds, comp_us, taps):
    xc_template = """\
// Copyright (c) 2016-2021, XMOS Ltd, All rights reserved
//
// This file is generated using src_ff3v_fir_generator.py
//
// Do not modify it directly, changes may be overwritten!
//

#include "src_ff3v_fir.h"
#include <stdint.h>

/** Used for FIR compensation for decimation*/
const unsigned src_ff3v_fir_comp_q_ds = %(comp_q_ds)s;

/** Used for FIR compensation for decimation*/
const int32_t src_ff3v_fir_comp_ds = %(comp_ds)s;

/** Used for FIR compensation for upsampling*/
const unsigned src_ff3v_fir_comp_q_us = %(comp_q_us)s;

/** Used for FIR compensation for upsampling*/
const int32_t src_ff3v_fir_comp_us = %(comp_us)s;

/** Used for self testing src_ds3_voice and src_us3_voice functionality */
int32_t src_ff3v_fir_coefs_debug[SRC_FF3V_FIR_NUM_PHASES * SRC_FF3V_FIR_TAPS_PER_PHASE] = {
%(coefs_debug)s
};

/** Coefficients for use with src_ds3_voice and src_us3_voice functions */
static const int32_t [[aligned(8)]] src_ff3v_fir_coefs_i[SRC_FF3V_FIR_NUM_PHASES][SRC_FF3V_FIR_TAPS_PER_PHASE] = {
%(coefs)s};

unsafe {
    const int32_t (* unsafe src_ff3v_fir_coefs_c)[SRC_FF3V_FIR_TAPS_PER_PHASE] = src_ff3v_fir_coefs_i;
}

const int32_t (*src_ff3v_fir_coefs_xc)[SRC_FF3V_FIR_TAPS_PER_PHASE] = src_ff3v_fir_coefs_i;
"""

    coefs_debug = ''
    i = 1
    for c in taps:
        coefs_debug += (' ' + str(int(c*(2**31 - 1))).rjust(12) + ',')
        if ((i % 6) == 0):
            coefs_debug += '\n'
        i+=1

    coefs = ''

    for step in range(num_phases-1, -1, -1):
        i = 1
        coefs += '    {\n    '
        for j in range(step, len(taps), num_phases):
            coefs += ' ' + str(int(taps[j]*(2**31 - 1))).rjust(12)  + ','
            if ((i % 6) == 0):
                coefs += '\n    '
            i+=1
        coefs += '},\n'

    xc_path = os.path.join(os.path.dirname(os.path.realpath(__file__)),
                               'src_ff3v_fir.xc')
    with open(xc_path, "w") as xc_file:

        xc_file.writelines(xc_template % {'comp_q_ds':str(q_ds),
                                          'comp_ds':str(comp_ds),
                                          'comp_q_us':str(q_us),
                                          'comp_us':str(comp_us),
                                          'coefs_debug':coefs_debug,
                                          'coefs':coefs})

# Low-pass filter design parameters
fs = 48000.0        # Sample rate, Hz
num_phases = 3
num_taps_per_phase = 2*12

taps = signal.remez((num_phases*num_taps_per_phase), [0, 7300, 8700, 0.5*fs], [1, 0], [.008, 1], Hz=fs)
w, h = signal.freqz(taps)

pass_band_atten =  sum(abs(taps))
upsampling_atten = num_phases
taps = taps / pass_band_atten    # Guarantee no overflow

# generate the compensation factors
q_ds = 30 - int(np.log2(pass_band_atten) + 0.5)
q_us = 30 - int(np.log2(pass_band_atten * upsampling_atten) + 0.5)
comp_ds = int(((2**q_ds)-1) * pass_band_atten)
comp_us = int(((2**q_us)-1) * pass_band_atten * upsampling_atten)

if __name__ == "__main__":
    # plot_response(fs, w, h, "Low-pass Filter")
    # plot_response_passband(fs, w, h, "Low-pass Filter")
    generate_header_file(num_taps_per_phase, num_phases)
    generate_xc_file(q_ds, q_us, comp_ds, comp_us, taps)

