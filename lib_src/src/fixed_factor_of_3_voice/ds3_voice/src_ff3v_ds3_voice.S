// Copyright (c) 2016, XMOS Ltd, All rights reserved
	.section	.dp.data,"awd",@progbits
	.text

#define STACKWORDS 4

#define sum_l 			    r0
#define sum_h               r1
#define data                r2
#define coefs               r3

#define c0					r4
#define c1					r5
#define d0					r6
#define d1					r7

#define s					r11

#define EVEN_STEP(N) \
	ldd c1, c0, coefs[N];\
	maccs sum_h, sum_l, c0, s;\
	ldd d1, d0, data[N];\
	maccs sum_h, sum_l, c1, d0;\
	std	d0, s, data[N];

#define ODD_STEP(N) \
	ldd c1, c0, coefs[N];\
	maccs sum_h, sum_l, c0, d1;\
	ldd s, d0, data[N];\
	maccs sum_h, sum_l, c1, d0;\
	std	d0, d1, data[N];

.cc_top src_ds3_voice_impl.function
.globl	src_ds3_voice_impl
.align	8
.type	src_ds3_voice_impl,@function
.issue_mode dual
src_ds3_voice_impl:
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
.src_ds3_impl_tmp:
	.size	src_ds3_voice_impl, .src_ds3_impl_tmp-src_ds3_voice_impl
	.align	4
	.cc_bottom src_ds3_voice_impl.function

	.set	src_ds3_voice_impl.nstackwords, 0
	.globl	src_ds3_voice_impl.nstackwords
	.set	src_ds3_voice_impl.maxcores,1
	.globl	src_ds3_voice_impl.maxcores
	.set	src_ds3_voice_impl.maxtimers,0
	.globl	src_ds3_voice_impl.maxtimers
	.set	src_ds3_voice_impl.maxchanends,0
	.globl	src_ds3_voice_impl.maxchanends



.cc_top src_ds3_voice_add_sample.function
.globl	src_ds3_voice_add_sample
.align	4
.type	src_ds3_voice_add_sample,@function
src_ds3_voice_add_sample://(int64_t sum, int32_t data[], int32_t coefs[], int32_t sample);

.issue_mode dual
	DUALENTSP_lu6 STACKWORDS

	std r4, r5, sp[0]
	std r6, r7, sp[1]

	ldw s, sp[STACKWORDS+1]

    bl src_ds3_voice_impl

	ldd r4, r5, sp[0]
	ldd r6, r7, sp[1]
	retsp STACKWORDS

.src_ds3_voice_add_sample_tmp:
	.size	src_ds3_voice_add_sample, .src_ds3_voice_add_sample_tmp-src_ds3_voice_add_sample
	.align	4
	.cc_bottom src_ds3_voice_add_sample.function

	.set	src_ds3_voice_add_sample.nstackwords, STACKWORDS
	.globl	src_ds3_voice_add_sample.nstackwords
	.set	src_ds3_voice_add_sample.maxcores,1
	.globl	src_ds3_voice_add_sample.maxcores
	.set	src_ds3_voice_add_sample.maxtimers,0
	.globl	src_ds3_voice_add_sample.maxtimers
	.set	src_ds3_voice_add_sample.maxchanends,0
	.globl	src_ds3_voice_add_sample.maxchanends

.cc_top src_ds3_voice_add_final_sample.function
.globl	src_ds3_voice_add_final_sample
.align	4
.type	src_ds3_voice_add_final_sample,@function
src_ds3_voice_add_final_sample://(int64_t sum, int32_t data[], int32_t coefs[], int32_t sample);

.issue_mode dual
	DUALENTSP_lu6 STACKWORDS

	std r4, r5, sp[0]
	std r6, r7, sp[1]

	ldw s, sp[STACKWORDS+1]

    bl src_ds3_voice_impl

	{ldc d0, 31; nop}
	lextract r0, r1, r0, d0, 32

    ldaw s, cp[src_ff3v_ds3_voice_fir_comp]
    ldw s, s[0]
    {ldc c0, 0; ldc c1, 0}
    maccs c0, c1, r0, s

    ldaw s, cp[src_ff3v_ds3_voice_fir_comp_q]
    ldw s, s[0]

    lsats c0, c1, s
	lextract r0, c0, c1, s, 32


	ldd r4, r5, sp[0]
	ldd r6, r7, sp[1]
	{retsp STACKWORDS; ldc r1, 0}

.src_ds3_voice_add_final_sample_tmp:
	.size	src_ds3_voice_add_final_sample, .src_ds3_voice_add_final_sample_tmp-src_ds3_voice_add_final_sample
	.align	4
	.cc_bottom src_ds3_voice_add_final_sample.function

	.set	src_ds3_voice_add_final_sample.nstackwords, STACKWORDS
	.globl	src_ds3_voice_add_final_sample.nstackwords
	.set	src_ds3_voice_add_final_sample.maxcores,1
	.globl	src_ds3_voice_add_final_sample.maxcores
	.set	src_ds3_voice_add_final_sample.maxtimers,0
	.globl	src_ds3_voice_add_final_sample.maxtimers
	.set	src_ds3_voice_add_final_sample.maxchanends,0
	.globl	src_ds3_voice_add_final_sample.maxchanends
