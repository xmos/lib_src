// General includes
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <timer.h>
#include "debug_print.h"

// SSRC includes
#include "src.h"

static void ssrc_error(int code)
{
    debug_printf("SSRC_proc Error code %d\n", code);
    delay_milliseconds(1);
	_Exit(code);
}


void ssrc_init(fs_code_t sr_in, fs_code_t sr_out, SSRCCtrl_t *sSSRCCtrl,
        const unsigned n_channels_per_instance, const unsigned n_in_samples, const unsigned dither_on_off)
{
    SSRCReturnCodes_t ret_code;
    int ui;
    printf("n_channels_per_instance=%d, n_in_samples=%d\n", n_channels_per_instance, n_in_samples);

    for(ui = 0; ui < n_channels_per_instance; ui++)
    {
        // Set number of channels per instance
        sSSRCCtrl[ui].uiNchannels               = n_channels_per_instance;

        // Set number of samples
        sSSRCCtrl[ui].uiNInSamples              = n_in_samples;

        // Set dither flag and random seeds
        sSSRCCtrl[ui].uiDitherOnOff             = dither_on_off;
        sSSRCCtrl[ui].uiRndSeedInit             = 12345 * ui;   //Some randomish numbers. Value not critical

        // Set the sample rate codes
        sSSRCCtrl[ui].eInFs                     = (int)sr_in;
        sSSRCCtrl[ui].eOutFs                    = (int)sr_out;

        // Init SSRC instances
        ret_code = SSRC_init(&sSSRCCtrl[ui]) ;
        if(ret_code != SSRC_NO_ERROR) ssrc_error(ret_code);
    }
}

unsigned ssrc_process(int in_buff[], int out_buff[], SSRCCtrl_t *sSSRCCtrl){

    int ui;
    unsigned n_samps_out;

    // Get the number of channels per instance from first channel
    const unsigned n_channels_per_instance = sSSRCCtrl[0].uiNchannels;

    for(ui = 0; ui < n_channels_per_instance; ui++)
    {
        sSSRCCtrl[ui].piIn = in_buff + ui;
        sSSRCCtrl[ui].piOut = out_buff + ui;
        if(SSRC_proc(&sSSRCCtrl[ui]) != SSRC_NO_ERROR) ssrc_error(0);
        n_samps_out = (*sSSRCCtrl[ui].puiNOutSamples);
    }
    return n_samps_out;
}
