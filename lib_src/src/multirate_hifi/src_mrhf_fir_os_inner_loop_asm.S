// Copyright 2016-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
    .section    .dp.data,"awd",@progbits
    .text

    .cc_top src_mrhf_fir_os_inner_loop_asm.function
    .globl    src_mrhf_fir_os_inner_loop_asm
    .align    8
    .type    src_mrhf_fir_os_inner_loop_asm,@function

src_mrhf_fir_os_inner_loop_asm:

#define piData_p             r0
#define piCoefs_p             r1
#define iDataRet_p    r2
#define step                    r2
#define counter            r3
#define iCoef0                r4
#define iCoef1                  r5
#define iData0              r6
#define iData1            r7
#define Acc0_l            r8
#define Acc0_h               r9
#define Acc1_l               r10
#define Acc1_h             r11



#define stack_size        10 //In words

.issue_mode dual
    //Push registers
    DUALENTSP_lu6 stack_size
    std r4, r5, sp[0]
    std r6, r7, sp[1]
    std r8, r9, sp[2]
    std r10, r11, sp[3]

    //push return value pointers onto stack. Frees up r2
    //Divide counter by 4
    {stw iDataRet_p, sp[8]; shr counter, counter, 2}

    //Constant for adding onto stuff. Gets updated later
    ldc step, 0x40

    //Initilise MACC regsiters to zero
    {ldc Acc0_h, 0;    ldc Acc0_l, 0}
    {ldc Acc1_h, 0; ldc Acc1_l, 0}

    bf counter, src_mrhf_fir_os_done
src_mrhf_fir_os_main_loop:
    sub counter, counter, 1

    ldd iData1, iData0, piData_p[0]
    ldd iCoef0, iCoef1, piCoefs_p[0]
    maccs Acc0_h, Acc0_l, iData0, iCoef0
    maccs Acc1_h, Acc1_l, iData0, iCoef1
    ldd iCoef0, iCoef1, piCoefs_p[1]
    maccs Acc0_h, Acc0_l, iData1, iCoef0
    maccs Acc1_h, Acc1_l, iData1, iCoef1

    ldd iData1, iData0, piData_p[1]
    ldd iCoef0, iCoef1, piCoefs_p[2]
    maccs Acc0_h, Acc0_l, iData0, iCoef0
    maccs Acc1_h, Acc1_l, iData0, iCoef1
    ldd iCoef0, iCoef1, piCoefs_p[3]
    maccs Acc0_h, Acc0_l, iData1, iCoef0
    maccs Acc1_h, Acc1_l, iData1, iCoef1

    ldd iData1, iData0, piData_p[2]
    ldd iCoef0, iCoef1, piCoefs_p[4]
    maccs Acc0_h, Acc0_l, iData0, iCoef0
    maccs Acc1_h, Acc1_l, iData0, iCoef1
    ldd iCoef0, iCoef1, piCoefs_p[5]
    maccs Acc0_h, Acc0_l, iData1, iCoef0
    maccs Acc1_h, Acc1_l, iData1, iCoef1

    ldd iData1, iData0, piData_p[3]
    ldd iCoef0, iCoef1, piCoefs_p[6]
    maccs Acc0_h, Acc0_l, iData0, iCoef0
    maccs Acc1_h, Acc1_l, iData0, iCoef1
    ldd iCoef0, iCoef1, piCoefs_p[7]
    maccs Acc0_h, Acc0_l, iData1, iCoef0
    maccs Acc1_h, Acc1_l, iData1, iCoef1

        //Increment coeffs by step
    {add piCoefs_p, piCoefs_p, step; shr step, step, 1}
        //Increment data by step/2
        {add piData_p, piData_p, step; shl step, step, 1}

    //Decrement loop counter
        bt counter, src_mrhf_fir_os_main_loop

src_mrhf_fir_os_done:
    //pop return value pointer from stack so we can use it to write back
    //Resuse counter (r3)
    {ldw iDataRet_p, sp[8]; ldc counter, 31}

    //Write back the two return values and do sat/ext
    lsats Acc0_h, Acc0_l, r3            //Saturate first
    lsats Acc1_h, Acc1_l, r3
    lextract iData0, Acc0_h, Acc0_l, counter, 32    //Extract
    lextract iData1, Acc1_h, Acc1_l, counter, 32
    //stw iData0, iDataRet_p[1]
    //stw iData1, iDataRet_p[0]
    std iData0, iData1, iDataRet_p[0]

    //Pop registers
    ldd r4, r5, sp[0]
    ldd r6, r7, sp[1]
    ldd r8, r9, sp[2]
    ldd r10, r11, sp[3]
  retsp stack_size

.atmp:
    .size    src_mrhf_fir_os_inner_loop_asm, .atmp-src_mrhf_fir_os_inner_loop_asm
    .align    8
    .cc_bottom src_mrhf_fir_os_inner_loop_asm.function

    .set    src_mrhf_fir_os_inner_loop_asm.nstackwords, stack_size
    .globl    src_mrhf_fir_os_inner_loop_asm.nstackwords
    .set    src_mrhf_fir_os_inner_loop_asm.maxcores, 1
    .globl    src_mrhf_fir_os_inner_loop_asm.maxcores
    .set    src_mrhf_fir_os_inner_loop_asm.maxtimers,0
    .globl    src_mrhf_fir_os_inner_loop_asm.maxtimers
    .set    src_mrhf_fir_os_inner_loop_asm.maxchanends,0
    .globl     src_mrhf_fir_os_inner_loop_asm.maxchanends









    .section    .dp.data,"awd",@progbits
    .text

    .cc_top src_mrhf_fir_os_inner_loop_asm_odd.function
    .globl    src_mrhf_fir_os_inner_loop_asm_odd
    .align    8
    .type    src_mrhf_fir_os_inner_loop_asm_odd,@function

src_mrhf_fir_os_inner_loop_asm_odd:

#define piData_p             r0
#define piCoefs_p             r1
#define iDataRet_p    r2
#define step                    r2
#define counter            r3
#define iCoef0                r4
#define iCoef1                  r5
#define iData0              r6
#define iData1            r7
#define Acc0_l            r8
#define Acc0_h               r9
#define Acc1_l               r10
#define Acc1_h             r11



#define stack_size        10 //In words

.issue_mode dual
    //Push registers
    DUALENTSP_lu6 stack_size
    std r4, r5, sp[0]
    std r6, r7, sp[1]
    std r8, r9, sp[2]
    std r10, r11, sp[3]

    //push return value pointers onto stack. Frees up r2
    //Divide counter by 4
    {stw iDataRet_p, sp[8] ; shr counter, counter, 2}

    //Initilise MACC regsiters to zero
    {ldc Acc0_h, 0; ldc Acc0_l, 0}
    {ldc Acc1_h, 0;    ldc Acc1_l, 0}

    bf counter, src_mrhf_fir_os_done_odd
src_mrhf_fir_os_main_loop_odd:
    //Constant for adding onto stuff. Gets updated later
    {ldc step, 0x18; sub counter, counter, 1}

    {ldw iData0, piData_p[0]; add piData_p, piData_p, 0x4}    //Single word data load
    ldd iCoef0, iCoef1, piCoefs_p[0]    //Double word coef load
    maccs Acc0_h, Acc0_l, iData0, iCoef0
    maccs Acc1_h, Acc1_l, iData0, iCoef1

    //Now double word aligned for 64b goodness

    ldd iData1, iData0, piData_p[0]
    ldd iCoef0, iCoef1, piCoefs_p[1]
    maccs Acc0_h, Acc0_l, iData0, iCoef0
    maccs Acc1_h, Acc1_l, iData0, iCoef1
    ldd iCoef0, iCoef1, piCoefs_p[2]
    maccs Acc0_h, Acc0_l, iData1, iCoef0
    maccs Acc1_h, Acc1_l, iData1, iCoef1

    ldd iData1, iData0, piData_p[1]
    ldd iCoef0, iCoef1, piCoefs_p[3]
    maccs Acc0_h, Acc0_l, iData0, iCoef0
    maccs Acc1_h, Acc1_l, iData0, iCoef1
    ldd iCoef0, iCoef1, piCoefs_p[4]
    maccs Acc0_h, Acc0_l, iData1, iCoef0
    maccs Acc1_h, Acc1_l, iData1, iCoef1

    ldd iData1, iData0, piData_p[2]
    ldd iCoef0, iCoef1, piCoefs_p[5]
    maccs Acc0_h, Acc0_l, iData0, iCoef0
    maccs Acc1_h, Acc1_l, iData0, iCoef1
    ldd iCoef0, iCoef1, piCoefs_p[6]
    maccs Acc0_h, Acc0_l, iData1, iCoef0
    maccs Acc1_h, Acc1_l, iData1, iCoef1

        add piData_p, piData_p, step                         //Back to word alignment

    //Single word data load
    {ldw iData0, piData_p[0]; add piData_p, piData_p, 0x4} // Increment pointer (will not be done until after this bundle)
    ldd iCoef0, iCoef1, piCoefs_p[7]        //Double word coef load
    maccs Acc0_h, Acc0_l, iData0, iCoef0
    maccs Acc1_h, Acc1_l, iData0, iCoef1

    //Increment pointers and branch if still running

    ldc step, 0x40
    {add piCoefs_p, piCoefs_p, step; bt counter, src_mrhf_fir_os_main_loop_odd}


src_mrhf_fir_os_done_odd:
    //pop return value pointer from stack so we can use it to write back
    {ldw iDataRet_p, sp[8]; ldc counter, 31}  //Resuse counter (r3)


    //Write back the two return values
          lsats Acc0_h, Acc0_l, r3            //Saturate first
    lsats Acc1_h, Acc1_l, r3
    lextract iData0, Acc0_h, Acc0_l, counter, 32    //Extract
    lextract iData1, Acc1_h, Acc1_l, counter, 32
    stw iData0, iDataRet_p[1]
    stw iData1, iDataRet_p[0]

    //Pop registers
    ldd r4, r5, sp[0]
    ldd r6, r7, sp[1]
    ldd r8, r9, sp[2]
    ldd r10, r11, sp[3]
  retsp stack_size

.atmp_odd:
    .size    src_mrhf_fir_os_inner_loop_asm_odd, .atmp_odd-src_mrhf_fir_os_inner_loop_asm_odd
    .align    8
    .cc_bottom src_mrhf_fir_os_inner_loop_asm_odd.function

    .set    src_mrhf_fir_os_inner_loop_asm_odd.nstackwords, stack_size
    .globl    src_mrhf_fir_os_inner_loop_asm_odd.nstackwords
    .set    src_mrhf_fir_os_inner_loop_asm_odd.maxcores, 1
    .globl    src_mrhf_fir_os_inner_loop_asm_odd.maxcores
    .set    src_mrhf_fir_os_inner_loop_asm_odd.maxtimers,0
    .globl    src_mrhf_fir_os_inner_loop_asm_odd.maxtimers
    .set    src_mrhf_fir_os_inner_loop_asm_odd.maxchanends,0
    .globl     src_mrhf_fir_os_inner_loop_asm_odd.maxchanends


