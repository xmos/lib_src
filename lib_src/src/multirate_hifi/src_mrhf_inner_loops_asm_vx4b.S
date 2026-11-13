// Copyright 2024-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#if defined(__VX4B__)

// ===================================================================== //
// VPU (VX4/xcore-400) implementations of the lib_src multirate HiFi     //
// inner loops.  These implement the same contracts as the XS3A VPU      //
// versions (the *_vpu functions selected by SRC_USE_VPU, see            //
// use_vpu.h):                                                           //
//  - VEC_INT_32 mode: 8 x 32-bit lanes, each product is shifted down    //
//    by 30 bits and accumulated into a 40-bit accumulator (vD:vR)       //
//  - VLMACCR computes an 8-term inner product of vC and memory and      //
//    rotates the 8-element accumulator ring, inserting at element 0     //
//  - VLSAT shifts (with rounding) and saturates each 40-bit lane by     //
//    the per-lane shift amounts loaded from memory                      //
//                                                                       //
// The FIR dot products sum (data*coef) >> 30 into the 8 ring slots,     //
// then reduce: VLSAT >> s, multiply the partials by ~2.0 (0x7fffffff    //
// in the Q30 product domain) with one more VLMACCR and saturate, for a  //
// net arithmetic of (sum(data*coef)) >> (30 + s + ... - 1):             //
//  - adfir: VLSAT >> 1 -> result = sat(acc64 >> 30)                     //
//  - fir/fir_os: VLSAT >> 2 -> result = sat(acc64 >> 31)                //
// This mirrors the XS3A code exactly so results match the scalar        //
// reference implementations within the documented +/-2 LSB.            //
//                                                                       //
// Note: instruction packets may only use registers r0..r11, which map   //
// to {a0-a3, s2-s8, t3}, and the first slot must be 16-bit encodable.   //
// xm.entsp saves the return address within the frame, so the 8-word     //
// vector scratch area at sp[0] needs a frame larger than 8 words.       //
// ===================================================================== //

    .section .rodata
    .p2align 5
.L_vec_ones:                        // ~2.0 in the (product >> 30) domain
    .word 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff
    .word 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff
.L_vec_evens:                       // picks even ring slots
    .word 0x7fffffff, 0, 0x7fffffff, 0, 0x7fffffff, 0, 0x7fffffff, 0
.L_vec_odds:                        // picks odd ring slots
    .word 0, 0x7fffffff, 0, 0x7fffffff, 0, 0x7fffffff, 0, 0x7fffffff
.L_vec_shifts0:
    .word 0, 0, 0, 0, 0, 0, 0, 0
.L_vec_shifts1:
    .word 1, 1, 1, 1, 1, 1, 1, 1
.L_vec_shifts2:
    .word 2, 2, 2, 2, 2, 2, 2, 2

#define NSTACKWORDS 16

// ===================================================================== //
// void src_mrhf_adfir_inner_loop_asm_vpu(int *data, int *coefs,         //
//                                        int result[], int count);      //
// Computes result[0] = sat((sum of 2*count data*coef products) >> 30)   //
// count must be a multiple of 8 (count/8 iterations of 16 taps).        //
// ===================================================================== //
    .text
    .p2align 4
    .globl  src_mrhf_adfir_inner_loop_asm_vpu
    .type   src_mrhf_adfir_inner_loop_asm_vpu, @function
src_mrhf_adfir_inner_loop_asm_vpu:
    xm.entsp (NSTACKWORDS)*4
    { li t3, 0                          ; nop                                   }
    { srli a3, a3, 3                    ; xm.vsetc t3                           }
    li t3, 32
    { nop                               ; xm.vclrdr                             }
    { nop                               ; xm.brff a3, .L_adfir_reduce           }
.L_adfir_loop:
    { add a0, a0, t3                    ; xm.vldc a0                            }
    { add a1, a1, t3                    ; xm.vlmaccr0 a1                        }
    { addi a3, a3, -1                   ; xm.vldc a0                            }
    { add a0, a0, t3                    ; xm.vlmaccr0 a1                        }
    { add a1, a1, t3                    ; xm.bt a3, .L_adfir_loop               }
.L_adfir_reduce:
    la t3, .L_vec_shifts1
    xm.vlsat t3
    { addi a0, sp, 0                    ; nop                                   }
    la a1, .L_vec_ones
    { nop                               ; xm.vstr a0                            }
    la t3, .L_vec_shifts0
    { nop                               ; xm.vldc a0                            }
    { nop                               ; xm.vclrdr                             }
    { nop                               ; xm.vlmaccr0 a1                        }
    xm.vlsat t3
    { xm.mkmski t3, 4                   ; nop                                   }
    xm.vstrpv a2, t3
    xm.retsp (NSTACKWORDS)*4
.L_adfir_end:
    .resource_const src_mrhf_adfir_inner_loop_asm_vpu, "stack_frame_bytes", (NSTACKWORDS)*4
    .resource_list_empty src_mrhf_adfir_inner_loop_asm_vpu, "tail_callees"
    .resource_list_empty src_mrhf_adfir_inner_loop_asm_vpu, "callees"
    .resource_list_empty src_mrhf_adfir_inner_loop_asm_vpu, "parallel_callees"
    .size   src_mrhf_adfir_inner_loop_asm_vpu, .L_adfir_end-src_mrhf_adfir_inner_loop_asm_vpu

// ===================================================================== //
// void src_mrhf_fir_inner_loop_asm_vpu(int *data, int *coefs,           //
//                                      int result[], int count);        //
// Computes result[0] = sat((sum of 2*count data*coef products) >> 31)   //
// count must be a multiple of 8 (count/8 iterations of 16 taps).        //
// ===================================================================== //
    .p2align 4
    .globl  src_mrhf_fir_inner_loop_asm_vpu
    .type   src_mrhf_fir_inner_loop_asm_vpu, @function
src_mrhf_fir_inner_loop_asm_vpu:
    xm.entsp (NSTACKWORDS)*4
    { li t3, 0                          ; nop                                   }
    { srli a3, a3, 3                    ; xm.vsetc t3                           }
    li t3, 32
    { nop                               ; xm.vclrdr                             }
    { nop                               ; xm.brff a3, .L_fir_reduce             }
.L_fir_loop:
    { add a0, a0, t3                    ; xm.vldc a0                            }
    { add a1, a1, t3                    ; xm.vlmaccr0 a1                        }
    { addi a3, a3, -1                   ; xm.vldc a0                            }
    { add a0, a0, t3                    ; xm.vlmaccr0 a1                        }
    { add a1, a1, t3                    ; xm.bt a3, .L_fir_loop                 }
.L_fir_reduce:
    la t3, .L_vec_shifts2
    xm.vlsat t3
    { addi a0, sp, 0                    ; nop                                   }
    la a1, .L_vec_ones
    { nop                               ; xm.vstr a0                            }
    la t3, .L_vec_shifts0
    { nop                               ; xm.vldc a0                            }
    { nop                               ; xm.vclrdr                             }
    { nop                               ; xm.vlmaccr0 a1                        }
    xm.vlsat t3
    { xm.mkmski t3, 4                   ; nop                                   }
    xm.vstrpv a2, t3
    xm.retsp (NSTACKWORDS)*4
.L_fir_end:
    .resource_const src_mrhf_fir_inner_loop_asm_vpu, "stack_frame_bytes", (NSTACKWORDS)*4
    .resource_list_empty src_mrhf_fir_inner_loop_asm_vpu, "tail_callees"
    .resource_list_empty src_mrhf_fir_inner_loop_asm_vpu, "callees"
    .resource_list_empty src_mrhf_fir_inner_loop_asm_vpu, "parallel_callees"
    .size   src_mrhf_fir_inner_loop_asm_vpu, .L_fir_end-src_mrhf_fir_inner_loop_asm_vpu

// ===================================================================== //
// void src_mrhf_fir_os_inner_loop_asm_vpu(int *data, int *coefs,        //
//                                         int result[], int count);     //
// Oversampling FIR: coefs hold two interleaved phases as alternating    //
// 8-word vectors (even vector, odd vector) per 8 data samples.          //
// result[0] (odd phase) and result[1] (even phase) = sat(acc >> 31).    //
// count must be a multiple of 4 (count/4 iterations of 8 samples).      //
// ===================================================================== //
    .p2align 4
    .globl  src_mrhf_fir_os_inner_loop_asm_vpu
    .type   src_mrhf_fir_os_inner_loop_asm_vpu, @function
src_mrhf_fir_os_inner_loop_asm_vpu:
    xm.entsp (NSTACKWORDS)*4
    { li t3, 0                          ; nop                                   }
    { srli a3, a3, 2                    ; xm.vsetc t3                           }
    li t3, 32
    { nop                               ; xm.vclrdr                             }
    { nop                               ; xm.brff a3, .L_fir_os_reduce          }
.L_fir_os_loop:
    { add a0, a0, t3                    ; xm.vldc a0                            }
    { add a1, a1, t3                    ; xm.vlmaccr0 a1                        }
    { addi a3, a3, -1                   ; xm.vlmaccr0 a1                        }
    { add a1, a1, t3                    ; xm.bt a3, .L_fir_os_loop              }
.L_fir_os_reduce:
    la t3, .L_vec_shifts2
    xm.vlsat t3
    { addi a0, sp, 0                    ; nop                                   }
    la a1, .L_vec_evens
    { nop                               ; xm.vstr a0                            }
    la t3, .L_vec_odds
    { nop                               ; xm.vldc a0                            }
    { nop                               ; xm.vclrdr                             }
    { nop                               ; xm.vlmaccr0 a1                        }
    la a1, .L_vec_shifts0
    { nop                               ; xm.vlmaccr0 t3                        }
    xm.vlsat a1
    { xm.mkmski t3, 8                   ; nop                                   }
    xm.vstrpv a2, t3
    xm.retsp (NSTACKWORDS)*4
.L_fir_os_end:
    .resource_const src_mrhf_fir_os_inner_loop_asm_vpu, "stack_frame_bytes", (NSTACKWORDS)*4
    .resource_list_empty src_mrhf_fir_os_inner_loop_asm_vpu, "tail_callees"
    .resource_list_empty src_mrhf_fir_os_inner_loop_asm_vpu, "callees"
    .resource_list_empty src_mrhf_fir_os_inner_loop_asm_vpu, "parallel_callees"
    .size   src_mrhf_fir_os_inner_loop_asm_vpu, .L_fir_os_end-src_mrhf_fir_os_inner_loop_asm_vpu

// ===================================================================== //
// void src_mrhf_spline_coeff_gen_inner_loop_asm_vpu(int *piPhase0,      //
//                              int *iH, int *piADCoefs, int n_taps);    //
// Generates the 16 adaptive FIR coefficients by interpolating three     //
// adjacent phases with the spline weights iH[0..2].                     //
// iADFirCoefs is stored transposed [16 taps][130 phases] so each        //
// VLMACCR reads phases (t, t+1, t+2) of one tap contiguously; the ring  //
// rotation reverses groups of 8, which ASRC_prepare_coefs compensates   //
// for with its (tap ^ 7) index shuffle.                                 //
// Output: piADCoefs[i] = (iH dot phases)[i] >> 32 (VLSAT >> 2 on top    //
// of the >> 30 product scaling), n_taps must be 16.                     //
// ===================================================================== //

#define ADFIR_PHASE_STRIDE ((128 + 2) * 4)  // FILTER_DEFS_ADFIR_N_PHASES + 2 words

    .p2align 4
    .globl  src_mrhf_spline_coeff_gen_inner_loop_asm_vpu
    .type   src_mrhf_spline_coeff_gen_inner_loop_asm_vpu, @function
src_mrhf_spline_coeff_gen_inner_loop_asm_vpu:
    xm.entsp (NSTACKWORDS)*4
    { li t3, 0                          ; nop                                   }
    { addi a3, sp, 0                    ; xm.vsetc t3                           }
    { nop                               ; xm.vclrdr                             }
    xm.vstr a3                          // zero the 8-word stack vector
    lw t3, 0(a1)
    sw t3, 0(sp)
    lw t3, 4(a1)
    sw t3, 4(sp)
    lw t3, 8(a1)
    sw t3, 8(sp)                        // stack vec = { H0, H1, H2, 0, 0, 0, 0, 0 }
    li a1, ADFIR_PHASE_STRIDE
    { nop                               ; xm.vldc a3                            }
    la t3, .L_vec_shifts2
    // taps 0..7
    { add a0, a0, a1                    ; xm.vlmaccr0 a0                        }
    { add a0, a0, a1                    ; xm.vlmaccr0 a0                        }
    { add a0, a0, a1                    ; xm.vlmaccr0 a0                        }
    { add a0, a0, a1                    ; xm.vlmaccr0 a0                        }
    { add a0, a0, a1                    ; xm.vlmaccr0 a0                        }
    { add a0, a0, a1                    ; xm.vlmaccr0 a0                        }
    { add a0, a0, a1                    ; xm.vlmaccr0 a0                        }
    { add a0, a0, a1                    ; xm.vlmaccr0 a0                        }
    xm.vlsat t3
    xm.vstr a2
    addi a2, a2, 32
    { nop                               ; xm.vclrdr                             }
    // taps 8..15
    { add a0, a0, a1                    ; xm.vlmaccr0 a0                        }
    { add a0, a0, a1                    ; xm.vlmaccr0 a0                        }
    { add a0, a0, a1                    ; xm.vlmaccr0 a0                        }
    { add a0, a0, a1                    ; xm.vlmaccr0 a0                        }
    { add a0, a0, a1                    ; xm.vlmaccr0 a0                        }
    { add a0, a0, a1                    ; xm.vlmaccr0 a0                        }
    { add a0, a0, a1                    ; xm.vlmaccr0 a0                        }
    { add a0, a0, a1                    ; xm.vlmaccr0 a0                        }
    xm.vlsat t3
    xm.vstr a2
    xm.retsp (NSTACKWORDS)*4
.L_spline_end:
    .resource_const src_mrhf_spline_coeff_gen_inner_loop_asm_vpu, "stack_frame_bytes", (NSTACKWORDS)*4
    .resource_list_empty src_mrhf_spline_coeff_gen_inner_loop_asm_vpu, "tail_callees"
    .resource_list_empty src_mrhf_spline_coeff_gen_inner_loop_asm_vpu, "callees"
    .resource_list_empty src_mrhf_spline_coeff_gen_inner_loop_asm_vpu, "parallel_callees"
    .size   src_mrhf_spline_coeff_gen_inner_loop_asm_vpu, .L_spline_end-src_mrhf_spline_coeff_gen_inner_loop_asm_vpu

#endif // defined(__VX4B__)
