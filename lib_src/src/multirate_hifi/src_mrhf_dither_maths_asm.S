// Copyright 2016-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
    .section    .dp.data,"awd",@progbits
    .text

    .cc_top dither_maths_asm.function
    .globl    dither_maths_asm
    .align    8
    .type    dither_maths_asm,@function

dither_maths_asm:

#define piData             r0
#define ACCh             r1
#define ACCl            r2


//In 32b words
#define stack_size        0

#if 0
.issue_mode dual
    //Push registers
    DUALENTSP_lu6 stack_size
    std r4, r5, sp[0]
    std r6, r7, sp[1]
    std r8, r9, sp[2]

    //push return value pointers onto stack. Frees up r2
    //Divide counter by 8
    {stw iDataRet_p, sp[6]; shr counter, counter, 3}

    //Constant for adding onto stuff. Gets updated later
    ldc step, 0x40

    //Initilise MACC regsiters to zero
    {ldc Acc0_h, 0; ldc Acc0_l, 0}

    ldd iData0, iData1, piData_p[0]
    ldd iCoef0, iCoef1, piCoefs_p[0]
    maccs Acc0_h, Acc0_l, iData0, iCoef0
    maccs Acc0_h, Acc0_l, iData1, iCoef1

    //pop return value pointer from stack so we can use it to write back
    //Write back the two return values
    {ldw iDataRet_p, sp[6];     ldc counter, 31}                        //Reuse counter (r3)

    lsats Acc0_h, Acc0_l, r3            //Saturate first
    lextract iData0, Acc0_h, Acc0_l, counter, 32    //Extract
    stw iData0, iDataRet_p[0]

    //Pop registers
    ldd r4, r5, sp[0]
    ldd r6, r7, sp[1]
    ldd r8, r9, sp[2]
  retsp stack_size
#endif
.atmp:
    .size    dither_maths_asm, .atmp-dither_maths_asm
    .align    8
    .cc_bottom dither_maths_asm.function

    .set    dither_maths_asm.nstackwords, stack_size
    .globl    dither_maths_asm.nstackwords
    .set    dither_maths_asm.maxcores, 1
    .globl    dither_maths_asm.maxcores
    .set    dither_maths_asm.maxtimers,0
    .globl    dither_maths_asm.maxtimers
    .set    dither_maths_asm.maxchanends,0
    .globl     dither_maths_asm.maxchanends



