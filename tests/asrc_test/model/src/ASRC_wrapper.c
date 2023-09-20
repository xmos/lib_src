// General includes
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <assert.h>

// ASRC includes
#include "ASRC.h"

#define			N_IN_SAMPLES_MAX								(1024)
#define         MAX_ASRC_N_IO_CHANNELS                          (2)
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

uint64_t wrapper_asrc_init(
        unsigned uiInFs,
        unsigned uiOutFs,
        unsigned uiNInSamples,
        unsigned num_io_channels,
        unsigned num_channels_per_asrc_instance,
		unsigned dither_on_off,
		unsigned *rand_seed)
{
    assert(num_io_channels <= MAX_ASRC_N_IO_CHANNELS);

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
