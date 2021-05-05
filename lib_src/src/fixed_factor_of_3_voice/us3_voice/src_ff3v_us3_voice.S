// Copyright 2017-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
	.section	.dp.data,"awd",@progbits
	.text

#define STACKWORDS 	4

#define sum_l 			r0
#define sum_h       r1
#define data        r2
#define coefs       r3

#define c0					r4
#define c1					r5
#define d0					r6
#define d1					r7

#define s						r11

#define TWO_TAPS_NO_STORE(N) \
	ldd c1, c0, coefs[N];\
	ldd d1, d0, data[N];\
	maccs sum_h, sum_l, c0, d0;\
	maccs sum_h, sum_l, c1, d1;

.cc_top src_us3_voice_input_sample.function
.globl	src_us3_voice_input_sample
.align	4
.type	src_us3_voice_input_sample,@function
src_us3_voice_input_sample://(int32_t data[], int32_t coefs[], int32_t sample);

.issue_mode dual
	DUALENTSP_lu6 STACKWORDS

	std r4, r5, sp[0]
	std r6, r7, sp[1]


	// TODO: fix this by renaming the registers
	mov s, r2
	{mov r2, r0; mov r3, r1}
	{ldc r0, 0; ldc r1, 0}

  bl src_ff3v_fir_impl

	{ldc d0, 31; nop}
	lextract r0, r1, r0, d0, 32

    ldaw s, cp[src_ff3v_fir_comp_us]
    ldw s, s[0]
    {ldc c0, 0; ldc c1, 0}
    maccs c0, c1, r0, s

    ldaw s, cp[src_ff3v_fir_comp_q_us]
    ldw s, s[0]

    lsats c0, c1, s
	lextract r0, c0, c1, s, 32

	ldd r4, r5, sp[0]
	ldd r6, r7, sp[1]
	retsp STACKWORDS

.src_us3_voice_input_sample_tmp:
	.size	src_us3_voice_input_sample, .src_us3_voice_input_sample_tmp-src_us3_voice_input_sample
	.align	4
	.cc_bottom src_us3_voice_input_sample.function

	.set	src_us3_voice_input_sample.nstackwords, STACKWORDS
	.globl	src_us3_voice_input_sample.nstackwords
	.set	src_us3_voice_input_sample.maxcores,1
	.globl	src_us3_voice_input_sample.maxcores
	.set	src_us3_voice_input_sample.maxtimers,0
	.globl	src_us3_voice_input_sample.maxtimers
	.set	src_us3_voice_input_sample.maxchanends,0
	.globl	src_us3_voice_input_sample.maxchanends

.cc_top src_us3_voice_get_next_sample.function
.globl	src_us3_voice_get_next_sample
.align	4
.type	src_us3_voice_get_next_sample,@function
src_us3_voice_get_next_sample://(int32_t data[], int32_t coefs[], int32_t sample);

.issue_mode dual
	DUALENTSP_lu6 STACKWORDS

	std r4, r5, sp[0]
	std r6, r7, sp[1]

	// TODO: fix this by renaming the registers
	{mov r2, r0; mov r3, r1}
	{ldc r0, 0; ldc r1, 0}

	TWO_TAPS_NO_STORE(0)
	TWO_TAPS_NO_STORE(1)
	TWO_TAPS_NO_STORE(2)
	TWO_TAPS_NO_STORE(3)
	TWO_TAPS_NO_STORE(4)
	TWO_TAPS_NO_STORE(5)
	TWO_TAPS_NO_STORE(6)
	TWO_TAPS_NO_STORE(7)
	TWO_TAPS_NO_STORE(8)
	TWO_TAPS_NO_STORE(9)
	TWO_TAPS_NO_STORE(10)
	TWO_TAPS_NO_STORE(11)

	{ldc d0, 31; nop}
	lextract r0, r1, r0, d0, 32

    ldaw s, cp[src_ff3v_fir_comp_us]
    ldw s, s[0]
    {ldc c0, 0; ldc c1, 0}
    maccs c0, c1, r0, s

    ldaw s, cp[src_ff3v_fir_comp_q_us]
    ldw s, s[0]

    lsats c0, c1, s
	lextract r0, c0, c1, s, 32

	ldd r4, r5, sp[0]
	ldd r6, r7, sp[1]
	retsp STACKWORDS

.src_us3_voice_get_next_sample_tmp:
	.size	src_us3_voice_get_next_sample, .src_us3_voice_get_next_sample_tmp-src_us3_voice_get_next_sample
	.align	4
	.cc_bottom src_us3_voice_get_next_sample.function

	.set	src_us3_voice_get_next_sample.nstackwords, STACKWORDS
	.globl	src_us3_voice_get_next_sample.nstackwords
	.set	src_us3_voice_get_next_sample.maxcores,1
	.globl	src_us3_voice_get_next_sample.maxcores
	.set	src_us3_voice_get_next_sample.maxtimers,0
	.globl	src_us3_voice_get_next_sample.maxtimers
	.set	src_us3_voice_get_next_sample.maxchanends,0
	.globl	src_us3_voice_get_next_sample.maxchanends
