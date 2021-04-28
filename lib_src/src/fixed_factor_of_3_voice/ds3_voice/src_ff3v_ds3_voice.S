// Copyright 2016-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
.section  .dp.data,"awd",@progbits
.text

#define STACKWORDS 4

#define c0     r4
#define c1     r5
#define d0     r6

#define s      r11

.cc_top src_ds3_voice_add_sample.function
.globl  src_ds3_voice_add_sample
.align  4
.type src_ds3_voice_add_sample,@function
src_ds3_voice_add_sample://(int64_t sum, int32_t data[], int32_t coefs[], int32_t sample);

.issue_mode dual
  DUALENTSP_lu6 STACKWORDS

  std r4, r5, sp[0]
  std r6, r7, sp[1]

  ldw s, sp[STACKWORDS+1]

  bl src_ff3v_fir_impl

  ldd r4, r5, sp[0]
  ldd r6, r7, sp[1]
  retsp STACKWORDS

.src_ds3_voice_add_sample_tmp:
  .size src_ds3_voice_add_sample, .src_ds3_voice_add_sample_tmp-src_ds3_voice_add_sample
  .align  4
  .cc_bottom src_ds3_voice_add_sample.function

  .set  src_ds3_voice_add_sample.nstackwords, STACKWORDS
  .globl  src_ds3_voice_add_sample.nstackwords
  .set  src_ds3_voice_add_sample.maxcores,1
  .globl  src_ds3_voice_add_sample.maxcores
  .set  src_ds3_voice_add_sample.maxtimers,0
  .globl  src_ds3_voice_add_sample.maxtimers
  .set  src_ds3_voice_add_sample.maxchanends,0
  .globl  src_ds3_voice_add_sample.maxchanends

.cc_top src_ds3_voice_add_final_sample.function
.globl  src_ds3_voice_add_final_sample
.align  4
.type src_ds3_voice_add_final_sample,@function
src_ds3_voice_add_final_sample://(int64_t sum, int32_t data[], int32_t coefs[], int32_t sample);

.issue_mode dual
  DUALENTSP_lu6 STACKWORDS

  std r4, r5, sp[0]
  std r6, r7, sp[1]

  ldw s, sp[STACKWORDS+1]

  bl src_ff3v_fir_impl

  {ldc d0, 31; nop}
  lextract r0, r1, r0, d0, 32

  ldaw s, cp[src_ff3v_fir_comp_ds]
  ldw s, s[0]
  {ldc c0, 0; ldc c1, 0}
  maccs c0, c1, r0, s

  ldaw s, cp[src_ff3v_fir_comp_q_ds]
  ldw s, s[0]

  lsats c0, c1, s
  lextract r0, c0, c1, s, 32

  ldd r4, r5, sp[0]
  ldd r6, r7, sp[1]
  {retsp STACKWORDS; ldc r1, 0}

.src_ds3_voice_add_final_sample_tmp:
  .size src_ds3_voice_add_final_sample, .src_ds3_voice_add_final_sample_tmp-src_ds3_voice_add_final_sample
  .align  4
  .cc_bottom src_ds3_voice_add_final_sample.function

  .set  src_ds3_voice_add_final_sample.nstackwords, STACKWORDS
  .globl  src_ds3_voice_add_final_sample.nstackwords
  .set  src_ds3_voice_add_final_sample.maxcores,1
  .globl  src_ds3_voice_add_final_sample.maxcores
  .set  src_ds3_voice_add_final_sample.maxtimers,0
  .globl  src_ds3_voice_add_final_sample.maxtimers
  .set  src_ds3_voice_add_final_sample.maxchanends,0
  .globl  src_ds3_voice_add_final_sample.maxchanends
