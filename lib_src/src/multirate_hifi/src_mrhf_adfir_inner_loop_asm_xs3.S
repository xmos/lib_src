// Copyright 2016-2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#if defined(__XS3A__)
    .text

#define NSTACKWORDS        8
    .issue_mode dual

    .align    16

    .cc_top src_mrhf_adfir_inner_loop_asm_xs3.function
    .globl    src_mrhf_adfir_inner_loop_asm_xs3
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
    ldap      r11, shifts1
    { vlsat   r11[0]             ; ldaw    r0, sp[0] }
    vstr      r0[0]
    { vldc    r0[0]              ; ldap    r11, twos }
    vclrdr
    { vlmaccr r11[0]             ; ldap    r11, shifts0   }
    { vlsat   r11[0]             ; mkmsk r0, 4 }
    vstrpv    r2[0], r0

    retsp NSTACKWORDS

.atmp:
    .size    src_mrhf_adfir_inner_loop_asm_xs3, .atmp-src_mrhf_adfir_inner_loop_asm_xs3

    .set    src_mrhf_adfir_inner_loop_asm_xs3.nstackwords, NSTACKWORDS
    .globl    src_mrhf_adfir_inner_loop_asm_xs3.nstackwords
    .set    src_mrhf_adfir_inner_loop_asm_xs3.maxcores, 1
    .globl    src_mrhf_adfir_inner_loop_asm_xs3.maxcores
    .set    src_mrhf_adfir_inner_loop_asm_xs3.maxtimers,0
    .globl    src_mrhf_adfir_inner_loop_asm_xs3.maxtimers
    .set    src_mrhf_adfir_inner_loop_asm_xs3.maxchanends,0
    .globl    src_mrhf_adfir_inner_loop_asm_xs3.maxchanends

    .globl    src_mrhf_fir_inner_loop_asm_xs3
    .type    src_mrhf_fir_inner_loop_asm_xs3,@function

#undef NSTACKWORDS
#define NSTACKWORDS        8


twos:
    .word 0x7fffffff,  0x7fffffff, 0x7fffffff,  0x7fffffff
    .word 0x7fffffff,  0x7fffffff, 0x7fffffff,  0x7fffffff
shifts0:
    .word 0, 0, 0, 0, 0, 0, 0, 0
shifts1:
    .word 1, 1, 1, 1, 1, 1, 1, 1
shifts2:
    .word 2, 2, 2, 2, 2, 2, 2, 2

    .align    16

src_mrhf_fir_inner_loop_asm_xs3:
    { DUALENTSP_u6 NSTACKWORDS    ; ldc r11, 0 }
    { vsetc r11                   ; shr r3, r3, 3}
    { ldc r11, 0x20               ; vclrdr }
    { bf r3, src_mrhf_fir_done2   ; sub r3, r3, 1 }

src_mrhf_fir_main_loop2:
    { vldc    r0[0]               ; add r0, r0, r11   }
    { vlmaccr r1[0]               ; add r1, r1, r11 }
    { vldc    r0[0]               ; add r0, r0, r11   }
    { vlmaccr r1[0]               ; add r1, r1, r11 }
    { bt r3, src_mrhf_fir_main_loop2 ;     sub r3, r3, 1 }

src_mrhf_fir_done2:
    ldap      r11, shifts2
    { vlsat   r11[0]             ; ldaw    r0, sp[0] }
    vstr      r0[0]
    { vldc    r0[0]              ; ldap    r11, twos }
    vclrdr
    { vlmaccr r11[0]             ; ldap    r11, shifts0   }
    { vlsat   r11[0]             ; mkmsk r0, 4 }
    vstrpv    r2[0], r0

    retsp NSTACKWORDS

.btmp:
    .size    src_mrhf_fir_inner_loop_asm_xs3, .btmp-src_mrhf_fir_inner_loop_asm_xs3

    .set    src_mrhf_fir_inner_loop_asm_xs3.nstackwords, NSTACKWORDS
    .globl    src_mrhf_fir_inner_loop_asm_xs3.nstackwords
    .set    src_mrhf_fir_inner_loop_asm_xs3.maxcores, 1
    .globl    src_mrhf_fir_inner_loop_asm_xs3.maxcores
    .set    src_mrhf_fir_inner_loop_asm_xs3.maxtimers,0
    .globl    src_mrhf_fir_inner_loop_asm_xs3.maxtimers
    .set    src_mrhf_fir_inner_loop_asm_xs3.maxchanends,0
    .globl     src_mrhf_fir_inner_loop_asm_xs3.maxchanends

    .globl    src_mrhf_fir_os_inner_loop_asm_xs3
    .type    src_mrhf_fir_os_inner_loop_asm_xs3,@function

#undef NSTACKWORDS
#define NSTACKWORDS        8

evens:
    .word 0x7fffffff
odds:
    .word 0, 0x7fffffff, 0, 0x7fffffff, 0, 0x7fffffff, 0, 0x7fffffff

    .align    16
src_mrhf_fir_os_inner_loop_asm_xs3:
    
    { DUALENTSP_u6 NSTACKWORDS     ; ldc r11, 0x0 }
    { vsetc r11                    ; shr r3, r3, 2 }
    { ldc r11, 0x20                ; vclrdr }
    { bf r3, src_mrhf_fir_os_done2 ; sub r3, r3, 1 }

src_mrhf_fir_os_main_loop2:

    { vldc r0[0]          ; add r0, r0, r11 }
    { vlmaccr r1[0]       ; add r1, r1, r11 }
    { vlmaccr r1[0]       ; add r1, r1, r11 }

    //Decrement loop r3
    { bt r3, src_mrhf_fir_os_main_loop2 ; sub r3, r3, 1 }

src_mrhf_fir_os_done2:
    ldap      r11, shifts2
    { vlsat   r11[0]       ; ldaw    r0, sp[0] }
    vstr      r0[0]
    { vldc    r0[0]        ; ldap    r11, evens }
    vclrdr
    { vlmaccr r11[0]       ; add r11, r11, 4 }
    { vlmaccr r11[0]       ; ldap    r11, shifts0   }
    { vlsat   r11[0]       ; mkmsk r0, 8 }
    vstrpv    r2[0], r0

    //Pop registers
    retsp NSTACKWORDS

.ctmp:
    .size      src_mrhf_fir_os_inner_loop_asm_xs3, .ctmp-src_mrhf_fir_os_inner_loop_asm_xs3

    .set      src_mrhf_fir_os_inner_loop_asm_xs3.nstackwords, NSTACKWORDS
    .globl    src_mrhf_fir_os_inner_loop_asm_xs3.nstackwords
    .set      src_mrhf_fir_os_inner_loop_asm_xs3.maxcores, 1
    .globl    src_mrhf_fir_os_inner_loop_asm_xs3.maxcores
    .set      src_mrhf_fir_os_inner_loop_asm_xs3.maxtimers,0
    .globl    src_mrhf_fir_os_inner_loop_asm_xs3.maxtimers
    .set      src_mrhf_fir_os_inner_loop_asm_xs3.maxchanends,0
    .globl    src_mrhf_fir_os_inner_loop_asm_xs3.maxchanends

#define FILTER_DEFS_ADFIR_N_PHASES            128 
    .globl    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3
    .align    16
    .type    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3,@function

#undef NSTACKWORDS
#define NSTACKWORDS        8

src_mrhf_spline_coeff_gen_inner_loop_asm_xs3:
    DUALENTSP_lu6 NSTACKWORDS

    ldc r3, (FILTER_DEFS_ADFIR_N_PHASES + 2)<< 2
    // p0, p1, p2, p3, ..., p15
    // p0+N, p1+N, p2+N, ....,p15+N
    // p0+2N, p1+2N, p2+2N, ....,p15+2N
    // p0+3N, p1+3N, p2+3N, ....,p15+3N
    // ...
    // p0+257N, p1+257N, p2+257N, ....,p15+257N

    //           ****
    // p0, p0+N, p0+2N, ...., p0+257N
    // p1, p1+N, p1+2N, ...., p1+257N
    // ...
    
// a0 = p[0] * H[2] + p[0+N] * H[1] * p[0+2N] * H[0]
// a1 = p[1] * H[2] + p[1+N] * H[1] * p[1+2N] * H[0]
// a2 = p[2] * H[2] + p[2+N] * H[1] * p[2+2N] * H[0]
// a3 = p[3] * H[2] + p[3+N] * H[1] * p[3+2N] * H[0]

    // Ensure that H is padded with five zeroes.
    // There may be a faster way to achieve this with VLMUL
    
    { vldd  r1[0]                 ; ldaw  r1, sp[0] }
    { vstd  r1[0]                 ; ldc   r11, 0    }
    vsetc r11
    stw   r11, sp[3]
    std   r11, r11, sp[2]
    std   r11, r11, sp[3]
src_mrhf_spline_coeff_gen_main_loop2:
    { vclrdr                       ; ldap  r11, shifts2 }
    vldc r1[0]                   

    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    vlsat r11[0]
    vstr r2[0]
    ldaw r2, r2[8]
    vclrdr
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    { vlmaccr  r0[0]               ; add r0, r0, r3 }
    vlsat r11[0]
    vstr r2[0]
src_mrhf_spline_coeff_gen_done2:
    retsp NSTACKWORDS

.dtmp:
    .size    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3, .dtmp-src_mrhf_spline_coeff_gen_inner_loop_asm_xs3
    .cc_bottom src_mrhf_adfir_inner_loop_asm_xs3.function

    .set    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3.nstackwords, NSTACKWORDS
    .globl    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3.nstackwords
    .set    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3.maxcores, 1
    .globl    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3.maxcores
    .set    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3.maxtimers,0
    .globl    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3.maxtimers
    .set    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3.maxchanends,0
    .globl     src_mrhf_spline_coeff_gen_inner_loop_asm_xs3.maxchanends
#endif
