// Copyright 2016-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef _SRC_MRHF_ASRC_CHECKS_H_
#define _SRC_MRHF_ASRC_CHECKS_H_

#include "src.h"

//Compile time checks on SRC configuration defines
#if (ASRC_N_CHANNELS % ASRC_N_INSTANCES != 0)
#error "Each core must process an integer number of channels. Check ASRC_N_CHANNELS & ASRC_N_INSTANCES."
#endif
#if (ASRC_N_CHANNELS < ASRC_N_INSTANCES)
#error "Each instance (logical core) must process at least one channel. Check ASRC_N_CHANNELS & ASRC_N_INSTANCES."
#endif
#if (ASRC_N_INSTANCES < 1)
#error "Number of instances (logical cores) must be at least 1. Check ASRC_N_INSTANCES."
#endif
#if (ASRC_N_CHANNELS < 1)
#error "Number of audio channels must be at least 1. Check ASRC_N_CHANNELS."
#endif
#if (ASRC_N_IN_SAMPLES < 4)
#error "Number of input audio samples in each block must be at least 4. Check ASRC_N_IN_SAMPLES."
#endif
#if ((ASRC_N_IN_SAMPLES & (ASRC_N_IN_SAMPLES - 1)) != 0)
#error "Number of input audio samples in each block must be a power of two. Check ASRC_N_IN_SAMPLES."
#endif
#if (ASRC_N_OUT_IN_RATIO_MAX < 5)
#warning "SRC buffering configured so that fs out <= 4 * fs in"
#endif
#if (ASRC_N_OUT_IN_RATIO_MAX < 3)
#warning "SRC buffering configured so that fs out <= 2 * fs in"
#endif
#if (ASRC_N_OUT_IN_RATIO_MAX < 1)
#warning "SRC buffering configured so that fs out <= fs in"
#endif

#endif // _SRC_MRHF_ASRC_CHECKS_H_
