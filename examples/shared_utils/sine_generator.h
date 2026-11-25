// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef SINE_GENERATOR_H_
#define SINE_GENERATOR_H_

/* Generate interleaved 1 kHz harmonics at 90% full scale; return zero on success. */
int sine_generate(int samples[], unsigned frames, unsigned channels, unsigned sample_rate);

#endif
