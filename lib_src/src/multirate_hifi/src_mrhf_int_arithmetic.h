// Copyright 2016-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef _SRC_MRHF_INT_ARITHEMTIC_H_
#define _SRC_MRHF_INT_ARITHEMTIC_H_

    // ===========================================================================
    //
    // Defines
    //
    // ===========================================================================

#define __int64         long long

    // ===========================================================================
    //
    // Variables
    //
    // ===========================================================================


    // ===========================================================================
    //
    // TypeDefs
    //
    // ===========================================================================
    // To avoid C type definitions when including this file from assembler
    #ifndef INCLUDE_FROM_ASM


        // ===========================================================================
        //
        // Function prototypes
        //
        // ===========================================================================

        // ==================================================================== //
        // Function:        MACC                                                //
        // Description:        32i x 32i -> 64i Multiply-Accumulate                //
        // ==================================================================== //
        void    MACC(__int64* plAcc, int ix, int iy);

        // ==================================================================== //
        // Function:        LMUL                                                //
        // Description:        32i x 32i +32i + 32i -> 64i Multiply (and add)        //
        // ==================================================================== //
        void    LMUL(__int64* plAcc, int ix, int iy, int ic, int id);

        // ==================================================================== //
        // Function:        LATS30                                                //
        // Description:        Saturate 64i to 2^62-1, -2^62                        //
        // ==================================================================== //
        #define        MAX_VAL64_30    (__int64)0x3FFFFFFFFFFFFFFF
        #define        MIN_VAL64_30    (__int64)0xC000000000000000
        void    LSAT30(__int64 *plx);

        // ==================================================================== //
        // Function:        EXT30                                                //
        // Description:        Extracts bits [62-31] into 32i                        //
        // ==================================================================== //
        void    EXT30(int *pr, __int64 lx);

        // ==================================================================== //
        // Function:        LATS29                                                //
        // Description:        Saturate 64i to 2^61-1, -2^61                        //
        // ==================================================================== //
        #define        MAX_VAL64_29    (__int64)0x1FFFFFFFFFFFFFFF
        #define        MIN_VAL64_29    (__int64)0xE000000000000000
        void    LSAT29(__int64 *plx);

        // ==================================================================== //
        // Function:        EXT29                                                //
        // Description:        Extracts bits [61-30] into 32i                        //
        // ==================================================================== //
        void    EXT29(int *pr, __int64 lx);

    #endif // nINCLUDE_FROM_ASM

#endif // _SRC_MRHF_INT_ARITHEMTIC_H_
