// Copyright 2016-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef SRC_MRHF_FIR_OS_INNER_LOOP_ASM_H_
#define SRC_MRHF_FIR_OS_INNER_LOOP_ASM_H_

void src_mrhf_fir_os_inner_loop_asm_odd(int *piData, int *piCoefs, int iData[], int count);
void src_mrhf_fir_os_inner_loop_asm(int *piData, int *piCoefs, int iData[], int count);

#endif // SRC_MRHF_FIR_OS_INNER_LOOP_ASM_H_
