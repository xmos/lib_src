/*
 * inner_loop_asm_unroll.h
 *
 *  Created on: Jul 14, 2015
 *      Author: Ed
 */


#ifndef fir_os_inner_loop_ASM_H_
#define fir_os_inner_loop_ASM_H_

void fir_os_inner_loop_asm_odd(int *piData, int *piCoefs, int iData[], int count);
void fir_os_inner_loop_asm(int *piData, int *piCoefs, int iData[], int count);

#endif
