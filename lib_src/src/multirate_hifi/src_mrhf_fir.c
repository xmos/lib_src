// Copyright 2016-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// ===========================================================================
// ===========================================================================
//
// FIR functions implementation file for the ASRC and SSRC
//
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
#include <time.h>
#include <math.h>

// Optimised assembler inner loop functions
#include "src_mrhf_fir_os_inner_loop_asm.h"
#include "src_mrhf_fir_inner_loop_asm.h"
#include "src_mrhf_adfir_inner_loop_asm.h"
#include "src_mrhf_spline_coeff_gen_inner_loop_asm.h"

// FIR includes
#include "src_mrhf_fir.h"

// ===========================================================================
//
// Defines
//
// ===========================================================================


// State init value
#define        FIR_STATE_INIT                        0





// ===========================================================================
//
// Variables
//
// ===========================================================================



// ===========================================================================
//
// Local Functions prototypes
//
// ===========================================================================



// ===========================================================================
//
// Functions implementations
//
// ===========================================================================

// ==================================================================== //
// Function:        FIR_init_from_desc                                    //
// Arguments:        FIRCtrl_t     *psFIRCtrl: Ctrl strct.                    //
//                    FIRDescriptor_t *psFIRDescriptor: Desc. strct.        //
// Return values:    FIR_NO_ERROR on success                                //
//                    FIR_ERROR on failure                                //
// Description:        Inits the FIR from the Descriptor                    //
// ==================================================================== //
FIRReturnCodes_t                FIR_init_from_desc(FIRCtrl_t* psFIRCtrl, FIRDescriptor_t* psFIRDescriptor)
{

    // Check if FIR is disabled (this is given by the number of coefficients being zero)
    if( psFIRDescriptor->uiNCoefs == 0)
    {
        // Number of coefficients is zero, so disable FIR
        psFIRCtrl->eEnable            = FIR_OFF;
        psFIRCtrl->uiNOutSamples    = 0;
        psFIRCtrl->pvProc            = 0;
        psFIRCtrl->uiDelayL            = 0;
        psFIRCtrl->piDelayW            = 0;
        psFIRCtrl->uiDelayO            = 0;
        psFIRCtrl->uiNLoops            = 0;
        psFIRCtrl->uiNCoefs            = 0;
        psFIRCtrl->piCoefs            = 0;

        return FIR_NO_ERROR;
    }

    // FIR is not disabled
    // Check that delay line base has been set
    if(psFIRCtrl->piDelayB == 0)
        return FIR_ERROR;

    // Check that number of samples has been set and is a multiple of 2
    if(psFIRCtrl->uiNInSamples == 0)
        return FIR_ERROR;
    if((psFIRCtrl->uiNInSamples & 0x1) != 0x0)
        return FIR_ERROR;

    // Check the input and output samples steps have been set
    if(psFIRCtrl->uiInStep == 0)
        return FIR_ERROR;
    if(psFIRCtrl->uiOutStep == 0)
        return FIR_ERROR;

    // Setup depending on FIR descriptor
    switch(psFIRDescriptor->eType)
    {
        // Over-sampler by 2 type
        case FIR_TYPE_OS2:
            if( (psFIRDescriptor->uiNCoefs & 0x3) != 0x0)                                    // Check that number of coefficients is a multiple of 4
                return FIR_ERROR;
            psFIRCtrl->eEnable            = FIR_ON;
            psFIRCtrl->uiNOutSamples    = (psFIRCtrl->uiNInSamples)<<1;                        // Os2 FIR doubles the number of samples
            psFIRCtrl->pvProc            = (FIRReturnCodes_t (*)(int *)) FIR_proc_os2;
            psFIRCtrl->uiDelayL            = psFIRDescriptor->uiNCoefs;                        // Double length for circular buffer simulation, but only half length due to OS2
            psFIRCtrl->piDelayW            = psFIRCtrl->piDelayB + (psFIRDescriptor->uiNCoefs>>1);
            psFIRCtrl->uiDelayO            = psFIRDescriptor->uiNCoefs>>1;
            psFIRCtrl->uiNLoops            = psFIRDescriptor->uiNCoefs>>2;                        // Due to 2 x 32bits read for data and 4 x 32bits for coefs per inner loop
            psFIRCtrl->uiNCoefs            = psFIRDescriptor->uiNCoefs;
            psFIRCtrl->piCoefs            = psFIRDescriptor->piCoefs;
        break;

        // Asynchronous type
        case FIR_TYPE_SYNC:
            if( (psFIRDescriptor->uiNCoefs & 0x1) != 0x0)                                    // Check that number of coefficients is a multiple of 2
                return FIR_ERROR;
            // Non zero coefficients number, so it is a true filter
            psFIRCtrl->eEnable            = FIR_ON;
            psFIRCtrl->uiNOutSamples    = psFIRCtrl->uiNInSamples;                            // Sync FIR does not change number of samples
            psFIRCtrl->pvProc            = (FIRReturnCodes_t (*)(int *)) FIR_proc_sync;
            psFIRCtrl->uiDelayL            = psFIRDescriptor->uiNCoefs<<1;                        // Double length for circular buffer simulation
            psFIRCtrl->piDelayW            = psFIRCtrl->piDelayB + psFIRDescriptor->uiNCoefs;
            psFIRCtrl->uiDelayO            = psFIRDescriptor->uiNCoefs;
            psFIRCtrl->uiNLoops            = psFIRDescriptor->uiNCoefs>>1;                        // Due to 2 x 32bits read for data and coefs per inner loop
            psFIRCtrl->uiNCoefs            = psFIRDescriptor->uiNCoefs;
            psFIRCtrl->piCoefs            = psFIRDescriptor->piCoefs;
        break;

        // Down-sample by 2 type
        case FIR_TYPE_DS2:
            if( (psFIRDescriptor->uiNCoefs & 0x1) != 0x0)                                    // Check that number of coefficients is a multiple of 2
                return FIR_ERROR;
            psFIRCtrl->eEnable            = FIR_ON;
            psFIRCtrl->uiNOutSamples    = psFIRCtrl->uiNInSamples>>1;                        // Ds2 FIR divides the number of samples by two
            psFIRCtrl->pvProc            = (FIRReturnCodes_t (*)(int *)) FIR_proc_ds2;
            psFIRCtrl->uiDelayL            = psFIRDescriptor->uiNCoefs<<1;                        // Double length for circular buffer simulation
            psFIRCtrl->piDelayW            = psFIRCtrl->piDelayB + psFIRDescriptor->uiNCoefs;
            psFIRCtrl->uiDelayO            = psFIRDescriptor->uiNCoefs;
            psFIRCtrl->uiNLoops            = psFIRDescriptor->uiNCoefs>>1;                        // Due to 2 x 32bits read for data and coefs per inner loop
            psFIRCtrl->uiNCoefs            = psFIRDescriptor->uiNCoefs;
            psFIRCtrl->piCoefs            = psFIRDescriptor->piCoefs;
        break;

        // Unrecognized type
        default:
            return FIR_ERROR;
    }

    // Sync the FIR
    if(FIR_sync(psFIRCtrl) != FIR_NO_ERROR)
        return FIR_ERROR;

    return FIR_NO_ERROR;
}


// ==================================================================== //
// Function:        FIR_sync                                            //
// Arguments:        FIRCtrl_t     *psFIRCtrl: Ctrl strct.                    //
// Return values:    FIR_NO_ERROR on success                                //
//                    FIR_ERROR on failure                                //
// Description:        Syncs the FIR                                         //
// ==================================================================== //
FIRReturnCodes_t                FIR_sync(FIRCtrl_t* psFIRCtrl)
{
    unsigned int        ui;

    if(psFIRCtrl->eEnable == FIR_ON)
    {
        // Set delay line index back to base
        psFIRCtrl->piDelayI    = psFIRCtrl->piDelayB;

        // Clear delay line
        for(ui = 0; ui < psFIRCtrl->uiDelayL; ui++)
            psFIRCtrl->piDelayB[ui]    = FIR_STATE_INIT;
    }

    return FIR_NO_ERROR;
}


// ==================================================================== //
// Function:        FIR_proc_os2                                        //
// Arguments:        FIRCtrl_t     *psFIRCtrl: Ctrl strct.                    //
// Return values:    FIR_NO_ERROR on success                                //
//                    FIR_ERROR on failure                                //
// Description:        Processes the FIR in over-sample by 2 mode            //
// ==================================================================== //
FIRReturnCodes_t                FIR_proc_os2(FIRCtrl_t* psFIRCtrl)
{
    int*            piIn        = psFIRCtrl->piIn;
    unsigned int    uiInStep    = psFIRCtrl->uiInStep;
    int*            piOut        = psFIRCtrl->piOut;
    unsigned int    uiOutStep    = psFIRCtrl->uiOutStep;
    int*            piDelayB    = psFIRCtrl->piDelayB;
    int*            piDelayI    = psFIRCtrl->piDelayI;
    int*            piDelayW    = psFIRCtrl->piDelayW;
    unsigned int    uiDelayO    = psFIRCtrl->uiDelayO;
    int*            piCoefsB    = psFIRCtrl->piCoefs;
    unsigned int    uiNLoops    = psFIRCtrl->uiNLoops;
    int*            piData;
    int*            piCoefs;
    int        DWORD_ALIGNED        iData[2];
    unsigned        ui;

    for(ui = 0; ui < psFIRCtrl->uiNInSamples; ui+=2) //Note step by 2 as inner loop unrolled twice
    {
        // Get new data sample to delay line (double write for circular buffer simulation) with step
        iData[0]                    = *piIn;
        piIn                    += uiInStep;
        // Double write to simulate circular buffer
        *piDelayI               = iData[0];
        *(piDelayI + uiDelayO)  = iData[0];

        // Step delay (with circular simulation)
        piDelayI++;
        if(piDelayI >= piDelayW)
            piDelayI                = piDelayB;

        // Set access pointers
        piData                  = piDelayI;
        piCoefs                 = piCoefsB;

        //printf("piData = %p, piCoefs = %p\n", piData, piCoefs);
        if ((unsigned)piData & 0b0100)
            src_mrhf_fir_os_inner_loop_asm_odd(piData, piCoefs, iData, uiNLoops);
        else
            src_mrhf_fir_os_inner_loop_asm(piData, piCoefs, iData, uiNLoops);

        // Write output with step
        // NOTE OUTPUT WRITE ORDER: First iData[1], then iData[0]
        *piOut                  = iData[1];
        piOut                   += uiOutStep;
        *piOut                  = iData[0];
        piOut                   += uiOutStep;

        // Get new data sample to delay line (double write for circular buffer simulation) with step
        iData[0]                    = *piIn;
        piIn                    += uiInStep;
        // Double write to simulate circular buffer
        *piDelayI               = iData[0];
        *(piDelayI + uiDelayO)  = iData[0];

        // Step delay (with circular simulation)
        piDelayI++;
        if(piDelayI >= piDelayW)
            piDelayI                = piDelayB;

        // Set access pointers
        piData                  = piDelayI;
        piCoefs                 = piCoefsB;

        //printf("piData = %p, piCoefs = %p\n", piData, piCoefs);
        if ((unsigned)piData & 0b0100)
            src_mrhf_fir_os_inner_loop_asm_odd(piData, piCoefs, iData, uiNLoops);
        else
            src_mrhf_fir_os_inner_loop_asm(piData, piCoefs, iData, uiNLoops);

        // Write output with step
        // NOTE OUTPUT WRITE ORDER: First iData[1], then iData[0]
        *piOut                  = iData[1];
        piOut                   += uiOutStep;
        *piOut                  = iData[0];
        piOut                   += uiOutStep;
    }
    // Write delay line index back for next round
    psFIRCtrl->piDelayI        = piDelayI;

    return FIR_NO_ERROR;
}


// ==================================================================== //
// Function:        FIR_proc_sync                                        //
// Arguments:        FIRCtrl_t     *psFIRCtrl: Ctrl strct.                    //
// Return values:    FIR_NO_ERROR on success                                //
//                    FIR_ERROR on failure                                //
// Description:        Processes the FIR in asynchronous mode                //
// ==================================================================== //
FIRReturnCodes_t                FIR_proc_sync(FIRCtrl_t* psFIRCtrl)
{
    int*            piIn        = psFIRCtrl->piIn;
    unsigned int    uiInStep    = psFIRCtrl->uiInStep;
    int*            piOut        = psFIRCtrl->piOut;
    unsigned int    uiOutStep    = psFIRCtrl->uiOutStep;
    int*            piDelayB    = psFIRCtrl->piDelayB;
    int*            piDelayI    = psFIRCtrl->piDelayI;
    int*            piDelayW    = psFIRCtrl->piDelayW;
    unsigned int    uiDelayO    = psFIRCtrl->uiDelayO;
    int*            piCoefsB    = psFIRCtrl->piCoefs;
    unsigned int    uiNLoops    = psFIRCtrl->uiNLoops;
    int*            piData;
    int*            piCoefs;
    int                iData0;
    unsigned        ui;

    for(ui = 0; ui < psFIRCtrl->uiNInSamples; ui++)
    {
        // Get new data sample to delay line (double write for circular buffer simulation) with step
        iData0                    = *piIn;
        piIn                    += uiInStep;
        // Double write to simulate circular buffer
        *piDelayI                = iData0;
        *(piDelayI + uiDelayO)    = iData0;
        // Step delay (with circular simulation)
        piDelayI++;
        if(piDelayI >= piDelayW)
            piDelayI                = piDelayB;

        // Clear accumulator and set access pointers
        piData                    = piDelayI;
        piCoefs                    = piCoefsB;

        if ((unsigned)piData & 0b0100) src_mrhf_fir_inner_loop_asm_odd(piData, piCoefs, &iData0, uiNLoops);
        else src_mrhf_fir_inner_loop_asm(piData, piCoefs, &iData0, uiNLoops);

        // Write output with step
        *piOut                    = iData0;
        piOut                    += uiOutStep;
    }

    // Write delay line index back for next round
    psFIRCtrl->piDelayI        = piDelayI;

    return FIR_NO_ERROR;
}


// ==================================================================== //
// Function:        FIR_proc_ds2                                        //
// Arguments:        FIRCtrl_t     *psFIRCtrl: Ctrl strct.                    //
// Return values:    FIR_NO_ERROR on success                                //
//                    FIR_ERROR on failure                                //
// Description:        Processes the FIR in down-sample by 2 mode            //
// ==================================================================== //
FIRReturnCodes_t                FIR_proc_ds2(FIRCtrl_t* psFIRCtrl)
{
    int*            piIn        = psFIRCtrl->piIn;
    unsigned int    uiInStep    = psFIRCtrl->uiInStep;
    int*            piOut        = psFIRCtrl->piOut;
    unsigned int    uiOutStep    = psFIRCtrl->uiOutStep;
    int*            piDelayB    = psFIRCtrl->piDelayB;
    int*            piDelayI    = psFIRCtrl->piDelayI;
    int*            piDelayW    = psFIRCtrl->piDelayW;
    unsigned int    uiDelayO    = psFIRCtrl->uiDelayO;
    int*            piCoefsB    = psFIRCtrl->piCoefs;
    unsigned int    uiNLoops    = psFIRCtrl->uiNLoops;
    int*            piData;
    int*            piCoefs;
    int                iData0, iData1;
    unsigned        ui;

    for(ui = 0; ui < psFIRCtrl->uiNInSamples>>1; ui++)
    {
        // Get two new data samples to delay line (double write for circular buffer simulation), with input buffer step
        iData0                    = *piIn;
        piIn                    += uiInStep;
        iData1                    = *piIn;
        piIn                    += uiInStep;
        // Double write to simulate circular buffer with DUAL store instruction
        *piDelayI                = iData0;
        *(piDelayI + 1)            = iData1;
        *(piDelayI + uiDelayO)    = iData0;
        *(piDelayI + uiDelayO + 1)    = iData1;
        // Step delay with circular simulation
        piDelayI                += 2;
        if(piDelayI >= piDelayW)
            piDelayI                = piDelayB;

        // Clear accumulator and set access pointers
        piData                    = piDelayI;
        piCoefs                    = piCoefsB;
        if ((unsigned)piData & 0b0100) src_mrhf_fir_inner_loop_asm_odd(piData, piCoefs, &iData0, uiNLoops);
        else src_mrhf_fir_inner_loop_asm(piData, piCoefs, &iData0, uiNLoops);
        // Write output with step
        *piOut                    = iData0;
        piOut                    += uiOutStep;
    }

    // Write delay line index back for next round
    psFIRCtrl->piDelayI        = piDelayI;

    return FIR_NO_ERROR;
}


// ==================================================================== //
// Function:        ADFIR_init_from_desc                                //
// Arguments:        ADFIRCtrl_t     *psADFIRCtrl: Ctrl strct.            //
//                    ADFIRDescriptor_t *psADFIRDescriptor: Desc. strct.    //
// Return values:    FIR_NO_ERROR on success                                //
//                    FIR_ERROR on failure                                //
// Description:        Inits the ADFIR from the Descriptor                    //
// ==================================================================== //
FIRReturnCodes_t                ADFIR_init_from_desc(ADFIRCtrl_t* psADFIRCtrl, ADFIRDescriptor_t* psADFIRDescriptor)
{
    unsigned int        uiPhaseLength;

    // Check that delay line base has been set
    if(psADFIRCtrl->piDelayB == 0)
        return FIR_ERROR;

    // Check that adaptive coefs buffer has been set
    if(psADFIRCtrl->piADCoefs == 0)
        return FIR_ERROR;

    // Check that number of phases and number of coefficients per phaseare set
    if(psADFIRDescriptor->uiNPhases == 0)
        return FIR_ERROR;
    if(psADFIRDescriptor->uiNCoefsPerPhase == 0)
        return FIR_ERROR;

    uiPhaseLength                    = psADFIRDescriptor->uiNCoefsPerPhase;
    // Setup ADFIR
    psADFIRCtrl->uiDelayL            = uiPhaseLength<<1;                    // Double length for circular buffer simulation
    psADFIRCtrl->piDelayW            = psADFIRCtrl->piDelayB + uiPhaseLength;
    psADFIRCtrl->uiDelayO            = uiPhaseLength;
    psADFIRCtrl->uiNLoops            = uiPhaseLength>>1;                    // Due to 2 x 32bits read for data and coefs per inner loop

    // Sync the ADFIR
    if(ADFIR_sync(psADFIRCtrl) != FIR_NO_ERROR)
        return FIR_ERROR;

    return FIR_NO_ERROR;
}

// ==================================================================== //
// Function:        ADFIR_snyc                                            //
// Arguments:        ADFIRCtrl_t     *psADFIRCtrl: Ctrl strct.            //
// Return values:    FIR_NO_ERROR on success                                //
//                    FIR_ERROR on failure                                //
// Description:        Syncs the ADFIR filter                                 //
// ==================================================================== //
FIRReturnCodes_t                ADFIR_sync(ADFIRCtrl_t* psADFIRCtrl)
{
    unsigned int        ui;

    // Set delay line index back to base
    psADFIRCtrl->piDelayI    = psADFIRCtrl->piDelayB;

    // Clear delay line
    for(ui = 0; ui < psADFIRCtrl->uiDelayL; ui++)
        psADFIRCtrl->piDelayB[ui]    = FIR_STATE_INIT;

    return FIR_NO_ERROR;
}

// ==================================================================== //
// Function:        ADFIR_proc_in_sample                                //
// Arguments:        ADFIRCtrl_t     *psADFIRCtrl: Ctrl strct.            //
// Return values:    FIR_NO_ERROR on success                                //
//                    FIR_ERROR on failure                                //
// Description:        Gets new input sample to delay line    with step         //
// ==================================================================== //
FIRReturnCodes_t                ADFIR_proc_in_spl(ADFIRCtrl_t* psADFIRCtrl)
{
    // Double write to simulate circular buffer
    *psADFIRCtrl->piDelayI                                    = psADFIRCtrl->iIn;
    *(psADFIRCtrl->piDelayI + psADFIRCtrl->uiDelayO)        = psADFIRCtrl->iIn;
    // Step delay (with circular simulation)
    psADFIRCtrl->piDelayI++;
    if(psADFIRCtrl->piDelayI >= psADFIRCtrl->piDelayW)
        psADFIRCtrl->piDelayI                = psADFIRCtrl->piDelayB;

    return FIR_NO_ERROR;
}

// ==================================================================== //
// Function:        ADFIR_proc_macc                                        //
// Arguments:        ADFIRCtrl_t     *psADFIRCtrl: Ctrl strct.            //
// Return values:    FIR_NO_ERROR on success                                //
//                    FIR_ERROR on failure                                //
// Description:        Processes the macc loop for the ADFIR filter        //
// ==================================================================== //
FIRReturnCodes_t                ADFIR_proc_macc(ADFIRCtrl_t* psADFIRCtrl)
{
    int*            piData;
    int*            piCoefs;
    int                iData;



    //*(psADFIRCtrl->piOut)    = *psADFIRCtrl->piDelayI;

    // Clear accumulator and set access pointers
    piData                    = psADFIRCtrl->piDelayI;
    piCoefs                    = psADFIRCtrl->piADCoefs;
    if ((unsigned)piData & 0b0100) src_mrhf_adfir_inner_loop_asm_odd(piData, piCoefs, &iData, psADFIRCtrl->uiNLoops);
    else                               src_mrhf_adfir_inner_loop_asm(piData, piCoefs, &iData, psADFIRCtrl->uiNLoops);

    // Write output
    *(psADFIRCtrl->piOut)        = iData;

    return FIR_NO_ERROR;
}


// ==================================================================== //
// Function:        PPFIR_init_from_desc                                //
// Arguments:        PPFIRCtrl_t     *psPPFIRCtrl: Ctrl strct.            //
//                    PPFIRDescriptor_t *psPPFIRDescriptor: Desc. strct.    //
// Return values:    FIR_NO_ERROR on success                                //
//                    FIR_ERROR on failure                                //
// Description:        Inits the PPFIR from the Descriptor                    //
// ==================================================================== //
FIRReturnCodes_t                PPFIR_init_from_desc(PPFIRCtrl_t* psPPFIRCtrl, PPFIRDescriptor_t* psPPFIRDescriptor)
{
    unsigned int        uiPhaseLength;

    // Check if PPFIR is disabled (this is given by the number of coefficients being zero)
    if( psPPFIRDescriptor->uiNCoefs == 0)
    {
        // Number of coefficients is zero, so disable FIR
        psPPFIRCtrl->eEnable            = FIR_OFF;
        psPPFIRCtrl->uiNOutSamples        = 0;
        psPPFIRCtrl->uiDelayL            = 0;
        psPPFIRCtrl->piDelayW            = 0;
        psPPFIRCtrl->uiDelayO            = 0;
        psPPFIRCtrl->uiNLoops            = 0;
        psPPFIRCtrl->uiNCoefs            = 0;
        psPPFIRCtrl->piCoefs            = 0;
        psPPFIRCtrl->uiNPhases            = 0;
        psPPFIRCtrl->uiPhaseStep        = 0;
        psPPFIRCtrl->uiCoefsPhaseStep    = 0;
        psPPFIRCtrl->uiCoefsPhase        = 0;
        return FIR_NO_ERROR;
    }

    // Check that delay line base has been set
    if(psPPFIRCtrl->piDelayB == 0)
        return FIR_ERROR;

    // Check that number of samples has been set and is a multiple of 2
    if(psPPFIRCtrl->uiNInSamples == 0)
        return FIR_ERROR;

    // Check the input and output samples steps have been set
    if(psPPFIRCtrl->uiInStep == 0)
        return FIR_ERROR;
    if(psPPFIRCtrl->uiOutStep == 0)
        return FIR_ERROR;

    // Check that phase step has been set properly
    if(psPPFIRCtrl->uiPhaseStep == 0)
        return FIR_ERROR;

    // Check that number of phases and number of coefficients are compatible
    uiPhaseLength                    = (psPPFIRDescriptor->uiNCoefs / psPPFIRDescriptor->uiNPhases);
    if(uiPhaseLength == 0)
        return FIR_ERROR;
    if((uiPhaseLength * psPPFIRDescriptor->uiNPhases) != psPPFIRDescriptor->uiNCoefs)
        return FIR_ERROR;
    if((uiPhaseLength & 0x1) != 0)
        return FIR_ERROR;

    // Setup PPFIR
    psPPFIRCtrl->eEnable            = FIR_ON;
    psPPFIRCtrl->uiDelayL            = uiPhaseLength<<1;                                    // Double length for circular buffer simulation
    psPPFIRCtrl->piDelayW            = psPPFIRCtrl->piDelayB + uiPhaseLength;
    psPPFIRCtrl->uiDelayO            = uiPhaseLength;
    psPPFIRCtrl->uiNLoops            = uiPhaseLength>>1;                                    // Due to 2 x 32bits read for data and coefs per inner loop
    psPPFIRCtrl->uiNCoefs            = psPPFIRDescriptor->uiNCoefs;
    psPPFIRCtrl->piCoefs            = psPPFIRDescriptor->piCoefs;
    psPPFIRCtrl->uiNPhases            = psPPFIRDescriptor->uiNPhases;
    psPPFIRCtrl->uiCoefsPhaseStep    = psPPFIRCtrl->uiPhaseStep * uiPhaseLength;            // Length (number of coefs) of a phase step

    // Sync the PPFIR
    if(PPFIR_sync(psPPFIRCtrl) != FIR_NO_ERROR)
        return FIR_ERROR;

    return FIR_NO_ERROR;
}


// ==================================================================== //
// Function:        PPFIR_snyc                                            //
// Arguments:        PPFIRCtrl_t     *psPPFIRCtrl: Ctrl strct.            //
// Return values:    FIR_NO_ERROR on success                                //
//                    FIR_ERROR on failure                                //
// Description:        Syncs the PPFIR polyphase filter                     //
// ==================================================================== //
FIRReturnCodes_t                PPFIR_sync(PPFIRCtrl_t* psPPFIRCtrl)
{
    unsigned int        ui;

    if(psPPFIRCtrl->eEnable == FIR_ON)
    {
        // Set delay line index back to base
        psPPFIRCtrl->piDelayI    = psPPFIRCtrl->piDelayB;

        // Clear delay line
        for(ui = 0; ui < psPPFIRCtrl->uiDelayL; ui++)
            psPPFIRCtrl->piDelayB[ui]    = FIR_STATE_INIT;

        // Set coefficients phase to zero
        psPPFIRCtrl->uiCoefsPhase        = 0;

        // Clear number of output sampes
        psPPFIRCtrl->uiNOutSamples        = 0;
    }


    return FIR_NO_ERROR;
}


// ==================================================================== //
// Function:        PPFIR_proc                                            //
// Arguments:        PPFIRCtrl_t     *psPPFIRCtrl: Ctrl strct.            //
// Return values:    FIR_NO_ERROR on success                                //
//                    FIR_ERROR on failure                                //
// Description:        Processes the PPFIR polyphase filter                 //
// ==================================================================== //
FIRReturnCodes_t                PPFIR_proc(PPFIRCtrl_t* psPPFIRCtrl)
{
    int*            piIn                = psPPFIRCtrl->piIn;
    unsigned int    uiInStep            = psPPFIRCtrl->uiInStep;
    int*            piOut                = psPPFIRCtrl->piOut;
    unsigned int    uiOutStep            = psPPFIRCtrl->uiOutStep;
    int*            piDelayB            = psPPFIRCtrl->piDelayB;
    int*            piDelayI            = psPPFIRCtrl->piDelayI;
    int*            piDelayW            = psPPFIRCtrl->piDelayW;
    unsigned int    uiDelayO            = psPPFIRCtrl->uiDelayO;
    int*            piCoefsB            = psPPFIRCtrl->piCoefs;
    unsigned int    uiNLoops            = psPPFIRCtrl->uiNLoops;
    unsigned int    uiNCoefs            = psPPFIRCtrl->uiNCoefs;
    unsigned int    uiCoefsPhase        = psPPFIRCtrl->uiCoefsPhase;
    unsigned int    uiCoefsPhaseStep    = psPPFIRCtrl->uiCoefsPhaseStep;
    int*            piData;
    int*            piCoefs;
    int                iData[2];
    unsigned int    uiNOutSamples        = 0;
    unsigned        ui;


    for(ui = 0; ui < psPPFIRCtrl->uiNInSamples; ui++)
    {
        // Get new data sample to delay line (double write for circular buffer simulation) with step
        iData[0]                    = *piIn;
        piIn                    += uiInStep;
        // Double write to simulate circular buffer
        *piDelayI                = iData[0];
        *(piDelayI + uiDelayO)    = iData[0];
        // Step delay (with circular simulation)
        piDelayI++;
        if(piDelayI >= piDelayW)
            piDelayI                = piDelayB;

        // Do while the current phase coefficient pointer points to phase coefficients
        // This is equivalent to know if the output sample is between the current and next input sample

        while(uiCoefsPhase < uiNCoefs)
        {
            // Clear accumulator and set access pointers
            piData                    = piDelayI;
            piCoefs                    = piCoefsB + uiCoefsPhase;

            if ((unsigned)piData & 0b0100) src_mrhf_fir_inner_loop_asm_odd(piData, piCoefs, iData, uiNLoops);
            else src_mrhf_fir_inner_loop_asm(piData, piCoefs, iData, uiNLoops);


            // Write output with step
            *piOut                  = iData[0];
            piOut                   += uiOutStep;

            // Step phase coefficient offset for next output phase
            uiCoefsPhase            += uiCoefsPhaseStep;

            // Increase output sample counter
            uiNOutSamples++;
        }

        // Reduce phase for next input sample (one input sample corresponds to all coefficients in the PP filter)
        uiCoefsPhase                -= uiNCoefs;
    }

    // Write delay line index back for next round
    psPPFIRCtrl->piDelayI        = piDelayI;
    // Write coefs phase back for next round
    psPPFIRCtrl->uiCoefsPhase    = uiCoefsPhase;
    // Write number of samples
    psPPFIRCtrl->uiNOutSamples    = uiNOutSamples;

    return FIR_NO_ERROR;
}
