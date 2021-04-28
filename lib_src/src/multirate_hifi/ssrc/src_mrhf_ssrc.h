// Copyright 2016-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// ===========================================================================
// ===========================================================================
//
// Top level definition file for the SSRC
//
// Target:    MS Windows
// Version: 1.0
//
// ===========================================================================
// ===========================================================================

#ifndef _SRC_MRHF_SSRC_H_
#define _SRC_MRHF_SSRC_H_

    // ===========================================================================
    //
    // Includes
    //
    // ===========================================================================
    #include "src_mrhf_fir.h"
    #include "src_mrhf_filter_defs.h"
    #include "src.h"

    // ===========================================================================
    //
    // Defines
    //
    // ===========================================================================

    // General defines
    // ---------------
    #define        SSRC_STACK_LENGTH_MULT                (SSRC_N_CHANNELS * SSRC_N_IN_SAMPLES)    // Multiplier for stack length (stack length = this value x the number of input samples to process)


    // Parameter values
    // ----------------
    #define        SSRC_DITHER_OFF                    0
    #define        SSRC_DITHER_ON                    1
    #define        SSRC_DITHER_ON_OFF_MIN                SSRC_DITHER_OFF
    #define        SSRC_DITHER_ON_OFF_MAX                SSRC_DITHER_ON




    // ===========================================================================
    //
    // Defines
    //
    // ===========================================================================


    // ===========================================================================
    //
    // TypeDefs
    //
    // ===========================================================================

    // To avoid C type definitions when including this file from assembler
    #ifndef INCLUDE_FROM_ASM

        // SSRC Return Codes
        // -----------------
        typedef enum _SSRCReturnCodes
        {
            SSRC_NO_ERROR                            = 0,
            SSRC_ERROR                                = 1
        } SSRCReturnCodes_t;


        // Sampling rate codes
        // -------------------
        typedef enum _SSRCFs
        {
            SSRC_FS_44                                = 0,                                // Fs = 44.1kHz code
            SSRC_FS_48                                = 1,                                // Fs = 48kHz code
            SSRC_FS_88                                = 2,                                // Fs = 88.2kHz code
            SSRC_FS_96                                = 3,                                // Fs = 96kHz code
            SSRC_FS_176                                = 4,                                // Fs = 176.4kHz code
            SSRC_FS_192                                = 5,                                // Fs = 192kHz code
        } SSRCFs_t;
        #define        SSRC_N_FS                        (SSRC_FS_192 + 1)
        #define        SSRC_FS_MIN                        SSRC_FS_44
        #define        SSRC_FS_MAX                        SSRC_FS_192


        // SSRC Filters IDs structure
        // --------------------------
        #define        SSRC_F1_INDEX                    0
        #define        SSRC_F2_INDEX                    1
        #define        SSRC_F3_INDEX                    2
        #define        SSRC_N_F                        (SSRC_F3_INDEX + 1)

        typedef struct _SSRCFiltersIDs
        {
            unsigned int        uiFID[SSRC_N_F];
            unsigned int        uiPPFIRPhaseStep;
        } SSRCFiltersIDs_t;


        // SSRC State structure
        // --------------------
        typedef struct _SSRCState
        {
            long long                               pad_to_64b_alignment;
            int                                        iDelayFIRLong[2 * FILTER_DEFS_FIR_MAX_TAPS_LONG];        // Doubled length for circular buffer simulation
            int                                        iDelayFIRShort[2 * FILTER_DEFS_FIR_MAX_TAPS_SHORT];        // Doubled length for circular buffer simulation
            int                                        iDelayPPFIR[2 * FILTER_DEFS_PPFIR_PHASE_MAX_TAPS];        // Doubled length for circular buffer simulation
            unsigned int                            uiRndSeed;                                              // Dither random seeds current values

        } ssrc_state_t;


        // SSRC Control structure
        // ----------------------
        typedef struct _SSRCCtrl
        {
#ifdef __XC__
            long long                               pad_to_64b_alignment;               //Force compiler to 64b align
            unsigned int                            uiNchannels;                        // Number of channels in this instance
            int* unsafe                                piIn;                                // Input buffer pointer (PCM, 32bits, 2 channels time domain interleaved data)
            unsigned int                            uiNInSamples;                        // Number of input samples to process in one call to the processing function
            SSRCFs_t                                eInFs;                                // Input sampling rate code
            int* unsafe                                piOut;                                // Output buffer poin ter (PCM, 32bits, 2 channels time domain interleaved data)
            unsigned int* unsafe                        puiNOutSamples;                        // Pointer to number of output samples produced during last call to the processing function
            SSRCFs_t                                eOutFs;                                // Output sampling rate code

            int* unsafe * unsafe                            ppiOut;                                // Pointer to (PP)FIR output data pointer for last filter in the chain

            FIRCtrl_t                                sFIRF1Ctrl;                            // F1 FIR controller
            FIRCtrl_t                                sFIRF2Ctrl;                            // F2 FIR controller
            PPFIRCtrl_t                                sPPFIRF3Ctrl;                        // F3 PPFIR controller

            unsigned int                            uiDitherOnOff;                        // Dither on/off flag
            unsigned int                            uiRndSeedInit;                        // Dither random seed initial value

            ssrc_state_t* unsafe                        psState;                            // Pointer to state structure
            int* unsafe                                piStack;                            // Pointer to stack buffer
#else
            long long                               pad_to_64b_alignment;               //Force compiler to 64b align
            unsigned int                            uiNchannels;                        // Number of channels in this instance

            int*                                      piIn;                                // Input buffer pointer (PCM, 32bits, 2 channels time domain interleaved data)
            unsigned int                            uiNInSamples;                        // Number of input samples to process in one call to the processing function
            SSRCFs_t                                eInFs;                                // Input sampling rate code
            int*                                      piOut;                                // Output buffer poin ter (PCM, 32bits, 2 channels time domain interleaved data)
            unsigned int*                             puiNOutSamples;                        // Pointer to number of output samples produced during last call to the processing function
            SSRCFs_t                                eOutFs;                                // Output sampling rate code

            int* *                                    ppiOut;                                // Pointer to (PP)FIR output data pointer for last filter in the chain

            FIRCtrl_t                                sFIRF1Ctrl;                            // F1 FIR controller
            FIRCtrl_t                                sFIRF2Ctrl;                            // F2 FIR controller
            PPFIRCtrl_t                                sPPFIRF3Ctrl;                        // F3 PPFIR controller

            unsigned int                            uiDitherOnOff;                        // Dither on/off flag
            unsigned int                            uiRndSeedInit;                        // Dither random seed initial value

            ssrc_state_t*                              psState;                            // Pointer to state structure
            int*                                    piStack;                            // Pointer to stack buffer
#endif
        } ssrc_ctrl_t;


        // ===========================================================================
        //
        // Function prototypes
        //
        // ===========================================================================

        // ==================================================================== //
        // Function:        SSRC_init                                            //
        // Arguments:        ssrc_ctrl_t     *pssrc_ctrl: Ctrl strct.                //
        // Return values:    SSRC_NO_ERROR on success                            //
        //                    SSRC_ERROR on failure                                //
        // Description:        Inits the SSRC passed as argument                    //
        // ==================================================================== //
        SSRCReturnCodes_t                SSRC_init(ssrc_ctrl_t* pssrc_ctrl);

        // ==================================================================== //
        // Function:        SSRC_sync                                            //
        // Arguments:        ssrc_ctrl_t     *pssrc_ctrl: Ctrl strct.                //
        // Return values:    SSRC_NO_ERROR on success                            //
        //                    SSRC_ERROR on failure                                //
        // Description:        Syncs the SSRC passed as argument                    //
        // ==================================================================== //
        SSRCReturnCodes_t                SSRC_sync(ssrc_ctrl_t* pssrc_ctrl);

        // ==================================================================== //
        // Function:        SSRC_proc                                            //
        // Arguments:        ssrc_ctrl_t     *pssrc_ctrl: Ctrl strct.                //
        // Return values:    SSRC_NO_ERROR on success                            //
        //                    SSRC_ERROR on failure                                //
        // Description:        Processes the SSRC passed as argument                //
        // ==================================================================== //
        SSRCReturnCodes_t                SSRC_proc(ssrc_ctrl_t* pssrc_ctrl);

    #endif // nINCLUDE_FROM_ASM

#endif // _SRC_MRHF_SSRC_H_
