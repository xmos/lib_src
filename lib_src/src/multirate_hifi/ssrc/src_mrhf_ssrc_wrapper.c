// Copyright 2016-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
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
    delay_milliseconds(1); //Allow xscope to display message before quit
    _Exit(code);
}


void ssrc_init(const fs_code_t sr_in, const fs_code_t sr_out, ssrc_ctrl_t *ssrc_ctrl,
        const unsigned n_channels_per_instance, const unsigned n_in_samples, const dither_flag_t dither_on_off)
{
    SSRCReturnCodes_t ret_code;
    unsigned ui;

    //Check to see if n_channels_per_instance, n_in_samples are reasonable
    if ((n_in_samples & 0x1) || (n_in_samples < 4)) ssrc_error(100);
    if (n_channels_per_instance < 1) ssrc_error(101);

    for(ui = 0; ui < n_channels_per_instance; ui++)
    {
        // Set number of channels per instance
        ssrc_ctrl[ui].uiNchannels               = n_channels_per_instance;

        // Set number of samples
        ssrc_ctrl[ui].uiNInSamples              = n_in_samples;

        // Set dither flag and random seeds
        ssrc_ctrl[ui].uiDitherOnOff             = dither_on_off;
        ssrc_ctrl[ui].uiRndSeedInit             = 12345 * ui;   //Some randomish numbers. Value not critical

        // Set the sample rate codes
        ssrc_ctrl[ui].eInFs                     = (int)sr_in;
        ssrc_ctrl[ui].eOutFs                    = (int)sr_out;

        // Init SSRC instances
        ret_code = SSRC_init(&ssrc_ctrl[ui]) ;
        if(ret_code != SSRC_NO_ERROR) ssrc_error(ret_code);
    }
}

unsigned ssrc_process(int in_buff[], int out_buff[], ssrc_ctrl_t *ssrc_ctrl){

    unsigned ui;
    unsigned n_samps_out;

    // Get the number of channels per instance from first channel
    const unsigned n_channels_per_instance = ssrc_ctrl[0].uiNchannels;

    for(ui = 0; ui < n_channels_per_instance; ui++)
    {
        ssrc_ctrl[ui].piIn = in_buff + ui;
        ssrc_ctrl[ui].piOut = out_buff + ui;
        if(SSRC_proc(&ssrc_ctrl[ui]) != SSRC_NO_ERROR) ssrc_error(0);
        n_samps_out = (*ssrc_ctrl[ui].puiNOutSamples);
    }
    return n_samps_out;
}
