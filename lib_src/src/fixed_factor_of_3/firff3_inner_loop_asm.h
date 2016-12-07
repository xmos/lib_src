// Copyright (c) 2016, XMOS Ltd, All rights reserved
#ifndef _firff3_inner_loop_ASM_H_
#define _firff3_inner_loop_ASM_H_

#define N_LOOPS_PER_ASM   12

void firff3_inner_loop_asm(int *piData, int *piCoefs, int iData[], int count);
void firff3_inner_loop_asm_odd(int *piData, int *piCoefs, int iData[], int count);


#endif // _firff3_inner_loop_ASM_H_
