// Copyright 2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#define FUNCTION_NAME fir_s32_48t

#define NSTACKWORDS     (1 + 8)
#define STACK_VEC_TMP   (NSTACKWORDS - 8)

#define state           r0
#define coef            r1
#define new_samp        r2
#define indx            r3
#define _32             r4

.text
.issue_mode dual
.globl FUNCTION_NAME;
.type FUNCTION_NAME,@function
.align 16
.cc_top FUNCTION_NAME.function,FUNCTION_NAME

FUNCTION_NAME: // (int32_t * state, int32_t * coef, int32_t new_samp)
        dualentsp NSTACKWORDS

        // r5 - r10 are not used here

        // Setting up the vpu and a poiter to the state[39]
    {   ldc r11, 0                      ;   stw r4, sp[0]                   }
    {   ldc indx, 39                    ;   vsetc r11                       }
    {   ldc _32, 32                     ;                                   }

        ldaw r11, state[indx]   // r11 -> st[39 - 47]
        // It's possible to undef "state" and reuse r0 for "_32" to reduce stack but it does not bring much value

#undef indx
#define buff            r3

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

        // Bring r11 back to state[0], clear the vector unit
    {   add r11, r11, 4                 ;   vclrdr                          } // r11 -> st[0]

    {   ldaw buff, sp[STACK_VEC_TMP]    ;   stw new_samp, state[0]          } // buff -> sp[10] // put new_samp in state[0]

        // Do convolution
    {                                   ;   vldc r11[0]                     }
    {   add r11, r11, _32               ;   vlmaccr coef[0]                 }

    {   add coef, coef, _32             ;   vldc r11[0]                     }
    {   add r11, r11, _32               ;   vlmaccr coef[0]                 }

    {   add coef, coef, _32             ;   vldc r11[0]                     }
    {   add r11, r11, _32               ;   vlmaccr coef[0]                 }
    
    {   add coef, coef, _32             ;   vldc r11[0]                     }
    {   add r11, r11, _32               ;   vlmaccr coef[0]                 }

    {   add coef, coef, _32             ;   vldc r11[0]                     }
    {   add r11, r11, _32               ;   vlmaccr coef[0]                 }

    {   add coef, coef, _32             ;   vldc r11[0]                     }
    {                                   ;   vlmaccr coef[0]                 }
        // vR [0 - 5] has 6 macc results, store them
    {                                   ;   vstr buff[0]                    }

#undef state
#define out             r0

#undef coef
#define tmp0            r1

#undef new_samp
#define tmp1            r2

        // Load all the results and add them together
        ldd out, tmp1, buff[0]

    {   add out, out, tmp1              ;   ldw tmp0, buff[2]               }
    {   add out, out, tmp0              ;   ldw tmp1, buff[3]               }
    {   add out, out, tmp1              ;   ldw tmp0, buff[4]               }
    {   add out, out, tmp0              ;   ldw tmp1, buff[5]               }
    {   add out, out, tmp1              ;   ldw r4, sp[0]                   }
    
        retsp NSTACKWORDS

.cc_bottom FUNCTION_NAME.function; 
.set FUNCTION_NAME.nstackwords,NSTACKWORDS;     .global FUNCTION_NAME.nstackwords; 
.set FUNCTION_NAME.maxcores,1;                  .global FUNCTION_NAME.maxcores; 
.set FUNCTION_NAME.maxtimers,0;                 .global FUNCTION_NAME.maxtimers; 
.set FUNCTION_NAME.maxchanends,0;               .global FUNCTION_NAME.maxchanends; 

#undef FUNCTION_NAME

