// Copyright (c) 2016, XMOS Ltd, All rights reserved

#ifndef __ssrc_checks_h__
#define __ssrc_checks_h__

#include "ssrc.h"

//Compile time checks on SRC configuration defines 
#if (SSRC_N_CHANNELS % SSRC_N_INSTANCES != 0)
#error "Each core must process an integer number of channels. Check SSRC_N_CHANNELS & SSRC_N_INSTANCES."
#endif
#if (SSRC_N_CHANNELS < SSRC_N_INSTANCES)
#error "Each instance (logical core) must process at least one channel. Check SSRC_N_CHANNELS & SSRC_N_INSTANCES."
#endif
#if (SSRC_N_INSTANCES < 1)
#error "Number of instances (logical cores) must be at least 1. Check SSRC_N_INSTANCES."
#endif
#if (SSRC_N_CHANNELS < 1)
#error "Number of audio channels must be at least 1. Check SSRC_N_CHANNELS."
#endif
#if (SSRC_N_IN_SAMPLES < 4)
#error "Number of input audio samples in each block must be at least 4. Check SSRC_N_IN_SAMPLES."
#endif
#if ((SSRC_N_IN_SAMPLES & (SSRC_N_IN_SAMPLES - 1)) != 0)
#error "Number of input audio samples in each block must be a power of two. Check SSRC_N_IN_SAMPLES."
#endif
#if (SSRC_N_OUT_IN_RATIO_MAX < 5)
#warning "SRC buffering configured so that fs out <= 4 * fs in"
#endif
#if (SSRC_N_OUT_IN_RATIO_MAX < 3)
#warning "SRC buffering configured so that fs out <= 2 * fs in"
#endif
#if (SSRC_N_OUT_IN_RATIO_MAX < 1)
#warning "SRC buffering configured so that fs out <= fs in"
#endif

#endif
