// Copyright (c) 2016, XMOS Ltd, All rights reserved
#ifndef _SRC_MRHF_FIR_INNER_LOOP_ASM_H_
#define _SRC_MRHF_FIR_INNER_LOOP_ASM_H_

void src_mrhf_fir_inner_loop_asm(int *piData, int *piCoefs, int iData[], int count);
void src_mrhf_fir_inner_loop_asm_odd(int *piData, int *piCoefs, int iData[], int count);

#endif // _SRC_MRHF_FIR_INNER_LOOP_ASM_H_
