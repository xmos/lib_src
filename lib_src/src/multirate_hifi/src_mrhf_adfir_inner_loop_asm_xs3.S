// Copyright 2016-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#if defined(__XS3A__)
    .section    .dp.data,"awd",@progbits
    .text

#define NSTACKWORDS        8
    .issue_mode dual
// TODO: share with others
ones:
    .word 0x40000000,  0x40000000, 0x40000000,  0x40000000
    .word 0x40000000,  0x40000000, 0x40000000,  0x40000000
shifts0:
    .word 0, 0, 0, 0, 0, 0, 0, 0

    .align    16

    .cc_top src_mrhf_adfir_inner_loop_asm_xs3.function
    .globl    src_mrhf_adfir_inner_loop_asm_xs3
    .align    8
    .type    src_mrhf_adfir_inner_loop_asm_xs3,@function

src_mrhf_adfir_inner_loop_asm_xs3:
    { DUALENTSP_u6 NSTACKWORDS    ; ldc r11, 0 }
    { vsetc r11                   ; shr r3, r3, 3}
    { ldc r11, 0x20               ; vclrdr }
    { bf r3, src_mrhf_adfir_done2 ; sub r3, r3, 1 }

src_mrhf_adfir_main_loop2:
    { vldc    r0[0]               ; add r0, r0, r11   }
    { vlmaccr r1[0]               ; add r1, r1, r11 }
    { vldc    r0[0]               ; add r0, r0, r11   }
    { vlmaccr r1[0]               ; add r1, r1, r11 }
    { bt r3, src_mrhf_adfir_main_loop2 ;     sub r3, r3, 1 }

src_mrhf_adfir_done2:
    ldap      r11, shifts0
    { vlsat   r11[0]             ; ldaw    r0, sp[0] }
    vstr      r0[0]
    { vldc    r0[0]              ; ldap    r11, ones }
    vclrdr
    { vlmaccr r11[0]             ; ldap    r11, shifts0   }
    { vlsat   r11[0]             ; mkmsk r0, 4 }
    vstrpv    r2[0], r0

    retsp NSTACKWORDS

.atmp:
    .size    src_mrhf_adfir_inner_loop_asm_xs3, .atmp-src_mrhf_adfir_inner_loop_asm_xs3
    .align    8
    .cc_bottom src_mrhf_adfir_inner_loop_asm_xs3.function

    .set    src_mrhf_adfir_inner_loop_asm_xs3.nstackwords, NSTACKWORDS
    .globl    src_mrhf_adfir_inner_loop_asm_xs3.nstackwords
    .set    src_mrhf_adfir_inner_loop_asm_xs3.maxcores, 1
    .globl    src_mrhf_adfir_inner_loop_asm_xs3.maxcores
    .set    src_mrhf_adfir_inner_loop_asm_xs3.maxtimers,0
    .globl    src_mrhf_adfir_inner_loop_asm_xs3.maxtimers
    .set    src_mrhf_adfir_inner_loop_asm_xs3.maxchanends,0
    .globl    src_mrhf_adfir_inner_loop_asm_xs3.maxchanends


#endif
