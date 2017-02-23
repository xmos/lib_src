#!/usr/bin/env python
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

def plot_response_passband(fs, w, h, title):
    plt.figure()
    plt.plot(0.5*fs*w/np.pi, 20*np.log10(np.abs(h)))
    plt.ylim(-1, 1)
    plt.xlim(0, 0.5*fs)
    plt.grid(True)
    plt.xlabel('Frequency (Hz)')
    plt.ylabel('Gain (dB)')
    plt.title(title)

def generate_header_file(num_taps_per_phase, num_phases):
    header_template = """\
#ifndef _SRC_FF3V_FIR_H_
#define _SRC_FF3V_FIR_H_

#include <stdint.h>

#define SRC_FF3V_FIR_NUM_PHASES (%(phases)s)
#define SRC_FF3V_FIR_TAPS_PER_PHASE (%(taps_per_phase)s)

extern const unsigned src_ff3v_fir_comp_q;
extern const int32_t src_ff3v_fir_comp;

extern int32_t src_ff3v_fir_coefs_debug[SRC_FF3V_FIR_NUM_PHASES * SRC_FF3V_FIR_TAPS_PER_PHASE];
extern const int32_t src_ff3v_fir_coefs[SRC_FF3V_FIR_NUM_PHASES][SRC_FF3V_FIR_TAPS_PER_PHASE];

#endif // _SRC_FF3V_FIR_H_
"""

    header_path = os.path.join(os.path.dirname(os.path.realpath(__file__)),
                               'src_ff3v_fir.h')
    with open(header_path, "w") as header_file:
        header_file.writelines(header_template % {'taps_per_phase':num_taps_per_phase,
                                                  'phases':num_phases})

def generate_xc_file(q, pass_band_atten, taps):
    xc_template = """\
#include "src_ff3v_fir.h"
#include <stdint.h>

/** Used for FIR compensation.
 *
 *  Generated using src_ff3v_fir_generator.py
 */
const unsigned src_ff3v_fir_comp_q = %(comp_q)s;

/** Used for FIR compensation.
 *
 *  Generated using src_ff3v_fir_generator.py
 */
const int32_t src_ff3v_fir_comp = %(comp)s;

/** Used for self testing src_ds3_voice and src_us3_voice functionality
 *
 *  Generated using src_ff3v_fir_generator.py
 */
int32_t src_ff3v_fir_coefs_debug[SRC_FF3V_FIR_NUM_PHASES * SRC_FF3V_FIR_TAPS_PER_PHASE] = {
   %(coefs_debug)s
};

/** Coefficients for use with src_ds3_voice and src_us3_voice functions
 *
 *  Generated using src_ff3v_fir_generator.py
 */
const int32_t src_ff3v_fir_coefs[SRC_FF3V_FIR_NUM_PHASES][SRC_FF3V_FIR_TAPS_PER_PHASE] = {
%(coefs)s};
"""

    coefs_debug = ''
    i = 1
    for c in taps:
        coefs_debug += (' ' + str(int(c*(2**31 - 1))) + ',')
        if ((i % 7) == 0):
            coefs_debug += '\n   '
        i+=1

    coefs = ''
    i = 1
    for step in range(2, -1, -1):
        coefs += '    {'
        for j in range(step, len(taps), 3):
            coefs += ' ' + str(int(taps[j]*(2**31 - 1))) + ','
            if ((i % 7) == 0):
                coefs += '\n    '
            i+=1
        coefs += ' },\n'
        i = 0

    xc_path = os.path.join(os.path.dirname(os.path.realpath(__file__)),
                               'src_ff3v_fir.xc')
    with open(xc_path, "w") as xc_file:
        xc_file.writelines(xc_template % {'comp_q':str(q),
                                          'comp':str(int(((2**q)-1) * pass_band_atten)),
                                          'coefs_debug':coefs_debug,
                                          'coefs':coefs})


# Low-pass filter design parameters
fs = 48000.0        # Sample rate, Hz
num_phases = 3
num_taps_per_phase = 12*2
# num_phases*num_taps_per_phase = Size of the FIR filter.

taps = signal.remez((num_phases*num_taps_per_phase), [0, 7300, 8700, 0.5*fs], [1, 0], [.008, 1], Hz=fs)
w, h = signal.freqz(taps)

pass_band_atten = sum(abs(taps))

taps = taps / pass_band_atten    # Guarantee no overflow

q = 31 -int(np.log2(pass_band_atten) + 0.5)


if __name__ == "__main__":
    # plot_response(fs, w, h, "Low-pass Filter")
    # plot_response_passband(fs, w, h, "Low-pass Filter")
    generate_header_file(num_taps_per_phase, num_phases)
    generate_xc_file(q, pass_band_atten, taps)

