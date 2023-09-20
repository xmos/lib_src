// Copyright 2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// General includes
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <assert.h>

// ASRC includes
#include "ASRC_wrapper.h"
#include "ASRC.h"

#define			N_IN_SAMPLES_MAX								(1024)
// State, Stack, Coefs and Control structures (one for each channel)
static ASRCState_t		sASRCState[MAX_ASRC_N_IO_CHANNELS];
static int				iASRCStack[MAX_ASRC_N_IO_CHANNELS][ASRC_STACK_LENGTH_MULT * N_IN_SAMPLES_MAX];
static ASRCCtrl_t		sASRCCtrl[MAX_ASRC_N_IO_CHANNELS];
static int				iASRCADFIRCoefs[ASRC_ADFIR_COEFS_LENGTH];

//Helper function for converting sample to fs index value
unsigned samp_rate_to_code(unsigned samp_rate){
    unsigned samp_code = 0xdead;
    switch (samp_rate){
    case 44100:
        samp_code = ASRC_FS_44;
        break;
    case 48000:
        samp_code = ASRC_FS_48;
        break;
    case 88200:
        samp_code = ASRC_FS_88;
        break;
    case 96000:
        samp_code = ASRC_FS_96;
        break;
    case 176400:
        samp_code = ASRC_FS_176;
        break;
    case 192000:
        samp_code = ASRC_FS_192;
        break;
    }
    return samp_code;
}

static unsigned g_num_io_channels;
static unsigned g_num_channels_per_asrc_instance;


uint64_t wrapper_asrc_init(
		ASRCCtrl_profile_only_t* (*profile_info_ptr)[MAX_ASRC_N_IO_CHANNELS],
        unsigned uiInFs,
        unsigned uiOutFs,
        unsigned uiNInSamples,
        unsigned num_io_channels,
        unsigned num_channels_per_asrc_instance,
		unsigned dither_on_off,
		unsigned *rand_seed)
{
    assert(num_io_channels <= MAX_ASRC_N_IO_CHANNELS);
	assert(num_channels_per_asrc_instance <= MAX_ASRC_N_IO_CHANNELS);

	g_num_io_channels = num_io_channels;
	g_num_channels_per_asrc_instance = num_channels_per_asrc_instance;

    unsigned int	ui, uj;

    // Prepare the ASRC coefficients
	if(ASRC_prepare_coefs() != ASRC_NO_ERROR)
	{
		printf("Error at ASRC coefficients preparation");
		assert(0);
	}

	for(ui = 0; ui < num_io_channels; ui++)
	{
		// Set state, stack and coefs into ctrl structure
		sASRCCtrl[ui].psState					= &sASRCState[ui];
		sASRCCtrl[ui].piStack					= iASRCStack[ui];
		sASRCCtrl[ui].piADCoefs					= iASRCADFIRCoefs;

		// Set input/output sampling rate codes
		sASRCCtrl[ui].eInFs						= samp_rate_to_code(uiInFs);
		sASRCCtrl[ui].eOutFs					= samp_rate_to_code(uiOutFs);

		// Set number of samples
		sASRCCtrl[ui].uiNInSamples				= uiNInSamples;

		// Set dither flag and random seeds
		sASRCCtrl[ui].uiDitherOnOff				= dither_on_off;
		sASRCCtrl[ui].uiRndSeedInit				= rand_seed[ui];

		sASRCCtrl[ui].uiInStep					= num_io_channels / num_channels_per_asrc_instance;
		sASRCCtrl[ui].uiOutStep					= num_channels_per_asrc_instance;

		profile_info_ptr[0][ui] = &sASRCCtrl[ui].sProfilingInfo;

		// Init ASRC instances
		if(ASRC_init(&sASRCCtrl[ui]) != ASRC_NO_ERROR)
		{
			printf("Error at ASRC initialization");
			assert(0);
		}
	}

    // Sync
	// ----
	// Sync ASRC. This is just to show that the function works and returns success
	for(ui = 0; ui < num_io_channels; ui++)
	{
		if(ASRC_sync(&sASRCCtrl[ui]) != ASRC_NO_ERROR)
		{
			printf("Error at ASRC sync");
			assert(0);
		}
	}
	return (uint64_t)(((uint64_t)sASRCCtrl[0].uiFsRatio << 32) | (uint64_t)sASRCCtrl[0].uiFsRatio_lo);
}


unsigned wrapper_asrc_process(
	int *piIn,
	int *piOut,
	uint64_t fs_ratio)
{
    unsigned int	ui, uj;
    int				uiSplCntr;

    for(ui = 0; ui < g_num_io_channels; ui++)
	{
		// Make Fs Ratio deviate
		sASRCCtrl[ui].uiFsRatio		= (unsigned int)(fs_ratio >> 32);
        sASRCCtrl[ui].uiFsRatio_lo		= (unsigned int)fs_ratio;
		if(ASRC_update_fs_ratio(&sASRCCtrl[ui]) != ASRC_NO_ERROR)
		{
			printf("Error at ASRC update fs ratio");
			assert(0);
		}
        // Set input and output data pointers
		sASRCCtrl[ui].piIn			= piIn + ui;
		sASRCCtrl[ui].piOut			= piOut + ui;
	}

    // Process synchronous part (F1 + F2)
    // ==================================
    for(ui = 0; ui < g_num_io_channels; ui++)
        // Note: this is block based similar to SSRC, output will be on stack
        // and there will be sASRCCtrl[0].uiNSyncSamples samples per channel produced
        if(ASRC_proc_F1_F2(&sASRCCtrl[ui]) != ASRC_NO_ERROR)
        {
            printf("Error at ASRC F1 F2 process");
            assert(0);
        }


    // Run the asynchronous part (F3)
    // ==============================
    // Clear number of output samples (note that this sample counter would actually not be needed if all was sample by sampe)
    for(ui = 0; ui < g_num_io_channels; ui++)
        sASRCCtrl[ui].uiNASRCOutSamples = 0;

    uiSplCntr = 0; // This is actually only used because of the bizarre mix of block and sample based processing

    // Driven by samples produced during the synchronous phase
    for(ui = 0; ui < sASRCCtrl[0].uiNSyncSamples; ui++)
    {
        // Push new samples into F3 delay line (input from stack) for each new "synchronous" sample (i.e. output of F1, respectively F2)
        for(uj = 0; uj < g_num_io_channels; uj++)
            if(ASRC_proc_F3_in_spl(&sASRCCtrl[uj], sASRCCtrl[uj].piStack[ui]) != ASRC_NO_ERROR)
            {
                printf("Error at ASRC F3 in sample process");
                assert(0);
            }

        // Run macc loop for F3
        // Check if a new output sample needs to be produced
        // Note that this will also update the adaptive filter coefficients
        // These must be computed for one channel only and reused in the macc loop of other channels
        while(ASRC_proc_F3_time(&sASRCCtrl[0]) == ASRC_NO_ERROR)
        {
            // Apply filter F3 with just computed adaptive coefficients
            for(uj = 0; uj < g_num_io_channels; uj++)
                if(ASRC_proc_F3_macc(&sASRCCtrl[uj], sASRCCtrl[uj].piOut + g_num_io_channels * uiSplCntr) != ASRC_NO_ERROR)
                {
                    printf("Error at ASRC F3 in sample process");
                    assert(0);
                }

            uiSplCntr++; // This is actually only used because of the bizarre mix of block and sample based processing
        }
    }


	// Process dither part
	// ===================
	// We are back to block based processing. This is where the number of ASRC output samples is required again
	// (would not be used if sample by sample based (on output samples))
	for(ui = 0; ui < g_num_io_channels; ui++)
	{
		// Note: this is block based similar to SSRC
		if(ASRC_proc_dither(&sASRCCtrl[ui]) != ASRC_NO_ERROR)
		{
			printf("Error at ASRC F1 F2 process");
			assert(0);
		}
	}

    unsigned n_samps_out = sASRCCtrl[0].uiNASRCOutSamples;
    return n_samps_out;

}
