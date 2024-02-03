// Copyright 2016-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// General includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <platform.h>
#include <xscope.h>
#include <assert.h>
#include <string.h>

#if TEST_WAV_XSCOPE
#include "xscope_io_device.h"
#endif

// ASRC includes
#include "src.h"

#define     ASRC_N_CHANNELS                  2  //Total number of audio channels to be processed by SRC (minimum 1)
#define     ASRC_N_INSTANCES                 1  //Number of instances (each usuall run a logical core) used to process audio (minimum 1)
#define     ASRC_CHANNELS_PER_INSTANCE       (ASRC_N_CHANNELS/ASRC_N_INSTANCES)
                                                //Calcualted number of audio channels processed by each core
#define     ASRC_N_IN_SAMPLES                4  //Number of samples per channel in each block passed into SRC each call
                                                //Must be a power of 2 and minimum value is 4 (due to two /2 decimation stages)
#define     ASRC_N_OUT_IN_RATIO_MAX          5  //Max ratio between samples out:in per processing step (44.1->192 is worst case)
#define     ASRC_DITHER_SETTING              OFF

#include "src_mrhf_asrc_checks.h"               //Do some checking on the defines above

//Global deviation ratio from nominal sample rate in/out ratio. Initialise to invalid.
double fFsRatioDeviation = (double)-1.0000;

extern void parse_args();
extern void parse_args();
extern void dsp_slave(chanend c_dsp);
extern void dsp_mgr(chanend c_dsp[], double fFsRatioDeviation);

int main()
{
    chan xscope_chan;
    chan c_dsp[ASRC_N_INSTANCES];
    par
    {
#if TEST_WAV_XSCOPE
        xscope_host_data(xscope_chan);
#endif
        on tile[1]: {
            parse_args();
            par {
                    par (unsigned i=0; i<ASRC_N_INSTANCES; i++)
                    {
                        dsp_slave(c_dsp[i]);
                    }
                    {
                    #if TEST_WAV_XSCOPE
                        xscope_io_init(xscope_chan);
                    #endif
                        dsp_mgr(c_dsp, fFsRatioDeviation);
                    }
            }
        }

    }
    return 0;
}
