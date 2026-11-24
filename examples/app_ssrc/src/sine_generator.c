// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "sine_generator.h"

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_SINE_SAMPLES 221

static unsigned gcd(unsigned a, unsigned b)
{
    while (b != 0) {
        unsigned remainder = a % b;
        a = b;
        b = remainder;
    }
    return a;
}

int sine_generate(int samples[], unsigned frames, unsigned channels, unsigned sample_rate)
{
    if (sample_rate == 0 || channels == 0 || (frames != 0 && samples == NULL) ||
        (frames != 0 && channels > UINT_MAX / frames)) {
        printf("Error: invalid sine generator arguments\n");
        return 1;
    }

    unsigned divisor = gcd(sample_rate, 1000);
    unsigned period = sample_rate / divisor;
    if (period > MAX_SINE_SAMPLES * 4) {
        printf("Error: sine lookup table is too small for %u Hz\n", sample_rate);
        return 1;
    }
    /* An even phase grid makes reflection exact even when the sample period is odd. */
    unsigned grid_scale = period % 2 ? 2 : 1;
    unsigned phase_period = period * grid_scale;
    unsigned phase_step = ((1000 / divisor) * grid_scale) % phase_period;
    unsigned table_size = phase_period / 4 + 1;
    if (table_size > MAX_SINE_SAMPLES) {
        printf("Error: sine lookup table is too small for %u Hz\n", sample_rate);
        return 1;
    }
    int sine_table[MAX_SINE_SAMPLES];
    for (unsigned i = 0; i < table_size; ++i) {
        sine_table[i] = (int)(0.9 * INT32_MAX * sin(2.0 * 3.141592653589793 * i / phase_period));
    }

    unsigned base_phase = 0;
    unsigned half_period = phase_period / 2;
    for (unsigned frame = 0; frame < frames; ++frame) {
        unsigned phase = 0;
        for (unsigned channel = 0; channel < channels; ++channel) {
            phase += base_phase;
            if (phase >= phase_period) {
                phase -= phase_period;
            }
            int negative = phase >= half_period;
            unsigned index = negative ? phase - half_period : phase;
            if (index > half_period / 2) {
                index = half_period - index;
            }
            samples[frame * channels + channel] = negative ? -sine_table[index] : sine_table[index];
        }
        base_phase += phase_step;
        if (base_phase >= phase_period) {
            base_phase -= phase_period;
        }
    }
    return 0;
}
