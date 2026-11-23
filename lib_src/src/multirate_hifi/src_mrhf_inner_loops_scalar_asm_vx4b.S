// Copyright 2024-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#if defined(__VX4B__)

// Stack frame sizes and offsets are in bytes.
    .text
    .p2align 4
    .globl src_mrhf_fir_inner_loop_asm
    .globl src_mrhf_fir_inner_loop_asm_odd
    .type src_mrhf_fir_inner_loop_asm, @function
    .type src_mrhf_fir_inner_loop_asm_odd, @function
src_mrhf_fir_inner_loop_asm:
src_mrhf_fir_inner_loop_asm_odd:
    li t3, 31
    j .L_dot

    .globl src_mrhf_adfir_inner_loop_asm
    .globl src_mrhf_adfir_inner_loop_asm_odd
    .type src_mrhf_adfir_inner_loop_asm, @function
    .type src_mrhf_adfir_inner_loop_asm_odd, @function
src_mrhf_adfir_inner_loop_asm:
src_mrhf_adfir_inner_loop_asm_odd:
    li t3, 30
.L_dot:
    xm.entsp 48
    xm.stdsp s2, s3, 0
    xm.stdsp s4, s5, 8
    xm.stdsp s6, s7, 16
    sw t3, 24(sp)
    { li s2, 0 ; li s3, 0 }
    andi t3, a1, 7
    bnez t3, .L_dot_unaligned
    andi t3, a3, 7
    { sw t3, 28(sp) ; xm.shri a3, a3, 3 }
    beqz a3, .L_dot_tail
    andi t3, a0, 7
    bnez t3, .L_dot_odd_setup
    li t3, 64

#define DOT_PAIR(offset) \
    xm.ldd s4, s5, offset(a0); \
    xm.ldd s6, s7, offset(a1); \
    xm.maccs s2, s3, s4, s6; \
    xm.maccs s2, s3, s5, s7

.L_dot_aligned_loop:
    DOT_PAIR(0)
    DOT_PAIR(8)
    DOT_PAIR(16)
    DOT_PAIR(24)
    DOT_PAIR(32)
    DOT_PAIR(40)
    DOT_PAIR(48)
    DOT_PAIR(56)
    { xm.add a0, a0, t3 ; xm.add a1, a1, t3 }
    addi a3, a3, -1
    bnez a3, .L_dot_aligned_loop
    j .L_dot_tail

#undef DOT_PAIR
#define DOT_ODD_PAIR(data_offset, coef_offset) \
    xm.ldd s4, s5, data_offset(a0); \
    xm.maccs s2, s3, s4, s7; \
    xm.ldd s6, s7, coef_offset(a1); \
    xm.maccs s2, s3, s5, s6

.L_dot_odd_setup:
    addi a0, a0, 4
    li t3, 64
.L_dot_odd_loop:
    lw s4, -4(a0)
    xm.ldd s6, s7, 0(a1)
    xm.maccs s2, s3, s4, s6
    DOT_ODD_PAIR(0, 8)
    DOT_ODD_PAIR(8, 16)
    DOT_ODD_PAIR(16, 24)
    DOT_ODD_PAIR(24, 32)
    DOT_ODD_PAIR(32, 40)
    DOT_ODD_PAIR(40, 48)
    DOT_ODD_PAIR(48, 56)
    lw s4, 56(a0)
    xm.maccs s2, s3, s4, s7
    { xm.add a0, a0, t3 ; xm.add a1, a1, t3 }
    addi a3, a3, -1
    bnez a3, .L_dot_odd_loop
    addi a0, a0, -4
#undef DOT_ODD_PAIR
.L_dot_tail:
    lw a3, 28(sp)
.L_dot_unaligned:
    slli a3, a3, 1
    beqz a3, .L_dot_done
.L_dot_loop:
    lw s4, 0(a0)
    lw s5, 0(a1)
    xm.maccs s2, s3, s4, s5
    { xm.addi a0, a0, 4 ; xm.subi a3, a3, 1 }
    { xm.addi a1, a1, 4 ; xm.bt a3, .L_dot_loop }
.L_dot_done:
    lw t3, 24(sp)
    xm.lsats s2, s3, t3
    xm.lextract s4, s2, s3, t3, 32
    sw s4, 0(a2)
    xm.lddsp s2, s3, 0
    xm.lddsp s4, s5, 8
    xm.lddsp s6, s7, 16
    xm.retsp 48
.L_dot_end:

    .size src_mrhf_fir_inner_loop_asm, .L_dot_end-src_mrhf_fir_inner_loop_asm
    .size src_mrhf_fir_inner_loop_asm_odd, .L_dot_end-src_mrhf_fir_inner_loop_asm_odd
    .size src_mrhf_adfir_inner_loop_asm, .L_dot_end-src_mrhf_adfir_inner_loop_asm
    .size src_mrhf_adfir_inner_loop_asm_odd, .L_dot_end-src_mrhf_adfir_inner_loop_asm_odd

    .p2align 4
    .globl src_mrhf_fir_os_inner_loop_asm
    .globl src_mrhf_fir_os_inner_loop_asm_odd
    .type src_mrhf_fir_os_inner_loop_asm, @function
    .type src_mrhf_fir_os_inner_loop_asm_odd, @function
src_mrhf_fir_os_inner_loop_asm:
src_mrhf_fir_os_inner_loop_asm_odd:
    xm.entsp 48
    xm.stdsp s2, s3, 0
    xm.stdsp s4, s5, 8
    xm.stdsp s6, s7, 16
    xm.stdsp s8, a2, 24
    { li s2, 0 ; li s3, 0 }
    { li s4, 0 ; li s5, 0 }
    srli a3, a3, 2
    andi t3, a1, 7
    bnez t3, .L_os_unaligned
    beqz a3, .L_os_done
    li a2, 64
    andi t3, a0, 7
    bnez t3, .L_os_odd_setup

#define OS_PAIR(data_offset, coef_offset) \
    xm.ldd s6, t3, data_offset(a0); \
    xm.ldd s7, s8, coef_offset(a1); \
    xm.maccs s2, s3, s6, s7; \
    xm.maccs s4, s5, s6, s8; \
    xm.ldd s7, s8, (coef_offset + 8)(a1); \
    xm.maccs s2, s3, t3, s7; \
    xm.maccs s4, s5, t3, s8

.L_os_aligned_loop:
    OS_PAIR(0, 0)
    OS_PAIR(8, 16)
    OS_PAIR(16, 32)
    OS_PAIR(24, 48)
    addi a0, a0, 32
    addi a3, a3, -1
    { xm.add a1, a1, a2 ; xm.bt a3, .L_os_aligned_loop }
    j .L_os_done

#define OS_SAMPLE(data_offset, coef_offset) \
    lw s6, data_offset(a0); \
    xm.ldd s7, s8, coef_offset(a1); \
    xm.maccs s2, s3, s6, s7; \
    xm.maccs s4, s5, s6, s8

.L_os_odd_setup:
    addi a0, a0, 4
.L_os_odd_loop:
    OS_SAMPLE(-4, 0)
    OS_PAIR(0, 8)
    OS_PAIR(8, 24)
    OS_PAIR(16, 40)
    { lw s6, 24(a0) ; xm.subi a3, a3, 1 }
    xm.ldd s7, s8, 56(a1)
    xm.maccs s2, s3, s6, s7
    xm.maccs s4, s5, s6, s8
    addi a0, a0, 32
    { xm.add a1, a1, a2 ; xm.bt a3, .L_os_odd_loop }
    j .L_os_done
#undef OS_PAIR
#undef OS_SAMPLE
.L_os_unaligned:
    slli a3, a3, 3
    beqz a3, .L_os_done
.L_os_loop:
    lw s6, 0(a0)
    lw s7, 0(a1)
    xm.maccs s2, s3, s6, s7
    lw s7, 4(a1)
    xm.maccs s4, s5, s6, s7
    { xm.addi a0, a0, 4 ; xm.subi a3, a3, 1 }
    { xm.addi a1, a1, 8 ; xm.bt a3, .L_os_loop }
.L_os_done:
    xm.lddsp s8, a2, 24
    li t3, 31
    xm.lsats s2, s3, t3
    xm.lsats s4, s5, t3
    xm.lextract s6, s2, s3, t3, 32
    xm.lextract s7, s4, s5, t3, 32
    sw s6, 0(a2)
    sw s7, 4(a2)
    xm.lddsp s2, s3, 0
    xm.lddsp s4, s5, 8
    xm.lddsp s6, s7, 16
    xm.retsp 48
.L_os_end:
    .size src_mrhf_fir_os_inner_loop_asm, .L_os_end-src_mrhf_fir_os_inner_loop_asm
    .size src_mrhf_fir_os_inner_loop_asm_odd, .L_os_end-src_mrhf_fir_os_inner_loop_asm_odd

    .p2align 4
    .globl src_mrhf_spline_coeff_gen_inner_loop_asm
    .type src_mrhf_spline_coeff_gen_inner_loop_asm, @function
src_mrhf_spline_coeff_gen_inner_loop_asm:
    xm.entsp 48
    xm.stdsp s2, s3, 0
    xm.stdsp s4, s5, 8
    xm.stdsp s6, s7, 16
    sw s8, 24(sp)
    blez a3, .L_spline_done
    slli t3, a3, 2
    add s2, a0, t3
    add s3, s2, t3
    or t3, a0, s2
    andi t3, t3, 7
    bnez t3, .L_spline_loop
    lw s8, 0(a1)
    lw t3, 4(a1)
    xm.stdsp s8, t3, 32
    lw t3, 8(a1)
    sw t3, 40(sp)
    srli a3, a3, 1
.L_spline_pair_loop:
    { li s4, 0 ; li s5, 0 }
    { li s6, 0 ; li s7, 0 }
    xm.ldd s8, t3, 0(a0)
    lw a1, 40(sp)
    xm.maccs s4, s5, s8, a1
    xm.maccs s6, s7, t3, a1
    xm.ldd s8, t3, 0(s2)
    lw a1, 36(sp)
    xm.maccs s4, s5, s8, a1
    xm.maccs s6, s7, t3, a1
    xm.ldd s8, t3, 0(s3)
    lw a1, 32(sp)
    xm.maccs s4, s5, s8, a1
    xm.maccs s6, s7, t3, a1
    sw s4, 0(a2)
    sw s6, 4(a2)
    { xm.addi a0, a0, 8 ; xm.addi s2, s2, 8 }
    { xm.addi s3, s3, 8 ; xm.subi a3, a3, 1 }
    { xm.addi a2, a2, 8 ; xm.bt a3, .L_spline_pair_loop }
    j .L_spline_done
.L_spline_loop:
    { li s4, 0 ; li s5, 0 }
    lw s6, 0(a0)
    lw s7, 8(a1)
    xm.maccs s4, s5, s6, s7
    lw s6, 0(s2)
    lw s7, 4(a1)
    xm.maccs s4, s5, s6, s7
    lw s6, 0(s3)
    lw s7, 0(a1)
    xm.maccs s4, s5, s6, s7
    sw s4, 0(a2)
    { xm.addi a0, a0, 4 ; xm.addi s2, s2, 4 }
    { xm.addi s3, s3, 4 ; xm.subi a3, a3, 1 }
    { xm.addi a2, a2, 4 ; xm.bt a3, .L_spline_loop }
.L_spline_done:
    xm.lddsp s2, s3, 0
    xm.lddsp s4, s5, 8
    xm.lddsp s6, s7, 16
    lw s8, 24(sp)
    xm.retsp 48
.L_spline_end:
    .size src_mrhf_spline_coeff_gen_inner_loop_asm, .L_spline_end-src_mrhf_spline_coeff_gen_inner_loop_asm

#define SCALAR_RESOURCE(name, frame) \
    .resource_const name, "stack_frame_bytes", frame; \
    .resource_list_empty name, "tail_callees"; \
    .resource_list_empty name, "callees"; \
    .resource_list_empty name, "parallel_callees"

    SCALAR_RESOURCE(src_mrhf_fir_inner_loop_asm, 48)
    SCALAR_RESOURCE(src_mrhf_fir_inner_loop_asm_odd, 48)
    SCALAR_RESOURCE(src_mrhf_adfir_inner_loop_asm, 48)
    SCALAR_RESOURCE(src_mrhf_adfir_inner_loop_asm_odd, 48)
    SCALAR_RESOURCE(src_mrhf_fir_os_inner_loop_asm, 48)
    SCALAR_RESOURCE(src_mrhf_fir_os_inner_loop_asm_odd, 48)
    SCALAR_RESOURCE(src_mrhf_spline_coeff_gen_inner_loop_asm, 48)

#endif
