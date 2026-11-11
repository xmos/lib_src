// Copyright 2024-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

// Portable C implementations of the scalar multirate HiFi inner loops for
// the VX4 (xcore-400) architecture.  On XS2/XS3 these entry points are
// provided by the dual-issue assembly in src_mrhf_*_inner_loop_asm.S; the
// implementations below replicate that arithmetic exactly (64-bit MACC,
// saturating extract).
//
// On VX4 the VPU data layouts are selected by default (see use_vpu.h), so
// the ASRC/SSRC processing paths mostly use the VPU implementations in
// src_mrhf_inner_loops_asm_vx4b.S.  These scalar versions remain required
// by the PPFIR polyphase path (SSRC 44.1k<->48k family conversions) and
// ADFIR_proc_macc, which have no VPU dispatch, and they serve as reference
// implementations for the architecture self-tests.

#if defined(__VX4B__)

#include <limits.h>
#include <stdint.h>

#include "src_mrhf_adfir_inner_loop_asm.h"
#include "src_mrhf_fir_inner_loop_asm.h"
#include "src_mrhf_fir_os_inner_loop_asm.h"
#include "src_mrhf_spline_coeff_gen_inner_loop_asm.h"

static int32_t saturate_shift(int64_t accumulator, unsigned shift)
{
    const int64_t scale = INT64_C(1) << shift;
    const int64_t maximum = (int64_t)INT32_MAX * scale;
    const int64_t minimum = (int64_t)INT32_MIN * scale;

    if (accumulator > maximum) {
        return INT32_MAX;
    }
    if (accumulator < minimum) {
        return INT32_MIN;
    }
    return (int32_t)(accumulator >> shift);
}

void src_mrhf_adfir_inner_loop_asm(int *data, int *coefs, int result[], int count)
{
    int64_t accumulator = 0;
    unsigned terms = (unsigned)count * 2U;

    for (unsigned term = 0; term < terms; ++term) {
        accumulator += (int64_t)data[term] * coefs[term];
    }
    result[0] = saturate_shift(accumulator, 30);
}

void src_mrhf_adfir_inner_loop_asm_odd(int *data, int *coefs, int result[], int count)
{
    src_mrhf_adfir_inner_loop_asm(data, coefs, result, count);
}

void src_mrhf_fir_inner_loop_asm(int *data, int *coefs, int result[], int count)
{
    int64_t accumulator = 0;
    unsigned terms = (unsigned)count * 2U;

    for (unsigned term = 0; term < terms; ++term) {
        accumulator += (int64_t)data[term] * coefs[term];
    }
    result[0] = saturate_shift(accumulator, 31);
}

void src_mrhf_fir_inner_loop_asm_odd(int *data, int *coefs, int result[], int count)
{
    src_mrhf_fir_inner_loop_asm(data, coefs, result, count);
}

void src_mrhf_fir_os_inner_loop_asm(int *data, int *coefs, int result[], int count)
{
    int64_t accumulators[2] = {0, 0};
    unsigned iterations = (unsigned)count >> 2;

    for (unsigned iteration = 0; iteration < iterations; ++iteration) {
        for (unsigned sample = 0; sample < 8; ++sample) {
            unsigned data_index = iteration * 8 + sample;
            unsigned coef_index = iteration * 16 + sample * 2;
            accumulators[0] += (int64_t)data[data_index] * coefs[coef_index];
            accumulators[1] += (int64_t)data[data_index] * coefs[coef_index + 1];
        }
    }
    result[0] = saturate_shift(accumulators[0], 31);
    result[1] = saturate_shift(accumulators[1], 31);
}

void src_mrhf_fir_os_inner_loop_asm_odd(int *data, int *coefs, int result[], int count)
{
    src_mrhf_fir_os_inner_loop_asm(data, coefs, result, count);
}

void src_mrhf_spline_coeff_gen_inner_loop_asm(int *phase, int *weights, int *coefs, int taps)
{
    for (int tap = 0; tap < taps; ++tap) {
        int64_t accumulator = 0;
        accumulator += (int64_t)phase[tap] * weights[2];
        accumulator += (int64_t)phase[taps + tap] * weights[1];
        accumulator += (int64_t)phase[2 * taps + tap] * weights[0];
        coefs[tap] = (int32_t)(uint32_t)((uint64_t)accumulator >> 32);
    }
}

#endif // defined(__VX4B__)
