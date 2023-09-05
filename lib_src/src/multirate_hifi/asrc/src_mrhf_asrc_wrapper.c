// Copyright 2016-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// General includes
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <timer.h>
#include "debug_print.h"

// ASRC includes
#include "src.h"

extern ASRCFsRatioConfigs_t     sFsRatioConfigs[ASRC_N_FS][ASRC_N_FS];

#define DO_FS_BOUNDS_CHECK      1   //This is important to prevent pointers going out of bounds when invalid fs_ratios are sent

static void asrc_error(int code)
{
    debug_printf("ASRC_proc Error code %d\n", code);
    delay_milliseconds(1); //Allow xscope to display message before quit
    _Exit(code);
}


uint64_t asrc_init(const fs_code_t sr_in, const fs_code_t sr_out, asrc_ctrl_t asrc_ctrl[], const unsigned n_channels_per_instance,
        const unsigned n_in_samples, const dither_flag_t dither_on_off)
{
    unsigned ui;

    ASRCReturnCodes_t ret_code;

    ret_code = ASRC_prepare_coefs();

    if (ret_code != ASRC_NO_ERROR) asrc_error(10);

    //Check to see if n_channels_per_instance, n_in_samples are reasonable
    if ((n_in_samples & 0x1) || (n_in_samples < 4)) asrc_error(100);
    if (n_channels_per_instance < 1) asrc_error(101);

    for(ui = 0; ui < n_channels_per_instance; ui++)
    {
        // Set number of channels per instance
        asrc_ctrl[ui].uiNchannels               = n_channels_per_instance;

        // Set input/output sampling rate codes
        asrc_ctrl[ui].eInFs                     = (int)sr_in;
        asrc_ctrl[ui].eOutFs                    = (int)sr_out;

        // Set number of samples
        asrc_ctrl[ui].uiNInSamples              = n_in_samples;

        // Set dither flag and random seeds
        asrc_ctrl[ui].uiDitherOnOff             = dither_on_off;
        asrc_ctrl[ui].uiRndSeedInit             = 12345 * ui;   //Some randomish numbers. Value not critical

        // Init ASRC instances
        ret_code = ASRC_init(&asrc_ctrl[ui]);

        if (ret_code != ASRC_NO_ERROR) asrc_error(11);
    }

    // Sync
    // ----
    // Sync ASRC. This is just to show that the function works and returns success

    for(ui = 0; ui < n_channels_per_instance; ui++)    {
        ret_code = ASRC_sync(&asrc_ctrl[ui]);
        if (ret_code != ASRC_NO_ERROR) asrc_error(12);
    }

    return (uint64_t)((((uint64_t)asrc_ctrl[0].uiFsRatio) << 32) | asrc_ctrl[0].uiFsRatio_lo);
}

unsigned asrc_process(int *in_buff, int *out_buff, uint64_t fs_ratio, asrc_ctrl_t asrc_ctrl[]){

    int ui, uj; //General counters
    int             uiSplCntr;  //Spline counter

    // Get the number of channels per instance from first channel
    const unsigned n_channels_per_instance = asrc_ctrl[0].uiNchannels;

    uint32_t fs_ratio_hi = (uint32_t)(fs_ratio >> 32);
    uint32_t fs_ratio_lo = (uint32_t)(fs_ratio);

    for(ui = 0; ui < n_channels_per_instance; ui++)
    {
    // Update Fs Ratio
        asrc_ctrl[ui].uiFsRatio     = fs_ratio_hi;
        asrc_ctrl[ui].uiFsRatio_lo = fs_ratio_lo;

#if DO_FS_BOUNDS_CHECK
        // Check for bounds of new Fs ratio
        if( (fs_ratio_hi < sFsRatioConfigs[asrc_ctrl[ui].eInFs][asrc_ctrl[ui].eOutFs].uiMinFsRatio) ||
            (fs_ratio_hi > sFsRatioConfigs[asrc_ctrl[ui].eInFs][asrc_ctrl[ui].eOutFs].uiMaxFsRatio) )
        {
            //debug_printf("Passed = %x, Nominal = 0x%x\n", fs_ratio_hi, sFsRatioConfigs[asrc_ctrl[ui].eInFs][asrc_ctrl[ui].eOutFs].uiNominalFsRatio);
            fs_ratio_hi = sFsRatioConfigs[asrc_ctrl[ui].eInFs][asrc_ctrl[ui].eOutFs].uiNominalFsRatio; //Important to prevent buffer overflow if fs_ratio requests too many samples.
            fs_ratio_lo = sFsRatioConfigs[asrc_ctrl[ui].eInFs][asrc_ctrl[ui].eOutFs].uiNominalFsRatio_lo;
            //debug_printf("!");
        }
#endif
        // Apply shift to time ratio to build integer and fractional parts of time step
        asrc_ctrl[ui].iTimeStepInt     = fs_ratio_hi >> (sFsRatioConfigs[asrc_ctrl[ui].eInFs][asrc_ctrl[ui].eOutFs].iFsRatioShift);
        asrc_ctrl[ui].uiTimeStepFract  = fs_ratio_hi << (32 - sFsRatioConfigs[asrc_ctrl[ui].eInFs][asrc_ctrl[ui].eOutFs].iFsRatioShift);
        asrc_ctrl[ui].uiTimeStepFract |= (uint32_t)(fs_ratio_lo >> sFsRatioConfigs[asrc_ctrl[ui].eInFs][asrc_ctrl[ui].eOutFs].iFsRatioShift);




        // Set input and output data pointers
        asrc_ctrl[ui].piIn          = in_buff + ui;
        asrc_ctrl[ui].piOut         = out_buff + ui;


    // Process synchronous part (F1 + F2)
    // ==================================

        // Note: this is block based similar to SSRC, output will be on stack
        // and there will be asrc_ctrl[chan_start].uiNSyncSamples samples per channel produced
        if(ASRC_proc_F1_F2(&asrc_ctrl[ui]) != ASRC_NO_ERROR)
        {
            asrc_error(12);
        }



    // Run the asynchronous part (F3)
    // ==============================
    // Clear number of output samples (note that this sample counter would actually not be needed if all was sample by sampe)

        asrc_ctrl[ui].uiNASRCOutSamples = 0;
    }

    uiSplCntr = 0; // This is actually only used because of the bizarre mix of block and sample based processing

    // Driven by samples produced during the synchronous phase
    for(ui = 0; ui < asrc_ctrl[0].uiNSyncSamples; ui++)
    {
        // Push new samples into F3 delay line (input from stack) for each new "synchronous" sample (i.e. output of F1, respectively F2)
        for(uj = 0; uj < n_channels_per_instance; uj++)
        {

            //The following is replicated/inlined code from ASRC_proc_F3_in_spl in ASRC.c
            asrc_ctrl[uj].sADFIRF3Ctrl.iIn        = asrc_ctrl[uj].piStack[ui];

            //The following is replicated/inlined code from ADFIR_proc_in_spl in FIR.c
            // Double write to simulate circular buffer
            *asrc_ctrl[uj].sADFIRF3Ctrl.piDelayI                                  = asrc_ctrl[uj].sADFIRF3Ctrl.iIn;
            *(asrc_ctrl[uj].sADFIRF3Ctrl.piDelayI + asrc_ctrl[uj].sADFIRF3Ctrl.uiDelayO)        = asrc_ctrl[uj].sADFIRF3Ctrl.iIn;
            // Step delay (with circular simulation)
            asrc_ctrl[uj].sADFIRF3Ctrl.piDelayI++;
            if(asrc_ctrl[uj].sADFIRF3Ctrl.piDelayI >= asrc_ctrl[uj].sADFIRF3Ctrl.piDelayW)
                asrc_ctrl[uj].sADFIRF3Ctrl.piDelayI               =asrc_ctrl[uj].sADFIRF3Ctrl.piDelayB;

            // Decrease next output time (this is an integer value, so no influence on fractional part)
            asrc_ctrl[uj].iTimeInt    -= FILTER_DEFS_ADFIR_N_PHASES;


        }
        // Run macc loop for F3
        // Check if a new output sample needs to be produced
        // Note that this will also update the adaptive filter coefficients
        // These must be computed for one channel only and reused in the macc loop of other channels
        while(asrc_ctrl[0].iTimeInt < FILTER_DEFS_ADFIR_N_PHASES)
        {
            unsigned int    uiTemp;
            int             iAlpha;

            int             iH[3]; //iH0, iH1, iH2;
            long long       i64Acc0;
            int*            piPhase0;
            int*            piADCoefs;

            // Compute adative coefficients spline factors
            // The fractional part of time gives alpha
            iAlpha      = asrc_ctrl[0].uiTimeFract>>1;      // Now alpha can be seen as a signed number
            i64Acc0 = (long long)iAlpha * (long long)iAlpha;

            iH[0]           = (int)(i64Acc0>>32);
            iH[2]           = 0x40000000;                       // Load H2 with 0.5;
            iH[1]           = iH[2] - iH[0];                        // H1 = 0.5 - 0.5 * alpha * alpha;
            iH[1]           = iH[1] - iH[0];                        // H1 = 0.5 - alpha * alpha
            iH[1]           = iH[1] + iAlpha;                       // H1 = 0.5 + alpha - alpha * alpha;
            iH[2]           = iH[2] - iAlpha;                       // H2 = 0.5 - alpha
            iH[2]           = iH[2] + iH[0];                        // H2 = 0.5 - alpha + 0.5 * alpha * alpha

            // The integer part of time gives the phase
            piPhase0        = iADFirCoefs[asrc_ctrl[0].iTimeInt];
            // These are calculated by the asm funcion  piPhase1        = piPhase0 + FILTER_DEFS_ADFIR_PHASE_N_TAPS;
            //                                          piPhase2        = piPhase1 + FILTER_DEFS_ADFIR_PHASE_N_TAPS;
            piADCoefs       = asrc_ctrl[0].piADCoefs;       // Given limited number of registers, this could be DP

            // Apply spline coefficients to filter coefficients
            src_mrhf_spline_coeff_gen_inner_loop_asm(piPhase0, iH, piADCoefs, FILTER_DEFS_ADFIR_PHASE_N_TAPS);


            // Step time for next output sample
            // --------------------------------
            // Step to next output time (add integer and fractional parts)
            asrc_ctrl[0].iTimeInt       += asrc_ctrl[0].iTimeStepInt;
            // For fractional part, this can be optimized using the add with carry instruction of XS2
            uiTemp      = asrc_ctrl[0].uiTimeFract;
            asrc_ctrl[0].uiTimeFract        += asrc_ctrl[0].uiTimeStepFract;
            if(asrc_ctrl[0].uiTimeFract < uiTemp)
                asrc_ctrl[0].iTimeInt++;


            // Not really needed, just for the beauty of it...
            //asrc_ctrl[0+1].iTimeInt     = asrc_ctrl[0].iTimeInt;
            //asrc_ctrl[0+1].uiTimeFract  = asrc_ctrl[0].uiTimeFract;

            // Apply filter F3 with just computed adaptive coefficients
            for(uj = 0; uj < n_channels_per_instance; uj++)    {

                //The following is replicated/inlined code from ADFIR_F3_proc_macc in ASRC.c
                asrc_ctrl[uj].sADFIRF3Ctrl.piOut      = (asrc_ctrl[uj].piOut + n_channels_per_instance * uiSplCntr);

                //The following is replicated/inlined code from ADFIR_proc_macc in FIR.c
                int*            piData;
                int*            piCoefs;
                int             iData;
                // Clear accumulator and set access pointers
                piData                  = asrc_ctrl[uj].sADFIRF3Ctrl.piDelayI;
                piCoefs                 = asrc_ctrl[uj].sADFIRF3Ctrl.piADCoefs;

                // Do FIR
                if ((unsigned)piData & 0b0100) src_mrhf_adfir_inner_loop_asm_odd(piData, piCoefs, &iData, asrc_ctrl[uj].sADFIRF3Ctrl.uiNLoops);
                else                               src_mrhf_adfir_inner_loop_asm(piData, piCoefs, &iData, asrc_ctrl[uj].sADFIRF3Ctrl.uiNLoops);

                // Write output
                *(asrc_ctrl[uj].sADFIRF3Ctrl.piOut)       = iData;
                asrc_ctrl[uj].uiNASRCOutSamples++;
            }
            uiSplCntr++; // This is actually only used because of the bizarre mix of block and sample based processing
        }
    }


#if (ASRC_DITHER_SETTING != ASRC_DITHER_OFF)    //Removed for speed optimisation
    // Process dither part
    // ===================
    // We are back to block based processing. This is where the number of ASRC output samples is required again
    // (would not be used if sample by sample based (on output samples))
    for(ui = 0; ui < n_channels_per_instance; ui++)
    {
        // Note: this is block based similar to SSRC
        if(ASRC_proc_dither(&asrc_ctrl[ui]) != ASRC_NO_ERROR)
        {
            asrc_error(4);
        }
    }
#endif

    unsigned n_samps_out = asrc_ctrl[0].uiNASRCOutSamples;
    return n_samps_out;
}
