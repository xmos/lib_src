// Copyright 2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#define FUNCTION_NAME conv_s32_24t

#define NSTACKWORDS     (8)
#define STACK_VEC_TMP   (NSTACKWORDS - 8)

#define state           r0
#define coef            r1
#define buff            r2
#define _32             r3

.text
.issue_mode dual
.globl FUNCTION_NAME;
.type FUNCTION_NAME,@function
.align 16
.cc_top FUNCTION_NAME.function,FUNCTION_NAME

FUNCTION_NAME: // (int32_t * state, int32_t * coef)
        dualentsp NSTACKWORDS

        // r4 - r10 are not used here

        // Setting up the vpu and some constants
    {   ldaw buff, sp[STACK_VEC_TMP]    ;   ldc r11, 0                      }
    {   ldc _32, 32                     ;   vsetc r11                       }
    {   vclrdr                          ;   mov r11, state                  }

        // Do convolution
    {                                   ;   vldc r11[0]                     }
    {   add r11, r11, _32               ;   vlmaccr coef[0]                 }

    {   add coef, coef, _32             ;   vldc r11[0]                     }
    {   add r11, r11, _32               ;   vlmaccr coef[0]                 }

    {   add coef, coef, _32             ;   vldc r11[0]                     }
    {                                   ;   vlmaccr coef[0]                 }
        // vR [0 - 2] has 3 macc results, store them
    {                                   ;   vstr buff[0]                    }

#undef state
#define out         r0

#undef coef
#define tmp0        r1

#undef _32
#define tmp1        r3

        // Load all the results and add them together
        ldd out, tmp1, buff[0]

    {   add out, out, tmp1              ;   ldw tmp0, buff[2]               }
    {   add out, out, tmp0              ;                                   }

        retsp NSTACKWORDS

.cc_bottom FUNCTION_NAME.function; 
.set FUNCTION_NAME.nstackwords,NSTACKWORDS;     .global FUNCTION_NAME.nstackwords; 
.set FUNCTION_NAME.maxcores,1;                  .global FUNCTION_NAME.maxcores; 
.set FUNCTION_NAME.maxtimers,0;                 .global FUNCTION_NAME.maxtimers; 
.set FUNCTION_NAME.maxchanends,0;               .global FUNCTION_NAME.maxchanends; 

#undef FUNCTION_NAME
