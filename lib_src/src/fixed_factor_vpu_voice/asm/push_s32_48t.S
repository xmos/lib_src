// Copyright 2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#define FUNCTION_NAME push_s32_48t

#define NSTACKWORDS     (0)

#define state           r0
#define new_samp        r1
#define indx            r2
#define _32             r3

.text
.issue_mode dual
.globl FUNCTION_NAME;
.type FUNCTION_NAME,@function
.align 16
.cc_top FUNCTION_NAME.function,FUNCTION_NAME

FUNCTION_NAME: // (int32_t * state, int32_t new_samp)
        dualentsp NSTACKWORDS

        // r4 - r10 are not used here

        // Setting up the vpu and a poiter to the state[39]
    {   ldc _32, 32                     ;   ldc r11, 0                      }
    {   ldc indx, 39                    ;   vsetc r11                       }
    {   ldaw r11, state[indx]           ;                                   } // r11 -> st[39 - 47]

#undef indx
#define buff            r2

    {   add buff, r11, 4                ;   vldr r11[0]                     } // buff -> st[40 - 48] // vR has st[39 - 47]
    {   sub r11, r11, _32               ;   vstr buff[0]                    } // r11  -> st[31 - 39] // buff has vR

    {   add buff, r11, 4                ;   vldr r11[0]                     } // buff -> st[32 - 40] // vR has st[31 - 39]
    {   sub r11, r11, _32               ;   vstr buff[0]                    } // r11  -> st[23 - 31] // buff has vR

    {   add buff, r11, 4                ;   vldr r11[0]                     } // buff -> st[24 - 32] // vR has st[23 - 31]
    {   sub r11, r11, _32               ;   vstr buff[0]                    } // r11  -> st[15 - 23] // buff has vR

    {   add buff, r11, 4                ;   vldr r11[0]                     } // buff -> st[16 - 24] // vR has st[15 - 23]
    {   sub r11, r11, _32               ;   vstr buff[0]                    } // r11  -> st[7  - 15] // buff has vR

    {   add buff, r11, 4                ;   vldr r11[0]                     } // buff -> st[8  - 16] // vR has st[7  - 15]
    {   sub r11, r11, _32               ;   vstr buff[0]                    } // r11  -> st[-1 -  7] // buff has vR

    {   add buff, r11, 4                ;   vldr r11[0]                     } // buff -> st[0  -  8] // vR has st[-1 -  7]
    {                                   ;   vstr buff[0]                    } // buff has vR

        // put new_samp in state[0]
    {                                   ;   stw new_samp, state[0]          }


        retsp NSTACKWORDS

.cc_bottom FUNCTION_NAME.function; 
.set FUNCTION_NAME.nstackwords,NSTACKWORDS;     .global FUNCTION_NAME.nstackwords; 
.set FUNCTION_NAME.maxcores,1;                  .global FUNCTION_NAME.maxcores; 
.set FUNCTION_NAME.maxtimers,0;                 .global FUNCTION_NAME.maxtimers; 
.set FUNCTION_NAME.maxchanends,0;               .global FUNCTION_NAME.maxchanends; 

#undef FUNCTION_NAME

