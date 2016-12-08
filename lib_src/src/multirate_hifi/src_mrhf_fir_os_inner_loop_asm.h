// Copyright (c) 2016, XMOS Ltd, All rights reserved
#ifndef SRC_MRHF_FIR_OS_INNER_LOOP_ASM_H_
#define SRC_MRHF_FIR_OS_INNER_LOOP_ASM_H_

void src_mrhf_fir_os_inner_loop_asm_odd(int *piData, int *piCoefs, int iData[], int count);
void src_mrhf_fir_os_inner_loop_asm(int *piData, int *piCoefs, int iData[], int count);

#endif // SRC_MRHF_FIR_OS_INNER_LOOP_ASM_H_
