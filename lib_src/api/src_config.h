// Copyright (c) 2016, XMOS Ltd, All rights reserved

#ifndef __src_cfg_h__
#define __src_cfg_h__

#include "src.h"


//General SRC configuration defines. Both SSRC and ASRC configurations are dervied from these
#define     SRC_N_CHANNELS                  2  //Total number of audio channels to be processed by SRC (minimum 1)

#define     SRC_N_CORES			            2  //Number of instances (each run a logical core) used to process audio (minimum 1)
#define     SRC_CHANNELS_PER_CORE           (SRC_N_CHANNELS/SRC_N_CORES)
                                               //Calcualted number of audio channels processed by each core
#define     SRC_N_IN_SAMPLES                16  //Number of samples per channel in each block passed into SRC each call
                                               //Must be a power of 2 and minimum value is 4 (due to two /2 decimation stages)
#define     SRC_N_OUT_IN_RATIO_MAX          5  //Max ratio between samples out:in per processing step (44.1->192 is worst case)
#define     SRC_DITHER_ON                   1
#define     SRC_DITHER_OFF                  0
#define     SRC_DITHER_SETTING              SRC_DITHER_OFF
                                               //Enables or disables quantisation of output with dithering to 24b 

//SSRC defines
#define     SSRC_N_CHANNELS                 SRC_N_CHANNELS
#define     SSRC_N_CORES                    SRC_N_CORES
#define     SSRC_CHANNELS_PER_CORE          SRC_CHANNELS_PER_CORE
#define     SSRC_N_IN_SAMPLES               SRC_N_IN_SAMPLES
#define     SSRC_N_OUT_IN_RATIO_MAX         SRC_N_OUT_IN_RATIO_MAX
#define     SSRC_DITHER_SETTING             SRC_DITHER_SETTING


//ASRC defines
#define     ASRC_N_CHANNELS                 SRC_N_CHANNELS
#define     ASRC_N_CORES                    SRC_N_CORES
#define     ASRC_CHANNELS_PER_CORE          SRC_CHANNELS_PER_CORE
#define     ASRC_N_IN_SAMPLES               SRC_N_IN_SAMPLES
#define     ASRC_N_OUT_IN_RATIO_MAX         SRC_N_OUT_IN_RATIO_MAX
#define     ASRC_DITHER_SETTING             SRC_DITHER_SETTING
#define     ASRC_NOMINAL_FS_SCALE           (268435456)      //Nominal Fs Ratio scale value in 4.28 format


//Compile time checks on SRC configuration defines 
#if (SRC_N_CHANNELS % SRC_N_CORES != 0) 
#error "Each core must process an integer number of channels. Check SRC_N_CHANNELS & SRC_N_CORES."
#endif
#if (SRC_N_CHANNELS < SRC_N_CORES) 
#error "Each instance (logical core) must process at least one channel. Check SRC_N_CHANNELS & SRC_N_CORES."
#endif
#if (SRC_N_CORES < 1)
#error "Number of instances (logical cores) must be at least 1. Check SRC_N_CORES."
#endif
#if (SRC_N_CHANNELS < 1)
#error "Number of audio channels must be at least 1. Check SRC_N_CHANNELS."
#endif
#if (SRC_N_IN_SAMPLES < 4)
#error "Number of input audio samples in each block must be at least 4. Check SRC_N_IN_SAMPLES."
#endif
#if ((SRC_N_IN_SAMPLES & (SRC_N_IN_SAMPLES - 1)) != 0)
#error "Number of input audio samples in each block must be a power of two. Check SRC_N_IN_SAMPLES."
#endif
#if (SRC_N_OUT_IN_RATIO_MAX < 5)
#warning "SRC buffering configured so that fs out <= 4 * fs in"
#endif
#if (SRC_N_OUT_IN_RATIO_MAX < 3)
#warning "SRC buffering configured so that fs out <= 2 * fs in"
#endif
#if (SRC_N_OUT_IN_RATIO_MAX < 1)
#warning "SRC buffering configured so that fs out <= fs in"
#endif

#endif //__src_cfg_h__
