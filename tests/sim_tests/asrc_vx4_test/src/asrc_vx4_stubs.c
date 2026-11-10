// Copyright 2024-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "src_mrhf_adfir_inner_loop_asm.h"
#include "src_mrhf_fir_inner_loop_asm.h"
#include "src_mrhf_fir_os_inner_loop_asm.h"
#include "src_mrhf_spline_coeff_gen_inner_loop_asm.h"
#include <basix/architecture.h>

void delay_milliseconds(unsigned milliseconds)
{
    uint64_t ticks = (uint64_t)milliseconds * BASIX_TIMER_HZ / 1000U;
    basix_delay_ticks((uint32_t)ticks);
}

void src_mrhf_adfir_inner_loop_asm(int *data, int *coefs, int result[], int count)
{
    (void)data;
    (void)coefs;
    (void)count;
    result[0] = 0;
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
    (void)data;
    (void)coefs;
    (void)count;
    result[0] = 0;
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
    (void)data;
    (void)coefs;
    (void)count;
    result[0] = 0;
    result[1] = 0;
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
    (void)phase;
    (void)weights;
    for (int tap = 0; tap < taps; ++tap) {
        coefs[tap] = 0;
    }
}

void src_mrhf_spline_coeff_gen_inner_loop_asm_xs3(int *phase, int *weights, int *coefs, int taps)
{
    src_mrhf_spline_coeff_gen_inner_loop_asm(phase, weights, coefs, taps);
}