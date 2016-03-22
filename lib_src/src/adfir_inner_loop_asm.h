// Copyright (c) 2016, XMOS Ltd, All rights reserved
/*
 * inner_loop_asm_unroll.h
 *
 *  Created on: Jul 14, 2015
 *      Author: Ed
 */


#ifndef adfir_inner_loop_ASM_H_
#define adfir_inner_loop_ASM_H_

void adfir_inner_loop_asm(int *piData, int *piCoefs, int iData[], int count);
void adfir_inner_loop_asm_odd(int *piData, int *piCoefs, int iData[], int count);

#endif
