// Copyright 2024-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "src_mrhf_adfir_inner_loop_asm.h"
#include "src_mrhf_fir_inner_loop_asm.h"
#include "src_mrhf_fir_os_inner_loop_asm.h"
#include "src_mrhf_spline_coeff_gen_inner_loop_asm.h"
#include <basix/architecture.h>

extern uint64_t vx4_mul_s32(int32_t left, int32_t right);

void delay_milliseconds(unsigned milliseconds)
{
    uint64_t ticks = (uint64_t)milliseconds * BASIX_TIMER_HZ / 1000U;
    basix_delay_ticks((uint32_t)ticks);
}

static int32_t vx4_saturate_shift(uint64_t accumulator, unsigned shift)
{
    int64_t signed_accumulator;
    const int64_t scale = INT64_C(1) << shift;
    const int64_t maximum = (int64_t)INT32_MAX * scale;
    const int64_t minimum = (int64_t)INT32_MIN * scale;

    memcpy(&signed_accumulator, &accumulator, sizeof(signed_accumulator));
    if (signed_accumulator > maximum) {
        return INT32_MAX;
    }
    if (signed_accumulator < minimum) {
        return INT32_MIN;
    }
    return (int32_t)(signed_accumulator >> shift);
}

void src_mrhf_adfir_inner_loop_asm(int *data, int *coefs, int result[], int count)
{
    uint64_t accumulator = 0;
    unsigned terms = (unsigned)count * 2U;

    for (unsigned term = 0; term < terms; ++term) {
        accumulator += vx4_mul_s32(data[term], coefs[term]);
    }
    result[0] = vx4_saturate_shift(accumulator, 30);
}

void src_mrhf_adfir_inner_loop_asm_odd(int *data, int *coefs, int result[], int count)
{
    src_mrhf_adfir_inner_loop_asm(data, coefs, result, count);
}

void src_mrhf_adfir_inner_loop_asm_xs3(int *data, int *coefs, int result[], int count)
{
    src_mrhf_adfir_inner_loop_asm(data, coefs, result, count);
}

void src_mrhf_fir_inner_loop_asm(int *data, int *coefs, int result[], int count)
{
    uint64_t accumulator = 0;
    unsigned terms = (unsigned)count * 2U;

    for (unsigned term = 0; term < terms; ++term) {
        accumulator += vx4_mul_s32(data[term], coefs[term]);
    }
    result[0] = vx4_saturate_shift(accumulator, 31);
}

void src_mrhf_fir_inner_loop_asm_odd(int *data, int *coefs, int result[], int count)
{
    src_mrhf_fir_inner_loop_asm(data, coefs, result, count);
}

void src_mrhf_fir_inner_loop_asm_xs3(int *data, int *coefs, int result[], int count)
{
    src_mrhf_fir_inner_loop_asm(data, coefs, result, count);
}

void src_mrhf_fir_os_inner_loop_asm(int *data, int *coefs, int result[], int count)
{
    uint64_t accumulators[2] = {0, 0};
    unsigned iterations = (unsigned)count >> 2;

    for (unsigned iteration = 0; iteration < iterations; ++iteration) {
        for (unsigned sample = 0; sample < 8; ++sample) {
            unsigned data_index = iteration * 8 + sample;
            unsigned coef_index = iteration * 16 + sample * 2;
            accumulators[0] += vx4_mul_s32(data[data_index], coefs[coef_index]);
            accumulators[1] += vx4_mul_s32(data[data_index], coefs[coef_index + 1]);
        }
    }
    result[0] = vx4_saturate_shift(accumulators[0], 31);
    result[1] = vx4_saturate_shift(accumulators[1], 31);
}

void src_mrhf_fir_os_inner_loop_asm_odd(int *data, int *coefs, int result[], int count)
{
    src_mrhf_fir_os_inner_loop_asm(data, coefs, result, count);
}

void src_mrhf_fir_os_inner_loop_asm_xs3(int *data, int *coefs, int result[], int count)
{
    src_mrhf_fir_os_inner_loop_asm(data, coefs, result, count);
}

void src_mrhf_spline_coeff_gen_inner_loop_asm(int *phase, int *weights, int *coefs, int taps)
{
    for (int tap = 0; tap < taps; ++tap) {
        uint64_t accumulator = 0;
        accumulator += vx4_mul_s32(phase[tap], weights[2]);
        accumulator += vx4_mul_s32(phase[taps + tap], weights[1]);
        accumulator += vx4_mul_s32(phase[2 * taps + tap], weights[0]);
        coefs[tap] = (int32_t)(uint32_t)(accumulator >> 32);
    }
}

void src_mrhf_spline_coeff_gen_inner_loop_asm_xs3(int *phase, int *weights, int *coefs, int taps)
{
    src_mrhf_spline_coeff_gen_inner_loop_asm(phase, weights, coefs, taps);
}