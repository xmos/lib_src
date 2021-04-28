// Copyright 2016-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// ===========================================================================
// ===========================================================================
//
// Top level implementation file for the SSRC
//
// Target:    MS Windows
// Version: 1.0
//
// ===========================================================================
// ===========================================================================


// ===========================================================================
//
// Includes
//
// ===========================================================================
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// Integer arithmetic include
#include "src_mrhf_int_arithmetic.h"
// XMOS built in functions
#include <xs1.h>
// SSRC include
#include "src.h"

// ===========================================================================
//
// Defines
//
// ===========================================================================


// State init value
#define        SSRC_STATE_INIT                        0

// Random number generator / dithering
#define        SSRC_R_CONS                            32767
#define        SSRC_R_BASE                            1664525
#define        SSRC_RPDF_BITS_SHIFT                16                        // Shift to select bits in pseudo-random number
#define        SSRC_RPDF_MASK                        0x0000007F                // For dithering at 24bits (in 2.30)
#define        SSRC_DATA24_MASK                    0xFFFFFF00                // Mask for 24bits data (once rescaled to 1.31)
#define        SSRC_DITHER_BIAS                    0xFFFFFFC0                // TPDF dither bias for compensating masking at 24bits but expressed in 2.30

// Cycle counter
#define        SSRC_FIR_OS2_OVERHEAD_CYCLE_COUNT    (15.0)
#define        SSRC_FIR_OS2_TAP_CYCLE_COUNT        (1.875)
#define        SSRC_FIR_DS2_OVERHEAD_CYCLE_COUNT    (15.0)
#define        SSRC_FIR_DS2_TAP_CYCLE_COUNT        (2.125)
#define        SSRC_FIR_SYNC_OVERHEAD_CYCLE_COUNT    (15.0)
#define        SSRC_FIR_SYNC_TAP_CYCLE_COUNT        (2.125)
#define        SSRC_FIR_PP_OVERHEAD_CYCLE_COUNT    (15.0 + SSRC_FIR_SYNC_OVERHEAD_CYCLE_COUNT)
#define        SSRC_FIR_PP_TAP_CYCLE_COUNT            (2.125)
#define        SSRC_DITHER_SAMPLE_COUNT            (20.0)



// ===========================================================================
//
// Variables
//
// ===========================================================================

SSRCFiltersIDs_t        sFiltersIDs[SSRC_N_FS][SSRC_N_FS] =                // Filter configuration table [Fsin][Fsout]
{
    {    // Fsin = 44.1kHz
        // F1                                  F2                                   F3                                Phase step
        {{FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_NONE_ID},        FILTER_DEFS_PPFIR_PHASE_STEP_0},        // Fsout = 44.1kHz
        {{FILTER_DEFS_SSRC_FIR_UP_ID,          FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_HS320_ID},       FILTER_DEFS_PPFIR_PHASE_STEP_294},      // Fsout = 48kHz
        {{FILTER_DEFS_SSRC_FIR_UP_ID,          FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_NONE_ID},        FILTER_DEFS_PPFIR_PHASE_STEP_0},        // Fsout = 88.2kHz
        {{FILTER_DEFS_SSRC_FIR_UP_ID,          FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_HS320_ID},       FILTER_DEFS_PPFIR_PHASE_STEP_147},      // Fsout = 96kHz
        {{FILTER_DEFS_SSRC_FIR_UP_ID,          FILTER_DEFS_SSRC_FIR_OS_ID,          FILTER_DEFS_PPFIR_NONE_ID},        FILTER_DEFS_PPFIR_PHASE_STEP_0},        // Fsout = 176.4kHz
        {{FILTER_DEFS_SSRC_FIR_UP_ID,          FILTER_DEFS_SSRC_FIR_OS_ID,          FILTER_DEFS_PPFIR_HS320_ID},       FILTER_DEFS_PPFIR_PHASE_STEP_147}       // Fsout = 192kHz
    },
    {    // Fsin = 48kHz
        // F1                                  F2                                   F3                                Phase step
        {{FILTER_DEFS_SSRC_FIR_UP4844_ID,      FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_HS294_ID},       FILTER_DEFS_PPFIR_PHASE_STEP_320},      // Fsout = 44.1kHz
        {{FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_NONE_ID},        FILTER_DEFS_PPFIR_PHASE_STEP_0},        // Fsout = 48kHz
        {{FILTER_DEFS_SSRC_FIR_UP_ID,          FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_HS294_ID},       FILTER_DEFS_PPFIR_PHASE_STEP_160},      // Fsout = 88.2kHz
        {{FILTER_DEFS_SSRC_FIR_UP_ID,          FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_NONE_ID},        FILTER_DEFS_PPFIR_PHASE_STEP_0},        // Fsout = 96kHz
        {{FILTER_DEFS_SSRC_FIR_UP_ID,          FILTER_DEFS_SSRC_FIR_OS_ID,          FILTER_DEFS_PPFIR_HS294_ID},       FILTER_DEFS_PPFIR_PHASE_STEP_160},      // Fsout = 176.4kHz
        {{FILTER_DEFS_SSRC_FIR_UP_ID,          FILTER_DEFS_SSRC_FIR_OS_ID,          FILTER_DEFS_PPFIR_NONE_ID},        FILTER_DEFS_PPFIR_PHASE_STEP_0}         // Fsout = 192kHz
    },
    {    // Fsin = 88.2kHz
        // F1                                  F2                                   F3                                Phase step
        {{FILTER_DEFS_SSRC_FIR_BL_ID,          FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_NONE_ID},        FILTER_DEFS_PPFIR_PHASE_STEP_0},        // Fsout = 44.1kHz
        {{FILTER_DEFS_SSRC_FIR_BL8848_ID,      FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_HS320_ID},       FILTER_DEFS_PPFIR_PHASE_STEP_294},      // Fsout = 48kHz
        {{FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_NONE_ID},        FILTER_DEFS_PPFIR_PHASE_STEP_0},        // Fsout = 88.2kHz
        {{FILTER_DEFS_SSRC_FIR_UP_ID,          FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_HS320_ID},       FILTER_DEFS_PPFIR_PHASE_STEP_294},      // Fsout = 96kHz
        {{FILTER_DEFS_SSRC_FIR_UP_ID,          FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_NONE_ID},        FILTER_DEFS_PPFIR_PHASE_STEP_0},        // Fsout = 176.4kHz
        {{FILTER_DEFS_SSRC_FIR_UP_ID,          FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_HS320_ID},       FILTER_DEFS_PPFIR_PHASE_STEP_147}       // Fsout = 192kHz
    },
    {    // Fsin = 96kHz
        // F1                                  F2                                   F3                                Phase step
        {{FILTER_DEFS_SSRC_FIR_BL9644_ID,      FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_HS294_ID},       FILTER_DEFS_PPFIR_PHASE_STEP_320},      // Fsout = 44.1kHz
        {{FILTER_DEFS_SSRC_FIR_BL_ID,          FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_NONE_ID},        FILTER_DEFS_PPFIR_PHASE_STEP_0},        // Fsout = 48kHz
        {{FILTER_DEFS_SSRC_FIR_UP4844_ID,      FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_HS294_ID},       FILTER_DEFS_PPFIR_PHASE_STEP_320},      // Fsout = 88.2kHz
        {{FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_NONE_ID},        FILTER_DEFS_PPFIR_PHASE_STEP_0},        // Fsout = 96kHz
        {{FILTER_DEFS_SSRC_FIR_UP_ID,          FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_HS294_ID},       FILTER_DEFS_PPFIR_PHASE_STEP_160},      // Fsout = 176.4kHz
        {{FILTER_DEFS_SSRC_FIR_UP_ID,          FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_NONE_ID},        FILTER_DEFS_PPFIR_PHASE_STEP_0}         // Fsout = 192kHz
    },
    {    // Fsin = 176.4kHz
        // F1                                 F2                                   F3                                Phase step
        {{FILTER_DEFS_SSRC_FIR_DS_ID,         FILTER_DEFS_SSRC_FIR_BL_ID,          FILTER_DEFS_PPFIR_NONE_ID},        FILTER_DEFS_PPFIR_PHASE_STEP_0},         // Fsout = 44.1kHz
        {{FILTER_DEFS_SSRC_FIR_DS_ID,         FILTER_DEFS_SSRC_FIR_BL8848_ID,      FILTER_DEFS_PPFIR_HS320_ID},       FILTER_DEFS_PPFIR_PHASE_STEP_294},       // Fsout = 48kHz
        {{FILTER_DEFS_SSRC_FIR_BL_ID,         FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_NONE_ID},        FILTER_DEFS_PPFIR_PHASE_STEP_0},         // Fsout = 88.2kHz
        {{FILTER_DEFS_SSRC_FIR_BL17696_ID,    FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_HS320_ID},       FILTER_DEFS_PPFIR_PHASE_STEP_294},       // Fsout = 96kHz
        {{FILTER_DEFS_SSRC_FIR_NONE_ID,       FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_NONE_ID},        FILTER_DEFS_PPFIR_PHASE_STEP_0},         // Fsout = 176.4kHz
        {{FILTER_DEFS_SSRC_FIR_UPF_ID,        FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_HS320_ID},       FILTER_DEFS_PPFIR_PHASE_STEP_294}        // Fsout = 192kHz
    },
    {    // Fsin = 192kHz
        // F1                                 F2                                   F3                                Phase step
        {{FILTER_DEFS_SSRC_FIR_DS_ID,         FILTER_DEFS_SSRC_FIR_BL9644_ID,      FILTER_DEFS_PPFIR_HS294_ID},       FILTER_DEFS_PPFIR_PHASE_STEP_320},       // Fsout = 44.1kHz
        {{FILTER_DEFS_SSRC_FIR_DS_ID,         FILTER_DEFS_SSRC_FIR_BL_ID,          FILTER_DEFS_PPFIR_NONE_ID},        FILTER_DEFS_PPFIR_PHASE_STEP_0},         // Fsout = 48kHz
        {{FITLER_DEFS_SSRC_FIR_BL19288_ID,    FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_HS294_ID},       FILTER_DEFS_PPFIR_PHASE_STEP_320},       // Fsout = 88.2kHz
        {{FILTER_DEFS_SSRC_FIR_BL_ID,         FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_NONE_ID},        FILTER_DEFS_PPFIR_PHASE_STEP_0},         // Fsout = 96kHz
        {{FILTER_DEFS_SSRC_FIR_UP192176_ID,   FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_HS294_ID},       FILTER_DEFS_PPFIR_PHASE_STEP_320},       // Fsout = 176.4kHz
        {{FILTER_DEFS_SSRC_FIR_NONE_ID,       FILTER_DEFS_SSRC_FIR_NONE_ID,        FILTER_DEFS_PPFIR_NONE_ID},        FILTER_DEFS_PPFIR_PHASE_STEP_0}          // Fsout = 192kHz
    }
};



// ===========================================================================
//
// Local Functions prototypes
//
// ===========================================================================


SSRCReturnCodes_t                SSRC_proc_F1_F2(ssrc_ctrl_t* pssrc_ctrl);
SSRCReturnCodes_t                SSRC_proc_F3(ssrc_ctrl_t* pssrc_ctrl);
SSRCReturnCodes_t                SSRC_proc_dither(ssrc_ctrl_t* pssrc_ctrl);


// ===========================================================================
//
// Functions implementations
//
// ===========================================================================



// ==================================================================== //
// Function:        SSRC_init                                            //
// Arguments:        ssrc_ctrl_t     *pssrc_ctrl: Ctrl strct.                //
// Return values:    SSRC_NO_ERROR on success                            //
//                    SSRC_ERROR on failure                                //
// Description:        Inits the SSRC passed as argument                    //
// ==================================================================== //
SSRCReturnCodes_t                SSRC_init(ssrc_ctrl_t* pssrc_ctrl)
{
    SSRCFiltersIDs_t*            psFiltersID;
    FIRDescriptor_t*            psFIRDescriptor;
    PPFIRDescriptor_t*            psPPFIRDescriptor;


    // Check if state is allocated
    if(pssrc_ctrl->psState == 0)
        return SSRC_ERROR;

    // Check if stack is allocated
    if(pssrc_ctrl->piStack == 0)
        return SSRC_ERROR;

    // Check if valid Fsin and Fsout have been provided
    if( (pssrc_ctrl->eInFs < SSRC_FS_MIN) || (pssrc_ctrl->eInFs > SSRC_FS_MAX))
        return SSRC_ERROR;
    if( (pssrc_ctrl->eOutFs < SSRC_FS_MIN) || (pssrc_ctrl->eOutFs > SSRC_FS_MAX))
        return SSRC_ERROR;

    // Check that number of input samples is allocated and is a multiple of 4
    if(pssrc_ctrl->uiNInSamples == 0)
        return SSRC_ERROR;
    if((pssrc_ctrl->uiNInSamples & 0x3) != 0x0)
        return SSRC_ERROR;

    // Load filters ID and number of samples
    psFiltersID        = &sFiltersIDs[pssrc_ctrl->eInFs][pssrc_ctrl->eOutFs];

    // Configure filters from filters ID and number of samples

    // Filter F1
    // ---------
    psFIRDescriptor                                = &sSSRCFirDescriptor[psFiltersID->uiFID[SSRC_F1_INDEX]];
    // Set number of input samples and input samples step
    pssrc_ctrl->sFIRF1Ctrl.uiNInSamples        = pssrc_ctrl->uiNInSamples;
    pssrc_ctrl->sFIRF1Ctrl.uiInStep            = pssrc_ctrl->uiNchannels;

    // Set delay line base pointer
    if( (psFiltersID->uiFID[SSRC_F1_INDEX] == FILTER_DEFS_SSRC_FIR_DS_ID) || (psFiltersID->uiFID[SSRC_F1_INDEX] == FILTER_DEFS_SSRC_FIR_OS_ID) )
        pssrc_ctrl->sFIRF1Ctrl.piDelayB        = pssrc_ctrl->psState->iDelayFIRShort;
    else
        pssrc_ctrl->sFIRF1Ctrl.piDelayB        = pssrc_ctrl->psState->iDelayFIRLong;

    // Set output buffer step
    if(psFiltersID->uiFID[SSRC_F2_INDEX] == FILTER_DEFS_SSRC_FIR_OS_ID)
        // F2 in use in over-sampling by 2 mode
        pssrc_ctrl->sFIRF1Ctrl.uiOutStep    = 2 * pssrc_ctrl->uiNchannels;
    else
        pssrc_ctrl->sFIRF1Ctrl.uiOutStep    = pssrc_ctrl->uiNchannels;

    // Call init for FIR F1
    if(FIR_init_from_desc(&pssrc_ctrl->sFIRF1Ctrl, psFIRDescriptor) != FIR_NO_ERROR)
        return SSRC_ERROR;


    // Filter F2
    // ---------
    psFIRDescriptor                            = &sSSRCFirDescriptor[psFiltersID->uiFID[SSRC_F2_INDEX]];

    // Set number of input samples and input samples step
    pssrc_ctrl->sFIRF2Ctrl.uiNInSamples        = pssrc_ctrl->sFIRF1Ctrl.uiNOutSamples;
    pssrc_ctrl->sFIRF2Ctrl.uiInStep            = pssrc_ctrl->sFIRF1Ctrl.uiOutStep;

    // Set delay line base pointer
    if( (psFiltersID->uiFID[SSRC_F2_INDEX] == FILTER_DEFS_SSRC_FIR_DS_ID) || (psFiltersID->uiFID[SSRC_F2_INDEX] == FILTER_DEFS_SSRC_FIR_OS_ID) )
        pssrc_ctrl->sFIRF2Ctrl.piDelayB        = pssrc_ctrl->psState->iDelayFIRShort;
    else
        pssrc_ctrl->sFIRF2Ctrl.piDelayB        = pssrc_ctrl->psState->iDelayFIRLong;

    // Set output buffer step
    pssrc_ctrl->sFIRF2Ctrl.uiOutStep    = pssrc_ctrl->uiNchannels;

    // Call init for FIR F1
    if(FIR_init_from_desc(&pssrc_ctrl->sFIRF2Ctrl, psFIRDescriptor) != FIR_NO_ERROR)
        return SSRC_ERROR;


    // Filter F3
    // ---------
    psPPFIRDescriptor                                = &sPPFirDescriptor[psFiltersID->uiFID[SSRC_F3_INDEX]];

    // Set number of input samples and input samples step
    if(psFiltersID->uiFID[SSRC_F2_INDEX] == FILTER_DEFS_SSRC_FIR_NONE_ID)
        pssrc_ctrl->sPPFIRF3Ctrl.uiNInSamples    = pssrc_ctrl->sFIRF1Ctrl.uiNOutSamples;
    else
        pssrc_ctrl->sPPFIRF3Ctrl.uiNInSamples    = pssrc_ctrl->sFIRF2Ctrl.uiNOutSamples;
    pssrc_ctrl->sPPFIRF3Ctrl.uiInStep        = pssrc_ctrl->sFIRF2Ctrl.uiOutStep;

    // Set delay line base pointer
    pssrc_ctrl->sPPFIRF3Ctrl.piDelayB        = pssrc_ctrl->psState->iDelayPPFIR;

    // Set output buffer step
    pssrc_ctrl->sPPFIRF3Ctrl.uiOutStep        = pssrc_ctrl->uiNchannels;

    // Set phase step
    pssrc_ctrl->sPPFIRF3Ctrl.uiPhaseStep    = psFiltersID->uiPPFIRPhaseStep;

    // Call init for PPFIR F3
    if(PPFIR_init_from_desc(&pssrc_ctrl->sPPFIRF3Ctrl, psPPFIRDescriptor) != FIR_NO_ERROR)
        return SSRC_ERROR;


    // Setup input/output buffers
    // --------------------------
    // We first set them all to stack base (some will be overwritten depending on configuration)

    // F1 input is never from stack, so don't set it
    pssrc_ctrl->sFIRF2Ctrl.piIn                = pssrc_ctrl->piStack;
    pssrc_ctrl->sPPFIRF3Ctrl.piIn            = pssrc_ctrl->piStack;

    pssrc_ctrl->sFIRF1Ctrl.piOut            = pssrc_ctrl->piStack;
    pssrc_ctrl->sFIRF2Ctrl.piOut            = pssrc_ctrl->piStack;
    // F3 output is never to stack, so don't set it


    // Finally setup pointer to output buffer that needs to be modified for data output depending on filter configuration
    // Also set pointer to number of output samples
    if(psFiltersID->uiFID[SSRC_F3_INDEX] != FILTER_DEFS_PPFIR_NONE_ID)
    {
        // F3 is in use so take output from F3 output
        pssrc_ctrl->ppiOut            = &pssrc_ctrl->sPPFIRF3Ctrl.piOut;
        pssrc_ctrl->puiNOutSamples    = &pssrc_ctrl->sPPFIRF3Ctrl.uiNOutSamples;
    }
    else
    {
        if(psFiltersID->uiFID[SSRC_F2_INDEX] != FILTER_DEFS_SSRC_FIR_NONE_ID)
        {
            // F3 not in use but F2 in use, take output from F2 output
            pssrc_ctrl->ppiOut            = &pssrc_ctrl->sFIRF2Ctrl.piOut;
            pssrc_ctrl->puiNOutSamples    = &pssrc_ctrl->sFIRF2Ctrl.uiNOutSamples;
        }
        else
        {
            // F3 and F2 not in use but F1 in use or not. Set output from F1 output
            // Note that we also set it to F1 output, even if F1 is not in use (Fsin = Fsout case, this won't cause any problem)
            pssrc_ctrl->ppiOut        = &pssrc_ctrl->sFIRF1Ctrl.piOut;

            if(psFiltersID->uiFID[SSRC_F1_INDEX] != FILTER_DEFS_SSRC_FIR_NONE_ID)
                // F1 in use so set number of output sample pointer to number of output sample field of F1
                pssrc_ctrl->puiNOutSamples    = &pssrc_ctrl->sFIRF1Ctrl.uiNOutSamples;
            else
                // F1 not in use so set number of output sample pointer to number of input sample field
                pssrc_ctrl->puiNOutSamples    = &pssrc_ctrl->uiNInSamples;
        }
    }

    // Call sync function
    if(SSRC_sync(pssrc_ctrl) != SSRC_NO_ERROR)
        return SSRC_ERROR;

    return SSRC_NO_ERROR;
}


// ==================================================================== //
// Function:        SSRC_sync                                            //
// Arguments:        ssrc_ctrl_t     *pssrc_ctrl: Ctrl strct.                //
// Return values:    SSRC_NO_ERROR on success                            //
//                    SSRC_ERROR on failure                                //
// Description:        Syncs the SSRC passed as argument                    //
// ==================================================================== //
SSRCReturnCodes_t                SSRC_sync(ssrc_ctrl_t* pssrc_ctrl)
{
    // Sync the FIR and PPFIR
    if(FIR_sync(&pssrc_ctrl->sFIRF1Ctrl) != FIR_NO_ERROR)
        return SSRC_ERROR;
    if(FIR_sync(&pssrc_ctrl->sFIRF2Ctrl) != FIR_NO_ERROR)
        return SSRC_ERROR;
    if(PPFIR_sync(&pssrc_ctrl->sPPFIRF3Ctrl) != FIR_NO_ERROR)
        return SSRC_ERROR;

    // Reset random seeds to initial values
    pssrc_ctrl->psState->uiRndSeed    = pssrc_ctrl->uiRndSeedInit;

    return SSRC_NO_ERROR;
}


// ==================================================================== //
// Function:        SSRC_proc                                            //
// Arguments:        ssrc_ctrl_t     *pssrc_ctrl: Ctrl strct.                //
// Return values:    SSRC_NO_ERROR on success                            //
//                    SSRC_ERROR on failure                                //
// Description:        Processes the SSRC passed as argument                //
// ==================================================================== //
#if (XCC_VERSION_MAJOR < 1402) //Beyond 14.2.0 we have proper function pointer support for C
#pragma stackfunction 64  //Generous stack allocation (probably needs just a handful through F1_F2, ASM etc).
#endif
SSRCReturnCodes_t                SSRC_proc(ssrc_ctrl_t* pssrc_ctrl)
{
    // Setup input / output buffers
    // ----------------------------
    pssrc_ctrl->sFIRF1Ctrl.piIn            = pssrc_ctrl->piIn;
    *(pssrc_ctrl->ppiOut)                = pssrc_ctrl->piOut;

    // F1 and F2 process
    // -----------------
    if( SSRC_proc_F1_F2(pssrc_ctrl) != SSRC_NO_ERROR)
        return SSRC_ERROR;

    // F3 process
    // ----------
    if( SSRC_proc_F3(pssrc_ctrl)    != SSRC_NO_ERROR)
        return SSRC_ERROR;

    // Dither process
    // --------------
    if( SSRC_proc_dither(pssrc_ctrl) != SSRC_NO_ERROR)
        return SSRC_ERROR;

    return SSRC_NO_ERROR;
}


// ==================================================================== //
// Function:        SSRC_proc_F1_F2                                        //
// Arguments:        ssrc_ctrl_t     *pssrc_ctrl: Ctrl strct.                //
// Return values:    SSRC_NO_ERROR on success                            //
//                    SSRC_ERROR on failure                                //
// Description:        Processes F1 and F2 for a channel                    //
// ==================================================================== //
SSRCReturnCodes_t                SSRC_proc_F1_F2(ssrc_ctrl_t* pssrc_ctrl)
{
    int*            piIn        = pssrc_ctrl->piIn;
    int*            piOut        = pssrc_ctrl->piOut;
    unsigned int    ui;


    // Check if F1 is disabled, in which case we just copy input to output as all filters are disabled
    if(pssrc_ctrl->sFIRF1Ctrl.eEnable == FIR_OFF)
    {
        // F1 is not enabled, which means that we are in 1:1 rate, so just copy input to output
        for(ui = 0; ui < pssrc_ctrl->uiNInSamples * pssrc_ctrl->uiNchannels; ui+= pssrc_ctrl->uiNchannels)
            piOut[ui]        = piIn[ui];

        return SSRC_NO_ERROR;
    }

    // F1 is enabled, so call F1
    if(pssrc_ctrl->sFIRF1Ctrl.pvProc((int *)&pssrc_ctrl->sFIRF1Ctrl) != FIR_NO_ERROR)
        return SSRC_ERROR; //Note blatant cast to int * to work around no FP support in XC


    // Check if F2 is enabled
    if(pssrc_ctrl->sFIRF2Ctrl.eEnable == FIR_ON)
    {
        // F2 is enabled, so call F2
        if(pssrc_ctrl->sFIRF2Ctrl.pvProc((int *)&pssrc_ctrl->sFIRF2Ctrl) != FIR_NO_ERROR)
            return SSRC_ERROR; //Note blatant cast to int * to work around no FP support in XC
    }

    return SSRC_NO_ERROR;
}


// ==================================================================== //
// Function:        SSRC_proc_F3                                        //
// Arguments:        ssrc_ctrl_t     *pssrc_ctrl: Ctrl strct.                //
// Return values:    SSRC_NO_ERROR on success                            //
//                    SSRC_ERROR on failure                                //
// Description:        Processes F3 for a channel                            //
// ==================================================================== //
SSRCReturnCodes_t                SSRC_proc_F3(ssrc_ctrl_t* pssrc_ctrl)
{

    // Check if F3 is enabled
    if(pssrc_ctrl->sPPFIRF3Ctrl.eEnable == FIR_ON)
    {
        // F3 is enabled, so call F3
        if(PPFIR_proc(&pssrc_ctrl->sPPFIRF3Ctrl) != FIR_NO_ERROR)
            return SSRC_ERROR;
    }

    return SSRC_NO_ERROR;
}


// ==================================================================== //
// Function:        SSRC_proc_dither                                    //
// Arguments:        ssrc_ctrl_t     *pssrc_ctrl: Ctrl strct.                //
// Return values:    SSRC_NO_ERROR on success                            //
//                    SSRC_ERROR on failure                                //
// Description:        Processes dither for a channel                        //
// ==================================================================== //
SSRCReturnCodes_t                SSRC_proc_dither(ssrc_ctrl_t* pssrc_ctrl)
{
    int*            piData;
    unsigned int    uiR;
    int                iDither;
    __int64_t            i64Acc;
    unsigned int    ui;


    // Apply dither if required
    if(pssrc_ctrl->uiDitherOnOff == SSRC_DITHER_ON)
    {
        // Get data buffer
        piData    = pssrc_ctrl->piOut;
        // Get random seed
        uiR        = pssrc_ctrl->psState->uiRndSeed;

        // Loop through samples
        for(ui = 0; ui < *(pssrc_ctrl->puiNOutSamples) * pssrc_ctrl->uiNchannels; ui += pssrc_ctrl->uiNchannels)
        {
            // Compute dither sample (TPDF)
            iDither        = SSRC_DITHER_BIAS;

            uiR            = (unsigned int)(SSRC_R_BASE * uiR);
            uiR            = (unsigned int)(SSRC_R_CONS + uiR);
            iDither        += ((uiR>>SSRC_RPDF_BITS_SHIFT) & SSRC_RPDF_MASK);

            uiR            = (unsigned int)(SSRC_R_BASE * uiR);
            uiR            = (unsigned int)(SSRC_R_CONS + uiR);
            iDither        += ((uiR>>SSRC_RPDF_BITS_SHIFT) & SSRC_RPDF_MASK);

                        //TODO - optimise this and remove IntArithmetic
            // Use MACC instruction to saturate and dither + signal
            i64Acc        = ((__int64_t)iDither <<32);    // On XMOS this is not necessary, just load dither in the top word of the ACC register
            MACC(&i64Acc, piData[ui], 0x7FFFFFFF);

                        // Saturate to 31 bits
                        LSAT30(&i64Acc);

                        // Extract 32bits result
            EXT30(&piData[ui], i64Acc);

            // Mask to 24bits
            piData[ui]    &= SSRC_DATA24_MASK;

        }

        // Write random seed back
        pssrc_ctrl->psState->uiRndSeed    = uiR;
    }

    return SSRC_NO_ERROR;
}
