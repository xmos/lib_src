// Copyright 2024-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef INLINE_ASM_VX4B_H_
#define INLINE_ASM_VX4B_H_

#define ASRC_TEST_GETTIME(value) asm volatile("xm.gettime %0" : "=r" (value))
#define ASRC_TEST_SPLINE_PHASE0 (&iADFirCoefs[3][15])
#define ASRC_TEST_SPLINE(piPhase, iH, coefs, taps) \
    src_mrhf_spline_coeff_gen_inner_loop_asm(piPhase, iH, coefs, taps)
#define ASRC_TEST_HAS_VPU_FIR 0

#endif