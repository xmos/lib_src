// Copyright (c) 2016, XMOS Ltd, All rights reserved

#ifndef fir_os_inner_loop_ASM_H_
#define fir_os_inner_loop_ASM_H_

void fir_os_inner_loop_asm_odd(int *piData, int *piCoefs, int iData[], int count);
void fir_os_inner_loop_asm(int *piData, int *piCoefs, int iData[], int count);

#endif
