// Copyright 2016-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#if defined(__XS3A__)

    .section    .dp.data,"awd",@progbits
    .text
    .issue_mode dual

#define FILTER_DEFS_ADFIR_N_PHASES            128 
    .cc_top src_mrhf_spline_coeff_gen_inner_loop_asm_xs3.function
    .globl    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3
    .align    16
    .type    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3,@function

#define NSTACKWORDS        8

shift:
    .word 2, 2, 2, 2, 2, 2, 2, 2

src_mrhf_spline_coeff_gen_inner_loop_asm_xs3:
    DUALENTSP_lu6 NSTACKWORDS

    ldc r3, (FILTER_DEFS_ADFIR_N_PHASES + 2)<< 2
    // p0, p1, p2, p3, ..., p15
    // p0+N, p1+N, p2+N, ....,p15+N
    // p0+2N, p1+2N, p2+2N, ....,p15+2N
    // p0+3N, p1+3N, p2+3N, ....,p15+3N
    // ...
    // p0+257N, p1+257N, p2+257N, ....,p15+257N

    //           ****
    // p0, p0+N, p0+2N, ...., p0+257N
    // p1, p1+N, p1+2N, ...., p1+257N
    // ...
    
// a0 = p[0] * H[2] + p[0+N] * H[1] * p[0+2N] * H[0]
// a1 = p[1] * H[2] + p[1+N] * H[1] * p[1+2N] * H[0]
// a2 = p[2] * H[2] + p[2+N] * H[1] * p[2+2N] * H[0]
// a3 = p[3] * H[2] + p[3+N] * H[1] * p[3+2N] * H[0]

    // Ensure that H is padded with five zeroes.
    // There may be a faster way to achieve this with VLMUL
    
    { vldd  r1[0]                 ; ldaw  r1, sp[0] }
    { vstd  r1[0]                 ; ldc   r11, 0    }
    vsetc r11
    stw   r11, sp[3]
    std   r11, r11, sp[2]
    std   r11, r11, sp[3]
src_mrhf_spline_coeff_gen_main_loop2:
    { vclrdr                       ; ldap  r11, shift }
    vldc r1[0]                   

    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    vlsat r11[0]
    vstr r2[0]
    ldaw r2, r2[8]
    vclrdr
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    vlsat r11[0]
    vstr r2[0]
src_mrhf_spline_coeff_gen_done2:
    retsp NSTACKWORDS

.atmp:
    .size    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3, .atmp-src_mrhf_spline_coeff_gen_inner_loop_asm_xs3
    .align    8
    .cc_bottom src_mrhf_spline_coeff_gen_inner_loop_asm_xs3.function

    .set    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3.nstackwords, NSTACKWORDS
    .globl    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3.nstackwords
    .set    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3.maxcores, 1
    .globl    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3.maxcores
    .set    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3.maxtimers,0
    .globl    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3.maxtimers
    .set    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3.maxchanends,0
    .globl     src_mrhf_spline_coeff_gen_inner_loop_asm_xs3.maxchanends
#endif
