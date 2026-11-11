// Copyright 2024-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

    .text
    .align  2
    .globl  vx4_mul_s32
    .type   vx4_mul_s32, @function
vx4_mul_s32:
    mul     a2, a0, a1
    mulh    a1, a0, a1
    mv      a0, a2
    ret
    .size   vx4_mul_s32, .-vx4_mul_s32