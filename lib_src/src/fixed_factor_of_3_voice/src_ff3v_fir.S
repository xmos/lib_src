// Copyright 2017-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
.section  .dp.data,"awd",@progbits
.text

#define sum_l  r0
#define sum_h  r1
#define data   r2
#define coefs  r3

#define c0     r4
#define c1     r5
#define d0     r6
#define d1     r7

#define s      r11

#define EVEN_STEP(N) \
  ldd c1, c0, coefs[N];\
  maccs sum_h, sum_l, c0, s;\
  ldd d1, d0, data[N];\
  maccs sum_h, sum_l, c1, d0;\
  std d0, s, data[N];

#define ODD_STEP(N) \
  ldd c1, c0, coefs[N];\
  maccs sum_h, sum_l, c0, d1;\
  ldd s, d0, data[N];\
  maccs sum_h, sum_l, c1, d0;\
  std d0, d1, data[N];

.cc_top src_ff3v_fir_impl.function
.globl  src_ff3v_fir_impl
.align  8
.type src_ff3v_fir_impl,@function
.issue_mode dual
src_ff3v_fir_impl:
  EVEN_STEP(0);
  ODD_STEP(1);
  EVEN_STEP(2);
  ODD_STEP(3);
  EVEN_STEP(4);
  ODD_STEP(5);
  EVEN_STEP(6);
  ODD_STEP(7);
  EVEN_STEP(8);
  ODD_STEP(9);
  EVEN_STEP(10);
  ODD_STEP(11);
  {retsp 0; nop}
.src_ff3v_fir_impl_tmp:
  .size src_ff3v_fir_impl, .src_ff3v_fir_impl_tmp-src_ff3v_fir_impl
  .align  4
  .cc_bottom src_ff3v_fir_impl.function

  .set  src_ff3v_fir_impl.nstackwords, 0
  .globl  src_ff3v_fir_impl.nstackwords
  .set  src_ff3v_fir_impl.maxcores,1
  .globl  src_ff3v_fir_impl.maxcores
  .set  src_ff3v_fir_impl.maxtimers,0
  .globl  src_ff3v_fir_impl.maxtimers
  .set  src_ff3v_fir_impl.maxchanends,0
  .globl  src_ff3v_fir_impl.maxchanends
