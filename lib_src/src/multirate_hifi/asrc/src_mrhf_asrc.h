// Copyright 2016-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// ===========================================================================
// ===========================================================================
//
// Top level definition file for the ASRC
//
// Target:    MS Windows
// Version: 1.0
//
// ===========================================================================
// ===========================================================================

#ifndef _SRC_MRHF_ASRC_H_
#define _SRC_MRHF_ASRC_H_

    // ===========================================================================
    //
    // Includes
    //
    // ===========================================================================
    #include "src_mrhf_fir.h"
    #include "src_mrhf_filter_defs.h"
    #include "src.h"
    #include "src_mrhf_spline_coeff_gen_inner_loop_asm.h"
    #include "src_mrhf_adfir_inner_loop_asm.h"

    // ===========================================================================
    //
    // Defines
    //
    // ===========================================================================

    // General defines
    // ---------------
    #define		   ASRC_FS_RATIO_UNIT_BIT					28
    #define        ASRC_STACK_LENGTH_MULT                (ASRC_N_CHANNELS * 4)                // Multiplier for stack length (stack length = this value x the number of input samples to process)
    #define        ASRC_ADFIR_COEFS_LENGTH               FILTER_DEFS_ADFIR_PHASE_N_TAPS        // Length of AD FIR coefficients buffer
    #define		   ASRC_NOMINAL_FS_SCALE				     (1 << ASRC_FS_RATIO_UNIT_BIT)


    // Parameter values
    // ----------------
    #define        ASRC_DITHER_OFF                    0
    #define        ASRC_DITHER_ON                    1
    #define        ASRC_DITHER_ON_OFF_MIN                ASRC_DITHER_OFF
    #define        ASRC_DITHER_ON_OFF_MAX                ASRC_DITHER_ON




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

        // ASRC Return Codes
        // -----------------
        typedef enum _ASRCReturnCodes
        {
            ASRC_NO_ERROR                            = 0,
            ASRC_ERROR                                = 1
        } ASRCReturnCodes_t;


        // Sampling rate codes
        // -------------------
        typedef enum _ASRCFs
        {
            ASRC_FS_44                                = 0,                                // Fs = 44.1kHz code
            ASRC_FS_48                                = 1,                                // Fs = 48kHz code
            ASRC_FS_88                                = 2,                                // Fs = 88.2kHz code
            ASRC_FS_96                                = 3,                                // Fs = 96kHz code
            ASRC_FS_176                                = 4,                                // Fs = 176.4kHz code
            ASRC_FS_192                                = 5,                                // Fs = 192kHz code
        } ASRCFs_t;
        #define        ASRC_N_FS                        (ASRC_FS_192 + 1)
        #define        ASRC_FS_MIN                        ASRC_FS_44
        #define        ASRC_FS_MAX                        ASRC_FS_192


        // ASRC Filters IDs structure
        // --------------------------
        #define        ASRC_F1_INDEX                    0
        #define        ASRC_F2_INDEX                    1
        #define        ASRC_N_F                        (ASRC_F2_INDEX + 1)

        typedef struct _ASRCFiltersIDs
        {
            unsigned int        uiFID[ASRC_N_F];
        } ASRCFiltersIDs_t;

        // ASRC time ratio configurations
        // ------------------------------
        typedef struct _ASRCFsRatioConfigs
        {
            unsigned int        uiNominalFsRatio;
            unsigned int		uiNominalFsRatio_lo;
            unsigned int        uiMinFsRatio;
            unsigned int        uiMaxFsRatio;
            int                    iFsRatioShift;
        } ASRCFsRatioConfigs_t;

        // ASRC State structure
        // --------------------
        typedef struct _ASRCState
        {
            long long                               pad_to_64b_alignment;               //Force compiler to 64b align
            unsigned int                            uiRndSeed;                                                // Dither random seeds current values
            int                                        iDelayFIRLong[2 * FILTER_DEFS_FIR_MAX_TAPS_LONG];        // Doubled length for circular buffer simulation
            int                                        iDelayFIRShort[2 * FILTER_DEFS_FIR_MAX_TAPS_SHORT];        // Doubled length for circular buffer simulation
            int                                        iDelayADFIR[2 * FILTER_DEFS_ADFIR_PHASE_N_TAPS];        // Doubled length for circular buffer simulation
        } asrc_state_t;


        // ASRC Control structure
        // ----------------------
        typedef struct _ASRCCtrl
        {
#ifdef __XC__
            long long                               pad_to_64b_alignment;               //Force compiler to 64b align
            unsigned int                            uiNchannels;                        // Number of channels in this instance
            int* unsafe                                piIn;                                // Input buffer pointer (PCM, 32bits, 2 channels time domain interleaved data)
            unsigned int                            uiNInSamples;                        // Number of input samples to process in one call to the processing function
            unsigned int                            uiNSyncSamples;                        // Number of synchronous samples produced in one call to the processing function
            ASRCFs_t                                eInFs;                                // Input sampling rate code
            int* unsafe                                piOut;                                // Output buffer poin ter (PCM, 32bits, 2 channels time domain interleaved data)
            unsigned int                            uiNASRCOutSamples;                    // Number of output samples produced during last call to the asynchronous processing function
            ASRCFs_t                                eOutFs;                                // Output sampling rate code

            FIRCtrl_t                                sFIRF1Ctrl;                            // F1 FIR controller
            FIRCtrl_t                                sFIRF2Ctrl;                            // F2 FIR controller
            ADFIRCtrl_t                                sADFIRF3Ctrl;                        // F3 ADFIR controller

            unsigned int                            uiFsRatio;                            // Fs ratio: Fsin / Fsout
            unsigned int							uiFsRatio_lo;

            int                                        iTimeInt;                            // Integer part of time
            unsigned int                            uiTimeFract;                        // Fractional part of time
            int                                        iTimeStepInt;                        // Integer part of time step
            unsigned int                            uiTimeStepFract;                    // Fractional part of time step

            unsigned int                            uiDitherOnOff;                        // Dither on/off flag
            unsigned int                            uiRndSeedInit;                        // Dither random seed initial value

            asrc_state_t* unsafe                        psState;                            // Pointer to state structure
            int* unsafe                                piStack;                            // Pointer to stack buffer
            int* unsafe                                piADCoefs;                            // Pointer to AD coefficients
#else
            long long                               pad_to_64b_alignment;               //Force compiler to 64b align
            unsigned int                            uiNchannels;                        // Number of channels in this instance
            int*                                    piIn;                                // Input buffer pointer (PCM, 32bits, 2 channels time domain interleaved data)
            unsigned int                            uiNInSamples;                        // Number of input samples to process in one call to the processing function
            unsigned int                            uiNSyncSamples;                        // Number of synchronous samples produced in one call to the processing function
            ASRCFs_t                                eInFs;                                // Input sampling rate code
            int*                                    piOut;                                // Output buffer poin ter (PCM, 32bits, 2 channels time domain interleaved data)
            unsigned int                            uiNASRCOutSamples;                    // Number of output samples produced during last call to the asynchronous processing function
            ASRCFs_t                                eOutFs;                                // Output sampling rate code

            FIRCtrl_t                                sFIRF1Ctrl;                            // F1 FIR controller
            FIRCtrl_t                                sFIRF2Ctrl;                            // F2 FIR controller
            ADFIRCtrl_t                                sADFIRF3Ctrl;                        // F3 ADFIR controller

            unsigned int                            uiFsRatio;                            // Fs ratio: Fsin / Fsout
            unsigned int							uiFsRatio_lo;

            int                                        iTimeInt;                            // Integer part of time
            unsigned int                            uiTimeFract;                        // Fractional part of time
            int                                        iTimeStepInt;                        // Integer part of time step
            unsigned int                            uiTimeStepFract;                    // Fractional part of time step

            unsigned int                            uiDitherOnOff;                        // Dither on/off flag
            unsigned int                            uiRndSeedInit;                        // Dither random seed initial value

            asrc_state_t*                            psState;                            // Pointer to state structure
            int*                                    piStack;                            // Pointer to stack buffer
            int*                                    piADCoefs;                            // Pointer to AD coefficients
#endif
        } asrc_ctrl_t;


        // Adaptive filter coefficients. Note this is a workaround to force the compiler to align the array to 64b boundary (required by inner loop assembler that uses load/store double)
        // -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

                typedef struct _asrc_adfir_coefs_t
                {
                    long long       padding_to_64b;                           //Force 64b alignment
                    int             iASRCADFIRCoefs[ASRC_ADFIR_COEFS_LENGTH]; //Adaptive FIR coefficients (one per instance)
                } asrc_adfir_coefs_t;





        // ===========================================================================
        //
        // Function prototypes
        //
        // ===========================================================================

        // ==================================================================== //
        // Function:        ASRC_prepare_coefs                                  //
        // Arguments:       asrc_ctrl_t  *pasrc_ctrl: Ctrl strct.                //
        // Return values:   ASRC_NO_ERROR on success                            //
        //                  ASRC_ERROR on failure                               //
        // Description:     Prepares the ASRC coefficients from the prototype   //
        //                  Needs to be called only once                        //
        // ==================================================================== //
        ASRCReturnCodes_t               ASRC_prepare_coefs(void);

        // ==================================================================== //
        // Function:        ASRC_init                                            //
        // Arguments:        asrc_ctrl_t     *pasrc_ctrl: Ctrl strct.                //
        // Return values:    ASRC_NO_ERROR on success                            //
        //                    ASRC_ERROR on failure                                //
        // Description:        Inits the ASRC passed as argument                    //
        // ==================================================================== //
        ASRCReturnCodes_t                ASRC_init(asrc_ctrl_t* pasrc_ctrl);

        // ==================================================================== //
        // Function:        ASRC_sync                                            //
        // Arguments:        asrc_ctrl_t     *pasrc_ctrl: Ctrl strct.                //
        // Return values:    ASRC_NO_ERROR on success                            //
        //                    ASRC_ERROR on failure                                //
        // Description:        Syncs the ASRC passed as argument                    //
        // ==================================================================== //
        ASRCReturnCodes_t                ASRC_sync(asrc_ctrl_t* pasrc_ctrl);

        // ==================================================================== //
        // Function:        ASRC_proc_F1_F2                                        //
        // Arguments:        asrc_ctrl_t     *pasrc_ctrl: Ctrl strct.                //
        // Return values:    ASRC_NO_ERROR on success                            //
        //                    ASRC_ERROR on failure                                //
        // Description:        Processes F1 and F2 for a channel                    //
        // ==================================================================== //
        ASRCReturnCodes_t                ASRC_proc_F1_F2(asrc_ctrl_t* pasrc_ctrl);

        // ==================================================================== //
        // Function:        ASRC_update_fs_ratio                                //
        // Arguments:        asrc_ctrl_t     *pasrc_ctrl: Ctrl strct.                //
        // Return values:    ASRC_NO_ERROR on success                            //
        //                    ASRC_ERROR on failure                                //
        // Description:        Updates the ASRC with the new Fs ratio                //
        // ==================================================================== //
        ASRCReturnCodes_t                ASRC_update_fs_ratio(asrc_ctrl_t* pasrc_ctrl);

        // ==================================================================== //
        // Function:        ASRC_proc_F3_in_spl                                    //
        // Arguments:        asrc_ctrl_t     *pasrc_ctrl: Ctrl strct.                //
        //                    int iInSample: new input sample                        //
        // Return values:    ASRC_NO_ERROR on success                            //
        //                    ASRC_ERROR on failure                                //
        // Description:        Writes new input sample to F3 delay line            //
        // ==================================================================== //
        ASRCReturnCodes_t                ASRC_proc_F3_in_spl(asrc_ctrl_t* pasrc_ctrl, int iInSample);

        // ==================================================================== //
        // Function:        ASRC_proc_F3_time                                    //
        // Arguments:        asrc_ctrl_t     *pasrc_ctrl: Ctrl strct.                //
        // Return values:    ASRC_NO_ERROR if an output sample must be produced    //
        //                    ASRC_ERROR if no output sample needs to be produced    //
        // Description:        Processes F3 time                                    //
        // ==================================================================== //
        ASRCReturnCodes_t                ASRC_proc_F3_time(asrc_ctrl_t* pasrc_ctrl);

        // ==================================================================== //
        // Function:        ASRC_proc_F3_macc                                    //
        // Arguments:        asrc_ctrl_t     *pasrc_ctrl: Ctrl strct.                //
        //                    int* piOutSample: Address of output sample            //
        // Return values:    ASRC_NO_ERROR on success                            //
        //                    ASRC_ERROR on failure                                //
        // Description:        Processes F3 for a channel                            //
        // ==================================================================== //
        ASRCReturnCodes_t                ASRC_proc_F3_macc(asrc_ctrl_t* pasrc_ctrl, int* piOutSample);

        // ==================================================================== //
        // Function:        ASRC_proc_dither                                    //
        // Arguments:        asrc_ctrl_t     *pasrc_ctrl: Ctrl strct.                //
        // Return values:    ASRC_NO_ERROR on success                            //
        //                    ASRC_ERROR on failure                                //
        // Description:        Processes dither for a channel                        //
        // ==================================================================== //
        ASRCReturnCodes_t                ASRC_proc_dither(asrc_ctrl_t* pasrc_ctrl);

    #endif // nINCLUDE_FROM_ASM

#endif // _SRC_MRHF_ASRC_H_
