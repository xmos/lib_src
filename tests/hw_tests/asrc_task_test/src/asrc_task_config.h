// Copyright 2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#define     MAX_ASRC_CHANNELS_TOTAL             8 // Used for buffer sizing and FIFO sizing (static)
#define     MAX_ASRC_THREADS                    4 // Sets upper limit of worker threads for ASRC task
#define     SRC_N_IN_SAMPLES                    4 // Number of samples per channel in each block passed into SRC each call
                                                  // Must be a power of 2 and minimum value is 4 (due to two /2 decimation stages)
                                                  // Lower improves latency and memory usage but costs MIPS
#define     SRC_N_OUT_IN_RATIO_MAX              5 // Max ratio between samples out:in per processing step (44.1->192 is worst case)
#define     SRC_DITHER_SETTING                  0 // Enables or disables quantisation of output with dithering to 24b