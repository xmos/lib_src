// Copyright 2016-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#define        FILTER_DEFS_ADFIR_PHASE_N_TAPS        ((1920 / 128) + 1)        // Number of taps per phase
    .section    .dp.data,"awd",@progbits
    .text

    .cc_top src_mrhf_spline_coeff_gen_inner_loop_asm.function
    .globl    src_mrhf_spline_coeff_gen_inner_loop_asm
    .align    8
    .type    src_mrhf_spline_coeff_gen_inner_loop_asm,@function

src_mrhf_spline_coeff_gen_inner_loop_asm:

#define piPhase                 r0 //Arg 1
#define piH                  r1 //Arg 2
#define piADCoefs           r2 //Arg 3
#define n_taps                r3 //Arg 4
#define Acc1_l                r4
#define Acc1_h                r5
#define Acc0_l                r6
#define Acc0_h                   r7
#define iData0              r8
#define iData1                r9
#define tmp                 r10
#define piPhase0                  r11 //This one remains constant - saves on shuffling

//    piPhase1        = piPhase0 + FILTER_DEFS_ADFIR_PHASE_N_TAPS;
//    piPhase2        = piPhase1 + FILTER_DEFS_ADFIR_PHASE_N_TAPS;

// Following assumes that  -  FILTER_DEFS_ADFIR_PHASE_N_TAPS = (1920 / 128 ) + 1 = 16
#if ((FILTER_DEFS_ADFIR_PHASE_N_TAPS) != 16)
#error "spline coeff asm hard coded to FILTER_DEFS_ADFIR_PHASE_N_TAPS  = (1920 / 128 ) + 1 = 16"
#endif

//In 32b words
#define stack_size        8

.issue_mode dual
    //Push registers
    DUALENTSP_lu6 stack_size
    std r4, r5, sp[0]
    std r6, r7, sp[1]
    std r8, r9, sp[2]
    std r10, r11, sp[3]


    {add piPhase0, piPhase, 0; shl n_taps, n_taps, 2}    //offset needs to be * 4 because is int
        add piPhase, piPhase, n_taps                    //PiPhase is PiPhase1 now - will swap between PiPhase1/2

src_mrhf_spline_coeff_gen_main_loop:
    //Loop = 0
        {ldc Acc0_h, 0; ldc Acc0_l, 0}        //Initialise MACC registers to zero
    {ldc Acc1_h, 0; ldc Acc1_l, 0}
    ldd iData0, iData1, piPhase0[0]
    ldw tmp, piH[2]
        maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    ldd iData0, iData1, piPhase[0]
    {ldw tmp, piH[1]; add piPhase, piPhase, n_taps} //piPhase2 now
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    ldd iData0, iData1, piPhase[0]
    {ldw tmp, piH[0]; sub piPhase, piPhase, n_taps} //piPhase1 now
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    std Acc0_h, Acc1_h, piADCoefs[0]

    //Loop = 2
        {ldc Acc0_h, 0; ldc Acc0_l, 0}        //Initialise MACC registers to zero
    {ldc Acc1_h, 0; ldc Acc1_l, 0}
    ldd iData0, iData1, piPhase0[1]
    ldw tmp, piH[2]
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    ldd iData0, iData1, piPhase[1]
    {ldw tmp, piH[1]; add piPhase, piPhase, n_taps} //piPhase2 now
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    ldd iData0, iData1, piPhase[1]
    {ldw tmp, piH[0]; sub piPhase, piPhase, n_taps} //piPhase1 now
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    std Acc0_h, Acc1_h, piADCoefs[1]

    //Loop = 4
        {ldc Acc0_h, 0; ldc Acc0_l, 0}        //Initialise MACC registers to zero
    {ldc Acc1_h, 0; ldc Acc1_l, 0}
    ldd iData0, iData1, piPhase0[2]
    ldw tmp, piH[2]
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    ldd iData0, iData1, piPhase[2]
    {ldw tmp, piH[1]; add piPhase, piPhase, n_taps} //piPhase2 now
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    ldd iData0, iData1, piPhase[2]
    {ldw tmp, piH[0]; sub piPhase, piPhase, n_taps} //piPhase1 now
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    std Acc0_h, Acc1_h, piADCoefs[2]

    //Loop = 6
        {ldc Acc0_h, 0; ldc Acc0_l, 0}        //Initialise MACC registers to zero
    {ldc Acc1_h, 0; ldc Acc1_l, 0}
    ldd iData0, iData1, piPhase0[3]
    ldw tmp, piH[2]
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    ldd iData0, iData1, piPhase[3]
    {ldw tmp, piH[1]; add piPhase, piPhase, n_taps} //piPhase2 now
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    ldd iData0, iData1, piPhase[3]
    {ldw tmp, piH[0]; sub piPhase, piPhase, n_taps} //piPhase1 now
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    std Acc0_h, Acc1_h, piADCoefs[3]

    //Loop = 8
        {ldc Acc0_h, 0; ldc Acc0_l, 0}        //Initialise MACC registers to zero
    {ldc Acc1_h, 0; ldc Acc1_l, 0}
    ldd iData0, iData1, piPhase0[4]
    ldw tmp, piH[2]
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    ldd iData0, iData1, piPhase[4]
    {ldw tmp, piH[1]; add piPhase, piPhase, n_taps} //piPhase2 now
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    ldd iData0, iData1, piPhase[4]
    {ldw tmp, piH[0]; sub piPhase, piPhase, n_taps} //piPhase1 now
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    std Acc0_h, Acc1_h, piADCoefs[4]

    //Loop = 10
        {ldc Acc0_h, 0; ldc Acc0_l, 0}        //Initialise MACC registers to zero
    {ldc Acc1_h, 0; ldc Acc1_l, 0}
    ldd iData0, iData1, piPhase0[5]
    ldw tmp, piH[2]
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    ldd iData0, iData1, piPhase[5]
    {ldw tmp, piH[1]; add piPhase, piPhase, n_taps} //piPhase2 now
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    ldd iData0, iData1, piPhase[5]
    {ldw tmp, piH[0]; sub piPhase, piPhase, n_taps} //piPhase1 now
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    std Acc0_h, Acc1_h, piADCoefs[5]

    //Loop = 12
        {ldc Acc0_h, 0; ldc Acc0_l, 0}        //Initialise MACC registers to zero
    {ldc Acc1_h, 0; ldc Acc1_l, 0}
    ldd iData0, iData1, piPhase0[6]
    ldw tmp, piH[2]
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    ldd iData0, iData1, piPhase[6]
    {ldw tmp, piH[1]; add piPhase, piPhase, n_taps} //piPhase2 now
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    ldd iData0, iData1, piPhase[6]
    {ldw tmp, piH[0]; sub piPhase, piPhase, n_taps} //piPhase1 now
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    std Acc0_h, Acc1_h, piADCoefs[6]

    //Loop = 14
        {ldc Acc0_h, 0; ldc Acc0_l, 0}        //Initialise MACC registers to zero
    {ldc Acc1_h, 0; ldc Acc1_l, 0}
    ldd iData0, iData1, piPhase0[7]
    ldw tmp, piH[2]
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    ldd iData0, iData1, piPhase[7]
    {ldw tmp, piH[1]; add piPhase, piPhase, n_taps} //piPhase2 now
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    ldd iData0, iData1, piPhase[7]
    {ldw tmp, piH[0]; sub piPhase, piPhase, n_taps} //piPhase1 now
    maccs Acc0_h, Acc0_l, tmp, iData0
    maccs Acc1_h, Acc1_l, tmp, iData1

    std Acc0_h, Acc1_h, piADCoefs[7]
src_mrhf_spline_coeff_gen_done:
    //Pop registers
    ldd r4, r5, sp[0]
    ldd r6, r7, sp[1]
    ldd r8, r9, sp[2]
    ldd r10, r11, sp[3]
  retsp stack_size

.atmp:
    .size    src_mrhf_spline_coeff_gen_inner_loop_asm, .atmp-src_mrhf_spline_coeff_gen_inner_loop_asm
    .align    8
    .cc_bottom src_mrhf_spline_coeff_gen_inner_loop_asm.function

    .set    src_mrhf_spline_coeff_gen_inner_loop_asm.nstackwords, stack_size
    .globl    src_mrhf_spline_coeff_gen_inner_loop_asm.nstackwords
    .set    src_mrhf_spline_coeff_gen_inner_loop_asm.maxcores, 1
    .globl    src_mrhf_spline_coeff_gen_inner_loop_asm.maxcores
    .set    src_mrhf_spline_coeff_gen_inner_loop_asm.maxtimers,0
    .globl    src_mrhf_spline_coeff_gen_inner_loop_asm.maxtimers
    .set    src_mrhf_spline_coeff_gen_inner_loop_asm.maxchanends,0
    .globl     src_mrhf_spline_coeff_gen_inner_loop_asm.maxchanends
