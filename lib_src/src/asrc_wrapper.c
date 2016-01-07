// General includes
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <timer.h>

// ASRC includes
#include "asrc.h"

static void bomb_out(int code){
    printf("ASRC_proc Error code %d\n", code);
    delay_milliseconds(1);
	_Exit(0);
}

// ASRC instances variables
// ------------------------
// State, Stack, Coefs and Control structures (one for each channel)
ASRCState_t     sASRCState[ASRC_N_CHANNELS];
int             iASRCStack[ASRC_N_CHANNELS][ASRC_STACK_LENGTH_MULT * SRC_N_IN_SAMPLES];
ASRCCtrl_t      sASRCCtrl[ASRC_N_CHANNELS];
int             iASRCADFIRCoefs[ASRC_ADFIR_COEFS_LENGTH];


extern ASRCFsRatioConfigs_t     sFsRatioConfigs[ASRC_N_FS][ASRC_N_FS];

unsigned asrc_init(unsigned sr_in, unsigned sr_out, unsigned thread_number)
{
    int ui;
    int chan_start  = thread_number * ASRC_CHANNELS_PER_CORE;         //Index of first channel processed by this thread
    int chan_end    = ((thread_number + 1) * ASRC_CHANNELS_PER_CORE); //Index + 1 of the last channel processed by this thread

    ASRCReturnCodes_t ret_code;

    printf("Thread %d, chan_start=%d, chan_end=%d\n", thread_number, chan_start, chan_end);

    ret_code = ASRC_prepare_coefs();
    if (ret_code != ASRC_NO_ERROR) bomb_out(10);

    for(ui = chan_start; ui < chan_end; ui++)
    {
        // Set state, stack and coefs into ctrl structure
        sASRCCtrl[ui].psState                   = &sASRCState[ui];
        sASRCCtrl[ui].piStack                   = iASRCStack[ui];
        sASRCCtrl[ui].piADCoefs                 = iASRCADFIRCoefs;

        // Set input/output sampling rate codes
        sASRCCtrl[ui].eInFs                     = sr_in;
        sASRCCtrl[ui].eOutFs                    = sr_out;

        // Set number of samples
        sASRCCtrl[ui].uiNInSamples              = SRC_N_IN_SAMPLES;

        // Set dither flag and random seeds
        sASRCCtrl[ui].uiDitherOnOff             = ASRC_DITHER_SETTING;
        sASRCCtrl[ui].uiRndSeedInit             = 12345 * ui;   //Some randomish numbers

        // Init ASRC instances
        ret_code = ASRC_init(&sASRCCtrl[ui]);
        if (ret_code != ASRC_NO_ERROR) bomb_out(11);
    }

    // Sync
    // ----
    // Sync ASRC. This is just to show that the function works and returns success
    for(ui = chan_start; ui < chan_end; ui++)    {
        ret_code = ASRC_sync(&sASRCCtrl[ui]);
        if (ret_code != ASRC_NO_ERROR) bomb_out(12);
    }

    return (sASRCCtrl[chan_start].uiFsRatio);


}

unsigned asrc_process(int *in_buff, int *out_buff, unsigned FsRatio, unsigned thread_number){

    int ui, uj; //General counters
    int             uiSplCntr;  //Spline counter
    int chan_start  = thread_number * ASRC_CHANNELS_PER_CORE;
    int chan_end    = ((thread_number + 1) * ASRC_CHANNELS_PER_CORE);


    for(ui = chan_start; ui < chan_end; ui++)
    {
    // Update Fs Ratio
        sASRCCtrl[ui].uiFsRatio     = FsRatio;

#if DO_FS_BOUNDS_CHECK
        // Check for bounds of new Fs ratio
        if( (FsRatio < sFsRatioConfigs[sASRCCtrl[ui].eInFs][sASRCCtrl[ui].eOutFs].uiMinFsRatio) ||
            (FsRatio > sFsRatioConfigs[sASRCCtrl[ui].eInFs][sASRCCtrl[ui].eOutFs].uiMaxFsRatio) )
            printf("FsRatio error");
#endif
        // Apply shift to time ratio to build integer and fractional parts of time step
        sASRCCtrl[ui].iTimeStepInt     = FsRatio >> (sFsRatioConfigs[sASRCCtrl[ui].eInFs][sASRCCtrl[ui].eOutFs].iFsRatioShift);
        sASRCCtrl[ui].uiTimeStepFract  = FsRatio << (32 - sFsRatioConfigs[sASRCCtrl[ui].eInFs][sASRCCtrl[ui].eOutFs].iFsRatioShift);




        // Set input and output data pointers
        sASRCCtrl[ui].piIn          = in_buff + ui - (thread_number * ASRC_CHANNELS_PER_CORE);
        sASRCCtrl[ui].piOut         = out_buff + ui - (thread_number * ASRC_CHANNELS_PER_CORE);


    // Process synchronous part (F1 + F2)
    // ==================================

        // Note: this is block based similar to SSRC, output will be on stack
        // and there will be sASRCCtrl[chan_start].uiNSyncSamples samples per channel produced
        if(ASRC_proc_F1_F2(&sASRCCtrl[ui]) != ASRC_NO_ERROR)
        {
            bomb_out(12);
        }



    // Run the asynchronous part (F3)
    // ==============================
    // Clear number of output samples (note that this sample counter would actually not be needed if all was sample by sampe)

        sASRCCtrl[ui].uiNASRCOutSamples = 0;
    }

    uiSplCntr = 0; // This is actually only used because of the bizarre mix of block and sample based processing

    // Driven by samples produced during the synchronous phase
    for(ui = 0; ui < sASRCCtrl[chan_start].uiNSyncSamples; ui++)
    {
        // Push new samples into F3 delay line (input from stack) for each new "synchronous" sample (i.e. output of F1, respectively F2)
        for(uj = chan_start; uj < chan_end; uj++)
        {

            //The following is replicated/inlined code from ASRC_proc_F3_in_spl in ASRC.c
            sASRCCtrl[uj].sADFIRF3Ctrl.iIn        = sASRCCtrl[uj].piStack[ui];

            //The following is replicated/inlined code from ADFIR_proc_in_spl in FIR.c
            // Double write to simulate circular buffer
            *sASRCCtrl[uj].sADFIRF3Ctrl.piDelayI                                  = sASRCCtrl[uj].sADFIRF3Ctrl.iIn;
            *(sASRCCtrl[uj].sADFIRF3Ctrl.piDelayI + sASRCCtrl[uj].sADFIRF3Ctrl.uiDelayO)        = sASRCCtrl[uj].sADFIRF3Ctrl.iIn;
            // Step delay (with circular simulation)
            sASRCCtrl[uj].sADFIRF3Ctrl.piDelayI++;
            if(sASRCCtrl[uj].sADFIRF3Ctrl.piDelayI >= sASRCCtrl[uj].sADFIRF3Ctrl.piDelayW)
                sASRCCtrl[uj].sADFIRF3Ctrl.piDelayI               =sASRCCtrl[uj].sADFIRF3Ctrl.piDelayB;

            // Decrease next output time (this is an integer value, so no influence on fractional part)
            sASRCCtrl[uj].iTimeInt    -= FILTER_DEFS_ADFIR_N_PHASES;


        }
        // Run macc loop for F3
        // Check if a new output sample needs to be produced
        // Note that this will also update the adaptive filter coefficients
        // These must be computed for one channel only and reused in the macc loop of other channels
        while(sASRCCtrl[chan_start].iTimeInt < FILTER_DEFS_ADFIR_N_PHASES)
        {
            unsigned int    uiTemp;
            int             iAlpha;

            int             iH[3]; //iH0, iH1, iH2;
            long long       i64Acc0;
            int*            piPhase0;
            int*            piADCoefs;

            // Compute adative coefficients spline factors
            // The fractional part of time gives alpha
            iAlpha      = sASRCCtrl[chan_start].uiTimeFract>>1;      // Now alpha can be seen as a signed number
            i64Acc0 = iAlpha * iAlpha;
            iH[0]           = (int)(i64Acc0>>32);
            iH[2]           = 0x40000000;                       // Load H2 with 0.5;
            iH[1]           = iH[2] - iH[0];                        // H1 = 0.5 - 0.5 * alpha * alpha;
            iH[1]           = iH[1] - iH[0];                        // H1 = 0.5 - alpha * alpha
            iH[1]           = iH[1] + iAlpha;                       // H1 = 0.5 + alpha - alpha * alpha;
            iH[2]           = iH[2] - iAlpha;                       // H2 = 0.5 - alpha
            iH[2]           = iH[2] + iH[0];                        // H2 = 0.5 - alpha + 0.5 * alpha * alpha

            // The integer part of time gives the phase
            piPhase0        = iADFirCoefs[sASRCCtrl[chan_start].iTimeInt];
            // These are calculated by the asm funcion  piPhase1        = piPhase0 + FILTER_DEFS_ADFIR_PHASE_N_TAPS;
            //                                          piPhase2        = piPhase1 + FILTER_DEFS_ADFIR_PHASE_N_TAPS;
            piADCoefs       = sASRCCtrl[chan_start].piADCoefs;       // Given limited number of registers, this should be DP

            spline_coeff_gen_inner_loop_asm(piPhase0, iH, piADCoefs, FILTER_DEFS_ADFIR_PHASE_N_TAPS);

            // Step time for next output sample
            // --------------------------------
            // Step to next output time (add integer and fractional parts)
            sASRCCtrl[chan_start].iTimeInt       += sASRCCtrl[chan_start].iTimeStepInt;
            // For fractional part, this can be optimized using the add with carry instruction of XS2
            uiTemp      = sASRCCtrl[chan_start].uiTimeFract;
            sASRCCtrl[chan_start].uiTimeFract        += sASRCCtrl[chan_start].uiTimeStepFract;
            if(sASRCCtrl[chan_start].uiTimeFract < uiTemp)
                sASRCCtrl[chan_start].iTimeInt++;


            // Not really needed, just for the beauty of it...
            //sASRCCtrl[chan_start+1].iTimeInt     = sASRCCtrl[chan_start].iTimeInt;
            //sASRCCtrl[chan_start+1].uiTimeFract  = sASRCCtrl[chan_start].uiTimeFract;

            // Apply filter F3 with just computed adaptive coefficients
            for(uj = chan_start; uj < chan_end; uj++)    {

                //The following is replicated/inlined code from ADFIR_F3_proc_macc in ASRC.c
                sASRCCtrl[uj].sADFIRF3Ctrl.piOut      = (sASRCCtrl[uj].piOut + ASRC_CHANNELS_PER_CORE * uiSplCntr);

                //The following is replicated/inlined code from ADFIR_proc_macc in FIR.c
                int*            piData;
                int*            piCoefs;
                int             iData;
                // Clear accumulator and set access pointers
                piData                  = sASRCCtrl[uj].sADFIRF3Ctrl.piDelayI;
                piCoefs                 = sASRCCtrl[uj].sADFIRF3Ctrl.piADCoefs;
                if ((unsigned)piData & 0b0100) adfir_inner_loop_asm_odd(piData, piCoefs, &iData, sASRCCtrl[uj].sADFIRF3Ctrl.uiNLoops);
                else                               adfir_inner_loop_asm(piData, piCoefs, &iData, sASRCCtrl[uj].sADFIRF3Ctrl.uiNLoops);

                // Write output
                *(sASRCCtrl[uj].sADFIRF3Ctrl.piOut)       = iData;
                sASRCCtrl[uj].uiNASRCOutSamples++;
            }
            uiSplCntr++; // This is actually only used because of the bizarre mix of block and sample based processing
        }
    }


#if (ASRC_DITHER_SETTING != ASRC_DITHER_OFF)
    // Process dither part
    // ===================
    // We are back to block based processing. This is where the number of ASRC output samples is required again
    // (would not be used if sample by sample based (on output samples))
    for(ui = chan_start; ui < chan_end; ui++)    {
        // Note: this is block based similar to SSRC
        if(ASRC_proc_dither(&sASRCCtrl[ui]) != ASRC_NO_ERROR)
        {
            bomb_out(4);
        }
    }
#endif

    unsigned n_samps_out = sASRCCtrl[chan_start].uiNASRCOutSamples;
    return n_samps_out;
}
